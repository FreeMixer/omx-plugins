// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#ifndef OMX_EQ16_PARAMS_H
#define OMX_EQ16_PARAMS_H
/*
 * GENERATED — DO NOT EDIT BY HAND.
 * Produced by tools/gen.mjs from plugins/omx-eq16/omx-eq16.decl.json.
 * Regenerate: `make -C plugins/omx-eq16 gen`, then commit the result. Order is append-only.
 * The include guard is omx-dsp's own <omxdsp/params/omx_eq16_params.h>: included first, this
 * table is the one the kernel's instance header reads.
 */
#include "omx_plugin_param.h"

/* The band count of this variant of plugins/omx-eq: the instance face's compile-time count. */
#define OMX_EQ_INSTANCE_BANDS OMX_EQ_BAND_COUNTS_EQ16_MAX

enum {
  OMX_EQ16_PARAM_HPF_ON = 0,
  OMX_EQ16_PARAM_HPF_FREQ = 1,
  OMX_EQ16_PARAM_HPF_SLOPE = 2,
  OMX_EQ16_PARAM_LPF_ON = 3,
  OMX_EQ16_PARAM_LPF_FREQ = 4,
  OMX_EQ16_PARAM_LPF_SLOPE = 5,
  OMX_EQ16_PARAM_B1_TYPE = 6,
  OMX_EQ16_PARAM_B1_FREQ = 7,
  OMX_EQ16_PARAM_B1_GAIN = 8,
  OMX_EQ16_PARAM_B1_Q = 9,
  OMX_EQ16_PARAM_B1_ON = 10,
  OMX_EQ16_PARAM_B2_TYPE = 11,
  OMX_EQ16_PARAM_B2_FREQ = 12,
  OMX_EQ16_PARAM_B2_GAIN = 13,
  OMX_EQ16_PARAM_B2_Q = 14,
  OMX_EQ16_PARAM_B2_ON = 15,
  OMX_EQ16_PARAM_B3_TYPE = 16,
  OMX_EQ16_PARAM_B3_FREQ = 17,
  OMX_EQ16_PARAM_B3_GAIN = 18,
  OMX_EQ16_PARAM_B3_Q = 19,
  OMX_EQ16_PARAM_B3_ON = 20,
  OMX_EQ16_PARAM_B4_TYPE = 21,
  OMX_EQ16_PARAM_B4_FREQ = 22,
  OMX_EQ16_PARAM_B4_GAIN = 23,
  OMX_EQ16_PARAM_B4_Q = 24,
  OMX_EQ16_PARAM_B4_ON = 25,
  OMX_EQ16_PARAM_B5_TYPE = 26,
  OMX_EQ16_PARAM_B5_FREQ = 27,
  OMX_EQ16_PARAM_B5_GAIN = 28,
  OMX_EQ16_PARAM_B5_Q = 29,
  OMX_EQ16_PARAM_B5_ON = 30,
  OMX_EQ16_PARAM_B6_TYPE = 31,
  OMX_EQ16_PARAM_B6_FREQ = 32,
  OMX_EQ16_PARAM_B6_GAIN = 33,
  OMX_EQ16_PARAM_B6_Q = 34,
  OMX_EQ16_PARAM_B6_ON = 35,
  OMX_EQ16_PARAM_B7_TYPE = 36,
  OMX_EQ16_PARAM_B7_FREQ = 37,
  OMX_EQ16_PARAM_B7_GAIN = 38,
  OMX_EQ16_PARAM_B7_Q = 39,
  OMX_EQ16_PARAM_B7_ON = 40,
  OMX_EQ16_PARAM_B8_TYPE = 41,
  OMX_EQ16_PARAM_B8_FREQ = 42,
  OMX_EQ16_PARAM_B8_GAIN = 43,
  OMX_EQ16_PARAM_B8_Q = 44,
  OMX_EQ16_PARAM_B8_ON = 45,
  OMX_EQ16_PARAM_B9_TYPE = 46,
  OMX_EQ16_PARAM_B9_FREQ = 47,
  OMX_EQ16_PARAM_B9_GAIN = 48,
  OMX_EQ16_PARAM_B9_Q = 49,
  OMX_EQ16_PARAM_B9_ON = 50,
  OMX_EQ16_PARAM_B10_TYPE = 51,
  OMX_EQ16_PARAM_B10_FREQ = 52,
  OMX_EQ16_PARAM_B10_GAIN = 53,
  OMX_EQ16_PARAM_B10_Q = 54,
  OMX_EQ16_PARAM_B10_ON = 55,
  OMX_EQ16_PARAM_B11_TYPE = 56,
  OMX_EQ16_PARAM_B11_FREQ = 57,
  OMX_EQ16_PARAM_B11_GAIN = 58,
  OMX_EQ16_PARAM_B11_Q = 59,
  OMX_EQ16_PARAM_B11_ON = 60,
  OMX_EQ16_PARAM_B12_TYPE = 61,
  OMX_EQ16_PARAM_B12_FREQ = 62,
  OMX_EQ16_PARAM_B12_GAIN = 63,
  OMX_EQ16_PARAM_B12_Q = 64,
  OMX_EQ16_PARAM_B12_ON = 65,
  OMX_EQ16_PARAM_B13_TYPE = 66,
  OMX_EQ16_PARAM_B13_FREQ = 67,
  OMX_EQ16_PARAM_B13_GAIN = 68,
  OMX_EQ16_PARAM_B13_Q = 69,
  OMX_EQ16_PARAM_B13_ON = 70,
  OMX_EQ16_PARAM_B14_TYPE = 71,
  OMX_EQ16_PARAM_B14_FREQ = 72,
  OMX_EQ16_PARAM_B14_GAIN = 73,
  OMX_EQ16_PARAM_B14_Q = 74,
  OMX_EQ16_PARAM_B14_ON = 75,
  OMX_EQ16_PARAM_B15_TYPE = 76,
  OMX_EQ16_PARAM_B15_FREQ = 77,
  OMX_EQ16_PARAM_B15_GAIN = 78,
  OMX_EQ16_PARAM_B15_Q = 79,
  OMX_EQ16_PARAM_B15_ON = 80,
  OMX_EQ16_PARAM_B16_TYPE = 81,
  OMX_EQ16_PARAM_B16_FREQ = 82,
  OMX_EQ16_PARAM_B16_GAIN = 83,
  OMX_EQ16_PARAM_B16_Q = 84,
  OMX_EQ16_PARAM_B16_ON = 85,
  OMX_EQ16_PARAM_COUNT = 86
};

