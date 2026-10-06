// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * omx_strip_lv2.c — the LV2 face of omx-strip, in plain C: the same declaration as the CLAP face
 * (generated/omx_strip_params.h, whose LV2 port indices are the ones tools/gen.mjs writes into the
 * bundle's TTL), the same core (omx_strip.h), the same callback: flush denormals, resolve, run.
 *
 * Ports: in_l, in_r, out_l, out_r, then every declared parameter in declaration order, then
 * `enabled` (lv2:enabled; 0 is the bypass) and `latency` (the gate's plus the compressor's).
 * A parameter port the host left unconnected reads its declared default.
 */
#include <stdint.h>
#include <stdlib.h>

#include <lv2/core/lv2.h>

#include "omx_strip.h"
#include <omxdsp/omx_denormal.h>

typedef struct {
  OmxStrip core;
  float sr;
  const float *in_l, *in_r;
  float *out_l, *out_r;
  const float *param[OMX_STRIP_PARAM_COUNT];
  const float *enabled;
  float *latency;
} Strip;

static LV2_Handle instantiate(const LV2_Descriptor *d, double rate, const char *bundle,
                              const LV2_Feature *const *features) {
  (void)d;
  (void)bundle;
  (void)features;
  if (!(rate > 0.0)) return NULL;
  Strip *s = (Strip *)calloc(1, sizeof *s);
  if (!s) return NULL;
  s->sr = (float)rate;
  if (!omx_strip_init(&s->core, s->sr)) {
    free(s);
    return NULL;
  }
  return s;
}

static void connect_port(LV2_Handle h, uint32_t port, void *data) {
  Strip *s = (Strip *)h;
  switch (port) {
  case OMX_STRIP_LV2_PORT_IN_L: s->in_l = (const float *)data; return;
  case OMX_STRIP_LV2_PORT_IN_R: s->in_r = (const float *)data; return;
  case OMX_STRIP_LV2_PORT_OUT_L: s->out_l = (float *)data; return;
  case OMX_STRIP_LV2_PORT_OUT_R: s->out_r = (float *)data; return;
  case OMX_STRIP_LV2_PORT_ENABLED: s->enabled = (const float *)data; return;
  case OMX_STRIP_LV2_PORT_LATENCY: s->latency = (float *)data; return;
  default:
    if (port >= OMX_STRIP_LV2_PORT_FIRST_PARAM && port < OMX_STRIP_LV2_PORT_FIRST_PARAM + OMX_STRIP_PARAM_COUNT)
      s->param[port - OMX_STRIP_LV2_PORT_FIRST_PARAM] = (const float *)data;
  }
}

static void activate(LV2_Handle h) {
  Strip *s = (Strip *)h;
  omx_strip_init(&s->core, s->sr);
}

static void run(LV2_Handle h, uint32_t frames) {
  Strip *s = (Strip *)h;
  omx_denormals_off();
  float v[OMX_STRIP_PARAM_COUNT];
  for (uint32_t i = 0; i < OMX_STRIP_PARAM_COUNT; i++) v[i] = s->param[i] ? *s->param[i] : OMX_STRIP_PARAMS[i].def;
  omx_strip_resolve(&s->core, s->enabled && *s->enabled < 0.5f, v);
  if (s->latency) *s->latency = omx_strip_latency(&s->core);
  omx_strip_run(&s->core, s->in_l, s->in_r, s->out_l, s->out_r, frames);
}

static void cleanup(LV2_Handle h) { free(h); }

static const void *extension_data(const char *uri) {
  (void)uri;
  return NULL;
}

static const LV2_Descriptor DESCRIPTOR = {OMX_STRIP_LV2_URI, instantiate, connect_port, activate, run, NULL, cleanup,
                                          extension_data};

LV2_SYMBOL_EXPORT const LV2_Descriptor *lv2_descriptor(uint32_t index) { return index == 0 ? &DESCRIPTOR : NULL; }
