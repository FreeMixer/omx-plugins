// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#ifndef OMX_DELAY_PARAMS_H
#define OMX_DELAY_PARAMS_H
/*
 * GENERATED — DO NOT EDIT BY HAND.
 * Produced by tools/gen.mjs from plugins/omx-delay/omx-delay.decl.json.
 * Regenerate: `make -C plugins/omx-delay gen`, then commit the result. Order is append-only.
 * The include guard is omx-dsp's own <omxdsp/params/omx_delay_params.h>: included first, this
 * table is the one the kernel's instance header reads.
 */
#include <stddef.h>

#include "omx_panel_description.h"
#include "omx_plugin_param.h"

enum {
  OMX_DELAY_PARAM_FX_DELAY_TIME = 0,
  OMX_DELAY_PARAM_FX_DELAY_FEEDBACK = 1,
  OMX_DELAY_PARAM_TONE = 2,
  OMX_DELAY_PARAM_MIX = 3,
  OMX_DELAY_PARAM_PINGPONG = 4,
  OMX_DELAY_PARAM_COUNT = 5
};

static const omx_plugin_param OMX_DELAY_PARAMS[OMX_DELAY_PARAM_COUNT] = {
  { "fxDelayTime", "Time", "ms", 0.0f, 2000.0f, 300.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "fxDelayFeedback", "Feedback", "", 0.0f, 0.99f, 0.3f, 0u },
  { "tone", "Tone", "", 0.0f, 1.0f, 0.3f, 0u },
  { "mix", "Mix", "", 0.0f, 1.0f, 0.3f, 0u },
  { "pingpong", "Pingpong", "", 0.0f, 1.0f, 0.0f, OMX_PLUGIN_PARAM_TOGGLE },
};

/* One macro per declared bound: what a C face reads where a constant is needed. */
#define OMX_DELAY_PARAM_FX_DELAY_TIME_MIN 0.0f
#define OMX_DELAY_PARAM_FX_DELAY_TIME_MAX 2000.0f
#define OMX_DELAY_PARAM_FX_DELAY_TIME_DEFAULT 300.0f
#define OMX_DELAY_PARAM_FX_DELAY_FEEDBACK_MIN 0.0f
#define OMX_DELAY_PARAM_FX_DELAY_FEEDBACK_MAX 0.99f
#define OMX_DELAY_PARAM_FX_DELAY_FEEDBACK_DEFAULT 0.3f
#define OMX_DELAY_PARAM_TONE_MIN 0.0f
#define OMX_DELAY_PARAM_TONE_MAX 1.0f
#define OMX_DELAY_PARAM_TONE_DEFAULT 0.3f
#define OMX_DELAY_PARAM_MIX_MIN 0.0f
#define OMX_DELAY_PARAM_MIX_MAX 1.0f
#define OMX_DELAY_PARAM_MIX_DEFAULT 0.3f
#define OMX_DELAY_PARAM_PINGPONG_MIN 0.0f
#define OMX_DELAY_PARAM_PINGPONG_MAX 1.0f
#define OMX_DELAY_PARAM_PINGPONG_DEFAULT 0.0f

/* The identity every face publishes. */
#define OMX_DELAY_NAME "omx delay"
#define OMX_DELAY_VENDOR "openmixer"
#define OMX_DELAY_URL "https://github.com/FreeMixer/omx-plugins"
#define OMX_DELAY_VERSION "0.4.0"
#define OMX_DELAY_DESCRIPTION "The OpenMixer console's stereo delay: one time, feedback through a tone filter, ping-pong and a wet/dry mix."
#define OMX_DELAY_CLAP_ID "org.openmixer.delay"
#define OMX_DELAY_CLAP_FEATURES "audio-effect", "delay", "stereo"
#define OMX_DELAY_LV2_URI "urn:openmixer:delay"
/* org.openmixer.declaration/1: the declaration's plain description and the digest of its parameters. */
#define OMX_DELAY_DECL_SOURCE "omx delay: Time 0 to 2000 ms, Feedback 0 to 0.99, Tone 0 to 1, Mix 0 to 1, Pingpong 0 to 1"
#define OMX_DELAY_DECL_DIGEST "b4d6be38d03fd4d086d878edf93bcbc5f6d678cbbbb742ddf597e3b7f160cafd"

/* org.openmixer.panel-description/1 (tools/panel-description.mjs): the declaration's panel as the rows
 * include/omx_panel_description.h serves, parameter ids the declaration's. Family delay. */
#define OMX_DELAY_PANEL_FINGERPRINT "b229558e6b54e1b55a0ebdc5d0204c0ce8e3ed6f916c29ae00730150dd75d32d"
static const struct omx_panel_section_row OMX_DELAY_PANEL_SECTIONS[] = {
  { "delay", "fx.delay.section.delay", "Delay" },
};
static const struct omx_panel_control_row OMX_DELAY_PANEL_CONTROLS[] = {
  { 4u, 0u, OMX_CLAP_WIDGET_SWITCH, "fx.delay.pingpong", "Pingpong", OMX_CLAP_PANEL_INVALID_ID, 0x0u },
  { 0u, 0u, OMX_CLAP_WIDGET_KNOB, "fx.delay.fx_delay_time", "Time", OMX_CLAP_PANEL_INVALID_ID, 0x0u },
  { 1u, 0u, OMX_CLAP_WIDGET_KNOB, "fx.delay.fx_delay_feedback", "Feedback", OMX_CLAP_PANEL_INVALID_ID, 0x0u },
  { 3u, 0u, OMX_CLAP_WIDGET_KNOB, "fx.delay.mix", "Mix", OMX_CLAP_PANEL_INVALID_ID, 0x0u },
  { 2u, 0u, OMX_CLAP_WIDGET_KNOB, "fx.delay.tone", "Tone", OMX_CLAP_PANEL_INVALID_ID, 0x0u },
};
static const struct omx_panel_role_row OMX_DELAY_PANEL_ROLES[] = {
  { "time", 0u, 0u },
  { "feedback", 0u, 1u },
  { "tone", 0u, 2u },
  { "pingpong", 0u, 4u },
  { "mix", 0u, 3u },
};
static const struct omx_panel_rows OMX_DELAY_PANEL = {
  OMX_DELAY_PANEL_FINGERPRINT, OMX_CLAP_FAMILY_DELAY,
  OMX_DELAY_PANEL_SECTIONS, 1u,
  OMX_DELAY_PANEL_CONTROLS, 5u,
  OMX_DELAY_PANEL_ROLES, 5u,
  NULL, 0u,
  NULL, 0u,
};

/* LV2 port indices (tools/gen.mjs lv2Ports): audio, then parameter i at FIRST_PARAM + i, then these. */
#define OMX_DELAY_LV2_PORT_IN_L 0u
#define OMX_DELAY_LV2_PORT_IN_R 1u
#define OMX_DELAY_LV2_PORT_OUT_L 2u
#define OMX_DELAY_LV2_PORT_OUT_R 3u
#define OMX_DELAY_LV2_PORT_ENABLED 9u
#define OMX_DELAY_LV2_PORT_LATENCY 10u
#define OMX_DELAY_LV2_PORT_FIRST_PARAM 4u
#define OMX_DELAY_LV2_PORT_COUNT 11u

#endif /* OMX_DELAY_PARAMS_H */
