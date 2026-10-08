/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
 *
 * plugin-probe.c — a minimal CLAP host and a minimal LV2 host over the BUILT omx-drive binaries:
 *
 *   plugin-probe clap-params <omx-drive.clap>
 *       one line per parameter, read through clap_plugin_params.get_info:
 *       `id<TAB>name<TAB>min<TAB>max<TAB>default<TAB>flags` — tools/clap-params-check.mjs compares it
 *       to the DECLARATION, never to the generated header.
 *   plugin-probe clap-identity <omx-drive.clap>
 *   plugin-probe clap-reactivate <x.clap>
 *       ONE instance re-activated at every declared rate, bit-identical to the REFERENCE at each
 *       (the omx-dsp instance core, <omxdsp/fx/omx_drive_instance.h>, called directly here with
 *       the same port values the shell resolves from its events/ports — so a shell that computed
 *       so much as one extra multiply, or crossed two parameters, would be red here), and
 *       params.get_value answering every delivered setting.
 *   plugin-probe lv2-identity <omx-drive.so> <uri> <in_l> <in_r> <out_l> <out_r> <latency>
 *                <driveDb> <character> <bandHz> <mix> <trimDb> <enabled>
 *       the same comparison through the LV2 face.
 *
 * The reference is the omx-dsp INSTANCE CORE, not a second DSP implementation: this file carries
 * no waveshaper, no oversampler, no auto-gain of its own. `band` is fixed to TILT in both the
 * reference and (per the shell's own file header) the shell, so `bandHz` is a live control.
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

#include <omxdsp/fx/omx_drive_instance.h>
#include <omxdsp/omx_denormal.h> /* omx_denormals_off */
#include <omxcontract/omx_contract_limits.h> /* OMX_STANDARD_SAMPLE_RATES */

/* ---- the scenario ------------------------------------------------------------------------ */

static const float RATES[] = OMX_STANDARD_SAMPLE_RATES_INIT; /* the rates the console declares */
#define N_RATES OMX_STANDARD_SAMPLE_RATES_COUNT

/** Uneven block sizes, cycled — a host is free to hand any size up to its declared maximum. */
static const uint32_t BLOCKS[] = {64, 257, 1024, 13, 512, 1000, 1};
#define N_BLOCKS (sizeof(BLOCKS) / sizeof(BLOCKS[0]))
#define MAX_BLOCK 1024u

typedef struct {
  float drive_db, character, band_hz, mix_pct, trim_db;
} Setting;

/** Three settings, switched at block boundaries: hot and biased, gentle and tilted low, unity. */
static const Setting SETTINGS[] = {
    {18.0f, 0.6f, 4000.0f, 70.0f, -3.0f},
    {30.0f, -0.8f, 400.0f, 100.0f, 2.0f},
    {0.0f, 0.0f, 2000.0f, 100.0f, 0.0f},
};
#define N_SETTINGS (sizeof(SETTINGS) / sizeof(SETTINGS[0]))

/** Which setting a block runs under: the first third, the second, the rest. */
static uint32_t setting_of(uint32_t block, uint32_t n_blocks) {
  uint32_t s = block * N_SETTINGS / n_blocks;
  return s < N_SETTINGS ? s : N_SETTINGS - 1;
}

typedef struct {
  float *in_l, *in_r, *ref_l, *ref_r, *out_l, *out_r;
  uint32_t frames, n_blocks;
} Run;

/** 0.3 s of deterministic noise with sparse full-scale impulses. */
static int run_alloc(Run *r, float sr) {
  r->frames = (uint32_t)(sr * 0.3f);
  r->n_blocks = 0;
  for (uint32_t f = 0, b = 0; f < r->frames; b++) {
    uint32_t n = BLOCKS[b % N_BLOCKS];
    f += n;
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
    float a = ((float)(seed >> 8) / 16777216.0f - 0.5f) * 1.6f;
    seed = seed * 1664525u + 1013904223u;
    float b = ((float)(seed >> 8) / 16777216.0f - 0.5f) * 1.6f;
    if (i % 9973u == 0u) a = 0.9f, b = -0.9f;
    r->in_l[i] = a;
    r->in_r[i] = b;
  }
  return 1;
}

static void run_free(Run *r) {
  free(r->in_l), free(r->in_r), free(r->ref_l), free(r->ref_r), free(r->out_l), free(r->out_r);
}

static void ports_for(struct omx_drive_lv2_ports *p, int bypass, const Setting *s) {
  *p = (struct omx_drive_lv2_ports)OMX_DRIVE_LV2_PORT_DEFAULTS;
  p->bypass = bypass ? 1.0f : 0.0f;
  p->band = (float)OMX_DRIVE_BAND_TILT;
  p->drive_db = s->drive_db;
  p->character = s->character;
  p->band_hz = s->band_hz;
  p->mix_pct = s->mix_pct;
  p->trim_db = s->trim_db;
}

/** THE REFERENCE: the omx-dsp instance core itself, driven by the same port values the shell
 * resolves, on the same blocks. */
static int kernel_reference(Run *r, float sr) {
  OmxDriveLv2 core;
  omx_drive_lv2_init(&core, (uint32_t)sr);
  memcpy(r->ref_l, r->in_l, r->frames * sizeof(float));
  memcpy(r->ref_r, r->in_r, r->frames * sizeof(float));
  uint32_t f = 0;
  for (uint32_t b = 0; b < r->n_blocks; b++) {
    uint32_t n = BLOCKS[b % N_BLOCKS];
    if (f + n > r->frames) n = r->frames - f;
    struct omx_drive_lv2_ports p;
    ports_for(&p, 0, &SETTINGS[setting_of(b, r->n_blocks)]);
    omx_drive_lv2_resolve(&core, &p);
    omx_drive_lv2_run(&core, r->ref_l + f, r->ref_r + f, r->ref_l + f, r->ref_r + f, n);
    f += n;
  }
  /* A reference that never left the input would make "plugin == reference" and "bypass == input"
   * the same claim; the engaged scenario must actually drive. */
  if (memcmp(r->ref_l, r->in_l, r->frames * sizeof(float)) == 0) {
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
  clap_event_param_value_t ev[8];
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

/** CLAP param ids are this shell's declaration order with the host's bypass appended. */
enum { P_DRIVE = 0, P_CHARACTER, P_BAND_HZ, P_MIX, P_TRIM, P_BYPASS };

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

static int clap_scenario(Clap *c, Run *r, int bypass, float *out_l, float *out_r, int readback) {
  uint32_t f = 0, prev = UINT32_MAX;
  for (uint32_t b = 0; b < r->n_blocks; b++) {
    uint32_t n = BLOCKS[b % N_BLOCKS];
    if (f + n > r->frames) n = r->frames - f;
    EvList evs = {.n = 0};
    uint32_t si = setting_of(b, r->n_blocks);
    const int delivered = si != prev;
    if (delivered) {
      const Setting *s = &SETTINGS[si];
      ev_add(&evs, P_DRIVE, s->drive_db);
      ev_add(&evs, P_CHARACTER, s->character);
      ev_add(&evs, P_BAND_HZ, s->band_hz);
      ev_add(&evs, P_MIX, s->mix_pct);
      ev_add(&evs, P_TRIM, s->trim_db);
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
      const Setting *s = &SETTINGS[si];
      const clap_id ids[] = {P_DRIVE, P_CHARACTER, P_BAND_HZ, P_MIX, P_TRIM};
      const float want[] = {s->drive_db, s->character, s->band_hz, s->mix_pct, s->trim_db};
      for (size_t k = 0; k < 5; k++) {
        double got = -1.0;
        if (!c->params->get_value(c->plugin, ids[k], &got) || (float)got != want[k]) {
          fprintf(stderr, "FAIL read-back: param %u delivered %.9g, get_value %.9g\n", (unsigned)ids[k],
                  (double)want[k], got);
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
      if (!compare("clap vs reference", sr, r.out_l, r.out_r, r.ref_l, r.ref_r, r.frames)) goto out;
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
         compare("clap re-activated vs reference", sr, r.out_l, r.out_r, r.ref_l, r.ref_r, r.frames);
    c.plugin->stop_processing(c.plugin);
    c.plugin->deactivate(c.plugin);
    run_free(&r);
  }
  clap_close(&c);
  return ok;
}

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

enum { L_IN_L, L_IN_R, L_OUT_L, L_OUT_R, L_LATENCY, L_DRIVE, L_CHARACTER, L_BAND_HZ, L_MIX, L_TRIM, L_ENABLED, L_N };

static int lv2_identity_at(const char *so_path, const char *uri, const uint32_t *port, float sr) {
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

  Run r;
  int ok = 0;
  LV2_Handle h = NULL;
  if (!run_alloc(&r, sr) || !kernel_reference(&r, sr)) goto out;
  h = d->instantiate(d, sr, "", feats);
  if (!h) {
    fprintf(stderr, "instantiate\n");
    goto out;
  }
  float ctl[L_N] = {0};
  for (int i = L_LATENCY; i < L_N; i++) d->connect_port(h, port[i], &ctl[i]);
  for (int pass = 0; pass < 2; pass++) {
    const int bypass = pass == 1;
    if (d->activate) d->activate(h);
    uint32_t f = 0;
    for (uint32_t b = 0; b < r.n_blocks; b++) {
      uint32_t n = BLOCKS[b % N_BLOCKS];
      if (f + n > r.frames) n = r.frames - f;
      const Setting *s = &SETTINGS[setting_of(b, r.n_blocks)];
      ctl[L_DRIVE] = s->drive_db;
      ctl[L_CHARACTER] = s->character;
      ctl[L_BAND_HZ] = s->band_hz;
      ctl[L_MIX] = s->mix_pct;
      ctl[L_TRIM] = s->trim_db;
      ctl[L_ENABLED] = bypass ? 0.0f : 1.0f;
      d->connect_port(h, port[L_IN_L], r.in_l + f);
      d->connect_port(h, port[L_IN_R], r.in_r + f);
      d->connect_port(h, port[L_OUT_L], r.out_l + f);
      d->connect_port(h, port[L_OUT_R], r.out_r + f);
      d->run(h, n);
      f += n;
    }
    if (d->deactivate) d->deactivate(h);
    if (bypass ? !compare("lv2 bypass", sr, r.out_l, r.out_r, r.in_l, r.in_r, r.frames)
               : !compare("lv2 vs reference", sr, r.out_l, r.out_r, r.ref_l, r.ref_r, r.frames))
      goto out;
  }
  ok = 1;
out:
  if (h) d->cleanup(h);
  run_free(&r);
  dlclose(so);
  return ok;
}

int main(int argc, char **argv) {
  /* The kernel as the console runs it: FTZ/DAZ on the calling thread, which is also what the
   * plugin shell sets per callback. */
  omx_denormals_off();
  if (argc >= 3 && strcmp(argv[1], "clap-params") == 0) return clap_params_dump(argv[2]);
  if (argc >= 3 && strcmp(argv[1], "clap-identity") == 0) {
    if (!clap_ports_ok(argv[2])) return 1;
    for (size_t i = 0; i < N_RATES; i++)
      if (!clap_identity_at(argv[2], RATES[i])) return 1;
    return 0;
  }
  if (argc >= 3 && strcmp(argv[1], "clap-reactivate") == 0) return clap_reactivate(argv[2]) ? 0 : 1;
  if (argc == 4 + L_N && strcmp(argv[1], "lv2-identity") == 0) {
    uint32_t port[L_N];
    for (int i = 0; i < L_N; i++) port[i] = (uint32_t)strtoul(argv[4 + i], NULL, 10);
    for (size_t i = 0; i < N_RATES; i++)
      if (!lv2_identity_at(argv[2], argv[3], port, RATES[i])) return 1;
    return 0;
  }
  fprintf(stderr, "usage: see the header of plugin-probe.c\n");
  return 2;
}
