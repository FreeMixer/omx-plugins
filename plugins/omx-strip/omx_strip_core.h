// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * omx_strip_core.h — omx-strip's binding to its chain of omx-dsp instance faces, Input > Filters > Gate > EQ > Comp: the ONE
 * place both faces (omx_strip_clap.c, omx_strip_lv2.c) reach the DSP. It holds no DSP of its
 * own: it copies in to out once, then runs each element's face in place on out, in the chosen order.
 * Each element's parameters go, by name, to its own face's resolve(); its switch off, or the host's
 * bypass, is that face's bypass. The latency is the sum of the elements'. Plain inline state: nothing
 * is allocated, locked or called into the system, in process or at activate.
 *
 * GENERATED — DO NOT EDIT BY HAND. Produced by tools/gen.mjs from plugins/omx-strip/omx-strip.decl.json and
 * each face's resolve() prototype (spec 2026-10-09-plugin-from-contract §15.2).
 * filter: `bandType` is not a parameter; it is passed at "default".
 * filter: `freq` is not a parameter; it is passed at "default".
 * filter: `gain` is not a parameter; it is passed at "default".
 * filter: `q` is not a parameter; it is passed at "default".
 * filter: `bandOn` is not a parameter; it is passed at "off".
 * gate: `keySource` is not a parameter; it is passed at "self".
 * eq: `hpfOn` is not a parameter; it is passed at "off".
 * eq: `hpfFreq` is not a parameter; it is passed at "default".
 * eq: `hpfSlope` is not a parameter; it is passed at "default".
 * eq: `lpfOn` is not a parameter; it is passed at "off".
 * eq: `lpfFreq` is not a parameter; it is passed at "default".
 * eq: `lpfSlope` is not a parameter; it is passed at "default".
 */
#ifndef OMX_STRIP_CORE_H
#define OMX_STRIP_CORE_H

#include <math.h>
#include <stdint.h>
#include <string.h>

/* The generated table FIRST, then the faces. */
#include "omx_strip_params.h"

#include <omxdsp/fx/omx_trim_instance.h>
#include <omxdsp/fx/omx_eq_instance.h>
#include <omxdsp/fx/omx_gate_instance.h>
#include <omxdsp/fx/omx_comp_instance.h>
_Static_assert(8 == OMX_EQ_INSTANCE_BANDS, "the declaration names one filter band_type parameter per band of the face");
_Static_assert(8 == OMX_EQ_INSTANCE_BANDS, "the declaration names one filter freq parameter per band of the face");
_Static_assert(8 == OMX_EQ_INSTANCE_BANDS, "the declaration names one filter gain parameter per band of the face");
_Static_assert(8 == OMX_EQ_INSTANCE_BANDS, "the declaration names one filter q parameter per band of the face");
_Static_assert(8 == OMX_EQ_INSTANCE_BANDS, "the declaration names one filter band_on parameter per band of the face");
_Static_assert(8 == OMX_EQ_INSTANCE_BANDS, "the declaration names one eq band_type parameter per band of the face");
_Static_assert(8 == OMX_EQ_INSTANCE_BANDS, "the declaration names one eq freq parameter per band of the face");
_Static_assert(8 == OMX_EQ_INSTANCE_BANDS, "the declaration names one eq gain parameter per band of the face");
_Static_assert(8 == OMX_EQ_INSTANCE_BANDS, "the declaration names one eq q parameter per band of the face");
_Static_assert(8 == OMX_EQ_INSTANCE_BANDS, "the declaration names one eq band_on parameter per band of the face");

/* The elements, in declared order, and every order they may run in (the `order` parameter's value is
 * the row: lexicographic, 0 the declared order). */
