// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#ifndef OMX_GEQ_PARAMS_H
#define OMX_GEQ_PARAMS_H
/*
 * GENERATED — DO NOT EDIT BY HAND.
 * Produced by tools/gen.mjs from plugins/omx-geq/omx-geq.decl.json.
 * Regenerate: `make -C plugins/omx-geq gen`, then commit the result. Order is append-only.
 * The include guard is omx-dsp's own <omxdsp/params/omx_geq_params.h>: included first, this
 * table is the one the kernel's instance header reads.
 */
#include "omx_plugin_param.h"

enum {
  OMX_GEQ_PARAM_BAND01 = 0,
  OMX_GEQ_PARAM_BAND02 = 1,
  OMX_GEQ_PARAM_BAND03 = 2,
  OMX_GEQ_PARAM_BAND04 = 3,
  OMX_GEQ_PARAM_BAND05 = 4,
  OMX_GEQ_PARAM_BAND06 = 5,
  OMX_GEQ_PARAM_BAND07 = 6,
  OMX_GEQ_PARAM_BAND08 = 7,
  OMX_GEQ_PARAM_BAND09 = 8,
  OMX_GEQ_PARAM_BAND10 = 9,
  OMX_GEQ_PARAM_BAND11 = 10,
  OMX_GEQ_PARAM_BAND12 = 11,
  OMX_GEQ_PARAM_BAND13 = 12,
  OMX_GEQ_PARAM_BAND14 = 13,
  OMX_GEQ_PARAM_BAND15 = 14,
  OMX_GEQ_PARAM_BAND16 = 15,
  OMX_GEQ_PARAM_BAND17 = 16,
  OMX_GEQ_PARAM_BAND18 = 17,
  OMX_GEQ_PARAM_BAND19 = 18,
  OMX_GEQ_PARAM_BAND20 = 19,
  OMX_GEQ_PARAM_BAND21 = 20,
  OMX_GEQ_PARAM_BAND22 = 21,
  OMX_GEQ_PARAM_BAND23 = 22,
  OMX_GEQ_PARAM_BAND24 = 23,
  OMX_GEQ_PARAM_BAND25 = 24,
  OMX_GEQ_PARAM_BAND26 = 25,
  OMX_GEQ_PARAM_BAND27 = 26,
  OMX_GEQ_PARAM_BAND28 = 27,
  OMX_GEQ_PARAM_BAND29 = 28,
  OMX_GEQ_PARAM_BAND30 = 29,
  OMX_GEQ_PARAM_BAND31 = 30,
  OMX_GEQ_PARAM_COUNT = 31
};

static const omx_plugin_param OMX_GEQ_PARAMS[OMX_GEQ_PARAM_COUNT] = {
  { "band01", "20 Hz", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "band02", "25 Hz", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "band03", "31.5 Hz", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "band04", "40 Hz", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "band05", "50 Hz", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "band06", "63 Hz", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "band07", "80 Hz", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "band08", "100 Hz", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "band09", "125 Hz", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "band10", "160 Hz", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "band11", "200 Hz", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "band12", "250 Hz", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "band13", "315 Hz", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "band14", "400 Hz", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "band15", "500 Hz", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "band16", "630 Hz", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "band17", "800 Hz", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "band18", "1 kHz", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "band19", "1.25 kHz", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "band20", "1.6 kHz", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "band21", "2 kHz", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "band22", "2.5 kHz", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "band23", "3.15 kHz", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "band24", "4 kHz", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "band25", "5 kHz", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "band26", "6.3 kHz", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "band27", "8 kHz", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "band28", "10 kHz", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "band29", "12.5 kHz", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "band30", "16 kHz", "dB", -15.0f, 15.0f, 0.0f, 0u },
  { "band31", "20 kHz", "dB", -15.0f, 15.0f, 0.0f, 0u },
};

