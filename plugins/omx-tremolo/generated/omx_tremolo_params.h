// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#ifndef OMX_TREMOLO_PARAMS_H
#define OMX_TREMOLO_PARAMS_H
/*
 * GENERATED — DO NOT EDIT BY HAND.
 * Produced by tools/gen.mjs from plugins/omx-tremolo/omx-tremolo.decl.json.
 * Regenerate: `make -C plugins/omx-tremolo gen`, then commit the result. Order is append-only.
 * The include guard is omx-dsp's own <omxdsp/params/omx_tremolo_params.h>: included first, this
 * table is the one the kernel's instance header reads.
 */
#include "omx_plugin_param.h"

enum {
  OMX_TREMOLO_PARAM_RATE_HZ = 0,
  OMX_TREMOLO_PARAM_DEPTH = 1,
  OMX_TREMOLO_PARAM_MIX = 2,
  OMX_TREMOLO_PARAM_MODE = 3,
  OMX_TREMOLO_PARAM_COUNT = 4
};

static const omx_plugin_param OMX_TREMOLO_PARAMS[OMX_TREMOLO_PARAM_COUNT] = {
  { "rateHz", "Rate", "Hz", 0.1f, 20.0f, 4.0f, 0u },
  { "depth", "Depth", "%", 0.0f, 100.0f, 50.0f, 0u },
  { "mix", "Mix", "%", 0.0f, 100.0f, 100.0f, 0u },
  { "mode", "Mode", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
};

/* One macro per declared bound: what a C face reads where a constant is needed. */
#define OMX_TREMOLO_PARAM_RATE_HZ_MIN 0.1f
#define OMX_TREMOLO_PARAM_RATE_HZ_MAX 20.0f
#define OMX_TREMOLO_PARAM_RATE_HZ_DEFAULT 4.0f
#define OMX_TREMOLO_PARAM_DEPTH_MIN 0.0f
#define OMX_TREMOLO_PARAM_DEPTH_MAX 100.0f
#define OMX_TREMOLO_PARAM_DEPTH_DEFAULT 50.0f
#define OMX_TREMOLO_PARAM_MIX_MIN 0.0f
#define OMX_TREMOLO_PARAM_MIX_MAX 100.0f
#define OMX_TREMOLO_PARAM_MIX_DEFAULT 100.0f
#define OMX_TREMOLO_PARAM_MODE_MIN 0.0f
#define OMX_TREMOLO_PARAM_MODE_MAX 1.0f
#define OMX_TREMOLO_PARAM_MODE_DEFAULT 0.0f

/* The identity every face publishes. */
#define OMX_TREMOLO_NAME "omx tremolo"
#define OMX_TREMOLO_VENDOR "openmixer"
#define OMX_TREMOLO_URL "https://github.com/FreeMixer/omx-plugins"
#define OMX_TREMOLO_VERSION "0.3.0"
#define OMX_TREMOLO_DESCRIPTION "The console's tremolo and auto-pan: one oscillator turned into a level change on both legs, or into a left-right pan."
#define OMX_TREMOLO_CLAP_ID "org.openmixer.tremolo"
#define OMX_TREMOLO_CLAP_FEATURES "audio-effect", "tremolo", "stereo"
#define OMX_TREMOLO_LV2_URI "urn:openmixer:tremolo"
/* org.openmixer.declaration/1: the declaration's plain description and the digest of its parameters. */
#define OMX_TREMOLO_DECL_SOURCE "omx tremolo: Rate 0.1 to 20 Hz, Depth 0 to 100 %, Mix 0 to 100 %, Mode 0 to 1"
#define OMX_TREMOLO_DECL_DIGEST "b57991962924d939b9966fa18b44ce86bb73f3c940611aaf165dd227ec9b6d1a"

/* LV2 port indices (tools/gen.mjs lv2Ports): audio, then parameter i at FIRST_PARAM + i, then these. */
#define OMX_TREMOLO_LV2_PORT_IN_L 0u
#define OMX_TREMOLO_LV2_PORT_IN_R 1u
#define OMX_TREMOLO_LV2_PORT_OUT_L 2u
#define OMX_TREMOLO_LV2_PORT_OUT_R 3u
#define OMX_TREMOLO_LV2_PORT_ENABLED 8u
#define OMX_TREMOLO_LV2_PORT_LATENCY 9u
#define OMX_TREMOLO_LV2_PORT_FIRST_PARAM 4u
#define OMX_TREMOLO_LV2_PORT_COUNT 10u

#endif /* OMX_TREMOLO_PARAMS_H */
