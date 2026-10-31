// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * omx_test_pool_voice_clap.c — omx test-pool-voice, a TEST instrument (role test: linked, created by
 * id, never listed or installed). It exists so a host's CLAP thread pool can be measured with a plugin
 * that asks for it.
 *
 * One voice per held note, rendered as one task of clap_host_thread_pool.request_exec. A voice started
 * by a note of key `key` and velocity `v` (0..1; a MIDI velocity is v/127) writes, k frames after the
 * block that took the note on,
 *
 *     y(k) = GAIN * v * env(k) * sin(2 pi f k / rate),   f = A4_HZ * 2^((key - 69) / 12)
 *     env(k) = min(1, k / A)                             before the note off, A = round(ATTACK_MS * rate / 1000)
 *     env(k1 + j) = env(k1) * (1 - j / R)                after a note off at k1, R = round(RELEASE_MS * rate / 1000)
 *
 * and is silent from j = R on, when it ends and reports CLAP_EVENT_NOTE_END at that frame. Events are
 * taken at the start of their block (their frame is ignored), so a voice never starts mid-block.
 *
 * The load: every frame of every voice also runs LOAD_ITERS steps of a dependent multiply-add chain into
 * a volatile sink. It changes no output sample; it costs the same CPU on every thread, so a block with
 * N sounding voices is N equal slices of work.
 *
 * Both output channels carry the voices summed in voice order 0..VOICES-1 into float, after all the
 * tasks are done, so the output is the same bits with the pool on, with the pool off, or with a host
 * that offers no pool: each voice writes only its own buffer, whichever thread runs it.
 *
 * With two or more sounding voices and a host that offers clap.thread-pool, process() calls
 * request_exec(host, sounding voices) once; task i is the i-th sounding voice. A host with no pool, or
 * whose request_exec returns false, gets the voices rendered one after another on the calling thread.
 */
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <clap/clap.h>

#include "omx_test_pool_voice_decl.h"

#define VOICES ((uint32_t)OMX_TEST_POOL_VOICE_VOICES)
#define MAXFRAMES 4096u

typedef struct {
  bool on;        /* sounding */
  bool released;  /* after its note off */
  int16_t key, channel, port;
  int32_t note_id;
  uint64_t age;   /* note-on order: the oldest is taken when every voice sounds */
  double level;   /* GAIN * velocity */
  double inc;     /* f / rate, cycles per frame */
  uint64_t k;     /* frames since the note on */
  uint64_t k1;    /* frame of the note off */
  double e0;      /* env at the note off */
  uint32_t end;   /* frame of this block where the voice ended, or UINT32_MAX */
  volatile double sink;
  float buf[MAXFRAMES];
} Voice;

typedef struct {
  clap_plugin_t plugin;
  const clap_host_t *host;
  const clap_host_thread_pool_t *pool;
  double rate;
  uint64_t attack, release; /* frames */
  uint64_t ages;
  uint32_t frames;          /* frames of the block being rendered */
  uint32_t act[VOICES];     /* the sounding voices of this block: task i renders voice act[i] */
  Voice v[VOICES];
} G;

static double g_env(const G *g, const Voice *v) {
  if (!v->released) return v->k >= g->attack ? 1.0 : (double)v->k / (double)g->attack;
  const uint64_t j = v->k - v->k1;
  return j >= g->release ? 0.0 : v->e0 * (1.0 - (double)j / (double)g->release);
}

static void g_end(Voice *v, uint32_t frame, const clap_output_events_t *out) {
  v->on = false;
  clap_event_note_t e = {{sizeof e, frame, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_NOTE_END, 0}, v->note_id, v->port,
                         v->channel, v->key, 0.0};
  out->try_push(out, &e.header);
}

static void g_note_on(G *g, int32_t id, int16_t port, int16_t ch, int16_t key, double vel,
                      const clap_output_events_t *out) {
  if (key < 0 || key > 127 || vel <= 0.0) return;
  Voice *v = NULL;
  for (uint32_t i = 0; i < VOICES && !v; i++)
    if (!g->v[i].on) v = &g->v[i];
  if (!v) { /* every voice sounds: take the oldest */
    v = &g->v[0];
    for (uint32_t i = 1; i < VOICES; i++)
      if (g->v[i].age < v->age) v = &g->v[i];
    g_end(v, 0, out);
  }
  v->on = true;
  v->released = false;
  v->key = key, v->channel = ch, v->port = port, v->note_id = id;
  v->age = g->ages++;
  v->level = OMX_TEST_POOL_VOICE_GAIN * (vel > 1.0 ? 1.0 : vel);
  v->inc = OMX_TEST_POOL_VOICE_A4_HZ * pow(2.0, (key - 69) / 12.0) / g->rate;
  v->k = 0;
}

