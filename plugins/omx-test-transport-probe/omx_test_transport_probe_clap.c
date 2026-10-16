// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * omx_test_transport_probe_clap.c — omx test-transport-probe, a TEST plugin (role test: linked,
 * created by id, never listed or installed). An effect: its stereo input reaches its stereo output
 * unchanged, bit for bit; each process() call records the clap_event_transport_t it was handed (or
 * that it was handed none), which a test reads through org.openmixer.test-transport-probe/1
 * (omx_test_transport_probe.h).
 *
 * The record is written once per block, at the end of process(), under a sequence counter: odd while
 * the copy is written, even when it is whole. The reader retries up to the declared readRetries times.
 * The process path allocates nothing, takes no lock and makes no system call.
 */
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <clap/clap.h>

#include "omx_test_transport_probe.h"
#include "omx_test_transport_probe_decl.h"

typedef struct {
  clap_plugin_t plugin;
  const clap_host_t *host;
  uint64_t blocks;
  _Atomic uint32_t seq;
  struct omx_test_transport_record rec;
} G;

static void g_record(G *g, const clap_process_t *proc) {
  struct omx_test_transport_record r;
  memset(&r, 0, sizeof r);
  r.blocks = ++g->blocks;
  r.steady_time = (uint64_t)proc->steady_time;
  r.frames = proc->frames_count;
  r.has_transport = proc->transport != NULL;
  if (proc->transport) memcpy(&r.transport, proc->transport, sizeof r.transport);
  const uint32_t s = atomic_load_explicit(&g->seq, memory_order_relaxed);
  atomic_store_explicit(&g->seq, s + 1u, memory_order_relaxed);
  atomic_thread_fence(memory_order_release);
  memcpy(&g->rec, &r, sizeof r);
  atomic_store_explicit(&g->seq, s + 2u, memory_order_release);
}

static bool x_last(const clap_plugin_t *p, struct omx_test_transport_record *out) {
  G *g = (G *)p->plugin_data;
  for (int i = 0; i < OMX_TEST_TRANSPORT_PROBE_READ_RETRIES; i++) {
    const uint32_t a = atomic_load_explicit(&g->seq, memory_order_acquire);
    if (a & 1u) continue;
    memcpy(out, &g->rec, sizeof *out);
    atomic_thread_fence(memory_order_acquire);
    if (atomic_load_explicit(&g->seq, memory_order_relaxed) == a) return a != 0u;
  }
  return false;
}

static const omx_test_transport_probe_t G_PROBE = {x_last};

static clap_process_status g_process(const clap_plugin_t *p, const clap_process_t *proc) {
  G *g = (G *)p->plugin_data;
  if (proc->audio_inputs_count < 1 || proc->audio_outputs_count < 1) return CLAP_PROCESS_ERROR;
  const clap_audio_buffer_t *in = &proc->audio_inputs[0], *out = &proc->audio_outputs[0];
  for (uint32_t c = 0; c < out->channel_count; c++) {
    const float *src = c < in->channel_count ? in->data32[c] : NULL;
    if (src) memmove(out->data32[c], src, proc->frames_count * sizeof(float));
    else memset(out->data32[c], 0, proc->frames_count * sizeof(float));
  }
  g_record(g, proc);
  return CLAP_PROCESS_CONTINUE;
}

/* ---- lifecycle ---------------------------------------------------------------------------- */

static bool g_init(const clap_plugin_t *p) {
  (void)p;
  return true;
}

static void g_destroy(const clap_plugin_t *p) { free(p->plugin_data); }

static bool g_activate(const clap_plugin_t *p, double rate, uint32_t min, uint32_t max) {
  (void)rate, (void)min, (void)max;
  G *g = (G *)p->plugin_data;
  g->blocks = 0;
  atomic_store_explicit(&g->seq, 0u, memory_order_relaxed);
  memset(&g->rec, 0, sizeof g->rec);
  return true;
}

static void g_deactivate(const clap_plugin_t *p) { (void)p; }

static bool g_start_processing(const clap_plugin_t *p) {
  (void)p;
  return true;
}

static void g_stop_processing(const clap_plugin_t *p) { (void)p; }

static void g_reset(const clap_plugin_t *p) { (void)p; }

/* ---- extensions --------------------------------------------------------------------------- */

static uint32_t ap_count(const clap_plugin_t *p, bool in) {
  (void)p, (void)in;
  return 1;
}

static bool ap_get(const clap_plugin_t *p, uint32_t i, bool in, clap_audio_port_info_t *info) {
  (void)p;
  if (i != 0) return false;
  memset(info, 0, sizeof *info);
  info->id = in ? 0 : 1;
  strcpy(info->name, in ? "in" : "out");
  info->flags = CLAP_AUDIO_PORT_IS_MAIN;
  info->channel_count = 2;
  info->port_type = CLAP_PORT_STEREO;
  info->in_place_pair = in ? 1 : 0;
  return true;
}

static const clap_plugin_audio_ports_t G_AUDIO_PORTS = {ap_count, ap_get};

static const void *g_get_extension(const clap_plugin_t *p, const char *id) {
  (void)p;
  if (strcmp(id, CLAP_EXT_AUDIO_PORTS) == 0) return &G_AUDIO_PORTS;
  if (strcmp(id, OMX_TEST_TRANSPORT_PROBE_ID) == 0) return &G_PROBE;
  return NULL;
}

static void g_on_main_thread(const clap_plugin_t *p) { (void)p; }

/* ---- descriptor, factory, entry --------------------------------------------------------- */

static const char *G_FEATURES[] = {OMX_TEST_TRANSPORT_PROBE_CLAP_FEATURES, NULL};

static const clap_plugin_descriptor_t G_DESC = {
    CLAP_VERSION_INIT, OMX_TEST_TRANSPORT_PROBE_CLAP_ID, OMX_TEST_TRANSPORT_PROBE_NAME, OMX_TEST_TRANSPORT_PROBE_VENDOR,
    OMX_TEST_TRANSPORT_PROBE_URL, "", "", OMX_TEST_TRANSPORT_PROBE_VERSION, OMX_TEST_TRANSPORT_PROBE_DESCRIPTION,
    G_FEATURES};

static const clap_plugin_t *g_create(const clap_host_t *host) {
  G *g = (G *)calloc(1, sizeof(G));
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
const clap_plugin_entry_t omx_clap_entry_test_transport_probe = {CLAP_VERSION_INIT, e_init, e_deinit, e_factory};

#ifndef OMX_CLAP_STATIC
/* A standalone .clap for the oracle: the same entry under CLAP's exported name. */
CLAP_EXPORT const clap_plugin_entry_t clap_entry = {CLAP_VERSION_INIT, e_init, e_deinit, e_factory};
#endif
