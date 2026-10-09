// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * omx_deesser_core.h — omx-deesser's binding to omx-dsp's deesser kernel: the ONE place both faces
 * (omx_deesser_clap.c, omx_deesser_lv2.c) reach the DSP. It holds no DSP of its own: it fills the
 * kernel's controls from the declared parameter values and calls the kernel.
 */
#ifndef OMX_DEESSER_CORE_H
#define OMX_DEESSER_CORE_H

#include <math.h>
#include <stdint.h>
#include <string.h>

#include <omxdsp/fx/omx_deesser.h>
#include <omxdsp/omx_eq_design.h>

/* The generated table FIRST: its guard is omx-dsp's own, so a kernel header reads this table. */
#include "omx_deesser_params.h"

/** The kernel's state and what a block runs with. */
typedef struct {
  uint32_t rate;
  float values[OMX_DEESSER_PARAM_COUNT]; /* the declared parameters, resolved once per block */
  int bypass;
  struct omx_deess atom;       /* the kernel's resolved controls, rebuilt each block */
  struct omx_deess_state state; /* plain inline floats: nothing to allocate */
} OmxDeesserCore;

/* The stage's constants, which the declaration does not carry (the console holds them too): the
 * gain computer's knee, in dB. Detection is PEAK and the make-up is unity: a de-esser only
 * attenuates. */
#define OMX_DEESSER_KNEE_DB 6.0f

/* Fill the kernel's atom from the declared values: the dynamics atom the compressor's own gain
 * computer reads, and the one bandpass section at freqHz whose Q the cookbook's bandwidth-in-octaves
 * relation gives widthOct. RT-safe: libm on the allowlist, no allocation. */
static inline void omx_deesser_core_atom(OmxDeesserCore *c) {
  const float *v = c->values;
  const float sr = (float)c->rate;
  struct omx_deess *p = &c->atom;
  memset(p, 0, sizeof *p);
  p->enabled = !c->bypass;
  p->mode = OMX_DEESS_SPLIT;
  p->dyn.enabled = 1;
  p->dyn.gc.mode = OMX_DYN_ABOVE;
  p->dyn.detect = OMX_DETECT_PEAK;
  p->dyn.gc.thresh_db = v[OMX_DEESSER_PARAM_THRESHOLD_DB];
  p->dyn.gc.ratio = v[OMX_DEESSER_PARAM_RATIO];
  p->dyn.gc.knee_db = OMX_DEESSER_KNEE_DB;
  p->dyn.gc.range_db = v[OMX_DEESSER_PARAM_RANGE_DB];
  p->dyn.gc.makeup_lin = 1.0f;
  p->dyn.attack_ms = v[OMX_DEESSER_PARAM_ATTACK_MS];
  p->dyn.ovs_mode = OMX_DYN_OVS_OFF;
  p->dyn.attack_coeff = omx_pole_from_time_ms(v[OMX_DEESSER_PARAM_ATTACK_MS], sr);
  p->dyn.release_coeff = omx_pole_from_time_ms(v[OMX_DEESSER_PARAM_RELEASE_MS], sr);
  const double f0 = fmin((double)v[OMX_DEESSER_PARAM_FREQ_HZ], 0.45 * (double)c->rate);
  const double w0 = 2.0 * 3.14159265358979323846 * f0 / (double)c->rate;
  const double q = 1.0 / (2.0 * sinh(0.34657359027997264 * (double)v[OMX_DEESSER_PARAM_WIDTH_OCT] * w0 / sin(w0)));
  omx_eq_design_f(OMX_EQ_BANDPASS, f0, q, 0.0, (double)c->rate, p->bp_c);
}

/** A fresh instance at `rate`: every state word zero, the parameters at their declared defaults. */
static inline void omx_deesser_core_init(OmxDeesserCore *c, uint32_t rate) {
  memset(c, 0, sizeof *c);
  c->rate = rate;
  for (uint32_t i = 0; i < OMX_DEESSER_PARAM_COUNT; ++i) c->values[i] = OMX_DEESSER_PARAMS[i].def;
  omx_deess_state_init(&c->state);
  omx_deesser_core_atom(c);
}

/** The block's parameters (declaration order) and the host's bypass. */
static inline void omx_deesser_core_resolve(OmxDeesserCore *c, const float *values, int bypass) {
  memcpy(c->values, values, sizeof c->values);
  c->bypass = bypass;
  omx_deesser_core_atom(c);
}

/** The latency the kernel adds, in samples. */
static inline uint32_t omx_deesser_core_latency(const OmxDeesserCore *c) {
  return (uint32_t)omx_deess_latency(&c->atom);
}

/** One block, stereo; the outputs may alias the inputs. */
static inline void omx_deesser_core_run(OmxDeesserCore *c, const float *in_l, const float *in_r, float *out_l,
                                         float *out_r, uint32_t frames) {
  if (out_l != in_l) memmove(out_l, in_l, frames * sizeof(float));
  if (out_r != in_r) memmove(out_r, in_r, frames * sizeof(float));
  omx_deess_process(out_l, out_r, frames, &c->atom, &c->state);
}

#endif /* OMX_DEESSER_CORE_H */
