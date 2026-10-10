// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#ifndef OMX_COMP_PARAMS_H
#define OMX_COMP_PARAMS_H
/*
 * GENERATED — DO NOT EDIT BY HAND.
 * Produced by tools/gen.mjs from plugins/omx-comp/omx-comp.decl.json.
 * Regenerate: `make -C plugins/omx-comp gen`, then commit the result. Order is append-only.
 * The include guard is omx-dsp's own <omxdsp/params/omx_comp_params.h>: included first, this
 * table is the one the kernel's instance header reads.
 */
#include "omx_plugin_param.h"

enum {
  OMX_COMP_PARAM_THRESHOLD_DB = 0,
  OMX_COMP_PARAM_RATIO = 1,
  OMX_COMP_PARAM_KNEE_DB = 2,
  OMX_COMP_PARAM_ATTACK_MS = 3,
  OMX_COMP_PARAM_RELEASE_MS = 4,
  OMX_COMP_PARAM_MAKEUP_DB = 5,
  OMX_COMP_PARAM_MIX_PCT = 6,
  OMX_COMP_PARAM_KIND = 7,
  OMX_COMP_PARAM_DETECTOR_OVERSAMPLING = 8,
  OMX_COMP_PARAM_COUNT = 9
};

static const omx_plugin_param OMX_COMP_PARAMS[OMX_COMP_PARAM_COUNT] = {
  { "thresholdDb", "Threshold", "dB", -60.0f, 0.0f, -18.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "ratio", "Ratio", "", 1.0f, 20.0f, 4.0f, 0u },
  { "kneeDb", "Knee", "dB", 0.0f, 24.0f, 6.0f, 0u },
  { "attackMs", "Attack", "ms", 0.1f, 100.0f, 5.0f, 0u },
  { "releaseMs", "Release", "ms", 5.0f, 3000.0f, 200.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "makeupDb", "Makeup", "dB", 0.0f, 24.0f, 0.0f, 0u },
  { "mixPct", "Mix", "%", 0.0f, 100.0f, 100.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "kind", "Kind", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "detectorOversampling", "Detector Oversampling", "", 0.0f, 2.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
};

/* One macro per declared bound: what a C face reads where a constant is needed. */
#define OMX_COMP_PARAM_THRESHOLD_DB_MIN -60.0f
#define OMX_COMP_PARAM_THRESHOLD_DB_MAX 0.0f
#define OMX_COMP_PARAM_THRESHOLD_DB_DEFAULT -18.0f
#define OMX_COMP_PARAM_RATIO_MIN 1.0f
#define OMX_COMP_PARAM_RATIO_MAX 20.0f
#define OMX_COMP_PARAM_RATIO_DEFAULT 4.0f
#define OMX_COMP_PARAM_KNEE_DB_MIN 0.0f
#define OMX_COMP_PARAM_KNEE_DB_MAX 24.0f
#define OMX_COMP_PARAM_KNEE_DB_DEFAULT 6.0f
#define OMX_COMP_PARAM_ATTACK_MS_MIN 0.1f
#define OMX_COMP_PARAM_ATTACK_MS_MAX 100.0f
#define OMX_COMP_PARAM_ATTACK_MS_DEFAULT 5.0f
#define OMX_COMP_PARAM_RELEASE_MS_MIN 5.0f
#define OMX_COMP_PARAM_RELEASE_MS_MAX 3000.0f
#define OMX_COMP_PARAM_RELEASE_MS_DEFAULT 200.0f
#define OMX_COMP_PARAM_MAKEUP_DB_MIN 0.0f
#define OMX_COMP_PARAM_MAKEUP_DB_MAX 24.0f
#define OMX_COMP_PARAM_MAKEUP_DB_DEFAULT 0.0f
#define OMX_COMP_PARAM_MIX_PCT_MIN 0.0f
#define OMX_COMP_PARAM_MIX_PCT_MAX 100.0f
#define OMX_COMP_PARAM_MIX_PCT_DEFAULT 100.0f
#define OMX_COMP_PARAM_KIND_MIN 0.0f
#define OMX_COMP_PARAM_KIND_MAX 1.0f
#define OMX_COMP_PARAM_KIND_DEFAULT 0.0f
#define OMX_COMP_PARAM_DETECTOR_OVERSAMPLING_MIN 0.0f
#define OMX_COMP_PARAM_DETECTOR_OVERSAMPLING_MAX 2.0f
#define OMX_COMP_PARAM_DETECTOR_OVERSAMPLING_DEFAULT 0.0f

/* The identity every face publishes. */
#define OMX_COMP_NAME "omx comp"
#define OMX_COMP_VENDOR "openmixer"
#define OMX_COMP_URL "https://github.com/FreeMixer/omx-plugins"
#define OMX_COMP_VERSION "0.3.0"
#define OMX_COMP_DESCRIPTION "The console's compressor: threshold, ratio and knee set how hard it holds the level down, attack and release how fast, with makeup gain and a dry/wet mix. Kind picks the RMS detector (Compressor) or the peak detector (Limiter), and detector oversampling can run the control path at 4x. The DSP is omx-dsp's <omxdsp/fx/omx_comp_instance.h>, the console's own compressor."
#define OMX_COMP_CLAP_ID "org.openmixer.comp"
#define OMX_COMP_CLAP_FEATURES "audio-effect", "compressor", "stereo"
#define OMX_COMP_LV2_URI "urn:openmixer:comp"
/* org.openmixer.declaration/1: the declaration's plain description and the digest of its parameters. */
#define OMX_COMP_DECL_SOURCE "omx comp: Threshold -60 to 0 dB, Ratio 1 to 20, Knee 0 to 24 dB, Attack 0.1 to 100 ms, Release 5 to 3000 ms, Makeup 0 to 24 dB, Mix 0 to 100 %, Kind 0 to 1, Detector Oversampling 0 to 2"
#define OMX_COMP_DECL_DIGEST "3a67a30c58264f094b79beb7cdb105698661368e85fe2f8e24506fe0f898d0ab"

/* LV2 port indices (tools/gen.mjs lv2Ports): audio, then parameter i at FIRST_PARAM + i, then these. */
#define OMX_COMP_LV2_PORT_IN_L 0u
#define OMX_COMP_LV2_PORT_IN_R 1u
#define OMX_COMP_LV2_PORT_OUT_L 2u
#define OMX_COMP_LV2_PORT_OUT_R 3u
#define OMX_COMP_LV2_PORT_ENABLED 13u
#define OMX_COMP_LV2_PORT_LATENCY 14u
#define OMX_COMP_LV2_PORT_FIRST_PARAM 4u
#define OMX_COMP_LV2_PORT_COUNT 15u

#endif /* OMX_COMP_PARAMS_H */
