// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * omx_delay_core.h — omx-delay's binding to omx-dsp's delay kernel: the ONE place both faces
 * (omx_delay_clap.c, omx_delay_lv2.c) reach the DSP. It holds no DSP of its own: it hands the
 * declared parameter values, by name, to omx-dsp's instance face (<omxdsp/fx/omx_delay_instance.h>)
 * and runs it. The instance is plain inline state: nothing is allocated, locked or called into the
 * system, in process or at activate.
 *
 * GENERATED — DO NOT EDIT BY HAND. Produced by tools/gen.mjs from plugins/omx-delay/omx-delay.decl.json and
 * the face's resolve() prototype: each argument takes the parameter whose contract control it names.
 * `fx_delay_taps` is no parameter: the declaration holds it at its kernel's declared default, OMX_FX_DELAY_TAPS_RANGE_DEFAULT.
 * `tap1_factor` is no parameter: the declaration holds it at its kernel's declared default, OMX_DELAY_TAP1_FACTORS_DEFAULT.
 * `tap2_factor` is no parameter: the declaration holds it at its kernel's declared default, OMX_DELAY_TAP2_FACTORS_DEFAULT.
 * `tap3_factor` is no parameter: the declaration holds it at its kernel's declared default, OMX_DELAY_TAP3_FACTORS_DEFAULT.
 * `sync` is no parameter: the declaration holds it at its kernel's declared default, OMX_DELAY_SYNCS_DEFAULT.
 * `division` is no parameter: the declaration holds it at its kernel's declared default, OMX_DELAY_DIVISIONS_DEFAULT.
 * `mode` is no parameter: the declaration holds it at its kernel's declared default, OMX_DELAY_MODES_DEFAULT.
 * `duck_db` is no parameter: the declaration holds it at its kernel's declared default, OMX_DELAY_DUCK_DB_RANGE_DEFAULT.
 * `duck_threshold_db` is no parameter: the declaration holds it at its kernel's declared default, OMX_DELAY_DUCK_THRESHOLD_DB_RANGE_DEFAULT.
 * `duck_release_ms` is no parameter: the declaration holds it at its kernel's declared default, OMX_DELAY_DUCK_RELEASE_MS_RANGE_DEFAULT.
 * `tap0Gain` is not a parameter of this plugin: it is passed at "default".
 * `tap0Pan` is not a parameter of this plugin: it is passed at "default".
 * `tap1Gain` is not a parameter of this plugin: it is passed at "default".
 * `tap1Pan` is not a parameter of this plugin: it is passed at "default".
 * `tap2Gain` is not a parameter of this plugin: it is passed at "default".
 * `tap2Pan` is not a parameter of this plugin: it is passed at "default".
 * `tap3Gain` is not a parameter of this plugin: it is passed at "default".
 * `tap3Pan` is not a parameter of this plugin: it is passed at "default".
 */
#ifndef OMX_DELAY_CORE_H
#define OMX_DELAY_CORE_H

#include <math.h>
#include <stdint.h>
#include <string.h>

/* The generated table FIRST: its guard is omx-dsp's own, so the instance header reads this table. */
#include "omx_delay_params.h"

#include <omxdsp/fx/omx_delay_instance.h>

/** The kernel's state and what a block runs with. */
typedef struct {
  uint32_t rate;
  OmxDelayInstance inst;
} OmxDelayCore;

/** A fresh instance at `rate`: every state word zero. A rate the kernel does not declare leaves it a wire. */
static inline void omx_delay_core_init(OmxDelayCore *c, uint32_t rate) {
  memset(c, 0, sizeof *c);
  c->rate = rate;
  (void)omx_delay_instance_init(&c->inst, (float)rate);
}

/** The block's parameters (declaration order) and the host's bypass, each to its resolve() argument. */
static inline void omx_delay_core_resolve(OmxDelayCore *c, const float *values, int bypass) {
  omx_delay_instance_resolve(&c->inst, bypass,
                                  /* fx_delay_time */ values[OMX_DELAY_PARAM_FX_DELAY_TIME],
                                  /* fx_delay_feedback */ values[OMX_DELAY_PARAM_FX_DELAY_FEEDBACK],
                                  /* tone */ values[OMX_DELAY_PARAM_TONE],
                                  /* mix */ values[OMX_DELAY_PARAM_MIX],
                                  /* pingpong */ (int)lrintf(values[OMX_DELAY_PARAM_PINGPONG]),
                                  /* fx_delay_taps */ (float)OMX_FX_DELAY_TAPS_RANGE_DEFAULT,
                                  /* tap0_gain */ 1.0f,
                                  /* tap0_pan */ 0.0f,
                                  /* tap1_factor */ (int)OMX_DELAY_TAP1_FACTORS_DEFAULT,
                                  /* tap1_gain */ 0.7f,
                                  /* tap1_pan */ -0.5f,
                                  /* tap2_factor */ (int)OMX_DELAY_TAP2_FACTORS_DEFAULT,
                                  /* tap2_gain */ 0.5f,
                                  /* tap2_pan */ 0.5f,
                                  /* tap3_factor */ (int)OMX_DELAY_TAP3_FACTORS_DEFAULT,
                                  /* tap3_gain */ 0.35f,
                                  /* tap3_pan */ 0.0f,
                                  /* sync */ (int)OMX_DELAY_SYNCS_DEFAULT,
                                  /* division */ (int)OMX_DELAY_DIVISIONS_DEFAULT,
                                  /* mode */ (int)OMX_DELAY_MODES_DEFAULT,
                                  /* duck_db */ (float)OMX_DELAY_DUCK_DB_RANGE_DEFAULT,
                                  /* duck_threshold_db */ (float)OMX_DELAY_DUCK_THRESHOLD_DB_RANGE_DEFAULT,
                                  /* duck_release_ms */ (float)OMX_DELAY_DUCK_RELEASE_MS_RANGE_DEFAULT);
}

/** The latency the kernel adds, in samples. */
static inline uint32_t omx_delay_core_latency(const OmxDelayCore *c) {
  (void)c;
  return (uint32_t)lrintf((float)(omx_delay_instance_latency(&c->inst)));
}

/** One block, stereo; the outputs may alias the inputs. */
static inline void omx_delay_core_run(OmxDelayCore *c, const float *in_l, const float *in_r, float *out_l,
                                         float *out_r, uint32_t frames) {
  omx_delay_instance_run(&c->inst, in_l, in_r, out_l, out_r, frames);
}

#endif /* OMX_DELAY_CORE_H */
