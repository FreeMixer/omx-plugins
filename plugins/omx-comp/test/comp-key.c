// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * comp-key.c: the compressor's key, through each BUILT face at the nine rates an audio interface clocks
 * at (OMX_ORACLE_FULL_RATES, 32 to 192 kHz), read from the audio a host sees:
 *
 *   comp-key clap <omx-comp.clap>
 *   comp-key lv2 <bundle-dir> <uri>
 *
 *   A  identity   at the declared defaults, with a loud key routed to the sidechain and with none, the
 *                 output is omx-dsp's unkeyed compressor (omx_comp_instance_run at the same controls),
 *                 memcmp-identical: the key path, its filter, tilt, fold, hold and ducker are idle
 *   B  keyed      keySource sidechain: a programme sine at PROG_DB dBFS RMS, under the threshold, is held
 *                 down by the gain the static law gives for the KEY's level, a square wave at KEY_DB
 *                 dBFS (its square is constant, so the RMS detector reads exactly its amplitude):
 *                 -(K - T)(1 - 1/R), within GAIN_TOL dB, measured as the output's RMS over the input's
 *   C  no key     keySource sidechain with nothing routed to the key: the programme passes at 0 dB
 *   D  self       keySource self with the same loud key routed: the programme passes at 0 dB
 *   E  duck       duck on, rangeDb DUCK_DB: the same key holds the programme at DUCK_DB, not at the law
 *   F  meters     (CLAP) after the keyed run, compKey reads the key's level and compGr the law's reduction
 *
 * The threshold, ratio, knee and times are the declared defaults; the key's level is chosen above the
 * threshold by more than half the knee, so the law is the straight line past the knee.
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

#include "omx_comp_params.h"

#include <omx-clap-host/clap_meters.h>
#include <omxdsp/fx/omx_comp_instance.h>
#include <omxdsp/omx_denormal.h>

#define NP OMX_COMP_PARAM_COUNT
#define BLOCK 256u
#define RUN_S 1.5           /* seconds: the detector settles well inside it at the declared attack and release */
#define MEASURE_S 0.25      /* the last this many seconds are measured */
#define PROG_DB (-40.0)     /* the programme's RMS level, under the default threshold */
#define KEY_DB (-6.0)       /* the key's level, a square wave, above the threshold by more than half the knee */
#define PROG_HZ 1000.0
#define KEY_HZ 220.0
#define DUCK_DB (-6.0)
#define GAIN_TOL 0.05       /* dB: the detector settled on a square key, the programme sine measured over MEASURE_S */
#define METER_TOL 0.05      /* dB: a meter is one block's reading of the settled detector */
#define MAXLEN 288000u      /* RUN_S at 192 kHz */
#define KEY_METER 3u        /* compKey, the declaration's fourth meter */
#define GR_METER 0u         /* compGr */

static const float RATES[OMX_ORACLE_FULL_RATES_COUNT] = OMX_ORACLE_FULL_RATES_INIT;

static int g_checks = 0, g_failed = 0;
static const char *g_face = "";
static double g_sr = 0.0;

static void ok(bool cond, const char *what) {
  g_checks++;
  if (!cond) g_failed++;
  printf("%s %s %s @ %.0f Hz\n", cond ? "PASS" : "FAIL", g_face, what, g_sr);
}

/* ---- a face: one instance, its controls, its blocks, its meters ------------------------------- */

typedef struct Inst Inst;
typedef struct {
  Inst *(*open)(void *ctx, double sr);
  bool (*run)(Inst *in, const float *v, const float *key, const float *il, const float *ir, float *ol, float *orr,
              uint32_t n);
  /* the meter's value after the last block; false where the face has no meters (LV2) */
  bool (*meter)(Inst *in, uint32_t id, double *value);
  void (*close)(Inst *in);
  void *ctx;
} Face;

static float g_il[MAXLEN], g_ir[MAXLEN], g_key[MAXLEN], g_ol[MAXLEN], g_or[MAXLEN], g_wl[MAXLEN], g_wr[MAXLEN];

static void defaults(float *v) {
  for (uint32_t i = 0; i < NP; i++) v[i] = OMX_COMP_PARAMS[i].def;
}