enum { OMX_STRIP_CHAIN_0_trim = 0, OMX_STRIP_CHAIN_1_filter = 1, OMX_STRIP_CHAIN_2_gate = 2, OMX_STRIP_CHAIN_3_eq = 3, OMX_STRIP_CHAIN_4_comp = 4, OMX_STRIP_CHAIN_LENGTH = 5 };
#define OMX_STRIP_CHAIN_ORDERS 120u
static const uint8_t OMX_STRIP_CHAIN_ORDER[OMX_STRIP_CHAIN_ORDERS][OMX_STRIP_CHAIN_LENGTH] = {
    { 0, 1, 2, 3, 4 },
    { 0, 1, 2, 4, 3 },
    { 0, 1, 3, 2, 4 },
    { 0, 1, 3, 4, 2 },
    { 0, 1, 4, 2, 3 },
    { 0, 1, 4, 3, 2 },
    { 0, 2, 1, 3, 4 },
    { 0, 2, 1, 4, 3 },
    { 0, 2, 3, 1, 4 },
    { 0, 2, 3, 4, 1 },
    { 0, 2, 4, 1, 3 },
    { 0, 2, 4, 3, 1 },
    { 0, 3, 1, 2, 4 },
    { 0, 3, 1, 4, 2 },
    { 0, 3, 2, 1, 4 },
    { 0, 3, 2, 4, 1 },
    { 0, 3, 4, 1, 2 },
    { 0, 3, 4, 2, 1 },
    { 0, 4, 1, 2, 3 },
    { 0, 4, 1, 3, 2 },
    { 0, 4, 2, 1, 3 },
    { 0, 4, 2, 3, 1 },
    { 0, 4, 3, 1, 2 },
    { 0, 4, 3, 2, 1 },
    { 1, 0, 2, 3, 4 },
    { 1, 0, 2, 4, 3 },
    { 1, 0, 3, 2, 4 },
    { 1, 0, 3, 4, 2 },
    { 1, 0, 4, 2, 3 },
    { 1, 0, 4, 3, 2 },
    { 1, 2, 0, 3, 4 },
    { 1, 2, 0, 4, 3 },
    { 1, 2, 3, 0, 4 },
    { 1, 2, 3, 4, 0 },
    { 1, 2, 4, 0, 3 },
    { 1, 2, 4, 3, 0 },
    { 1, 3, 0, 2, 4 },
    { 1, 3, 0, 4, 2 },
    { 1, 3, 2, 0, 4 },
    { 1, 3, 2, 4, 0 },
    { 1, 3, 4, 0, 2 },
    { 1, 3, 4, 2, 0 },
    { 1, 4, 0, 2, 3 },
    { 1, 4, 0, 3, 2 },
    { 1, 4, 2, 0, 3 },
    { 1, 4, 2, 3, 0 },
    { 1, 4, 3, 0, 2 },
    { 1, 4, 3, 2, 0 },
    { 2, 0, 1, 3, 4 },
    { 2, 0, 1, 4, 3 },
    { 2, 0, 3, 1, 4 },
    { 2, 0, 3, 4, 1 },
    { 2, 0, 4, 1, 3 },
    { 2, 0, 4, 3, 1 },
    { 2, 1, 0, 3, 4 },
    { 2, 1, 0, 4, 3 },
    { 2, 1, 3, 0, 4 },
    { 2, 1, 3, 4, 0 },
    { 2, 1, 4, 0, 3 },
    { 2, 1, 4, 3, 0 },
    { 2, 3, 0, 1, 4 },
    { 2, 3, 0, 4, 1 },
    { 2, 3, 1, 0, 4 },
    { 2, 3, 1, 4, 0 },
    { 2, 3, 4, 0, 1 },
    { 2, 3, 4, 1, 0 },
    { 2, 4, 0, 1, 3 },
    { 2, 4, 0, 3, 1 },
    { 2, 4, 1, 0, 3 },
    { 2, 4, 1, 3, 0 },
    { 2, 4, 3, 0, 1 },
    { 2, 4, 3, 1, 0 },
    { 3, 0, 1, 2, 4 },
    { 3, 0, 1, 4, 2 },
    { 3, 0, 2, 1, 4 },
    { 3, 0, 2, 4, 1 },
    { 3, 0, 4, 1, 2 },
    { 3, 0, 4, 2, 1 },
    { 3, 1, 0, 2, 4 },
    { 3, 1, 0, 4, 2 },
    { 3, 1, 2, 0, 4 },
    { 3, 1, 2, 4, 0 },
    { 3, 1, 4, 0, 2 },
    { 3, 1, 4, 2, 0 },
    { 3, 2, 0, 1, 4 },
    { 3, 2, 0, 4, 1 },
    { 3, 2, 1, 0, 4 },
    { 3, 2, 1, 4, 0 },
    { 3, 2, 4, 0, 1 },
    { 3, 2, 4, 1, 0 },
    { 3, 4, 0, 1, 2 },
    { 3, 4, 0, 2, 1 },
    { 3, 4, 1, 0, 2 },
    { 3, 4, 1, 2, 0 },
    { 3, 4, 2, 0, 1 },
    { 3, 4, 2, 1, 0 },
    { 4, 0, 1, 2, 3 },
    { 4, 0, 1, 3, 2 },
    { 4, 0, 2, 1, 3 },
    { 4, 0, 2, 3, 1 },
    { 4, 0, 3, 1, 2 },
    { 4, 0, 3, 2, 1 },
    { 4, 1, 0, 2, 3 },
    { 4, 1, 0, 3, 2 },
    { 4, 1, 2, 0, 3 },
    { 4, 1, 2, 3, 0 },
    { 4, 1, 3, 0, 2 },
    { 4, 1, 3, 2, 0 },
    { 4, 2, 0, 1, 3 },
    { 4, 2, 0, 3, 1 },
    { 4, 2, 1, 0, 3 },
    { 4, 2, 1, 3, 0 },
    { 4, 2, 3, 0, 1 },
    { 4, 2, 3, 1, 0 },
    { 4, 3, 0, 1, 2 },
    { 4, 3, 0, 2, 1 },
    { 4, 3, 1, 0, 2 },
    { 4, 3, 1, 2, 0 },
    { 4, 3, 2, 0, 1 },
    { 4, 3, 2, 1, 0 },
};

