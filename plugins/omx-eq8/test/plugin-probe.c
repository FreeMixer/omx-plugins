/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
 *
 * plugin-probe.c — a minimal CLAP host and a minimal LV2 host over the BUILT omx-eq8 binaries.
 *
 *   plugin-probe clap-params <omx-eq8.clap>
 *       one line per parameter, read through clap_plugin_params.get_info:
 *       `id<TAB>name<TAB>min<TAB>max<TAB>default<TAB>flags`, compared by tools/clap-params-check.mjs
 *       to the DECLARATION, never to the generated header.
 *   plugin-probe clap-identity <omx-eq8.clap>
 *   plugin-probe clap-reactivate <omx-eq8.clap>
 *       the scenario at every declared rate (and ONE instance re-activated through all of them),
 *       every output sample memcmp'd against the instance core called DIRECTLY, engaged and
 *       bypassed; params.get_value answering every delivered setting.
 *   plugin-probe lv2-identity <.so> <uri> <in_l> <in_r> <out_l> <out_r> <latency> <first_param> <enabled>
 *       the same scenario through the LV2 face.
 *   plugin-probe clap-vs-lv2 <.clap> <.so> <uri> <the same seven ports>
 *       the two faces side by side, every sample memcmp'd: the two-path conformance.
 *
 * The reference is omx-dsp's omx_eq_lv2 core (<omxdsp/fx/omx_eq_instance.h>), one per leg, driven
 * by omx_eq_lv2_set_controls and omx_eq_lv2_run on the same blocks, NOT omx_eq8_stereo.h the faces
 * share, so a shell that added so much as a gain stage would be red here.
 */
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clap/entry.h"
#include "clap/ext/audio-ports.h"
#include "clap/ext/params.h"
#include "clap/host.h"
#include "clap/factory/plugin-factory.h"

#include <lv2/core/lv2.h>
#include <lv2/options/options.h>
#include <lv2/urid/urid.h>
#include <lv2/buf-size/buf-size.h>
#include <lv2/parameters/parameters.h>
#include <lv2/atom/atom.h>

#include "omx_eq8_params.h"
#define OMX_EQ_LV2_BANDS 8
#include <omxdsp/fx/omx_eq_instance.h>
#include <omxdsp/omx_denormal.h>        /* omx_denormals_off */
#include <omxdsp/omx_contract_limits.h> /* OMX_DECLARED_RATES */

/* ---- the scenario ------------------------------------------------------------------------ */

#define RATES OMX_DECLARED_RATES
#define N_RATES OMX_DECLARED_RATE_COUNT
#define NP OMX_EQ8_PARAM_COUNT

static const uint32_t BLOCKS[] = {64, 257, 1024, 13, 512, 1000, 1};
#define N_BLOCKS (sizeof(BLOCKS) / sizeof(BLOCKS[0]))
#define MAX_BLOCK 1024u

typedef struct {
  float v[NP];
} Setting;

#define BAND(s, b, k) ((s)->v[OMX_EQ8_PARAM_B1_TYPE + 5u * (b) + (k)])
enum { T, F, G, Q, ON };

/** Four settings, switched at block boundaries; every value is inside the declared travel and the
 * rest are the declared defaults. 0: HPF 12, two shelves, a bell and a notch. 1: both pass filters
 * at 24 dB/oct, an allpass pair, a deep narrow bell. 2: a wide bell at the top, the whole EQ off.
 * 3: the same bank as 0 back on. */
