// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * transient-arms.c — the transient designer's oracle, moved from the console engine with the effect:
 * arms A–E, G and H, the declared travel and the N-thread identity, run through each BUILT face at
 * the nine rates an RME interface clocks at (OMX_RME_RATES, 32 to 192 kHz).
 *
 *   transient-arms clap <omx-transient.clap>
 *   transient-arms lv2 <bundle-dir> <uri>
 *
 * The engine's arms read the kernel's state; a host sees only the face's audio. So each arm reads
 * the applied gain from the output: the kernel multiplies both legs by 10^(g_dB/20), so g_dB is
 * 20·log10(out/in) wherever the input word is not zero. Two things separate that reading from the
 * kernel's word, and both are measured, not assumed: omx_db_to_lin_poly()'s distance from 10^(x/20)
 * (POLY_DB, measured below over the gains the arms reach) and the output's one rounding. They are
 * the only slack added to the engine's bounds. Where the engine read the state itself, the arm says
 * what of it a host can see:
 *
 *   A  steps        the followers are settled on the step's first level (SETTLE_S of it), not
 *                   preloaded; g_dB on the onset (decay) is attackDb (sustainDb) times
 *                   min(1, Δ/REF) of the cascade's closed form, within the engine's bound
 *   B  full knob    a 20 dB step reaches the knob's full value: the output is the input times
 *                   omx_db_to_lin_poly(knob), exactly
 *   C  ordering     a positive attackDb never cuts, a negative sustainDb never boosts
 *   D  level        at x2^-7, x2^-3, x2^3 the output is the reference's times the scale, bit for bit
 *                   (the gain is bit-identical); at x0.01, x0.1, x10 the gain agrees within arm A's bound
 *   E  bounds       over the 5 x 5 x 3 grid of the travels, g_dB inside L4's interval at every sample
 *                   and the output peak inside its bound
 *   G  bypass       bypass and the all-zero gains leave both legs bit-identical, subnormal input words
 *                   included; after a bypassed block the face starts from silence (a fresh face's
 *                   output); at zero gains the detectors still run (the next block differs from a
 *                   fresh face's)
 *   H  blocks       one block and random splits 1..4096 are bit-identical, identical legs stay
 *                   identical, a decay into silence leaves no subnormal output word, the latency is 0
 *   travel          a slow attack below the declared travel runs as its floor
 *   threads         8 faces on 8 threads, each its own instance, are byte-identical to one
 *
 * The step's closed form needs the cascade's per-stage poles, which the oracle takes from omx-dsp's
 * omx_transient_resolve() at the same controls and rate, as the engine read them from its atom.
 */
#include <dlfcn.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <clap/clap.h>
#include <lilv/lilv.h>

#include "omx_transient_params.h"

#include <omxdsp/fx/omx_transient.h>
#include <omxdsp/omx_denormal.h>
#include <omxdsp/omx_units.h>

#define NP OMX_TRANSIENT_PARAM_COUNT
#define BLOCK 512u
#define MAXLEN 192000u
#define SETTLE_S 2u /* seconds */

static const float RATES[OMX_RME_RATES_COUNT] = OMX_RME_RATES_INIT;

static int g_checks = 0, g_failed = 0;
static const char *g_arm = "";
static const char *g_face = "";
static double g_sr = 0.0;

static void ok(int cond, const char *what, double measured, double limit) {
  g_checks++;
  if (!cond) {
    g_failed++;
    printf("FAIL %s [%s] %s @ %.0f Hz — measured %.9g, limit %.9g\n", g_face, g_arm, what, g_sr, measured, limit);
  }
}

/* ---- a face: one instance, its controls, its blocks -------------------------------------------- */

typedef struct Face Face;
typedef struct Inst Inst;
struct Face {
  Inst *(*open)(Face *f, double sr);
  /* the five declared controls (declaration order) and the host's bypass, from the next block on */
  void (*set)(Inst *in, const float *v, int bypass);
  bool (*run)(Inst *in, const float *il, const float *ir, float *ol, float *orr, uint32_t n);
  uint32_t (*latency)(Inst *in);
  void (*close)(Inst *in);
  void *ctx;
};

/* The block in `block`-frame pieces (the last one shorter). */
static bool feed(Face *f, Inst *in, const float *il, const float *ir, float *ol, float *orr, uint32_t n, uint32_t block) {
  for (uint32_t at = 0; at < n; at += block) {
    const uint32_t k = n - at < block ? n - at : block;
    if (!f->run(in, il + at, ir + at, ol + at, orr + at, k)) return false;
  }
  return true;
}

static void controls(float *v, float attack_db, float sustain_db, float attack_ms, float sustain_ms, float output_db) {
  v[OMX_TRANSIENT_PARAM_ATTACK_DB] = attack_db;
  v[OMX_TRANSIENT_PARAM_SUSTAIN_DB] = sustain_db;
  v[OMX_TRANSIENT_PARAM_ATTACK_TIME_MS] = attack_ms;
  v[OMX_TRANSIENT_PARAM_SUSTAIN_TIME_MS] = sustain_ms;
  v[OMX_TRANSIENT_PARAM_OUTPUT_DB] = output_db;
}

/* One fresh face at `sr` with the controls `v`, over the whole block in `block`-frame pieces. */
static bool run_fresh(Face *f, double sr, const float *v, int bypass, const float *il, const float *ir, float *ol,
                      float *orr, uint32_t n, uint32_t block) {
  Inst *in = f->open(f, sr);
  if (!in) return false;
  f->set(in, v, bypass);
  const bool done = feed(f, in, il, ir, ol, orr, n, block);
  f->close(in);
  return done;
}

/* ---- what the engine's oracle computes ---------------------------------------------------------- */

static uint32_t g_seed;
static float rnd(void) {
  g_seed ^= g_seed << 13;
  g_seed ^= g_seed >> 17;
  g_seed ^= g_seed << 5;
  return (float)(g_seed >> 8) / 16777216.0f * 2.0f - 1.0f;
}

/* The N-stage cascade of identical one-poles q from rest: P(Bin(n+N, 1-q) >= N). */
static double cascade_step(uint32_t n, double q) {
  double below = 0.0, c = 1.0;
  for (int k = 0; k < OMX_DYN_ENV_STAGES; k++) {
    if (k > 0) c *= (double)(n + OMX_DYN_ENV_STAGES + 1u - (uint32_t)k) / (double)k;
    below += c * pow(1.0 - q, k) * pow(q, (double)(n + OMX_DYN_ENV_STAGES) - k);
  }
  return 1.0 - below;
}

/* The arm A bound on a contrast, dB: N·3·2^-24/(1-q) relative on the slowest pole, plus the poly log. */
static double contrast_bound(double q) { return 20.0 * log10(1.0 + OMX_DYN_ENV_STAGES * 3.0 * ldexp(1.0, -24) / (1.0 - q)) + 2e-4; }

/* Gated noise on both legs (R a scaled copy of L when `stereo`), `amp` the gate's two levels. */
static void gated(float *l, float *r, uint32_t len, float sr, float scale, int stereo) {
  const uint32_t gate = (uint32_t)(sr / 8.0f);
  g_seed = 0x2f6e2b1du;
  for (uint32_t i = 0; i < len; i++) {
    const float amp = ((i / gate) & 1u) ? 0.1f : 0.9f;
    l[i] = scale * (amp * rnd());
    r[i] = stereo ? scale * (0.5f * amp * rnd()) : l[i];
  }
}

/* What reading g_dB from the audio adds: the poly's distance from 10^(x/20) over the gains the
 * arms reach (-72..+60 dB), measured, and the output word's one rounding. */
static double POLY_DB;
static void measure_poly(void) {
  double worst = 0.0;
  for (int k = -72 * 4096; k <= 60 * 4096; k++) {
    const float x = (float)k / 4096.0f;
    const double e = fabs(20.0 * log10((double)omx_db_to_lin_poly(x)) - (double)x);
    if (e > worst) worst = e;
  }
  POLY_DB = worst + 20.0 * log10(1.0 + ldexp(1.0, -23));
}

/* The gain the face applied to one leg's sample, dB; NAN where the input word is zero. */
static double gain_db(float in, float out) { return in != 0.0f ? 20.0 * log10(fabs((double)out / (double)in)) : NAN; }

static float g_l[MAXLEN], g_r[MAXLEN], g_l2[MAXLEN], g_r2[MAXLEN];
static float o_l[MAXLEN], o_r[MAXLEN], o_l2[MAXLEN], o_r2[MAXLEN];
#define SLEN ((SETTLE_S + 3u) * MAXLEN)
static float s_l[SLEN], s_o[SLEN], s_o2[SLEN];

/* ---- the arms ----------------------------------------------------------------------------------- */

/* A: on a 0.1 -> 1.0 step (rise) or back, the applied g_dB is the knob times min(1, Δ/REF) of the
 * closed form. B: a 20 dB step's contrast passes REF, so the knob reaches its full value exactly. */
static void arm_steps(Face *f, float sr) {
  const float times[2][3] = {{OMX_TRANSIENT_ATTACK_TIME_MS_MIN, OMX_TRANSIENT_ATTACK_TIME_MS_DEFAULT, OMX_TRANSIENT_ATTACK_TIME_MS_MAX},
                             {OMX_TRANSIENT_SUSTAIN_TIME_MS_MIN, OMX_TRANSIENT_SUSTAIN_TIME_MS_DEFAULT, OMX_TRANSIENT_SUSTAIN_TIME_MS_MAX}};
  for (int rise = 1; rise >= 0; rise--) {
    for (int k = 0; k < 3; k++) {
      g_arm = "A: steps";
      const float knob = 12.0f;
      float v[NP];
      controls(v, rise ? knob : 0.0f, rise ? 0.0f : knob, rise ? times[0][k] : OMX_TRANSIENT_ATTACK_TIME_MS_DEFAULT,
               rise ? OMX_TRANSIENT_SUSTAIN_TIME_MS_DEFAULT : times[1][k], 0.0f);
      struct omx_transient t;
      omx_transient_resolve(&t, 0, v[0], v[1], v[2], v[3], v[4], sr);
      const float af = rise ? 0.1f : 1.0f, bf = rise ? 1.0f : 0.1f;
      const double a = af, b = bf;
      const double qf = rise ? t.on.fast_attack : t.dec.fast_release, qs = rise ? t.on.slow_attack : t.dec.slow_release;
      const double bound = contrast_bound(qs) * knob / OMX_TRANSIENT_REF_DB + POLY_DB;
      const uint32_t settle = SETTLE_S * (uint32_t)sr, len = (uint32_t)((rise ? 0.2f : 3.0f) * sr);
      for (uint32_t i = 0; i < settle + len; i++) s_l[i] = i < settle ? af : bf;
      if (!run_fresh(f, sr, v, 0, s_l, s_l, s_o, s_o2, settle + len, BLOCK)) {
        ok(0, "the face runs", 0.0, 0.0);
        continue;
      }
      double worst = 0.0;
      float peak = 0.0f;
      uint32_t legs = 0u;
      for (uint32_t i = 0; i < len; i++) {
        const double ef = a + (b - a) * cascade_step(i, qf), es = a + (b - a) * cascade_step(i, qs);
        const double delta = rise ? 20.0 * log10(ef / es) : 20.0 * log10(es / ef);
        const double want = knob * fmin(1.0, delta / OMX_TRANSIENT_REF_DB);
        const double got = gain_db(bf, s_o[settle + i]);
        if (fabs(got - want) > worst) worst = fabs(got - want);
        if (s_o[settle + i] > peak) peak = s_o[settle + i];
        legs += s_o2[settle + i] != s_o[settle + i];
      }
      ok(legs == 0u, "identical legs stay identical", legs, 0.0);
      ok(worst <= bound, rise ? "g_dB on the onset is attackDb·min(1, ΔA/REF) of the closed form" : "g_dB on the decay is sustainDb·min(1, ΔS/REF) of the closed form", worst, bound);
      g_arm = "B: full knob";
      const float full = bf * omx_db_to_lin_poly(knob);
      ok(peak == full, "a 20 dB step reaches the knob's full value exactly", peak, full);
    }
  }
}

/* C: the attack knob acts only on onsets, the sustain knob only on decays (L2): signs never cross. */
static void arm_ordering(Face *f, float sr) {
  g_arm = "C: ordering";
  const uint32_t len = (uint32_t)sr;
  const float knobs[2][2] = {{12.0f, 0.0f}, {0.0f, -12.0f}};
  for (int c = 0; c < 2; c++) {
    float v[NP];
    controls(v, knobs[c][0], knobs[c][1], OMX_TRANSIENT_ATTACK_TIME_MS_DEFAULT, OMX_TRANSIENT_SUSTAIN_TIME_MS_DEFAULT, 0.0f);
    gated(g_l, g_r, len, sr, 1.0f, 1);
    if (!run_fresh(f, sr, v, 0, g_l, g_r, o_l, o_r, len, BLOCK)) {
      ok(0, "the face runs", 0.0, 0.0);
      continue;
    }
    uint32_t wrong = 0u;
    for (uint32_t i = 0; i < len; i++) {
      if (c == 0) wrong += fabsf(o_l[i]) < fabsf(g_l[i]) || fabsf(o_r[i]) < fabsf(g_r[i]);
      else wrong += fabsf(o_l[i]) > fabsf(g_l[i]) || fabsf(o_r[i]) > fabsf(g_r[i]);
    }
    ok(wrong == 0u, c == 0 ? "a positive attackDb never cuts" : "a negative sustainDb never boosts", wrong, 0.0);
  }
}

/* D: the applied gain does not depend on the input level (L3). */
static void arm_level(Face *f, float sr) {
  g_arm = "D: level";
  const uint32_t len = (uint32_t)sr, skip = (uint32_t)(0.01f * sr);
  float v[NP];
  controls(v, 12.0f, -12.0f, OMX_TRANSIENT_ATTACK_TIME_MS_DEFAULT, OMX_TRANSIENT_SUSTAIN_TIME_MS_DEFAULT, 0.0f);
  struct omx_transient t;
  omx_transient_resolve(&t, 0, v[0], v[1], v[2], v[3], v[4], sr);
  gated(g_l, g_r, len, sr, 1.0f, 1);
  if (!run_fresh(f, sr, v, 0, g_l, g_r, o_l, o_r, len, BLOCK)) {
    ok(0, "the face runs", 0.0, 0.0);
    return;
  }
  const float scales[6] = {0x1p-7f, 0x1p-3f, 0x1p3f, 1e-2f, 1e-1f, 10.0f};
  const double bound = contrast_bound(t.dec.slow_release) * 24.0 / OMX_TRANSIENT_REF_DB + 2.0 * POLY_DB;
  uint32_t differ = 0u;
  double worst = 0.0;
  for (int k = 0; k < 6; k++) {
    gated(g_l2, g_r2, len, sr, scales[k], 1);
    if (!run_fresh(f, sr, v, 0, g_l2, g_r2, o_l2, o_r2, len, BLOCK)) {
      ok(0, "the face runs", 0.0, 0.0);
      continue;
    }
    for (uint32_t i = skip; i < len; i++) {
      if (k < 3) {
        differ += o_l2[i] != scales[k] * o_l[i] || o_r2[i] != scales[k] * o_r[i];
      } else {
        const double dl = gain_db(g_l2[i], o_l2[i]) - gain_db(g_l[i], o_l[i]);
        const double dr = gain_db(g_r2[i], o_r2[i]) - gain_db(g_r[i], o_r[i]);
        if (fabs(dl) > worst) worst = fabs(dl);
        if (fabs(dr) > worst) worst = fabs(dr);
      }
    }
  }
  ok(differ == 0u, "at x2^-7, x2^-3, x2^3 the applied gain is bit-identical", differ, 0.0);
  ok(worst <= bound, "at x0.01, x0.1, x10 the applied gain agrees within the arm A bound", worst, bound);
}

/* E: over a 5 x 5 x 3 grid of the travels, g_dB inside L4's interval and the output peak inside its bound. */
static void arm_bounds(Face *f, float sr) {
  g_arm = "E: bounds";
  const uint32_t len = (uint32_t)(0.5f * sr);
  uint32_t outside = 0u, over = 0u;
  gated(g_l, g_r, len, sr, 1.0f, 1);
  float in_peak = 0.0f;
  for (uint32_t i = 0; i < len; i++) in_peak = fmaxf(in_peak, fmaxf(fabsf(g_l[i]), fabsf(g_r[i])));
  for (int ia = 0; ia < 5; ia++)
    for (int is = 0; is < 5; is++)
      for (int io = 0; io < 3; io++) {
        const float a = OMX_TRANSIENT_ATTACK_DB_MIN + ia * (OMX_TRANSIENT_ATTACK_DB_MAX - OMX_TRANSIENT_ATTACK_DB_MIN) / 4.0f;
        const float s = OMX_TRANSIENT_SUSTAIN_DB_MIN + is * (OMX_TRANSIENT_SUSTAIN_DB_MAX - OMX_TRANSIENT_SUSTAIN_DB_MIN) / 4.0f;
        const float o = OMX_TRANSIENT_OUTPUT_DB_MIN + io * (OMX_TRANSIENT_OUTPUT_DB_MAX - OMX_TRANSIENT_OUTPUT_DB_MIN) / 2.0f;
        float v[NP];
        controls(v, a, s, OMX_TRANSIENT_ATTACK_TIME_MS_DEFAULT, OMX_TRANSIENT_SUSTAIN_TIME_MS_DEFAULT, o);
        if (!run_fresh(f, sr, v, 0, g_l, g_r, o_l, o_r, len, BLOCK)) {
          ok(0, "the face runs", 0.0, 0.0);
          continue;
        }
        const float hi = fmaxf(0.0f, a) + fmaxf(0.0f, s) + o, lo = fminf(0.0f, a) + fminf(0.0f, s) + o;
        float out_peak = 0.0f;
        for (uint32_t i = 0; i < len; i++) {
          const double gl = gain_db(g_l[i], o_l[i]), gr = gain_db(g_r[i], o_r[i]);
          outside += gl > hi + POLY_DB || gl < lo - POLY_DB || gr > hi + POLY_DB || gr < lo - POLY_DB;
          out_peak = fmaxf(out_peak, fmaxf(fabsf(o_l[i]), fabsf(o_r[i])));
        }
        over += (double)out_peak > (double)in_peak * pow(10.0, hi / 20.0) * (1.0 + 4e-6);
      }
  ok(outside == 0u, "g_dB lies inside L4's interval at every sample of every grid point", outside, 0.0);
  ok(over == 0u, "the output peak is inside input peak x 10^(bound/20)", over, 0.0);
}

/* G: bypass and the all-zero gains are memcmp-identical, subnormal input words included (L5). */
static void arm_bypass(Face *f, float sr) {
  g_arm = "G: bypass";
  const uint32_t len = 4096u;
  for (int mode = 0; mode < 2; mode++) {
    float v[NP];
    controls(v, mode == 0 ? 12.0f : 0.0f, mode == 0 ? 12.0f : 0.0f, OMX_TRANSIENT_ATTACK_TIME_MS_DEFAULT,
             OMX_TRANSIENT_SUSTAIN_TIME_MS_DEFAULT, 0.0f);
    gated(g_l, g_r, len, sr, 1.0f, 1);
    for (uint32_t i = 0; i < len; i += 7u) { g_l[i] = 1e-40f; g_r[i] = -1e-41f; }
    const bool ran = run_fresh(f, sr, v, mode == 0, g_l, g_r, o_l, o_r, len, BLOCK);
    ok(ran && memcmp(o_l, g_l, len * sizeof(float)) == 0 && memcmp(o_r, g_r, len * sizeof(float)) == 0,
       mode == 0 ? "bypass leaves both legs bit-identical" : "zero gains leave both legs bit-identical", 0.0, 0.0);
  }
  /* What a bypass leaves: engaged, bypassed, then engaged again on a new block, the face's output
   * on that block is a fresh face's, which starts from silence. */
  float v[NP], z[NP];
  controls(v, 12.0f, -12.0f, OMX_TRANSIENT_ATTACK_TIME_MS_DEFAULT, OMX_TRANSIENT_SUSTAIN_TIME_MS_DEFAULT, 0.0f);
  controls(z, 0.0f, 0.0f, OMX_TRANSIENT_ATTACK_TIME_MS_DEFAULT, OMX_TRANSIENT_SUSTAIN_TIME_MS_DEFAULT, 0.0f);
  gated(g_l, g_r, 3u * len, sr, 1.0f, 1);
  Inst *in = f->open(f, sr);
  bool ran = in != NULL;
  if (ran) {
    f->set(in, v, 0);
    ran = feed(f, in, g_l, g_r, o_l, o_r, len, BLOCK);
    f->set(in, v, 1);
    ran = ran && feed(f, in, g_l + len, g_r + len, o_l + len, o_r + len, len, BLOCK);
    f->set(in, v, 0);
    ran = ran && feed(f, in, g_l + 2u * len, g_r + 2u * len, o_l + 2u * len, o_r + 2u * len, len, BLOCK);
    f->close(in);
  }
  ran = ran && run_fresh(f, sr, v, 0, g_l + 2u * len, g_r + 2u * len, o_l2, o_r2, len, BLOCK);
  ok(ran && memcmp(o_l + 2u * len, o_l2, len * sizeof(float)) == 0 && memcmp(o_r + 2u * len, o_r2, len * sizeof(float)) == 0,
     "after a bypass the face starts from silence, as a fresh one", 0.0, 0.0);
  /* At zero gains the detectors run: zero gains on one block, then the knobs on the next, differ
   * from a fresh face's output on that next block. */
  in = f->open(f, sr);
  ran = in != NULL;
  if (ran) {
    f->set(in, z, 0);
    ran = feed(f, in, g_l, g_r, o_l, o_r, len, BLOCK);
    f->set(in, v, 0);
    ran = ran && feed(f, in, g_l + len, g_r + len, o_l + len, o_r + len, len, BLOCK);
    f->close(in);
  }
  ran = ran && run_fresh(f, sr, v, 0, g_l + len, g_r + len, o_l2, o_r2, len, BLOCK);
  ok(ran && (memcmp(o_l + len, o_l2, len * sizeof(float)) != 0 || memcmp(o_r + len, o_r2, len * sizeof(float)) != 0),
     "at zero gains the detectors still run", 0.0, 0.0);
}

/* H: block splits are memcmp-identical, identical legs stay identical, no subnormal survives. */
static void arm_blocks(Face *f, float sr) {
  g_arm = "H: blocks";
  const uint32_t len = (uint32_t)sr;
  float v[NP];
  controls(v, 9.0f, -7.0f, 5.0f, 400.0f, 1.5f);
  gated(g_l, g_r, len, sr, 1.0f, 0);
  bool ran = run_fresh(f, sr, v, 0, g_l, g_r, o_l, o_r, len, len);
  Inst *in = f->open(f, sr);
  ran = ran && in != NULL;
  if (in) {
    f->set(in, v, 0);
    uint32_t seed = 0x51ed27u;
    for (uint32_t i = 0; ran && i < len;) {
      seed = seed * 1664525u + 1013904223u;
      uint32_t n = 1u + (seed >> 8) % 4096u;
      if (n > len - i) n = len - i;
      ran = f->run(in, g_l + i, g_r + i, o_l2 + i, o_r2 + i, n);
      i += n;
    }
  }
  ok(ran && memcmp(o_l, o_l2, len * sizeof(float)) == 0 && memcmp(o_r, o_r2, len * sizeof(float)) == 0,
     "one block and random splits 1..4096 are memcmp-identical", 0.0, 0.0);
  ok(ran && memcmp(o_l, o_r, len * sizeof(float)) == 0, "identical legs stay identical", 0.0, 0.0);
  for (uint32_t i = 0; i < len; i++) g_l[i] = g_r[i] = 1e-3f * powf(1e-3f, (float)i / (float)len * 12.0f);
  ran = ran && feed(f, in, g_l, g_r, o_l, o_r, len, BLOCK);
  memset(g_l, 0, len * sizeof(float));
  memset(g_r, 0, len * sizeof(float));
  ran = ran && feed(f, in, g_l, g_r, o_l2, o_r2, len, BLOCK);
  uint32_t sub = 0u;
  for (uint32_t i = 0; i < len; i++)
    sub += (fpclassify(o_l[i]) == FP_SUBNORMAL) + (fpclassify(o_r[i]) == FP_SUBNORMAL) +
           (fpclassify(o_l2[i]) == FP_SUBNORMAL) + (fpclassify(o_r2[i]) == FP_SUBNORMAL);
  ok(ran && sub == 0u, "a decay into silence leaves no subnormal output word", sub, 0.0);
  const uint32_t lat = in ? f->latency(in) : 99u;
  ok(lat == 0u, "the latency is zero (L1)", lat, 0.0);
  if (in) f->close(in);
}

/* The face follows the declared travel: a slow attack below OMX_TRANSIENT_ATTACK_TIME_MS_MIN runs
 * as the floor, and one inside it is its own. */
static void arm_declared_travel(Face *f, float sr) {
  g_arm = "declared travel";
  const uint32_t len = (uint32_t)(0.5f * sr);
  float below[NP], floor_[NP], inside[NP];
  controls(below, 12.0f, 0.0f, 0.5f * OMX_TRANSIENT_ATTACK_TIME_MS_MIN, OMX_TRANSIENT_SUSTAIN_TIME_MS_DEFAULT, 0.0f);
  controls(floor_, 12.0f, 0.0f, OMX_TRANSIENT_ATTACK_TIME_MS_MIN, OMX_TRANSIENT_SUSTAIN_TIME_MS_DEFAULT, 0.0f);
  controls(inside, 12.0f, 0.0f, 10.0f, OMX_TRANSIENT_SUSTAIN_TIME_MS_DEFAULT, 0.0f);
  gated(g_l, g_r, len, sr, 1.0f, 1);
  bool ran = run_fresh(f, sr, below, 0, g_l, g_r, o_l, o_r, len, BLOCK);
  ran = ran && run_fresh(f, sr, floor_, 0, g_l, g_r, o_l2, o_r2, len, BLOCK);
  ok(ran && memcmp(o_l, o_l2, len * sizeof(float)) == 0 && memcmp(o_r, o_r2, len * sizeof(float)) == 0,
     "a slow attack below the declared travel runs as its floor", 0.0, 0.0);
  ran = ran && run_fresh(f, sr, inside, 0, g_l, g_r, o_l, o_r, len, BLOCK);
  ok(ran && (memcmp(o_l, o_l2, len * sizeof(float)) != 0 || memcmp(o_r, o_r2, len * sizeof(float)) != 0),
     "a slow attack of 10 ms inside the travel is its own", 0.0, 0.0);
}

/* N-thread identity (rt-thread-split §6): 8 faces, each its OWN instance, run the same input at
 * every rate in ragged blocks on 8 threads; every output is memcmp-identical to one face's. */
#define THREADS 8
#define TLEN 16384u
struct tjob {
  Face *f;
  Inst *in;
  bool ran;
  float l[TLEN], r[TLEN], ol[TLEN], orr[TLEN];
};
static void *tjob_run(void *arg) {
  struct tjob *j = arg;
  static const uint32_t splits[5] = {64u, 37u, 1u, 256u, 100u};
  j->ran = true;
  for (uint32_t i = 0, b = 0; j->ran && i < TLEN; b++) {
    const uint32_t n = splits[b % 5u] < TLEN - i ? splits[b % 5u] : TLEN - i;
    j->ran = j->f->run(j->in, j->l + i, j->r + i, j->ol + i, j->orr + i, n);
    i += n;
  }
  return NULL;
}
static void arm_threads(Face *f, float sr) {
  g_arm = "threads";
  static struct tjob ref, jobs[THREADS];
  float v[NP];
  controls(v, 9.0f, -6.0f, 5.0f, 400.0f, 1.0f);
  ref.f = f;
  gated(ref.l, ref.r, TLEN, sr, 1.0f, 1);
  for (int k = 0; k < THREADS; k++) jobs[k] = ref;
  bool opened = true;
  for (int k = 0; k <= THREADS; k++) {
    struct tjob *j = k < THREADS ? &jobs[k] : &ref;
    j->in = f->open(f, sr);
    if (j->in) f->set(j->in, v, 0);
    opened = opened && j->in != NULL;
  }
  uint32_t differ = THREADS;
  if (opened) {
    tjob_run(&ref);
    pthread_t th[THREADS];
    for (int k = 0; k < THREADS; k++) pthread_create(&th[k], NULL, tjob_run, &jobs[k]);
    differ = 0u;
    for (int k = 0; k < THREADS; k++) {
      pthread_join(th[k], NULL);
      differ += !ref.ran || !jobs[k].ran || memcmp(jobs[k].ol, ref.ol, sizeof ref.ol) != 0 ||
                memcmp(jobs[k].orr, ref.orr, sizeof ref.orr) != 0;
    }
  }
  for (int k = 0; k <= THREADS; k++) {
    struct tjob *j = k < THREADS ? &jobs[k] : &ref;
    if (j->in) f->close(j->in);
  }
  ok(differ == 0u, "8 faces on 8 threads, each its own instance, are byte-identical to one", differ, 0.0);
}

static void all_arms(const char *face, Face *f) {
  g_face = face;
  for (uint32_t ri = 0; ri < OMX_RME_RATES_COUNT; ri++) {
    const float sr = RATES[ri];
    g_sr = sr;
    const int before = g_failed;
    arm_steps(f, sr);
    arm_ordering(f, sr);
    arm_level(f, sr);
    arm_bounds(f, sr);
    arm_bypass(f, sr);
    arm_blocks(f, sr);
    arm_declared_travel(f, sr);
    arm_threads(f, sr);
    printf("%s %s every arm @ %.0f Hz\n", g_failed == before ? "PASS" : "FAIL", face, (double)sr);
  }
}

/* ---- CLAP ---------------------------------------------------------------------------------------- */

static const void *h_ext(const clap_host_t *h, const char *id) {
  (void)h, (void)id;
  return NULL;
}
static void h_noop(const clap_host_t *h) { (void)h; }
static const clap_host_t HOST = {CLAP_VERSION_INIT, NULL, "transient-arms", "openmixer", "", "0.1", h_ext, h_noop,
                                 h_noop, h_noop};

struct Inst {
  /* CLAP */
  const clap_plugin_t *p;
  clap_id ids[NP + 1];
  uint64_t steady;
  /* LV2 */
  LilvInstance *lv;
  int32_t ap[4];
  /* both: the controls the next block carries, the host's bypass last; the LV2 latency port */
  float cv[NP + 2];
};

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
  if (in->p) {
    in->p->stop_processing(in->p);
    in->p->deactivate(in->p);
    in->p->destroy(in->p);
  }
  free(in);
}

