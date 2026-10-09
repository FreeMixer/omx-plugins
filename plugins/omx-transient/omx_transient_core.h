// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * omx_transient_core.h — omx-transient's binding to omx-dsp's transient kernel: the ONE place both faces
 * (omx_transient_clap.c, omx_transient_lv2.c) reach the DSP. It holds no DSP of its own: it hands the
 * declared parameter values to omx-dsp's instance core (<omxdsp/fx/omx_transient_instance.h>, over the
 * kernel <omxdsp/fx/omx_transient.h>) and runs it. The instance is plain inline state: nothing is
 * allocated, locked or called into the system, in process or at activate.
 */
#ifndef OMX_TRANSIENT_CORE_H
#define OMX_TRANSIENT_CORE_H

#include <stdint.h>
#include <string.h>

/* The generated table FIRST: its guard is omx-dsp's own, so the instance header reads this table. */
#include "omx_transient_params.h"

#include <omxdsp/fx/omx_transient_instance.h>

/** The kernel's state and what a block runs with. */
typedef struct {
  uint32_t rate;
  OmxTransientInstance inst;
} OmxTransientCore;

/** A fresh instance at `rate`: every state word zero. A rate the kernel does not declare leaves it a wire. */
static inline void omx_transient_core_init(OmxTransientCore *c, uint32_t rate) {
  memset(c, 0, sizeof *c);
  c->rate = rate;
  (void)omx_transient_instance_init(&c->inst, (float)rate);
}

/** The block's parameters (declaration order) and the host's bypass. */
static inline void omx_transient_core_resolve(OmxTransientCore *c, const float *values, int bypass) {
  omx_transient_instance_resolve(&c->inst, bypass, values[OMX_TRANSIENT_PARAM_ATTACK_DB],
                                 values[OMX_TRANSIENT_PARAM_SUSTAIN_DB], values[OMX_TRANSIENT_PARAM_ATTACK_TIME_MS],
                                 values[OMX_TRANSIENT_PARAM_SUSTAIN_TIME_MS], values[OMX_TRANSIENT_PARAM_OUTPUT_DB]);
}

/** The latency the kernel adds, in samples. */
static inline uint32_t omx_transient_core_latency(const OmxTransientCore *c) {
  (void)c;
  return (uint32_t)omx_transient_latency();
}

/** One block, stereo; the outputs may alias the inputs. */
static inline void omx_transient_core_run(OmxTransientCore *c, const float *in_l, const float *in_r, float *out_l,
                                         float *out_r, uint32_t frames) {
  omx_transient_instance_run(&c->inst, in_l, in_r, out_l, out_r, frames);
}

#endif /* OMX_TRANSIENT_CORE_H */
