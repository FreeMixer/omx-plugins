// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * strip-oracle.c — the BUILT strip equals omx-dsp's strip modules called in sequence, checked on
 * both faces at 44.1, 48, 96 and 192 kHz.
 *
 *   strip-oracle clap <omx-strip.clap>
 *   strip-oracle lv2 <bundle-dir> <uri>
 *   strip-oracle clap-params <omx-strip.clap>    the CLAP parameter list, for clap-params-check.mjs
 *
 * The reference is written here, never read from the plugin (this file does not include
 * omx_strip.h): per block, the trim (omx_db_to_lin through omx_ramp), the HPF/LPF (an
 * omx_eq_instance per leg with no band), the gate (omx_gate_instance), the EQ (an omx_eq_instance
 * per leg) and the compressor (omx_dynamics_instance), in the order the `order` parameter names —
 * decoded here as the index of a permutation of (input, gate, eq, comp) in lexicographic order, so
 * 0 is that default order. Every one of the 24 orders is checked with every stage engaged and
 * acting on a bursty signal, then the defaults, then bypass. A sanity check first proves the
 * reference is order-sensitive on this signal, so an order the plugin got wrong cannot pass.
 * Parameters are found BY NAME (CLAP) or BY SYMBOL (LV2, through lilv).
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

#include "omx_strip_params.h"
#define OMX_EQ_LV2_BANDS 4
#include <omxdsp/fx/omx_dynamics_instance.h>
#include <omxdsp/fx/omx_eq_instance.h>
#include <omxdsp/fx/omx_gate_instance.h>
#include <omxdsp/omx_denormal.h>
#include <omxdsp/omx_ramp.h>
#include <omxdsp/omx_units.h>

static const double RATES[] = {44100.0, 48000.0, 96000.0, 192000.0};
#define BLOCK 512u
#define TOL 1e-6f
#define N_PARAMS OMX_STRIP_PARAM_COUNT
#define P(sym) OMX_STRIP_PARAM_##sym

typedef struct {
  float v[N_PARAMS];
  int bypass;
} Setting;

typedef bool (*RunFn)(void *face, double sr, const Setting *s, const float *il, const float *ir, float *ol, float *or_,
                      uint32_t frames);

static int failures;

static void check(bool ok, const char *face, double sr, const char *what) {
  if (!ok) failures++;
  printf("%s %s %s @ %.0f Hz\n", ok ? "PASS" : "FAIL", face, what, sr);
}

static bool same(const float *got, const float *want, uint32_t n, const char *leg) {
  for (uint32_t i = 0; i < n; i++)
    if (!(fabsf(got[i] - want[i]) <= TOL)) {
      fprintf(stderr, "  %s frame %u: got %.9g, want %.9g\n", leg, i, (double)got[i], (double)want[i]);
      return false;
    }
  return true;
}

/* ---- the reference: the modules, in sequence ----------------------------------------------- */

enum { INPUT, GATE, EQ, COMP };

/** Permutation `k` of (0 1 2 3) in lexicographic order, by the factorial number system. */
static void order_of(uint32_t k, int out[4]) {
  int pool[4] = {INPUT, GATE, EQ, COMP}, left = 4;
  static const uint32_t FACT[4] = {6, 2, 1, 1};
  for (int i = 0; i < 4; i++) {
    const uint32_t j = k / FACT[i];
    k %= FACT[i];
    out[i] = pool[j];
    for (int m = (int)j; m < left - 1; m++) pool[m] = pool[m + 1];
    left--;
  }
}