/* -1 in key, channel or note id matches every voice (CLAP's wildcard). */
static bool g_match(const Voice *v, int32_t id, int16_t ch, int16_t key) {
  return v->on && (id < 0 || v->note_id == id) && (ch < 0 || v->channel == ch) && (key < 0 || v->key == key);
}

static void g_note_off(G *g, int32_t id, int16_t ch, int16_t key) {
  for (uint32_t i = 0; i < VOICES; i++) {
    Voice *v = &g->v[i];
    if (!g_match(v, id, ch, key) || v->released) continue;
    v->e0 = g_env(g, v);
    v->released = true;
    v->k1 = v->k;
  }
}

static void g_event(G *g, const clap_event_header_t *h, const clap_output_events_t *out) {
  if (h->space_id != CLAP_CORE_EVENT_SPACE_ID) return;
  if (h->type == CLAP_EVENT_NOTE_ON || h->type == CLAP_EVENT_NOTE_OFF || h->type == CLAP_EVENT_NOTE_CHOKE) {
    const clap_event_note_t *n = (const clap_event_note_t *)h;
    if (h->type == CLAP_EVENT_NOTE_ON && n->velocity > 0.0)
      g_note_on(g, n->note_id, n->port_index, n->channel, n->key, n->velocity, out);
    else if (h->type == CLAP_EVENT_NOTE_CHOKE) {
      for (uint32_t i = 0; i < VOICES; i++)
        if (g_match(&g->v[i], n->note_id, n->channel, n->key)) g_end(&g->v[i], 0, out);
    } else
      g_note_off(g, n->note_id, n->channel, n->key);
  } else if (h->type == CLAP_EVENT_MIDI) {
    const clap_event_midi_t *m = (const clap_event_midi_t *)h;
    const uint8_t kind = m->data[0] & 0xF0, ch = m->data[0] & 0x0F, key = m->data[1] & 0x7F, vel = m->data[2] & 0x7F;
    if (kind == 0x90 && vel > 0)
      g_note_on(g, -1, (int16_t)m->port_index, ch, key, vel / 127.0, out);
    else if (kind == 0x80 || kind == 0x90)
      g_note_off(g, -1, ch, key);
  }
}

/* One voice's whole block into its own buffer, and its load. This is the pool's task: it touches the
 * voice and nothing else. */
static void g_render_voice(G *g, Voice *v) {
  const uint32_t n = g->frames;
  double x = 1.0;
  v->end = UINT32_MAX;
  for (uint32_t f = 0; f < n; f++) {
    if (!v->on || (v->released && v->k - v->k1 >= g->release)) {
      if (v->on) v->end = f;
      v->on = false;
      v->buf[f] = 0.0f;
      continue;
    }
    const double ph = fmod(v->inc * (double)v->k, 1.0);
    v->buf[f] = (float)(v->level * g_env(g, v) * sin(2.0 * M_PI * ph));
    v->k++;
    for (uint32_t i = 0; i < (uint32_t)OMX_TEST_POOL_VOICE_LOAD_ITERS; i++) x = x * 0.9999999 + 1e-7 * (double)(f + 1);
  }
  v->sink = x;
}

static void g_task(const clap_plugin_t *p, uint32_t task) {
  G *g = (G *)p->plugin_data;
  if (task < VOICES) g_render_voice(g, &g->v[g->act[task]]);
}

