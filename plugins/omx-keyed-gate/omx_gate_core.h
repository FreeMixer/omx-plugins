// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * omx_gate_core.h — omx-keyed-gate's binding to omx-dsp's gate kernel: the ONE place both faces
 * (omx_gate_clap.c, omx_gate_lv2.c) reach the DSP. It holds no DSP of its own: it hands the
 * declared parameter values to <omxdsp/fx/omx_gate_instance.h>, whose resolve is the console's
 * gate resolution (below mode, peak detector, hard knee, unity make-up, detector oversampling on
 * auto) and whose run is the console's own keyed gate kernel, omx_dynamics_keyed.
 *
 * The key is a block the host hands in, or NULL when nothing is routed to it: then, as when the
 * Key parameter is Self, the gate listens to its own input, bit for bit the kernel's NULL key.
 * Every control lands inside the declared travel inside the instance core (a NaN floors), so a
 * face only reads its ports and calls these.
 */
#ifndef OMX_GATE_CORE_H
#define OMX_GATE_CORE_H

#include <stdint.h>

/* The generated table FIRST: its guard is omx-dsp's own, so a kernel header reads this table. */
#include "omx_gate_params.h"
#include <omxdsp/fx/omx_gate_instance.h>

/** The instance core and the rate it was armed at. */
typedef struct {
  OmxGateInstance inst;
  float rate;
} OmxGateCore;

/** A fresh instance at `rate`: the gate's state cleared, the controls at the declared defaults.
 * Returns 0 for a rate that is not positive. */
static inline int omx_gate_core_init(OmxGateCore *c, float rate) {
  c->rate = rate;
  return omx_gate_instance_init(&c->inst, rate);
}

/** The block's parameters (declaration order) and the host's bypass. The Key parameter picks the
 * key (Sidechain, >= 0.5) or the gate's own input (Self); a NaN is Self. */
static inline void omx_gate_core_resolve(OmxGateCore *c, const float *v, int bypass) {
  omx_gate_instance_resolve(&c->inst, bypass, v[OMX_GATE_PARAM_KEY_EXTERNAL] >= 0.5f, v[OMX_GATE_PARAM_THRESHOLD],
                            v[OMX_GATE_PARAM_RATIO], v[OMX_GATE_PARAM_RANGE], v[OMX_GATE_PARAM_ATTACK],
                            v[OMX_GATE_PARAM_RELEASE]);
}

/** The latency the kernel adds, in samples: OMX_OVS_LATENCY_4X while the attack engages the 4x
 * detector path, else 0. */
static inline uint32_t omx_gate_core_latency(const OmxGateCore *c) {
  return (uint32_t)omx_gate_instance_latency(&c->inst);
}

/** One block, stereo, keyed by `key` (NULL: the gate's own input); the outputs may alias the
 * inputs or the key. */
static inline void omx_gate_core_run(OmxGateCore *c, const float *key, const float *in_l, const float *in_r,
                                     float *out_l, float *out_r, uint32_t frames) {
  omx_gate_instance_run(&c->inst, key, in_l, in_r, out_l, out_r, frames);
}

#endif /* OMX_GATE_CORE_H */
