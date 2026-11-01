// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#ifndef OMX_SPRING_PARAMS_H
#define OMX_SPRING_PARAMS_H
/*
 * GENERATED — DO NOT EDIT BY HAND.
 * Produced by tools/gen.mjs from plugins/omx-spring/omx-spring.decl.json.
 * Regenerate: `make -C plugins/omx-spring gen`, then commit the result. Order is append-only.
 * The include guard is omx-dsp's own <omxdsp/params/omx_spring_params.h>: included first, this
 * table is the one the kernel's instance header reads.
 */
#include <stddef.h>

#include <omxcontract/omx_resource_hints.h>

#include "omx_panel_description.h"
#include "omx_param_contract.h"
#include "omx_plugin_param.h"

enum {
  OMX_SPRING_PARAM_DECAY_S = 0,
  OMX_SPRING_PARAM_LENGTH_MS = 1,
  OMX_SPRING_PARAM_TENSION_PCT = 2,
  OMX_SPRING_PARAM_DAMP_HZ = 3,
  OMX_SPRING_PARAM_MIX_PCT = 4,
  OMX_SPRING_PARAM_COUNT = 5
};

static const omx_plugin_param OMX_SPRING_PARAMS[OMX_SPRING_PARAM_COUNT] = {
  { "decayS", "Decay", "s", 0.5f, 8.0f, 2.0f, 0u },
  { "lengthMs", "Length", "ms", 20.0f, 60.0f, 35.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "tensionPct", "Tension", "%", 0.0f, 100.0f, 50.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "dampHz", "Damping", "Hz", 1000.0f, 10000.0f, 4000.0f, OMX_PLUGIN_PARAM_INTEGER },
  { "mixPct", "Mix", "%", 0.0f, 100.0f, 0.0f, OMX_PLUGIN_PARAM_INTEGER },
};

/* Each parameter's CLAP module (clap_param_info.module): its descriptor control's group as a module path, "" for a
 * plugin with no descriptor. */
static const char *const OMX_SPRING_PARAM_MODULES[OMX_SPRING_PARAM_COUNT] = {
  "Spring",
  "Spring",
  "Spring",
  "Spring",
  "Spring",
};

/* One macro per declared bound: what a C face reads where a constant is needed. */
#define OMX_SPRING_PARAM_DECAY_S_MIN 0.5f
#define OMX_SPRING_PARAM_DECAY_S_MAX 8.0f
#define OMX_SPRING_PARAM_DECAY_S_DEFAULT 2.0f
#define OMX_SPRING_PARAM_LENGTH_MS_MIN 20.0f
#define OMX_SPRING_PARAM_LENGTH_MS_MAX 60.0f
#define OMX_SPRING_PARAM_LENGTH_MS_DEFAULT 35.0f
#define OMX_SPRING_PARAM_TENSION_PCT_MIN 0.0f
#define OMX_SPRING_PARAM_TENSION_PCT_MAX 100.0f
#define OMX_SPRING_PARAM_TENSION_PCT_DEFAULT 50.0f
#define OMX_SPRING_PARAM_DAMP_HZ_MIN 1000.0f
#define OMX_SPRING_PARAM_DAMP_HZ_MAX 10000.0f
#define OMX_SPRING_PARAM_DAMP_HZ_DEFAULT 4000.0f
#define OMX_SPRING_PARAM_MIX_PCT_MIN 0.0f
#define OMX_SPRING_PARAM_MIX_PCT_MAX 100.0f
#define OMX_SPRING_PARAM_MIX_PCT_DEFAULT 0.0f

/* The identity every face publishes. */
#define OMX_SPRING_NAME "omx spring"
#define OMX_SPRING_VENDOR "openmixer"
#define OMX_SPRING_URL "https://github.com/FreeMixer/omx-plugins"
#define OMX_SPRING_VERSION "0.17.0"
#define OMX_SPRING_DESCRIPTION "A spring reverb: a delay, a chain of all-passes and a darkening loop, with decay, length, tension and damping."
#define OMX_SPRING_CLAP_ID "org.openmixer.spring"
#define OMX_SPRING_CLAP_FEATURES "audio-effect", "reverb", "stereo"
#define OMX_SPRING_LV2_URI "urn:openmixer:spring"
/* org.openmixer.declaration/1: the declaration's plain description and the digest of its parameters. */
#define OMX_SPRING_DECL_SOURCE "omx spring: Decay 0.5 to 8 s, Length 20 to 60 ms, Tension 0 to 100 %, Damping 1000 to 10000 Hz, Mix 0 to 100 %"
#define OMX_SPRING_DECL_DIGEST "ae132ec37dfeaf6fb16508365b4898dcb7adaaa84715d27b8b1e6784cab73059"

