// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#ifndef OMX_FLANGER_PARAMS_H
#define OMX_FLANGER_PARAMS_H
/*
 * GENERATED — DO NOT EDIT BY HAND.
 * Produced by tools/gen.mjs from plugins/omx-flanger/omx-flanger.decl.json.
 * Regenerate: `make -C plugins/omx-flanger gen`, then commit the result. Order is append-only.
 * The include guard is omx-dsp's own <omxdsp/params/omx_flanger_params.h>: included first, this
 * table is the one the kernel's instance header reads.
 */
#include "omx_plugin_param.h"

enum {
  OMX_FLANGER_PARAM_RATE = 0,
  OMX_FLANGER_PARAM_DEPTH = 1,
  OMX_FLANGER_PARAM_FEEDBACK = 2,
  OMX_FLANGER_PARAM_MIX = 3,
  OMX_FLANGER_PARAM_COUNT = 4
};

static const omx_plugin_param OMX_FLANGER_PARAMS[OMX_FLANGER_PARAM_COUNT] = {
  { "rate", "Rate", "Hz", 0.05f, 5.0f, 0.25f, 0u },
  { "depth", "Depth", "ms", 0.0f, 5.0f, 2.0f, 0u },
  { "feedback", "Feedback", "", -0.95f, 0.95f, 0.6f, 0u },
  { "mix", "Mix", "%", 0.0f, 100.0f, 50.0f, 0u },
};

/* One macro per declared bound: what a C face reads where a constant is needed. */
#define OMX_FLANGER_PARAM_RATE_MIN 0.05f
#define OMX_FLANGER_PARAM_RATE_MAX 5.0f
#define OMX_FLANGER_PARAM_RATE_DEFAULT 0.25f
#define OMX_FLANGER_PARAM_DEPTH_MIN 0.0f
#define OMX_FLANGER_PARAM_DEPTH_MAX 5.0f
#define OMX_FLANGER_PARAM_DEPTH_DEFAULT 2.0f
#define OMX_FLANGER_PARAM_FEEDBACK_MIN -0.95f
#define OMX_FLANGER_PARAM_FEEDBACK_MAX 0.95f
#define OMX_FLANGER_PARAM_FEEDBACK_DEFAULT 0.6f
#define OMX_FLANGER_PARAM_MIX_MIN 0.0f
#define OMX_FLANGER_PARAM_MIX_MAX 100.0f
#define OMX_FLANGER_PARAM_MIX_DEFAULT 50.0f

/* The identity every face publishes. */
#define OMX_FLANGER_NAME "omx flanger"
#define OMX_FLANGER_VENDOR "openmixer"
#define OMX_FLANGER_URL "https://github.com/FreeMixer/omx-plugins"
#define OMX_FLANGER_VERSION "0.2.0"
#define OMX_FLANGER_DESCRIPTION "the native FLANGER stage: one modulated fractional delay with FEEDBACK."
#define OMX_FLANGER_CLAP_ID "org.openmixer.flanger"
#define OMX_FLANGER_CLAP_FEATURES "audio-effect", "flanger", "stereo"
#define OMX_FLANGER_LV2_URI "urn:openmixer:flanger"
/* org.openmixer.declaration/1: the declaration's plain description and the digest of its parameters. */
#define OMX_FLANGER_DECL_SOURCE "omx flanger: Rate 0.05 to 5 Hz, Depth 0 to 5 ms, Feedback -0.95 to 0.95, Mix 0 to 100 %"
#define OMX_FLANGER_DECL_DIGEST "323560b13042be1596a25511cdd1cabcddfc4f976cb7c1ab68f318f2ad8fc73e"

/* LV2 port indices (tools/gen.mjs lv2Ports): audio, then parameter i at FIRST_PARAM + i, then these. */
#define OMX_FLANGER_LV2_PORT_IN_L 0u
#define OMX_FLANGER_LV2_PORT_IN_R 1u
#define OMX_FLANGER_LV2_PORT_OUT_L 2u
#define OMX_FLANGER_LV2_PORT_OUT_R 3u
#define OMX_FLANGER_LV2_PORT_ENABLED 8u
#define OMX_FLANGER_LV2_PORT_LATENCY 9u
#define OMX_FLANGER_LV2_PORT_FIRST_PARAM 4u
#define OMX_FLANGER_LV2_PORT_COUNT 10u

#endif /* OMX_FLANGER_PARAMS_H */