/** Each element's state, and the order a block runs in. */
typedef struct {
  uint32_t rate;
  uint32_t order;
  OmxTrimInstance trim;
  OmxEqInstance filter;
  OmxGateInstance gate;
  OmxEqInstance eq;
  OmxCompInstance comp;
} OmxStripCore;

/** A fresh chain at `rate`: every state word zero, each face bound to the rate. A rate a face does not
 * declare leaves that element a wire. */
static inline void omx_strip_core_init(OmxStripCore *c, uint32_t rate) {
  memset(c, 0, sizeof *c);
  c->rate = rate;
  (void)omx_trim_instance_init(&c->trim, (float)rate);
  (void)omx_eq_instance_init(&c->filter, (float)rate);
  (void)omx_gate_instance_init(&c->gate, (float)rate);
  (void)omx_eq_instance_init(&c->eq, (float)rate);
  (void)omx_comp_instance_init(&c->comp, (float)rate);
}

/** The block's parameters (declaration order) and the host's bypass, each element's to its own face. */
static inline void omx_strip_core_resolve(OmxStripCore *c, const float *values, int bypass) {
  omx_trim_instance_resolve(&c->trim, bypass || values[OMX_STRIP_PARAM_TRIM_ON] < 0.5f,
                                  /* trim_db */ values[OMX_STRIP_PARAM_TRIM_DB]);
  omx_eq_instance_resolve(&c->filter, bypass || values[OMX_STRIP_PARAM_FILTER_ON] < 0.5f,
                                  /* hpf_on */ (int)lrintf(values[OMX_STRIP_PARAM_HPF_ON]),
                                  /* hpf_freq */ values[OMX_STRIP_PARAM_HPF_FREQ],
                                  /* hpf_slope */ (int)lrintf(values[OMX_STRIP_PARAM_HPF_SLOPE]),
                                  /* lpf_on */ (int)lrintf(values[OMX_STRIP_PARAM_LPF_ON]),
                                  /* lpf_freq */ values[OMX_STRIP_PARAM_LPF_FREQ],
                                  /* lpf_slope */ (int)lrintf(values[OMX_STRIP_PARAM_LPF_SLOPE]),
                                  /* band_type */ (const int[OMX_EQ_INSTANCE_BANDS]){1, 0, 0, 0, 0, 0, 0, 2},
                                  /* freq */ (const float[OMX_EQ_INSTANCE_BANDS]){31.5f, 80.0f, 160.0f, 400.0f, 1000.0f, 2500.0f, 5000.0f, 12500.0f},
                                  /* gain */ (const float[OMX_EQ_INSTANCE_BANDS]){0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
                                  /* q */ (const float[OMX_EQ_INSTANCE_BANDS]){1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
                                  /* band_on */ (const int[OMX_EQ_INSTANCE_BANDS]){0, 0, 0, 0, 0, 0, 0, 0});
  omx_gate_instance_resolve(&c->gate, bypass || values[OMX_STRIP_PARAM_GATE_ON] < 0.5f,
                                  /* key_source */ 0,
                                  /* threshold_db */ values[OMX_STRIP_PARAM_GATE_THRESHOLD],
                                  /* range_db */ values[OMX_STRIP_PARAM_GATE_RANGE],
                                  /* knee_start_db */ values[OMX_STRIP_PARAM_GATE_KNEE_START],
                                  /* knee_end_db */ values[OMX_STRIP_PARAM_GATE_KNEE_END],
                                  /* attack_ms */ values[OMX_STRIP_PARAM_GATE_ATTACK],
                                  /* hold_ms */ values[OMX_STRIP_PARAM_GATE_HOLD],
                                  /* release_ms */ values[OMX_STRIP_PARAM_GATE_RELEASE],
                                  /* hysteresis_db */ values[OMX_STRIP_PARAM_GATE_HYSTERESIS],
                                  /* ratio */ values[OMX_STRIP_PARAM_GATE_RATIO]);
  omx_eq_instance_resolve(&c->eq, bypass || values[OMX_STRIP_PARAM_EQ_ON] < 0.5f,
                                  /* hpf_on */ 0,
                                  /* hpf_freq */ 20.0f,
                                  /* hpf_slope */ 12,
                                  /* lpf_on */ 0,
                                  /* lpf_freq */ 1000.0f,
                                  /* lpf_slope */ 12,
                                  /* band_type */ (const int[OMX_EQ_INSTANCE_BANDS]){(int)lrintf(values[OMX_STRIP_PARAM_EQ1_TYPE]), (int)lrintf(values[OMX_STRIP_PARAM_EQ2_TYPE]), (int)lrintf(values[OMX_STRIP_PARAM_EQ3_TYPE]), (int)lrintf(values[OMX_STRIP_PARAM_EQ4_TYPE]), (int)lrintf(values[OMX_STRIP_PARAM_EQ5_TYPE]), (int)lrintf(values[OMX_STRIP_PARAM_EQ6_TYPE]), (int)lrintf(values[OMX_STRIP_PARAM_EQ7_TYPE]), (int)lrintf(values[OMX_STRIP_PARAM_EQ8_TYPE])},
                                  /* freq */ (const float[OMX_EQ_INSTANCE_BANDS]){values[OMX_STRIP_PARAM_EQ1_FREQ], values[OMX_STRIP_PARAM_EQ2_FREQ], values[OMX_STRIP_PARAM_EQ3_FREQ], values[OMX_STRIP_PARAM_EQ4_FREQ], values[OMX_STRIP_PARAM_EQ5_FREQ], values[OMX_STRIP_PARAM_EQ6_FREQ], values[OMX_STRIP_PARAM_EQ7_FREQ], values[OMX_STRIP_PARAM_EQ8_FREQ]},
                                  /* gain */ (const float[OMX_EQ_INSTANCE_BANDS]){values[OMX_STRIP_PARAM_EQ1_GAIN], values[OMX_STRIP_PARAM_EQ2_GAIN], values[OMX_STRIP_PARAM_EQ3_GAIN], values[OMX_STRIP_PARAM_EQ4_GAIN], values[OMX_STRIP_PARAM_EQ5_GAIN], values[OMX_STRIP_PARAM_EQ6_GAIN], values[OMX_STRIP_PARAM_EQ7_GAIN], values[OMX_STRIP_PARAM_EQ8_GAIN]},
                                  /* q */ (const float[OMX_EQ_INSTANCE_BANDS]){values[OMX_STRIP_PARAM_EQ1_Q], values[OMX_STRIP_PARAM_EQ2_Q], values[OMX_STRIP_PARAM_EQ3_Q], values[OMX_STRIP_PARAM_EQ4_Q], values[OMX_STRIP_PARAM_EQ5_Q], values[OMX_STRIP_PARAM_EQ6_Q], values[OMX_STRIP_PARAM_EQ7_Q], values[OMX_STRIP_PARAM_EQ8_Q]},
                                  /* band_on */ (const int[OMX_EQ_INSTANCE_BANDS]){(int)lrintf(values[OMX_STRIP_PARAM_EQ1_ON]), (int)lrintf(values[OMX_STRIP_PARAM_EQ2_ON]), (int)lrintf(values[OMX_STRIP_PARAM_EQ3_ON]), (int)lrintf(values[OMX_STRIP_PARAM_EQ4_ON]), (int)lrintf(values[OMX_STRIP_PARAM_EQ5_ON]), (int)lrintf(values[OMX_STRIP_PARAM_EQ6_ON]), (int)lrintf(values[OMX_STRIP_PARAM_EQ7_ON]), (int)lrintf(values[OMX_STRIP_PARAM_EQ8_ON])});
  omx_comp_instance_resolve(&c->comp, bypass || values[OMX_STRIP_PARAM_COMP_ON] < 0.5f,
                                  /* threshold_db */ values[OMX_STRIP_PARAM_COMP_THRESHOLD],
                                  /* ratio */ values[OMX_STRIP_PARAM_COMP_RATIO],
                                  /* knee_db */ values[OMX_STRIP_PARAM_COMP_KNEE],
                                  /* attack_ms */ values[OMX_STRIP_PARAM_COMP_ATTACK],
                                  /* release_ms */ values[OMX_STRIP_PARAM_COMP_RELEASE],
                                  /* makeup_db */ values[OMX_STRIP_PARAM_COMP_MAKEUP],
                                  /* mix_pct */ values[OMX_STRIP_PARAM_COMP_MIX],
                                  /* kind */ (int)lrintf(values[OMX_STRIP_PARAM_COMP_KIND]),
                                  /* detector_oversampling */ (int)lrintf(values[OMX_STRIP_PARAM_COMP_DETECTOR_OVERSAMPLING]));
  const float o = values[OMX_STRIP_PARAM_ORDER];
  const long row = o - o == 0.0f ? lrintf(o) : 0;
  c->order = row >= 0 && row < (long)OMX_STRIP_CHAIN_ORDERS ? (uint32_t)row : 0u;
}

/** The latency the chain adds, in samples: the sum of its elements'. */
static inline uint32_t omx_strip_core_latency(const OmxStripCore *c) {
  (void)c;
  return (uint32_t)lrintf((float)(omx_trim_instance_latency(&c->trim)) + (float)(omx_eq_instance_latency(&c->filter)) + (float)(omx_gate_instance_latency(&c->gate)) + (float)(omx_eq_instance_latency(&c->eq)) + (float)(omx_comp_instance_latency(&c->comp)));
}

/** One element over the block, in place on out. */
static inline void omx_strip_core_element(OmxStripCore *c, uint32_t e, const float *key, float *out_l, float *out_r,
                                            uint32_t frames) {
  (void)key;
  switch (e) {
  case OMX_STRIP_CHAIN_0_trim:
    omx_trim_instance_run(&c->trim, out_l, out_r, out_l, out_r, frames);
    return;
  case OMX_STRIP_CHAIN_1_filter:
    omx_eq_instance_run(&c->filter, out_l, out_r, out_l, out_r, frames);
    return;
  case OMX_STRIP_CHAIN_2_gate:
    omx_gate_instance_run(&c->gate, NULL, out_l, out_r, out_l, out_r, frames);
    return;
  case OMX_STRIP_CHAIN_3_eq:
    omx_eq_instance_run(&c->eq, out_l, out_r, out_l, out_r, frames);
    return;
  case OMX_STRIP_CHAIN_4_comp:
    omx_comp_instance_run(&c->comp, out_l, out_r, out_l, out_r, frames);
    return;
  }
}

/** One block, stereo: in to out once (they may alias), then
 * each element in the chosen order. */
static inline void omx_strip_core_run(OmxStripCore *c, const float *in_l, const float *in_r, float *out_l,
                                         float *out_r, uint32_t frames) {
  if (!in_l || !in_r || !out_l || !out_r || frames == 0u) return;
  if (out_l != in_l) memmove(out_l, in_l, (size_t)frames * sizeof(float));
  if (out_r != in_r) memmove(out_r, in_r, (size_t)frames * sizeof(float));
  const float *key = NULL;
  for (uint32_t k = 0; k < OMX_STRIP_CHAIN_LENGTH; k++) omx_strip_core_element(c, OMX_STRIP_CHAIN_ORDER[c->order][k], key, out_l, out_r, frames);
}

#endif /* OMX_STRIP_CORE_H */
