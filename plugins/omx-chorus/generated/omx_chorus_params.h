// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#ifndef OMX_CHORUS_PARAMS_H
#define OMX_CHORUS_PARAMS_H
/*
 * GENERATED — DO NOT EDIT BY HAND.
 * Produced by tools/gen.mjs from plugins/omx-chorus/omx-chorus.decl.json.
 * Regenerate: `make -C plugins/omx-chorus gen`, then commit the result. Order is append-only.
 * The include guard is omx-dsp's own <omxdsp/params/omx_chorus_params.h>: included first, this
 * table is the one the kernel's instance header reads.
 */
#include "omx_plugin_param.h"

enum {
  OMX_CHORUS_PARAM_RATE_HZ = 0,
  OMX_CHORUS_PARAM_DEPTH_MS = 1,
  OMX_CHORUS_PARAM_VOICES = 2,
  OMX_CHORUS_PARAM_MIX = 3,
  OMX_CHORUS_PARAM_SPREAD = 4,
  OMX_CHORUS_PARAM_COUNT = 5
};

static const omx_plugin_param OMX_CHORUS_PARAMS[OMX_CHORUS_PARAM_COUNT] = {
  { "rateHz", "Rate", "Hz", 0.05f, 8.0f, 0.6f, 0u },
  { "depthMs", "Depth", "ms", 0.0f, 12.0f, 4.0f, 0u },
  { "voices", "Voices", "", 1.0f, 4.0f, 3.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "mix", "Mix", "%", 0.0f, 100.0f, 35.0f, 0u },
  { "spread", "Spread", "", 0.0f, 0.5f, 0.0f, 0u },
};

/* One macro per declared bound: what a C face reads where a constant is needed. */
#define OMX_CHORUS_PARAM_RATE_HZ_MIN 0.05f
#define OMX_CHORUS_PARAM_RATE_HZ_MAX 8.0f
#define OMX_CHORUS_PARAM_RATE_HZ_DEFAULT 0.6f
#define OMX_CHORUS_PARAM_DEPTH_MS_MIN 0.0f
#define OMX_CHORUS_PARAM_DEPTH_MS_MAX 12.0f
#define OMX_CHORUS_PARAM_DEPTH_MS_DEFAULT 4.0f
#define OMX_CHORUS_PARAM_VOICES_MIN 1.0f
#define OMX_CHORUS_PARAM_VOICES_MAX 4.0f
#define OMX_CHORUS_PARAM_VOICES_DEFAULT 3.0f
#define OMX_CHORUS_PARAM_MIX_MIN 0.0f
#define OMX_CHORUS_PARAM_MIX_MAX 100.0f
#define OMX_CHORUS_PARAM_MIX_DEFAULT 35.0f
#define OMX_CHORUS_PARAM_SPREAD_MIN 0.0f
#define OMX_CHORUS_PARAM_SPREAD_MAX 0.5f
#define OMX_CHORUS_PARAM_SPREAD_DEFAULT 0.0f

/* The identity every face publishes. */
#define OMX_CHORUS_NAME "omx chorus"
#define OMX_CHORUS_VENDOR "openmixer"
#define OMX_CHORUS_URL "https://github.com/FreeMixer/omx-plugins"
#define OMX_CHORUS_VERSION "0.1.0"
#define OMX_CHORUS_DESCRIPTION "the native CHORUS stage: N voices reading ONE modulated fractional delay line."
#define OMX_CHORUS_CLAP_ID "org.openmixer.chorus"
#define OMX_CHORUS_CLAP_FEATURES "audio-effect", "stereo"
#define OMX_CHORUS_LV2_URI "urn:openmixer:chorus"
/* org.openmixer.declaration/1: the declaration's plain description and the digest of its parameters. */
#define OMX_CHORUS_DECL_SOURCE "omx chorus: Rate 0.05 to 8 Hz, Depth 0 to 12 ms, Voices 1 to 4, Mix 0 to 100 %, Spread 0 to 0.5"
#define OMX_CHORUS_DECL_DIGEST "9282092ea121c8b503a493f00e4e744a4ea23bc7b29f3e2b5f0e265b3218f185"

/* LV2 port indices (tools/gen.mjs lv2Ports): audio, then parameter i at FIRST_PARAM + i, then these. */
#define OMX_CHORUS_LV2_PORT_IN_L 0u
#define OMX_CHORUS_LV2_PORT_IN_R 1u
#define OMX_CHORUS_LV2_PORT_OUT_L 2u
#define OMX_CHORUS_LV2_PORT_OUT_R 3u
#define OMX_CHORUS_LV2_PORT_ENABLED 9u
#define OMX_CHORUS_LV2_PORT_LATENCY 10u
#define OMX_CHORUS_LV2_PORT_FIRST_PARAM 4u
#define OMX_CHORUS_LV2_PORT_COUNT 11u

#endif /* OMX_CHORUS_PARAMS_H */
