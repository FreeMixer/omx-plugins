// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * mbcomp-readout.c: each band's gain reduction (band<n>Gr, the mbcomp kernel's band<n>GainReductionDb readout)
 * as a host reads it from each BUILT face, at the nine rates an audio interface clocks at
 * (OMX_ORACLE_FULL_RATES, 32 to 192 kHz), held to the static law and to the reduction heard on the audio:
 *
 *   mbcomp-readout clap <omx-multiband.clap>
 *   mbcomp-readout lv2 <bundle-dir> <uri>
 *
 * Three bands at the declared crossovers; the programme a sine at TONE_DB dBFS RMS at the geometric centre of
 * band 2, so band 2 carries it and the RMS detector settles on its level:
 *
 *   A  level   band 2 soloed at ratio 1: the output's RMS is the band's level L, within LEVEL_TOL of the tone's
 *              (the crossovers' skirts at the centre); every band reads exactly 0
 *   B  law     band 2 at threshold T = -60, ratio R = 20: band2Gr reads -(L - T)(1 - 1/R) within TOL, the band
 *              being 20 dB past its threshold, far beyond any knee; bands 1 and 3, the tone's skirts well under
 *              their thresholds, read exactly 0. Band 2's attack and release are both BALLISTICS_MS, so its RMS
 *              detector settles on the mean square itself: with a faster attack than release it settles between
 *              the RMS and the peak, as the console's round trip measured (25.40, 27.30 and 24.44 dB at the
 *              defaults, openmixer#2368), and the law at L would not be the reading
 *   C  heard   the same with band 2 soloed: the output's RMS over L is band2Gr within TOL
 *   D  bypass  the host's bypass after the reduction: every band reads exactly 0
 *
 * The readings come from clap.meters/1 on the CLAP face and from the output control ports on the LV2 face, by
 * the symbol a host reads them by (tools/test/face-driver.h).
 */
#include "omx_mbcomp_params.h"
#include "face-driver.h"

#define NP OMX_MBCOMP_PARAM_COUNT
#define BLOCK 256u
#define RUN_S 1.5         /* seconds: the RMS detector settles well inside it at the declared attack and release */
#define MEASURE_S 0.25    /* the last this many seconds are measured */
#define TONE_DB (-40.0)   /* the tone's RMS level */
#define T_DB (-60.0)
#define RATIO 20.0
#define TOL 0.05          /* dB: a settled detector on a sine, read at a block's end */
#define LEVEL_TOL 0.5     /* dB: the band's skirt at its geometric centre */
#define BALLISTICS_MS 100.0f /* band 2's attack and release: a symmetric detector, settled within RUN_S */
#define BANDS 5u
#define MAXLEN 288000u    /* RUN_S at 192 kHz */

static const float RATES[OMX_ORACLE_FULL_RATES_COUNT] = OMX_ORACLE_FULL_RATES_INIT;
static const struct face_spec SPEC = {OMX_MBCOMP_PARAMS, NP, NULL, "mbcomp-readout"};
static float g_il[MAXLEN], g_ir[MAXLEN], g_ol[MAXLEN], g_or[MAXLEN];

static void defaults(float *v) {
  for (uint32_t i = 0; i < NP; i++) v[i] = OMX_MBCOMP_PARAMS[i].def;
}

static double rms_db(const float *x, uint32_t from, uint32_t n) {
  double e = 0.0;
  for (uint32_t i = from; i < n; i++) e += (double)x[i] * (double)x[i];
  return 10.0 * log10(e / (double)(n - from));
}

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
static double band_gr(const Face *f, Inst *in, uint32_t b) {
  char sym[32];
  snprintf(sym, sizeof sym, "band%uGr", b);
  double v;
  return in && f->meter(in, sym, &v) ? v : NAN;
}

static bool all_zero(const Face *f, Inst *in, uint32_t except) {
  bool z = in != NULL;
  for (uint32_t b = 1; in && b <= BANDS; b++)
    if (b != except) z = z && band_gr(f, in, b) == 0.0;
  return z;
}

static void arms(const Face *f) {
  char what[260];
  for (uint32_t ri = 0; ri < OMX_ORACLE_FULL_RATES_COUNT; ri++) {
    g_sr = RATES[ri];
    const uint32_t n = (uint32_t)(RUN_S * g_sr), from = n - (uint32_t)(MEASURE_S * g_sr);
    float v[NP];
    defaults(v);
    v[OMX_MBCOMP_PARAM_BANDS] = 3.0f;
    const double hz = sqrt((double)v[OMX_MBCOMP_PARAM_XOVER_HZ1] * (double)v[OMX_MBCOMP_PARAM_XOVER_HZ2]);
    const double a = pow(10.0, TONE_DB / 20.0) * sqrt(2.0);
    for (uint32_t i = 0; i < n; i++) g_il[i] = g_ir[i] = (float)(a * sin(2.0 * M_PI * hz * (double)i / g_sr));
    const double in_db = rms_db(g_il, from, n);

    /* A: band 2's level, soloed and uncompressed */
    v[OMX_MBCOMP_PARAM_SOLO2] = 1.0f;
    v[OMX_MBCOMP_PARAM_RATIO2] = 1.0f;
    v[OMX_MBCOMP_PARAM_THRESHOLD_DB2] = (float)T_DB;
    v[OMX_MBCOMP_PARAM_ATTACK_MS2] = v[OMX_MBCOMP_PARAM_RELEASE_MS2] = BALLISTICS_MS;
    Inst *in = run_face(f, v, false, n);
    const double L = rms_db(g_ol, from, n);
    snprintf(what, sizeof what, "A: band 2 soloed at ratio 1 carries the %.1f Hz tone at %.3f dBFS (the tone %.3f); every band reads exactly 0", hz, L, in_db);
    face_ok(in && fabs(L - in_db) <= LEVEL_TOL && all_zero(f, in, 0), what);
    if (in) f->close(in);

    /* B: the law at the band's level */
    const double law = -(L - T_DB) * (1.0 - 1.0 / RATIO);
    v[OMX_MBCOMP_PARAM_SOLO2] = 0.0f;
    v[OMX_MBCOMP_PARAM_RATIO2] = (float)RATIO;
    in = run_face(f, v, false, n);
    double r = band_gr(f, in, 2);
    snprintf(what, sizeof what, "B: band 2 at %.3f dBFS, threshold %.0f, ratio %.0f: band2Gr %.4f dB, the law %.4f (within %.2f)", L, T_DB, RATIO, r, law, TOL);
    face_ok(in && fabs(r - law) <= TOL, what);
    face_ok(all_zero(f, in, 2), "B: bands 1 and 3, the tone's skirts under their thresholds, and the bands past the count read exactly 0");
    if (in) f->close(in);

    /* C: the reduction read is the reduction heard */
    v[OMX_MBCOMP_PARAM_SOLO2] = 1.0f;
    in = run_face(f, v, false, n);
    r = band_gr(f, in, 2);
    const double heard = rms_db(g_ol, from, n) - L;
    snprintf(what, sizeof what, "C: band 2 soloed: the band is held down by %.4f dB, band2Gr reads %.4f (within %.2f)", heard, r, TOL);
    face_ok(in && fabs(heard - r) <= TOL && r < -10.0, what);
    if (in) f->close(in);

    /* D: bypassed after the reduction */
    v[OMX_MBCOMP_PARAM_SOLO2] = 0.0f;
    in = run_face(f, v, false, n / 2);
    const bool reducing = in && band_gr(f, in, 2) < -10.0;
    for (uint32_t at = n / 2; in && at < n; at += BLOCK) {
      const uint32_t k = n - at < BLOCK ? n - at : BLOCK;
      f->run(in, v, true, NULL, g_il + at, g_ir + at, g_ol + at, g_or + at, k);
    }
    face_ok(reducing && all_zero(f, in, 0), "D: reducing, then bypassed by the host: every band reads exactly 0");
    if (in) f->close(in);
  }
}

int main(int argc, char **argv) { return face_main(argc, argv, &SPEC, arms); }
