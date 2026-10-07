// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * omx_drive_lv2.c — the LV2 face of omx-drive, in plain C: the generated declaration
 * (generated/omx_drive_params.h) over omx-dsp's instance core
 * (<omxdsp/fx/omx_drive_instance.h>, itself over <omxdsp/fx/omx_drive.h>). No DSP of its own:
 * flush denormals, fill the core's twelve-field ports struct, resolve, run.
 *
 * Ports: in_l, in_r, out_l, out_r, then driveDb character bandHz mix trimDb (the declaration,
 * in order), then `enabled` (lv2:enabled; 0 is the bypass) and `latency`. A parameter port the
 * host left unconnected reads its declared default.
 *
 * The instance core's own seven other fields (curve, band, autoGain, stereoLink, hfRolloff,
 * oversample) are not declared here and are not host-controllable: they take the core's fixed
 * operating point, `FIXED_PORTS` below — ONE exception to its own defaults, named at the field:
 * `band` is pinned to TILT rather than the core's own default of FULL, because FULL ignores
 * `band_hz` entirely (mix_drive's band split only reads it for LOW/HIGH/TILT) and `bandHz` is a
 * declared, travel-bearing parameter here — a declared knob a fixed FULL would make inert.
 */
#include <stdint.h>
#include <stdlib.h>

#include <lv2/core/lv2.h>

/* The generated table FIRST: its guard is omx-dsp's own, so the instance header reads this table. */
#include "omx_drive_params.h"
#include <omxdsp/fx/omx_drive_instance.h>
#include <omxdsp/omx_denormal.h>

typedef struct {
  OmxDriveLv2 core;
  const float *in_l, *in_r;
  float *out_l, *out_r;
  const float *param[OMX_DRIVE_PARAM_COUNT];
  const float *enabled;
  float *latency;
} Drive;

static LV2_Handle instantiate(const LV2_Descriptor *d, double rate, const char *bundle,
                              const LV2_Feature *const *features) {
  (void)d;
  (void)bundle;
  (void)features;
  if (!(rate > 0.0)) return NULL;
  Drive *s = (Drive *)calloc(1, sizeof *s);
  if (!s) return NULL;
  omx_drive_lv2_init(&s->core, (uint32_t)rate);
  return s;
}

static void connect_port(LV2_Handle h, uint32_t port, void *data) {
  Drive *s = (Drive *)h;
  switch (port) {
  case OMX_DRIVE_LV2_PORT_IN_L: s->in_l = (const float *)data; return;
  case OMX_DRIVE_LV2_PORT_IN_R: s->in_r = (const float *)data; return;
  case OMX_DRIVE_LV2_PORT_OUT_L: s->out_l = (float *)data; return;
  case OMX_DRIVE_LV2_PORT_OUT_R: s->out_r = (float *)data; return;
  case OMX_DRIVE_LV2_PORT_ENABLED: s->enabled = (const float *)data; return;
  case OMX_DRIVE_LV2_PORT_LATENCY: s->latency = (float *)data; return;
  default:
    if (port >= OMX_DRIVE_LV2_PORT_FIRST_PARAM && port < OMX_DRIVE_LV2_PORT_FIRST_PARAM + OMX_DRIVE_PARAM_COUNT)
      s->param[port - OMX_DRIVE_LV2_PORT_FIRST_PARAM] = (const float *)data;
  }
}

static void activate(LV2_Handle h) { omx_drive_lv2_init(&((Drive *)h)->core, ((Drive *)h)->core.rate); }

static float value(const Drive *s, uint32_t i) { return s->param[i] ? *s->param[i] : OMX_DRIVE_PARAMS[i].def; }

static void run(LV2_Handle h, uint32_t frames) {
  Drive *s = (Drive *)h;
  omx_denormals_off();
  const int bypass = s->enabled && *s->enabled < 0.5f;
  struct omx_drive_lv2_ports p = OMX_DRIVE_LV2_PORT_DEFAULTS;
  p.bypass = bypass ? 1.0f : 0.0f;
  p.band = (float)OMX_DRIVE_BAND_TILT; /* the fixed point that keeps bandHz audible; see file header */
  p.drive_db = value(s, OMX_DRIVE_PARAM_DRIVE_DB);
  p.character = value(s, OMX_DRIVE_PARAM_CHARACTER);
  p.band_hz = value(s, OMX_DRIVE_PARAM_BAND_HZ);
  p.mix_pct = value(s, OMX_DRIVE_PARAM_MIX);
  p.trim_db = value(s, OMX_DRIVE_PARAM_TRIM_DB);
  omx_drive_lv2_resolve(&s->core, &p);
  if (s->latency) *s->latency = (float)omx_drive_lv2_latency(&s->core);
  omx_drive_lv2_run(&s->core, s->in_l, s->in_r, s->out_l, s->out_r, frames);
}

static void cleanup(LV2_Handle h) { free(h); }

static const void *extension_data(const char *uri) {
  (void)uri;
  return NULL;
}

static const LV2_Descriptor DESCRIPTOR = {OMX_DRIVE_LV2_URI, instantiate, connect_port, activate, run, NULL, cleanup,
                                          extension_data};

LV2_SYMBOL_EXPORT const LV2_Descriptor *lv2_descriptor(uint32_t index) { return index == 0 ? &DESCRIPTOR : NULL; }
