// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#ifndef OMX_REVERB_PARAMS_H
#define OMX_REVERB_PARAMS_H
/*
 * GENERATED — DO NOT EDIT BY HAND.
 * Produced by tools/gen.mjs from plugins/omx-reverb/omx-reverb.decl.json.
 * Regenerate: `make -C plugins/omx-reverb gen`, then commit the result. Order is append-only.
 * The include guard is omx-dsp's own <omxdsp/params/omx_reverb_params.h>: included first, this
 * table is the one the kernel's instance header reads.
 */
#include "omx_plugin_param.h"

enum {
  OMX_REVERB_PARAM_PLATE_MOD_DEPTH = 0,
  OMX_REVERB_PARAM_MIX = 1,
  OMX_REVERB_PARAM_SIZE = 2,
  OMX_REVERB_PARAM_DAMPING = 3,
  OMX_REVERB_PARAM_WIDTH = 4,
  OMX_REVERB_PARAM_PREDELAY = 5,
  OMX_REVERB_PARAM_LOWCUT = 6,
  OMX_REVERB_PARAM_HIGHCUT = 7,
  OMX_REVERB_PARAM_REVERSE = 8,
  OMX_REVERB_PARAM_HOLD = 9,
  OMX_REVERB_PARAM_RELEASE = 10,
  OMX_REVERB_PARAM_GATE_THRESHOLD = 11,
  OMX_REVERB_PARAM_ALGORITHM = 12,
  OMX_REVERB_PARAM_COUNT = 13
};