static void settings(Setting *s) {
  for (int i = 0; i < 4; i++)
    for (uint32_t k = 0; k < NP; k++) s[i].v[k] = OMX_EQ8_PARAMS[k].def;
  Setting *a = &s[0];
  a->v[OMX_EQ8_PARAM_HPF_ON] = 1, a->v[OMX_EQ8_PARAM_HPF_FREQ] = 100, a->v[OMX_EQ8_PARAM_HPF_SLOPE] = 12;
  BAND(a, 0, T) = 0, BAND(a, 0, F) = 1000, BAND(a, 0, G) = 6, BAND(a, 0, Q) = 1, BAND(a, 0, ON) = 1;
  BAND(a, 1, T) = 1, BAND(a, 1, F) = 120, BAND(a, 1, G) = -4, BAND(a, 1, Q) = 0.7f, BAND(a, 1, ON) = 1;
  BAND(a, 2, T) = 2, BAND(a, 2, F) = 8000, BAND(a, 2, G) = 3, BAND(a, 2, Q) = 0.7f, BAND(a, 2, ON) = 1;
  BAND(a, 3, T) = 3, BAND(a, 3, F) = 3000, BAND(a, 3, G) = 0, BAND(a, 3, Q) = 30, BAND(a, 3, ON) = 1;
  Setting *b = &s[1];
  b->v[OMX_EQ8_PARAM_HPF_ON] = 1, b->v[OMX_EQ8_PARAM_HPF_FREQ] = 40, b->v[OMX_EQ8_PARAM_HPF_SLOPE] = 24;
  b->v[OMX_EQ8_PARAM_LPF_ON] = 1, b->v[OMX_EQ8_PARAM_LPF_FREQ] = 12000, b->v[OMX_EQ8_PARAM_LPF_SLOPE] = 24;
  BAND(b, 0, T) = 4, BAND(b, 0, F) = 500, BAND(b, 0, Q) = 2, BAND(b, 0, ON) = 1;
  BAND(b, 1, T) = 5, BAND(b, 1, F) = 2500, BAND(b, 1, Q) = 0.5f, BAND(b, 1, ON) = 1;
  BAND(b, 4, T) = 0, BAND(b, 4, F) = 250, BAND(b, 4, G) = -9, BAND(b, 4, Q) = 8, BAND(b, 4, ON) = 1;
  BAND(b, 7, T) = 0, BAND(b, 7, F) = 16000, BAND(b, 7, G) = 12, BAND(b, 7, Q) = 4, BAND(b, 7, ON) = 1;
  Setting *c = &s[2];
  BAND(c, 7, T) = 0, BAND(c, 7, F) = 16000, BAND(c, 7, G) = 12, BAND(c, 7, Q) = 0.3f, BAND(c, 7, ON) = 1;
  c->v[OMX_EQ8_PARAM_ON] = 0;
  s[3] = s[0];
}
#define N_SETTINGS 4u

static uint32_t setting_of(uint32_t block, uint32_t n_blocks) {
  uint32_t s = block * N_SETTINGS / n_blocks;
  return s < N_SETTINGS ? s : N_SETTINGS - 1;
}

typedef struct {
  float *in_l, *in_r, *ref_l, *ref_r, *out_l, *out_r;
  uint32_t frames, n_blocks;
  Setting set[N_SETTINGS];
} Run;

/** 1.6 s of deterministic noise with sparse impulses, long enough for every filter to settle and
 * ring through each switch of setting. */
static int run_alloc(Run *r, float sr) {
  r->frames = (uint32_t)(sr * 1.6f);
  r->n_blocks = 0;
  for (uint32_t f = 0, b = 0; f < r->frames; b++) {
    f += BLOCKS[b % N_BLOCKS];
    r->n_blocks++;
  }
  float **bufs[] = {&r->in_l, &r->in_r, &r->ref_l, &r->ref_r, &r->out_l, &r->out_r};
  for (size_t i = 0; i < 6; i++) {
    *bufs[i] = (float *)calloc(r->frames, sizeof(float));
    if (!*bufs[i]) return 0;
  }
  uint32_t seed = 0x6f6d78u;
  for (uint32_t i = 0; i < r->frames; i++) {
    seed = seed * 1664525u + 1013904223u;
    float a = ((float)(seed >> 8) / 16777216.0f - 0.5f) * 0.5f;
    seed = seed * 1664525u + 1013904223u;
    float b = ((float)(seed >> 8) / 16777216.0f - 0.5f) * 0.5f;
    if (i % 9973u == 0u) a = 0.9f, b = -0.9f;
    r->in_l[i] = a;
    r->in_r[i] = b;
  }
  settings(r->set);
  return 1;
}

static void run_free(Run *r) {
  free(r->in_l), free(r->in_r), free(r->ref_l), free(r->ref_r), free(r->out_l), free(r->out_r);
}