static const omx_plugin_param OMX_EQ16_PARAMS[OMX_EQ16_PARAM_COUNT] = {
  { "hpf_on", "HPF On", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "hpf_freq", "HPF Frequency", "Hz", 20.0f, 1000.0f, 80.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "hpf_slope", "HPF Slope", "dB/oct", 12.0f, 24.0f, 12.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "lpf_on", "LPF On", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "lpf_freq", "LPF Frequency", "Hz", 1000.0f, 20000.0f, 18000.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "lpf_slope", "LPF Slope", "dB/oct", 12.0f, 24.0f, 12.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b1_type", "Band 1 Type", "", 0.0f, 5.0f, 1.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b1_freq", "Band 1 Frequency", "Hz", 20.0f, 20000.0f, 25.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b1_gain", "Band 1 Gain", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "b1_q", "Band 1 Q", "", 0.3f, 116.0f, 1.0f, 0u },
  { "b1_on", "Band 1 On", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "b2_type", "Band 2 Type", "", 0.0f, 5.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b2_freq", "Band 2 Frequency", "Hz", 20.0f, 20000.0f, 40.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b2_gain", "Band 2 Gain", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "b2_q", "Band 2 Q", "", 0.3f, 116.0f, 1.0f, 0u },
  { "b2_on", "Band 2 On", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "b3_type", "Band 3 Type", "", 0.0f, 5.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b3_freq", "Band 3 Frequency", "Hz", 20.0f, 20000.0f, 63.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b3_gain", "Band 3 Gain", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "b3_q", "Band 3 Q", "", 0.3f, 116.0f, 1.0f, 0u },
  { "b3_on", "Band 3 On", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "b4_type", "Band 4 Type", "", 0.0f, 5.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b4_freq", "Band 4 Frequency", "Hz", 20.0f, 20000.0f, 100.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b4_gain", "Band 4 Gain", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "b4_q", "Band 4 Q", "", 0.3f, 116.0f, 1.0f, 0u },
  { "b4_on", "Band 4 On", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "b5_type", "Band 5 Type", "", 0.0f, 5.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b5_freq", "Band 5 Frequency", "Hz", 20.0f, 20000.0f, 125.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b5_gain", "Band 5 Gain", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "b5_q", "Band 5 Q", "", 0.3f, 116.0f, 1.0f, 0u },
  { "b5_on", "Band 5 On", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "b6_type", "Band 6 Type", "", 0.0f, 5.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b6_freq", "Band 6 Frequency", "Hz", 20.0f, 20000.0f, 200.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b6_gain", "Band 6 Gain", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "b6_q", "Band 6 Q", "", 0.3f, 116.0f, 1.0f, 0u },
  { "b6_on", "Band 6 On", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "b7_type", "Band 7 Type", "", 0.0f, 5.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b7_freq", "Band 7 Frequency", "Hz", 20.0f, 20000.0f, 315.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b7_gain", "Band 7 Gain", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "b7_q", "Band 7 Q", "", 0.3f, 116.0f, 1.0f, 0u },
  { "b7_on", "Band 7 On", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "b8_type", "Band 8 Type", "", 0.0f, 5.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b8_freq", "Band 8 Frequency", "Hz", 20.0f, 20000.0f, 500.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b8_gain", "Band 8 Gain", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "b8_q", "Band 8 Q", "", 0.3f, 116.0f, 1.0f, 0u },
  { "b8_on", "Band 8 On", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "b9_type", "Band 9 Type", "", 0.0f, 5.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b9_freq", "Band 9 Frequency", "Hz", 20.0f, 20000.0f, 800.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b9_gain", "Band 9 Gain", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "b9_q", "Band 9 Q", "", 0.3f, 116.0f, 1.0f, 0u },
  { "b9_on", "Band 9 On", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "b10_type", "Band 10 Type", "", 0.0f, 5.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b10_freq", "Band 10 Frequency", "Hz", 20.0f, 20000.0f, 1250.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b10_gain", "Band 10 Gain", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "b10_q", "Band 10 Q", "", 0.3f, 116.0f, 1.0f, 0u },
  { "b10_on", "Band 10 On", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "b11_type", "Band 11 Type", "", 0.0f, 5.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b11_freq", "Band 11 Frequency", "Hz", 20.0f, 20000.0f, 2000.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b11_gain", "Band 11 Gain", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "b11_q", "Band 11 Q", "", 0.3f, 116.0f, 1.0f, 0u },
  { "b11_on", "Band 11 On", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "b12_type", "Band 12 Type", "", 0.0f, 5.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b12_freq", "Band 12 Frequency", "Hz", 20.0f, 20000.0f, 3150.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b12_gain", "Band 12 Gain", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "b12_q", "Band 12 Q", "", 0.3f, 116.0f, 1.0f, 0u },
  { "b12_on", "Band 12 On", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "b13_type", "Band 13 Type", "", 0.0f, 5.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b13_freq", "Band 13 Frequency", "Hz", 20.0f, 20000.0f, 4000.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b13_gain", "Band 13 Gain", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "b13_q", "Band 13 Q", "", 0.3f, 116.0f, 1.0f, 0u },
  { "b13_on", "Band 13 On", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "b14_type", "Band 14 Type", "", 0.0f, 5.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b14_freq", "Band 14 Frequency", "Hz", 20.0f, 20000.0f, 6300.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b14_gain", "Band 14 Gain", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "b14_q", "Band 14 Q", "", 0.3f, 116.0f, 1.0f, 0u },
  { "b14_on", "Band 14 On", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "b15_type", "Band 15 Type", "", 0.0f, 5.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b15_freq", "Band 15 Frequency", "Hz", 20.0f, 20000.0f, 10000.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b15_gain", "Band 15 Gain", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "b15_q", "Band 15 Q", "", 0.3f, 116.0f, 1.0f, 0u },
  { "b15_on", "Band 15 On", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "b16_type", "Band 16 Type", "", 0.0f, 5.0f, 2.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b16_freq", "Band 16 Frequency", "Hz", 20.0f, 20000.0f, 16000.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "b16_gain", "Band 16 Gain", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "b16_q", "Band 16 Q", "", 0.3f, 116.0f, 1.0f, 0u },
  { "b16_on", "Band 16 On", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
};

/* One macro per declared bound: what a C face reads where a constant is needed. */
#define OMX_EQ16_PARAM_HPF_ON_MIN 0.0f
#define OMX_EQ16_PARAM_HPF_ON_MAX 1.0f
#define OMX_EQ16_PARAM_HPF_ON_DEFAULT 0.0f
#define OMX_EQ16_PARAM_HPF_FREQ_MIN 20.0f
#define OMX_EQ16_PARAM_HPF_FREQ_MAX 1000.0f
#define OMX_EQ16_PARAM_HPF_FREQ_DEFAULT 80.0f
#define OMX_EQ16_PARAM_HPF_SLOPE_MIN 12.0f
#define OMX_EQ16_PARAM_HPF_SLOPE_MAX 24.0f
#define OMX_EQ16_PARAM_HPF_SLOPE_DEFAULT 12.0f
#define OMX_EQ16_PARAM_LPF_ON_MIN 0.0f
#define OMX_EQ16_PARAM_LPF_ON_MAX 1.0f
#define OMX_EQ16_PARAM_LPF_ON_DEFAULT 0.0f
#define OMX_EQ16_PARAM_LPF_FREQ_MIN 1000.0f
#define OMX_EQ16_PARAM_LPF_FREQ_MAX 20000.0f
#define OMX_EQ16_PARAM_LPF_FREQ_DEFAULT 18000.0f
#define OMX_EQ16_PARAM_LPF_SLOPE_MIN 12.0f
#define OMX_EQ16_PARAM_LPF_SLOPE_MAX 24.0f
#define OMX_EQ16_PARAM_LPF_SLOPE_DEFAULT 12.0f
#define OMX_EQ16_PARAM_B1_TYPE_MIN 0.0f
#define OMX_EQ16_PARAM_B1_TYPE_MAX 5.0f
#define OMX_EQ16_PARAM_B1_TYPE_DEFAULT 1.0f
#define OMX_EQ16_PARAM_B1_FREQ_MIN 20.0f
#define OMX_EQ16_PARAM_B1_FREQ_MAX 20000.0f
#define OMX_EQ16_PARAM_B1_FREQ_DEFAULT 25.0f
#define OMX_EQ16_PARAM_B1_GAIN_MIN -15.0f
#define OMX_EQ16_PARAM_B1_GAIN_MAX 15.0f
#define OMX_EQ16_PARAM_B1_GAIN_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B1_Q_MIN 0.3f
#define OMX_EQ16_PARAM_B1_Q_MAX 116.0f
#define OMX_EQ16_PARAM_B1_Q_DEFAULT 1.0f
#define OMX_EQ16_PARAM_B1_ON_MIN 0.0f
#define OMX_EQ16_PARAM_B1_ON_MAX 1.0f
#define OMX_EQ16_PARAM_B1_ON_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B2_TYPE_MIN 0.0f
#define OMX_EQ16_PARAM_B2_TYPE_MAX 5.0f
#define OMX_EQ16_PARAM_B2_TYPE_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B2_FREQ_MIN 20.0f
#define OMX_EQ16_PARAM_B2_FREQ_MAX 20000.0f
#define OMX_EQ16_PARAM_B2_FREQ_DEFAULT 40.0f
#define OMX_EQ16_PARAM_B2_GAIN_MIN -15.0f
#define OMX_EQ16_PARAM_B2_GAIN_MAX 15.0f
#define OMX_EQ16_PARAM_B2_GAIN_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B2_Q_MIN 0.3f
#define OMX_EQ16_PARAM_B2_Q_MAX 116.0f
#define OMX_EQ16_PARAM_B2_Q_DEFAULT 1.0f
#define OMX_EQ16_PARAM_B2_ON_MIN 0.0f
#define OMX_EQ16_PARAM_B2_ON_MAX 1.0f
#define OMX_EQ16_PARAM_B2_ON_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B3_TYPE_MIN 0.0f
#define OMX_EQ16_PARAM_B3_TYPE_MAX 5.0f
#define OMX_EQ16_PARAM_B3_TYPE_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B3_FREQ_MIN 20.0f
#define OMX_EQ16_PARAM_B3_FREQ_MAX 20000.0f
#define OMX_EQ16_PARAM_B3_FREQ_DEFAULT 63.0f
#define OMX_EQ16_PARAM_B3_GAIN_MIN -15.0f
#define OMX_EQ16_PARAM_B3_GAIN_MAX 15.0f
#define OMX_EQ16_PARAM_B3_GAIN_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B3_Q_MIN 0.3f
#define OMX_EQ16_PARAM_B3_Q_MAX 116.0f
#define OMX_EQ16_PARAM_B3_Q_DEFAULT 1.0f
#define OMX_EQ16_PARAM_B3_ON_MIN 0.0f
#define OMX_EQ16_PARAM_B3_ON_MAX 1.0f
#define OMX_EQ16_PARAM_B3_ON_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B4_TYPE_MIN 0.0f
#define OMX_EQ16_PARAM_B4_TYPE_MAX 5.0f
#define OMX_EQ16_PARAM_B4_TYPE_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B4_FREQ_MIN 20.0f
#define OMX_EQ16_PARAM_B4_FREQ_MAX 20000.0f
#define OMX_EQ16_PARAM_B4_FREQ_DEFAULT 100.0f
#define OMX_EQ16_PARAM_B4_GAIN_MIN -15.0f
#define OMX_EQ16_PARAM_B4_GAIN_MAX 15.0f
#define OMX_EQ16_PARAM_B4_GAIN_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B4_Q_MIN 0.3f
#define OMX_EQ16_PARAM_B4_Q_MAX 116.0f
#define OMX_EQ16_PARAM_B4_Q_DEFAULT 1.0f
#define OMX_EQ16_PARAM_B4_ON_MIN 0.0f
#define OMX_EQ16_PARAM_B4_ON_MAX 1.0f
#define OMX_EQ16_PARAM_B4_ON_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B5_TYPE_MIN 0.0f
#define OMX_EQ16_PARAM_B5_TYPE_MAX 5.0f
#define OMX_EQ16_PARAM_B5_TYPE_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B5_FREQ_MIN 20.0f
#define OMX_EQ16_PARAM_B5_FREQ_MAX 20000.0f
#define OMX_EQ16_PARAM_B5_FREQ_DEFAULT 125.0f
#define OMX_EQ16_PARAM_B5_GAIN_MIN -15.0f
#define OMX_EQ16_PARAM_B5_GAIN_MAX 15.0f
#define OMX_EQ16_PARAM_B5_GAIN_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B5_Q_MIN 0.3f
#define OMX_EQ16_PARAM_B5_Q_MAX 116.0f
#define OMX_EQ16_PARAM_B5_Q_DEFAULT 1.0f
#define OMX_EQ16_PARAM_B5_ON_MIN 0.0f
#define OMX_EQ16_PARAM_B5_ON_MAX 1.0f
#define OMX_EQ16_PARAM_B5_ON_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B6_TYPE_MIN 0.0f
#define OMX_EQ16_PARAM_B6_TYPE_MAX 5.0f
#define OMX_EQ16_PARAM_B6_TYPE_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B6_FREQ_MIN 20.0f
#define OMX_EQ16_PARAM_B6_FREQ_MAX 20000.0f
#define OMX_EQ16_PARAM_B6_FREQ_DEFAULT 200.0f
#define OMX_EQ16_PARAM_B6_GAIN_MIN -15.0f
#define OMX_EQ16_PARAM_B6_GAIN_MAX 15.0f
#define OMX_EQ16_PARAM_B6_GAIN_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B6_Q_MIN 0.3f
#define OMX_EQ16_PARAM_B6_Q_MAX 116.0f
#define OMX_EQ16_PARAM_B6_Q_DEFAULT 1.0f
#define OMX_EQ16_PARAM_B6_ON_MIN 0.0f
#define OMX_EQ16_PARAM_B6_ON_MAX 1.0f
#define OMX_EQ16_PARAM_B6_ON_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B7_TYPE_MIN 0.0f
#define OMX_EQ16_PARAM_B7_TYPE_MAX 5.0f
#define OMX_EQ16_PARAM_B7_TYPE_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B7_FREQ_MIN 20.0f
#define OMX_EQ16_PARAM_B7_FREQ_MAX 20000.0f
#define OMX_EQ16_PARAM_B7_FREQ_DEFAULT 315.0f
#define OMX_EQ16_PARAM_B7_GAIN_MIN -15.0f
#define OMX_EQ16_PARAM_B7_GAIN_MAX 15.0f
#define OMX_EQ16_PARAM_B7_GAIN_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B7_Q_MIN 0.3f
#define OMX_EQ16_PARAM_B7_Q_MAX 116.0f
#define OMX_EQ16_PARAM_B7_Q_DEFAULT 1.0f
#define OMX_EQ16_PARAM_B7_ON_MIN 0.0f
#define OMX_EQ16_PARAM_B7_ON_MAX 1.0f
#define OMX_EQ16_PARAM_B7_ON_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B8_TYPE_MIN 0.0f
#define OMX_EQ16_PARAM_B8_TYPE_MAX 5.0f
#define OMX_EQ16_PARAM_B8_TYPE_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B8_FREQ_MIN 20.0f
#define OMX_EQ16_PARAM_B8_FREQ_MAX 20000.0f
#define OMX_EQ16_PARAM_B8_FREQ_DEFAULT 500.0f
#define OMX_EQ16_PARAM_B8_GAIN_MIN -15.0f
#define OMX_EQ16_PARAM_B8_GAIN_MAX 15.0f
#define OMX_EQ16_PARAM_B8_GAIN_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B8_Q_MIN 0.3f
#define OMX_EQ16_PARAM_B8_Q_MAX 116.0f
#define OMX_EQ16_PARAM_B8_Q_DEFAULT 1.0f
#define OMX_EQ16_PARAM_B8_ON_MIN 0.0f
#define OMX_EQ16_PARAM_B8_ON_MAX 1.0f
#define OMX_EQ16_PARAM_B8_ON_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B9_TYPE_MIN 0.0f
#define OMX_EQ16_PARAM_B9_TYPE_MAX 5.0f
#define OMX_EQ16_PARAM_B9_TYPE_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B9_FREQ_MIN 20.0f
#define OMX_EQ16_PARAM_B9_FREQ_MAX 20000.0f
#define OMX_EQ16_PARAM_B9_FREQ_DEFAULT 800.0f
#define OMX_EQ16_PARAM_B9_GAIN_MIN -15.0f
#define OMX_EQ16_PARAM_B9_GAIN_MAX 15.0f
#define OMX_EQ16_PARAM_B9_GAIN_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B9_Q_MIN 0.3f
#define OMX_EQ16_PARAM_B9_Q_MAX 116.0f
#define OMX_EQ16_PARAM_B9_Q_DEFAULT 1.0f
#define OMX_EQ16_PARAM_B9_ON_MIN 0.0f
#define OMX_EQ16_PARAM_B9_ON_MAX 1.0f
#define OMX_EQ16_PARAM_B9_ON_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B10_TYPE_MIN 0.0f
#define OMX_EQ16_PARAM_B10_TYPE_MAX 5.0f
#define OMX_EQ16_PARAM_B10_TYPE_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B10_FREQ_MIN 20.0f
#define OMX_EQ16_PARAM_B10_FREQ_MAX 20000.0f
#define OMX_EQ16_PARAM_B10_FREQ_DEFAULT 1250.0f
#define OMX_EQ16_PARAM_B10_GAIN_MIN -15.0f
#define OMX_EQ16_PARAM_B10_GAIN_MAX 15.0f
#define OMX_EQ16_PARAM_B10_GAIN_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B10_Q_MIN 0.3f
#define OMX_EQ16_PARAM_B10_Q_MAX 116.0f
#define OMX_EQ16_PARAM_B10_Q_DEFAULT 1.0f
#define OMX_EQ16_PARAM_B10_ON_MIN 0.0f
#define OMX_EQ16_PARAM_B10_ON_MAX 1.0f
#define OMX_EQ16_PARAM_B10_ON_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B11_TYPE_MIN 0.0f
#define OMX_EQ16_PARAM_B11_TYPE_MAX 5.0f
#define OMX_EQ16_PARAM_B11_TYPE_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B11_FREQ_MIN 20.0f
#define OMX_EQ16_PARAM_B11_FREQ_MAX 20000.0f
#define OMX_EQ16_PARAM_B11_FREQ_DEFAULT 2000.0f
#define OMX_EQ16_PARAM_B11_GAIN_MIN -15.0f
#define OMX_EQ16_PARAM_B11_GAIN_MAX 15.0f
#define OMX_EQ16_PARAM_B11_GAIN_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B11_Q_MIN 0.3f
#define OMX_EQ16_PARAM_B11_Q_MAX 116.0f
#define OMX_EQ16_PARAM_B11_Q_DEFAULT 1.0f
#define OMX_EQ16_PARAM_B11_ON_MIN 0.0f
#define OMX_EQ16_PARAM_B11_ON_MAX 1.0f
#define OMX_EQ16_PARAM_B11_ON_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B12_TYPE_MIN 0.0f
#define OMX_EQ16_PARAM_B12_TYPE_MAX 5.0f
#define OMX_EQ16_PARAM_B12_TYPE_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B12_FREQ_MIN 20.0f
#define OMX_EQ16_PARAM_B12_FREQ_MAX 20000.0f
#define OMX_EQ16_PARAM_B12_FREQ_DEFAULT 3150.0f
#define OMX_EQ16_PARAM_B12_GAIN_MIN -15.0f
#define OMX_EQ16_PARAM_B12_GAIN_MAX 15.0f
#define OMX_EQ16_PARAM_B12_GAIN_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B12_Q_MIN 0.3f
#define OMX_EQ16_PARAM_B12_Q_MAX 116.0f
#define OMX_EQ16_PARAM_B12_Q_DEFAULT 1.0f
#define OMX_EQ16_PARAM_B12_ON_MIN 0.0f
#define OMX_EQ16_PARAM_B12_ON_MAX 1.0f
#define OMX_EQ16_PARAM_B12_ON_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B13_TYPE_MIN 0.0f
#define OMX_EQ16_PARAM_B13_TYPE_MAX 5.0f
#define OMX_EQ16_PARAM_B13_TYPE_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B13_FREQ_MIN 20.0f
#define OMX_EQ16_PARAM_B13_FREQ_MAX 20000.0f
#define OMX_EQ16_PARAM_B13_FREQ_DEFAULT 4000.0f
#define OMX_EQ16_PARAM_B13_GAIN_MIN -15.0f
#define OMX_EQ16_PARAM_B13_GAIN_MAX 15.0f
#define OMX_EQ16_PARAM_B13_GAIN_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B13_Q_MIN 0.3f
#define OMX_EQ16_PARAM_B13_Q_MAX 116.0f
#define OMX_EQ16_PARAM_B13_Q_DEFAULT 1.0f
#define OMX_EQ16_PARAM_B13_ON_MIN 0.0f
#define OMX_EQ16_PARAM_B13_ON_MAX 1.0f
#define OMX_EQ16_PARAM_B13_ON_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B14_TYPE_MIN 0.0f
#define OMX_EQ16_PARAM_B14_TYPE_MAX 5.0f
#define OMX_EQ16_PARAM_B14_TYPE_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B14_FREQ_MIN 20.0f
#define OMX_EQ16_PARAM_B14_FREQ_MAX 20000.0f
#define OMX_EQ16_PARAM_B14_FREQ_DEFAULT 6300.0f
#define OMX_EQ16_PARAM_B14_GAIN_MIN -15.0f
#define OMX_EQ16_PARAM_B14_GAIN_MAX 15.0f
#define OMX_EQ16_PARAM_B14_GAIN_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B14_Q_MIN 0.3f
#define OMX_EQ16_PARAM_B14_Q_MAX 116.0f
#define OMX_EQ16_PARAM_B14_Q_DEFAULT 1.0f
#define OMX_EQ16_PARAM_B14_ON_MIN 0.0f
#define OMX_EQ16_PARAM_B14_ON_MAX 1.0f
#define OMX_EQ16_PARAM_B14_ON_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B15_TYPE_MIN 0.0f
#define OMX_EQ16_PARAM_B15_TYPE_MAX 5.0f
#define OMX_EQ16_PARAM_B15_TYPE_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B15_FREQ_MIN 20.0f
#define OMX_EQ16_PARAM_B15_FREQ_MAX 20000.0f
#define OMX_EQ16_PARAM_B15_FREQ_DEFAULT 10000.0f
#define OMX_EQ16_PARAM_B15_GAIN_MIN -15.0f
#define OMX_EQ16_PARAM_B15_GAIN_MAX 15.0f
#define OMX_EQ16_PARAM_B15_GAIN_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B15_Q_MIN 0.3f
#define OMX_EQ16_PARAM_B15_Q_MAX 116.0f
#define OMX_EQ16_PARAM_B15_Q_DEFAULT 1.0f
#define OMX_EQ16_PARAM_B15_ON_MIN 0.0f
#define OMX_EQ16_PARAM_B15_ON_MAX 1.0f
#define OMX_EQ16_PARAM_B15_ON_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B16_TYPE_MIN 0.0f
#define OMX_EQ16_PARAM_B16_TYPE_MAX 5.0f
#define OMX_EQ16_PARAM_B16_TYPE_DEFAULT 2.0f
#define OMX_EQ16_PARAM_B16_FREQ_MIN 20.0f
#define OMX_EQ16_PARAM_B16_FREQ_MAX 20000.0f
#define OMX_EQ16_PARAM_B16_FREQ_DEFAULT 16000.0f
#define OMX_EQ16_PARAM_B16_GAIN_MIN -15.0f
#define OMX_EQ16_PARAM_B16_GAIN_MAX 15.0f
#define OMX_EQ16_PARAM_B16_GAIN_DEFAULT 0.0f
#define OMX_EQ16_PARAM_B16_Q_MIN 0.3f
#define OMX_EQ16_PARAM_B16_Q_MAX 116.0f
#define OMX_EQ16_PARAM_B16_Q_DEFAULT 1.0f
#define OMX_EQ16_PARAM_B16_ON_MIN 0.0f
#define OMX_EQ16_PARAM_B16_ON_MAX 1.0f
#define OMX_EQ16_PARAM_B16_ON_DEFAULT 0.0f

/* The identity every face publishes. */
#define OMX_EQ16_NAME "omx eq16"
#define OMX_EQ16_VENDOR "openmixer"
#define OMX_EQ16_URL "https://github.com/FreeMixer/omx-plugins"
#define OMX_EQ16_VERSION "0.2.0"
#define OMX_EQ16_DESCRIPTION "The OpenMixer console's channel EQ, 16-band form: 16 parametric bands (bell, shelves, notch, all-pass) and a high-pass and a low-pass filter at 12 or 24 dB/oct, stereo, zero latency; every band ships off, so a racked instance is a wire."
#define OMX_EQ16_CLAP_ID "org.openmixer.eq16"
#define OMX_EQ16_CLAP_FEATURES "audio-effect", "equalizer", "stereo"
#define OMX_EQ16_LV2_URI "urn:openmixer:eq16"
/* org.openmixer.declaration/1: the declaration's plain description and the digest of its parameters. */
#define OMX_EQ16_DECL_SOURCE "omx eq16: HPF On 0 to 1, HPF Frequency 20 to 1000 Hz, HPF Slope 12 to 24 dB/oct, LPF On 0 to 1, LPF Frequency 1000 to 20000 Hz, LPF Slope 12 to 24 dB/oct, Band 1 Type 0 to 5, Band 1 Frequency 20 to 20000 Hz, Band 1 Gain -15 to 15 dB, Band 1 Q 0.3 to 116, Band 1 On 0 to 1, Band 2 Type 0 to 5, Band 2 Frequency 20 to 20000 Hz, Band 2 Gain -15 to 15 dB, Band 2 Q 0.3 to 116, Band 2 On 0 to 1, Band 3 Type 0 to 5, Band 3 Frequency 20 to 20000 Hz, Band 3 Gain -15 to 15 dB, Band 3 Q 0.3 to 116, Band 3 On 0 to 1, Band 4 Type 0 to 5, Band 4 Frequency 20 to 20000 Hz, Band 4 Gain -15 to 15 dB, Band 4 Q 0.3 to 116, Band 4 On 0 to 1, Band 5 Type 0 to 5, Band 5 Frequency 20 to 20000 Hz, Band 5 Gain -15 to 15 dB, Band 5 Q 0.3 to 116, Band 5 On 0 to 1, Band 6 Type 0 to 5, Band 6 Frequency 20 to 20000 Hz, Band 6 Gain -15 to 15 dB, Band 6 Q 0.3 to 116, Band 6 On 0 to 1, Band 7 Type 0 to 5, Band 7 Frequency 20 to 20000 Hz, Band 7 Gain -15 to 15 dB, Band 7 Q 0.3 to 116, Band 7 On 0 to 1, Band 8 Type 0 to 5, Band 8 Frequency 20 to 20000 Hz, Band 8 Gain -15 to 15 dB, Band 8 Q 0.3 to 116, Band 8 On 0 to 1, Band 9 Type 0 to 5, Band 9 Frequency 20 to 20000 Hz, Band 9 Gain -15 to 15 dB, Band 9 Q 0.3 to 116, Band 9 On 0 to 1, Band 10 Type 0 to 5, Band 10 Frequency 20 to 20000 Hz, Band 10 Gain -15 to 15 dB, Band 10 Q 0.3 to 116, Band 10 On 0 to 1, Band 11 Type 0 to 5, Band 11 Frequency 20 to 20000 Hz, Band 11 Gain -15 to 15 dB, Band 11 Q 0.3 to 116, Band 11 On 0 to 1, Band 12 Type 0 to 5, Band 12 Frequency 20 to 20000 Hz, Band 12 Gain -15 to 15 dB, Band 12 Q 0.3 to 116, Band 12 On 0 to 1, Band 13 Type 0 to 5, Band 13 Frequency 20 to 20000 Hz, Band 13 Gain -15 to 15 dB, Band 13 Q 0.3 to 116, Band 13 On 0 to 1, Band 14 Type 0 to 5, Band 14 Frequency 20 to 20000 Hz, Band 14 Gain -15 to 15 dB, Band 14 Q 0.3 to 116, Band 14 On 0 to 1, Band 15 Type 0 to 5, Band 15 Frequency 20 to 20000 Hz, Band 15 Gain -15 to 15 dB, Band 15 Q 0.3 to 116, Band 15 On 0 to 1, Band 16 Type 0 to 5, Band 16 Frequency 20 to 20000 Hz, Band 16 Gain -15 to 15 dB, Band 16 Q 0.3 to 116, Band 16 On 0 to 1"
#define OMX_EQ16_DECL_DIGEST "7b5f7b1f46ebc116414f68203cf8240c72c0154227d0169c0da832ee9aad2c56"

/* LV2 port indices (tools/gen.mjs lv2Ports): audio, then parameter i at FIRST_PARAM + i, then these. */
#define OMX_EQ16_LV2_PORT_IN_L 0u
#define OMX_EQ16_LV2_PORT_IN_R 1u
#define OMX_EQ16_LV2_PORT_OUT_L 2u
#define OMX_EQ16_LV2_PORT_OUT_R 3u
#define OMX_EQ16_LV2_PORT_ENABLED 90u
#define OMX_EQ16_LV2_PORT_LATENCY 91u
#define OMX_EQ16_LV2_PORT_FIRST_PARAM 4u
#define OMX_EQ16_LV2_PORT_COUNT 92u

#endif /* OMX_EQ16_PARAMS_H */
