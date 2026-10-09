// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * deesser-oracle.c — omx-deesser's kernel-identity test: both built faces, sample for sample, against
 * omx-dsp's deesser kernel called DIRECTLY on the same blocks, at every declared rate, engaged and
 * bypassed — no tolerance.
 *
 *   deesser-oracle clap <omx-deesser.clap>
 *   deesser-oracle lv2 <omx-deesser.lv2> <uri>
 *
 * The reference is built here from the kernel's own header (<omxdsp/fx/omx_deesser.h>): the same
 * resolved atom the console's controller builds (an ABOVE gain computer over a PEAK detector, the
 * knee at 6 dB, no make-up) and the detector band as the cookbook's bandpass at the Q the cookbook's
 * bandwidth-in-octaves relation gives, written out again in this file and not read from the plugin.
 * Both faces must hand the kernel the same atom, so the output is compared with memcmp.
 * Parameters are found BY NAME (CLAP) or BY SYMBOL (LV2, through lilv), so a renumbered face cannot
 * pass by accident. A reference that never reduced anything would make the identity vacuous, so the
 * engaged settings must also differ from the input (the positive control).
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
#include <omxcontract/omx_contract_limits.h>
#include <omxdsp/fx/omx_deesser.h>

static const float RATES[] = {44100.0f, 48000.0f, 88200.0f, 96000.0f, 176400.0f, 192000.0f}; /* OMX_DECLARED_RATES */
#define NRATES (sizeof RATES / sizeof RATES[0])
#define BLOCK 512u
#define FRAMES 8192u

typedef struct {
  double freq_hz, width_oct, threshold_db, ratio, range_db, attack_ms, release_ms, bypass;
} Setting;

static int failures;

typedef struct Driver Driver;
static void run_checks(const char *face, Driver *d, double sr);

static void check(bool ok, const char *face, double sr, const char *what) {
  if (!ok) failures++;
  printf("%s %s %s @ %.0f Hz\n", ok ? "PASS" : "FAIL", face, what, sr);
}

/* A hot noise with a 7 kHz "s" riding on it, so the band's detector is over the threshold. */
static void signal(float *l, float *r, uint32_t n, double sr, uint32_t seed) {
  for (uint32_t i = 0; i < n; i++) {
    seed = seed * 1664525u + 1013904223u;
    const float nl = ((float)(seed >> 8) / 16777216.0f - 0.5f) * 0.6f;
    seed = seed * 1664525u + 1013904223u;
    const float nr = ((float)(seed >> 8) / 16777216.0f - 0.5f) * 0.6f;
    const float s = 0.35f * sinf((float)(2.0 * 3.14159265358979323846 * 7000.0 * (double)i / sr));
    l[i] = nl + s;
    r[i] = nr + 0.8f * s;
  }
}

/* ---- the reference: omx-dsp's kernel, called directly ----------------------------------------- */

static void reference(double sr, const Setting *s, const float *il, const float *ir, float *ol, float *orr,
                      uint32_t frames) {
  struct omx_deess p;
  struct omx_deess_state st;
  memset(&p, 0, sizeof p);
  omx_deess_state_init(&st);
  p.enabled = s->bypass > 0.5 ? 0 : 1;
  p.mode = OMX_DEESS_SPLIT;
  p.dyn.enabled = 1;
  p.dyn.gc.mode = OMX_DYN_ABOVE;
  p.dyn.detect = OMX_DETECT_PEAK;
  p.dyn.gc.thresh_db = (float)s->threshold_db;
  p.dyn.gc.ratio = (float)s->ratio;
  p.dyn.gc.knee_db = 6.0f;
  p.dyn.gc.range_db = (float)s->range_db;
  p.dyn.gc.makeup_lin = 1.0f;
  p.dyn.attack_ms = (float)s->attack_ms;
  p.dyn.ovs_mode = OMX_DYN_OVS_OFF;
  p.dyn.attack_coeff = omx_pole_from_time_ms((float)s->attack_ms, (float)sr);
  p.dyn.release_coeff = omx_pole_from_time_ms((float)s->release_ms, (float)sr);
  /* the cookbook bandpass at freqHz; Q from the band's width in octaves */
  const double w0 = 2.0 * 3.14159265358979323846 * (double)(float)s->freq_hz / sr;
  const double q = 1.0 / (2.0 * sinh(log(2.0) / 2.0 * (double)(float)s->width_oct * w0 / sin(w0)));
  const double alpha = sin(w0) / (2.0 * q), a0 = 1.0 + alpha;
  p.bp_c[0] = (float)(alpha / a0);
  p.bp_c[1] = 0.0f;
  p.bp_c[2] = (float)(-alpha / a0);
  p.bp_c[3] = (float)(-2.0 * cos(w0) / a0);
  p.bp_c[4] = (float)((1.0 - alpha) / a0);
  memcpy(ol, il, frames * sizeof(float));
  memcpy(orr, ir, frames * sizeof(float));
  for (uint32_t f = 0; f < frames; f += BLOCK) {
    const uint32_t n = frames - f < BLOCK ? frames - f : BLOCK;
    omx_deess_process(ol + f, orr + f, n, &p, &st);
  }
}