/** THE REFERENCE: omx-dsp's mono core, one per leg, on the same blocks under the same settings. */
static int kernel_reference(Run *r, float sr) {
  static struct omx_eq_lv2 leg[2];
  omx_eq_lv2_init(&leg[0], sr);
  omx_eq_lv2_init(&leg[1], sr);
  memcpy(r->ref_l, r->in_l, r->frames * sizeof(float));
  memcpy(r->ref_r, r->in_r, r->frames * sizeof(float));
  uint32_t f = 0;
  for (uint32_t b = 0; b < r->n_blocks; b++) {
    uint32_t n = BLOCKS[b % N_BLOCKS];
    if (f + n > r->frames) n = r->frames - f;
    const Setting *s = &r->set[setting_of(b, r->n_blocks)];
    struct omx_eq_lv2_controls c;
    memset(&c, 0, sizeof c);
    c.on = &s->v[OMX_EQ8_PARAM_ON];
    c.hpf_on = &s->v[OMX_EQ8_PARAM_HPF_ON], c.hpf_freq = &s->v[OMX_EQ8_PARAM_HPF_FREQ], c.hpf_slope = &s->v[OMX_EQ8_PARAM_HPF_SLOPE];
    c.lpf_on = &s->v[OMX_EQ8_PARAM_LPF_ON], c.lpf_freq = &s->v[OMX_EQ8_PARAM_LPF_FREQ], c.lpf_slope = &s->v[OMX_EQ8_PARAM_LPF_SLOPE];
    for (uint32_t i = 0; i < 8; i++)
      for (uint32_t k = 0; k < 5; k++) c.band[i][k] = &s->v[OMX_EQ8_PARAM_B1_TYPE + 5u * i + k];
    omx_eq_lv2_set_controls(&leg[0], &c);
    omx_eq_lv2_set_controls(&leg[1], &c);
    omx_eq_lv2_run(&leg[0], r->in_l + f, r->ref_l + f, n);
    omx_eq_lv2_run(&leg[1], r->in_r + f, r->ref_r + f, n);
    f += n;
  }
  /* A reference that never left the input would make "plugin == core" and "bypass == input" the
   * same claim; the engaged scenario must actually filter. */
  if (memcmp(r->ref_l, r->in_l, r->frames * sizeof(float)) == 0 ||
      memcmp(r->ref_r, r->in_r, r->frames * sizeof(float)) == 0) {
    fprintf(stderr, "FAIL reference @ %.0f Hz equals its input — the scenario proves nothing\n", (double)sr);
    return 0;
  }
  return 1;
}

static int compare(const char *what, float sr, const float *a_l, const float *a_r, const float *b_l,
                   const float *b_r, uint32_t n) {
  for (uint32_t i = 0; i < n; i++) {
    if (memcmp(&a_l[i], &b_l[i], sizeof(float)) != 0 || memcmp(&a_r[i], &b_r[i], sizeof(float)) != 0) {
      fprintf(stderr, "FAIL %s @ %.0f Hz: frame %u differs (L %.9g vs %.9g, R %.9g vs %.9g)\n", what,
              (double)sr, i, (double)a_l[i], (double)b_l[i], (double)a_r[i], (double)b_r[i]);
      return 0;
    }
  }
  printf("ok %s @ %.0f Hz: %u frames bit-identical\n", what, (double)sr, n);
  return 1;
}

/* ---- CLAP ---------------------------------------------------------------------------------- */

static const void *host_get_extension(const clap_host_t *h, const char *id) {
  (void)h, (void)id;
  return NULL;
}
static void host_noop(const clap_host_t *h) { (void)h; }

static const clap_host_t HOST = {
    CLAP_VERSION_INIT, NULL, "omx plugin-probe", "openmixer", "", "0.1",
    host_get_extension, host_noop, host_noop, host_noop,
};

typedef struct {
  clap_event_param_value_t ev[NP + 2];
  uint32_t n;
} EvList;

static uint32_t ev_size(const clap_input_events_t *l) { return ((const EvList *)l->ctx)->n; }
static const clap_event_header_t *ev_get(const clap_input_events_t *l, uint32_t i) {
  return &((const EvList *)l->ctx)->ev[i].header;
}
static bool ev_push(const clap_output_events_t *l, const clap_event_header_t *e) {
  (void)l, (void)e;
  return true;
}

