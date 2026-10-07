// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * omx_eq32_lv2.c — the LV2 face of omx-eq32, in plain C: the same declaration as the CLAP face
 * (generated/omx_eq32_params.h, whose LV2 port indices are the ones tools/gen.mjs writes into the
 * bundle's TTL), the same two-leg core through omx_eq32_stereo.h, the same callback: flush denormals,
 * gather the declared values, run.
 *
 * Ports: in_l, in_r, out_l, out_r, then every declared parameter in declaration order, then
 * `enabled` (lv2:enabled; 0 is the bypass) and `latency` (always 0: a biquad cascade has no delay
 * line). A parameter port the host left unconnected reads its declared default.
 */
#include <stdint.h>
#include <stdlib.h>

#include <lv2/core/lv2.h>

#include "omx_eq32_stereo.h"
#include <omxdsp/omx_denormal.h>

typedef struct {
  OmxEq32 core;
  const float *in_l, *in_r;
  float *out_l, *out_r;
  const float *param[OMX_EQ32_PARAM_COUNT];
  const float *enabled;
  float *latency;
} Eq32;

static LV2_Handle instantiate(const LV2_Descriptor *d, double rate, const char *bundle,
                              const LV2_Feature *const *features) {
  (void)d;
  (void)bundle;
  (void)features;
  if (!(rate > 0.0)) return NULL;
  Eq32 *s = (Eq32 *)calloc(1, sizeof *s);
  if (s) omx_eq32_init(&s->core, rate);
  return s;
}

static void connect_port(LV2_Handle h, uint32_t port, void *data) {
  Eq32 *s = (Eq32 *)h;
  switch (port) {
  case OMX_EQ32_LV2_PORT_IN_L: s->in_l = (const float *)data; return;
  case OMX_EQ32_LV2_PORT_IN_R: s->in_r = (const float *)data; return;
  case OMX_EQ32_LV2_PORT_OUT_L: s->out_l = (float *)data; return;
  case OMX_EQ32_LV2_PORT_OUT_R: s->out_r = (float *)data; return;
  case OMX_EQ32_LV2_PORT_ENABLED: s->enabled = (const float *)data; return;
  case OMX_EQ32_LV2_PORT_LATENCY: s->latency = (float *)data; return;
  default:
    if (port >= OMX_EQ32_LV2_PORT_FIRST_PARAM && port < OMX_EQ32_LV2_PORT_FIRST_PARAM + OMX_EQ32_PARAM_COUNT)
      s->param[port - OMX_EQ32_LV2_PORT_FIRST_PARAM] = (const float *)data;
  }
}

static void activate(LV2_Handle h) { omx_eq32_reset(&((Eq32 *)h)->core); }

static void run(LV2_Handle h, uint32_t frames) {
  Eq32 *s = (Eq32 *)h;
  omx_denormals_off();
  if (s->latency) *s->latency = OMX_EQ_LV2_LATENCY_FRAMES;
  if (!s->in_l || !s->in_r || !s->out_l || !s->out_r) return;
  float v[OMX_EQ32_PARAM_COUNT];
  for (uint32_t i = 0; i < OMX_EQ32_PARAM_COUNT; i++) v[i] = s->param[i] ? *s->param[i] : OMX_EQ32_PARAMS[i].def;
  omx_eq32_run(&s->core, v, s->enabled && *s->enabled < 0.5f, s->in_l, s->in_r, s->out_l, s->out_r, frames);
}

static void cleanup(LV2_Handle h) { free(h); }

static const void *extension_data(const char *uri) {
  (void)uri;
  return NULL;
}

static const LV2_Descriptor DESCRIPTOR = {OMX_EQ32_LV2_URI, instantiate, connect_port, activate, run, NULL, cleanup,
                                          extension_data};

LV2_SYMBOL_EXPORT const LV2_Descriptor *lv2_descriptor(uint32_t index) { return index == 0 ? &DESCRIPTOR : NULL; }
