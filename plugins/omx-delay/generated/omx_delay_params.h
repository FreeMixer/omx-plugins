// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#ifndef OMX_DELAY_PARAMS_H
#define OMX_DELAY_PARAMS_H
/*
 * GENERATED — DO NOT EDIT BY HAND.
 * Produced by tools/gen.mjs from plugins/omx-delay/omx-delay.decl.json (openmixer's
 * CONSOLE_TRAVEL_DECLS['/channel/{kind}/{index}/delay'], limitForKind(DELAY_MIX_RANGE, 'input'), FX_DELAY_PINGPONG_DEFAULT).
 * Regenerate: `make -C plugins/omx-delay gen`, then commit the result. Order is append-only.
 * The include guard is omx-dsp's own <omxdsp/params/omx_delay_params.h>: included first, this
 * table is the one the kernel's instance header reads.
 */
#include "omx_plugin_param.h"

enum {
  OMX_DELAY_PARAM_TIME_MS = 0,
  OMX_DELAY_PARAM_FEEDBACK = 1,
  OMX_DELAY_PARAM_MIX = 2,
  OMX_DELAY_PARAM_TONE = 3,
  OMX_DELAY_PARAM_PINGPONG = 4,
  OMX_DELAY_PARAM_COUNT = 5
};

static const omx_plugin_param OMX_DELAY_PARAMS[OMX_DELAY_PARAM_COUNT] = {
  { "timeMs", "Time", "ms", 0.0f, 2000.0f, 300.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "feedback", "Feedback", "", 0.0f, 0.99f, 0.3f, 0u },
  { "mix", "Mix", "", 0.0f, 1.0f, 0.3f, 0u },
  { "tone", "Tone", "", 0.0f, 1.0f, 0.3f, 0u },
  { "pingpong", "Pingpong", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
};

/* One macro per declared bound: what a C face reads where a constant is needed. */
#define OMX_DELAY_PARAM_TIME_MS_MIN 0.0f
#define OMX_DELAY_PARAM_TIME_MS_MAX 2000.0f
#define OMX_DELAY_PARAM_TIME_MS_DEFAULT 300.0f
#define OMX_DELAY_PARAM_FEEDBACK_MIN 0.0f
#define OMX_DELAY_PARAM_FEEDBACK_MAX 0.99f
#define OMX_DELAY_PARAM_FEEDBACK_DEFAULT 0.3f
#define OMX_DELAY_PARAM_MIX_MIN 0.0f
#define OMX_DELAY_PARAM_MIX_MAX 1.0f
#define OMX_DELAY_PARAM_MIX_DEFAULT 0.3f
#define OMX_DELAY_PARAM_TONE_MIN 0.0f
#define OMX_DELAY_PARAM_TONE_MAX 1.0f
#define OMX_DELAY_PARAM_TONE_DEFAULT 0.3f
#define OMX_DELAY_PARAM_PINGPONG_MIN 0.0f
#define OMX_DELAY_PARAM_PINGPONG_MAX 1.0f
#define OMX_DELAY_PARAM_PINGPONG_DEFAULT 0.0f

/* The identity every face publishes. */
#define OMX_DELAY_NAME "omx delay"
#define OMX_DELAY_VENDOR "openmixer"
#define OMX_DELAY_URL "https://github.com/FreeMixer/omx-plugins"
#define OMX_DELAY_VERSION "0.1.0"
#define OMX_DELAY_DESCRIPTION "openmixer's stereo delay: one time, feedback through a tone filter, ping-pong and a wet/dry mix. The DSP is omx-dsp's <omxdsp/fx/omx_delay.h>, the console's own delay."
#define OMX_DELAY_CLAP_ID "org.openmixer.delay"
#define OMX_DELAY_CLAP_FEATURES "audio-effect", "delay", "stereo"
#define OMX_DELAY_LV2_URI "urn:openmixer:delay"
/* org.openmixer.declaration/1: the source expression and the digest of the resolved parameters. */
#define OMX_DELAY_DECL_SOURCE "CONSOLE_TRAVEL_DECLS['/channel/{kind}/{index}/delay'], limitForKind(DELAY_MIX_RANGE, 'input'), FX_DELAY_PINGPONG_DEFAULT"
#define OMX_DELAY_DECL_DIGEST "68548b33d9f285d8a5795ca46ca9a74993b0e5d2be87580acffea96ea34ec77f"

/* LV2 port indices (tools/gen.mjs lv2Ports): audio, then parameter i at FIRST_PARAM + i, then these. */
#define OMX_DELAY_LV2_PORT_IN_L 0u
#define OMX_DELAY_LV2_PORT_IN_R 1u
#define OMX_DELAY_LV2_PORT_OUT_L 2u
#define OMX_DELAY_LV2_PORT_OUT_R 3u
#define OMX_DELAY_LV2_PORT_ENABLED 9u
#define OMX_DELAY_LV2_PORT_LATENCY 10u
#define OMX_DELAY_LV2_PORT_FIRST_PARAM 4u
#define OMX_DELAY_LV2_PORT_COUNT 11u

#endif /* OMX_DELAY_PARAMS_H */
