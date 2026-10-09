// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * omx_phaser_core.h — omx-phaser's binding to omx-dsp's phaser kernel: the ONE place both faces
 * (omx_phaser_clap.c, omx_phaser_lv2.c) reach the DSP. It holds no DSP of its own: it resolves the
 * declared parameter values into the kernel's control atom (<omxdsp/fx/omx_phaser.h>) once per
 * block and calls omx_phaser_process on the kernel's caller-owned, fixed-size state.
 *
 * Units: Rate (Hz) becomes the oscillator's turns per sample, Base (Hz) and Depth (octaves) pass
 * straight through, Stages is rounded and handed to the kernel as the even count it runs, Feedback
 * is clamped to the kernel's travel and Mix (percent) becomes the convex wet/dry in [0, 1]. A
 * non-finite value reads as the parameter's declared default. Bypass is the kernel's `enabled`
 * low: it returns before a sample or a state word is touched, so the output is the input.
 * Nothing here allocates, locks or makes a system call; the state lives in the instance.
 */
#ifndef OMX_PHASER_CORE_H
#define OMX_PHASER_CORE_H

#include <math.h>
#include <stdint.h>
#include <string.h>

/* The generated table FIRST: its guard is omx-dsp's own, so a kernel header reads this table. */
#include "omx_phaser_params.h"
#include <omxdsp/fx/omx_phaser.h>

/** The kernel's state and what a block runs with. */
typedef struct {
  uint32_t rate;
  float values[OMX_PHASER_PARAM_COUNT]; /* the declared parameters, resolved once per block */
  int bypass;
  struct omx_phaser atom;        /* the kernel's control atom for the block */
  struct omx_phaser_state state; /* the kernel's caller-owned state */
} OmxPhaserCore;

/** A declared parameter inside its travel; a non-finite value reads as the declared default. */
static inline float omx_phaser_core_param(const float *v, uint32_t i) {
  return omx_clamp_or(v[i], OMX_PHASER_PARAMS[i].min, OMX_PHASER_PARAMS[i].max, OMX_PHASER_PARAMS[i].def);
}

/** The kernel's atom for the declared values (declaration order) at `rate`. */
static inline struct omx_phaser omx_phaser_core_atom(const float *v, uint32_t rate, int bypass) {
  struct omx_phaser p;
  p.enabled = !bypass;
  p.stages = omx_phaser_stages_run((int)lrintf(omx_phaser_core_param(v, OMX_PHASER_PARAM_STAGES)));
  p.base_hz = omx_phaser_core_param(v, OMX_PHASER_PARAM_BASE);
  p.depth_oct = omx_phaser_core_param(v, OMX_PHASER_PARAM_DEPTH);
  p.lfo_inc = omx_lfo_inc(omx_phaser_core_param(v, OMX_PHASER_PARAM_RATE), (float)rate);
  p.feedback = omx_phaser_clamp_fb(omx_phaser_core_param(v, OMX_PHASER_PARAM_FEEDBACK));
  p.mix = omx_unit(omx_phaser_core_param(v, OMX_PHASER_PARAM_MIX) * 0.01f);
  return p;
}

/** A fresh instance at `rate`: every state word zero, the parameters at their declared defaults. */
static inline void omx_phaser_core_init(OmxPhaserCore *c, uint32_t rate) {
  memset(c, 0, sizeof *c);
  c->rate = rate;
  for (uint32_t i = 0; i < OMX_PHASER_PARAM_COUNT; ++i) c->values[i] = OMX_PHASER_PARAMS[i].def;
  omx_phaser_state_init(&c->state);
  c->atom = omx_phaser_core_atom(c->values, rate, 0);
}

/** The block's parameters (declaration order) and the host's bypass. */
static inline void omx_phaser_core_resolve(OmxPhaserCore *c, const float *values, int bypass) {
  memcpy(c->values, values, sizeof c->values);
  c->bypass = bypass;
  c->atom = omx_phaser_core_atom(c->values, c->rate, bypass);
}

/** The latency the kernel adds, in samples: none, the chain is causal and runs in place. */
static inline uint32_t omx_phaser_core_latency(const OmxPhaserCore *c) {
  (void)c;
  return 0;
}

/** One block, stereo; the outputs may alias the inputs. */
static inline void omx_phaser_core_run(OmxPhaserCore *c, const float *in_l, const float *in_r, float *out_l,
                                         float *out_r, uint32_t frames) {
  if (out_l != in_l) memmove(out_l, in_l, frames * sizeof(float));
  if (out_r != in_r) memmove(out_r, in_r, frames * sizeof(float));
  omx_phaser_process(out_l, out_r, frames, &c->atom, &c->state, (float)c->rate);
}

#endif /* OMX_PHASER_CORE_H */