/* org.openmixer.param-contract/1 (tools/param-contract.mjs): each parameter's unit, scale and step, in
 * id order; the host's bypass, the last id, is the face's. */
#define OMX_SPRING_PARAM_CONTRACT_VERSION 1u
#define OMX_SPRING_PARAM_CONTRACT_FINGERPRINT "d92f4dc985e249e557b7cd62fd8c612c268932f592740230f11a9ace230a8095"
static const struct omx_param_contract_row OMX_SPRING_PARAM_CONTRACT[OMX_SPRING_PARAM_COUNT] = {
  { 1u, OMX_CLAP_SCALE_LINEAR, 0.0 }, /* decayS s */
  { 2u, OMX_CLAP_SCALE_STEPPED, 1.0 }, /* lengthMs ms */
  { 14u, OMX_CLAP_SCALE_STEPPED, 1.0 }, /* tensionPct pc */
  { 16u, OMX_CLAP_SCALE_LOG, 1.0 }, /* dampHz hz */
  { 14u, OMX_CLAP_SCALE_STEPPED, 1.0 }, /* mixPct pc */
};

/* org.openmixer.resource-hints.draft/0 (omx-contract RESOURCE_HINTS, tools/resource-hints.mjs): the hints the
 * declaration's "resourceHints" states. A hint it leaves out has its bit of `declared` clear: unknown, never zero. */
static const omx_resource_hints_t OMX_SPRING_RESOURCE_HINTS = {
  .declared = 0u,
};

/* org.openmixer.panel-description/1 (tools/panel-description.mjs): the declaration's panel as the rows
 * include/omx_panel_description.h serves, parameter ids the declaration's. Family none. */
#define OMX_SPRING_PANEL_FINGERPRINT "2dcde81668fd45187faf1989f0dddb6c655546f96683a4d63e22f3d0b0fe2fa3"
static const struct omx_panel_section_row OMX_SPRING_PANEL_SECTIONS[] = {
  { "spring", "fx.spring.section.spring", "Spring" },
};
static const struct omx_panel_control_row OMX_SPRING_PANEL_CONTROLS[] = {
  { 0u, 0u, OMX_CLAP_WIDGET_KNOB, "fx.spring.decay_s", "Decay", OMX_CLAP_PANEL_INVALID_ID, 0x0u },
  { 1u, 0u, OMX_CLAP_WIDGET_KNOB, "fx.spring.length_ms", "Length", OMX_CLAP_PANEL_INVALID_ID, 0x0u },
  { 2u, 0u, OMX_CLAP_WIDGET_KNOB, "fx.spring.tension_pct", "Tension", OMX_CLAP_PANEL_INVALID_ID, 0x0u },
  { 3u, 0u, OMX_CLAP_WIDGET_KNOB, "fx.spring.damp_hz", "Damping", OMX_CLAP_PANEL_INVALID_ID, 0x0u },
  { 4u, 0u, OMX_CLAP_WIDGET_KNOB, "fx.spring.mix_pct", "Mix", OMX_CLAP_PANEL_INVALID_ID, 0x0u },
};
/* clap.remote-controls/2: the pages of the page rule over the sections. */
static const struct omx_panel_page_row OMX_SPRING_PANEL_PAGES[] = {
  { "Spring", "Spring", { 0u, 1u, 2u, 3u, 4u, OMX_CLAP_PANEL_INVALID_ID, OMX_CLAP_PANEL_INVALID_ID, OMX_CLAP_PANEL_INVALID_ID } },
};
#define OMX_SPRING_PANEL_PAGE_COUNT 1u
static const struct omx_panel_rows OMX_SPRING_PANEL = {
  OMX_SPRING_PANEL_FINGERPRINT, OMX_CLAP_FAMILY_NONE,
  OMX_SPRING_PANEL_SECTIONS, 1u,
  OMX_SPRING_PANEL_CONTROLS, 5u,
  NULL, 0u,
  NULL, 0u,
  NULL, 0u,
};

/* LV2 port indices (tools/gen.mjs lv2Ports): audio, then parameter i at FIRST_PARAM + i, then these. */
#define OMX_SPRING_LV2_PORT_IN_L 0u
#define OMX_SPRING_LV2_PORT_IN_R 1u
#define OMX_SPRING_LV2_PORT_OUT_L 2u
#define OMX_SPRING_LV2_PORT_OUT_R 3u
#define OMX_SPRING_LV2_PORT_ENABLED 9u
#define OMX_SPRING_LV2_PORT_LATENCY 10u
#define OMX_SPRING_LV2_PORT_FIRST_PARAM 4u
#define OMX_SPRING_LV2_PORT_COUNT 11u

#endif /* OMX_SPRING_PARAMS_H */