/* The whole run through one fresh face in BLOCK-frame pieces; the key, or NULL. */
static bool run_face(const Face *f, const float *v, const float *key, uint32_t n, Inst **keep) {
  Inst *in = f->open(f->ctx, g_sr);
  if (!in) return false;
  bool good = true;
  for (uint32_t at = 0; good && at < n; at += BLOCK) {
    const uint32_t k = n - at < BLOCK ? n - at : BLOCK;
    good = f->run(in, v, key ? key + at : NULL, g_il + at, g_ir + at, g_ol + at, g_or + at, k);
  }
  if (keep && good) *keep = in;
  else f->close(in);
  return good;
}

/* omx-dsp's compressor, unkeyed, at the same controls: today's comp. */
static void reference(const float *v, uint32_t n) {
  static OmxCompInstance s;
  omx_comp_instance_init(&s, (float)g_sr);
  for (uint32_t at = 0; at < n; at += BLOCK) {
    const uint32_t k = n - at < BLOCK ? n - at : BLOCK;
    omx_comp_instance_resolve(&s, 0, v[OMX_COMP_PARAM_THRESHOLD_DB], v[OMX_COMP_PARAM_RATIO], v[OMX_COMP_PARAM_KNEE_DB],
                              v[OMX_COMP_PARAM_ATTACK_MS], v[OMX_COMP_PARAM_RELEASE_MS], v[OMX_COMP_PARAM_MAKEUP_DB],
                              v[OMX_COMP_PARAM_MIX_PCT], (int)v[OMX_COMP_PARAM_KIND], (int)v[OMX_COMP_PARAM_DETECTOR_OVERSAMPLING],
                              (int)v[OMX_COMP_PARAM_DETECTOR_TOPOLOGY], (int)v[OMX_COMP_PARAM_KEY_SOURCE],
                              v[OMX_COMP_PARAM_KEY_HP_HZ], v[OMX_COMP_PARAM_KEY_LP_HZ], v[OMX_COMP_PARAM_KEY_TILT_DB],
                              (int)v[OMX_COMP_PARAM_KEY_LISTEN], v[OMX_COMP_PARAM_HOLD_MS], (int)v[OMX_COMP_PARAM_DETECT_LINK],
                              (int)v[OMX_COMP_PARAM_DUCK], v[OMX_COMP_PARAM_RANGE_DB]);
    omx_comp_instance_run(&s, g_il + at, g_ir + at, g_wl + at, g_wr + at, k);
  }
}

static double rms_db(const float *x, uint32_t from, uint32_t n) {
  double e = 0.0;
  for (uint32_t i = from; i < n; i++) e += (double)x[i] * (double)x[i];
  return 10.0 * log10(e / (double)(n - from));
}

/* A sine at `db` dBFS RMS. */
static void sine(float *x, uint32_t n, double hz, double db) {
  const double a = pow(10.0, db / 20.0) * sqrt(2.0);
  for (uint32_t i = 0; i < n; i++) x[i] = (float)(a * sin(2.0 * M_PI * hz * (double)i / g_sr));
}

/* A square wave at `db` dBFS: its square is constant, so the RMS detector, whatever its attack and release,
 * settles on exactly its amplitude, and the static law at that level is the closed form. */
static void square(float *x, uint32_t n, double hz, double db) {
  const float a = (float)pow(10.0, db / 20.0);
  const uint32_t half = (uint32_t)lrint(g_sr / hz / 2.0);
  for (uint32_t i = 0; i < n; i++) x[i] = (i / half) % 2u ? -a : a;
}

/* Noise bursts on a quiet bed, the legs unequal: the identity arm's programme, compressed at the defaults. */
static void bursts(float *l, float *r, uint32_t n) {
  uint32_t seed = 0x636f6d70u;
  const uint32_t period = (uint32_t)(0.09 * g_sr);
  for (uint32_t i = 0; i < n; i++) {
    seed = seed * 1664525u + 1013904223u;
    const float a = (float)(seed >> 8) / 16777216.0f - 0.5f;
    seed = seed * 1664525u + 1013904223u;
    const float c = (float)(seed >> 8) / 16777216.0f - 0.5f;
    const float env = expf(-(float)(i % period) / (0.012f * (float)g_sr));
    l[i] = 1.8f * env * a + 0.02f * c;
    r[i] = 1.4f * env * c + 0.02f * a;
  }
}