static const omx_plugin_param OMX_REVERB_PARAMS[OMX_REVERB_PARAM_COUNT] = {
  { "plateModDepth", "Plate Mod Depth", "%", 0.0f, 400.0f, 100.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "mix", "Mix", "", 0.0f, 1.0f, 0.3f, 0u },
  { "size", "Size", "", 0.0f, 1.0f, 0.7f, 0u },
  { "damping", "Damping", "", 0.0f, 1.0f, 0.5f, 0u },
  { "width", "Width", "", 0.0f, 1.0f, 1.0f, 0u },
  { "predelay", "Predelay", "ms", 0.0f, 100.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "lowcut", "Lowcut", "Hz", 0.0f, 20000.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "highcut", "Highcut", "Hz", 0.0f, 20000.0f, 20000.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "reverse", "Reverse", "ms", 50.0f, 500.0f, 300.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "hold", "Hold", "ms", 10.0f, 2000.0f, 120.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "release", "Release", "ms", 1.0f, 500.0f, 20.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "gateThreshold", "Gate Threshold", "dBFS", -80.0f, 0.0f, -40.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "algorithm", "Algorithm", "", 0.0f, 4.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
};

/* One macro per declared bound: what a C face reads where a constant is needed. */
#define OMX_REVERB_PARAM_PLATE_MOD_DEPTH_MIN 0.0f
#define OMX_REVERB_PARAM_PLATE_MOD_DEPTH_MAX 400.0f
#define OMX_REVERB_PARAM_PLATE_MOD_DEPTH_DEFAULT 100.0f
#define OMX_REVERB_PARAM_MIX_MIN 0.0f
#define OMX_REVERB_PARAM_MIX_MAX 1.0f
#define OMX_REVERB_PARAM_MIX_DEFAULT 0.3f
#define OMX_REVERB_PARAM_SIZE_MIN 0.0f
#define OMX_REVERB_PARAM_SIZE_MAX 1.0f
#define OMX_REVERB_PARAM_SIZE_DEFAULT 0.7f
#define OMX_REVERB_PARAM_DAMPING_MIN 0.0f
#define OMX_REVERB_PARAM_DAMPING_MAX 1.0f
#define OMX_REVERB_PARAM_DAMPING_DEFAULT 0.5f
#define OMX_REVERB_PARAM_WIDTH_MIN 0.0f
#define OMX_REVERB_PARAM_WIDTH_MAX 1.0f
#define OMX_REVERB_PARAM_WIDTH_DEFAULT 1.0f
#define OMX_REVERB_PARAM_PREDELAY_MIN 0.0f
#define OMX_REVERB_PARAM_PREDELAY_MAX 100.0f
#define OMX_REVERB_PARAM_PREDELAY_DEFAULT 0.0f
#define OMX_REVERB_PARAM_LOWCUT_MIN 0.0f
#define OMX_REVERB_PARAM_LOWCUT_MAX 20000.0f
#define OMX_REVERB_PARAM_LOWCUT_DEFAULT 0.0f
#define OMX_REVERB_PARAM_HIGHCUT_MIN 0.0f
#define OMX_REVERB_PARAM_HIGHCUT_MAX 20000.0f
#define OMX_REVERB_PARAM_HIGHCUT_DEFAULT 20000.0f
#define OMX_REVERB_PARAM_REVERSE_MIN 50.0f
#define OMX_REVERB_PARAM_REVERSE_MAX 500.0f
#define OMX_REVERB_PARAM_REVERSE_DEFAULT 300.0f
#define OMX_REVERB_PARAM_HOLD_MIN 10.0f
#define OMX_REVERB_PARAM_HOLD_MAX 2000.0f
#define OMX_REVERB_PARAM_HOLD_DEFAULT 120.0f
#define OMX_REVERB_PARAM_RELEASE_MIN 1.0f
#define OMX_REVERB_PARAM_RELEASE_MAX 500.0f
#define OMX_REVERB_PARAM_RELEASE_DEFAULT 20.0f
#define OMX_REVERB_PARAM_GATE_THRESHOLD_MIN -80.0f
#define OMX_REVERB_PARAM_GATE_THRESHOLD_MAX 0.0f
#define OMX_REVERB_PARAM_GATE_THRESHOLD_DEFAULT -40.0f
#define OMX_REVERB_PARAM_ALGORITHM_MIN 0.0f
#define OMX_REVERB_PARAM_ALGORITHM_MAX 4.0f
#define OMX_REVERB_PARAM_ALGORITHM_DEFAULT 0.0f

/* The identity every face publishes. */
#define OMX_REVERB_NAME "omx reverb"
#define OMX_REVERB_VENDOR "openmixer"
#define OMX_REVERB_URL "https://github.com/FreeMixer/omx-plugins"
#define OMX_REVERB_VERSION "0.2.0"
#define OMX_REVERB_DESCRIPTION "native algorithmic reverb, ONE Schroeder-Moorer comb+allpass kernel (Freeverb lineage) serving TWO configurations (OMX_REVERB_ROOM, OMX_REVERB_HALL — same omx_fv_comb / omx_fv_allpass functions, each with its own delay-line lengths + feedback/damping curve) plus a second, structurally different kernel, OMX_REVERB_PLATE (Dattorro figure-8 plate tank: input diffusers -> a damped delay-network tank)."
#define OMX_REVERB_CLAP_ID "org.openmixer.reverb"
#define OMX_REVERB_CLAP_FEATURES "audio-effect", "reverb", "stereo"
#define OMX_REVERB_LV2_URI "urn:openmixer:reverb"
/* org.openmixer.declaration/1: the declaration's plain description and the digest of its parameters. */
#define OMX_REVERB_DECL_SOURCE "omx reverb: Plate Mod Depth 0 to 400 %, Mix 0 to 1, Size 0 to 1, Damping 0 to 1, Width 0 to 1, Predelay 0 to 100 ms, Lowcut 0 to 20000 Hz, Highcut 0 to 20000 Hz, Reverse 50 to 500 ms, Hold 10 to 2000 ms, Release 1 to 500 ms, Gate Threshold -80 to 0 dBFS, Algorithm 0 to 4"
#define OMX_REVERB_DECL_DIGEST "a42c6e20426afd868f08938af4dfb54b487bdf4d8a5d9b13f75a967319df7009"

/* LV2 port indices (tools/gen.mjs lv2Ports): audio, then parameter i at FIRST_PARAM + i, then these. */
#define OMX_REVERB_LV2_PORT_IN_L 0u
#define OMX_REVERB_LV2_PORT_IN_R 1u
#define OMX_REVERB_LV2_PORT_OUT_L 2u
#define OMX_REVERB_LV2_PORT_OUT_R 3u
#define OMX_REVERB_LV2_PORT_ENABLED 17u
#define OMX_REVERB_LV2_PORT_LATENCY 18u
#define OMX_REVERB_LV2_PORT_FIRST_PARAM 4u
#define OMX_REVERB_LV2_PORT_COUNT 19u

#endif /* OMX_REVERB_PARAMS_H */
