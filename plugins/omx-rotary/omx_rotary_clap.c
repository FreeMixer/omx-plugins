// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * omx_rotary_clap.c — the CLAP face of omx-rotary, in plain C over our CLAP ABI (the released
 * omx-clap-host's <omx-clap-host/omx_clap_ext.h> for the openmixer extensions).
 *
 * Every number comes from generated/omx_rotary_params.h (tools/gen.mjs, from omx-rotary.decl.json);
 * the DSP is omx-dsp's rotary kernel, reached through omx_rotary_core.h, the binding both faces
 * share. This file adds no arithmetic on the audio path: drain the events, resolve, run.
 */
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clap/entry.h"
#include "clap/events.h"
#include "clap/ext/audio-ports.h"
#include "clap/ext/latency.h"
#include "clap/ext/params.h"
#include "clap/ext/state.h"
#include "clap/factory/plugin-factory.h"
#include "clap/plugin.h"
#include "clap/process.h"
#include "clap/stream.h"

/* The generated table FIRST: its guard is omx-dsp's own, so the instance header reads this table. */
#include "omx_rotary_params.h"
#include "omx_rotary_core.h"
#include <omx-clap-host/omx_clap_ext.h>
#include <omxdsp/omx_denormal.h> /* omx_denormals_off, per callback */

#define G_DECLARED OMX_ROTARY_PARAM_COUNT
/* The host's bypass, appended after the declared ids. */
#define G_BYPASS G_DECLARED
#define G_PARAMS (G_DECLARED + 1)

static const char G_SOURCE[] = OMX_ROTARY_DECL_SOURCE;
static const char G_DIGEST[] = OMX_ROTARY_DECL_DIGEST;

typedef struct {
  clap_plugin_t plugin;
  const clap_host_t *host;
  OmxRotaryCore core;
  float values[G_PARAMS];          /* audio-thread owned */
  _Atomic float applied[G_PARAMS]; /* the APPLIED values, published after each drain */
  int active, processing;
} G;

static float g_min(uint32_t i) { return i < G_DECLARED ? OMX_ROTARY_PARAMS[i].min : 0.0f; }
static float g_max(uint32_t i) { return i < G_DECLARED ? OMX_ROTARY_PARAMS[i].max : 1.0f; }
static float g_def(uint32_t i) { return i < G_DECLARED ? OMX_ROTARY_PARAMS[i].def : 0.0f; }

/** Into the declared travel; NaN floors. */
static float g_clamp(uint32_t i, double v) {
  if (!(v > (double)g_min(i))) return g_min(i);
  if (v > (double)g_max(i)) return g_max(i);
  return (float)v;
}

static void g_publish(G *g) {
  for (uint32_t i = 0; i < G_PARAMS; ++i) atomic_store_explicit(&g->applied[i], g->values[i], memory_order_relaxed);
}

/** Frame 0: every value event in the block is applied before the kernel runs. */
static void g_drain(G *g, const clap_input_events_t *in) {
  if (!in) return;
  const uint32_t n = in->size(in);
  for (uint32_t k = 0; k < n; ++k) {
    const clap_event_header_t *h = in->get(in, k);
    if (!h || h->space_id != CLAP_CORE_EVENT_SPACE_ID || h->type != CLAP_EVENT_PARAM_VALUE) continue;
    const clap_event_param_value_t *e = (const clap_event_param_value_t *)h;
    if (e->param_id < G_PARAMS) g->values[e->param_id] = g_clamp(e->param_id, e->value);
  }
}

/* ---- clap_plugin ------------------------------------------------------------------------ */

static bool g_init(const clap_plugin_t *p) {
  (void)p;
  return true;
}

static void g_destroy(const clap_plugin_t *p) { free((G *)p->plugin_data); }

static bool g_activate(const clap_plugin_t *p, double sample_rate, uint32_t min_frames, uint32_t max_frames) {
  (void)min_frames;
  (void)max_frames;
  G *g = (G *)p->plugin_data;
  if (g->active || !(sample_rate > 0.0)) return false;
  omx_rotary_core_init(&g->core, (uint32_t)sample_rate);
  g->active = 1;
  return true;
}

static void g_deactivate(const clap_plugin_t *p) { ((G *)p->plugin_data)->active = 0; }

static bool g_start_processing(const clap_plugin_t *p) {
  ((G *)p->plugin_data)->processing = 1;
  return true;
}

static void g_stop_processing(const clap_plugin_t *p) { ((G *)p->plugin_data)->processing = 0; }

static void g_reset(const clap_plugin_t *p) {
  G *g = (G *)p->plugin_data;
  omx_rotary_core_init(&g->core, g->core.rate);
}

