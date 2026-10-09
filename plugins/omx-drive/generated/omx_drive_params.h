// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#ifndef OMX_DRIVE_PARAMS_H
#define OMX_DRIVE_PARAMS_H
/*
 * GENERATED — DO NOT EDIT BY HAND.
 * Produced by tools/gen.mjs from plugins/omx-drive/omx-drive.decl.json.
 * Regenerate: `make -C plugins/omx-drive gen`, then commit the result. Order is append-only.
 * The include guard is omx-dsp's own <omxdsp/params/omx_drive_params.h>: included first, this
 * table is the one the kernel's instance header reads.
 */
#include "omx_plugin_param.h"

enum {
  OMX_DRIVE_PARAM_AMOUNT = 0,
  OMX_DRIVE_PARAM_CHARACTER = 1,
  OMX_DRIVE_PARAM_BAND_FREQ = 2,
  OMX_DRIVE_PARAM_MIX = 3,
  OMX_DRIVE_PARAM_TRIM = 4,
  OMX_DRIVE_PARAM_CURVE = 5,
  OMX_DRIVE_PARAM_BAND = 6,
  OMX_DRIVE_PARAM_AUTO_GAIN = 7,
  OMX_DRIVE_PARAM_STEREO_LINK = 8,
  OMX_DRIVE_PARAM_HF_ROLLOFF = 9,
  OMX_DRIVE_PARAM_COUNT = 10
};

static const omx_plugin_param OMX_DRIVE_PARAMS[OMX_DRIVE_PARAM_COUNT] = {
  { "amount", "Drive", "dB", 0.0f, 36.0f, 0.0f, 0u },
  { "character", "Character", "", -1.0f, 1.0f, 0.0f, 0u },
  { "bandFreq", "Band Frequency", "Hz", 20.0f, 20000.0f, 2000.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "mix", "Mix", "%", 0.0f, 100.0f, 100.0f, 0u },
  { "trim", "Trim", "dB", -24.0f, 12.0f, 0.0f, 0u },
  { "curve", "Curve", "", 0.0f, 3.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "band", "Band", "", 0.0f, 3.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "autoGain", "Auto Gain", "", 0.0f, 1.0f, 1.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "stereoLink", "Stereo Link", "", 0.0f, 1.0f, 1.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "hfRolloff", "HF Roll-off", "", 0.0f, 16000.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
};

/* One macro per declared bound: what a C face reads where a constant is needed. */
#define OMX_DRIVE_PARAM_AMOUNT_MIN 0.0f
#define OMX_DRIVE_PARAM_AMOUNT_MAX 36.0f
#define OMX_DRIVE_PARAM_AMOUNT_DEFAULT 0.0f
#define OMX_DRIVE_PARAM_CHARACTER_MIN -1.0f
#define OMX_DRIVE_PARAM_CHARACTER_MAX 1.0f
#define OMX_DRIVE_PARAM_CHARACTER_DEFAULT 0.0f
#define OMX_DRIVE_PARAM_BAND_FREQ_MIN 20.0f
#define OMX_DRIVE_PARAM_BAND_FREQ_MAX 20000.0f
#define OMX_DRIVE_PARAM_BAND_FREQ_DEFAULT 2000.0f
#define OMX_DRIVE_PARAM_MIX_MIN 0.0f
#define OMX_DRIVE_PARAM_MIX_MAX 100.0f
#define OMX_DRIVE_PARAM_MIX_DEFAULT 100.0f
#define OMX_DRIVE_PARAM_TRIM_MIN -24.0f
#define OMX_DRIVE_PARAM_TRIM_MAX 12.0f
#define OMX_DRIVE_PARAM_TRIM_DEFAULT 0.0f
#define OMX_DRIVE_PARAM_CURVE_MIN 0.0f
#define OMX_DRIVE_PARAM_CURVE_MAX 3.0f
#define OMX_DRIVE_PARAM_CURVE_DEFAULT 0.0f
#define OMX_DRIVE_PARAM_BAND_MIN 0.0f
#define OMX_DRIVE_PARAM_BAND_MAX 3.0f
#define OMX_DRIVE_PARAM_BAND_DEFAULT 0.0f
#define OMX_DRIVE_PARAM_AUTO_GAIN_MIN 0.0f
#define OMX_DRIVE_PARAM_AUTO_GAIN_MAX 1.0f
#define OMX_DRIVE_PARAM_AUTO_GAIN_DEFAULT 1.0f
#define OMX_DRIVE_PARAM_STEREO_LINK_MIN 0.0f
#define OMX_DRIVE_PARAM_STEREO_LINK_MAX 1.0f
#define OMX_DRIVE_PARAM_STEREO_LINK_DEFAULT 1.0f
#define OMX_DRIVE_PARAM_HF_ROLLOFF_MIN 0.0f
#define OMX_DRIVE_PARAM_HF_ROLLOFF_MAX 16000.0f
#define OMX_DRIVE_PARAM_HF_ROLLOFF_DEFAULT 0.0f

/* The identity every face publishes. */
#define OMX_DRIVE_NAME "omx drive"
#define OMX_DRIVE_VENDOR "openmixer"
#define OMX_DRIVE_URL "https://github.com/FreeMixer/omx-plugins"
#define OMX_DRIVE_VERSION "0.2.0"
#define OMX_DRIVE_DESCRIPTION "The console's drive: one waveshaper (soft, tape, tube or exciter) run inside the console's oversampler, on the full band, the lows, the highs or a tilt, with a wet/dry mix and a trim."
#define OMX_DRIVE_CLAP_ID "org.openmixer.drive"
#define OMX_DRIVE_CLAP_FEATURES "audio-effect", "distortion", "stereo"
#define OMX_DRIVE_LV2_URI "urn:openmixer:drive"
/* org.openmixer.declaration/1: the declaration's plain description and the digest of its parameters. */
#define OMX_DRIVE_DECL_SOURCE "omx drive: Drive 0 to 36 dB, Character -1 to 1, Band Frequency 20 to 20000 Hz, Mix 0 to 100 %, Trim -24 to 12 dB, Curve 0 to 3, Band 0 to 3, Auto Gain 0 to 1, Stereo Link 0 to 1, HF Roll-off 0 to 16000"
#define OMX_DRIVE_DECL_DIGEST "b53c3fb88d798020fa4916d7c259c609a36baa22a20f8e28105e53e2506b73d2"

/* LV2 port indices (tools/gen.mjs lv2Ports): audio, then parameter i at FIRST_PARAM + i, then these. */
#define OMX_DRIVE_LV2_PORT_IN_L 0u
#define OMX_DRIVE_LV2_PORT_IN_R 1u
#define OMX_DRIVE_LV2_PORT_OUT_L 2u
#define OMX_DRIVE_LV2_PORT_OUT_R 3u
#define OMX_DRIVE_LV2_PORT_ENABLED 14u
#define OMX_DRIVE_LV2_PORT_LATENCY 15u
#define OMX_DRIVE_LV2_PORT_FIRST_PARAM 4u
#define OMX_DRIVE_LV2_PORT_COUNT 16u

#endif /* OMX_DRIVE_PARAMS_H */