static void all_arms(const char *name, const Face *f) {
  g_face = name;
  char what[240];
  for (uint32_t ri = 0; ri < OMX_ORACLE_FULL_RATES_COUNT; ri++) {
    g_sr = RATES[ri];
    const uint32_t n = (uint32_t)(RUN_S * g_sr), from = n - (uint32_t)(MEASURE_S * g_sr);
    float v[NP];

    /* A: identity at the defaults, keyed and not */
    defaults(v);
    bursts(g_il, g_ir, n);
    for (uint32_t i = 0; i < n; i++) g_key[i] = 0.9f * (float)sin(2.0 * M_PI * KEY_HZ * (double)i / g_sr);
    reference(v, n);
    bool moved = memcmp(g_wl, g_il, n * sizeof(float)) != 0;
    ok(moved, "A: the unkeyed compressor shapes the programme at the defaults (the identity is not vacuous)");
    bool ran = run_face(f, v, g_key, n, NULL);
    ok(ran && memcmp(g_ol, g_wl, n * sizeof(float)) == 0 && memcmp(g_or, g_wr, n * sizeof(float)) == 0,
       "A: at the defaults, a loud key routed, the output is omx-dsp's unkeyed compressor, memcmp-identical");
    ran = run_face(f, v, NULL, n, NULL);
    ok(ran && memcmp(g_ol, g_wl, n * sizeof(float)) == 0 && memcmp(g_or, g_wr, n * sizeof(float)) == 0,
       "A: at the defaults, no key routed, the output is omx-dsp's unkeyed compressor, memcmp-identical");

    /* B: keyed by the sidechain, the programme under the threshold */
    sine(g_il, n, PROG_HZ, PROG_DB);
    memcpy(g_ir, g_il, n * sizeof(float));
    square(g_key, n, KEY_HZ, KEY_DB);
    defaults(v);
    v[OMX_COMP_PARAM_KEY_SOURCE] = 1.0f;
    const double T = v[OMX_COMP_PARAM_THRESHOLD_DB], R = v[OMX_COMP_PARAM_RATIO];
    const double law = -(KEY_DB - T) * (1.0 - 1.0 / R);
    Inst *kept = NULL;
    ran = run_face(f, v, g_key, n, &kept);
    const double in_db = rms_db(g_il, from, n), gl = rms_db(g_ol, from, n) - in_db, gr = rms_db(g_or, from, n) - in_db;
    snprintf(what, sizeof what, "B: keyed, programme %.2f dBFS, key %.2f dBFS square: gain %.3f/%.3f dB, the law at the key %.3f dB (threshold %.0f, ratio %.0f)",
             in_db, KEY_DB, gl, gr, law, T, R);
    ok(ran && fabs(gl - law) <= GAIN_TOL && fabs(gr - law) <= GAIN_TOL, what);

    /* F: the meters of that run (CLAP) */
    double key_level, gr_db;
    if (kept && f->meter(kept, KEY_METER, &key_level) && f->meter(kept, GR_METER, &gr_db)) {
      snprintf(what, sizeof what, "F: after the keyed run compKey reads %.3f dBFS (the key %.2f) and compGr %.3f dB (the law %.3f)", key_level,
               KEY_DB, gr_db, law);
      ok(fabs(key_level - KEY_DB) <= METER_TOL && fabs(gr_db - law) <= METER_TOL, what);
    }
    if (kept) f->close(kept);

    /* C: the sidechain listened to, nothing routed: the programme's own level, under the threshold */
    ran = run_face(f, v, NULL, n, NULL);
    double g0 = rms_db(g_ol, from, n) - in_db;
    snprintf(what, sizeof what, "C: keySource sidechain, no key routed: gain %.4f dB", g0);
    ok(ran && fabs(g0) <= 0.01, what);

    /* D: the key routed, the detector on the programme */
    v[OMX_COMP_PARAM_KEY_SOURCE] = 0.0f;
    ran = run_face(f, v, g_key, n, NULL);
    g0 = rms_db(g_ol, from, n) - in_db;
    snprintf(what, sizeof what, "D: keySource self, the key routed: gain %.4f dB", g0);
    ok(ran && fabs(g0) <= 0.01, what);

    /* E: the ducker holds the reduction to its range */
    v[OMX_COMP_PARAM_KEY_SOURCE] = 1.0f;
    v[OMX_COMP_PARAM_DUCK] = 1.0f;
    v[OMX_COMP_PARAM_RANGE_DB] = (float)DUCK_DB;
    ran = run_face(f, v, g_key, n, NULL);
    const double gd = rms_db(g_ol, from, n) - in_db;
    snprintf(what, sizeof what, "E: duck on, range %.0f dB, the law asking %.3f dB: gain %.3f dB", DUCK_DB, law, gd);
    ok(ran && fabs(gd - DUCK_DB) <= GAIN_TOL, what);
  }
}

