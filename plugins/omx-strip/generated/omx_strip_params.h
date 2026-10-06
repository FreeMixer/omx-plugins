// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#ifndef OMX_STRIP_PARAMS_H
#define OMX_STRIP_PARAMS_H
/*
 * GENERATED — DO NOT EDIT BY HAND.
 * Produced by tools/gen.mjs from plugins/omx-strip/omx-strip.decl.json (openmixer's
 * OMX_TRIM_RANGE_*_DB, OMX_HPF_FREQ_RANGE_*, OMX_LPF_FREQ_RANGE_*, OMX_GATE_*, OMX_EQ_*_RANGE_*, OMX_COMP_* (omx_contract_limits.h)).
 * Regenerate: `make -C plugins/omx-strip gen`, then commit the result. Order is append-only.
 * The include guard is omx-dsp's own <omxdsp/params/omx_strip_params.h>: included first, this
 * table is the one the kernel's instance header reads.
 */
#include "omx_plugin_param.h"

enum {
  OMX_STRIP_PARAM_TRIM_DB = 0,
  OMX_STRIP_PARAM_HPF_ON = 1,
  OMX_STRIP_PARAM_HPF_FREQ = 2,
  OMX_STRIP_PARAM_HPF_SLOPE = 3,
  OMX_STRIP_PARAM_LPF_ON = 4,
  OMX_STRIP_PARAM_LPF_FREQ = 5,
  OMX_STRIP_PARAM_LPF_SLOPE = 6,
  OMX_STRIP_PARAM_GATE_ON = 7,
  OMX_STRIP_PARAM_GATE_THRESHOLD = 8,
  OMX_STRIP_PARAM_GATE_RATIO = 9,
  OMX_STRIP_PARAM_GATE_RANGE = 10,
  OMX_STRIP_PARAM_GATE_ATTACK = 11,
  OMX_STRIP_PARAM_GATE_RELEASE = 12,
  OMX_STRIP_PARAM_EQ_ON = 13,
  OMX_STRIP_PARAM_EQ1_TYPE = 14,
  OMX_STRIP_PARAM_EQ1_FREQ = 15,
  OMX_STRIP_PARAM_EQ1_GAIN = 16,
  OMX_STRIP_PARAM_EQ1_Q = 17,
  OMX_STRIP_PARAM_EQ1_ON = 18,
  OMX_STRIP_PARAM_EQ2_TYPE = 19,
  OMX_STRIP_PARAM_EQ2_FREQ = 20,
  OMX_STRIP_PARAM_EQ2_GAIN = 21,
  OMX_STRIP_PARAM_EQ2_Q = 22,
  OMX_STRIP_PARAM_EQ2_ON = 23,
  OMX_STRIP_PARAM_EQ3_TYPE = 24,
  OMX_STRIP_PARAM_EQ3_FREQ = 25,
  OMX_STRIP_PARAM_EQ3_GAIN = 26,
  OMX_STRIP_PARAM_EQ3_Q = 27,
  OMX_STRIP_PARAM_EQ3_ON = 28,
  OMX_STRIP_PARAM_EQ4_TYPE = 29,
  OMX_STRIP_PARAM_EQ4_FREQ = 30,
  OMX_STRIP_PARAM_EQ4_GAIN = 31,
  OMX_STRIP_PARAM_EQ4_Q = 32,
  OMX_STRIP_PARAM_EQ4_ON = 33,
  OMX_STRIP_PARAM_COMP_ON = 34,
  OMX_STRIP_PARAM_COMP_THRESHOLD = 35,
  OMX_STRIP_PARAM_COMP_RATIO = 36,
  OMX_STRIP_PARAM_COMP_KNEE = 37,
  OMX_STRIP_PARAM_COMP_ATTACK = 38,
  OMX_STRIP_PARAM_COMP_RELEASE = 39,
  OMX_STRIP_PARAM_COMP_MAKEUP = 40,
  OMX_STRIP_PARAM_COMP_RMS = 41,
  OMX_STRIP_PARAM_ORDER = 42,
  OMX_STRIP_PARAM_COUNT = 43
};

