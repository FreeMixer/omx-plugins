// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#ifndef OMX_STRIP_PARAMS_H
#define OMX_STRIP_PARAMS_H
/*
 * GENERATED — DO NOT EDIT BY HAND.
 * Produced by tools/gen.mjs from plugins/omx-strip/omx-strip.decl.json.
 * Regenerate: `make -C plugins/omx-strip gen`, then commit the result. Order is append-only.
 * The include guard is omx-dsp's own <omxdsp/params/omx_strip_params.h>: included first, this
 * table is the one the kernel's instance header reads.
 */
#include "omx_plugin_param.h"

/* The band count of the chain's filter: its instance face's compile-time count, omx-contract's OMX_EQ_BAND_COUNTS_EQ8_MAX. */
#define OMX_EQ_INSTANCE_BANDS OMX_EQ_BAND_COUNTS_EQ8_MAX

enum {
  OMX_STRIP_PARAM_TRIM_ON = 0,
  OMX_STRIP_PARAM_TRIM_DB = 1,
  OMX_STRIP_PARAM_FILTER_ON = 2,
  OMX_STRIP_PARAM_HPF_ON = 3,
  OMX_STRIP_PARAM_HPF_FREQ = 4,
  OMX_STRIP_PARAM_HPF_SLOPE = 5,
  OMX_STRIP_PARAM_LPF_ON = 6,
  OMX_STRIP_PARAM_LPF_FREQ = 7,
  OMX_STRIP_PARAM_LPF_SLOPE = 8,
  OMX_STRIP_PARAM_GATE_ON = 9,
  OMX_STRIP_PARAM_GATE_THRESHOLD = 10,
  OMX_STRIP_PARAM_GATE_RATIO = 11,
  OMX_STRIP_PARAM_GATE_RANGE = 12,
  OMX_STRIP_PARAM_GATE_ATTACK = 13,
  OMX_STRIP_PARAM_GATE_RELEASE = 14,
  OMX_STRIP_PARAM_GATE_KNEE_START = 15,
  OMX_STRIP_PARAM_GATE_KNEE_END = 16,
  OMX_STRIP_PARAM_GATE_HOLD = 17,
  OMX_STRIP_PARAM_GATE_HYSTERESIS = 18,
  OMX_STRIP_PARAM_EQ_ON = 19,
  OMX_STRIP_PARAM_EQ1_TYPE = 20,
  OMX_STRIP_PARAM_EQ1_FREQ = 21,
  OMX_STRIP_PARAM_EQ1_GAIN = 22,
  OMX_STRIP_PARAM_EQ1_Q = 23,
  OMX_STRIP_PARAM_EQ1_ON = 24,
  OMX_STRIP_PARAM_EQ2_TYPE = 25,
  OMX_STRIP_PARAM_EQ2_FREQ = 26,
  OMX_STRIP_PARAM_EQ2_GAIN = 27,
  OMX_STRIP_PARAM_EQ2_Q = 28,
  OMX_STRIP_PARAM_EQ2_ON = 29,
  OMX_STRIP_PARAM_EQ3_TYPE = 30,
  OMX_STRIP_PARAM_EQ3_FREQ = 31,
  OMX_STRIP_PARAM_EQ3_GAIN = 32,
  OMX_STRIP_PARAM_EQ3_Q = 33,
  OMX_STRIP_PARAM_EQ3_ON = 34,
  OMX_STRIP_PARAM_EQ4_TYPE = 35,
  OMX_STRIP_PARAM_EQ4_FREQ = 36,
  OMX_STRIP_PARAM_EQ4_GAIN = 37,
  OMX_STRIP_PARAM_EQ4_Q = 38,
  OMX_STRIP_PARAM_EQ4_ON = 39,
  OMX_STRIP_PARAM_COMP_ON = 40,
  OMX_STRIP_PARAM_COMP_THRESHOLD = 41,
  OMX_STRIP_PARAM_COMP_RATIO = 42,
  OMX_STRIP_PARAM_COMP_KNEE = 43,
  OMX_STRIP_PARAM_COMP_ATTACK = 44,
  OMX_STRIP_PARAM_COMP_RELEASE = 45,
  OMX_STRIP_PARAM_COMP_MAKEUP = 46,
  OMX_STRIP_PARAM_COMP_KIND = 47,
  OMX_STRIP_PARAM_COMP_MIX = 48,
  OMX_STRIP_PARAM_COMP_DETECTOR_OVERSAMPLING = 49,
  OMX_STRIP_PARAM_ORDER = 50,
  OMX_STRIP_PARAM_COUNT = 51
};

