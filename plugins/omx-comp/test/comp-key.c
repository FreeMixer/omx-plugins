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
 *   F  meters     after the keyed run, compKey reads the key's level and compGr the law's reduction, through
 *                 clap.meters/1 and through the LV2 output control ports (tools/test/face-driver.h)
 *
 * The threshold, ratio, knee and times are the declared defaults; the key's level is chosen above the
 * threshold by more than half the knee, so the law is the straight line past the knee.
 */
#include "omx_comp_params.h"
#include "face-driver.h"

#include <omxdsp/fx/omx_comp_instance.h>

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
#define KEY_METER "compKey" /* the key level's symbol */
#define GR_METER "compGr"   /* the gain reduction's */

static const float RATES[OMX_ORACLE_FULL_RATES_COUNT] = OMX_ORACLE_FULL_RATES_INIT;

static const struct face_spec SPEC = {OMX_COMP_PARAMS, NP, "key", "comp-key"};

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
    good = f->run(in, v, false, key ? key + at : NULL, g_il + at, g_ir + at, g_ol + at, g_or + at, k);
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
                              (int)v[OMX_COMP_PARAM_DUCK], v[OMX_COMP_PARAM_RANGE_DB], (int)OMX_COMP_RELEASE_MODES_DEFAULT);
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

static void all_arms(const Face *f) {
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
    face_ok(moved, "A: the unkeyed compressor shapes the programme at the defaults (the identity is not vacuous)");
    bool ran = run_face(f, v, g_key, n, NULL);
    face_ok(ran && memcmp(g_ol, g_wl, n * sizeof(float)) == 0 && memcmp(g_or, g_wr, n * sizeof(float)) == 0,
       "A: at the defaults, a loud key routed, the output is omx-dsp's unkeyed compressor, memcmp-identical");
    ran = run_face(f, v, NULL, n, NULL);
    face_ok(ran && memcmp(g_ol, g_wl, n * sizeof(float)) == 0 && memcmp(g_or, g_wr, n * sizeof(float)) == 0,
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
    face_ok(ran && fabs(gl - law) <= GAIN_TOL && fabs(gr - law) <= GAIN_TOL, what);

    /* F: the meters of that run, by the symbol a host reads them by */
    double key_level = NAN, gr_db = NAN;
    const bool read = kept && f->meter(kept, KEY_METER, &key_level) && f->meter(kept, GR_METER, &gr_db);
    snprintf(what, sizeof what, "F: after the keyed run compKey reads %.3f dBFS (the key %.2f) and compGr %.3f dB (the law %.3f)", key_level,
             KEY_DB, gr_db, law);
    face_ok(read && fabs(key_level - KEY_DB) <= METER_TOL && fabs(gr_db - law) <= METER_TOL, what);
    if (kept) f->close(kept);

    /* C: the sidechain listened to, nothing routed: the programme's own level, under the threshold */
    ran = run_face(f, v, NULL, n, NULL);
    double g0 = rms_db(g_ol, from, n) - in_db;
    snprintf(what, sizeof what, "C: keySource sidechain, no key routed: gain %.4f dB", g0);
    face_ok(ran && fabs(g0) <= 0.01, what);

    /* D: the key routed, the detector on the programme */
    v[OMX_COMP_PARAM_KEY_SOURCE] = 0.0f;
    ran = run_face(f, v, g_key, n, NULL);
    g0 = rms_db(g_ol, from, n) - in_db;
    snprintf(what, sizeof what, "D: keySource self, the key routed: gain %.4f dB", g0);
    face_ok(ran && fabs(g0) <= 0.01, what);

    /* E: the ducker holds the reduction to its range */
    v[OMX_COMP_PARAM_KEY_SOURCE] = 1.0f;
    v[OMX_COMP_PARAM_DUCK] = 1.0f;
    v[OMX_COMP_PARAM_RANGE_DB] = (float)DUCK_DB;
    ran = run_face(f, v, g_key, n, NULL);
    const double gd = rms_db(g_ol, from, n) - in_db;
    snprintf(what, sizeof what, "E: duck on, range %.0f dB, the law asking %.3f dB: gain %.3f dB", DUCK_DB, law, gd);
    face_ok(ran && fabs(gd - DUCK_DB) <= GAIN_TOL, what);
  }
}

int main(int argc, char **argv) { return face_main(argc, argv, &SPEC, all_arms); }