static void reference(double sr, const Setting *s, const float *il, const float *ir, float *ol, float *or_,
                      uint32_t frames) {
  const float *v = s->v;
  memcpy(ol, il, frames * sizeof(float));
  memcpy(or_, ir, frames * sizeof(float));
  if (s->bypass) return;
  struct omx_eq_lv2 filt[2], eq[2];
  OmxGateInstance gate;
  OmxDynamicsInstance comp;
  for (int c = 0; c < 2; c++) omx_eq_lv2_init(&filt[c], sr), omx_eq_lv2_init(&eq[c], sr);
  omx_gate_instance_init(&gate, (float)sr);
  omx_dynamics_instance_init(&comp, (float)sr);
  float trim = 1.0f;
  int order[4];
  order_of((uint32_t)lrintf(v[P(ORDER)]), order);
  omx_denormals_off();
  for (uint32_t f = 0; f < frames; f += BLOCK) {
    const uint32_t n = frames - f < BLOCK ? frames - f : BLOCK;
    float *l = ol + f, *r = or_ + f;
    for (int c = 0; c < 2; c++) {
      omx_eq_lv2_set_on(&filt[c], 1);
      omx_eq_lv2_set_hpf(&filt[c], v[P(HPF_ON)] > 0.5f, v[P(HPF_FREQ)], v[P(HPF_SLOPE)] >= 24.0f);
      omx_eq_lv2_set_lpf(&filt[c], v[P(LPF_ON)] > 0.5f, v[P(LPF_FREQ)], v[P(LPF_SLOPE)] >= 24.0f);
      omx_eq_lv2_set_on(&eq[c], v[P(EQ_ON)] > 0.5f);
      for (uint32_t b = 0; b < 4; b++) {
        const float *q = &v[P(EQ1_TYPE) + 5u * b];
        omx_eq_lv2_set_band(&eq[c], b, (int)lrintf(q[0]), q[1], q[2], q[3], q[4] > 0.5f);
      }
    }
    omx_gate_instance_resolve(&gate, v[P(GATE_ON)] < 0.5f, 0, v[P(GATE_THRESHOLD)], v[P(GATE_RATIO)],
                              v[P(GATE_RANGE)], v[P(GATE_ATTACK)], v[P(GATE_RELEASE)]);
    omx_dynamics_instance_resolve(&comp, v[P(COMP_ON)] < 0.5f, v[P(COMP_THRESHOLD)], v[P(COMP_RATIO)],
                                  v[P(COMP_KNEE)], v[P(COMP_ATTACK)], v[P(COMP_RELEASE)], v[P(COMP_MAKEUP)],
                                  v[P(COMP_RMS)] > 0.5f, OMX_DYN_OVS_AUTO);
    for (int k = 0; k < 4; k++) {
      switch (order[k]) {
      case INPUT: {
        const struct omx_ramp g = omx_ramp_begin(&trim, omx_db_to_lin(v[P(TRIM_DB)]), n);
        for (uint32_t i = 0; i < n; i++) l[i] *= omx_ramp_at(g, i), r[i] *= omx_ramp_at(g, i);
        omx_ramp_end(g, &trim);
        omx_eq_lv2_run(&filt[0], l, l, n);
        omx_eq_lv2_run(&filt[1], r, r, n);
        break;
      }
      case GATE: omx_gate_instance_run(&gate, NULL, l, r, l, r, n); break;
      case EQ: omx_eq_lv2_run(&eq[0], l, l, n), omx_eq_lv2_run(&eq[1], r, r, n); break;
      case COMP: omx_dynamics_instance_run(&comp, l, r, l, r, n); break;
      }
    }
  }
}

static void defaults(Setting *s) {
  for (uint32_t i = 0; i < N_PARAMS; i++) s->v[i] = OMX_STRIP_PARAMS[i].def;
  s->bypass = 0;
}

/** Every stage engaged and acting: a hot trim, both filters, a gate that closes in the quiet
 * bursts, four bands that move, a compressor that works on the loud ones. */
static void busy(Setting *s, uint32_t order) {
  defaults(s);
  float *v = s->v;
  v[P(TRIM_DB)] = 6.0f;
  v[P(HPF_ON)] = 1.0f, v[P(HPF_FREQ)] = 120.0f, v[P(HPF_SLOPE)] = 24.0f;
  v[P(LPF_ON)] = 1.0f, v[P(LPF_FREQ)] = 9000.0f;
  v[P(GATE_ON)] = 1.0f, v[P(GATE_THRESHOLD)] = -30.0f, v[P(GATE_RANGE)] = -60.0f, v[P(GATE_RELEASE)] = 40.0f;
  static const float GAIN[4] = {6.0f, -4.0f, 5.0f, -6.0f};
  for (uint32_t b = 0; b < 4; b++) v[P(EQ1_GAIN) + 5u * b] = GAIN[b];
  v[P(COMP_ON)] = 1.0f, v[P(COMP_THRESHOLD)] = -24.0f, v[P(COMP_RATIO)] = 6.0f, v[P(COMP_ATTACK)] = 2.0f;
  v[P(COMP_RELEASE)] = 80.0f, v[P(COMP_MAKEUP)] = 4.0f;
  v[P(ORDER)] = (float)order;
}

