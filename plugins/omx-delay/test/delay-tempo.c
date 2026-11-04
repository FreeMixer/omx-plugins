// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * delay-tempo.c: the delay's tempo sync, taps, ping-pong and duck through each BUILT face at the nine rates an
 * audio interface clocks at (OMX_ORACLE_FULL_RATES, 32 to 192 kHz), read from the audio a host sees, the tempo
 * handed the way a host hands it (CLAP: clap_process.transport; LV2: a time:Position on the `control` port):
 *
 *   delay-tempo clap <omx-delay.clap>
 *   delay-tempo lv2 <bundle-dir> <uri>
 *
 *   A  identity   at the declared defaults (sync free) the face is omx-dsp's delay at the old seventeen controls,
 *                 memcmp-identical, with a tempo handed and without: a session saved before sync existed sounds
 *                 the same
 *   B  tempo      sync tempo, every division (1/1 to 1/32, straight, dotted, triplet) at 90, 120 and 137.5 BPM:
 *                 an impulse's first echo lands at round(60000 / BPM x beats x rate / 1000) samples, held to the
 *                 time travel's top, at unity (mix 1, feedback 0)
 *   C  no tempo   sync tempo, Time 137 ms: with no transport, a transport without a tempo (CLAP: no
 *                 CLAP_TRANSPORT_HAS_TEMPO; LV2: a time:Position without time:beatsPerMinute), the echo lands at
 *                 the Time control's sample
 *   D  held       (LV2) a time:Position handed in the first block only: the tempo holds in the blocks after it
 *   E  ping-pong  an impulse in the left leg, feedback 0.5, synced to a quarter at 120 BPM (d samples): 1 in the
 *                 left at d, 0.5 in the right at 2d, 0.25 in the left at 3d, nothing anywhere else
 *   F  taps       four taps at a quarter of 120 and 90 BPM: tap k's echo at round(T x factor_k x rate / 1000), the
 *                 whole output equal to omx-dsp's delay handed the same tempo and controls, memcmp-identical
 *   G  duck       a held 0.5 (-6.02 dBFS) input, duck -12 dB over a -30 dBFS threshold: the repeats settle exactly
 *                 12 dB under the unducked ones (within 0.01 dB)
 *
 * Before the arms, a self-test runs arms B and C with the tempo handed WRONG (CLAP: the tempo without its flag; LV2: the
 * position's type changed) and fails unless both arms go red, so a check that cannot see the tempo is not
 * mistaken for one that passes.
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
#include <lv2/atom/atom.h>
#include <lv2/atom/forge.h>
#include <lv2/time/time.h>
#include <lv2/urid/urid.h>

#include "omx_delay_params.h"

#include <omxdsp/fx/omx_delay_instance.h>
#include <omxdsp/omx_denormal.h>

#define NP OMX_DELAY_PARAM_COUNT
#define BLOCK 256u
#define MAXLEN 400000u /* the 2 s time travel at 192 kHz, and room after it */

static const float RATES[OMX_ORACLE_FULL_RATES_COUNT] = OMX_ORACLE_FULL_RATES_INIT;
/* The beats of each division, stated here so a moved declaration turns arm B red. */
static const double BEATS[] = {4,   6,    8.0 / 3.0, 2,    3,     4.0 / 3.0, 1,     1.5,    2.0 / 3.0,
                               0.5, 0.75, 1.0 / 3.0, 0.25, 0.375, 1.0 / 6.0, 0.125, 0.1875, 1.0 / 12.0};
_Static_assert(sizeof BEATS / sizeof BEATS[0] == OMX_DELAY_DIVISIONS_COUNT, "the oracle states every declared division");
static const double BPMS[] = {90.0, 120.0, 137.5};

static int g_checks = 0, g_failed = 0, g_quiet = 0;
static const char *g_face = "";
static double g_sr = 0.0;

static void ok(bool cond, const char *what) {
  g_checks++;
  if (!cond) g_failed++;
  if (!g_quiet) printf("%s %s %s @ %.0f Hz\n", cond ? "PASS" : "FAIL", g_face, what, g_sr);
}

/* How a block's tempo reaches the face. */
enum tempo_mode {
  T_NONE,   /* CLAP: no transport; LV2: the control port unconnected */
  T_TEMPO,  /* CLAP: a transport with its tempo; LV2: a time:Position with time:beatsPerMinute, every block */
  T_NOFLAG, /* CLAP: a transport without CLAP_TRANSPORT_HAS_TEMPO; LV2: a time:Position without a tempo */
  T_ONCE,   /* LV2: a time:Position with a tempo in the first block, an empty sequence after */
  T_WRONG,  /* --self-test: the tempo handed in a way the face must not read (see the header) */
};

typedef struct Inst Inst;
typedef struct {
  Inst *(*open)(void *ctx, double sr);
  bool (*run)(Inst *in, const float *v, int mode, double bpm, uint32_t block, const float *il, const float *ir,
              float *ol, float *orr, uint32_t n);
  void (*close)(Inst *in);
  void *ctx;
  bool lv2;
} Face;

static float g_il[MAXLEN], g_ir[MAXLEN], g_ol[MAXLEN], g_or[MAXLEN], g_wl[MAXLEN], g_wr[MAXLEN];

static void defaults(float *v) {
  for (uint32_t i = 0; i < NP; i++) v[i] = OMX_DELAY_PARAMS[i].def;
}

/* The whole run through one fresh face in BLOCK-frame pieces. */
static bool run_face(const Face *f, const float *v, int mode, double bpm, uint32_t n) {
  Inst *in = f->open(f->ctx, g_sr);
  if (!in) return false;
  bool good = true;
  for (uint32_t at = 0, b = 0; good && at < n; at += BLOCK, b++) {
    const uint32_t k = n - at < BLOCK ? n - at : BLOCK;
    good = f->run(in, v, mode, bpm, b, g_il + at, g_ir + at, g_ol + at, g_or + at, k);
  }
  f->close(in);
  return good;
}

/* omx-dsp's delay at the same controls, handed `bpm` (0: none), into g_wl/g_wr. */
static void reference(const float *v, double bpm, uint32_t n) {
  static OmxDelayInstance s;
  omx_delay_instance_init(&s, (float)g_sr);
  for (uint32_t at = 0; at < n; at += BLOCK) {
    const uint32_t k = n - at < BLOCK ? n - at : BLOCK;
    omx_delay_instance_set_tempo(&s, (float)bpm);
    omx_delay_instance_resolve(
        &s, 0, v[OMX_DELAY_PARAM_FX_DELAY_TIME], v[OMX_DELAY_PARAM_FX_DELAY_FEEDBACK], v[OMX_DELAY_PARAM_TONE],
        v[OMX_DELAY_PARAM_MIX], (int)lrintf(v[OMX_DELAY_PARAM_PINGPONG]), v[OMX_DELAY_PARAM_FX_DELAY_TAPS],
        v[OMX_DELAY_PARAM_TAP0_GAIN], v[OMX_DELAY_PARAM_TAP0_PAN], (int)lrintf(v[OMX_DELAY_PARAM_TAP1_FACTOR]),
        v[OMX_DELAY_PARAM_TAP1_GAIN], v[OMX_DELAY_PARAM_TAP1_PAN], (int)lrintf(v[OMX_DELAY_PARAM_TAP2_FACTOR]),
        v[OMX_DELAY_PARAM_TAP2_GAIN], v[OMX_DELAY_PARAM_TAP2_PAN], (int)lrintf(v[OMX_DELAY_PARAM_TAP3_FACTOR]),
        v[OMX_DELAY_PARAM_TAP3_GAIN], v[OMX_DELAY_PARAM_TAP3_PAN], (int)lrintf(v[OMX_DELAY_PARAM_SYNC]),
        (int)lrintf(v[OMX_DELAY_PARAM_DIVISION]), (int)lrintf(v[OMX_DELAY_PARAM_MODE]), v[OMX_DELAY_PARAM_DUCK_DB],
        v[OMX_DELAY_PARAM_DUCK_THRESHOLD_DB], v[OMX_DELAY_PARAM_DUCK_RELEASE_MS]);
    memcpy(g_wl + at, g_il + at, k * sizeof(float));
    memcpy(g_wr + at, g_ir + at, k * sizeof(float));
    omx_delay_instance_run(&s, g_wl + at, g_wr + at, g_wl + at, g_wr + at, k);
  }
}

/* The sample of `ms` at this rate, as a host counts it: round(ms x rate / 1000), ms held to the travel's top. */
static uint32_t at_ms(double ms) {
  if (ms > OMX_DELAY_PARAM_FX_DELAY_TIME_MAX) ms = OMX_DELAY_PARAM_FX_DELAY_TIME_MAX;
  return (uint32_t)floor(ms * g_sr / 1000.0 + 0.5);
}

static void impulse(uint32_t n, bool right) {
  memset(g_il, 0, n * sizeof(float));
  memset(g_ir, 0, n * sizeof(float));
  g_il[0] = 1.0f;
  if (right) g_ir[0] = 1.0f;
}

/* The first non-zero sample of `x`, or n. */
static uint32_t first(const float *x, uint32_t n) {
  uint32_t i = 0;
  while (i < n && x[i] == 0.0f) i++;
  return i;
}

/* A synced unity echo: mix 1, feedback 0, tone 0. */
static void echo_controls(float *v, int sync, int division, float time_ms) {
  defaults(v);
  v[OMX_DELAY_PARAM_SYNC] = (float)sync;
  v[OMX_DELAY_PARAM_DIVISION] = (float)division;
  v[OMX_DELAY_PARAM_FX_DELAY_TIME] = time_ms;
  v[OMX_DELAY_PARAM_FX_DELAY_FEEDBACK] = 0.0f;
  v[OMX_DELAY_PARAM_TONE] = 0.0f;
  v[OMX_DELAY_PARAM_MIX] = 1.0f;
}

static void arm_tempo(const Face *f, int mode) {
  char what[240];
  float v[NP];
  bool all = true;
  for (unsigned b = 0; b < sizeof BPMS / sizeof BPMS[0]; b++)
    for (int dv = 0; dv < (int)OMX_DELAY_DIVISIONS_COUNT; dv++) {
      echo_controls(v, OMX_DELAY_SYNCS_TEMPO, dv, 300.0f);
      const uint32_t want = at_ms(60000.0 / BPMS[b] * BEATS[dv]), n = want + 64u;
      impulse(n, true);
      const bool ran = run_face(f, v, mode, BPMS[b], n);
      const uint32_t got = first(g_ol, n);
      const bool good = ran && got == want && g_ol[got] == 1.0f && first(g_or, n) == want;
      all = all && good;
      if (!good) {
        snprintf(what, sizeof what, "B: division %d at %.1f BPM: the echo at %u, want %u", dv, BPMS[b], got, want);
        ok(false, what);
      }
    }
  snprintf(what, sizeof what, "B: every division (%u) at 90, 120 and 137.5 BPM: the echo at round(60000 / BPM x beats x rate / 1000)",
           (unsigned)OMX_DELAY_DIVISIONS_COUNT);
  ok(all, what);
}

static void arm_no_tempo(const Face *f, int mode, const char *how) {
  char what[240];
  float v[NP];
  echo_controls(v, OMX_DELAY_SYNCS_TEMPO, OMX_DELAY_DIVISIONS_EIGHTH, 137.0f);
  const uint32_t want = at_ms(137.0), n = want + 64u;
  impulse(n, true);
  const bool ran = run_face(f, v, mode, 120.0, n);
  const uint32_t got = first(g_ol, n);
  snprintf(what, sizeof what, "C: sync tempo, %s: the echo at the Time control's sample %u (got %u; an eighth at 120 BPM is %u)", how,
           want, got, at_ms(250.0));
  ok(ran && got == want && g_ol[got] == 1.0f, what);
}

static void all_arms(const char *name, const Face *f) {
  g_face = name;
  char what[240];
  for (uint32_t ri = 0; ri < OMX_ORACLE_FULL_RATES_COUNT; ri++) {
    g_sr = RATES[ri];
    float v[NP];

    /* A: the defaults are the delay that shipped, with a tempo handed or not */
    const uint32_t na = (uint32_t)(0.8 * g_sr);
    uint32_t seed = 0x64656cu;
    for (uint32_t i = 0; i < na; i++) {
      seed = seed * 1664525u + 1013904223u;
      g_il[i] = (float)(seed >> 8) / 16777216.0f - 0.5f;
      seed = seed * 1664525u + 1013904223u;
      g_ir[i] = (float)(seed >> 8) / 16777216.0f - 0.5f;
    }
    defaults(v);
    ok(v[OMX_DELAY_PARAM_SYNC] == (float)OMX_DELAY_SYNCS_FREE && v[OMX_DELAY_PARAM_MODE] == (float)OMX_DELAY_MODES_DIGITAL &&
           v[OMX_DELAY_PARAM_DUCK_DB] == 0.0f,
       "A: the new controls come up free, digital, no duck");
    reference(v, 0.0, na);
    bool ran = run_face(f, v, T_NONE, 0.0, na);
    ok(ran && memcmp(g_ol, g_wl, na * sizeof(float)) == 0 && memcmp(g_or, g_wr, na * sizeof(float)) == 0,
       "A: at the defaults, no tempo: omx-dsp's delay at the old controls, memcmp-identical");
    ran = run_face(f, v, T_TEMPO, 97.0, na);
    ok(ran && memcmp(g_ol, g_wl, na * sizeof(float)) == 0 && memcmp(g_or, g_wr, na * sizeof(float)) == 0,
       "A: at the defaults, a tempo handed: unchanged (sync free ignores it)");

    /* B, C */
    arm_tempo(f, T_TEMPO);
    arm_no_tempo(f, T_NONE, f->lv2 ? "the control port unconnected" : "no transport");
    arm_no_tempo(f, T_NOFLAG, f->lv2 ? "a time:Position with no tempo" : "a transport without its tempo flag");

    /* D: an LV2 position's tempo holds */
    if (f->lv2) {
      echo_controls(v, OMX_DELAY_SYNCS_TEMPO, OMX_DELAY_DIVISIONS_QUARTER, 300.0f);
      const uint32_t want = at_ms(60000.0 / 90.0), n = want + 64u;
      impulse(n, true);
      ran = run_face(f, v, T_ONCE, 90.0, n);
      snprintf(what, sizeof what, "D: a time:Position in the first block only: the quarter at 90 BPM holds (echo at %u, want %u)",
               first(g_ol, n), want);
      ok(ran && first(g_ol, n) == want, what);
    }

    /* E: ping-pong by value */
    echo_controls(v, OMX_DELAY_SYNCS_TEMPO, OMX_DELAY_DIVISIONS_QUARTER, 300.0f);
    v[OMX_DELAY_PARAM_FX_DELAY_FEEDBACK] = 0.5f;
    v[OMX_DELAY_PARAM_PINGPONG] = 1.0f;
    {
      const uint32_t d = at_ms(500.0), n = 3u * d + 64u;
      impulse(n, false);
      ran = run_face(f, v, T_TEMPO, 120.0, n);
      bool clean = true;
      for (uint32_t i = 0; i < n; i++) {
        const float wl = i == d ? 1.0f : i == 3u * d ? 0.25f : 0.0f, wr = i == 2u * d ? 0.5f : 0.0f;
        clean = clean && g_ol[i] == wl && g_or[i] == wr;
      }
      snprintf(what, sizeof what, "E: ping-pong at a quarter of 120 BPM (d = %u): L 1 at d, R 0.5 at 2d, L 0.25 at 3d, zero elsewhere (got %.4f %.4f %.4f)",
               d, g_ol[d], g_or[2u * d], g_ol[3u * d]);
      ok(ran && clean, what);
    }

    /* F: the taps follow the tempo */
    for (unsigned b = 0; b < 2; b++) {
      const double bpm = b ? 90.0 : 120.0, t = 60000.0 / bpm;
      echo_controls(v, OMX_DELAY_SYNCS_TEMPO, OMX_DELAY_DIVISIONS_QUARTER, 300.0f);
      v[OMX_DELAY_PARAM_FX_DELAY_TAPS] = 4.0f;
      static const uint32_t NUM[] = OMX_DELAY_TAP_FACTOR_NUMS_INIT, DEN[] = OMX_DELAY_TAP_FACTOR_DENS_INIT;
      const int fk[3] = {(int)v[OMX_DELAY_PARAM_TAP1_FACTOR], (int)v[OMX_DELAY_PARAM_TAP2_FACTOR], (int)v[OMX_DELAY_PARAM_TAP3_FACTOR]};
      uint32_t want[4] = {at_ms(t), 0, 0, 0}, n = 0;
      for (int k = 0; k < 3; k++) want[k + 1] = at_ms(t * NUM[fk[k]] / DEN[fk[k]]);
      for (int k = 0; k < 4; k++) n = want[k] > n ? want[k] : n;
      n += 64u;
      impulse(n, true);
      ran = run_face(f, v, T_TEMPO, bpm, n);
      bool there = true;
      for (int k = 0; k < 4; k++) there = there && (g_ol[want[k]] != 0.0f || g_or[want[k]] != 0.0f);
      uint32_t nz = 0;
      for (uint32_t i = 0; i < n; i++) nz += g_ol[i] != 0.0f || g_or[i] != 0.0f;
      reference(v, bpm, n);
      snprintf(what, sizeof what, "F: four taps at a quarter of %.0f BPM: echoes at %u %u %u %u and only there, omx-dsp's delay memcmp-identical",
               bpm, want[0], want[1], want[2], want[3]);
      ok(ran && there && nz == 4u && memcmp(g_ol, g_wl, n * sizeof(float)) == 0 && memcmp(g_or, g_wr, n * sizeof(float)) == 0, what);
    }

    /* G: the duck by value */
    {
      const uint32_t n = (uint32_t)(1.5 * g_sr), from = n - (uint32_t)(0.1 * g_sr);
      for (uint32_t i = 0; i < n; i++) g_il[i] = g_ir[i] = 0.5f;
      double lvl[2];
      for (int dk = 0; dk < 2; dk++) {
        echo_controls(v, OMX_DELAY_SYNCS_FREE, 0, 100.0f);
        v[OMX_DELAY_PARAM_DUCK_DB] = dk ? -12.0f : 0.0f;
        v[OMX_DELAY_PARAM_DUCK_THRESHOLD_DB] = -30.0f;
        ran = run_face(f, v, T_NONE, 0.0, n);
        double e = 0.0;
        for (uint32_t i = from; i < n; i++) e += (double)g_ol[i] * g_ol[i];
        lvl[dk] = 10.0 * log10(e / (double)(n - from));
      }
      snprintf(what, sizeof what, "G: a held -6.02 dBFS input, duck -12 dB over -30 dBFS: the repeats %.4f dB under the unducked (want -12)",
               lvl[1] - lvl[0]);
      ok(ran && fabs(lvl[1] - lvl[0] + 12.0) <= 0.01, what);
    }
  }
}

/* The self-test: arms B and C with the tempo handed wrong must go red. */
static int self_test(const char *name, const Face *f) {
  g_face = name;
  g_quiet = 1;
  const int before = g_failed;
  g_sr = 48000.0;
  arm_tempo(f, T_WRONG);
  const bool b_red = g_failed > before;
  const int mid = g_failed;
  /* a face that read the wrong tempo as a tempo would put the echo at the eighth, not the Time control */
  arm_no_tempo(f, T_WRONG, "the tempo handed wrong");
  const bool c_green = g_failed == mid;
  g_quiet = 0;
  g_failed = before;
  printf("%s %s self-test: arm B %s with the tempo handed wrong, arm C %s\n", b_red && c_green ? "PASS" : "FAIL", name,
         b_red ? "went red" : "STAYED GREEN", c_green ? "held the Time control" : "read the wrong tempo");
  g_checks++;
  if (!(b_red && c_green)) g_failed++;
  return 0;
}

/* ---- CLAP ---------------------------------------------------------------------------------------- */

static const void *h_ext(const clap_host_t *h, const char *id) {
  (void)h, (void)id;
  return NULL;
}
static void h_noop(const clap_host_t *h) { (void)h; }
static const clap_host_t HOST = {CLAP_VERSION_INIT, NULL, "delay-tempo", "openmixer", "", "0.1", h_ext, h_noop, h_noop, h_noop};

struct Inst {
  const clap_plugin_t *p;
  clap_id ids[NP + 1];
  uint64_t steady;
  LilvInstance *lv;
  int32_t ap[4], cp[NP + 2], ctl;
  float cv[NP + 2];
  uint8_t seq[512];
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
  bool good = pp != NULL;
  for (uint32_t i = 0; good && i < NP; i++) good = clap_param(pp, in->p, OMX_DELAY_PARAMS[i].name, &in->ids[i]);
  good = good && clap_param(pp, in->p, NULL, &in->ids[NP]);
  if (!good || !in->p->activate(in->p, sr, 1, BLOCK) || !in->p->start_processing(in->p)) {
    in->p->destroy(in->p);
    return free(in), NULL;
  }
  return in;
}

static bool clap_run(Inst *in, const float *v, int mode, double bpm, uint32_t block, const float *il, const float *ir,
                     float *ol, float *orr, uint32_t n) {
  (void)block;
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
  clap_event_transport_t t;
  memset(&t, 0, sizeof t);
  t.header = (clap_event_header_t){sizeof t, 0, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_TRANSPORT, 0};
  t.flags = CLAP_TRANSPORT_IS_PLAYING | (mode == T_TEMPO ? CLAP_TRANSPORT_HAS_TEMPO : 0u);
  t.tempo = mode == T_NOFLAG ? 0.0 : bpm; /* T_WRONG: the tempo, without its flag */
  float *ib[2] = {(float *)il, (float *)ir}, *ob[2] = {ol, orr};
  clap_audio_buffer_t ai = {ib, NULL, 2, 0, 0}, ao = {ob, NULL, 2, 0, 0};
  clap_process_t pr = {.steady_time = (int64_t)in->steady, .frames_count = n, .transport = mode == T_NONE ? NULL : &t,
                       .audio_inputs = &ai, .audio_outputs = &ao, .audio_inputs_count = 1, .audio_outputs_count = 1,
                       .in_events = &ie, .out_events = &oe};
  in->steady += n;
  return in->p->process(in->p, &pr) != CLAP_PROCESS_ERROR;
}

static int clap_main(const char *path, bool self) {
  void *lib = dlopen(path, RTLD_NOW | RTLD_LOCAL);
  if (!lib) return fprintf(stderr, "FAIL dlopen %s: %s\n", path, dlerror()), 1;
  const clap_plugin_entry_t *entry = (const clap_plugin_entry_t *)dlsym(lib, "clap_entry");
  if (!entry || !entry->init(path)) return fprintf(stderr, "FAIL clap_entry %s\n", path), 1;
  const clap_plugin_factory_t *factory = (const clap_plugin_factory_t *)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
  if (!factory || factory->get_plugin_count(factory) != 1) return fprintf(stderr, "FAIL factory\n"), 1;
  Face f = {clap_open, clap_run, clap_close, (void *)factory, false};
  if (self) self_test("clap", &f);
  all_arms("clap", &f);
  entry->deinit();
  dlclose(lib);
  return 0;
}

/* ---- LV2 ----------------------------------------------------------------------------------------- */

#define MAX_URIS 64
static char *g_uris[MAX_URIS];
static uint32_t g_nuris = 0;
static LV2_URID map_uri(LV2_URID_Map_Handle h, const char *uri) {
  (void)h;
  for (uint32_t i = 0; i < g_nuris; i++)
    if (strcmp(g_uris[i], uri) == 0) return i + 1;
  if (g_nuris == MAX_URIS) abort();
  g_uris[g_nuris] = strdup(uri);
  return ++g_nuris;
}
static LV2_URID_Map g_map = {NULL, map_uri};
static const LV2_Feature g_map_feature = {LV2_URID__map, &g_map};
static const LV2_Feature *const g_features[] = {&g_map_feature, NULL};

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
  const char *audio[] = {"in_l", "in_r", "out_l", "out_r"};
  bool good = true;
  for (int i = 0; i < 4; i++) good = (in->ap[i] = port_of(l, audio[i])) >= 0 && good;
  for (uint32_t i = 0; i < NP; i++) good = (in->cp[i] = port_of(l, OMX_DELAY_PARAMS[i].symbol)) >= 0 && good;
  good = (in->cp[NP] = port_of(l, "enabled")) >= 0 && good;
  good = (in->cp[NP + 1] = port_of(l, "latency")) >= 0 && good;
  good = (in->ctl = port_of(l, "control")) >= 0 && good;
  if (!good || !(in->lv = lilv_plugin_instantiate(l->plugin, sr, g_features))) return free(in), NULL;
  for (uint32_t i = 0; i < NP + 2; i++) lilv_instance_connect_port(in->lv, (uint32_t)in->cp[i], &in->cv[i]);
  lilv_instance_activate(in->lv);
  return in;
}

