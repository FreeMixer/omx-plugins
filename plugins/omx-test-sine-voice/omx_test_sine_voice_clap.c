// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * omx_test_sine_voice_clap.c — omx test-sine-voice, a TEST instrument (role test: linked, created by
 * id, never listed or installed). A host's own tests play it and predict every output sample.
 *
 * One voice per held note. A voice started at frame t0 by a note of key `key` and velocity `v`
 * (0..1; a MIDI velocity is v/127) writes, at frame t0 + k,
 *
 *     y(k) = GAIN * v * env(k) * sin(2 pi f k / rate),   f = A4_HZ * 2^((key - 69) / 12)
 *     env(k) = min(1, k / A)                             before the note off, A = round(ATTACK_MS * rate / 1000)
 *     env(k1 + j) = env(k1) * (1 - j / R)                after a note off at k1, R = round(RELEASE_MS * rate / 1000)
 *
 * and is silent from j = R on, when it ends and reports CLAP_EVENT_NOTE_END at that frame. Both
 * output channels carry the sum of the sounding voices. A choke ends a voice at once. At most VOICES
 * sound at once; a note on past that takes the oldest voice. GAIN, ATTACK_MS, RELEASE_MS, VOICES and
 * A4_HZ are the declaration's constants (generated/omx_test_sine_voice_decl.h).
 *
 * Ports: one note input (CLAP and MIDI dialects), no audio input, one stereo audio output.
 * Extensions: audio-ports, note-ports, voice-info. Events are taken at their frame. The process path
 * allocates nothing, takes no lock and makes no system call; the voices are allocated with the plugin.
 */
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <clap/clap.h>

#include "omx_test_sine_voice_decl.h"

#define VOICES ((uint32_t)OMX_TEST_SINE_VOICE_VOICES)

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
} Voice;

typedef struct {
  clap_plugin_t plugin;
  const clap_host_t *host;
  double rate;
  uint64_t attack, release; /* frames */
  uint64_t ages;
  Voice v[VOICES];
} G;

static double g_env(const G *g, const Voice *v) {
  if (!v->released) return v->k >= g->attack ? 1.0 : (double)v->k / (double)g->attack;
  const uint64_t j = v->k - v->k1;
  return j >= g->release ? 0.0 : v->e0 * (1.0 - (double)j / (double)g->release);
}

static void g_end(G *g, Voice *v, uint32_t frame, const clap_output_events_t *out) {
  (void)g;
  v->on = false;
  clap_event_note_t e = {{sizeof e, frame, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_NOTE_END, 0}, v->note_id, v->port,
                         v->channel, v->key, 0.0};
  out->try_push(out, &e.header);
}