static void ev_add(EvList *l, clap_id id, double v) {
  clap_event_param_value_t *e = &l->ev[l->n++];
  memset(e, 0, sizeof *e);
  e->header.size = sizeof *e;
  e->header.time = 0;
  e->header.space_id = CLAP_CORE_EVENT_SPACE_ID;
  e->header.type = CLAP_EVENT_PARAM_VALUE;
  e->param_id = id;
  e->note_id = -1;
  e->port_index = -1;
  e->channel = -1;
  e->key = -1;
  e->value = v;
}

typedef struct {
  void *so;
  const clap_plugin_entry_t *entry;
  const clap_plugin_t *plugin;
  const clap_plugin_params_t *params;
} Clap;

static int clap_open(Clap *c, const char *path) {
  memset(c, 0, sizeof *c);
  c->so = dlopen(path, RTLD_NOW | RTLD_LOCAL);
  if (!c->so) return fprintf(stderr, "dlopen %s: %s\n", path, dlerror()), 0;
  c->entry = (const clap_plugin_entry_t *)dlsym(c->so, "clap_entry");
  if (!c->entry || !c->entry->init(path)) return fprintf(stderr, "no clap_entry\n"), 0;
  const clap_plugin_factory_t *fac =
      (const clap_plugin_factory_t *)c->entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
  if (!fac || fac->get_plugin_count(fac) != 1) return fprintf(stderr, "factory\n"), 0;
  const clap_plugin_descriptor_t *d = fac->get_plugin_descriptor(fac, 0);
  c->plugin = fac->create_plugin(fac, &HOST, d->id);
  if (!c->plugin || !c->plugin->init(c->plugin)) return fprintf(stderr, "create/init\n"), 0;
  c->params = (const clap_plugin_params_t *)c->plugin->get_extension(c->plugin, CLAP_EXT_PARAMS);
  if (!c->params) return fprintf(stderr, "no params extension\n"), 0;
  return 1;
}

static void clap_close(Clap *c) {
  if (c->plugin) c->plugin->destroy(c->plugin);
  if (c->entry) c->entry->deinit();
  if (c->so) dlclose(c->so);
}

enum { P_BYPASS = NP };

static int clap_params_dump(const char *path) {
  Clap c;
  if (!clap_open(&c, path)) return 1;
  uint32_t n = c.params->count(c.plugin);
  for (uint32_t i = 0; i < n; i++) {
    clap_param_info_t info;
    if (!c.params->get_info(c.plugin, i, &info)) return fprintf(stderr, "get_info %u\n", i), 1;
    printf("%u\t%s\t%.17g\t%.17g\t%.17g\t%u\n", info.id, info.name, info.min_value, info.max_value,
           info.default_value, (unsigned)info.flags);
  }
  clap_close(&c);
  return 0;
}

/** The scenario through one ACTIVE, processing instance: uneven blocks, the settings switched
 * mid-stream as CLAP_EVENT_PARAM_VALUE at frame 0. `readback` also checks that after the block
 * that delivered a setting, params.get_value answers every delivered value. */