/* The block's control sequence: a time:Position (with its tempo, or without one), or nothing. */
static void lv2_sequence(Inst *in, int mode, double bpm, uint32_t block) {
  LV2_Atom_Forge forge;
  lv2_atom_forge_init(&forge, &g_map);
  lv2_atom_forge_set_buffer(&forge, in->seq, sizeof in->seq);
  LV2_Atom_Forge_Frame seq, obj;
  lv2_atom_forge_sequence_head(&forge, &seq, 0);
  const bool position = mode == T_TEMPO || mode == T_NOFLAG || mode == T_WRONG || (mode == T_ONCE && block == 0);
  if (position) {
    lv2_atom_forge_frame_time(&forge, 0);
    /* T_WRONG: the tempo in an object that is no time:Position */
    lv2_atom_forge_object(&forge, &obj, 0, map_uri(NULL, mode == T_WRONG ? "urn:delay-tempo:not-a-position" : LV2_TIME__Position));
    lv2_atom_forge_key(&forge, map_uri(NULL, LV2_TIME__speed));
    lv2_atom_forge_float(&forge, 1.0f);
    if (mode != T_NOFLAG) {
      lv2_atom_forge_key(&forge, map_uri(NULL, LV2_TIME__beatsPerMinute));
      lv2_atom_forge_float(&forge, (float)bpm);
    }
    lv2_atom_forge_pop(&forge, &obj);
  }
  lv2_atom_forge_pop(&forge, &seq);
}

