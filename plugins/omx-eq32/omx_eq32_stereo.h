// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * omx_eq32_stereo.h — the one place the CLAP and the LV2 face of omx-eq32 meet the DSP. omx-dsp's
 * <omxdsp/fx/omx_eq_instance.h> is a MONO core (the console runs one per leg), so a stereo plugin
 * is two of them fed the same declared values. Both faces build the declared value array, call
 * omx_eq32_run() and nothing else: the CLAP/LV2 pair cannot differ in anything but their ports.
 *
 * This file adds no arithmetic on the audio path. The whole-EQ switch is where a bypass lands: an
 * off EQ is an empty bank, a wire, bit for bit.
 */
#ifndef OMX_EQ32_STEREO_H
#define OMX_EQ32_STEREO_H

#include <string.h>

#include "omx_eq32_params.h"

#define OMX_EQ_LV2_BANDS 32
#include <omxdsp/fx/omx_eq_instance.h>

/** The first band's declared index, and the width of one band: type, freq, gain, Q, on. */
#define OMX_EQ32_BAND0 OMX_EQ32_PARAM_B1_TYPE
#define OMX_EQ32_BAND_WIDTH 5u

_Static_assert(OMX_EQ32_PARAM_COUNT == OMX_EQ32_BAND0 + OMX_EQ32_BAND_WIDTH * OMX_EQ_LV2_BANDS,
               "the declaration is the controls struct's words, in its order");

typedef struct {
  struct omx_eq_lv2 leg[2];
} OmxEq32;

static inline void omx_eq32_init(OmxEq32 *e, double rate) {
  omx_eq_lv2_init(&e->leg[0], rate);
  omx_eq_lv2_init(&e->leg[1], rate);
}

/** `activate`: the history is cleared, the designs are kept. */
static inline void omx_eq32_reset(OmxEq32 *e) {
  omx_eq_lv2_reset_state(&e->leg[0]);
  omx_eq_lv2_reset_state(&e->leg[1]);
}

/** One block: `v` holds the declared values in declaration order; `bypass` makes the EQ a wire. */
static inline void omx_eq32_run(OmxEq32 *e, const float *v, int bypass, const float *in_l, const float *in_r,
                               float *out_l, float *out_r, uint32_t n) {
  const float on = bypass ? 0.0f : v[OMX_EQ32_PARAM_ON];
  struct omx_eq_lv2_controls c;
  memset(&c, 0, sizeof c);
  c.on = &on;
  c.hpf_on = &v[OMX_EQ32_PARAM_HPF_ON];
  c.hpf_freq = &v[OMX_EQ32_PARAM_HPF_FREQ];
  c.hpf_slope = &v[OMX_EQ32_PARAM_HPF_SLOPE];
  c.lpf_on = &v[OMX_EQ32_PARAM_LPF_ON];
  c.lpf_freq = &v[OMX_EQ32_PARAM_LPF_FREQ];
  c.lpf_slope = &v[OMX_EQ32_PARAM_LPF_SLOPE];
  for (uint32_t b = 0; b < (uint32_t)OMX_EQ_LV2_BANDS; b++)
    for (uint32_t k = 0; k < OMX_EQ32_BAND_WIDTH; k++) c.band[b][k] = &v[OMX_EQ32_BAND0 + b * OMX_EQ32_BAND_WIDTH + k];
  for (uint32_t leg = 0; leg < 2; leg++) {
    omx_eq_lv2_set_controls(&e->leg[leg], &c);
    omx_eq_lv2_run(&e->leg[leg], leg ? in_r : in_l, leg ? out_r : out_l, n);
  }
}

#endif /* OMX_EQ32_STEREO_H */
