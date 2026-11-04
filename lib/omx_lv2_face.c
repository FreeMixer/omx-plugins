// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * omx_lv2_face.c — the LV2 face of every plugin, in plain C: one plugin's struct omx_face
 * (include/omx_plugin_face.h), its generated declaration over omx-dsp's kernel, reached through its
 * omx_<kernel>_core.h's vtable. No DSP of its own: flush denormals, read the control ports (an
 * unconnected one is its declared default), resolve, run; the `enabled` port is the host's bypass and
 * `latency` reports the kernel's.
 *
 * A keyed plugin's sidechain key (lv2:isSideChain, connectionOptional) follows in_l and in_r; an
 * unconnected key is the kernel's own detector.
 */
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "omx_lv2_face.h"
#include <omxdsp/omx_denormal.h>

/* One allocation: this, the control ports and the run's values (each param_count long), then the core. */
typedef struct {
  const struct omx_face *face;
  void *core;
  uint32_t rate;
  const float *in_l, *in_r;
  const float *key;
  float *out_l, *out_r;
  const float *enabled;
  float *latency;
  const float **param;
  float *values;
  int bypass; /* the bypass the core last resolved */
  int dirty;  /* resolve before the next block: a value or the bypass moved, or the core is fresh */
} Face;

LV2_Handle omx_lv2_face_instantiate(const LV2_Descriptor *d, double rate, const char *bundle,
                                    const LV2_Feature *const *features) {
  (void)bundle;
  (void)features;
  if (!(rate > 0.0)) return NULL;
  const struct omx_face *f = ((const struct omx_lv2_face *)d)->face;
  const size_t n = f->param_count;
  const size_t at_values = sizeof(Face) + n * sizeof(const float *);
  const size_t at_core = (at_values + n * sizeof(float) + alignof(max_align_t) - 1u) / alignof(max_align_t) * alignof(max_align_t);
  char *mem = (char *)calloc(1, at_core + f->core->size);
  if (!mem) return NULL;
  Face *s = (Face *)mem;
  s->face = f;
  s->param = (const float **)(mem + sizeof(Face));
  s->values = (float *)(mem + at_values);
  s->core = mem + at_core;
  s->rate = (uint32_t)rate;
  f->core->init(s->core, s->rate);
  s->dirty = 1;
  return s;
}

void omx_lv2_face_connect_port(LV2_Handle h, uint32_t port, void *data) {
  Face *s = (Face *)h;
  const struct omx_face_lv2_ports *p = &s->face->lv2;
  if (port == p->in_l) s->in_l = (const float *)data;
  else if (port == p->in_r) s->in_r = (const float *)data;
  else if (s->face->key_symbol && port == p->key) s->key = (const float *)data;
  else if (port == p->out_l) s->out_l = (float *)data;
  else if (port == p->out_r) s->out_r = (float *)data;
  else if (port == p->enabled) s->enabled = (const float *)data;
  else if (port == p->latency) s->latency = (float *)data;
  else if (port >= p->first_param && port < p->first_param + s->face->param_count)
    s->param[port - p->first_param] = (const float *)data;
}

void omx_lv2_face_activate(LV2_Handle h) {
  Face *s = (Face *)h;
  s->face->core->init(s->core, s->rate);
  s->dirty = 1;
}

static float value(const Face *s, uint32_t i) { return s->param[i] ? *s->param[i] : s->face->params[i].def; }

void omx_lv2_face_run(LV2_Handle h, uint32_t frames) {
  Face *s = (Face *)h;
  const struct omx_face_core *c = s->face->core;
  omx_denormals_off();
  const int bypass = s->enabled && *s->enabled < 0.5f;
  /* A block whose control ports read what the core last resolved skips the resolve: the core keeps its
   * smoothing as it is, which resolving the same values again gives, bit for bit. */
  int dirty = s->dirty || bypass != s->bypass;
  for (uint32_t i = 0; i < s->face->param_count; ++i) {
    const float v = value(s, i);
    if (v != s->values[i]) s->values[i] = v, dirty = 1;
  }
  if (dirty) c->resolve(s->core, s->values, bypass), s->bypass = bypass, s->dirty = 0;
  if (s->latency) *s->latency = (float)c->latency(s->core);
  c->run(s->core, s->key, s->in_l, s->in_r, s->out_l, s->out_r, frames);
}

void omx_lv2_face_cleanup(LV2_Handle h) { free(h); }

/* org.openmixer.panel-description/1: the declaration's panel, its parameter ids the LV2 port indices
 * (the first parameter's port added). */
static const char *panel_fingerprint(void *h) { return ((const Face *)h)->face->panel->fingerprint; }

static uint32_t panel_family(void *h) { return ((const Face *)h)->face->panel->family; }

static uint32_t panel_count(void *h, uint32_t table) { return omx_panel_count(((const Face *)h)->face->panel, table); }

static bool panel_get(void *h, uint32_t table, uint32_t index, void *out) {
  const struct omx_face *f = ((const Face *)h)->face;
  return omx_panel_get(f->panel, table, index, out, f->lv2.first_param);
}

static const omx_lv2_panel_description_t PANEL = {panel_fingerprint, panel_family, panel_count, panel_get};

const void *omx_lv2_face_extension_data(const char *uri) {
  if (strcmp(uri, OMX_PANEL_DESCRIPTION_ID) == 0) return &PANEL;
  return NULL;
}
