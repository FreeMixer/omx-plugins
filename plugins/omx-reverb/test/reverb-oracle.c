// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * reverb-oracle.c — omx-reverb's kernel-identity test: both built faces, sample for sample, against
 * omx-dsp's reverb instance face called DIRECTLY on the same blocks, at every declared rate,
 * engaged and bypassed — no tolerance.
 *
 *   reverb-oracle clap <omx-reverb.clap>
 *   reverb-oracle lv2 <bundle-dir> <uri>
 *
 * GENERATED — DO NOT EDIT BY HAND. Produced by tools/gen.mjs from plugins/omx-reverb/omx-reverb.decl.json:
 * the plan moves every declared parameter across its travel between uneven blocks (1 to 3000 frames)
 * and toggles the bypass; then, timed in milliseconds so every rate gets the same time, a full-scale
 * burst and a quiet tail at the defaults (a dynamics kernel engages and releases), and each value of
 * every choice held over its own burst and tail with the other parameters stepped once (a control
 * read only under one value is reached). All of it runs through ONE instance of the face. Parameters are found BY NAME (CLAP) or BY
 * SYMBOL (LV2, through lilv), so a renumbered face cannot pass by accident. Two guards keep the test
 * honest: the reference must move the signal, and a reference with any ONE parameter one step off must
 * differ from the face (the sabotage arm), so a wrong or dead coefficient is seen.
 */
#include <dlfcn.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <clap/clap.h>
#include <lilv/lilv.h>

#include "omx_reverb_params.h"

#include <omxdsp/fx/omx_reverb_instance.h>
#include <omxdsp/omx_denormal.h>

#define NP OMX_REVERB_PARAM_COUNT

/* One block: its length (frames, or ms when ms > 0), the stimulus it carries (0 the moving bursts,
 * 1 a full-scale burst, 2 the quiet tail), the declared parameters (declaration order) and the
 * host's bypass. */
typedef struct {
  uint32_t frames;
  float ms;
  int signal;
  float v[NP];
  int bypass;
} Block;

