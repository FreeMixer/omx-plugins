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
  OMX_DEESSER_PARAM_FREQ_HZ = 0,
  OMX_DEESSER_PARAM_WIDTH_OCT = 1,
  OMX_DEESSER_PARAM_THRESHOLD_DB = 2,
  OMX_DEESSER_PARAM_RATIO = 3,
  OMX_DEESSER_PARAM_RANGE_DB = 4,
  OMX_DEESSER_PARAM_ATTACK_MS = 5,
  OMX_DEESSER_PARAM_RELEASE_MS = 6,
  OMX_DEESSER_PARAM_COUNT = 7
};

static const omx_plugin_param OMX_DEESSER_PARAMS[OMX_DEESSER_PARAM_COUNT] = {
  { "freqHz", "Freq", "Hz", 2000.0f, 16000.0f, 7000.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "widthOct", "Width", "oct", 0.25f, 4.0f, 1.0f, 0u },
  { "thresholdDb", "Threshold", "dB", -60.0f, 0.0f, -30.0f, 0u },
  { "ratio", "Ratio", "", 1.0f, 20.0f, 4.0f, 0u },
  { "rangeDb", "Range", "dB", -24.0f, 0.0f, -12.0f, 0u },
  { "attackMs", "Attack", "ms", 0.1f, 50.0f, 1.0f, 0u },
  { "releaseMs", "Release", "ms", 5.0f, 500.0f, 60.0f, OMX_PLUGIN_PARAM_INTEGER },
};

/* One macro per declared bound: what a C face reads where a constant is needed. */
#define OMX_DEESSER_PARAM_FREQ_HZ_MIN 2000.0f
#define OMX_DEESSER_PARAM_FREQ_HZ_MAX 16000.0f
#define OMX_DEESSER_PARAM_FREQ_HZ_DEFAULT 7000.0f
#define OMX_DEESSER_PARAM_WIDTH_OCT_MIN 0.25f
#define OMX_DEESSER_PARAM_WIDTH_OCT_MAX 4.0f
#define OMX_DEESSER_PARAM_WIDTH_OCT_DEFAULT 1.0f
#define OMX_DEESSER_PARAM_THRESHOLD_DB_MIN -60.0f
#define OMX_DEESSER_PARAM_THRESHOLD_DB_MAX 0.0f
#define OMX_DEESSER_PARAM_THRESHOLD_DB_DEFAULT -30.0f
#define OMX_DEESSER_PARAM_RATIO_MIN 1.0f
#define OMX_DEESSER_PARAM_RATIO_MAX 20.0f
#define OMX_DEESSER_PARAM_RATIO_DEFAULT 4.0f
#define OMX_DEESSER_PARAM_RANGE_DB_MIN -24.0f
#define OMX_DEESSER_PARAM_RANGE_DB_MAX 0.0f
#define OMX_DEESSER_PARAM_RANGE_DB_DEFAULT -12.0f
#define OMX_DEESSER_PARAM_ATTACK_MS_MIN 0.1f
#define OMX_DEESSER_PARAM_ATTACK_MS_MAX 50.0f
#define OMX_DEESSER_PARAM_ATTACK_MS_DEFAULT 1.0f
#define OMX_DEESSER_PARAM_RELEASE_MS_MIN 5.0f
#define OMX_DEESSER_PARAM_RELEASE_MS_MAX 500.0f
#define OMX_DEESSER_PARAM_RELEASE_MS_DEFAULT 60.0f

/* The identity every face publishes. */
#define OMX_DEESSER_NAME "omx deesser"
#define OMX_DEESSER_VENDOR "openmixer"
#define OMX_DEESSER_URL "https://github.com/FreeMixer/omx-plugins"
#define OMX_DEESSER_VERSION "0.2.0"
#define OMX_DEESSER_DESCRIPTION "the native DE-ESSER stage: a band-limited detector driving the COMPRESSOR's own gain computer."
#define OMX_DEESSER_CLAP_ID "org.openmixer.deesser"
#define OMX_DEESSER_CLAP_FEATURES "audio-effect", "stereo"
#define OMX_DEESSER_LV2_URI "urn:openmixer:deesser"
/* org.openmixer.declaration/1: the declaration's plain description and the digest of its parameters. */
#define OMX_DEESSER_DECL_SOURCE "omx deesser: Freq 2000 to 16000 Hz, Width 0.25 to 4 oct, Threshold -60 to 0 dB, Ratio 1 to 20, Range -24 to 0 dB, Attack 0.1 to 50 ms, Release 5 to 500 ms"
#define OMX_DEESSER_DECL_DIGEST "dd94080599a6850a161e387a4a6cfb9d9b93762e07318da2afca0d9e6f6db409"

/* LV2 port indices (tools/gen.mjs lv2Ports): audio, then parameter i at FIRST_PARAM + i, then these. */
#define OMX_DEESSER_LV2_PORT_IN_L 0u
#define OMX_DEESSER_LV2_PORT_IN_R 1u
#define OMX_DEESSER_LV2_PORT_OUT_L 2u
#define OMX_DEESSER_LV2_PORT_OUT_R 3u
#define OMX_DEESSER_LV2_PORT_ENABLED 11u
#define OMX_DEESSER_LV2_PORT_LATENCY 12u
#define OMX_DEESSER_LV2_PORT_FIRST_PARAM 4u
#define OMX_DEESSER_LV2_PORT_COUNT 13u

#endif /* OMX_DEESSER_PARAMS_H */
