// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * omx_comp_core.h — omx-comp's binding to omx-dsp's comp kernel: the ONE place both faces
 * (omx_comp_clap.c, omx_comp_lv2.c) reach the DSP. It holds no DSP of its own: it hands the
 * declared parameter values, by name, to omx-dsp's instance face (<omxdsp/fx/omx_comp_instance.h>)
 * and runs it. The instance is plain inline state: nothing is allocated, locked or called into the
 * system, in process or at activate.
 *
 * GENERATED — DO NOT EDIT BY HAND. Produced by tools/gen.mjs from plugins/omx-comp/omx-comp.decl.json and
 * the face's resolve() prototype: each argument takes the parameter whose contract control it names.
 */
#ifndef OMX_COMP_CORE_H
#define OMX_COMP_CORE_H

#include <math.h>
#include <stdint.h>
#include <string.h>

/* The generated table FIRST: its guard is omx-dsp's own, so the instance header reads this table. */
#include "omx_comp_params.h"

#include <omxdsp/fx/omx_comp_instance.h>

/** The kernel's state and what a block runs with. */
typedef struct {
  uint32_t rate;
  OmxCompInstance inst;
} OmxCompCore;

/** A fresh instance at `rate`: every state word zero. A rate the kernel does not declare leaves it a wire. */
static inline void omx_comp_core_init(OmxCompCore *c, uint32_t rate) {
  memset(c, 0, sizeof *c);
  c->rate = rate;
  (void)omx_comp_instance_init(&c->inst, (float)rate);
}

/** The block's parameters (declaration order) and the host's bypass, each to its resolve() argument. */
static inline void omx_comp_core_resolve(OmxCompCore *c, const float *values, int bypass) {
  omx_comp_instance_resolve(&c->inst, bypass,
                                  /* threshold_db */ values[OMX_COMP_PARAM_THRESHOLD_DB],
                                  /* ratio */ values[OMX_COMP_PARAM_RATIO],
                                  /* knee_db */ values[OMX_COMP_PARAM_KNEE_DB],
                                  /* attack_ms */ values[OMX_COMP_PARAM_ATTACK_MS],
                                  /* release_ms */ values[OMX_COMP_PARAM_RELEASE_MS],
                                  /* makeup_db */ values[OMX_COMP_PARAM_MAKEUP_DB],
                                  /* mix_pct */ values[OMX_COMP_PARAM_MIX_PCT],
                                  /* kind */ (int)lrintf(values[OMX_COMP_PARAM_KIND]),
                                  /* detector_oversampling */ (int)lrintf(values[OMX_COMP_PARAM_DETECTOR_OVERSAMPLING]));
}

/** The latency the kernel adds, in samples. */
static inline uint32_t omx_comp_core_latency(const OmxCompCore *c) {
  (void)c;
  return (uint32_t)lrintf((float)(omx_comp_instance_latency(&c->inst)));
}

/** One block, stereo; the outputs may alias the inputs. */
static inline void omx_comp_core_run(OmxCompCore *c, const float *in_l, const float *in_r, float *out_l,
                                         float *out_r, uint32_t frames) {
  omx_comp_instance_run(&c->inst, in_l, in_r, out_l, out_r, frames);
}

#endif /* OMX_COMP_CORE_H */
