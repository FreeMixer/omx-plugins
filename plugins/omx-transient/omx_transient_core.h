// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * omx_transient_core.h — omx-transient's binding to omx-dsp's transient kernel: the ONE place both faces
 * (omx_transient_clap.c, omx_transient_lv2.c) reach the DSP. It holds no DSP of its own: it fills the
 * kernel's controls from the declared parameter values and calls the kernel.
 *
 * OMX_WIZARD_STUB: written by tools/omx-new-plugin.mjs. Until it is replaced, the binding passes the
 * input through and no parameter reaches a kernel, so `clap-probe live` and test/transient-oracle.c
 * are red. Replace the body of each function with the calls into omx-dsp's transient kernel
 * (<omxdsp/fx/omx_transient.h>, or its instance core), then delete this paragraph and the marker
 * line below.
 */
#ifndef OMX_TRANSIENT_CORE_H
#define OMX_TRANSIENT_CORE_H

#define OMX_WIZARD_STUB 1

#include <stdint.h>
#include <string.h>

/* The generated table FIRST: its guard is omx-dsp's own, so a kernel header reads this table. */
#include "omx_transient_params.h"

/** The kernel's state and what a block runs with. */
typedef struct {
  uint32_t rate;
  float values[OMX_TRANSIENT_PARAM_COUNT]; /* the declared parameters, resolved once per block */
  int bypass;
} OmxTransientCore;

/** A fresh instance at `rate`: every state word zero, the parameters at their declared defaults. */
static inline void omx_transient_core_init(OmxTransientCore *c, uint32_t rate) {
  memset(c, 0, sizeof *c);
  c->rate = rate;
  for (uint32_t i = 0; i < OMX_TRANSIENT_PARAM_COUNT; ++i) c->values[i] = OMX_TRANSIENT_PARAMS[i].def;
}

/** The block's parameters (declaration order) and the host's bypass. */
static inline void omx_transient_core_resolve(OmxTransientCore *c, const float *values, int bypass) {
  memcpy(c->values, values, sizeof c->values);
  c->bypass = bypass;
}

/** The latency the kernel adds, in samples. */
static inline uint32_t omx_transient_core_latency(const OmxTransientCore *c) {
  (void)c;
  return 0;
}

/** One block, stereo; the outputs may alias the inputs. */
static inline void omx_transient_core_run(OmxTransientCore *c, const float *in_l, const float *in_r, float *out_l,
                                         float *out_r, uint32_t frames) {
  (void)c;
  if (out_l != in_l) memmove(out_l, in_l, frames * sizeof(float));
  if (out_r != in_r) memmove(out_r, in_r, frames * sizeof(float));
}

#endif /* OMX_TRANSIENT_CORE_H */