static int clap_scenario(Clap *c, Run *r, int bypass, float *out_l, float *out_r, int readback) {
  uint32_t f = 0, prev = UINT32_MAX;
  for (uint32_t b = 0; b < r->n_blocks; b++) {
    uint32_t n = BLOCKS[b % N_BLOCKS];
    if (f + n > r->frames) n = r->frames - f;
    EvList evs = {.n = 0};
    uint32_t si = setting_of(b, r->n_blocks);
    const int delivered = si != prev;
    if (delivered) {
      for (uint32_t k = 0; k < NP; k++) ev_add(&evs, k, r->set[si].v[k]);
      ev_add(&evs, P_BYPASS, bypass ? 1.0 : 0.0);
      prev = si;
    }
    clap_input_events_t in_ev = {&evs, ev_size, ev_get};
    clap_output_events_t out_ev = {NULL, ev_push};
    float *ins[2] = {r->in_l + f, r->in_r + f};
    float *outs[2] = {out_l + f, out_r + f};
    clap_audio_buffer_t ain = {ins, NULL, 2, 0, 0};
    clap_audio_buffer_t aout = {outs, NULL, 2, 0, 0};
    clap_process_t proc;
    memset(&proc, 0, sizeof proc);
    proc.steady_time = f;
    proc.frames_count = n;
    proc.audio_inputs = &ain;
    proc.audio_outputs = &aout;
    proc.audio_inputs_count = 1;
    proc.audio_outputs_count = 1;
    proc.in_events = &in_ev;
    proc.out_events = &out_ev;
    if (c->plugin->process(c->plugin, &proc) == CLAP_PROCESS_ERROR) {
      fprintf(stderr, "process error\n");
      return 0;
    }
    if (readback && delivered) {
      for (uint32_t k = 0; k < NP; k++) {
        double got = -1.0;
        if (!c->params->get_value(c->plugin, k, &got) || (float)got != r->set[si].v[k]) {
          fprintf(stderr, "FAIL read-back: param %u delivered %.9g, get_value %.9g\n", (unsigned)k,
                  (double)r->set[si].v[k], got);
          return 0;
        }
      }
    }
    f += n;
  }
  return 1;
}

static int clap_identity_at(const char *path, float sr) {
  Clap c;
  Run r;
  int ok = 0;
  if (!clap_open(&c, path) || !run_alloc(&r, sr) || !kernel_reference(&r, sr)) return 0;
  if (!c.plugin->activate(c.plugin, sr, 1, MAX_BLOCK) || !c.plugin->start_processing(c.plugin)) {
    fprintf(stderr, "activate\n");
    goto out;
  }
  for (int pass = 0; pass < 2; pass++) {
    const int bypass = pass == 1;
    if (!clap_scenario(&c, &r, bypass, r.out_l, r.out_r, 0)) goto out;
    if (bypass) {
      if (!compare("clap bypass", sr, r.out_l, r.out_r, r.in_l, r.in_r, r.frames)) goto out;
    } else {
      if (!compare("clap vs core", sr, r.out_l, r.out_r, r.ref_l, r.ref_r, r.frames)) goto out;
    }
    c.plugin->stop_processing(c.plugin);
    c.plugin->deactivate(c.plugin);
    if (!c.plugin->activate(c.plugin, sr, 1, MAX_BLOCK) || !c.plugin->start_processing(c.plugin)) goto out;
  }
  ok = 1;
out:
  run_free(&r);
  clap_close(&c);
  return ok;
}

/** ONE instance re-activated at every declared rate in turn (a rate change is deactivate +
 * activate), bit-identical to the core at each, and get_value answering every delivered setting. */
static int clap_reactivate(const char *path) {
  Clap c;
  if (!clap_open(&c, path)) return 0;
  int ok = 1;
  for (size_t i = 0; ok && i < N_RATES; i++) {
    Run r;
    const float sr = RATES[i];
    ok = run_alloc(&r, sr) && kernel_reference(&r, sr) && c.plugin->activate(c.plugin, sr, 1, MAX_BLOCK) &&
         c.plugin->start_processing(c.plugin);
    ok = ok && clap_scenario(&c, &r, 0, r.out_l, r.out_r, 1) &&
         compare("clap re-activated vs core", sr, r.out_l, r.out_r, r.ref_l, r.ref_r, r.frames);
    c.plugin->stop_processing(c.plugin);
    c.plugin->deactivate(c.plugin);
    run_free(&r);
  }
  clap_close(&c);
  return ok;
}

/** The probe's CLAP port layout assumption, checked rather than trusted: ONE stereo in, ONE
 * stereo out. */
static int clap_ports_ok(const char *path) {
  Clap c;
  if (!clap_open(&c, path)) return 0;
  const clap_plugin_audio_ports_t *ap =
      (const clap_plugin_audio_ports_t *)c.plugin->get_extension(c.plugin, CLAP_EXT_AUDIO_PORTS);
  int ok = ap && ap->count(c.plugin, true) == 1 && ap->count(c.plugin, false) == 1;
  clap_audio_port_info_t pi;
  ok = ok && ap->get(c.plugin, 0, true, &pi) && pi.channel_count == 2 &&
       ap->get(c.plugin, 0, false, &pi) && pi.channel_count == 2;
  if (!ok) fprintf(stderr, "FAIL clap audio ports are not one stereo in + one stereo out\n");
  clap_close(&c);
  return ok;
}