/* One macro per declared bound: what a C face reads where a constant is needed. */
#define OMX_GEQ_PARAM_BAND01_MIN -15.0f
#define OMX_GEQ_PARAM_BAND01_MAX 15.0f
#define OMX_GEQ_PARAM_BAND01_DEFAULT 0.0f
#define OMX_GEQ_PARAM_BAND02_MIN -15.0f
#define OMX_GEQ_PARAM_BAND02_MAX 15.0f
#define OMX_GEQ_PARAM_BAND02_DEFAULT 0.0f
#define OMX_GEQ_PARAM_BAND03_MIN -15.0f
#define OMX_GEQ_PARAM_BAND03_MAX 15.0f
#define OMX_GEQ_PARAM_BAND03_DEFAULT 0.0f
#define OMX_GEQ_PARAM_BAND04_MIN -15.0f
#define OMX_GEQ_PARAM_BAND04_MAX 15.0f
#define OMX_GEQ_PARAM_BAND04_DEFAULT 0.0f
#define OMX_GEQ_PARAM_BAND05_MIN -15.0f
#define OMX_GEQ_PARAM_BAND05_MAX 15.0f
#define OMX_GEQ_PARAM_BAND05_DEFAULT 0.0f
#define OMX_GEQ_PARAM_BAND06_MIN -15.0f
#define OMX_GEQ_PARAM_BAND06_MAX 15.0f
#define OMX_GEQ_PARAM_BAND06_DEFAULT 0.0f
#define OMX_GEQ_PARAM_BAND07_MIN -15.0f
#define OMX_GEQ_PARAM_BAND07_MAX 15.0f
#define OMX_GEQ_PARAM_BAND07_DEFAULT 0.0f
#define OMX_GEQ_PARAM_BAND08_MIN -15.0f
#define OMX_GEQ_PARAM_BAND08_MAX 15.0f
#define OMX_GEQ_PARAM_BAND08_DEFAULT 0.0f
#define OMX_GEQ_PARAM_BAND09_MIN -15.0f
#define OMX_GEQ_PARAM_BAND09_MAX 15.0f
#define OMX_GEQ_PARAM_BAND09_DEFAULT 0.0f
#define OMX_GEQ_PARAM_BAND10_MIN -15.0f
#define OMX_GEQ_PARAM_BAND10_MAX 15.0f
#define OMX_GEQ_PARAM_BAND10_DEFAULT 0.0f
#define OMX_GEQ_PARAM_BAND11_MIN -15.0f
#define OMX_GEQ_PARAM_BAND11_MAX 15.0f
#define OMX_GEQ_PARAM_BAND11_DEFAULT 0.0f
#define OMX_GEQ_PARAM_BAND12_MIN -15.0f
#define OMX_GEQ_PARAM_BAND12_MAX 15.0f
#define OMX_GEQ_PARAM_BAND12_DEFAULT 0.0f
#define OMX_GEQ_PARAM_BAND13_MIN -15.0f
#define OMX_GEQ_PARAM_BAND13_MAX 15.0f
#define OMX_GEQ_PARAM_BAND13_DEFAULT 0.0f
#define OMX_GEQ_PARAM_BAND14_MIN -15.0f
#define OMX_GEQ_PARAM_BAND14_MAX 15.0f
#define OMX_GEQ_PARAM_BAND14_DEFAULT 0.0f
#define OMX_GEQ_PARAM_BAND15_MIN -15.0f
#define OMX_GEQ_PARAM_BAND15_MAX 15.0f
#define OMX_GEQ_PARAM_BAND15_DEFAULT 0.0f
#define OMX_GEQ_PARAM_BAND16_MIN -15.0f
#define OMX_GEQ_PARAM_BAND16_MAX 15.0f
#define OMX_GEQ_PARAM_BAND16_DEFAULT 0.0f
#define OMX_GEQ_PARAM_BAND17_MIN -15.0f
#define OMX_GEQ_PARAM_BAND17_MAX 15.0f
#define OMX_GEQ_PARAM_BAND17_DEFAULT 0.0f
#define OMX_GEQ_PARAM_BAND18_MIN -15.0f
#define OMX_GEQ_PARAM_BAND18_MAX 15.0f
#define OMX_GEQ_PARAM_BAND18_DEFAULT 0.0f
#define OMX_GEQ_PARAM_BAND19_MIN -15.0f
#define OMX_GEQ_PARAM_BAND19_MAX 15.0f
#define OMX_GEQ_PARAM_BAND19_DEFAULT 0.0f
#define OMX_GEQ_PARAM_BAND20_MIN -15.0f
#define OMX_GEQ_PARAM_BAND20_MAX 15.0f
#define OMX_GEQ_PARAM_BAND20_DEFAULT 0.0f
#define OMX_GEQ_PARAM_BAND21_MIN -15.0f
#define OMX_GEQ_PARAM_BAND21_MAX 15.0f
#define OMX_GEQ_PARAM_BAND21_DEFAULT 0.0f
#define OMX_GEQ_PARAM_BAND22_MIN -15.0f
#define OMX_GEQ_PARAM_BAND22_MAX 15.0f
#define OMX_GEQ_PARAM_BAND22_DEFAULT 0.0f
#define OMX_GEQ_PARAM_BAND23_MIN -15.0f
#define OMX_GEQ_PARAM_BAND23_MAX 15.0f
#define OMX_GEQ_PARAM_BAND23_DEFAULT 0.0f
#define OMX_GEQ_PARAM_BAND24_MIN -15.0f
#define OMX_GEQ_PARAM_BAND24_MAX 15.0f
#define OMX_GEQ_PARAM_BAND24_DEFAULT 0.0f
#define OMX_GEQ_PARAM_BAND25_MIN -15.0f
#define OMX_GEQ_PARAM_BAND25_MAX 15.0f
#define OMX_GEQ_PARAM_BAND25_DEFAULT 0.0f
#define OMX_GEQ_PARAM_BAND26_MIN -15.0f
#define OMX_GEQ_PARAM_BAND26_MAX 15.0f
#define OMX_GEQ_PARAM_BAND26_DEFAULT 0.0f
#define OMX_GEQ_PARAM_BAND27_MIN -15.0f
#define OMX_GEQ_PARAM_BAND27_MAX 15.0f
#define OMX_GEQ_PARAM_BAND27_DEFAULT 0.0f
#define OMX_GEQ_PARAM_BAND28_MIN -15.0f
#define OMX_GEQ_PARAM_BAND28_MAX 15.0f
#define OMX_GEQ_PARAM_BAND28_DEFAULT 0.0f
#define OMX_GEQ_PARAM_BAND29_MIN -15.0f
#define OMX_GEQ_PARAM_BAND29_MAX 15.0f
#define OMX_GEQ_PARAM_BAND29_DEFAULT 0.0f
#define OMX_GEQ_PARAM_BAND30_MIN -15.0f
#define OMX_GEQ_PARAM_BAND30_MAX 15.0f
#define OMX_GEQ_PARAM_BAND30_DEFAULT 0.0f
#define OMX_GEQ_PARAM_BAND31_MIN -15.0f
#define OMX_GEQ_PARAM_BAND31_MAX 15.0f
#define OMX_GEQ_PARAM_BAND31_DEFAULT 0.0f

