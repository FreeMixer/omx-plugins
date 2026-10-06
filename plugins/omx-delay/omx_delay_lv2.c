// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * omx_delay_lv2.c — the LV2 face of omx-delay, in plain C: the same declaration as the CLAP face
 * (generated/omx_delay_params.h, whose LV2 port indices are the ones tools/gen.mjs writes into the
 * bundle's TTL), the same omx-dsp instance core, the same callback: flush denormals, resolve, run.
 *
 * Ports: in_l, in_r, out_l, out_r, then every declared parameter in declaration order, then
 * `enabled` (lv2:enabled; 0 is the bypass) and `latency` (always 0: the delay IS the effect).
 * A parameter port the host left unconnected reads its declared default.
 */
#include <stdint.h>
#include <stdlib.h>

#include <lv2/core/lv2.h>

/* The generated table FIRST: its guard is omx-dsp's own, so the instance header reads this table. */
#include "omx_delay_params.h"
#include <omxdsp/fx/omx_delay_instance.h>
#include <omxdsp/omx_denormal.h>

typedef struct {
  OmxDelayLv2 core;
  float *block; /* both rings, one allocation */
  const float *in_l, *in_r;
  float *out_l, *out_r;
  const float *param[OMX_DELAY_PARAM_COUNT];
  const float *enabled;
  float *latency;
} Delay;

static LV2_Handle instantiate(const LV2_Descriptor *d, double rate, const char *bundle,
                              const LV2_Feature *const *features) {
  (void)d;
  (void)bundle;
  (void)features;
  if (!(rate > 0.0)) return NULL;
  Delay *s = (Delay *)calloc(1, sizeof *s);
  if (!s) return NULL;
  const uint32_t cap = (uint32_t)((double)OMX_FXDELAY_MAX_MS * rate / 1000.0) + 1u;
  s->block = (float *)calloc((size_t)2 * cap, sizeof(float));
  if (!s->block || !omx_delay_lv2_init(&s->core, (float)rate, s->block, s->block + cap, cap)) {
    free(s->block);
    free(s);
    return NULL;
  }
  return s;
}

static void connect_port(LV2_Handle h, uint32_t port, void *data) {
  Delay *s = (Delay *)h;
  switch (port) {
  case OMX_DELAY_LV2_PORT_IN_L: s->in_l = (const float *)data; return;
  case OMX_DELAY_LV2_PORT_IN_R: s->in_r = (const float *)data; return;
  case OMX_DELAY_LV2_PORT_OUT_L: s->out_l = (float *)data; return;
  case OMX_DELAY_LV2_PORT_OUT_R: s->out_r = (float *)data; return;
  case OMX_DELAY_LV2_PORT_ENABLED: s->enabled = (const float *)data; return;
  case OMX_DELAY_LV2_PORT_LATENCY: s->latency = (float *)data; return;
  default:
    if (port >= OMX_DELAY_LV2_PORT_FIRST_PARAM && port < OMX_DELAY_LV2_PORT_FIRST_PARAM + OMX_DELAY_PARAM_COUNT)
      s->param[port - OMX_DELAY_LV2_PORT_FIRST_PARAM] = (const float *)data;
  }
}

static void activate(LV2_Handle h) { omx_delay_lv2_clear(&((Delay *)h)->core); }

static float value(const Delay *s, uint32_t i) { return s->param[i] ? *s->param[i] : OMX_DELAY_PARAMS[i].def; }

static void run(LV2_Handle h, uint32_t frames) {
  Delay *s = (Delay *)h;
  omx_denormals_off();
  if (s->latency) *s->latency = OMX_DELAY_LV2_LATENCY_FRAMES;
  const int bypass = s->enabled && *s->enabled < 0.5f;
  const float t = value(s, OMX_DELAY_PARAM_TIME_MS);
  /* Time is ONE declared value; the kernel's two taps are set equal, as the console's row does. */
  omx_delay_lv2_resolve(&s->core, bypass, t, t, value(s, OMX_DELAY_PARAM_FEEDBACK), value(s, OMX_DELAY_PARAM_MIX),
                        value(s, OMX_DELAY_PARAM_TONE), value(s, OMX_DELAY_PARAM_PINGPONG) > 0.5f ? 1 : 0);
  omx_delay_lv2_run(&s->core, s->in_l, s->in_r, s->out_l, s->out_r, frames);
}

static void cleanup(LV2_Handle h) {
  Delay *s = (Delay *)h;
  free(s->block);
  free(s);
}

static const void *extension_data(const char *uri) {
  (void)uri;
  return NULL;
}

static const LV2_Descriptor DESCRIPTOR = {OMX_DELAY_LV2_URI, instantiate, connect_port, activate, run, NULL, cleanup,
                                          extension_data};

LV2_SYMBOL_EXPORT const LV2_Descriptor *lv2_descriptor(uint32_t index) { return index == 0 ? &DESCRIPTOR : NULL; }