static clap_process_status g_process(const clap_plugin_t *p, const clap_process_t *pr) {
  G *g = (G *)p->plugin_data;
  omx_denormals_off();
  g_drain(g, pr->in_events);
  if (pr->audio_inputs_count < 1 || pr->audio_outputs_count < 1) return CLAP_PROCESS_ERROR;
  const clap_audio_buffer_t *ai = &pr->audio_inputs[0];
  const clap_audio_buffer_t *ao = &pr->audio_outputs[0];
  if (ai->channel_count < 2 || ao->channel_count < 2 || !ai->data32 || !ao->data32) return CLAP_PROCESS_ERROR;
  omx_rotary_core_resolve(&g->core, g->values, g->values[G_BYPASS] > 0.5f);
  omx_rotary_core_run(&g->core, ai->data32[0], ai->data32[1], ao->data32[0], ao->data32[1], pr->frames_count);
  g_publish(g);
  return CLAP_PROCESS_CONTINUE;
}

/* ---- params --------------------------------------------------------------------- */

static uint32_t g_params_count(const clap_plugin_t *p) {
  (void)p;
  return G_PARAMS;
}

static bool g_params_info(const clap_plugin_t *p, uint32_t i, clap_param_info_t *info) {
  (void)p;
  if (i >= G_PARAMS) return false;
  memset(info, 0, sizeof *info);
  info->id = i;
  info->flags = CLAP_PARAM_IS_AUTOMATABLE;
  if (i == G_BYPASS) {
    info->flags |= CLAP_PARAM_IS_STEPPED | CLAP_PARAM_IS_BYPASS;
    snprintf(info->name, sizeof info->name, "%s", "Bypass");
  } else {
    if (OMX_ROTARY_PARAMS[i].flags & (OMX_PLUGIN_PARAM_INTEGER | OMX_PLUGIN_PARAM_TOGGLE)) info->flags |= CLAP_PARAM_IS_STEPPED;
    snprintf(info->name, sizeof info->name, "%s", OMX_ROTARY_PARAMS[i].name);
  }
  info->min_value = g_min(i);
  info->max_value = g_max(i);
  info->default_value = g_def(i);
  return true;
}

/** The APPLIED value, safe on the host's main (control) thread while audio runs. */
static bool g_params_value(const clap_plugin_t *p, clap_id id, double *out) {
  G *g = (G *)p->plugin_data;
  if (id >= G_PARAMS) return false;
  *out = atomic_load_explicit(&g->applied[id], memory_order_relaxed);
  return true;
}

static bool g_params_to_text(const clap_plugin_t *p, clap_id id, double v, char *out, uint32_t size) {
  (void)p;
  if (id >= G_PARAMS) return false;
  const char *unit = id < G_DECLARED ? OMX_ROTARY_PARAMS[id].unit : "";
  snprintf(out, size, unit[0] ? "%g %s" : "%g", v, unit);
  return true;
}

static bool g_params_from_text(const clap_plugin_t *p, clap_id id, const char *text, double *out) {
  (void)p;
  char *end = NULL;
  if (id >= G_PARAMS || !text) return false;
  *out = strtod(text, &end);
  return end != text;
}

static void g_params_flush(const clap_plugin_t *p, const clap_input_events_t *in, const clap_output_events_t *out) {
  (void)out;
  G *g = (G *)p->plugin_data;
  g_drain(g, in);
  g_publish(g);
}

static const clap_plugin_params_t G_PARAMS_EXT = {g_params_count, g_params_info, g_params_value,
                                                  g_params_to_text, g_params_from_text, g_params_flush};

/* ---- latency, audio-ports ----------------------------------------------------- */

static uint32_t g_latency(const clap_plugin_t *p) {
  G *g = (G *)p->plugin_data;
  return omx_rotary_core_latency(&g->core);
}

static const clap_plugin_latency_t G_LATENCY_EXT = {g_latency};

static uint32_t g_ports_count(const clap_plugin_t *p, bool is_input) {
  (void)p;
  (void)is_input;
  return 1;
}

static bool g_ports_get(const clap_plugin_t *p, uint32_t index, bool is_input, clap_audio_port_info_t *info) {
  (void)p;
  if (index != 0) return false;
  memset(info, 0, sizeof *info);
  info->id = 0;
  snprintf(info->name, sizeof info->name, "%s", is_input ? "in" : "out");
  info->flags = CLAP_AUDIO_PORT_IS_MAIN;
  info->channel_count = 2;
  info->port_type = CLAP_PORT_STEREO;
  info->in_place_pair = 0;
  return true;
}

static const clap_plugin_audio_ports_t G_PORTS_EXT = {g_ports_count, g_ports_get};

/* ---- state: symbol=value lines, declaration order ----------------------------------- */

static bool g_state_save(const clap_plugin_t *p, const clap_ostream_t *s) {
  G *g = (G *)p->plugin_data;
  char line[160];
  for (uint32_t i = 0; i < G_DECLARED; ++i) {
    int n = snprintf(line, sizeof line, "%s=%.9g\n", OMX_ROTARY_PARAMS[i].symbol, (double)atomic_load(&g->applied[i]));
    for (int off = 0; off < n;) {
      int64_t w = s->write(s, line + off, (uint64_t)(n - off));
      if (w <= 0) return false;
      off += (int)w;
    }
  }
  return true;
}

