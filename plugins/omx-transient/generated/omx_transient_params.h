// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#ifndef OMX_TRANSIENT_PARAMS_H
#define OMX_TRANSIENT_PARAMS_H
/*
 * GENERATED — DO NOT EDIT BY HAND.
 * Produced by tools/gen.mjs from plugins/omx-transient/omx-transient.decl.json.
 * Regenerate: `make -C plugins/omx-transient gen`, then commit the result. Order is append-only.
 * The include guard is omx-dsp's own <omxdsp/params/omx_transient_params.h>: included first, this
 * table is the one the kernel's instance header reads.
 */
#include "omx_plugin_param.h"

enum {
  OMX_TRANSIENT_PARAM_ATTACK_DB = 0,
  OMX_TRANSIENT_PARAM_SUSTAIN_DB = 1,
  OMX_TRANSIENT_PARAM_ATTACK_TIME_MS = 2,
  OMX_TRANSIENT_PARAM_SUSTAIN_TIME_MS = 3,
  OMX_TRANSIENT_PARAM_OUTPUT_DB = 4,
  OMX_TRANSIENT_PARAM_COUNT = 5
};

static const omx_plugin_param OMX_TRANSIENT_PARAMS[OMX_TRANSIENT_PARAM_COUNT] = {
  { "attackDb", "Attack Db", "dB", -24.0f, 24.0f, 0.0f, 0u },
  { "sustainDb", "Sustain Db", "dB", -24.0f, 24.0f, 0.0f, 0u },
  { "attackTimeMs", "Attack Time Ms", "ms", 2.0f, 50.0f, 10.0f, 0u },
  { "sustainTimeMs", "Sustain Time Ms", "ms", 50.0f, 2000.0f, 250.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "outputDb", "Output Db", "dB", -24.0f, 12.0f, 0.0f, 0u },
};

/* One macro per declared bound: what a C face reads where a constant is needed. */
#define OMX_TRANSIENT_PARAM_ATTACK_DB_MIN -24.0f
#define OMX_TRANSIENT_PARAM_ATTACK_DB_MAX 24.0f
#define OMX_TRANSIENT_PARAM_ATTACK_DB_DEFAULT 0.0f
#define OMX_TRANSIENT_PARAM_SUSTAIN_DB_MIN -24.0f
#define OMX_TRANSIENT_PARAM_SUSTAIN_DB_MAX 24.0f
#define OMX_TRANSIENT_PARAM_SUSTAIN_DB_DEFAULT 0.0f
#define OMX_TRANSIENT_PARAM_ATTACK_TIME_MS_MIN 2.0f
#define OMX_TRANSIENT_PARAM_ATTACK_TIME_MS_MAX 50.0f
#define OMX_TRANSIENT_PARAM_ATTACK_TIME_MS_DEFAULT 10.0f
#define OMX_TRANSIENT_PARAM_SUSTAIN_TIME_MS_MIN 50.0f
#define OMX_TRANSIENT_PARAM_SUSTAIN_TIME_MS_MAX 2000.0f
#define OMX_TRANSIENT_PARAM_SUSTAIN_TIME_MS_DEFAULT 250.0f
#define OMX_TRANSIENT_PARAM_OUTPUT_DB_MIN -24.0f
#define OMX_TRANSIENT_PARAM_OUTPUT_DB_MAX 12.0f
#define OMX_TRANSIENT_PARAM_OUTPUT_DB_DEFAULT 0.0f

/* The identity every face publishes. */
#define OMX_TRANSIENT_NAME "omx transient"
#define OMX_TRANSIENT_VENDOR "openmixer"
#define OMX_TRANSIENT_URL "https://github.com/FreeMixer/omx-plugins"
#define OMX_TRANSIENT_VERSION "0.2.0"
#define OMX_TRANSIENT_DESCRIPTION "The native transient designer: a gain driven by two envelope contrasts, with no threshold."
#define OMX_TRANSIENT_CLAP_ID "org.openmixer.transient"
#define OMX_TRANSIENT_CLAP_FEATURES "audio-effect", "transient-shaper", "stereo"
#define OMX_TRANSIENT_LV2_URI "urn:openmixer:transient"
/* org.openmixer.declaration/1: the declaration's plain description and the digest of its parameters. */
#define OMX_TRANSIENT_DECL_SOURCE "omx transient: Attack Db -24 to 24 dB, Sustain Db -24 to 24 dB, Attack Time Ms 2 to 50 ms, Sustain Time Ms 50 to 2000 ms, Output Db -24 to 12 dB"
#define OMX_TRANSIENT_DECL_DIGEST "b735a9f45b45b01b0ef5660e8ac5e43729aaab9fd5b9c7cce4976934c82759a5"

/* LV2 port indices (tools/gen.mjs lv2Ports): audio, then parameter i at FIRST_PARAM + i, then these. */
#define OMX_TRANSIENT_LV2_PORT_IN_L 0u
#define OMX_TRANSIENT_LV2_PORT_IN_R 1u
#define OMX_TRANSIENT_LV2_PORT_OUT_L 2u
#define OMX_TRANSIENT_LV2_PORT_OUT_R 3u
#define OMX_TRANSIENT_LV2_PORT_ENABLED 9u
#define OMX_TRANSIENT_LV2_PORT_LATENCY 10u
#define OMX_TRANSIENT_LV2_PORT_FIRST_PARAM 4u
#define OMX_TRANSIENT_LV2_PORT_COUNT 11u

#endif /* OMX_TRANSIENT_PARAMS_H */