static clap_process_status g_process(const clap_plugin_t *p, const clap_process_t *proc) {
  G *g = (G *)p->plugin_data;
  if (proc->audio_outputs_count < 1 || proc->audio_outputs[0].channel_count < 1) return CLAP_PROCESS_ERROR;
  float *l = proc->audio_outputs[0].data32[0];
  float *r = proc->audio_outputs[0].channel_count > 1 ? proc->audio_outputs[0].data32[1] : l;
  const uint32_t n = proc->frames_count;
  if (n > MAXFRAMES) return CLAP_PROCESS_ERROR;
  const uint32_t ne = proc->in_events ? proc->in_events->size(proc->in_events) : 0;
  for (uint32_t i = 0; i < ne; i++) g_event(g, proc->in_events->get(proc->in_events, i), proc->out_events);

  g->frames = n;
  uint32_t nact = 0;
  for (uint32_t i = 0; i < VOICES; i++)
    if (g->v[i].on) g->act[nact++] = i;
  bool done = false;
  if (nact >= 2 && g->pool && g->pool->request_exec) done = g->pool->request_exec(g->host, nact);
  if (!done)
    for (uint32_t i = 0; i < nact; i++) g_render_voice(g, &g->v[g->act[i]]);

  memset(l, 0, n * sizeof *l);
  for (uint32_t a = 0; a < nact; a++) {
    Voice *v = &g->v[g->act[a]];
    for (uint32_t f = 0; f < n; f++) l[f] += v->buf[f];
    if (v->end != UINT32_MAX) {
      clap_event_note_t e = {{sizeof e, v->end, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_NOTE_END, 0}, v->note_id, v->port,
                             v->channel, v->key, 0.0};
      proc->out_events->try_push(proc->out_events, &e.header);
    }
  }
  if (r != l) memcpy(r, l, n * sizeof *l);
  return CLAP_PROCESS_CONTINUE;
}

/* ---- lifecycle ---------------------------------------------------------------------------- */

static bool g_init(const clap_plugin_t *p) {
  G *g = (G *)p->plugin_data;
  g->pool = g->host && g->host->get_extension ? (const clap_host_thread_pool_t *)g->host->get_extension(g->host, CLAP_EXT_THREAD_POOL) : NULL;
  return true;
}

static void g_destroy(const clap_plugin_t *p) { free(p->plugin_data); }

static bool g_activate(const clap_plugin_t *p, double rate, uint32_t min, uint32_t max) {
  (void)min;
  G *g = (G *)p->plugin_data;
  if (!(rate > 0.0) || max > MAXFRAMES) return false;
  g->rate = rate;
  g->attack = (uint64_t)llround(OMX_TEST_POOL_VOICE_ATTACK_MS * rate / 1000.0);
  g->release = (uint64_t)llround(OMX_TEST_POOL_VOICE_RELEASE_MS * rate / 1000.0);
  if (g->attack < 1) g->attack = 1;
  if (g->release < 1) g->release = 1;
  memset(g->v, 0, sizeof g->v);
  return true;
}

static void g_deactivate(const clap_plugin_t *p) { (void)p; }

static bool g_start_processing(const clap_plugin_t *p) {
  (void)p;
  return true;
}

static void g_stop_processing(const clap_plugin_t *p) { (void)p; }

static void g_reset(const clap_plugin_t *p) {
  G *g = (G *)p->plugin_data;
  memset(g->v, 0, sizeof g->v);
}

/* ---- extensions --------------------------------------------------------------------------- */

static uint32_t ap_count(const clap_plugin_t *p, bool in) {
  (void)p;
  return in ? 0 : 1;
}

static bool ap_get(const clap_plugin_t *p, uint32_t i, bool in, clap_audio_port_info_t *info) {
  (void)p;
  if (in || i != 0) return false;
  memset(info, 0, sizeof *info);
  info->id = 0;
  strcpy(info->name, "out");
  info->flags = CLAP_AUDIO_PORT_IS_MAIN;
  info->channel_count = 2;
  info->port_type = CLAP_PORT_STEREO;
  info->in_place_pair = CLAP_INVALID_ID;
  return true;
}

static const clap_plugin_audio_ports_t G_AUDIO_PORTS = {ap_count, ap_get};

static uint32_t np_count(const clap_plugin_t *p, bool in) {
  (void)p;
  return in ? 1 : 0;
}

static bool np_get(const clap_plugin_t *p, uint32_t i, bool in, clap_note_port_info_t *info) {
  (void)p;
  if (!in || i != 0) return false;
  memset(info, 0, sizeof *info);
  info->id = 0;
  info->supported_dialects = CLAP_NOTE_DIALECT_CLAP | CLAP_NOTE_DIALECT_MIDI;
  info->preferred_dialect = CLAP_NOTE_DIALECT_CLAP;
  strcpy(info->name, "notes");
  return true;
}

