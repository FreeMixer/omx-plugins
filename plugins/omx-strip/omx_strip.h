// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * omx_strip.h — the channel strip as ONE plugin core, shared by the CLAP and the LV2 face, with no
 * DSP of its own: four stages, each an omx-dsp strip module called through its plain C instance
 * functions, run one after the other over the block in place.
 *
 *   INPUT  trim (the desk's de-zippered gain: omx_db_to_lin, omx_ramp) then the HPF and the LPF
 *          (<omxdsp/fx/omx_eq_instance.h> with no band on: the EQ's own pass filters, per leg)
 *   GATE   <omxdsp/fx/omx_gate_instance.h>, self-keyed
 *   EQ     <omxdsp/fx/omx_eq_instance.h>, OMX_STRIP_EQ_BANDS parametric bands, per leg
 *   COMP   <omxdsp/fx/omx_dynamics_instance.h>
 *
 * The strip's default order is INPUT, GATE, EQ, COMP. The `order` parameter picks any of the 24
 * orders: its value is the index of the permutation in lexicographic order over the four stage
 * ids, so 0 is the default. test/strip-oracle.c checks both faces, at 44.1/48/96/192 kHz and in
 * every order, against the modules called in sequence by hand.
 *
 * Bypass (the host's) is the identity for the whole strip; each stage's own switch is its module's
 * enable. Latency is the gate's plus the compressor's (each OMX_OVS_LATENCY_4X while its 4x
 * control path is engaged), whatever the order.
 */
#ifndef OMX_STRIP_H
#define OMX_STRIP_H

#include <stdint.h>
#include <string.h>

/* The generated table FIRST, then the modules. */
#include "omx_strip_params.h"

#define OMX_STRIP_EQ_BANDS 4
#define OMX_EQ_LV2_BANDS OMX_STRIP_EQ_BANDS
#include <omxdsp/fx/omx_dynamics_instance.h>
#include <omxdsp/fx/omx_eq_instance.h>
#include <omxdsp/fx/omx_gate_instance.h>
#include <omxdsp/omx_ramp.h>
#include <omxdsp/omx_units.h>

enum { OMX_STRIP_INPUT = 0, OMX_STRIP_GATE, OMX_STRIP_EQ, OMX_STRIP_COMP, OMX_STRIP_STAGES };
#define OMX_STRIP_ORDERS 24

/** Every order of the four stages, lexicographic: row `order` is the stage run first to last. */
static const uint8_t OMX_STRIP_ORDER[OMX_STRIP_ORDERS][OMX_STRIP_STAGES] = {
    {0, 1, 2, 3}, {0, 1, 3, 2}, {0, 2, 1, 3}, {0, 2, 3, 1}, {0, 3, 1, 2}, {0, 3, 2, 1},
    {1, 0, 2, 3}, {1, 0, 3, 2}, {1, 2, 0, 3}, {1, 2, 3, 0}, {1, 3, 0, 2}, {1, 3, 2, 0},
    {2, 0, 1, 3}, {2, 0, 3, 1}, {2, 1, 0, 3}, {2, 1, 3, 0}, {2, 3, 0, 1}, {2, 3, 1, 0},
    {3, 0, 1, 2}, {3, 0, 2, 1}, {3, 1, 0, 2}, {3, 1, 2, 0}, {3, 2, 0, 1}, {3, 2, 1, 0},
};

typedef struct {
  float trim_cur, trim_tgt; /* linear: the gain applied at the end of the last block, the target */
  struct omx_eq_lv2 filter[2], eq[2]; /* per leg */
  OmxGateInstance gate;
  OmxDynamicsInstance comp;
  uint32_t order;
  int bypass, ready;
} OmxStrip;

/** Instantiate/activate: every module bound to the rate and cleared. 0 for a rate that is not
 * positive; a refused strip is the identity. No allocation. */
static inline int omx_strip_init(OmxStrip *s, float sr) {
  memset(s, 0, sizeof *s);
  if (!(sr > 0.0f)) return 0;
  for (int c = 0; c < 2; c++) omx_eq_lv2_init(&s->filter[c], sr), omx_eq_lv2_init(&s->eq[c], sr);
  if (!omx_gate_instance_init(&s->gate, sr) || !omx_dynamics_instance_init(&s->comp, sr)) return 0;
  s->trim_cur = s->trim_tgt = 1.0f;
  s->ready = 1;
  return 1;
}

/** One cycle's parameter values (declaration order, OMX_STRIP_PARAM_COUNT of them) handed to the
 * modules' own resolvers, which clamp into their declared travels. */
static inline void omx_strip_resolve(OmxStrip *s, int bypass, const float *v) {
  if (!s->ready) return;
  s->bypass = bypass ? 1 : 0;
  s->trim_tgt = omx_db_to_lin(omx_clampf(v[OMX_STRIP_PARAM_TRIM_DB], OMX_TRIM_RANGE_MIN_DB, OMX_TRIM_RANGE_MAX_DB));
  for (int c = 0; c < 2; c++) {
    const struct omx_eq_lv2_controls f = {
        .hpf_on = &v[OMX_STRIP_PARAM_HPF_ON], .hpf_freq = &v[OMX_STRIP_PARAM_HPF_FREQ],
        .hpf_slope = &v[OMX_STRIP_PARAM_HPF_SLOPE], .lpf_on = &v[OMX_STRIP_PARAM_LPF_ON],
        .lpf_freq = &v[OMX_STRIP_PARAM_LPF_FREQ], .lpf_slope = &v[OMX_STRIP_PARAM_LPF_SLOPE]};
    omx_eq_lv2_set_controls(&s->filter[c], &f);
    struct omx_eq_lv2_controls e = {.on = &v[OMX_STRIP_PARAM_EQ_ON]};
    for (uint32_t b = 0; b < OMX_STRIP_EQ_BANDS; b++)
      for (uint32_t k = 0; k < 5; k++) e.band[b][k] = &v[OMX_STRIP_PARAM_EQ1_TYPE + 5u * b + k];
    omx_eq_lv2_set_controls(&s->eq[c], &e);
  }
  omx_gate_instance_resolve(&s->gate, v[OMX_STRIP_PARAM_GATE_ON] < 0.5f, 0, v[OMX_STRIP_PARAM_GATE_THRESHOLD],
                            v[OMX_STRIP_PARAM_GATE_RATIO], v[OMX_STRIP_PARAM_GATE_RANGE],
                            v[OMX_STRIP_PARAM_GATE_ATTACK], v[OMX_STRIP_PARAM_GATE_RELEASE]);
  omx_dynamics_instance_resolve(&s->comp, v[OMX_STRIP_PARAM_COMP_ON] < 0.5f, v[OMX_STRIP_PARAM_COMP_THRESHOLD],
                                v[OMX_STRIP_PARAM_COMP_RATIO], v[OMX_STRIP_PARAM_COMP_KNEE],
                                v[OMX_STRIP_PARAM_COMP_ATTACK], v[OMX_STRIP_PARAM_COMP_RELEASE],
                                v[OMX_STRIP_PARAM_COMP_MAKEUP], v[OMX_STRIP_PARAM_COMP_RMS] >= 0.5f,
                                OMX_DYN_OVS_AUTO);
  s->order = (uint32_t)omx_eq_lv2_word_int(&v[OMX_STRIP_PARAM_ORDER], 0, OMX_STRIP_ORDERS - 1, 0);
}

/** The frames of latency the strip reports now. */
static inline float omx_strip_latency(const OmxStrip *s) {
  return omx_gate_instance_latency(&s->gate) + omx_dynamics_instance_latency(&s->comp);
}

/** One stage over the block, in place on both legs. */
static inline void omx_strip_stage(OmxStrip *s, uint32_t stage, float *l, float *r, uint32_t n) {
  switch (stage) {
  case OMX_STRIP_INPUT: {
    const struct omx_ramp g = omx_ramp_begin(&s->trim_cur, s->trim_tgt, n);
    for (uint32_t i = 0; i < n; i++) l[i] *= omx_ramp_at(g, i), r[i] *= omx_ramp_at(g, i);
    omx_ramp_end(g, &s->trim_cur);
    omx_eq_lv2_run(&s->filter[0], l, l, n);
    omx_eq_lv2_run(&s->filter[1], r, r, n);
    return;
  }
  case OMX_STRIP_GATE: omx_gate_instance_run(&s->gate, NULL, l, r, l, r, n); return;
  case OMX_STRIP_EQ:
    omx_eq_lv2_run(&s->eq[0], l, l, n);
    omx_eq_lv2_run(&s->eq[1], r, r, n);
    return;
  case OMX_STRIP_COMP: omx_dynamics_instance_run(&s->comp, l, r, l, r, n); return;
  }
}

/** THE AUDIO CALLBACK'S WHOLE SHARE: in to out (they may alias), then the four stages in the
 * resolved order. Bypassed or not ready, out is in. RT-safe: no allocation, no lock. */
static inline void omx_strip_run(OmxStrip *s, const float *in_l, const float *in_r, float *out_l, float *out_r,
                                 uint32_t n) {
  if (!in_l || !in_r || !out_l || !out_r || n == 0u) return;
  if (out_l != in_l) memmove(out_l, in_l, (size_t)n * sizeof(float));
  if (out_r != in_r) memmove(out_r, in_r, (size_t)n * sizeof(float));
  if (!s->ready || s->bypass) return;
  for (uint32_t k = 0; k < OMX_STRIP_STAGES; k++) omx_strip_stage(s, OMX_STRIP_ORDER[s->order][k], out_l, out_r, n);
}

#endif /* OMX_STRIP_H */