/** Noise in bursts: loud, quiet (under the gate), medium, each 20 ms, both legs different. */
static void signal(double sr, float *l, float *r, uint32_t n) {
  uint32_t seed = 0x6f6d78u;
  const uint32_t seg = (uint32_t)(0.02 * sr);
  static const float LEVEL[3] = {0.9f, 0.003f, 0.2f};
  for (uint32_t i = 0; i < n; i++) {
    const float a = LEVEL[(i / seg) % 3];
    seed = seed * 1664525u + 1013904223u;
    const float x = (float)(seed >> 8) / 16777216.0f - 0.5f;
    l[i] = a * x;
    r[i] = a * (0.7f * x + 0.3f * sinf(0.05f * (float)i));
  }
}

static double max_diff(const float *a, const float *b, uint32_t n) {
  double m = 0.0;
  for (uint32_t i = 0; i < n; i++) m = fmax(m, fabs((double)a[i] - (double)b[i]));
  return m;
}

static void oracle(const char *face, void *h, RunFn run) {
  for (size_t ri = 0; ri < sizeof RATES / sizeof RATES[0]; ri++) {
    const double sr = RATES[ri];
    const uint32_t n = (uint32_t)(0.15 * sr);
    float *il = calloc(n, sizeof(float)), *ir = calloc(n, sizeof(float)), *ol = calloc(n, sizeof(float));
    float *or_ = calloc(n, sizeof(float)), *wl = calloc(n, sizeof(float)), *wr = calloc(n, sizeof(float));
    float *xl = calloc(n, sizeof(float)), *xr = calloc(n, sizeof(float));
    if (!il || !ir || !ol || !or_ || !wl || !wr || !xl || !xr) abort();
    signal(sr, il, ir, n);
    char what[160];
    Setting s;

    /* The reference is order-sensitive here: the default order and gate/comp swapped differ. */
    busy(&s, 0), reference(sr, &s, il, ir, wl, wr, n);
    busy(&s, 23), reference(sr, &s, il, ir, xl, xr, n);
    check(max_diff(wl, xl, n) > 1e-3, face, sr, "the reference is order-sensitive on this signal");

    for (uint32_t k = 0; k < 24; k++) {
      int o[4];
      static const char *NAME[4] = {"input", "gate", "eq", "comp"};
      order_of(k, o);
      busy(&s, k);
      reference(sr, &s, il, ir, wl, wr, n);
      snprintf(what, sizeof what, "order %2u (%s > %s > %s > %s) = the modules in sequence", k, NAME[o[0]], NAME[o[1]],
               NAME[o[2]], NAME[o[3]]);
      check(run(h, sr, &s, il, ir, ol, or_, n) && same(ol, wl, n, "L") && same(or_, wr, n, "R"), face, sr, what);
    }

    defaults(&s);
    reference(sr, &s, il, ir, wl, wr, n);
    check(run(h, sr, &s, il, ir, ol, or_, n) && same(ol, wl, n, "L") && same(or_, wr, n, "R"), face, sr,
          "declared defaults = the modules in sequence");

    busy(&s, 5), s.bypass = 1;
    check(run(h, sr, &s, il, ir, ol, or_, n) && same(ol, il, n, "L") && same(or_, ir, n, "R"), face, sr,
          "bypass is the identity");
    free(il), free(ir), free(ol), free(or_), free(wl), free(wr), free(xl), free(xr);
  }
}

/* ---- CLAP ----------------------------------------------------------------------------------- */