static Inst *clap_open(Face *f, double sr) {
  const clap_plugin_factory_t *factory = f->ctx;
  const clap_plugin_descriptor_t *desc = factory->get_plugin_descriptor(factory, 0);
  Inst *in = calloc(1, sizeof *in);
  if (!in) abort();
  in->p = factory->create_plugin(factory, &HOST, desc->id);
  if (!in->p || !in->p->init(in->p)) return free(in), NULL;
  const clap_plugin_params_t *pp = (const clap_plugin_params_t *)in->p->get_extension(in->p, CLAP_EXT_PARAMS);
  bool good = pp != NULL;
  for (uint32_t i = 0; good && i < NP; i++) good = clap_param(pp, in->p, OMX_TRANSIENT_PARAMS[i].name, &in->ids[i]);
  good = good && clap_param(pp, in->p, NULL, &in->ids[NP]);
  if (!good || !in->p->activate(in->p, sr, 1, MAXLEN)) {
    in->p->destroy(in->p);
    return free(in), NULL;
  }
  if (!in->p->start_processing(in->p)) {
    in->p->deactivate(in->p);
    in->p->destroy(in->p);
    return free(in), NULL;
  }
  return in;
}

static void any_set(Inst *in, const float *v, int bypass) {
  memcpy(in->cv, v, NP * sizeof(float));
  in->cv[NP] = (float)bypass;
}

