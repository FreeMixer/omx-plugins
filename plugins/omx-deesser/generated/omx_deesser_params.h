// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#ifndef OMX_DEESSER_PARAMS_H
#define OMX_DEESSER_PARAMS_H
/*
 * GENERATED — DO NOT EDIT BY HAND.
 * Produced by tools/gen.mjs from plugins/omx-deesser/omx-deesser.decl.json.
 * Regenerate: `make -C plugins/omx-deesser gen`, then commit the result. Order is append-only.
 * The include guard is omx-dsp's own <omxdsp/params/omx_deesser_params.h>: included first, this
 * table is the one the kernel's instance header reads.
 */
#include "omx_plugin_param.h"

enum {
  OMX_DEESSER_PARAM_DEESS_FREQ = 0,
  OMX_DEESSER_PARAM_DEESS_WIDTH = 1,
  OMX_DEESSER_PARAM_DEESS_THRESHOLD = 2,
  OMX_DEESSER_PARAM_DEESS_RATIO = 3,
  OMX_DEESSER_PARAM_DEESS_RANGE = 4,
  OMX_DEESSER_PARAM_DEESS_ATTACK = 5,
  OMX_DEESSER_PARAM_DEESS_RELEASE = 6,
  OMX_DEESSER_PARAM_DEESS_MODE = 7,
  OMX_DEESSER_PARAM_COUNT = 8
};

static const omx_plugin_param OMX_DEESSER_PARAMS[OMX_DEESSER_PARAM_COUNT] = {
  { "deessFreq", "Freq", "Hz", 2000.0f, 16000.0f, 7000.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "deessWidth", "Width", "oct", 0.25f, 4.0f, 1.0f, 0u },
  { "deessThreshold", "Threshold", "dB", -60.0f, 0.0f, -30.0f, 0u },
  { "deessRatio", "Ratio", "", 1.0f, 20.0f, 4.0f, 0u },
  { "deessRange", "Range", "dB", -24.0f, 0.0f, -12.0f, 0u },
  { "deessAttack", "Attack", "ms", 0.1f, 50.0f, 1.0f, 0u },
  { "deessRelease", "Release", "ms", 5.0f, 500.0f, 60.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "deessMode", "Mode", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
};

/* One macro per declared bound: what a C face reads where a constant is needed. */
#define OMX_DEESSER_PARAM_DEESS_FREQ_MIN 2000.0f
#define OMX_DEESSER_PARAM_DEESS_FREQ_MAX 16000.0f
#define OMX_DEESSER_PARAM_DEESS_FREQ_DEFAULT 7000.0f
#define OMX_DEESSER_PARAM_DEESS_WIDTH_MIN 0.25f
#define OMX_DEESSER_PARAM_DEESS_WIDTH_MAX 4.0f
#define OMX_DEESSER_PARAM_DEESS_WIDTH_DEFAULT 1.0f
#define OMX_DEESSER_PARAM_DEESS_THRESHOLD_MIN -60.0f
#define OMX_DEESSER_PARAM_DEESS_THRESHOLD_MAX 0.0f
#define OMX_DEESSER_PARAM_DEESS_THRESHOLD_DEFAULT -30.0f
#define OMX_DEESSER_PARAM_DEESS_RATIO_MIN 1.0f
#define OMX_DEESSER_PARAM_DEESS_RATIO_MAX 20.0f
#define OMX_DEESSER_PARAM_DEESS_RATIO_DEFAULT 4.0f
#define OMX_DEESSER_PARAM_DEESS_RANGE_MIN -24.0f
#define OMX_DEESSER_PARAM_DEESS_RANGE_MAX 0.0f
#define OMX_DEESSER_PARAM_DEESS_RANGE_DEFAULT -12.0f
#define OMX_DEESSER_PARAM_DEESS_ATTACK_MIN 0.1f
#define OMX_DEESSER_PARAM_DEESS_ATTACK_MAX 50.0f
#define OMX_DEESSER_PARAM_DEESS_ATTACK_DEFAULT 1.0f
#define OMX_DEESSER_PARAM_DEESS_RELEASE_MIN 5.0f
#define OMX_DEESSER_PARAM_DEESS_RELEASE_MAX 500.0f
#define OMX_DEESSER_PARAM_DEESS_RELEASE_DEFAULT 60.0f
#define OMX_DEESSER_PARAM_DEESS_MODE_MIN 0.0f
#define OMX_DEESSER_PARAM_DEESS_MODE_MAX 1.0f
#define OMX_DEESSER_PARAM_DEESS_MODE_DEFAULT 0.0f

/* The identity every face publishes. */
#define OMX_DEESSER_NAME "omx deesser"
#define OMX_DEESSER_VENDOR "openmixer"
#define OMX_DEESSER_URL "https://github.com/FreeMixer/omx-plugins"
#define OMX_DEESSER_VERSION "0.2.0"
#define OMX_DEESSER_DESCRIPTION "The console's de-esser: a detector on a band around the sibilance drives the compressor's gain computer, and the reduction lands on that band alone or on the whole signal."
#define OMX_DEESSER_CLAP_ID "org.openmixer.deesser"
#define OMX_DEESSER_CLAP_FEATURES "audio-effect", "de-esser", "stereo"
#define OMX_DEESSER_LV2_URI "urn:openmixer:deesser"
/* org.openmixer.declaration/1: the declaration's plain description and the digest of its parameters. */
#define OMX_DEESSER_DECL_SOURCE "omx deesser: Freq 2000 to 16000 Hz, Width 0.25 to 4 oct, Threshold -60 to 0 dB, Ratio 1 to 20, Range -24 to 0 dB, Attack 0.1 to 50 ms, Release 5 to 500 ms, Mode 0 to 1"
#define OMX_DEESSER_DECL_DIGEST "5229f9c0817d98fea312f088c1f8519628c36ef5f1d2b453b22f3ba0571f097f"

/* LV2 port indices (tools/gen.mjs lv2Ports): audio, then parameter i at FIRST_PARAM + i, then these. */
#define OMX_DEESSER_LV2_PORT_IN_L 0u
#define OMX_DEESSER_LV2_PORT_IN_R 1u
#define OMX_DEESSER_LV2_PORT_OUT_L 2u
#define OMX_DEESSER_LV2_PORT_OUT_R 3u
#define OMX_DEESSER_LV2_PORT_ENABLED 12u
#define OMX_DEESSER_LV2_PORT_LATENCY 13u
#define OMX_DEESSER_LV2_PORT_FIRST_PARAM 4u
#define OMX_DEESSER_LV2_PORT_COUNT 14u

#endif /* OMX_DEESSER_PARAMS_H */