static const Block PLAN[] = {
    { 64, 0.0f, 0, {100.0f, 1.0f, 0.75f, 0.5f, 0.1f, 35.0f, 0.0f, 20000.0f, 388.0f, 1005.0f, 51.0f, -52.0f, 0.0f}, 0 },
    { 1, 0.0f, 0, {100.0f, 0.0f, 0.7f, 0.9f, 0.6f, 0.0f, 5000.0f, 0.0f, 300.0f, 1801.0f, 300.0f, -40.0f, 1.0f}, 0 },
    { 333, 0.0f, 0, {400.0f, 0.75f, 0.5f, 0.1f, 0.35f, 0.0f, 20000.0f, 15000.0f, 275.0f, 209.0f, 176.0f, -40.0f, 4.0f}, 0 },
    { 512, 0.0f, 0, {0.0f, 0.3f, 0.9f, 0.6f, 1.0f, 25.0f, 0.0f, 20000.0f, 455.0f, 1204.0f, 20.0f, -60.0f, 0.0f}, 0 },
    { 17, 0.0f, 0, {300.0f, 0.5f, 0.1f, 0.35f, 1.0f, 100.0f, 15000.0f, 10000.0f, 95.0f, 707.0f, 20.0f, 0.0f, 3.0f}, 1 },
    { 480, 0.0f, 0, {100.0f, 0.9f, 0.6f, 0.5f, 0.25f, 0.0f, 0.0f, 18000.0f, 320.0f, 120.0f, 126.0f, -80.0f, 0.0f}, 1 },
    { 129, 0.0f, 0, {200.0f, 0.1f, 0.35f, 0.5f, 1.0f, 75.0f, 10000.0f, 2000.0f, 208.0f, 120.0f, 500.0f, -20.0f, 2.0f}, 0 },
    { 1024, 0.0f, 0, {360.0f, 0.6f, 0.7f, 0.25f, 0.0f, 0.0f, 18000.0f, 12000.0f, 300.0f, 508.0f, 1.0f, -40.0f, 4.0f}, 0 },
    { 7, 0.0f, 0, {40.0f, 0.35f, 0.7f, 1.0f, 0.75f, 50.0f, 2000.0f, 7000.0f, 300.0f, 2000.0f, 375.0f, -40.0f, 0.0f}, 0 },
    { 2500, 0.0f, 0, {240.0f, 0.3f, 0.25f, 0.0f, 1.0f, 90.0f, 12000.0f, 20000.0f, 163.0f, 10.0f, 20.0f, -8.0f, 2.0f}, 0 },
    { 600, 0.0f, 0, {140.0f, 0.3f, 1.0f, 0.75f, 0.5f, 10.0f, 7000.0f, 20000.0f, 500.0f, 1503.0f, 251.0f, -72.0f, 1.0f}, 0 },
    { 3000, 0.0f, 0, {100.0f, 0.25f, 0.0f, 0.5f, 0.9f, 60.0f, 0.0f, 5000.0f, 50.0f, 120.0f, 450.0f, -32.0f, 0.0f}, 0 },
    { 0, 10.0f, 1, {100.0f, 0.3f, 0.7f, 0.5f, 1.0f, 0.0f, 0.0f, 20000.0f, 300.0f, 120.0f, 20.0f, -40.0f, 0.0f}, 0 },
    { 0, 440.0f, 2, {100.0f, 0.3f, 0.7f, 0.5f, 1.0f, 0.0f, 0.0f, 20000.0f, 300.0f, 120.0f, 20.0f, -40.0f, 0.0f}, 0 },
    { 0, 10.0f, 1, {100.0f, 0.3f, 0.7f, 0.5f, 1.0f, 0.0f, 0.0f, 20000.0f, 300.0f, 120.0f, 20.0f, -40.0f, 0.0f}, 0 },
    { 0, 440.0f, 2, {100.0f, 0.3f, 0.7f, 0.5f, 1.0f, 0.0f, 0.0f, 20000.0f, 300.0f, 120.0f, 20.0f, -40.0f, 0.0f}, 0 },
    { 0, 440.0f, 2, {300.0f, 0.75f, 0.75f, 0.75f, 0.75f, 75.0f, 15000.0f, 15000.0f, 388.0f, 1503.0f, 375.0f, -20.0f, 0.0f}, 0 },
    { 0, 10.0f, 1, {100.0f, 0.3f, 0.7f, 0.5f, 1.0f, 0.0f, 0.0f, 20000.0f, 300.0f, 120.0f, 20.0f, -40.0f, 1.0f}, 0 },
    { 0, 440.0f, 2, {100.0f, 0.3f, 0.7f, 0.5f, 1.0f, 0.0f, 0.0f, 20000.0f, 300.0f, 120.0f, 20.0f, -40.0f, 1.0f}, 0 },
    { 0, 440.0f, 2, {300.0f, 0.75f, 0.75f, 0.75f, 0.75f, 75.0f, 15000.0f, 15000.0f, 388.0f, 1503.0f, 375.0f, -20.0f, 1.0f}, 0 },
    { 0, 10.0f, 1, {100.0f, 0.3f, 0.7f, 0.5f, 1.0f, 0.0f, 0.0f, 20000.0f, 300.0f, 120.0f, 20.0f, -40.0f, 2.0f}, 0 },
    { 0, 440.0f, 2, {100.0f, 0.3f, 0.7f, 0.5f, 1.0f, 0.0f, 0.0f, 20000.0f, 300.0f, 120.0f, 20.0f, -40.0f, 2.0f}, 0 },
    { 0, 440.0f, 2, {300.0f, 0.75f, 0.75f, 0.75f, 0.75f, 75.0f, 15000.0f, 15000.0f, 388.0f, 1503.0f, 375.0f, -20.0f, 2.0f}, 0 },
    { 0, 10.0f, 1, {100.0f, 0.3f, 0.7f, 0.5f, 1.0f, 0.0f, 0.0f, 20000.0f, 300.0f, 120.0f, 20.0f, -40.0f, 3.0f}, 0 },
    { 0, 440.0f, 2, {100.0f, 0.3f, 0.7f, 0.5f, 1.0f, 0.0f, 0.0f, 20000.0f, 300.0f, 120.0f, 20.0f, -40.0f, 3.0f}, 0 },
    { 0, 440.0f, 2, {300.0f, 0.75f, 0.75f, 0.75f, 0.75f, 75.0f, 15000.0f, 15000.0f, 388.0f, 1503.0f, 375.0f, -20.0f, 3.0f}, 0 },
    { 0, 10.0f, 1, {100.0f, 0.3f, 0.7f, 0.5f, 1.0f, 0.0f, 0.0f, 20000.0f, 300.0f, 120.0f, 20.0f, -40.0f, 4.0f}, 0 },
    { 0, 440.0f, 2, {100.0f, 0.3f, 0.7f, 0.5f, 1.0f, 0.0f, 0.0f, 20000.0f, 300.0f, 120.0f, 20.0f, -40.0f, 4.0f}, 0 },
    { 0, 440.0f, 2, {300.0f, 0.75f, 0.75f, 0.75f, 0.75f, 75.0f, 15000.0f, 15000.0f, 388.0f, 1503.0f, 375.0f, -20.0f, 4.0f}, 0 },
};
#define NBLOCKS (sizeof PLAN / sizeof PLAN[0])