static bool clap_block(Inst *in, const float *il, const float *ir, float *ol, float *orr, uint32_t n) {
  Events evs = {.n = 0};
  for (uint32_t i = 0; i <= NP; i++) {
    clap_event_param_value_t *e = &evs.ev[evs.n++];
    memset(e, 0, sizeof *e);
    e->header = (clap_event_header_t){sizeof *e, 0, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_PARAM_VALUE, 0};
    e->param_id = in->ids[i], e->note_id = -1, e->port_index = -1, e->channel = -1, e->key = -1;
    e->value = (double)in->cv[i];
  }
  clap_input_events_t ie = {&evs, ev_size, ev_get};
  clap_output_events_t oe = {NULL, ev_push};
  float *ib[2] = {(float *)il, (float *)ir}, *ob[2] = {ol, orr};
  clap_audio_buffer_t ai = {ib, NULL, 2, 0, 0}, ao = {ob, NULL, 2, 0, 0};
  clap_process_t pr = {.steady_time = (int64_t)in->steady, .frames_count = n, .audio_inputs = &ai, .audio_outputs = &ao,
                       .audio_inputs_count = 1, .audio_outputs_count = 1, .in_events = &ie, .out_events = &oe};
  in->steady += n;
  return in->p->process(in->p, &pr) != CLAP_PROCESS_ERROR;
}

