// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#ifndef OMX_PHASER_PARAMS_H
#define OMX_PHASER_PARAMS_H
/*
 * GENERATED — DO NOT EDIT BY HAND.
 * Produced by tools/gen.mjs from plugins/omx-phaser/omx-phaser.decl.json.
 * Regenerate: `make -C plugins/omx-phaser gen`, then commit the result. Order is append-only.
 * The include guard is omx-dsp's own <omxdsp/params/omx_phaser_params.h>: included first, this
 * table is the one the kernel's instance header reads.
 */
#include "omx_plugin_param.h"

enum {
  OMX_PHASER_PARAM_RATE_HZ = 0,
  OMX_PHASER_PARAM_BASE_HZ = 1,
  OMX_PHASER_PARAM_DEPTH_OCT = 2,
  OMX_PHASER_PARAM_STAGES = 3,
  OMX_PHASER_PARAM_FEEDBACK = 4,
  OMX_PHASER_PARAM_MIX = 5,
  OMX_PHASER_PARAM_COUNT = 6
};

static const omx_plugin_param OMX_PHASER_PARAMS[OMX_PHASER_PARAM_COUNT] = {
  { "rateHz", "Rate", "Hz", 0.05f, 5.0f, 0.5f, 0u },
  { "baseHz", "Base", "Hz", 50.0f, 2000.0f, 200.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "depthOct", "Depth", "oct", 0.0f, 6.0f, 4.0f, 0u },
  { "stages", "Stages", "", 2.0f, 12.0f, 6.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "feedback", "Feedback", "", -0.9f, 0.9f, 0.4f, 0u },
  { "mix", "Mix", "%", 0.0f, 100.0f, 50.0f, 0u },
};

/* One macro per declared bound: what a C face reads where a constant is needed. */
#define OMX_PHASER_PARAM_RATE_HZ_MIN 0.05f
#define OMX_PHASER_PARAM_RATE_HZ_MAX 5.0f
#define OMX_PHASER_PARAM_RATE_HZ_DEFAULT 0.5f
#define OMX_PHASER_PARAM_BASE_HZ_MIN 50.0f
#define OMX_PHASER_PARAM_BASE_HZ_MAX 2000.0f
#define OMX_PHASER_PARAM_BASE_HZ_DEFAULT 200.0f
#define OMX_PHASER_PARAM_DEPTH_OCT_MIN 0.0f
#define OMX_PHASER_PARAM_DEPTH_OCT_MAX 6.0f
#define OMX_PHASER_PARAM_DEPTH_OCT_DEFAULT 4.0f
#define OMX_PHASER_PARAM_STAGES_MIN 2.0f
#define OMX_PHASER_PARAM_STAGES_MAX 12.0f
#define OMX_PHASER_PARAM_STAGES_DEFAULT 6.0f
#define OMX_PHASER_PARAM_FEEDBACK_MIN -0.9f
#define OMX_PHASER_PARAM_FEEDBACK_MAX 0.9f
#define OMX_PHASER_PARAM_FEEDBACK_DEFAULT 0.4f
#define OMX_PHASER_PARAM_MIX_MIN 0.0f
#define OMX_PHASER_PARAM_MIX_MAX 100.0f
#define OMX_PHASER_PARAM_MIX_DEFAULT 50.0f

/* The identity every face publishes. */
#define OMX_PHASER_NAME "omx phaser"
#define OMX_PHASER_VENDOR "openmixer"
#define OMX_PHASER_URL "https://github.com/FreeMixer/omx-plugins"
#define OMX_PHASER_VERSION "0.2.0"
#define OMX_PHASER_DESCRIPTION "The console's phaser: identical all-pass sections swept in octaves by one oscillator, with feedback around the chain and a wet/dry mix."
#define OMX_PHASER_CLAP_ID "org.openmixer.phaser"
#define OMX_PHASER_CLAP_FEATURES "audio-effect", "phaser", "stereo"
#define OMX_PHASER_LV2_URI "urn:openmixer:phaser"
/* org.openmixer.declaration/1: the declaration's plain description and the digest of its parameters. */
#define OMX_PHASER_DECL_SOURCE "omx phaser: Rate 0.05 to 5 Hz, Base 50 to 2000 Hz, Depth 0 to 6 oct, Stages 2 to 12, Feedback -0.9 to 0.9, Mix 0 to 100 %"
#define OMX_PHASER_DECL_DIGEST "d39a2a6935c7d0e32234d86d28aa84bcb731db7c680d1cd28dd517edff5fe512"

/* LV2 port indices (tools/gen.mjs lv2Ports): audio, then parameter i at FIRST_PARAM + i, then these. */
#define OMX_PHASER_LV2_PORT_IN_L 0u
#define OMX_PHASER_LV2_PORT_IN_R 1u
#define OMX_PHASER_LV2_PORT_OUT_L 2u
#define OMX_PHASER_LV2_PORT_OUT_R 3u
#define OMX_PHASER_LV2_PORT_ENABLED 10u
#define OMX_PHASER_LV2_PORT_LATENCY 11u
#define OMX_PHASER_LV2_PORT_FIRST_PARAM 4u
#define OMX_PHASER_LV2_PORT_COUNT 12u

#endif /* OMX_PHASER_PARAMS_H */
