// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#ifndef OMX_LIMITER_PARAMS_H
#define OMX_LIMITER_PARAMS_H
/*
 * GENERATED — DO NOT EDIT BY HAND.
 * Produced by tools/gen.mjs from plugins/omx-limiter/omx-limiter.decl.json.
 * Regenerate: `make -C plugins/omx-limiter gen`, then commit the result. Order is append-only.
 * The include guard is omx-dsp's own <omxdsp/params/omx_limiter_params.h>: included first, this
 * table is the one the kernel's instance header reads.
 */
#include "omx_plugin_param.h"

enum {
  OMX_LIMITER_PARAM_CEILING_DB = 0,
  OMX_LIMITER_PARAM_LOOKAHEAD_MS = 1,
  OMX_LIMITER_PARAM_RELEASE_MS = 2,
  OMX_LIMITER_PARAM_COUNT = 3
};

static const omx_plugin_param OMX_LIMITER_PARAMS[OMX_LIMITER_PARAM_COUNT] = {
  { "ceilingDb", "Ceiling", "dBFS", -12.0f, 0.0f, -1.0f, 0u },
  { "lookaheadMs", "Lookahead", "ms", 0.5f, 5.0f, 1.5f, 0u },
  { "releaseMs", "Release", "ms", 1.0f, 1000.0f, 50.0f, OMX_PLUGIN_PARAM_INTEGER },
};

/* One macro per declared bound: what a C face reads where a constant is needed. */
#define OMX_LIMITER_PARAM_CEILING_DB_MIN -12.0f
#define OMX_LIMITER_PARAM_CEILING_DB_MAX 0.0f
#define OMX_LIMITER_PARAM_CEILING_DB_DEFAULT -1.0f
#define OMX_LIMITER_PARAM_LOOKAHEAD_MS_MIN 0.5f
#define OMX_LIMITER_PARAM_LOOKAHEAD_MS_MAX 5.0f
#define OMX_LIMITER_PARAM_LOOKAHEAD_MS_DEFAULT 1.5f
#define OMX_LIMITER_PARAM_RELEASE_MS_MIN 1.0f
#define OMX_LIMITER_PARAM_RELEASE_MS_MAX 1000.0f
#define OMX_LIMITER_PARAM_RELEASE_MS_DEFAULT 50.0f

/* The identity every face publishes. */
#define OMX_LIMITER_NAME "omx limiter"
#define OMX_LIMITER_VENDOR "openmixer"
#define OMX_LIMITER_URL "https://github.com/FreeMixer/omx-plugins"
#define OMX_LIMITER_VERSION "0.2.0"
#define OMX_LIMITER_DESCRIPTION "The precision limiter: a look-ahead, true-peak, stereo-linked brickwall."
#define OMX_LIMITER_CLAP_ID "org.openmixer.limiter"
#define OMX_LIMITER_CLAP_FEATURES "audio-effect", "limiter", "stereo"
#define OMX_LIMITER_LV2_URI "urn:openmixer:limiter"
/* org.openmixer.declaration/1: the declaration's plain description and the digest of its parameters. */
#define OMX_LIMITER_DECL_SOURCE "omx limiter: Ceiling -12 to 0 dBFS, Lookahead 0.5 to 5 ms, Release 1 to 1000 ms"
#define OMX_LIMITER_DECL_DIGEST "aaae50a0d0219bc9406793d74ddc52d57717d8a4433e4d39f174cd5ef6e4c52c"

/* LV2 port indices (tools/gen.mjs lv2Ports): audio, then parameter i at FIRST_PARAM + i, then these. */
#define OMX_LIMITER_LV2_PORT_IN_L 0u
#define OMX_LIMITER_LV2_PORT_IN_R 1u
#define OMX_LIMITER_LV2_PORT_OUT_L 2u
#define OMX_LIMITER_LV2_PORT_OUT_R 3u
#define OMX_LIMITER_LV2_PORT_ENABLED 7u
#define OMX_LIMITER_LV2_PORT_LATENCY 8u
#define OMX_LIMITER_LV2_PORT_FIRST_PARAM 4u
#define OMX_LIMITER_LV2_PORT_COUNT 9u

#endif /* OMX_LIMITER_PARAMS_H */