static uint32_t clap_latency(Inst *in) {
  const clap_plugin_latency_t *lat = (const clap_plugin_latency_t *)in->p->get_extension(in->p, CLAP_EXT_LATENCY);
  return lat ? lat->get(in->p) : 99u;
}

static int clap_main(const char *path) {
  void *lib = dlopen(path, RTLD_NOW | RTLD_LOCAL);
  if (!lib) return fprintf(stderr, "FAIL dlopen %s: %s\n", path, dlerror()), 1;
  const clap_plugin_entry_t *entry = (const clap_plugin_entry_t *)dlsym(lib, "clap_entry");
  if (!entry || !entry->init(path)) return fprintf(stderr, "FAIL clap_entry %s\n", path), 1;
  const clap_plugin_factory_t *factory = (const clap_plugin_factory_t *)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
  if (!factory || factory->get_plugin_count(factory) != 1) return fprintf(stderr, "FAIL factory\n"), 1;
  Face f = {clap_open, any_set, clap_block, clap_latency, clap_close, (void *)factory};
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
  if (in->lv) {
    lilv_instance_deactivate(in->lv);
    lilv_instance_free(in->lv);
  }
  free(in);
}

static Inst *lv2_open(Face *f, double sr) {
  const Lv2Face *l = f->ctx;
  Inst *in = calloc(1, sizeof *in);
  if (!in) abort();
  const char *audio[] = {"in_l", "in_r", "out_l", "out_r"};
  int32_t cp[NP + 2];
  bool good = true;
  for (int i = 0; i < 4; i++) good = (in->ap[i] = port_of(l, audio[i])) >= 0 && good;
  for (uint32_t i = 0; i < NP; i++) good = (cp[i] = port_of(l, OMX_TRANSIENT_PARAMS[i].symbol)) >= 0 && good;
  good = (cp[NP] = port_of(l, "enabled")) >= 0 && good;
  good = (cp[NP + 1] = port_of(l, "latency")) >= 0 && good;
  if (!good || !(in->lv = lilv_plugin_instantiate(l->plugin, sr, NULL))) return free(in), NULL;
  for (uint32_t i = 0; i < NP + 2; i++) lilv_instance_connect_port(in->lv, (uint32_t)cp[i], &in->cv[i]);
  lilv_instance_activate(in->lv);
  return in;
}

