// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#ifndef OMX_PITCH_PARAMS_H
#define OMX_PITCH_PARAMS_H
/*
 * GENERATED — DO NOT EDIT BY HAND.
 * Produced by tools/gen.mjs from plugins/omx-pitch/omx-pitch.decl.json.
 * Regenerate: `make -C plugins/omx-pitch gen`, then commit the result. Order is append-only.
 * The include guard is omx-dsp's own <omxdsp/params/omx_pitch_params.h>: included first, this
 * table is the one the kernel's instance header reads.
 */
#include "omx_plugin_param.h"

enum {
  OMX_PITCH_PARAM_SEMITONES = 0,
  OMX_PITCH_PARAM_CENTS = 1,
  OMX_PITCH_PARAM_MIX = 2,
  OMX_PITCH_PARAM_COUNT = 3
};

static const omx_plugin_param OMX_PITCH_PARAMS[OMX_PITCH_PARAM_COUNT] = {
  { "semitones", "Semitones", "", -12.0f, 12.0f, 0.0f, 0u },
  { "cents", "Cents", "", -50.0f, 50.0f, 0.0f, 0u },
  { "mix", "Mix", "", 0.0f, 100.0f, 100.0f, 0u },
};

/* One macro per declared bound: what a C face reads where a constant is needed. */
#define OMX_PITCH_PARAM_SEMITONES_MIN -12.0f
#define OMX_PITCH_PARAM_SEMITONES_MAX 12.0f
#define OMX_PITCH_PARAM_SEMITONES_DEFAULT 0.0f
#define OMX_PITCH_PARAM_CENTS_MIN -50.0f
#define OMX_PITCH_PARAM_CENTS_MAX 50.0f
#define OMX_PITCH_PARAM_CENTS_DEFAULT 0.0f
#define OMX_PITCH_PARAM_MIX_MIN 0.0f
#define OMX_PITCH_PARAM_MIX_MAX 100.0f
#define OMX_PITCH_PARAM_MIX_DEFAULT 100.0f

/* The identity every face publishes. */
#define OMX_PITCH_NAME "omx pitch"
#define OMX_PITCH_VENDOR "openmixer"
#define OMX_PITCH_URL "https://github.com/FreeMixer/omx-plugins"
#define OMX_PITCH_VERSION "0.2.0"
#define OMX_PITCH_DESCRIPTION "The console's pitch shifter: shifts both channels up or down by semitones and cents, without changing their length, mixed with the dry signal."
#define OMX_PITCH_CLAP_ID "org.openmixer.pitch"
#define OMX_PITCH_CLAP_FEATURES "audio-effect", "pitch-shifter", "stereo"
#define OMX_PITCH_LV2_URI "urn:openmixer:pitch"
/* org.openmixer.declaration/1: the declaration's plain description and the digest of its parameters. */
#define OMX_PITCH_DECL_SOURCE "omx pitch: Semitones -12 to 12, Cents -50 to 50, Mix 0 to 100"
#define OMX_PITCH_DECL_DIGEST "be51e9f044d94539dbfc82ff4669e9327b5e9199863a6e0b37c64ca700988836"

/* LV2 port indices (tools/gen.mjs lv2Ports): audio, then parameter i at FIRST_PARAM + i, then these. */
#define OMX_PITCH_LV2_PORT_IN_L 0u
#define OMX_PITCH_LV2_PORT_IN_R 1u
#define OMX_PITCH_LV2_PORT_OUT_L 2u
#define OMX_PITCH_LV2_PORT_OUT_R 3u
#define OMX_PITCH_LV2_PORT_ENABLED 7u
#define OMX_PITCH_LV2_PORT_LATENCY 8u
#define OMX_PITCH_LV2_PORT_FIRST_PARAM 4u
#define OMX_PITCH_LV2_PORT_COUNT 9u

#endif /* OMX_PITCH_PARAMS_H */