/* ---- LV2 ----------------------------------------------------------------------------------- */

static const char *URIS[64];
static uint32_t N_URIS;

static LV2_URID urid_map(LV2_URID_Map_Handle h, const char *uri) {
  (void)h;
  for (uint32_t i = 0; i < N_URIS; i++)
    if (strcmp(URIS[i], uri) == 0) return i + 1;
  if (N_URIS == 64) return 0;
  URIS[N_URIS++] = uri;
  return N_URIS;
}

/** The seven port numbers the Makefile reads out of the generated header. */
typedef struct {
  uint32_t in_l, in_r, out_l, out_r, latency, first_param, enabled;
} Ports;

/** The scenario through the LV2 face into `out_l/out_r`, engaged or bypassed. 0 on any failure. */
static int lv2_scenario(const char *so_path, const char *uri, const Ports *port, float sr, Run *r, int bypass,
                        float *out_l, float *out_r) {
  void *so = dlopen(so_path, RTLD_NOW | RTLD_LOCAL);
  if (!so) return fprintf(stderr, "dlopen %s: %s\n", so_path, dlerror()), 0;
  LV2_Descriptor_Function fn = (LV2_Descriptor_Function)dlsym(so, "lv2_descriptor");
  const LV2_Descriptor *d = NULL;
  for (uint32_t i = 0; fn && (d = fn(i)) != NULL; i++)
    if (strcmp(d->URI, uri) == 0) break;
  if (!d) return fprintf(stderr, "no descriptor %s\n", uri), dlclose(so), 0;

  LV2_URID_Map map = {NULL, urid_map};
  const int32_t max_block = (int32_t)MAX_BLOCK;
  const float rate = sr;
  LV2_Options_Option opts[] = {
      {LV2_OPTIONS_INSTANCE, 0, urid_map(NULL, LV2_BUF_SIZE__maxBlockLength), sizeof(int32_t),
       urid_map(NULL, LV2_ATOM__Int), &max_block},
      {LV2_OPTIONS_INSTANCE, 0, urid_map(NULL, LV2_BUF_SIZE__nominalBlockLength), sizeof(int32_t),
       urid_map(NULL, LV2_ATOM__Int), &max_block},
      {LV2_OPTIONS_INSTANCE, 0, urid_map(NULL, LV2_PARAMETERS__sampleRate), sizeof(float),
       urid_map(NULL, LV2_ATOM__Float), &rate},
      {LV2_OPTIONS_INSTANCE, 0, 0, 0, 0, NULL},
  };
  LV2_Feature f_map = {LV2_URID__map, &map};
  LV2_Feature f_opts = {LV2_OPTIONS__options, opts};
  LV2_Feature f_bounded = {LV2_BUF_SIZE__boundedBlockLength, NULL};
  const LV2_Feature *feats[] = {&f_map, &f_opts, &f_bounded, NULL};

  int ok = 0;
  LV2_Handle h = d->instantiate(d, sr, "", feats);
  if (!h) {
    fprintf(stderr, "instantiate\n");
    dlclose(so);
    return 0;
  }
  float ctl[NP], enabled = bypass ? 0.0f : 1.0f, latency = -1.0f;
  for (uint32_t k = 0; k < NP; k++) d->connect_port(h, port->first_param + k, &ctl[k]);
  d->connect_port(h, port->enabled, &enabled);
  d->connect_port(h, port->latency, &latency);
  if (d->activate) d->activate(h);
  uint32_t f = 0;
  for (uint32_t b = 0; b < r->n_blocks; b++) {
    uint32_t n = BLOCKS[b % N_BLOCKS];
    if (f + n > r->frames) n = r->frames - f;
    memcpy(ctl, r->set[setting_of(b, r->n_blocks)].v, sizeof ctl);
    d->connect_port(h, port->in_l, r->in_l + f);
    d->connect_port(h, port->in_r, r->in_r + f);
    d->connect_port(h, port->out_l, out_l + f);
    d->connect_port(h, port->out_r, out_r + f);
    d->run(h, n);
    f += n;
  }
  if (d->deactivate) d->deactivate(h);
  if (latency != 0.0f) {
    fprintf(stderr, "FAIL lv2 latency port reads %g, declared 0\n", (double)latency);
    goto out;
  }
  ok = 1;
out:
  d->cleanup(h);
  dlclose(so);
  return ok;
}