/* ---- CLAP ---------------------------------------------------------------------------------------- */

static const void *h_ext(const clap_host_t *h, const char *id) {
  (void)h, (void)id;
  return NULL;
}
static void h_noop(const clap_host_t *h) { (void)h; }
static const clap_host_t HOST = {CLAP_VERSION_INIT, NULL, "comp-key", "openmixer", "", "0.1", h_ext, h_noop, h_noop, h_noop};

struct Inst {
  const clap_plugin_t *p;
  const clap_plugin_meters_t *mx;
  clap_id ids[NP + 1];
  uint64_t steady;
  LilvInstance *lv;
  int32_t ap[5], cp[NP + 2];
  float cv[NP + 2];
};

typedef struct {
  clap_event_param_value_t ev[NP + 1];
  uint32_t n;
} Events;
static uint32_t ev_size(const clap_input_events_t *l) { return ((const Events *)l->ctx)->n; }
static const clap_event_header_t *ev_get(const clap_input_events_t *l, uint32_t i) { return &((const Events *)l->ctx)->ev[i].header; }
static bool ev_push(const clap_output_events_t *l, const clap_event_header_t *e) {
  (void)l, (void)e;
  return true;
}

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

static void clap_close(Inst *in) {
  in->p->stop_processing(in->p);
  in->p->deactivate(in->p);
  in->p->destroy(in->p);
  free(in);
}

static Inst *clap_open(void *ctx, double sr) {
  const clap_plugin_factory_t *factory = ctx;
  Inst *in = calloc(1, sizeof *in);
  if (!in) abort();
  in->p = factory->create_plugin(factory, &HOST, factory->get_plugin_descriptor(factory, 0)->id);
  if (!in->p || !in->p->init(in->p)) return free(in), NULL;
  const clap_plugin_params_t *pp = (const clap_plugin_params_t *)in->p->get_extension(in->p, CLAP_EXT_PARAMS);
  in->mx = (const clap_plugin_meters_t *)in->p->get_extension(in->p, CLAP_EXT_METERS);
  bool good = pp != NULL;
  for (uint32_t i = 0; good && i < NP; i++) good = clap_param(pp, in->p, OMX_COMP_PARAMS[i].name, &in->ids[i]);
  good = good && clap_param(pp, in->p, NULL, &in->ids[NP]);
  if (!good || !in->p->activate(in->p, sr, 1, BLOCK) || !in->p->start_processing(in->p)) {
    in->p->destroy(in->p);
    return free(in), NULL;
  }
  return in;
}

static bool clap_run(Inst *in, const float *v, const float *key, const float *il, const float *ir, float *ol, float *orr,
                     uint32_t n) {
  Events evs = {.n = 0};
  for (uint32_t i = 0; i <= NP; i++) {
    clap_event_param_value_t *e = &evs.ev[evs.n++];
    memset(e, 0, sizeof *e);
    e->header = (clap_event_header_t){sizeof *e, 0, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_PARAM_VALUE, 0};
    e->param_id = in->ids[i], e->note_id = -1, e->port_index = -1, e->channel = -1, e->key = -1;
    e->value = i < NP ? (double)v[i] : 0.0;
  }
  clap_input_events_t ie = {&evs, ev_size, ev_get};
  clap_output_events_t oe = {NULL, ev_push};
  float *ib[2] = {(float *)il, (float *)ir}, *ob[2] = {ol, orr}, *kb[1] = {(float *)key};
  clap_audio_buffer_t ai[2] = {{ib, NULL, 2, 0, 0}, {kb, NULL, 1, 0, 0}}, ao = {ob, NULL, 2, 0, 0};
  clap_process_t pr = {.steady_time = (int64_t)in->steady, .frames_count = n, .audio_inputs = ai, .audio_outputs = &ao,
                       .audio_inputs_count = key ? 2u : 1u, .audio_outputs_count = 1, .in_events = &ie, .out_events = &oe};
  in->steady += n;
  return in->p->process(in->p, &pr) != CLAP_PROCESS_ERROR;
}