/* ---- the two faces, behind one interface ------------------------------------------------- */

struct Driver {
  /* run `frames` of stereo input through a FRESH instance under `s`; `*latency` is read back. */
  bool (*run)(void *face, double sr, const Setting *s, const float *il, const float *ir, float *ol, float *orr,
             uint32_t frames, uint32_t *latency);
  void *face;
};

/* ---- CLAP ---------------------------------------------------------------------------------- */

static const void *h_ext(const clap_host_t *h, const char *id) {
  (void)h, (void)id;
  return NULL;
}
static void h_noop(const clap_host_t *h) { (void)h; }
static const clap_host_t HOST = {CLAP_VERSION_INIT, NULL, "deesser-oracle", "openmixer", "", "0.1", h_ext, h_noop, h_noop, h_noop};

typedef struct {
  clap_event_param_value_t ev[10];
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

static bool clap_run(void *face, double sr, const Setting *s, const float *il, const float *ir, float *ol, float *orr,
                     uint32_t frames, uint32_t *latency) {
  ClapFace *c = (ClapFace *)face;
  const clap_plugin_descriptor_t *desc = c->factory->get_plugin_descriptor(c->factory, 0);
  const clap_plugin_t *p = c->factory->create_plugin(c->factory, &HOST, desc->id);
  if (!p || !p->init(p)) return false;
  const clap_plugin_params_t *pp = (const clap_plugin_params_t *)p->get_extension(p, CLAP_EXT_PARAMS);
  const clap_plugin_latency_t *lat = (const clap_plugin_latency_t *)p->get_extension(p, CLAP_EXT_LATENCY);
  bool ok = pp && lat && p->activate(p, sr, 1, BLOCK) && p->start_processing(p);
  Events evs = {.n = 0};
  const char *names[] = {"Freq", "Width", "Threshold", "Ratio", "Range", "Attack", "Release", NULL};
  const double vals[] = {s->freq_hz, s->width_oct, s->threshold_db, s->ratio, s->range_db, s->attack_ms, s->release_ms, s->bypass};
  for (size_t i = 0; ok && i < 8; i++) {
    clap_id id;
    ok = clap_param(pp, p, names[i], &id);
    clap_event_param_value_t *e = &evs.ev[evs.n++];
    memset(e, 0, sizeof *e);
    e->header = (clap_event_header_t){sizeof *e, 0, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_PARAM_VALUE, 0};
    e->param_id = ok ? id : 0, e->note_id = -1, e->port_index = -1, e->channel = -1, e->key = -1, e->value = vals[i];
  }
  clap_input_events_t in = {&evs, ev_size, ev_get};
  Events none = {.n = 0};
  clap_input_events_t in_none = {&none, ev_size, ev_get};
  clap_output_events_t out = {NULL, ev_push};
  for (uint32_t f = 0; ok && f < frames; f += BLOCK) {
    const uint32_t n = frames - f < BLOCK ? frames - f : BLOCK;
    float *ib[2] = {(float *)il + f, (float *)ir + f}, *ob[2] = {ol + f, orr + f};
    clap_audio_buffer_t ai = {ib, NULL, 2, 0, 0}, ao = {ob, NULL, 2, 0, 0};
    clap_process_t pr = {.steady_time = f, .frames_count = n, .audio_inputs = &ai, .audio_outputs = &ao,
                         .audio_inputs_count = 1, .audio_outputs_count = 1, .in_events = f ? &in_none : &in,
                         .out_events = &out};
    ok = p->process(p, &pr) != CLAP_PROCESS_ERROR;
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
  for (size_t i = 0; i < NRATES; i++) run_checks("clap", &d, RATES[i]);
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

static bool lv2_run(void *face, double sr, const Setting *s, const float *il, const float *ir, float *ol, float *orr,
                    uint32_t frames, uint32_t *latency) {
  Lv2Face *l = (Lv2Face *)face;
  LilvInstance *inst = lilv_plugin_instantiate(l->plugin, sr, NULL);
  if (!inst) return false;
  const char *audio[] = {"in_l", "in_r", "out_l", "out_r"};
  const char *ctl[] = {"freqHz", "widthOct", "thresholdDb", "ratio", "rangeDb", "attackMs", "releaseMs", "enabled", "latency"};
  float cv[9] = {(float)s->freq_hz, (float)s->width_oct, (float)s->threshold_db, (float)s->ratio, (float)s->range_db,
                 (float)s->attack_ms, (float)s->release_ms, s->bypass > 0.5 ? 0.0f : 1.0f, 0.0f};
  int32_t ap[4];
  bool ok = true;
  for (int i = 0; i < 4; i++) ok = (ap[i] = port_of(l, audio[i])) >= 0 && ok;
  for (int i = 0; i < 9; i++) {
    int32_t idx = port_of(l, ctl[i]);
    ok = idx >= 0 && ok;
    if (idx >= 0) lilv_instance_connect_port(inst, (uint32_t)idx, &cv[i]);
  }
  if (ok) lilv_instance_activate(inst);
  for (uint32_t f = 0; ok && f < frames; f += BLOCK) {
    const uint32_t n = frames - f < BLOCK ? frames - f : BLOCK;
    lilv_instance_connect_port(inst, (uint32_t)ap[0], (void *)(il + f));
    lilv_instance_connect_port(inst, (uint32_t)ap[1], (void *)(ir + f));
    lilv_instance_connect_port(inst, (uint32_t)ap[2], ol + f);
    lilv_instance_connect_port(inst, (uint32_t)ap[3], orr + f);
    lilv_instance_run(inst, n);
  }
  if (ok) lilv_instance_deactivate(inst);
  lilv_instance_free(inst);
  if (ok && latency) *latency = (uint32_t)(cv[8] + 0.5f);
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
  for (size_t i = 0; i < NRATES; i++) run_checks("lv2", &d, RATES[i]);
  lilv_node_free(u), lilv_node_free(b);
  lilv_world_free(l.world);
  return failures ? 1 : 0;
}

/* ---- the checks, written once against the Driver interface ------------------------------- */

static bool same(const float *a, const float *b, uint32_t n) { return memcmp(a, b, n * sizeof(float)) == 0; }

static void run_checks(const char *face, Driver *d, double sr) {
  float *il = calloc(FRAMES, sizeof(float)), *ir = calloc(FRAMES, sizeof(float));
  float *ol = calloc(FRAMES, sizeof(float)), *orr = calloc(FRAMES, sizeof(float));
  float *rl = calloc(FRAMES, sizeof(float)), *rr = calloc(FRAMES, sizeof(float));
  if (!il || !ir || !ol || !orr || !rl || !rr) abort();
  signal(il, ir, FRAMES, sr, 0x6f6d78u);

  const Setting engaged[] = {
      {7000.0, 1.0, -30.0, 4.0, -12.0, 1.0, 60.0, 0.0},    /* the declared defaults */
      {3000.0, 0.5, -45.0, 12.0, -24.0, 0.1, 5.0, 0.0},    /* narrow, low, hard, fast */
      {16000.0, 4.0, -60.0, 20.0, -6.0, 50.0, 500.0, 0.0}, /* the far ends of every travel */
  };
  for (size_t k = 0; k < sizeof engaged / sizeof engaged[0]; k++) {
    uint32_t lat = 0;
    reference(sr, &engaged[k], il, ir, rl, rr, FRAMES);
    bool ok = d->run(d->face, sr, &engaged[k], il, ir, ol, orr, FRAMES, &lat) && same(ol, rl, FRAMES) &&
              same(orr, rr, FRAMES) && lat == (uint32_t)omx_deess_latency(&(struct omx_deess){0});
    char what[96];
    snprintf(what, sizeof what, "engaged setting %zu is the kernel's output, bytes (latency %u)", k, lat);
    check(ok, face, sr, what);
    /* the positive control: the kernel did reduce, so the identity above is not a pass-through */
    check(!same(rl, il, FRAMES) || !same(rr, ir, FRAMES), face, sr, "the reference reduces on this signal");
  }

  Setting by = engaged[0];
  by.bypass = 1.0;
  uint32_t lat = 0;
  reference(sr, &by, il, ir, rl, rr, FRAMES);
  bool ok = d->run(d->face, sr, &by, il, ir, ol, orr, FRAMES, &lat) && same(ol, rl, FRAMES) && same(orr, rr, FRAMES) &&
            same(ol, il, FRAMES) && same(orr, ir, FRAMES);
  check(ok, face, sr, "bypassed is the kernel's output and the input, bytes");

  free(il), free(ir), free(ol), free(orr), free(rl), free(rr);
}

int main(int argc, char **argv) {
  int rc;
  if (argc == 3 && strcmp(argv[1], "clap") == 0) rc = clap_main(argv[2]);
  else if (argc == 4 && strcmp(argv[1], "lv2") == 0) rc = lv2_main(argv[2], argv[3]);
  else return fprintf(stderr, "usage: deesser-oracle clap <x.clap> | lv2 <bundle-dir> <uri>\n"), 2;
  printf("%s %s: %d failure(s)\n", rc ? "ORACLE-RED" : "ORACLE-GREEN", argv[1], failures);
  return rc;
}