static bool g_state_load(const clap_plugin_t *p, const clap_istream_t *s) {
  G *g = (G *)p->plugin_data;
  if (g->processing) return false; /* never race the audio thread's values */
  char buf[4096];
  size_t len = 0;
  for (;;) {
    if (len == sizeof buf - 1) return false;
    int64_t r = s->read(s, buf + len, sizeof buf - 1 - len);
    if (r < 0) return false;
    if (r == 0) break;
    len += (size_t)r;
  }
  buf[len] = 0;
  for (char *line = strtok(buf, "\n"); line; line = strtok(NULL, "\n")) {
    char *eq = strchr(line, '=');
    if (!eq) continue;
    *eq = 0;
    for (uint32_t i = 0; i < G_DECLARED; ++i) {
      if (strcmp(line, OMX_ROTARY_PARAMS[i].symbol) == 0) g->values[i] = g_clamp(i, strtod(eq + 1, NULL));
    }
  }
  g_publish(g);
  return true;
}

static const clap_plugin_state_t G_STATE_EXT = {g_state_save, g_state_load};

/* ---- omx.declaration -------------------------------------------------------------- */

static const char *g_decl_source(const clap_plugin_t *p) {
  (void)p;
  return G_SOURCE;
}

static const char *g_decl_digest(const clap_plugin_t *p) {
  (void)p;
  return G_DIGEST;
}

static const omx_clap_plugin_declaration_t G_DECL_EXT = {g_decl_source, g_decl_digest};

static const void *g_get_extension(const clap_plugin_t *p, const char *id) {
  (void)p;
  if (strcmp(id, CLAP_EXT_PARAMS) == 0) return &G_PARAMS_EXT;
  if (strcmp(id, CLAP_EXT_LATENCY) == 0) return &G_LATENCY_EXT;
  if (strcmp(id, CLAP_EXT_AUDIO_PORTS) == 0) return &G_PORTS_EXT;
  if (strcmp(id, CLAP_EXT_STATE) == 0) return &G_STATE_EXT;
  if (strcmp(id, OMX_CLAP_EXT_DECLARATION) == 0) return &G_DECL_EXT;
  return NULL;
}

static void g_on_main_thread(const clap_plugin_t *p) { (void)p; }

/* ---- descriptor, factory, entry --------------------------------------------------------- */

static const char *G_FEATURES[] = {OMX_ROTARY_CLAP_FEATURES, NULL};

static const clap_plugin_descriptor_t G_DESC = {
    CLAP_VERSION_INIT, OMX_ROTARY_CLAP_ID, OMX_ROTARY_NAME, OMX_ROTARY_VENDOR, OMX_ROTARY_URL, "", "",
    OMX_ROTARY_VERSION, OMX_ROTARY_DESCRIPTION, G_FEATURES};

static const clap_plugin_t *g_create(const clap_host_t *host) {
  G *g = (G *)calloc(1, sizeof(G)); /* the plugin, the core and the values, one allocation */
  if (!g) return NULL;
  g->host = host;
  g->plugin.desc = &G_DESC;
  g->plugin.plugin_data = g;
  g->plugin.init = g_init;
  g->plugin.destroy = g_destroy;
  g->plugin.activate = g_activate;
  g->plugin.deactivate = g_deactivate;
  g->plugin.start_processing = g_start_processing;
  g->plugin.stop_processing = g_stop_processing;
  g->plugin.reset = g_reset;
  g->plugin.process = g_process;
  g->plugin.get_extension = g_get_extension;
  g->plugin.on_main_thread = g_on_main_thread;
  for (uint32_t i = 0; i < G_PARAMS; ++i) g->values[i] = g_def(i);
  g_publish(g);
  return &g->plugin;
}

static uint32_t f_count(const clap_plugin_factory_t *f) {
  (void)f;
  return 1;
}

static const clap_plugin_descriptor_t *f_desc(const clap_plugin_factory_t *f, uint32_t i) {
  (void)f;
  return i == 0 ? &G_DESC : NULL;
}

static const clap_plugin_t *f_create(const clap_plugin_factory_t *f, const clap_host_t *host, const char *id) {
  (void)f;
  return id && strcmp(id, G_DESC.id) == 0 ? g_create(host) : NULL;
}

static const clap_plugin_factory_t G_FACTORY = {f_count, f_desc, f_create};

static bool e_init(const char *path) {
  (void)path;
  return true;
}

static void e_deinit(void) {}

static const void *e_factory(const char *id) { return strcmp(id, CLAP_PLUGIN_FACTORY_ID) == 0 ? &G_FACTORY : NULL; }

/* The entry by name, for a host that links the plugin statically and opens it by id without dlopen. */
const clap_plugin_entry_t omx_clap_entry_rotary = {CLAP_VERSION_INIT, e_init, e_deinit, e_factory};

#ifndef OMX_CLAP_STATIC
/* A standalone .clap for a foreign host: the same entry under CLAP's exported name. */
CLAP_EXPORT const clap_plugin_entry_t clap_entry = {CLAP_VERSION_INIT, e_init, e_deinit, e_factory};
#endif
