// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#ifndef OMX_GATE_PARAMS_H
#define OMX_GATE_PARAMS_H
/*
 * GENERATED — DO NOT EDIT BY HAND.
 * Produced by tools/gen.mjs from plugins/omx-keyed-gate/omx-keyed-gate.decl.json.
 * Regenerate: `make -C plugins/omx-keyed-gate gen`, then commit the result. Order is append-only.
 * The include guard is omx-dsp's own <omxdsp/params/omx_gate_params.h>: included first, this
 * table is the one the kernel's instance header reads.
 */
#include "omx_plugin_param.h"

enum {
  OMX_GATE_PARAM_KEY_EXTERNAL = 0,
  OMX_GATE_PARAM_THRESHOLD = 1,
  OMX_GATE_PARAM_RATIO = 2,
  OMX_GATE_PARAM_RANGE = 3,
  OMX_GATE_PARAM_ATTACK = 4,
  OMX_GATE_PARAM_RELEASE = 5,
  OMX_GATE_PARAM_COUNT = 6
};

static const omx_plugin_param OMX_GATE_PARAMS[OMX_GATE_PARAM_COUNT] = {
  { "key_external", "Key", "", 0.0f, 1.0f, 1.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "threshold", "Threshold", "dB", -80.0f, 0.0f, -40.0f, 0u },
  { "ratio", "Ratio", "", 1.0f, 100.0f, 16.0f, 0u },
  { "range", "Range", "dB", -90.0f, 0.0f, -90.0f, 0u },
  { "attack", "Attack", "ms", 0.0f, 500.0f, 1.0f, 0u },
  { "release", "Release", "ms", 0.0f, 5000.0f, 100.0f, 0u },
};

/* One macro per declared bound: what a C face reads where a constant is needed. */
#define OMX_GATE_PARAM_KEY_EXTERNAL_MIN 0.0f
#define OMX_GATE_PARAM_KEY_EXTERNAL_MAX 1.0f
#define OMX_GATE_PARAM_KEY_EXTERNAL_DEFAULT 1.0f
#define OMX_GATE_PARAM_THRESHOLD_MIN -80.0f
#define OMX_GATE_PARAM_THRESHOLD_MAX 0.0f
#define OMX_GATE_PARAM_THRESHOLD_DEFAULT -40.0f
#define OMX_GATE_PARAM_RATIO_MIN 1.0f
#define OMX_GATE_PARAM_RATIO_MAX 100.0f
#define OMX_GATE_PARAM_RATIO_DEFAULT 16.0f
#define OMX_GATE_PARAM_RANGE_MIN -90.0f
#define OMX_GATE_PARAM_RANGE_MAX 0.0f
#define OMX_GATE_PARAM_RANGE_DEFAULT -90.0f
#define OMX_GATE_PARAM_ATTACK_MIN 0.0f
#define OMX_GATE_PARAM_ATTACK_MAX 500.0f
#define OMX_GATE_PARAM_ATTACK_DEFAULT 1.0f
#define OMX_GATE_PARAM_RELEASE_MIN 0.0f
#define OMX_GATE_PARAM_RELEASE_MAX 5000.0f
#define OMX_GATE_PARAM_RELEASE_DEFAULT 100.0f

/* The identity every face publishes. */
#define OMX_GATE_NAME "omx keyed-gate"
#define OMX_GATE_VENDOR "openmixer"
#define OMX_GATE_URL "https://github.com/FreeMixer/omx-plugins"
#define OMX_GATE_VERSION "0.1.0"
#define OMX_GATE_DESCRIPTION "The OpenMixer console's channel gate with an external key: the stereo signal is gated while the detector listens to the sidechain input the host routes to it, or to the signal itself with the key unconnected or set to Self. One gain drives both legs. An attack under 0.5 ms engages 4x detector oversampling, which adds 72 frames of latency, reported to the host. The DSP is omx-dsp's <omxdsp/fx/omx_gate_instance.h>, the console's own gate."
#define OMX_GATE_CLAP_ID "org.openmixer.keyed-gate"
#define OMX_GATE_CLAP_FEATURES "audio-effect", "gate", "stereo"
#define OMX_GATE_LV2_URI "urn:openmixer:keyed-gate"
/* org.openmixer.declaration/1: the declaration's plain description and the digest of its parameters. */
#define OMX_GATE_DECL_SOURCE "omx keyed-gate: Key 0 to 1, Threshold -80 to 0 dB, Ratio 1 to 100, Range -90 to 0 dB, Attack 0 to 500 ms, Release 0 to 5000 ms"
#define OMX_GATE_DECL_DIGEST "7cd2c792f3671a4d48aae0e44c9908fe2fd20805e537d53dbc9306f4bb59bb44"

/* LV2 port indices (tools/gen.mjs lv2Ports): audio, then parameter i at FIRST_PARAM + i, then these. */
#define OMX_GATE_LV2_PORT_IN_L 0u
#define OMX_GATE_LV2_PORT_IN_R 1u
#define OMX_GATE_LV2_PORT_KEY 2u
#define OMX_GATE_LV2_PORT_OUT_L 3u
#define OMX_GATE_LV2_PORT_OUT_R 4u
#define OMX_GATE_LV2_PORT_ENABLED 5u
#define OMX_GATE_LV2_PORT_LATENCY 12u
#define OMX_GATE_LV2_PORT_FIRST_PARAM 6u
#define OMX_GATE_LV2_PORT_COUNT 13u

#endif /* OMX_GATE_PARAMS_H */