static const omx_plugin_param OMX_STRIP_PARAMS[OMX_STRIP_PARAM_COUNT] = {
  { "trimDb", "Trim", "dB", -24.0f, 24.0f, 0.0f, 0u },
  { "hpfOn", "HPF", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "hpfFreq", "HPF Freq", "Hz", 20.0f, 1000.0f, 80.0f, 0u },
  { "hpfSlope", "HPF Slope", "dB/oct", 12.0f, 24.0f, 12.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "lpfOn", "LPF", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "lpfFreq", "LPF Freq", "Hz", 1000.0f, 20000.0f, 18000.0f, 0u },
  { "lpfSlope", "LPF Slope", "dB/oct", 12.0f, 24.0f, 12.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "gateOn", "Gate", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "gateThreshold", "Gate Threshold", "dB", -80.0f, 0.0f, -40.0f, 0u },
  { "gateRatio", "Gate Ratio", "", 1.0f, 100.0f, 16.0f, 0u },
  { "gateRange", "Gate Range", "dB", -90.0f, 0.0f, -90.0f, 0u },
  { "gateAttack", "Gate Attack", "ms", 0.0f, 500.0f, 1.0f, 0u },
  { "gateRelease", "Gate Release", "ms", 0.0f, 5000.0f, 100.0f, 0u },
  { "eqOn", "EQ", "", 0.0f, 1.0f, 1.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "eq1Type", "EQ 1 Type", "", 0.0f, 5.0f, 1.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "eq1Freq", "EQ 1 Freq", "Hz", 20.0f, 20000.0f, 100.0f, 0u },
  { "eq1Gain", "EQ 1 Gain", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "eq1Q", "EQ 1 Q", "", 0.3f, 8.0f, 1.0f, 0u },
  { "eq1On", "EQ 1", "", 0.0f, 1.0f, 1.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "eq2Type", "EQ 2 Type", "", 0.0f, 5.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "eq2Freq", "EQ 2 Freq", "Hz", 20.0f, 20000.0f, 400.0f, 0u },
  { "eq2Gain", "EQ 2 Gain", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "eq2Q", "EQ 2 Q", "", 0.3f, 8.0f, 1.0f, 0u },
  { "eq2On", "EQ 2", "", 0.0f, 1.0f, 1.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "eq3Type", "EQ 3 Type", "", 0.0f, 5.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "eq3Freq", "EQ 3 Freq", "Hz", 20.0f, 20000.0f, 2000.0f, 0u },
  { "eq3Gain", "EQ 3 Gain", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "eq3Q", "EQ 3 Q", "", 0.3f, 8.0f, 1.0f, 0u },
  { "eq3On", "EQ 3", "", 0.0f, 1.0f, 1.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "eq4Type", "EQ 4 Type", "", 0.0f, 5.0f, 2.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "eq4Freq", "EQ 4 Freq", "Hz", 20.0f, 20000.0f, 8000.0f, 0u },
  { "eq4Gain", "EQ 4 Gain", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "eq4Q", "EQ 4 Q", "", 0.3f, 8.0f, 1.0f, 0u },
  { "eq4On", "EQ 4", "", 0.0f, 1.0f, 1.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "compOn", "Comp", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "compThreshold", "Comp Threshold", "dB", -60.0f, 0.0f, -18.0f, 0u },
  { "compRatio", "Comp Ratio", "", 1.0f, 20.0f, 4.0f, 0u },
  { "compKnee", "Comp Knee", "dB", 0.0f, 24.0f, 6.0f, 0u },
  { "compAttack", "Comp Attack", "ms", 0.1f, 100.0f, 5.0f, 0u },
  { "compRelease", "Comp Release", "ms", 5.0f, 3000.0f, 200.0f, 0u },
  { "compMakeup", "Comp Makeup", "dB", 0.0f, 24.0f, 0.0f, 0u },
  { "compRms", "Comp RMS", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
  { "order", "Order", "", 0.0f, 23.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
};

/* One macro per declared bound: what a C face reads where a constant is needed. */
#define OMX_STRIP_PARAM_TRIM_DB_MIN -24.0f
#define OMX_STRIP_PARAM_TRIM_DB_MAX 24.0f
#define OMX_STRIP_PARAM_TRIM_DB_DEFAULT 0.0f
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
#define OMX_STRIP_PARAM_EQ1_Q_MAX 8.0f
#define OMX_STRIP_PARAM_EQ1_Q_DEFAULT 1.0f
#define OMX_STRIP_PARAM_EQ1_ON_MIN 0.0f
#define OMX_STRIP_PARAM_EQ1_ON_MAX 1.0f
#define OMX_STRIP_PARAM_EQ1_ON_DEFAULT 1.0f
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
#define OMX_STRIP_PARAM_EQ2_Q_MAX 8.0f
#define OMX_STRIP_PARAM_EQ2_Q_DEFAULT 1.0f
#define OMX_STRIP_PARAM_EQ2_ON_MIN 0.0f
#define OMX_STRIP_PARAM_EQ2_ON_MAX 1.0f
#define OMX_STRIP_PARAM_EQ2_ON_DEFAULT 1.0f
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
#define OMX_STRIP_PARAM_EQ3_Q_MAX 8.0f
#define OMX_STRIP_PARAM_EQ3_Q_DEFAULT 1.0f
#define OMX_STRIP_PARAM_EQ3_ON_MIN 0.0f
#define OMX_STRIP_PARAM_EQ3_ON_MAX 1.0f
#define OMX_STRIP_PARAM_EQ3_ON_DEFAULT 1.0f
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
#define OMX_STRIP_PARAM_EQ4_Q_MAX 8.0f
#define OMX_STRIP_PARAM_EQ4_Q_DEFAULT 1.0f
#define OMX_STRIP_PARAM_EQ4_ON_MIN 0.0f
#define OMX_STRIP_PARAM_EQ4_ON_MAX 1.0f
#define OMX_STRIP_PARAM_EQ4_ON_DEFAULT 1.0f
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
#define OMX_STRIP_PARAM_COMP_RMS_MIN 0.0f
#define OMX_STRIP_PARAM_COMP_RMS_MAX 1.0f
#define OMX_STRIP_PARAM_COMP_RMS_DEFAULT 0.0f
#define OMX_STRIP_PARAM_ORDER_MIN 0.0f
#define OMX_STRIP_PARAM_ORDER_MAX 23.0f
#define OMX_STRIP_PARAM_ORDER_DEFAULT 0.0f

/* The identity every face publishes. */
#define OMX_STRIP_NAME "omx strip"
#define OMX_STRIP_VENDOR "openmixer"
#define OMX_STRIP_URL "https://github.com/FreeMixer/omx-plugins"
#define OMX_STRIP_VERSION "0.1.0"
#define OMX_STRIP_DESCRIPTION "openmixer’s channel strip in one plugin: input trim with high- and low-pass filters, gate, four-band EQ and compressor, in the console’s default order or any other. No DSP of its own: each stage is omx-dsp’s strip module, called in order."
#define OMX_STRIP_CLAP_ID "org.openmixer.strip"
#define OMX_STRIP_CLAP_FEATURES "audio-effect", "mixing", "stereo"
#define OMX_STRIP_LV2_URI "urn:openmixer:strip"
/* org.openmixer.declaration/1: the source expression and the digest of the resolved parameters. */
#define OMX_STRIP_DECL_SOURCE "OMX_TRIM_RANGE_*_DB, OMX_HPF_FREQ_RANGE_*, OMX_LPF_FREQ_RANGE_*, OMX_GATE_*, OMX_EQ_*_RANGE_*, OMX_COMP_* (omx_contract_limits.h)"
#define OMX_STRIP_DECL_DIGEST "f0520137037652deb3de4a92c4a067573585bdd0bebdac8e477b742560b3a91b"

/* LV2 port indices (tools/gen.mjs lv2Ports): audio, then parameter i at FIRST_PARAM + i, then these. */
#define OMX_STRIP_LV2_PORT_IN_L 0u
#define OMX_STRIP_LV2_PORT_IN_R 1u
#define OMX_STRIP_LV2_PORT_OUT_L 2u
#define OMX_STRIP_LV2_PORT_OUT_R 3u
#define OMX_STRIP_LV2_PORT_ENABLED 47u
#define OMX_STRIP_LV2_PORT_LATENCY 48u
#define OMX_STRIP_LV2_PORT_FIRST_PARAM 4u
#define OMX_STRIP_LV2_PORT_COUNT 49u

#endif /* OMX_STRIP_PARAMS_H */
