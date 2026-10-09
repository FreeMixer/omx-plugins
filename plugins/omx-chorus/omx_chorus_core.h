// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * omx_chorus_core.h — omx-chorus's binding to omx-dsp's chorus kernel: the ONE place both faces
 * (omx_chorus_clap.c, omx_chorus_lv2.c) reach the DSP. It holds no DSP of its own: it hands the
 * declared parameter values to omx-dsp's chorus instance core (<omxdsp/fx/omx_chorus_instance.h>),
 * which is the console's chorus kernel in a plugin's port model, and calls it.
 *
 * Real-time safe in process: no allocation, no lock, no syscall. Both delay rings are inside the
 * core, sized for the highest rate the kernel admits (OMX_CHORUS_CAP), so activating at any rate
 * allocates nothing; a rate above that is refused by the kernel and the core then passes the input
 * through.
 */
#ifndef OMX_CHORUS_CORE_H
#define OMX_CHORUS_CORE_H

#include <stdint.h>
#include <string.h>

/* The generated table FIRST: its guard is omx-dsp's own, so a kernel header reads this table. */
#include "omx_chorus_params.h"

#include <omxdsp/fx/omx_chorus_instance.h>

/** The declared parameters' positions (declaration order, append-only). */
enum { OMX_CHORUS_P_RATE_HZ, OMX_CHORUS_P_DEPTH_MS, OMX_CHORUS_P_VOICES, OMX_CHORUS_P_MIX, OMX_CHORUS_P_SPREAD };

/** The kernel's state and what a block runs with. */
typedef struct {
  uint32_t rate;
  float values[OMX_CHORUS_PARAM_COUNT]; /* the declared parameters, resolved once per block */
  int bypass;
  float ring_l[OMX_CHORUS_CAP];
  float ring_r[OMX_CHORUS_CAP];
  OmxChorusInstance inst;
} OmxChorusCore;

/** A fresh instance at `rate`: every state word zero, the parameters at their declared defaults. */
static inline void omx_chorus_core_init(OmxChorusCore *c, uint32_t rate) {
  memset(c, 0, sizeof *c);
  c->rate = rate;
  for (uint32_t i = 0; i < OMX_CHORUS_PARAM_COUNT; ++i) c->values[i] = OMX_CHORUS_PARAMS[i].def;
  (void)omx_chorus_instance_init(&c->inst, (float)rate, c->ring_l, c->ring_r, OMX_CHORUS_CAP);
}

/** The block's parameters (declaration order) and the host's bypass. */
static inline void omx_chorus_core_resolve(OmxChorusCore *c, const float *values, int bypass) {
  memcpy(c->values, values, sizeof c->values);
  c->bypass = bypass;
  omx_chorus_instance_resolve(&c->inst, bypass, c->values[OMX_CHORUS_P_VOICES],
                              c->values[OMX_CHORUS_P_DEPTH_MS], c->values[OMX_CHORUS_P_RATE_HZ],
                              c->values[OMX_CHORUS_P_MIX], c->values[OMX_CHORUS_P_SPREAD]);
}

/** The latency the kernel adds, in samples. */
static inline uint32_t omx_chorus_core_latency(const OmxChorusCore *c) {
  (void)c;
  return (uint32_t)OMX_CHORUS_INSTANCE_LATENCY_FRAMES;
}

/** One block, stereo; the outputs may alias the inputs. */
static inline void omx_chorus_core_run(OmxChorusCore *c, const float *in_l, const float *in_r, float *out_l,
                                       float *out_r, uint32_t frames) {
  omx_chorus_instance_run(&c->inst, in_l, in_r, out_l, out_r, frames);
}

#endif /* OMX_CHORUS_CORE_H */