static const omx_plugin_param OMX_STRIP_PARAMS[OMX_STRIP_PARAM_COUNT] = {
  { "trimOn", "Input", "", 0.0f, 1.0f, 1.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "trimDb", "Trim", "dB", -24.0f, 24.0f, 0.0f, 0u },
  { "filterOn", "Filters", "", 0.0f, 1.0f, 1.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "hpfOn", "HPF", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "hpfFreq", "HPF Freq", "Hz", 20.0f, 1000.0f, 80.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "hpfSlope", "HPF Slope", "dB/oct", 12.0f, 24.0f, 12.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "lpfOn", "LPF", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "lpfFreq", "LPF Freq", "Hz", 1000.0f, 20000.0f, 18000.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "lpfSlope", "LPF Slope", "dB/oct", 12.0f, 24.0f, 12.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "gateOn", "Gate", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "gateThreshold", "Gate Threshold", "dB", -80.0f, 0.0f, -40.0f, 0u },
  { "gateRatio", "Gate Ratio", "", 1.0f, 100.0f, 16.0f, 0u },
  { "gateRange", "Gate Range", "dB", -90.0f, 0.0f, -90.0f, 0u },
  { "gateAttack", "Gate Attack", "ms", 0.0f, 500.0f, 1.0f, 0u },
  { "gateRelease", "Gate Release", "ms", 0.0f, 5000.0f, 100.0f, 0u },
  { "gateKneeStart", "Gate Knee Start", "dB", -80.0f, 0.0f, -43.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "gateKneeEnd", "Gate Knee End", "dB", -80.0f, 0.0f, -37.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "gateHold", "Gate Hold", "ms", 0.0f, 2000.0f, 10.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "gateHysteresis", "Gate Hysteresis", "dB", 0.0f, 24.0f, 3.0f, 0u },
  { "eqOn", "EQ", "", 0.0f, 1.0f, 1.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "eq1Type", "EQ 1 Type", "", 0.0f, 5.0f, 1.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "eq1Freq", "EQ 1 Freq", "Hz", 20.0f, 20000.0f, 100.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "eq1Gain", "EQ 1 Gain", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "eq1Q", "EQ 1 Q", "", 0.3f, 116.0f, 1.0f, 0u },
  { "eq1On", "EQ 1", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "eq2Type", "EQ 2 Type", "", 0.0f, 5.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "eq2Freq", "EQ 2 Freq", "Hz", 20.0f, 20000.0f, 400.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "eq2Gain", "EQ 2 Gain", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "eq2Q", "EQ 2 Q", "", 0.3f, 116.0f, 1.0f, 0u },
  { "eq2On", "EQ 2", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "eq3Type", "EQ 3 Type", "", 0.0f, 5.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "eq3Freq", "EQ 3 Freq", "Hz", 20.0f, 20000.0f, 2000.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "eq3Gain", "EQ 3 Gain", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "eq3Q", "EQ 3 Q", "", 0.3f, 116.0f, 1.0f, 0u },
  { "eq3On", "EQ 3", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "eq4Type", "EQ 4 Type", "", 0.0f, 5.0f, 2.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "eq4Freq", "EQ 4 Freq", "Hz", 20.0f, 20000.0f, 8000.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "eq4Gain", "EQ 4 Gain", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "eq4Q", "EQ 4 Q", "", 0.3f, 116.0f, 1.0f, 0u },
  { "eq4On", "EQ 4", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "compOn", "Comp", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "compThreshold", "Comp Threshold", "dB", -60.0f, 0.0f, -18.0f, 0u },
  { "compRatio", "Comp Ratio", "", 1.0f, 20.0f, 4.0f, 0u },
  { "compKnee", "Comp Knee", "dB", 0.0f, 24.0f, 6.0f, 0u },
  { "compAttack", "Comp Attack", "ms", 0.1f, 100.0f, 5.0f, 0u },
  { "compRelease", "Comp Release", "ms", 5.0f, 3000.0f, 200.0f, 0u },
  { "compMakeup", "Comp Makeup", "dB", 0.0f, 24.0f, 0.0f, 0u },
  { "compKind", "Comp Kind", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "compMix", "Comp Mix", "%", 0.0f, 100.0f, 100.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "compDetectorOversampling", "Comp Detector Oversampling", "", 0.0f, 2.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "order", "Order", "", 0.0f, 119.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
};

/* One macro per declared bound: what a C face reads where a constant is needed. */
#define OMX_STRIP_PARAM_TRIM_ON_MIN 0.0f
#define OMX_STRIP_PARAM_TRIM_ON_MAX 1.0f
#define OMX_STRIP_PARAM_TRIM_ON_DEFAULT 1.0f
#define OMX_STRIP_PARAM_TRIM_DB_MIN -24.0f
#define OMX_STRIP_PARAM_TRIM_DB_MAX 24.0f
#define OMX_STRIP_PARAM_TRIM_DB_DEFAULT 0.0f
#define OMX_STRIP_PARAM_FILTER_ON_MIN 0.0f
#define OMX_STRIP_PARAM_FILTER_ON_MAX 1.0f
#define OMX_STRIP_PARAM_FILTER_ON_DEFAULT 1.0f
#define OMX_STRIP_PARAM_HPF_ON_MIN 0.0f
#define OMX_STRIP_PARAM_HPF_ON_MAX 1.0f
#define OMX_STRIP_PARAM_HPF_ON_DEFAULT 0.0f
#define OMX_STRIP_PARAM_HPF_FREQ_MIN 20.0f
#define OMX_STRIP_PARAM_HPF_FREQ_MAX 1000.0f
#define OMX_STRIP_PARAM_HPF_FREQ_DEFAULT 80.0f
#define OMX_STRIP_PARAM_HPF_SLOPE_MIN 12.0f
#define OMX_STRIP_PARAM_HPF_SLOPE_MAX 24.0f
#define OMX_STRIP_PARAM_HPF_SLOPE_DEFAULT 12.0f
#define OMX_STRIP_PARAM_LPF_ON_MIN 0.0f
#define OMX_STRIP_PARAM_LPF_ON_MAX 1.0f
#define OMX_STRIP_PARAM_LPF_ON_DEFAULT 0.0f
#define OMX_STRIP_PARAM_LPF_FREQ_MIN 1000.0f
#define OMX_STRIP_PARAM_LPF_FREQ_MAX 20000.0f
#define OMX_STRIP_PARAM_LPF_FREQ_DEFAULT 18000.0f
#define OMX_STRIP_PARAM_LPF_SLOPE_MIN 12.0f
#define OMX_STRIP_PARAM_LPF_SLOPE_MAX 24.0f
#define OMX_STRIP_PARAM_LPF_SLOPE_DEFAULT 12.0f
#define OMX_STRIP_PARAM_GATE_ON_MIN 0.0f
#define OMX_STRIP_PARAM_GATE_ON_MAX 1.0f
#define OMX_STRIP_PARAM_GATE_ON_DEFAULT 0.0f
#define OMX_STRIP_PARAM_GATE_THRESHOLD_MIN -80.0f
#define OMX_STRIP_PARAM_GATE_THRESHOLD_MAX 0.0f
#define OMX_STRIP_PARAM_GATE_THRESHOLD_DEFAULT -40.0f
#define OMX_STRIP_PARAM_GATE_RATIO_MIN 1.0f
#define OMX_STRIP_PARAM_GATE_RATIO_MAX 100.0f
#define OMX_STRIP_PARAM_GATE_RATIO_DEFAULT 16.0f
#define OMX_STRIP_PARAM_GATE_RANGE_MIN -90.0f
#define OMX_STRIP_PARAM_GATE_RANGE_MAX 0.0f
#define OMX_STRIP_PARAM_GATE_RANGE_DEFAULT -90.0f
#define OMX_STRIP_PARAM_GATE_ATTACK_MIN 0.0f
#define OMX_STRIP_PARAM_GATE_ATTACK_MAX 500.0f
#define OMX_STRIP_PARAM_GATE_ATTACK_DEFAULT 1.0f
#define OMX_STRIP_PARAM_GATE_RELEASE_MIN 0.0f
#define OMX_STRIP_PARAM_GATE_RELEASE_MAX 5000.0f
#define OMX_STRIP_PARAM_GATE_RELEASE_DEFAULT 100.0f
#define OMX_STRIP_PARAM_GATE_KNEE_START_MIN -80.0f
#define OMX_STRIP_PARAM_GATE_KNEE_START_MAX 0.0f
#define OMX_STRIP_PARAM_GATE_KNEE_START_DEFAULT -43.0f
#define OMX_STRIP_PARAM_GATE_KNEE_END_MIN -80.0f
#define OMX_STRIP_PARAM_GATE_KNEE_END_MAX 0.0f
#define OMX_STRIP_PARAM_GATE_KNEE_END_DEFAULT -37.0f
#define OMX_STRIP_PARAM_GATE_HOLD_MIN 0.0f
#define OMX_STRIP_PARAM_GATE_HOLD_MAX 2000.0f
#define OMX_STRIP_PARAM_GATE_HOLD_DEFAULT 10.0f
#define OMX_STRIP_PARAM_GATE_HYSTERESIS_MIN 0.0f
#define OMX_STRIP_PARAM_GATE_HYSTERESIS_MAX 24.0f
#define OMX_STRIP_PARAM_GATE_HYSTERESIS_DEFAULT 3.0f
#define OMX_STRIP_PARAM_EQ_ON_MIN 0.0f
#define OMX_STRIP_PARAM_EQ_ON_MAX 1.0f
#define OMX_STRIP_PARAM_EQ_ON_DEFAULT 1.0f
#define OMX_STRIP_PARAM_EQ1_TYPE_MIN 0.0f
#define OMX_STRIP_PARAM_EQ1_TYPE_MAX 5.0f
#define OMX_STRIP_PARAM_EQ1_TYPE_DEFAULT 1.0f
#define OMX_STRIP_PARAM_EQ1_FREQ_MIN 20.0f
#define OMX_STRIP_PARAM_EQ1_FREQ_MAX 20000.0f
#define OMX_STRIP_PARAM_EQ1_FREQ_DEFAULT 100.0f
#define OMX_STRIP_PARAM_EQ1_GAIN_MIN -15.0f
#define OMX_STRIP_PARAM_EQ1_GAIN_MAX 15.0f
#define OMX_STRIP_PARAM_EQ1_GAIN_DEFAULT 0.0f
#define OMX_STRIP_PARAM_EQ1_Q_MIN 0.3f
#define OMX_STRIP_PARAM_EQ1_Q_MAX 116.0f
#define OMX_STRIP_PARAM_EQ1_Q_DEFAULT 1.0f
#define OMX_STRIP_PARAM_EQ1_ON_MIN 0.0f
#define OMX_STRIP_PARAM_EQ1_ON_MAX 1.0f
#define OMX_STRIP_PARAM_EQ1_ON_DEFAULT 0.0f
#define OMX_STRIP_PARAM_EQ2_TYPE_MIN 0.0f
#define OMX_STRIP_PARAM_EQ2_TYPE_MAX 5.0f
#define OMX_STRIP_PARAM_EQ2_TYPE_DEFAULT 0.0f
#define OMX_STRIP_PARAM_EQ2_FREQ_MIN 20.0f
#define OMX_STRIP_PARAM_EQ2_FREQ_MAX 20000.0f
#define OMX_STRIP_PARAM_EQ2_FREQ_DEFAULT 400.0f
#define OMX_STRIP_PARAM_EQ2_GAIN_MIN -15.0f
#define OMX_STRIP_PARAM_EQ2_GAIN_MAX 15.0f
#define OMX_STRIP_PARAM_EQ2_GAIN_DEFAULT 0.0f
#define OMX_STRIP_PARAM_EQ2_Q_MIN 0.3f
#define OMX_STRIP_PARAM_EQ2_Q_MAX 116.0f
#define OMX_STRIP_PARAM_EQ2_Q_DEFAULT 1.0f
#define OMX_STRIP_PARAM_EQ2_ON_MIN 0.0f
#define OMX_STRIP_PARAM_EQ2_ON_MAX 1.0f
#define OMX_STRIP_PARAM_EQ2_ON_DEFAULT 0.0f
#define OMX_STRIP_PARAM_EQ3_TYPE_MIN 0.0f
#define OMX_STRIP_PARAM_EQ3_TYPE_MAX 5.0f
#define OMX_STRIP_PARAM_EQ3_TYPE_DEFAULT 0.0f
#define OMX_STRIP_PARAM_EQ3_FREQ_MIN 20.0f
#define OMX_STRIP_PARAM_EQ3_FREQ_MAX 20000.0f
#define OMX_STRIP_PARAM_EQ3_FREQ_DEFAULT 2000.0f
#define OMX_STRIP_PARAM_EQ3_GAIN_MIN -15.0f
#define OMX_STRIP_PARAM_EQ3_GAIN_MAX 15.0f
#define OMX_STRIP_PARAM_EQ3_GAIN_DEFAULT 0.0f
#define OMX_STRIP_PARAM_EQ3_Q_MIN 0.3f
#define OMX_STRIP_PARAM_EQ3_Q_MAX 116.0f
#define OMX_STRIP_PARAM_EQ3_Q_DEFAULT 1.0f
#define OMX_STRIP_PARAM_EQ3_ON_MIN 0.0f
#define OMX_STRIP_PARAM_EQ3_ON_MAX 1.0f
#define OMX_STRIP_PARAM_EQ3_ON_DEFAULT 0.0f
#define OMX_STRIP_PARAM_EQ4_TYPE_MIN 0.0f
#define OMX_STRIP_PARAM_EQ4_TYPE_MAX 5.0f
#define OMX_STRIP_PARAM_EQ4_TYPE_DEFAULT 2.0f
#define OMX_STRIP_PARAM_EQ4_FREQ_MIN 20.0f
#define OMX_STRIP_PARAM_EQ4_FREQ_MAX 20000.0f
#define OMX_STRIP_PARAM_EQ4_FREQ_DEFAULT 8000.0f
#define OMX_STRIP_PARAM_EQ4_GAIN_MIN -15.0f
#define OMX_STRIP_PARAM_EQ4_GAIN_MAX 15.0f
#define OMX_STRIP_PARAM_EQ4_GAIN_DEFAULT 0.0f
#define OMX_STRIP_PARAM_EQ4_Q_MIN 0.3f
#define OMX_STRIP_PARAM_EQ4_Q_MAX 116.0f
#define OMX_STRIP_PARAM_EQ4_Q_DEFAULT 1.0f
#define OMX_STRIP_PARAM_EQ4_ON_MIN 0.0f
#define OMX_STRIP_PARAM_EQ4_ON_MAX 1.0f
#define OMX_STRIP_PARAM_EQ4_ON_DEFAULT 0.0f
#define OMX_STRIP_PARAM_COMP_ON_MIN 0.0f
#define OMX_STRIP_PARAM_COMP_ON_MAX 1.0f
#define OMX_STRIP_PARAM_COMP_ON_DEFAULT 0.0f
#define OMX_STRIP_PARAM_COMP_THRESHOLD_MIN -60.0f
#define OMX_STRIP_PARAM_COMP_THRESHOLD_MAX 0.0f
#define OMX_STRIP_PARAM_COMP_THRESHOLD_DEFAULT -18.0f
#define OMX_STRIP_PARAM_COMP_RATIO_MIN 1.0f
#define OMX_STRIP_PARAM_COMP_RATIO_MAX 20.0f
#define OMX_STRIP_PARAM_COMP_RATIO_DEFAULT 4.0f
#define OMX_STRIP_PARAM_COMP_KNEE_MIN 0.0f
#define OMX_STRIP_PARAM_COMP_KNEE_MAX 24.0f
#define OMX_STRIP_PARAM_COMP_KNEE_DEFAULT 6.0f
#define OMX_STRIP_PARAM_COMP_ATTACK_MIN 0.1f
#define OMX_STRIP_PARAM_COMP_ATTACK_MAX 100.0f
#define OMX_STRIP_PARAM_COMP_ATTACK_DEFAULT 5.0f
#define OMX_STRIP_PARAM_COMP_RELEASE_MIN 5.0f
#define OMX_STRIP_PARAM_COMP_RELEASE_MAX 3000.0f
#define OMX_STRIP_PARAM_COMP_RELEASE_DEFAULT 200.0f
#define OMX_STRIP_PARAM_COMP_MAKEUP_MIN 0.0f
#define OMX_STRIP_PARAM_COMP_MAKEUP_MAX 24.0f
#define OMX_STRIP_PARAM_COMP_MAKEUP_DEFAULT 0.0f
#define OMX_STRIP_PARAM_COMP_KIND_MIN 0.0f
#define OMX_STRIP_PARAM_COMP_KIND_MAX 1.0f
#define OMX_STRIP_PARAM_COMP_KIND_DEFAULT 0.0f
#define OMX_STRIP_PARAM_COMP_MIX_MIN 0.0f
#define OMX_STRIP_PARAM_COMP_MIX_MAX 100.0f
#define OMX_STRIP_PARAM_COMP_MIX_DEFAULT 100.0f
#define OMX_STRIP_PARAM_COMP_DETECTOR_OVERSAMPLING_MIN 0.0f
#define OMX_STRIP_PARAM_COMP_DETECTOR_OVERSAMPLING_MAX 2.0f
#define OMX_STRIP_PARAM_COMP_DETECTOR_OVERSAMPLING_DEFAULT 0.0f
#define OMX_STRIP_PARAM_ORDER_MIN 0.0f
#define OMX_STRIP_PARAM_ORDER_MAX 119.0f
#define OMX_STRIP_PARAM_ORDER_DEFAULT 0.0f

/* The identity every face publishes. */
#define OMX_STRIP_NAME "omx strip"
#define OMX_STRIP_VENDOR "openmixer"
#define OMX_STRIP_URL "https://github.com/FreeMixer/omx-plugins"
#define OMX_STRIP_VERSION "0.3.0"
#define OMX_STRIP_DESCRIPTION "The OpenMixer console's channel strip in one plugin: input trim, high- and low-pass filters, gate, four-band EQ and compressor, in the console's order or any other. No DSP of its own: each stage is omx-dsp's instance face of the console's kernel, called in order."
#define OMX_STRIP_CLAP_ID "org.openmixer.strip"
#define OMX_STRIP_CLAP_FEATURES "audio-effect", "mixing", "stereo"
#define OMX_STRIP_LV2_URI "urn:openmixer:strip"
/* org.openmixer.declaration/1: the declaration's plain description and the digest of its parameters. */
#define OMX_STRIP_DECL_SOURCE "omx strip: Input 0 to 1, Trim -24 to 24 dB, Filters 0 to 1, HPF 0 to 1, HPF Freq 20 to 1000 Hz, HPF Slope 12 to 24 dB/oct, LPF 0 to 1, LPF Freq 1000 to 20000 Hz, LPF Slope 12 to 24 dB/oct, Gate 0 to 1, Gate Threshold -80 to 0 dB, Gate Ratio 1 to 100, Gate Range -90 to 0 dB, Gate Attack 0 to 500 ms, Gate Release 0 to 5000 ms, Gate Knee Start -80 to 0 dB, Gate Knee End -80 to 0 dB, Gate Hold 0 to 2000 ms, Gate Hysteresis 0 to 24 dB, EQ 0 to 1, EQ 1 Type 0 to 5, EQ 1 Freq 20 to 20000 Hz, EQ 1 Gain -15 to 15 dB, EQ 1 Q 0.3 to 116, EQ 1 0 to 1, EQ 2 Type 0 to 5, EQ 2 Freq 20 to 20000 Hz, EQ 2 Gain -15 to 15 dB, EQ 2 Q 0.3 to 116, EQ 2 0 to 1, EQ 3 Type 0 to 5, EQ 3 Freq 20 to 20000 Hz, EQ 3 Gain -15 to 15 dB, EQ 3 Q 0.3 to 116, EQ 3 0 to 1, EQ 4 Type 0 to 5, EQ 4 Freq 20 to 20000 Hz, EQ 4 Gain -15 to 15 dB, EQ 4 Q 0.3 to 116, EQ 4 0 to 1, Comp 0 to 1, Comp Threshold -60 to 0 dB, Comp Ratio 1 to 20, Comp Knee 0 to 24 dB, Comp Attack 0.1 to 100 ms, Comp Release 5 to 3000 ms, Comp Makeup 0 to 24 dB, Comp Kind 0 to 1, Comp Mix 0 to 100 %, Comp Detector Oversampling 0 to 2, Order 0 to 119"
#define OMX_STRIP_DECL_DIGEST "7bc2f63aae815b41303644971f60fdc441e2c9407e093d98200c1c9e1ae0ea2d"

/* LV2 port indices (tools/gen.mjs lv2Ports): audio, then parameter i at FIRST_PARAM + i, then these. */
#define OMX_STRIP_LV2_PORT_IN_L 0u
#define OMX_STRIP_LV2_PORT_IN_R 1u
#define OMX_STRIP_LV2_PORT_OUT_L 2u
#define OMX_STRIP_LV2_PORT_OUT_R 3u
#define OMX_STRIP_LV2_PORT_ENABLED 55u
#define OMX_STRIP_LV2_PORT_LATENCY 56u
#define OMX_STRIP_LV2_PORT_FIRST_PARAM 4u
#define OMX_STRIP_LV2_PORT_COUNT 57u

#endif /* OMX_STRIP_PARAMS_H */