static const void *h_ext(const clap_host_t *h, const char *id) {
  (void)h, (void)id;
  return NULL;
}
static void h_noop(const clap_host_t *h) { (void)h; }
static const clap_host_t HOST = {CLAP_VERSION_INIT, NULL, "strip-oracle", "openmixer", "", "0.1", h_ext, h_noop, h_noop, h_noop};

typedef struct {
  clap_event_param_value_t ev[N_PARAMS + 1];
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
  void *lib;
  const clap_plugin_entry_t *entry;
  const clap_plugin_factory_t *factory;
} ClapFace;

/** The param whose name is `name`, or the bypass param when `name` is NULL. */
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

static bool clap_run(void *face, double sr, const Setting *s, const float *il, const float *ir, float *ol, float *or_,
                     uint32_t frames) {
  ClapFace *c = (ClapFace *)face;
  const clap_plugin_descriptor_t *desc = c->factory->get_plugin_descriptor(c->factory, 0);
  const clap_plugin_t *p = c->factory->create_plugin(c->factory, &HOST, desc->id);
  if (!p || !p->init(p)) return false;
  const clap_plugin_params_t *pp = (const clap_plugin_params_t *)p->get_extension(p, CLAP_EXT_PARAMS);
  bool ok = pp && p->activate(p, sr, 1, BLOCK) && p->start_processing(p);
  Events evs = {.n = 0};
  for (uint32_t i = 0; ok && i <= N_PARAMS; i++) {
    clap_id id;
    ok = clap_param(pp, p, i < N_PARAMS ? OMX_STRIP_PARAMS[i].name : NULL, &id);
    clap_event_param_value_t *e = &evs.ev[evs.n++];
    memset(e, 0, sizeof *e);
    e->header = (clap_event_header_t){sizeof *e, 0, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_PARAM_VALUE, 0};
    e->param_id = ok ? id : 0, e->note_id = -1, e->port_index = -1, e->channel = -1, e->key = -1;
    e->value = i < N_PARAMS ? s->v[i] : s->bypass;
  }
  clap_input_events_t in = {&evs, ev_size, ev_get};
  Events none = {.n = 0};
  clap_input_events_t in_none = {&none, ev_size, ev_get};
  clap_output_events_t out = {NULL, ev_push};
  for (uint32_t f = 0; ok && f < frames; f += BLOCK) {
    const uint32_t n = frames - f < BLOCK ? frames - f : BLOCK;
    float *ib[2] = {(float *)il + f, (float *)ir + f}, *ob[2] = {ol + f, or_ + f};
    clap_audio_buffer_t ai = {ib, NULL, 2, 0, 0}, ao = {ob, NULL, 2, 0, 0};
    clap_process_t pr = {.steady_time = f, .frames_count = n, .audio_inputs = &ai, .audio_outputs = &ao,
                         .audio_inputs_count = 1, .audio_outputs_count = 1, .in_events = f ? &in_none : &in,
                         .out_events = &out};
    ok = p->process(p, &pr) != CLAP_PROCESS_ERROR;
  }
  p->stop_processing(p);
  p->deactivate(p);
  p->destroy(p);
  return ok;
}

static bool clap_open(ClapFace *c, const char *path) {
  c->lib = dlopen(path, RTLD_NOW | RTLD_LOCAL);
  if (!c->lib) return fprintf(stderr, "FAIL dlopen %s: %s\n", path, dlerror()), false;
  c->entry = (const clap_plugin_entry_t *)dlsym(c->lib, "clap_entry");
  if (!c->entry || !c->entry->init(path)) return fprintf(stderr, "FAIL clap_entry %s\n", path), false;
  c->factory = (const clap_plugin_factory_t *)c->entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
  if (!c->factory || c->factory->get_plugin_count(c->factory) != 1) return fprintf(stderr, "FAIL factory\n"), false;
  return true;
}

static int clap_main(const char *path) {
  ClapFace c;
  if (!clap_open(&c, path)) return 1;
  oracle("clap", &c, clap_run);
  c.entry->deinit();
  dlclose(c.lib);
  return failures ? 1 : 0;
}

