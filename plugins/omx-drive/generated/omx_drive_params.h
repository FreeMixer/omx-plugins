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
  OMX_DRIVE_PARAM_DRIVE_DB = 0,
  OMX_DRIVE_PARAM_CHARACTER = 1,
  OMX_DRIVE_PARAM_BAND_HZ = 2,
  OMX_DRIVE_PARAM_MIX = 3,
  OMX_DRIVE_PARAM_TRIM_DB = 4,
  OMX_DRIVE_PARAM_COUNT = 5
};

static const omx_plugin_param OMX_DRIVE_PARAMS[OMX_DRIVE_PARAM_COUNT] = {
  { "driveDb", "Drive", "dB", 0.0f, 36.0f, 0.0f, 0u },
  { "character", "Character", "", -1.0f, 1.0f, 0.0f, 0u },
  { "bandHz", "Band", "Hz", 20.0f, 20000.0f, 2000.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "mix", "Mix", "%", 0.0f, 100.0f, 100.0f, 0u },
  { "trimDb", "Trim", "dB", -24.0f, 12.0f, 0.0f, 0u },
};

/* One macro per declared bound: what a C face reads where a constant is needed. */
#define OMX_DRIVE_PARAM_DRIVE_DB_MIN 0.0f
#define OMX_DRIVE_PARAM_DRIVE_DB_MAX 36.0f
#define OMX_DRIVE_PARAM_DRIVE_DB_DEFAULT 0.0f
#define OMX_DRIVE_PARAM_CHARACTER_MIN -1.0f
#define OMX_DRIVE_PARAM_CHARACTER_MAX 1.0f
#define OMX_DRIVE_PARAM_CHARACTER_DEFAULT 0.0f
#define OMX_DRIVE_PARAM_BAND_HZ_MIN 20.0f
#define OMX_DRIVE_PARAM_BAND_HZ_MAX 20000.0f
#define OMX_DRIVE_PARAM_BAND_HZ_DEFAULT 2000.0f
#define OMX_DRIVE_PARAM_MIX_MIN 0.0f
#define OMX_DRIVE_PARAM_MIX_MAX 100.0f
#define OMX_DRIVE_PARAM_MIX_DEFAULT 100.0f
#define OMX_DRIVE_PARAM_TRIM_DB_MIN -24.0f
#define OMX_DRIVE_PARAM_TRIM_DB_MAX 12.0f
#define OMX_DRIVE_PARAM_TRIM_DB_DEFAULT 0.0f

/* The identity every face publishes. */
#define OMX_DRIVE_NAME "omx drive"
#define OMX_DRIVE_VENDOR "openmixer"
#define OMX_DRIVE_URL "https://github.com/FreeMixer/omx-plugins"
#define OMX_DRIVE_VERSION "0.2.0"
#define OMX_DRIVE_DESCRIPTION "the native DRIVE stage: one waveshaper, run inside the console's ONE oversampler."
#define OMX_DRIVE_CLAP_ID "org.openmixer.drive"
#define OMX_DRIVE_CLAP_FEATURES "audio-effect", "stereo"
#define OMX_DRIVE_LV2_URI "urn:openmixer:drive"
/* org.openmixer.declaration/1: the declaration's plain description and the digest of its parameters. */
#define OMX_DRIVE_DECL_SOURCE "omx drive: Drive 0 to 36 dB, Character -1 to 1, Band 20 to 20000 Hz, Mix 0 to 100 %, Trim -24 to 12 dB"
#define OMX_DRIVE_DECL_DIGEST "f0931beb3084e2ef070de18ce082d1c72938674a9971d68ffc7508bb6775d1bf"

/* LV2 port indices (tools/gen.mjs lv2Ports): audio, then parameter i at FIRST_PARAM + i, then these. */
#define OMX_DRIVE_LV2_PORT_IN_L 0u
#define OMX_DRIVE_LV2_PORT_IN_R 1u
#define OMX_DRIVE_LV2_PORT_OUT_L 2u
#define OMX_DRIVE_LV2_PORT_OUT_R 3u
#define OMX_DRIVE_LV2_PORT_ENABLED 9u
#define OMX_DRIVE_LV2_PORT_LATENCY 10u
#define OMX_DRIVE_LV2_PORT_FIRST_PARAM 4u
#define OMX_DRIVE_LV2_PORT_COUNT 11u

#endif /* OMX_DRIVE_PARAMS_H */