/* LV2's switch is `enabled`, the inverse of the host's bypass. */
static bool lv2_block(Inst *in, const float *il, const float *ir, float *ol, float *orr, uint32_t n) {
  const float bypass = in->cv[NP];
  in->cv[NP] = bypass > 0.5f ? 0.0f : 1.0f;
  lilv_instance_connect_port(in->lv, (uint32_t)in->ap[0], (void *)il);
  lilv_instance_connect_port(in->lv, (uint32_t)in->ap[1], (void *)ir);
  lilv_instance_connect_port(in->lv, (uint32_t)in->ap[2], ol);
  lilv_instance_connect_port(in->lv, (uint32_t)in->ap[3], orr);
  lilv_instance_run(in->lv, n);
  in->cv[NP] = bypass;
  return true;
}

static uint32_t lv2_latency(Inst *in) { return (uint32_t)(in->cv[NP + 1] + 0.5f); }

static int lv2_main(const char *bundle, const char *uri) {
  Lv2Face l = {lilv_world_new(), NULL};
  char dir[4096];
  snprintf(dir, sizeof dir, "%s%s", bundle, bundle[strlen(bundle) - 1] == '/' ? "" : "/");
  LilvNode *b = lilv_new_file_uri(l.world, NULL, dir);
  lilv_world_load_bundle(l.world, b);
  LilvNode *u = lilv_new_uri(l.world, uri);
  l.plugin = lilv_plugins_get_by_uri(lilv_world_get_all_plugins(l.world), u);
  if (!l.plugin) return fprintf(stderr, "FAIL no LV2 plugin %s in %s\n", uri, dir), 1;
  Face f = {lv2_open, any_set, lv2_block, lv2_latency, lv2_close, &l};
  all_arms("lv2", &f);
  lilv_node_free(u), lilv_node_free(b);
  lilv_world_free(l.world);
  return g_failed ? 1 : 0;
}

int main(int argc, char **argv) {
  omx_denormals_off(); /* the faces run with flush-to-zero; so does the closed form's input */
  measure_poly();
  int rc;
  if (argc == 3 && strcmp(argv[1], "clap") == 0) rc = clap_main(argv[2]);
  else if (argc == 4 && strcmp(argv[1], "lv2") == 0) rc = lv2_main(argv[2], argv[3]);
  else return fprintf(stderr, "usage: transient-arms clap <x.clap> | lv2 <bundle-dir> <uri>\n"), 2;
  printf("%s %s: %d checks, %d failed, %u rates\n", rc ? "ORACLE-RED" : "ORACLE-GREEN", argv[1], g_checks, g_failed,
         OMX_RME_RATES_COUNT);
  return rc;
}