static void g_note_on(G *g, int32_t id, int16_t port, int16_t ch, int16_t key, double vel, uint32_t frame,
                      const clap_output_events_t *out) {
  if (key < 0 || key > 127 || vel <= 0.0) return;
  Voice *v = NULL;
  for (uint32_t i = 0; i < VOICES && !v; i++)
    if (!g->v[i].on) v = &g->v[i];
  if (!v) { /* every voice sounds: take the oldest */
    v = &g->v[0];
    for (uint32_t i = 1; i < VOICES; i++)
      if (g->v[i].age < v->age) v = &g->v[i];
    g_end(g, v, frame, out);
  }
  v->on = true;
  v->released = false;
  v->key = key, v->channel = ch, v->port = port, v->note_id = id;
  v->age = g->ages++;
  v->level = OMX_TEST_SINE_VOICE_GAIN * (vel > 1.0 ? 1.0 : vel);
  v->inc = OMX_TEST_SINE_VOICE_A4_HZ * pow(2.0, (key - 69) / 12.0) / g->rate;
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

static void g_choke(G *g, int32_t id, int16_t ch, int16_t key, uint32_t frame, const clap_output_events_t *out) {
  for (uint32_t i = 0; i < VOICES; i++)
    if (g_match(&g->v[i], id, ch, key)) g_end(g, &g->v[i], frame, out);
}

static void g_event(G *g, const clap_event_header_t *h, const clap_output_events_t *out) {
  if (h->space_id != CLAP_CORE_EVENT_SPACE_ID) return;
  if (h->type == CLAP_EVENT_NOTE_ON || h->type == CLAP_EVENT_NOTE_OFF || h->type == CLAP_EVENT_NOTE_CHOKE) {
    const clap_event_note_t *n = (const clap_event_note_t *)h;
    if (h->type == CLAP_EVENT_NOTE_ON && n->velocity > 0.0)
      g_note_on(g, n->note_id, n->port_index, n->channel, n->key, n->velocity, h->time, out);
    else if (h->type == CLAP_EVENT_NOTE_CHOKE)
      g_choke(g, n->note_id, n->channel, n->key, h->time, out);
    else
      g_note_off(g, n->note_id, n->channel, n->key);
  } else if (h->type == CLAP_EVENT_MIDI) {
    const clap_event_midi_t *m = (const clap_event_midi_t *)h;
    const uint8_t kind = m->data[0] & 0xF0, ch = m->data[0] & 0x0F, key = m->data[1] & 0x7F, vel = m->data[2] & 0x7F;
    if (kind == 0x90 && vel > 0)
      g_note_on(g, -1, (int16_t)m->port_index, ch, key, vel / 127.0, h->time, out);
    else if (kind == 0x80 || kind == 0x90)
      g_note_off(g, -1, ch, key);
  }
}

/* Frames [from, to) of every sounding voice into both legs. A voice whose release has run its R frames
 * ends at the first frame it would have written (frame 0 of the next block when the release ran out
 * on a block's last frame). */
static void g_render(G *g, float *l, float *r, uint32_t from, uint32_t to, const clap_output_events_t *out) {
  for (uint32_t f = from; f < to; f++) l[f] = 0.0f;
  for (uint32_t i = 0; i < VOICES; i++) {
    Voice *v = &g->v[i];
    for (uint32_t f = from; f < to && v->on; f++) {
      if (v->released && v->k - v->k1 >= g->release) {
        g_end(g, v, f, out);
        break;
      }
      const double ph = fmod(v->inc * (double)v->k, 1.0);
      l[f] += (float)(v->level * g_env(g, v) * sin(2.0 * M_PI * ph));
      v->k++;
    }
  }
  if (r != l) memcpy(r + from, l + from, (to - from) * sizeof *l);
}

static clap_process_status g_process(const clap_plugin_t *p, const clap_process_t *proc) {
  G *g = (G *)p->plugin_data;
  if (proc->audio_outputs_count < 1 || proc->audio_outputs[0].channel_count < 1) return CLAP_PROCESS_ERROR;
  float *l = proc->audio_outputs[0].data32[0];
  float *r = proc->audio_outputs[0].channel_count > 1 ? proc->audio_outputs[0].data32[1] : l;
  const uint32_t n = proc->frames_count;
  const uint32_t ne = proc->in_events ? proc->in_events->size(proc->in_events) : 0;
  uint32_t at = 0;
  for (uint32_t i = 0; i < ne; i++) {
    const clap_event_header_t *h = proc->in_events->get(proc->in_events, i);
    const uint32_t t = h->time < n ? h->time : n;
    if (t > at) g_render(g, l, r, at, t, proc->out_events), at = t;
    g_event(g, h, proc->out_events);
  }
  if (at < n) g_render(g, l, r, at, n, proc->out_events);
  return CLAP_PROCESS_CONTINUE;
}

/* ---- lifecycle ---------------------------------------------------------------------------- */

static bool g_init(const clap_plugin_t *p) {
  (void)p;
  return true;
}

static void g_destroy(const clap_plugin_t *p) { free(p->plugin_data); }

static bool g_activate(const clap_plugin_t *p, double rate, uint32_t min, uint32_t max) {
  (void)min, (void)max;
  G *g = (G *)p->plugin_data;
  if (!(rate > 0.0)) return false;
  g->rate = rate;
  g->attack = (uint64_t)llround(OMX_TEST_SINE_VOICE_ATTACK_MS * rate / 1000.0);
  g->release = (uint64_t)llround(OMX_TEST_SINE_VOICE_RELEASE_MS * rate / 1000.0);
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

static const void *g_get_extension(const clap_plugin_t *p, const char *id) {
  (void)p;
  if (strcmp(id, CLAP_EXT_AUDIO_PORTS) == 0) return &G_AUDIO_PORTS;
  if (strcmp(id, CLAP_EXT_NOTE_PORTS) == 0) return &G_NOTE_PORTS;
  if (strcmp(id, CLAP_EXT_VOICE_INFO) == 0) return &G_VOICE_INFO;
  return NULL;
}

static void g_on_main_thread(const clap_plugin_t *p) { (void)p; }

/* ---- descriptor, factory, entry --------------------------------------------------------- */

static const char *G_FEATURES[] = {OMX_TEST_SINE_VOICE_CLAP_FEATURES, NULL};

static const clap_plugin_descriptor_t G_DESC = {
    CLAP_VERSION_INIT, OMX_TEST_SINE_VOICE_CLAP_ID, OMX_TEST_SINE_VOICE_NAME, OMX_TEST_SINE_VOICE_VENDOR,
    OMX_TEST_SINE_VOICE_URL, "", "", OMX_TEST_SINE_VOICE_VERSION, OMX_TEST_SINE_VOICE_DESCRIPTION, G_FEATURES};

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
const clap_plugin_entry_t omx_clap_entry_test_sine_voice = {CLAP_VERSION_INIT, e_init, e_deinit, e_factory};

#ifndef OMX_CLAP_STATIC
/* A standalone .clap for the oracle: the same entry under CLAP's exported name. */
CLAP_EXPORT const clap_plugin_entry_t clap_entry = {CLAP_VERSION_INIT, e_init, e_deinit, e_factory};
#endif