static int lv2_identity_at(const char *so_path, const char *uri, const Ports *port, float sr) {
  Run r;
  int ok = run_alloc(&r, sr) && kernel_reference(&r, sr);
  ok = ok && lv2_scenario(so_path, uri, port, sr, &r, 0, r.out_l, r.out_r) &&
       compare("lv2 vs core", sr, r.out_l, r.out_r, r.ref_l, r.ref_r, r.frames);
  ok = ok && lv2_scenario(so_path, uri, port, sr, &r, 1, r.out_l, r.out_r) &&
       compare("lv2 bypass", sr, r.out_l, r.out_r, r.in_l, r.in_r, r.frames);
  run_free(&r);
  return ok;
}

/** The two faces on one scenario, every sample memcmp'd: the two-path conformance. */
static int clap_vs_lv2_at(const char *clap_path, const char *so_path, const char *uri, const Ports *port, float sr) {
  Clap c;
  Run r;
  int ok = 0;
  if (!clap_open(&c, clap_path) || !run_alloc(&r, sr)) return 0;
  for (int pass = 0; pass < 2; pass++) {
    const int bypass = pass == 1;
    if (!c.plugin->activate(c.plugin, sr, 1, MAX_BLOCK) || !c.plugin->start_processing(c.plugin)) goto out;
    if (!clap_scenario(&c, &r, bypass, r.out_l, r.out_r, 0) ||
        !lv2_scenario(so_path, uri, port, sr, &r, bypass, r.ref_l, r.ref_r))
      goto out;
    if (!compare(bypass ? "clap vs lv2 bypassed" : "clap vs lv2 engaged", sr, r.out_l, r.out_r, r.ref_l, r.ref_r, r.frames)) goto out;
    c.plugin->stop_processing(c.plugin);
    c.plugin->deactivate(c.plugin);
  }
  ok = 1;
out:
  run_free(&r);
  clap_close(&c);
  return ok;
}

static void ports_from(char **argv, Ports *p) {
  uint32_t *w = &p->in_l;
  for (int i = 0; i < 7; i++) w[i] = (uint32_t)strtoul(argv[i], NULL, 10);
}

int main(int argc, char **argv) {
  /* The core as the console runs it: FTZ/DAZ on the calling thread, which is also what the plugin
   * shell sets per callback. */
  omx_denormals_off();
  if (argc >= 3 && strcmp(argv[1], "clap-params") == 0) return clap_params_dump(argv[2]);
  if (argc >= 3 && strcmp(argv[1], "clap-identity") == 0) {
    if (!clap_ports_ok(argv[2])) return 1;
    for (size_t i = 0; i < N_RATES; i++)
      if (!clap_identity_at(argv[2], RATES[i])) return 1;
    return 0;
  }
  if (argc >= 3 && strcmp(argv[1], "clap-reactivate") == 0) return clap_reactivate(argv[2]) ? 0 : 1;
  if (argc == 11 && strcmp(argv[1], "lv2-identity") == 0) {
    Ports p;
    ports_from(argv + 4, &p);
    for (size_t i = 0; i < N_RATES; i++)
      if (!lv2_identity_at(argv[2], argv[3], &p, RATES[i])) return 1;
    return 0;
  }
  if (argc == 12 && strcmp(argv[1], "clap-vs-lv2") == 0) {
    Ports p;
    ports_from(argv + 5, &p);
    for (size_t i = 0; i < N_RATES; i++)
      if (!clap_vs_lv2_at(argv[2], argv[3], argv[4], &p, RATES[i])) return 1;
    return 0;
  }
  fprintf(stderr, "usage: see the header of plugin-probe.c\n");
  return 2;
}