/** One line per parameter, as clap-params-check.mjs reads them: id, name, min, max, default, flags. */
static int clap_params(const char *path) {
  ClapFace c;
  if (!clap_open(&c, path)) return 1;
  const clap_plugin_t *p = c.factory->create_plugin(c.factory, &HOST, c.factory->get_plugin_descriptor(c.factory, 0)->id);
  if (!p || !p->init(p)) return fprintf(stderr, "FAIL create/init\n"), 1;
  const clap_plugin_params_t *pp = (const clap_plugin_params_t *)p->get_extension(p, CLAP_EXT_PARAMS);
  if (!pp) return fprintf(stderr, "FAIL no params extension\n"), 1;
  for (uint32_t i = 0; i < pp->count(p); i++) {
    clap_param_info_t info;
    if (!pp->get_info(p, i, &info)) return fprintf(stderr, "FAIL get_info %u\n", i), 1;
    printf("%u\t%s\t%.17g\t%.17g\t%.17g\t%u\n", info.id, info.name, info.min_value, info.max_value, info.default_value,
           (unsigned)info.flags);
  }
  p->destroy(p);
  c.entry->deinit();
  dlclose(c.lib);
  return 0;
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

static bool lv2_run(void *face, double sr, const Setting *s, const float *il, const float *ir, float *ol, float *or_,
                    uint32_t frames) {
  Lv2Face *l = (Lv2Face *)face;
  LilvInstance *inst = lilv_plugin_instantiate(l->plugin, sr, NULL);
  if (!inst) return false;
  const char *audio[] = {"in_l", "in_r", "out_l", "out_r"};
  float cv[N_PARAMS], enabled = s->bypass ? 0.0f : 1.0f, latency = -1.0f;
  memcpy(cv, s->v, sizeof cv);
  int32_t ap[4];
  bool ok = true;
  for (int i = 0; i < 4; i++) ok = (ap[i] = port_of(l, audio[i])) >= 0 && ok;
  for (uint32_t i = 0; i < N_PARAMS; i++) {
    const int32_t idx = port_of(l, OMX_STRIP_PARAMS[i].symbol);
    ok = idx >= 0 && ok;
    if (idx >= 0) lilv_instance_connect_port(inst, (uint32_t)idx, &cv[i]);
  }
  const int32_t pe = port_of(l, "enabled"), pl = port_of(l, "latency");
  ok = pe >= 0 && pl >= 0 && ok;
  if (ok) {
    lilv_instance_connect_port(inst, (uint32_t)pe, &enabled);
    lilv_instance_connect_port(inst, (uint32_t)pl, &latency);
    lilv_instance_activate(inst);
  }
  for (uint32_t f = 0; ok && f < frames; f += BLOCK) {
    const uint32_t n = frames - f < BLOCK ? frames - f : BLOCK;
    lilv_instance_connect_port(inst, (uint32_t)ap[0], (void *)(il + f));
    lilv_instance_connect_port(inst, (uint32_t)ap[1], (void *)(ir + f));
    lilv_instance_connect_port(inst, (uint32_t)ap[2], ol + f);
    lilv_instance_connect_port(inst, (uint32_t)ap[3], or_ + f);
    lilv_instance_run(inst, n);
  }
  if (ok) lilv_instance_deactivate(inst);
  lilv_instance_free(inst);
  return ok && latency >= 0.0f;
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
  oracle("lv2", &l, lv2_run);
  lilv_node_free(u), lilv_node_free(b);
  lilv_world_free(l.world);
  return failures ? 1 : 0;
}

int main(int argc, char **argv) {
  int rc;
  if (argc == 3 && strcmp(argv[1], "clap-params") == 0) return clap_params(argv[2]);
  if (argc == 3 && strcmp(argv[1], "clap") == 0) rc = clap_main(argv[2]);
  else if (argc == 4 && strcmp(argv[1], "lv2") == 0) rc = lv2_main(argv[2], argv[3]);
  else return fprintf(stderr, "usage: strip-oracle clap <x.clap> | lv2 <bundle-dir> <uri> | clap-params <x.clap>\n"), 2;
  printf("%s %s: %d failure(s)\n", rc ? "ORACLE-RED" : "ORACLE-GREEN", argv[1], failures);
  return rc;
}