static const clap_plugin_note_ports_t G_NOTE_PORTS = {np_count, np_get};

static bool vi_get(const clap_plugin_t *p, clap_voice_info_t *info) {
  (void)p;
  info->voice_count = VOICES;
  info->voice_capacity = VOICES;
  info->flags = 0;
  return true;
}

static const clap_plugin_voice_info_t G_VOICE_INFO = {vi_get};
static const clap_plugin_thread_pool_t G_THREAD_POOL = {g_task};

static const void *g_get_extension(const clap_plugin_t *p, const char *id) {
  (void)p;
  if (strcmp(id, CLAP_EXT_AUDIO_PORTS) == 0) return &G_AUDIO_PORTS;
  if (strcmp(id, CLAP_EXT_NOTE_PORTS) == 0) return &G_NOTE_PORTS;
  if (strcmp(id, CLAP_EXT_VOICE_INFO) == 0) return &G_VOICE_INFO;
  if (strcmp(id, CLAP_EXT_THREAD_POOL) == 0) return &G_THREAD_POOL;
  return NULL;
}

static void g_on_main_thread(const clap_plugin_t *p) { (void)p; }

/* ---- descriptor, factory, entry --------------------------------------------------------- */

static const char *G_FEATURES[] = {OMX_TEST_POOL_VOICE_CLAP_FEATURES, NULL};

static const clap_plugin_descriptor_t G_DESC = {
    CLAP_VERSION_INIT, OMX_TEST_POOL_VOICE_CLAP_ID, OMX_TEST_POOL_VOICE_NAME, OMX_TEST_POOL_VOICE_VENDOR,
    OMX_TEST_POOL_VOICE_URL, "", "", OMX_TEST_POOL_VOICE_VERSION, OMX_TEST_POOL_VOICE_DESCRIPTION, G_FEATURES};

static const clap_plugin_t *g_create(const clap_host_t *host) {
  G *g = (G *)calloc(1, sizeof(G));
  if (!g) return NULL;
  g->host = host;
  g->rate = 48000.0;
  g->plugin.desc = &G_DESC;
  g->plugin.plugin_data = g;
  g->plugin.init = g_init;
  g->plugin.destroy = g_destroy;
  g->plugin.activate = g_activate;
  g->plugin.deactivate = g_deactivate;
  g->plugin.start_processing = g_start_processing;
  g->plugin.stop_processing = g_stop_processing;
  g->plugin.reset = g_reset;
  g->plugin.process = g_process;
  g->plugin.get_extension = g_get_extension;
  g->plugin.on_main_thread = g_on_main_thread;
  return &g->plugin;
}

static uint32_t f_count(const clap_plugin_factory_t *f) {
  (void)f;
  return 1;
}

static const clap_plugin_descriptor_t *f_desc(const clap_plugin_factory_t *f, uint32_t i) {
  (void)f;
  return i == 0 ? &G_DESC : NULL;
}

static const clap_plugin_t *f_create(const clap_plugin_factory_t *f, const clap_host_t *host, const char *id) {
  (void)f;
  return id && strcmp(id, G_DESC.id) == 0 ? g_create(host) : NULL;
}

static const clap_plugin_factory_t G_FACTORY = {f_count, f_desc, f_create};

static bool e_init(const char *path) {
  (void)path;
  return true;
}

static void e_deinit(void) {}

static const void *e_factory(const char *id) { return strcmp(id, CLAP_PLUGIN_FACTORY_ID) == 0 ? &G_FACTORY : NULL; }

/* The entry by name, for a host that links the plugin statically and opens it by id without dlopen. */
const clap_plugin_entry_t omx_clap_entry_test_pool_voice = {CLAP_VERSION_INIT, e_init, e_deinit, e_factory};

#ifndef OMX_CLAP_STATIC
/* A standalone .clap for the oracle: the same entry under CLAP's exported name. */
CLAP_EXPORT const clap_plugin_entry_t clap_entry = {CLAP_VERSION_INIT, e_init, e_deinit, e_factory};
#endif
