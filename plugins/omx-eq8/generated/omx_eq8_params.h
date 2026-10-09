// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#ifndef OMX_EQ8_PARAMS_H
#define OMX_EQ8_PARAMS_H
/*
 * GENERATED — DO NOT EDIT BY HAND.
 * Produced by tools/gen.mjs from plugins/omx-eq8/omx-eq8.decl.json.
 * Regenerate: `make -C plugins/omx-eq8 gen`, then commit the result. Order is append-only.
 * The include guard is omx-dsp's own <omxdsp/params/omx_eq8_params.h>: included first, this
 * table is the one the kernel's instance header reads.
 */
#include "omx_plugin_param.h"

/* The band count of this variant of plugins/omx-eq: the instance face's compile-time count. */
#define OMX_EQ_INSTANCE_BANDS OMX_EQ_BAND_COUNTS_EQ8_MAX

enum {
  OMX_EQ8_PARAM_HPF_ON = 0,
  OMX_EQ8_PARAM_HPF_FREQ = 1,
  OMX_EQ8_PARAM_HPF_SLOPE = 2,
  OMX_EQ8_PARAM_LPF_ON = 3,
  OMX_EQ8_PARAM_LPF_FREQ = 4,
  OMX_EQ8_PARAM_LPF_SLOPE = 5,
  OMX_EQ8_PARAM_B1_TYPE = 6,
  OMX_EQ8_PARAM_B1_FREQ = 7,
  OMX_EQ8_PARAM_B1_GAIN = 8,
  OMX_EQ8_PARAM_B1_Q = 9,
  OMX_EQ8_PARAM_B1_ON = 10,
  OMX_EQ8_PARAM_B2_TYPE = 11,
  OMX_EQ8_PARAM_B2_FREQ = 12,
  OMX_EQ8_PARAM_B2_GAIN = 13,
  OMX_EQ8_PARAM_B2_Q = 14,
  OMX_EQ8_PARAM_B2_ON = 15,
  OMX_EQ8_PARAM_B3_TYPE = 16,
  OMX_EQ8_PARAM_B3_FREQ = 17,
  OMX_EQ8_PARAM_B3_GAIN = 18,
  OMX_EQ8_PARAM_B3_Q = 19,
  OMX_EQ8_PARAM_B3_ON = 20,
  OMX_EQ8_PARAM_B4_TYPE = 21,
  OMX_EQ8_PARAM_B4_FREQ = 22,
  OMX_EQ8_PARAM_B4_GAIN = 23,
  OMX_EQ8_PARAM_B4_Q = 24,
  OMX_EQ8_PARAM_B4_ON = 25,
  OMX_EQ8_PARAM_B5_TYPE = 26,
  OMX_EQ8_PARAM_B5_FREQ = 27,
  OMX_EQ8_PARAM_B5_GAIN = 28,
  OMX_EQ8_PARAM_B5_Q = 29,
  OMX_EQ8_PARAM_B5_ON = 30,
  OMX_EQ8_PARAM_B6_TYPE = 31,
  OMX_EQ8_PARAM_B6_FREQ = 32,
  OMX_EQ8_PARAM_B6_GAIN = 33,
  OMX_EQ8_PARAM_B6_Q = 34,
  OMX_EQ8_PARAM_B6_ON = 35,
  OMX_EQ8_PARAM_B7_TYPE = 36,
  OMX_EQ8_PARAM_B7_FREQ = 37,
  OMX_EQ8_PARAM_B7_GAIN = 38,
  OMX_EQ8_PARAM_B7_Q = 39,
  OMX_EQ8_PARAM_B7_ON = 40,
  OMX_EQ8_PARAM_B8_TYPE = 41,
  OMX_EQ8_PARAM_B8_FREQ = 42,
  OMX_EQ8_PARAM_B8_GAIN = 43,
  OMX_EQ8_PARAM_B8_Q = 44,
  OMX_EQ8_PARAM_B8_ON = 45,
  OMX_EQ8_PARAM_COUNT = 46
};