/* One step of each parameter: what the sabotage arm moves it by. */
static const float STEP[NP] = { 1.0f, 0.01f, 0.01f, 0.01f, 0.01f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

static int failures;

static void check(bool ok, const char *face, double sr, const char *what) {
  if (!ok) failures++;
  printf("%s %s %s @ %.0f Hz\n", ok ? "PASS" : "FAIL", face, what, sr);
}

static uint32_t frames_of(const Block *k, double sr) {
  return k->ms > 0.0f ? (uint32_t)ceil((double)k->ms * sr / 1000.0) : k->frames;
}

static uint32_t total_frames(double sr) {
  uint32_t n = 0;
  for (size_t i = 0; i < NBLOCKS; i++) n += frames_of(&PLAN[i], sr);
  return n;
}

static uint32_t longest_block(double sr) {
  uint32_t n = 1;
  for (size_t i = 0; i < NBLOCKS; i++) n = frames_of(&PLAN[i], sr) > n ? frames_of(&PLAN[i], sr) : n;
  return n;
}

/* The moving blocks: bursts of decaying noise on a quiet bed, the legs unequal (onsets, decays and a
 * stereo image). A burst block: full-scale noise. A tail block: noise falling from full scale by
 * 8.7 dB every 10 ms onto a bed at about -60 dBFS, so a level crosses every threshold at its own
 * time and a gate closes under its default threshold. */
static void signal_make(float *l, float *r, uint32_t n, double sr) {
  uint32_t seed = 0x6f6d78u;
  const uint32_t period = (uint32_t)(0.09 * sr);
  uint32_t start = 0, end = 0;
  size_t b = 0;
  for (uint32_t i = 0; i < n; i++) {
    while (i >= end && b < NBLOCKS) start = end, end += frames_of(&PLAN[b++], sr);
    const int sig = PLAN[b - 1].signal;
    seed = seed * 1664525u + 1013904223u;
    const float a = (float)(seed >> 8) / 16777216.0f - 0.5f;
    seed = seed * 1664525u + 1013904223u;
    const float c = (float)(seed >> 8) / 16777216.0f - 0.5f;
    if (sig == 1) {
      l[i] = 2.0f * a;
      r[i] = 1.8f * c;
    } else if (sig == 2) {
      const float fall = 2.0f * expf(-(float)(i - start) / (0.01f * (float)sr)) + 0.002f;
      l[i] = fall * c;
      r[i] = 0.9f * fall * a;
    } else {
      const float env = expf(-(float)(i % period) / (0.012f * (float)sr));
      l[i] = 0.9f * env * a + 0.02f * c;
      r[i] = 0.7f * env * c + 0.02f * a;
    }
  }
}

/* The block's values, with parameter `off` (or none, -1) moved one step inside its travel. */
static void values_of(const Block *k, int off, float *v) {
  memcpy(v, k->v, sizeof k->v);
  if (off < 0) return;
  const omx_plugin_param *p = &OMX_REVERB_PARAMS[off];
  v[off] = v[off] + STEP[off] <= p->max ? v[off] + STEP[off] : v[off] - STEP[off];
}

/* The reference: omx-dsp's instance face, called directly, on the same blocks. */
static void reference(double sr, int off, const float *il, const float *ir, float *ol, float *orr, uint32_t *latency) {
  static OmxReverbInstance inst;
  (void)omx_reverb_instance_init(&inst, (float)sr);
  uint32_t at = 0;
  for (size_t b = 0; b < NBLOCKS; b++) {
    const Block *k = &PLAN[b];
    float values[NP];
    values_of(k, off, values);
    const uint32_t frames = frames_of(k, sr);
    omx_reverb_instance_resolve(&inst, k->bypass, values[OMX_REVERB_PARAM_PLATE_MOD_DEPTH], values[OMX_REVERB_PARAM_MIX], values[OMX_REVERB_PARAM_SIZE], values[OMX_REVERB_PARAM_DAMPING], values[OMX_REVERB_PARAM_WIDTH], values[OMX_REVERB_PARAM_PREDELAY], values[OMX_REVERB_PARAM_LOWCUT], values[OMX_REVERB_PARAM_HIGHCUT], values[OMX_REVERB_PARAM_REVERSE], values[OMX_REVERB_PARAM_HOLD], values[OMX_REVERB_PARAM_RELEASE], values[OMX_REVERB_PARAM_GATE_THRESHOLD], (int)lrintf(values[OMX_REVERB_PARAM_ALGORITHM]));
    omx_reverb_instance_run(&inst, il + at, ir + at, ol + at, orr + at, frames);
    at += frames;
  }
  if (latency) *latency = (uint32_t)lrintf((float)(OMX_REVERB_INSTANCE_LATENCY_FRAMES));
}

typedef struct Driver Driver;
struct Driver {
  /* the whole plan through ONE fresh instance; `*latency` is read back */
  bool (*run)(void *face, double sr, const float *il, const float *ir, float *ol, float *orr, uint32_t *latency);
  void *face;
};

static bool same(const float *a, const float *b, uint32_t n) { return memcmp(a, b, (size_t)n * sizeof(float)) == 0; }

static void run_checks(const char *face, Driver *d, double sr) {
  const uint32_t n = total_frames(sr);
  float *buf = calloc((size_t)n * 8u, sizeof(float));
  if (!buf) abort();
  float *il = buf, *ir = buf + n, *ol = buf + 2u * n, *orr = buf + 3u * n, *wl = buf + 4u * n, *wr = buf + 5u * n;
  float *xl = buf + 6u * n, *xr = buf + 7u * n;
  signal_make(il, ir, n, sr);
  omx_denormals_off(); /* the faces run with flush-to-zero; so does the reference */
  uint32_t want = 0;
  reference(sr, -1, il, ir, wl, wr, &want);
  uint32_t lat = 99;
  bool ran = d->run(d->face, sr, il, ir, ol, orr, &lat);
  bool ident = ran && same(ol, wl, n) && same(orr, wr, n);
  for (uint32_t i = 0; ran && !ident && i < n; i++)
    if (memcmp(ol + i, wl + i, sizeof(float)) != 0 || memcmp(orr + i, wr + i, sizeof(float)) != 0) {
      fprintf(stderr, "  first difference at frame %u: got %.9g/%.9g, kernel %.9g/%.9g\n", i, (double)ol[i],
              (double)orr[i], (double)wl[i], (double)wr[i]);
      break;
    }
  check(ident, face, sr, "output is the instance face's, bit for bit");
  /* the reference must have moved the signal, or identity would be vacuous */
  check(!same(wl, il, n) || !same(wr, ir, n), face, sr, "the kernel shapes the signal");
  check(ran && lat == want, face, sr, "published latency is the kernel's");
  /* the sabotage arm: each parameter one step off must be seen */
  for (int p = 0; p < (int)NP; p++) {
    reference(sr, p, il, ir, xl, xr, NULL);
    char what[160];
    snprintf(what, sizeof what, "a reference with %s one step off differs", OMX_REVERB_PARAMS[p].symbol);
    check(ran && (!same(xl, ol, n) || !same(xr, orr, n)), face, sr, what);
  }
  free(buf);
}

/* ---- CLAP ---------------------------------------------------------------------------------- */

static const void *h_ext(const clap_host_t *h, const char *id) {
  (void)h, (void)id;
  return NULL;
}
static void h_noop(const clap_host_t *h) { (void)h; }
static const clap_host_t HOST = {CLAP_VERSION_INIT, NULL, "reverb-oracle", "openmixer", "", "0.1", h_ext, h_noop,
                                 h_noop, h_noop};

typedef struct {
  clap_event_param_value_t ev[NP + 1];
  uint32_t n;
} Events;
static uint32_t ev_size(const clap_input_events_t *l) { return ((const Events *)l->ctx)->n; }
static const clap_event_header_t *ev_get(const clap_input_events_t *l, uint32_t i) {
  return &((const Events *)l->ctx)->ev[i].header;
}
static bool ev_push(const clap_output_events_t *l, const clap_event_header_t *e) {
  (void)l, (void)e;
  return true;
}

typedef struct {
  const clap_plugin_entry_t *entry;
  const clap_plugin_factory_t *factory;
} ClapFace;

static bool clap_param(const clap_plugin_params_t *pp, const clap_plugin_t *p, const char *name, clap_id *id) {
  for (uint32_t i = 0; i < pp->count(p); i++) {
    clap_param_info_t info;
    if (!pp->get_info(p, i, &info)) continue;
    if (name ? strcmp(info.name, name) == 0 : (info.flags & CLAP_PARAM_IS_BYPASS) != 0) {
      *id = info.id;
      return true;
    }
  }
  fprintf(stderr, "  no CLAP parameter named %s\n", name ? name : "(bypass)");
  return false;
}

static bool clap_run(void *face, double sr, const float *il, const float *ir, float *ol, float *orr,
                     uint32_t *latency) {
  ClapFace *c = (ClapFace *)face;
  const clap_plugin_descriptor_t *desc = c->factory->get_plugin_descriptor(c->factory, 0);
  const clap_plugin_t *p = c->factory->create_plugin(c->factory, &HOST, desc->id);
  if (!p || !p->init(p)) return false;
  const clap_plugin_params_t *pp = (const clap_plugin_params_t *)p->get_extension(p, CLAP_EXT_PARAMS);
  const clap_plugin_latency_t *lat = (const clap_plugin_latency_t *)p->get_extension(p, CLAP_EXT_LATENCY);
  bool ok = pp && lat && p->activate(p, sr, 1, longest_block(sr)) && p->start_processing(p);
  clap_id ids[NP + 1];
  for (uint32_t i = 0; ok && i < NP; i++) ok = clap_param(pp, p, OMX_REVERB_PARAMS[i].name, &ids[i]);
  ok = ok && clap_param(pp, p, NULL, &ids[NP]);
  clap_output_events_t out = {NULL, ev_push};
  uint32_t at = 0;
  for (size_t b = 0; ok && b < NBLOCKS; b++) {
    const Block *k = &PLAN[b];
    Events evs = {.n = 0};
    for (uint32_t i = 0; i <= NP; i++) {
      clap_event_param_value_t *e = &evs.ev[evs.n++];
      memset(e, 0, sizeof *e);
      e->header = (clap_event_header_t){sizeof *e, 0, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_PARAM_VALUE, 0};
      e->param_id = ids[i], e->note_id = -1, e->port_index = -1, e->channel = -1, e->key = -1;
      e->value = i < NP ? (double)k->v[i] : (k->bypass ? 1.0 : 0.0);
    }
    clap_input_events_t in = {&evs, ev_size, ev_get};
    float *ib[2] = {(float *)il + at, (float *)ir + at}, *ob[2] = {ol + at, orr + at};
    clap_audio_buffer_t ai = {ib, NULL, 2, 0, 0}, ao = {ob, NULL, 2, 0, 0};
    const uint32_t frames = frames_of(k, sr);
    clap_process_t pr = {.steady_time = at, .frames_count = frames, .audio_inputs = &ai, .audio_outputs = &ao,
                         .audio_inputs_count = 1, .audio_outputs_count = 1, .in_events = &in, .out_events = &out};
    ok = p->process(p, &pr) != CLAP_PROCESS_ERROR;
    at += frames;
  }
  if (ok && latency) *latency = lat->get(p);
  p->stop_processing(p);
  p->deactivate(p);
  p->destroy(p);
  return ok;
}

static int clap_main(const char *path) {
  void *lib = dlopen(path, RTLD_NOW | RTLD_LOCAL);
  if (!lib) return fprintf(stderr, "FAIL dlopen %s: %s\n", path, dlerror()), 1;
  ClapFace c = {(const clap_plugin_entry_t *)dlsym(lib, "clap_entry"), NULL};
  if (!c.entry || !c.entry->init(path)) return fprintf(stderr, "FAIL clap_entry %s\n", path), 1;
  c.factory = (const clap_plugin_factory_t *)c.entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
  if (!c.factory || c.factory->get_plugin_count(c.factory) != 1) return fprintf(stderr, "FAIL factory\n"), 1;
  Driver d = {clap_run, &c};
  for (uint32_t i = 0; i < OMX_DECLARED_RATE_COUNT; i++) run_checks("clap", &d, (double)OMX_DECLARED_RATES[i]);
  c.entry->deinit();
  dlclose(lib);
  return failures ? 1 : 0;
}

/* ---- LV2 ------------------------------------------------------------------------------------ */

typedef struct {
  LilvWorld *world;
  const LilvPlugin *plugin;
} Lv2Face;

static int32_t port_of(const Lv2Face *l, const char *symbol) {
  LilvNode *sym = lilv_new_string(l->world, symbol);
  const LilvPort *port = lilv_plugin_get_port_by_symbol(l->plugin, sym);
  lilv_node_free(sym);
  if (!port) fprintf(stderr, "  no LV2 port '%s'\n", symbol);
  return port ? (int32_t)lilv_port_get_index(l->plugin, port) : -1;
}

static bool lv2_run(void *face, double sr, const float *il, const float *ir, float *ol, float *orr,
                    uint32_t *latency) {
  Lv2Face *l = (Lv2Face *)face;
  LilvInstance *inst = lilv_plugin_instantiate(l->plugin, sr, NULL);
  if (!inst) return false;
  const char *audio[] = {"in_l", "in_r", "out_l", "out_r"};
  int32_t ap[4], cp[NP + 2];
  float cv[NP + 2] = {0};
  bool ok = true;
  for (int i = 0; i < 4; i++) ok = (ap[i] = port_of(l, audio[i])) >= 0 && ok;
  for (uint32_t i = 0; i < NP; i++) ok = (cp[i] = port_of(l, OMX_REVERB_PARAMS[i].symbol)) >= 0 && ok;
  ok = (cp[NP] = port_of(l, "enabled")) >= 0 && ok;
  ok = (cp[NP + 1] = port_of(l, "latency")) >= 0 && ok;
  for (uint32_t i = 0; ok && i < NP + 2; i++) lilv_instance_connect_port(inst, (uint32_t)cp[i], &cv[i]);
  if (ok) lilv_instance_activate(inst);
  uint32_t at = 0;
  for (size_t b = 0; ok && b < NBLOCKS; b++) {
    const Block *k = &PLAN[b];
    for (uint32_t i = 0; i < NP; i++) cv[i] = k->v[i];
    cv[NP] = k->bypass ? 0.0f : 1.0f;
    lilv_instance_connect_port(inst, (uint32_t)ap[0], (void *)(il + at));
    lilv_instance_connect_port(inst, (uint32_t)ap[1], (void *)(ir + at));
    lilv_instance_connect_port(inst, (uint32_t)ap[2], ol + at);
    lilv_instance_connect_port(inst, (uint32_t)ap[3], orr + at);
    const uint32_t frames = frames_of(k, sr);
    lilv_instance_run(inst, frames);
    at += frames;
  }
  if (ok) lilv_instance_deactivate(inst);
  lilv_instance_free(inst);
  if (ok && latency) *latency = (uint32_t)(cv[NP + 1] + 0.5f);
  return ok;
}

static int lv2_main(const char *bundle, const char *uri) {
  Lv2Face l = {lilv_world_new(), NULL};
  char dir[4096];
  snprintf(dir, sizeof dir, "%s%s", bundle, bundle[strlen(bundle) - 1] == '/' ? "" : "/");
  LilvNode *b = lilv_new_file_uri(l.world, NULL, dir);
  lilv_world_load_bundle(l.world, b);
  LilvNode *u = lilv_new_uri(l.world, uri);
  l.plugin = lilv_plugins_get_by_uri(lilv_world_get_all_plugins(l.world), u);
  if (!l.plugin) return fprintf(stderr, "FAIL no LV2 plugin %s in %s\n", uri, dir), 1;
  Driver d = {lv2_run, &l};
  for (uint32_t i = 0; i < OMX_DECLARED_RATE_COUNT; i++) run_checks("lv2", &d, (double)OMX_DECLARED_RATES[i]);
  lilv_node_free(u), lilv_node_free(b);
  lilv_world_free(l.world);
  return failures ? 1 : 0;
}

int main(int argc, char **argv) {
  int rc;
  if (argc == 3 && strcmp(argv[1], "clap") == 0) rc = clap_main(argv[2]);
  else if (argc == 4 && strcmp(argv[1], "lv2") == 0) rc = lv2_main(argv[2], argv[3]);
  else return fprintf(stderr, "usage: reverb-oracle clap <x.clap> | lv2 <bundle-dir> <uri>\n"), 2;
  printf("%s %s: %d failure(s)\n", rc ? "ORACLE-RED" : "ORACLE-GREEN", argv[1], failures);
  return rc;
}
