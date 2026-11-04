// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * dyneq-readout.c: each band's live gain (band<n>Gain, the dyneq kernel's band<n>GainDb readout) as a host
 * reads it from each BUILT face, at the nine rates an audio interface clocks at (OMX_ORACLE_FULL_RATES, 32 to
 * 192 kHz), held to the gain the band's dynamics put on a tone, measured on the audio:
 *
 *   dyneq-readout clap <omx-dyneq.clap>
 *   dyneq-readout lv2 <bundle-dir> <uri>
 *
 * Band 2 is a bell at TONE_HZ (gain 0, Q 1) and the programme a sine there at TONE_DB dBFS RMS, so the band's
 * whole gain lands on the tone and the output's RMS over the input's is that gain:
 *
 *   A  rest    dynamics Down with the threshold at 0 dBFS, over the tone: band2Gain reads 0 and the tone passes
 *              at 0 dB, within TOL
 *   B  cut     threshold -60, ratio 20: the static law asks 38 dB, the range -6 holds it: band2Gain reads -6 and
 *              the tone is cut by 6, each within TOL; every other band reads exactly 0
 *   C  range   the same at range -9: -9 and -9
 *   D  static  dynamics off, a static -6 dB bell: the tone is cut by 6 and band2Gain reads exactly 0 (the readout
 *              is the dynamics' gain, the bell's own gain is the operator's setting)
 *   E  bypass  the host's bypass after the cut: every band reads exactly 0
 *
 * The readings come from clap.meters/1 on the CLAP face and from the output control ports on the LV2 face, by
 * the symbol a host reads them by (tools/test/face-driver.h).
 */
#include "omx_dyneq_params.h"
#include "face-driver.h"

#define NP OMX_DYNEQ_PARAM_COUNT
#define BLOCK 256u
#define RUN_S 1.0         /* seconds: the detector settles well inside it at the declared attack and release */
#define MEASURE_S 0.25    /* the last this many seconds are measured */
#define TONE_HZ 1000.0
#define TONE_DB (-20.0)   /* the tone's RMS level */
#define TOL 0.05          /* dB: a settled detector, a sine measured over MEASURE_S */
#define BANDS 6u
#define MAXLEN 192000u    /* RUN_S at 192 kHz */

static const float RATES[OMX_ORACLE_FULL_RATES_COUNT] = OMX_ORACLE_FULL_RATES_INIT;
static const struct face_spec SPEC = {OMX_DYNEQ_PARAMS, NP, NULL, "dyneq-readout"};
static float g_il[MAXLEN], g_ir[MAXLEN], g_ol[MAXLEN], g_or[MAXLEN];

static void defaults(float *v) {
  for (uint32_t i = 0; i < NP; i++) v[i] = OMX_DYNEQ_PARAMS[i].def;
}

static double rms_db(const float *x, uint32_t from, uint32_t n) {
  double e = 0.0;
  for (uint32_t i = from; i < n; i++) e += (double)x[i] * (double)x[i];
  return 10.0 * log10(e / (double)(n - from));
}

/* The run through one fresh face in BLOCK-frame pieces; the instance is kept for its readings. */
static Inst *run_face(const Face *f, const float *v, bool bypass, uint32_t n) {
  Inst *in = f->open(f->ctx, g_sr);
  if (!in) return NULL;
  for (uint32_t at = 0; at < n; at += BLOCK) {
    const uint32_t k = n - at < BLOCK ? n - at : BLOCK;
    if (!f->run(in, v, bypass, NULL, g_il + at, g_ir + at, g_ol + at, g_or + at, k)) return f->close(in), NULL;
  }
  return in;
}

/* Band `b`'s (1-based) reading, NAN when the face has none. */
static double band_gain(const Face *f, Inst *in, uint32_t b) {
  char sym[32];
  snprintf(sym, sizeof sym, "band%uGain", b);
  double v;
  return in && f->meter(in, sym, &v) ? v : NAN;
}

static void arms(const Face *f) {
  char what[240];
  for (uint32_t ri = 0; ri < OMX_ORACLE_FULL_RATES_COUNT; ri++) {
    g_sr = RATES[ri];
    const uint32_t n = (uint32_t)(RUN_S * g_sr), from = n - (uint32_t)(MEASURE_S * g_sr);
    const double a = pow(10.0, TONE_DB / 20.0) * sqrt(2.0);
    for (uint32_t i = 0; i < n; i++) g_il[i] = g_ir[i] = (float)(a * sin(2.0 * M_PI * TONE_HZ * (double)i / g_sr));
    const double in_db = rms_db(g_il, from, n);
    float v[NP];
    defaults(v);
    v[OMX_DYNEQ_PARAM_BAND_ON2] = 1.0f;
    v[OMX_DYNEQ_PARAM_BAND_TYPE2] = 0.0f; /* bell */
    v[OMX_DYNEQ_PARAM_FREQ2] = (float)TONE_HZ;
    v[OMX_DYNEQ_PARAM_GAIN2] = 0.0f;
    v[OMX_DYNEQ_PARAM_Q2] = 1.0f;
    v[OMX_DYNEQ_PARAM_DYN_MODE2] = 1.0f; /* down */
    v[OMX_DYNEQ_PARAM_RATIO2] = 20.0f;

    /* A: the tone under the threshold: the band rests */
    v[OMX_DYNEQ_PARAM_THRESHOLD2] = 0.0f;
    v[OMX_DYNEQ_PARAM_RANGE2] = -6.0f;
    Inst *in = run_face(f, v, false, n);
    double g = rms_db(g_ol, from, n) - in_db, r = band_gain(f, in, 2);
    snprintf(what, sizeof what, "A: a %.0f dBFS tone under a 0 dBFS threshold: band2Gain %.4f dB, the tone %.4f dB (both 0)", TONE_DB, r, g);
    face_ok(in && fabs(r) <= TOL && fabs(g) <= TOL, what);
    if (in) f->close(in);

    /* B, C: the law past the range: the range is the gain, read and heard */
    const float ranges[2] = {-6.0f, -9.0f};
    for (int k = 0; k < 2; k++) {
      v[OMX_DYNEQ_PARAM_THRESHOLD2] = -60.0f;
      v[OMX_DYNEQ_PARAM_RANGE2] = ranges[k];
      in = run_face(f, v, false, n);
      g = rms_db(g_ol, from, n) - in_db, r = band_gain(f, in, 2);
      snprintf(what, sizeof what, "%c: threshold -60, ratio 20, range %.0f: band2Gain %.4f dB, the tone %.4f dB (both %.0f, within %.2f)",
               k ? 'C' : 'B', (double)ranges[k], r, g, (double)ranges[k], TOL);
      face_ok(in && fabs(r - ranges[k]) <= TOL && fabs(g - ranges[k]) <= TOL, what);
      bool quiet = in != NULL;
      for (uint32_t b = 1; in && b <= BANDS; b++)
        if (b != 2) quiet = quiet && band_gain(f, in, b) == 0.0;
      snprintf(what, sizeof what, "%c: every band but band 2 reads exactly 0", k ? 'C' : 'B');
      face_ok(quiet, what);
      if (in) f->close(in);
    }

    /* D: the bell's own gain is no reading of the dynamics */
    v[OMX_DYNEQ_PARAM_DYN_MODE2] = 0.0f;
    v[OMX_DYNEQ_PARAM_GAIN2] = -6.0f;
    in = run_face(f, v, false, n);
    g = rms_db(g_ol, from, n) - in_db, r = band_gain(f, in, 2);
    snprintf(what, sizeof what, "D: dynamics off, a static -6 dB bell: the tone %.4f dB (-6), band2Gain %.4f (exactly 0)", g, r);
    face_ok(in && fabs(g + 6.0) <= TOL && r == 0.0, what);
    if (in) f->close(in);

    /* E: bypassed after the cut */
    v[OMX_DYNEQ_PARAM_DYN_MODE2] = 1.0f;
    v[OMX_DYNEQ_PARAM_GAIN2] = 0.0f;
    v[OMX_DYNEQ_PARAM_THRESHOLD2] = -60.0f;
    v[OMX_DYNEQ_PARAM_RANGE2] = -6.0f;
    in = run_face(f, v, false, n / 2);
    bool cut = in && band_gain(f, in, 2) < -1.0;
    for (uint32_t at = n / 2; in && at < n; at += BLOCK) {
      const uint32_t k = n - at < BLOCK ? n - at : BLOCK;
      f->run(in, v, true, NULL, g_il + at, g_ir + at, g_ol + at, g_or + at, k);
    }
    bool zero = in != NULL;
    for (uint32_t b = 1; in && b <= BANDS; b++) zero = zero && band_gain(f, in, b) == 0.0;
    face_ok(cut && zero, "E: cutting, then bypassed by the host: every band reads exactly 0");
    if (in) f->close(in);
  }
}

int main(int argc, char **argv) { return face_main(argc, argv, &SPEC, arms); }