static bool lv2_run(Inst *in, const float *v, int mode, double bpm, uint32_t block, const float *il, const float *ir,
                    float *ol, float *orr, uint32_t n) {
  memcpy(in->cv, v, NP * sizeof(float));
  in->cv[NP] = 1.0f; /* enabled */
  lilv_instance_connect_port(in->lv, (uint32_t)in->ap[0], (void *)il);
  lilv_instance_connect_port(in->lv, (uint32_t)in->ap[1], (void *)ir);
  lilv_instance_connect_port(in->lv, (uint32_t)in->ap[2], ol);
  lilv_instance_connect_port(in->lv, (uint32_t)in->ap[3], orr);
  if (mode == T_NONE) lilv_instance_connect_port(in->lv, (uint32_t)in->ctl, NULL);
  else {
    lv2_sequence(in, mode, bpm, block);
    lilv_instance_connect_port(in->lv, (uint32_t)in->ctl, in->seq);
  }
  lilv_instance_run(in->lv, n);
  return true;
}

static int lv2_main(const char *bundle, const char *uri, bool self) {
  Lv2Face l = {lilv_world_new(), NULL};
  char dir[4096];
  snprintf(dir, sizeof dir, "%s%s", bundle, bundle[strlen(bundle) - 1] == '/' ? "" : "/");
  LilvNode *b = lilv_new_file_uri(l.world, NULL, dir);
  lilv_world_load_bundle(l.world, b);
  LilvNode *u = lilv_new_uri(l.world, uri);
  l.plugin = lilv_plugins_get_by_uri(lilv_world_get_all_plugins(l.world), u);
  if (!l.plugin) return fprintf(stderr, "FAIL no LV2 plugin %s in %s\n", uri, dir), 1;
  Face f = {lv2_open, lv2_run, lv2_close, &l, true};
  if (self) self_test("lv2", &f);
  all_arms("lv2", &f);
  lilv_node_free(u), lilv_node_free(b);
  lilv_world_free(l.world);
  return 0;
}

int main(int argc, char **argv) {
  omx_denormals_off();
  int rc;
  if (argc == 3 && strcmp(argv[1], "clap") == 0) rc = clap_main(argv[2], true);
  else if (argc == 4 && strcmp(argv[1], "lv2") == 0) rc = lv2_main(argv[2], argv[3], true);
  else return fprintf(stderr, "usage: delay-tempo clap <plugin.clap> | lv2 <bundle> <uri>\n"), 2;
  printf("delay-tempo %s: %d checks, %d failed\n", argv[1], g_checks, g_failed);
  return rc ? rc : g_failed ? 1 : 0;
}