/* The identity every face publishes. */
#define OMX_GEQ_NAME "omx geq"
#define OMX_GEQ_VENDOR "openmixer"
#define OMX_GEQ_URL "https://github.com/FreeMixer/omx-plugins"
#define OMX_GEQ_VERSION "0.2.0"
#define OMX_GEQ_DESCRIPTION "The console's 31-band graphic EQ: one fader per ISO third-octave band from 20 Hz to 20 kHz, each cutting or boosting by up to 15 dB."
#define OMX_GEQ_CLAP_ID "org.openmixer.geq"
#define OMX_GEQ_CLAP_FEATURES "audio-effect", "equalizer", "stereo"
#define OMX_GEQ_LV2_URI "urn:openmixer:geq"
/* org.openmixer.declaration/1: the declaration's plain description and the digest of its parameters. */
#define OMX_GEQ_DECL_SOURCE "omx geq: 20 Hz -15 to 15 dB, 25 Hz -15 to 15 dB, 31.5 Hz -15 to 15 dB, 40 Hz -15 to 15 dB, 50 Hz -15 to 15 dB, 63 Hz -15 to 15 dB, 80 Hz -15 to 15 dB, 100 Hz -15 to 15 dB, 125 Hz -15 to 15 dB, 160 Hz -15 to 15 dB, 200 Hz -15 to 15 dB, 250 Hz -15 to 15 dB, 315 Hz -15 to 15 dB, 400 Hz -15 to 15 dB, 500 Hz -15 to 15 dB, 630 Hz -15 to 15 dB, 800 Hz -15 to 15 dB, 1 kHz -15 to 15 dB, 1.25 kHz -15 to 15 dB, 1.6 kHz -15 to 15 dB, 2 kHz -15 to 15 dB, 2.5 kHz -15 to 15 dB, 3.15 kHz -15 to 15 dB, 4 kHz -15 to 15 dB, 5 kHz -15 to 15 dB, 6.3 kHz -15 to 15 dB, 8 kHz -15 to 15 dB, 10 kHz -15 to 15 dB, 12.5 kHz -15 to 15 dB, 16 kHz -15 to 15 dB, 20 kHz -15 to 15 dB"
#define OMX_GEQ_DECL_DIGEST "d0a6a22360a6ce7914f59d4b3eb245c16250f1a772ad308a64bbe05698fe2f48"

/* LV2 port indices (tools/gen.mjs lv2Ports): audio, then parameter i at FIRST_PARAM + i, then these. */
#define OMX_GEQ_LV2_PORT_IN_L 0u
#define OMX_GEQ_LV2_PORT_IN_R 1u
#define OMX_GEQ_LV2_PORT_OUT_L 2u
#define OMX_GEQ_LV2_PORT_OUT_R 3u
#define OMX_GEQ_LV2_PORT_ENABLED 35u
#define OMX_GEQ_LV2_PORT_LATENCY 36u
#define OMX_GEQ_LV2_PORT_FIRST_PARAM 4u
#define OMX_GEQ_LV2_PORT_COUNT 37u

#endif /* OMX_GEQ_PARAMS_H */