static bool clap_meter(Inst *in, uint32_t id, double *value) {
  return in->mx && in->mx->get_values(in->p, id, value, 1) == 1;
}

static int clap_main(const char *path) {
  void *lib = dlopen(path, RTLD_NOW | RTLD_LOCAL);
  if (!lib) return fprintf(stderr, "FAIL dlopen %s: %s\n", path, dlerror()), 1;
  const clap_plugin_entry_t *entry = (const clap_plugin_entry_t *)dlsym(lib, "clap_entry");
  if (!entry || !entry->init(path)) return fprintf(stderr, "FAIL clap_entry %s\n", path), 1;
  const clap_plugin_factory_t *factory = (const clap_plugin_factory_t *)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
  if (!factory || factory->get_plugin_count(factory) != 1) return fprintf(stderr, "FAIL factory\n"), 1;
  Face f = {clap_open, clap_run, clap_meter, clap_close, (void *)factory};
  all_arms("clap", &f);
  entry->deinit();
  dlclose(lib);
  return g_failed ? 1 : 0;
}

/* ---- LV2 ----------------------------------------------------------------------------------------- */

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

static void lv2_close(Inst *in) {
  lilv_instance_deactivate(in->lv);
  lilv_instance_free(in->lv);
  free(in);
}

static Inst *lv2_open(void *ctx, double sr) {
  const Lv2Face *l = ctx;
  Inst *in = calloc(1, sizeof *in);
  if (!in) abort();
  const char *audio[] = {"in_l", "in_r", "out_l", "out_r", "key"};
  bool good = true;
  for (int i = 0; i < 5; i++) good = (in->ap[i] = port_of(l, audio[i])) >= 0 && good;
  for (uint32_t i = 0; i < NP; i++) good = (in->cp[i] = port_of(l, OMX_COMP_PARAMS[i].symbol)) >= 0 && good;
  good = (in->cp[NP] = port_of(l, "enabled")) >= 0 && good;
  good = (in->cp[NP + 1] = port_of(l, "latency")) >= 0 && good;
  if (!good || !(in->lv = lilv_plugin_instantiate(l->plugin, sr, NULL))) return free(in), NULL;
  for (uint32_t i = 0; i < NP + 2; i++) lilv_instance_connect_port(in->lv, (uint32_t)in->cp[i], &in->cv[i]);
  lilv_instance_activate(in->lv);
  return in;
}

static bool lv2_run(Inst *in, const float *v, const float *key, const float *il, const float *ir, float *ol, float *orr,
                    uint32_t n) {
  memcpy(in->cv, v, NP * sizeof(float));
  in->cv[NP] = 1.0f; /* enabled */
  lilv_instance_connect_port(in->lv, (uint32_t)in->ap[0], (void *)il);
  lilv_instance_connect_port(in->lv, (uint32_t)in->ap[1], (void *)ir);
  lilv_instance_connect_port(in->lv, (uint32_t)in->ap[2], ol);
  lilv_instance_connect_port(in->lv, (uint32_t)in->ap[3], orr);
  lilv_instance_connect_port(in->lv, (uint32_t)in->ap[4], (void *)key);
  lilv_instance_run(in->lv, n);
  return true;
}

static bool lv2_meter(Inst *in, uint32_t id, double *value) {
  (void)in, (void)id, (void)value;
  return false;
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
  Face f = {lv2_open, lv2_run, lv2_meter, lv2_close, &l};
  all_arms("lv2", &f);
  lilv_node_free(u), lilv_node_free(b);
  lilv_world_free(l.world);
  return g_failed ? 1 : 0;
}

int main(int argc, char **argv) {
  omx_denormals_off(); /* the faces run with flush-to-zero; so does the reference */
  int rc;
  if (argc == 3 && strcmp(argv[1], "clap") == 0) rc = clap_main(argv[2]);
  else if (argc == 4 && strcmp(argv[1], "lv2") == 0) rc = lv2_main(argv[2], argv[3]);
  else return fprintf(stderr, "usage: comp-key clap <x.clap> | lv2 <bundle-dir> <uri>\n"), 2;
  printf("%s %s: %d checks, %d failed, %u rates\n", rc ? "ORACLE-RED" : "ORACLE-GREEN", argv[1], g_checks, g_failed,
         OMX_ORACLE_FULL_RATES_COUNT);
  return rc;
}