static const omx_plugin_param OMX_EQ8_PARAMS[OMX_EQ8_PARAM_COUNT] = {
  { "hpf_on", "HPF On", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "hpf_freq", "HPF Frequency", "Hz", 20.0f, 1000.0f, 80.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "hpf_slope", "HPF Slope", "dB/oct", 12.0f, 24.0f, 12.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "lpf_on", "LPF On", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "lpf_freq", "LPF Frequency", "Hz", 1000.0f, 20000.0f, 18000.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "lpf_slope", "LPF Slope", "dB/oct", 12.0f, 24.0f, 12.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b1_type", "Band 1 Type", "", 0.0f, 5.0f, 1.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b1_freq", "Band 1 Frequency", "Hz", 20.0f, 20000.0f, 31.5f, OMX_PLUGIN_PARAM_INTEGER },
  { "b1_gain", "Band 1 Gain", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "b1_q", "Band 1 Q", "", 0.3f, 116.0f, 1.0f, 0u },
  { "b1_on", "Band 1 On", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "b2_type", "Band 2 Type", "", 0.0f, 5.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b2_freq", "Band 2 Frequency", "Hz", 20.0f, 20000.0f, 80.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b2_gain", "Band 2 Gain", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "b2_q", "Band 2 Q", "", 0.3f, 116.0f, 1.0f, 0u },
  { "b2_on", "Band 2 On", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "b3_type", "Band 3 Type", "", 0.0f, 5.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b3_freq", "Band 3 Frequency", "Hz", 20.0f, 20000.0f, 160.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b3_gain", "Band 3 Gain", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "b3_q", "Band 3 Q", "", 0.3f, 116.0f, 1.0f, 0u },
  { "b3_on", "Band 3 On", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "b4_type", "Band 4 Type", "", 0.0f, 5.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b4_freq", "Band 4 Frequency", "Hz", 20.0f, 20000.0f, 400.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b4_gain", "Band 4 Gain", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "b4_q", "Band 4 Q", "", 0.3f, 116.0f, 1.0f, 0u },
  { "b4_on", "Band 4 On", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "b5_type", "Band 5 Type", "", 0.0f, 5.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b5_freq", "Band 5 Frequency", "Hz", 20.0f, 20000.0f, 1000.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b5_gain", "Band 5 Gain", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "b5_q", "Band 5 Q", "", 0.3f, 116.0f, 1.0f, 0u },
  { "b5_on", "Band 5 On", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "b6_type", "Band 6 Type", "", 0.0f, 5.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b6_freq", "Band 6 Frequency", "Hz", 20.0f, 20000.0f, 2500.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b6_gain", "Band 6 Gain", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "b6_q", "Band 6 Q", "", 0.3f, 116.0f, 1.0f, 0u },
  { "b6_on", "Band 6 On", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "b7_type", "Band 7 Type", "", 0.0f, 5.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b7_freq", "Band 7 Frequency", "Hz", 20.0f, 20000.0f, 5000.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b7_gain", "Band 7 Gain", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "b7_q", "Band 7 Q", "", 0.3f, 116.0f, 1.0f, 0u },
  { "b7_on", "Band 7 On", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "b8_type", "Band 8 Type", "", 0.0f, 5.0f, 2.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b8_freq", "Band 8 Frequency", "Hz", 20.0f, 20000.0f, 12500.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b8_gain", "Band 8 Gain", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "b8_q", "Band 8 Q", "", 0.3f, 116.0f, 1.0f, 0u },
  { "b8_on", "Band 8 On", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
};

/* One macro per declared bound: what a C face reads where a constant is needed. */
#define OMX_EQ8_PARAM_HPF_ON_MIN 0.0f
#define OMX_EQ8_PARAM_HPF_ON_MAX 1.0f
#define OMX_EQ8_PARAM_HPF_ON_DEFAULT 0.0f
#define OMX_EQ8_PARAM_HPF_FREQ_MIN 20.0f
#define OMX_EQ8_PARAM_HPF_FREQ_MAX 1000.0f
#define OMX_EQ8_PARAM_HPF_FREQ_DEFAULT 80.0f
#define OMX_EQ8_PARAM_HPF_SLOPE_MIN 12.0f
#define OMX_EQ8_PARAM_HPF_SLOPE_MAX 24.0f
#define OMX_EQ8_PARAM_HPF_SLOPE_DEFAULT 12.0f
#define OMX_EQ8_PARAM_LPF_ON_MIN 0.0f
#define OMX_EQ8_PARAM_LPF_ON_MAX 1.0f
#define OMX_EQ8_PARAM_LPF_ON_DEFAULT 0.0f
#define OMX_EQ8_PARAM_LPF_FREQ_MIN 1000.0f
#define OMX_EQ8_PARAM_LPF_FREQ_MAX 20000.0f
#define OMX_EQ8_PARAM_LPF_FREQ_DEFAULT 18000.0f
#define OMX_EQ8_PARAM_LPF_SLOPE_MIN 12.0f
#define OMX_EQ8_PARAM_LPF_SLOPE_MAX 24.0f
#define OMX_EQ8_PARAM_LPF_SLOPE_DEFAULT 12.0f
#define OMX_EQ8_PARAM_B1_TYPE_MIN 0.0f
#define OMX_EQ8_PARAM_B1_TYPE_MAX 5.0f
#define OMX_EQ8_PARAM_B1_TYPE_DEFAULT 1.0f
#define OMX_EQ8_PARAM_B1_FREQ_MIN 20.0f
#define OMX_EQ8_PARAM_B1_FREQ_MAX 20000.0f
#define OMX_EQ8_PARAM_B1_FREQ_DEFAULT 31.5f
#define OMX_EQ8_PARAM_B1_GAIN_MIN -15.0f
#define OMX_EQ8_PARAM_B1_GAIN_MAX 15.0f
#define OMX_EQ8_PARAM_B1_GAIN_DEFAULT 0.0f
#define OMX_EQ8_PARAM_B1_Q_MIN 0.3f
#define OMX_EQ8_PARAM_B1_Q_MAX 116.0f
#define OMX_EQ8_PARAM_B1_Q_DEFAULT 1.0f
#define OMX_EQ8_PARAM_B1_ON_MIN 0.0f
#define OMX_EQ8_PARAM_B1_ON_MAX 1.0f
#define OMX_EQ8_PARAM_B1_ON_DEFAULT 0.0f
#define OMX_EQ8_PARAM_B2_TYPE_MIN 0.0f
#define OMX_EQ8_PARAM_B2_TYPE_MAX 5.0f
#define OMX_EQ8_PARAM_B2_TYPE_DEFAULT 0.0f
#define OMX_EQ8_PARAM_B2_FREQ_MIN 20.0f
#define OMX_EQ8_PARAM_B2_FREQ_MAX 20000.0f
#define OMX_EQ8_PARAM_B2_FREQ_DEFAULT 80.0f
#define OMX_EQ8_PARAM_B2_GAIN_MIN -15.0f
#define OMX_EQ8_PARAM_B2_GAIN_MAX 15.0f
#define OMX_EQ8_PARAM_B2_GAIN_DEFAULT 0.0f
#define OMX_EQ8_PARAM_B2_Q_MIN 0.3f
#define OMX_EQ8_PARAM_B2_Q_MAX 116.0f
#define OMX_EQ8_PARAM_B2_Q_DEFAULT 1.0f
#define OMX_EQ8_PARAM_B2_ON_MIN 0.0f
#define OMX_EQ8_PARAM_B2_ON_MAX 1.0f
#define OMX_EQ8_PARAM_B2_ON_DEFAULT 0.0f
#define OMX_EQ8_PARAM_B3_TYPE_MIN 0.0f
#define OMX_EQ8_PARAM_B3_TYPE_MAX 5.0f
#define OMX_EQ8_PARAM_B3_TYPE_DEFAULT 0.0f
#define OMX_EQ8_PARAM_B3_FREQ_MIN 20.0f
#define OMX_EQ8_PARAM_B3_FREQ_MAX 20000.0f
#define OMX_EQ8_PARAM_B3_FREQ_DEFAULT 160.0f
#define OMX_EQ8_PARAM_B3_GAIN_MIN -15.0f
#define OMX_EQ8_PARAM_B3_GAIN_MAX 15.0f
#define OMX_EQ8_PARAM_B3_GAIN_DEFAULT 0.0f
#define OMX_EQ8_PARAM_B3_Q_MIN 0.3f
#define OMX_EQ8_PARAM_B3_Q_MAX 116.0f
#define OMX_EQ8_PARAM_B3_Q_DEFAULT 1.0f
#define OMX_EQ8_PARAM_B3_ON_MIN 0.0f
#define OMX_EQ8_PARAM_B3_ON_MAX 1.0f
#define OMX_EQ8_PARAM_B3_ON_DEFAULT 0.0f
#define OMX_EQ8_PARAM_B4_TYPE_MIN 0.0f
#define OMX_EQ8_PARAM_B4_TYPE_MAX 5.0f
#define OMX_EQ8_PARAM_B4_TYPE_DEFAULT 0.0f
#define OMX_EQ8_PARAM_B4_FREQ_MIN 20.0f
#define OMX_EQ8_PARAM_B4_FREQ_MAX 20000.0f
#define OMX_EQ8_PARAM_B4_FREQ_DEFAULT 400.0f
#define OMX_EQ8_PARAM_B4_GAIN_MIN -15.0f
#define OMX_EQ8_PARAM_B4_GAIN_MAX 15.0f
#define OMX_EQ8_PARAM_B4_GAIN_DEFAULT 0.0f
#define OMX_EQ8_PARAM_B4_Q_MIN 0.3f
#define OMX_EQ8_PARAM_B4_Q_MAX 116.0f
#define OMX_EQ8_PARAM_B4_Q_DEFAULT 1.0f
#define OMX_EQ8_PARAM_B4_ON_MIN 0.0f
#define OMX_EQ8_PARAM_B4_ON_MAX 1.0f
#define OMX_EQ8_PARAM_B4_ON_DEFAULT 0.0f
#define OMX_EQ8_PARAM_B5_TYPE_MIN 0.0f
#define OMX_EQ8_PARAM_B5_TYPE_MAX 5.0f
#define OMX_EQ8_PARAM_B5_TYPE_DEFAULT 0.0f
#define OMX_EQ8_PARAM_B5_FREQ_MIN 20.0f
#define OMX_EQ8_PARAM_B5_FREQ_MAX 20000.0f
#define OMX_EQ8_PARAM_B5_FREQ_DEFAULT 1000.0f
#define OMX_EQ8_PARAM_B5_GAIN_MIN -15.0f
#define OMX_EQ8_PARAM_B5_GAIN_MAX 15.0f
#define OMX_EQ8_PARAM_B5_GAIN_DEFAULT 0.0f
#define OMX_EQ8_PARAM_B5_Q_MIN 0.3f
#define OMX_EQ8_PARAM_B5_Q_MAX 116.0f
#define OMX_EQ8_PARAM_B5_Q_DEFAULT 1.0f
#define OMX_EQ8_PARAM_B5_ON_MIN 0.0f
#define OMX_EQ8_PARAM_B5_ON_MAX 1.0f
#define OMX_EQ8_PARAM_B5_ON_DEFAULT 0.0f
#define OMX_EQ8_PARAM_B6_TYPE_MIN 0.0f
#define OMX_EQ8_PARAM_B6_TYPE_MAX 5.0f
#define OMX_EQ8_PARAM_B6_TYPE_DEFAULT 0.0f
#define OMX_EQ8_PARAM_B6_FREQ_MIN 20.0f
#define OMX_EQ8_PARAM_B6_FREQ_MAX 20000.0f
#define OMX_EQ8_PARAM_B6_FREQ_DEFAULT 2500.0f
#define OMX_EQ8_PARAM_B6_GAIN_MIN -15.0f
#define OMX_EQ8_PARAM_B6_GAIN_MAX 15.0f
#define OMX_EQ8_PARAM_B6_GAIN_DEFAULT 0.0f
#define OMX_EQ8_PARAM_B6_Q_MIN 0.3f
#define OMX_EQ8_PARAM_B6_Q_MAX 116.0f
#define OMX_EQ8_PARAM_B6_Q_DEFAULT 1.0f
#define OMX_EQ8_PARAM_B6_ON_MIN 0.0f
#define OMX_EQ8_PARAM_B6_ON_MAX 1.0f
#define OMX_EQ8_PARAM_B6_ON_DEFAULT 0.0f
#define OMX_EQ8_PARAM_B7_TYPE_MIN 0.0f
#define OMX_EQ8_PARAM_B7_TYPE_MAX 5.0f
#define OMX_EQ8_PARAM_B7_TYPE_DEFAULT 0.0f
#define OMX_EQ8_PARAM_B7_FREQ_MIN 20.0f
#define OMX_EQ8_PARAM_B7_FREQ_MAX 20000.0f
#define OMX_EQ8_PARAM_B7_FREQ_DEFAULT 5000.0f
#define OMX_EQ8_PARAM_B7_GAIN_MIN -15.0f
#define OMX_EQ8_PARAM_B7_GAIN_MAX 15.0f
#define OMX_EQ8_PARAM_B7_GAIN_DEFAULT 0.0f
#define OMX_EQ8_PARAM_B7_Q_MIN 0.3f
#define OMX_EQ8_PARAM_B7_Q_MAX 116.0f
#define OMX_EQ8_PARAM_B7_Q_DEFAULT 1.0f
#define OMX_EQ8_PARAM_B7_ON_MIN 0.0f
#define OMX_EQ8_PARAM_B7_ON_MAX 1.0f
#define OMX_EQ8_PARAM_B7_ON_DEFAULT 0.0f
#define OMX_EQ8_PARAM_B8_TYPE_MIN 0.0f
#define OMX_EQ8_PARAM_B8_TYPE_MAX 5.0f
#define OMX_EQ8_PARAM_B8_TYPE_DEFAULT 2.0f
#define OMX_EQ8_PARAM_B8_FREQ_MIN 20.0f
#define OMX_EQ8_PARAM_B8_FREQ_MAX 20000.0f
#define OMX_EQ8_PARAM_B8_FREQ_DEFAULT 12500.0f
#define OMX_EQ8_PARAM_B8_GAIN_MIN -15.0f
#define OMX_EQ8_PARAM_B8_GAIN_MAX 15.0f
#define OMX_EQ8_PARAM_B8_GAIN_DEFAULT 0.0f
#define OMX_EQ8_PARAM_B8_Q_MIN 0.3f
#define OMX_EQ8_PARAM_B8_Q_MAX 116.0f
#define OMX_EQ8_PARAM_B8_Q_DEFAULT 1.0f
#define OMX_EQ8_PARAM_B8_ON_MIN 0.0f
#define OMX_EQ8_PARAM_B8_ON_MAX 1.0f
#define OMX_EQ8_PARAM_B8_ON_DEFAULT 0.0f

/* The identity every face publishes. */
#define OMX_EQ8_NAME "omx eq8"
#define OMX_EQ8_VENDOR "openmixer"
#define OMX_EQ8_URL "https://github.com/FreeMixer/omx-plugins"
#define OMX_EQ8_VERSION "0.2.0"
#define OMX_EQ8_DESCRIPTION "The OpenMixer console's channel EQ, 8-band form: 8 parametric bands (bell, shelves, notch, all-pass) and a high-pass and a low-pass filter at 12 or 24 dB/oct, stereo, zero latency; every band ships off, so a racked instance is a wire."
#define OMX_EQ8_CLAP_ID "org.openmixer.eq8"
#define OMX_EQ8_CLAP_FEATURES "audio-effect", "equalizer", "stereo"
#define OMX_EQ8_LV2_URI "urn:openmixer:eq8"
/* org.openmixer.declaration/1: the declaration's plain description and the digest of its parameters. */
#define OMX_EQ8_DECL_SOURCE "omx eq8: HPF On 0 to 1, HPF Frequency 20 to 1000 Hz, HPF Slope 12 to 24 dB/oct, LPF On 0 to 1, LPF Frequency 1000 to 20000 Hz, LPF Slope 12 to 24 dB/oct, Band 1 Type 0 to 5, Band 1 Frequency 20 to 20000 Hz, Band 1 Gain -15 to 15 dB, Band 1 Q 0.3 to 116, Band 1 On 0 to 1, Band 2 Type 0 to 5, Band 2 Frequency 20 to 20000 Hz, Band 2 Gain -15 to 15 dB, Band 2 Q 0.3 to 116, Band 2 On 0 to 1, Band 3 Type 0 to 5, Band 3 Frequency 20 to 20000 Hz, Band 3 Gain -15 to 15 dB, Band 3 Q 0.3 to 116, Band 3 On 0 to 1, Band 4 Type 0 to 5, Band 4 Frequency 20 to 20000 Hz, Band 4 Gain -15 to 15 dB, Band 4 Q 0.3 to 116, Band 4 On 0 to 1, Band 5 Type 0 to 5, Band 5 Frequency 20 to 20000 Hz, Band 5 Gain -15 to 15 dB, Band 5 Q 0.3 to 116, Band 5 On 0 to 1, Band 6 Type 0 to 5, Band 6 Frequency 20 to 20000 Hz, Band 6 Gain -15 to 15 dB, Band 6 Q 0.3 to 116, Band 6 On 0 to 1, Band 7 Type 0 to 5, Band 7 Frequency 20 to 20000 Hz, Band 7 Gain -15 to 15 dB, Band 7 Q 0.3 to 116, Band 7 On 0 to 1, Band 8 Type 0 to 5, Band 8 Frequency 20 to 20000 Hz, Band 8 Gain -15 to 15 dB, Band 8 Q 0.3 to 116, Band 8 On 0 to 1"
#define OMX_EQ8_DECL_DIGEST "c3aed8574463db0f192ce1c2001a47944578978db7ebdfd2cadcd5602dc271a0"

/* LV2 port indices (tools/gen.mjs lv2Ports): audio, then parameter i at FIRST_PARAM + i, then these. */
#define OMX_EQ8_LV2_PORT_IN_L 0u
#define OMX_EQ8_LV2_PORT_IN_R 1u
#define OMX_EQ8_LV2_PORT_OUT_L 2u
#define OMX_EQ8_LV2_PORT_OUT_R 3u
#define OMX_EQ8_LV2_PORT_ENABLED 50u
#define OMX_EQ8_LV2_PORT_LATENCY 51u
#define OMX_EQ8_LV2_PORT_FIRST_PARAM 4u
#define OMX_EQ8_LV2_PORT_COUNT 52u

#endif /* OMX_EQ8_PARAMS_H */
