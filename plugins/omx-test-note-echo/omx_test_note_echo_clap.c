// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * omx_test_note_echo_clap.c — omx test-note-echo, a TEST plugin (role test: linked, created by id,
 * never listed or installed). Every note event it is handed (CLAP note on, off and choke; a MIDI note
 * on or off) is pushed to its note output unchanged, at the same frame: one note out per note in,
 * same key, velocity, channel and note id. Its stereo audio output is silence.
 *
 * Ports: one note input and one note output (CLAP and MIDI dialects), no audio input, one audio
 * output of the declared outputChannels. The process path allocates nothing, takes no lock and makes
 * no system call.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <clap/clap.h>

#include "omx_test_note_echo_decl.h"

typedef struct {
  clap_plugin_t plugin;
  const clap_host_t *host;
} G;

static bool g_is_note(const clap_event_header_t *h) {
  if (h->space_id != CLAP_CORE_EVENT_SPACE_ID) return false;
  if (h->type == CLAP_EVENT_NOTE_ON || h->type == CLAP_EVENT_NOTE_OFF || h->type == CLAP_EVENT_NOTE_CHOKE) return true;
  if (h->type != CLAP_EVENT_MIDI) return false;
  const uint8_t kind = ((const clap_event_midi_t *)h)->data[0] & 0xF0;
  return kind == 0x80 || kind == 0x90;
}

static clap_process_status g_process(const clap_plugin_t *p, const clap_process_t *proc) {
  (void)p;
  for (uint32_t o = 0; o < proc->audio_outputs_count; o++)
    for (uint32_t c = 0; c < proc->audio_outputs[o].channel_count; c++)
      memset(proc->audio_outputs[o].data32[c], 0, proc->frames_count * sizeof(float));
  const uint32_t n = proc->in_events ? proc->in_events->size(proc->in_events) : 0;
  for (uint32_t i = 0; i < n; i++) {
    const clap_event_header_t *h = proc->in_events->get(proc->in_events, i);
    if (g_is_note(h)) proc->out_events->try_push(proc->out_events, h);
  }
  return CLAP_PROCESS_CONTINUE;
}

/* ---- lifecycle ---------------------------------------------------------------------------- */

static bool g_init(const clap_plugin_t *p) {
  (void)p;
  return true;
}

static void g_destroy(const clap_plugin_t *p) { free(p->plugin_data); }

static bool g_activate(const clap_plugin_t *p, double rate, uint32_t min, uint32_t max) {
  (void)p, (void)min, (void)max;
  return rate > 0.0;
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
  (void)p;
  return in ? 0 : 1;
}

static bool ap_get(const clap_plugin_t *p, uint32_t i, bool in, clap_audio_port_info_t *info) {
  (void)p;
  if (in || i != 0) return false;
  memset(info, 0, sizeof *info);
  info->id = 0;
  strcpy(info->name, "out");
  info->flags = CLAP_AUDIO_PORT_IS_MAIN;
  info->channel_count = OMX_TEST_NOTE_ECHO_OUTPUT_CHANNELS;
  info->port_type = OMX_TEST_NOTE_ECHO_OUTPUT_CHANNELS == 2 ? CLAP_PORT_STEREO : NULL;
  info->in_place_pair = CLAP_INVALID_ID;
  return true;
}

static const clap_plugin_audio_ports_t G_AUDIO_PORTS = {ap_count, ap_get};

static uint32_t np_count(const clap_plugin_t *p, bool in) {
  (void)p, (void)in;
  return 1;
}

static bool np_get(const clap_plugin_t *p, uint32_t i, bool in, clap_note_port_info_t *info) {
  (void)p;
  if (i != 0) return false;
  memset(info, 0, sizeof *info);
  info->id = in ? 0 : 1;
  info->supported_dialects = CLAP_NOTE_DIALECT_CLAP | CLAP_NOTE_DIALECT_MIDI;
  info->preferred_dialect = CLAP_NOTE_DIALECT_CLAP;
  strcpy(info->name, in ? "notes in" : "notes out");
  return true;
}

static const clap_plugin_note_ports_t G_NOTE_PORTS = {np_count, np_get};

static const void *g_get_extension(const clap_plugin_t *p, const char *id) {
  (void)p;
  if (strcmp(id, CLAP_EXT_AUDIO_PORTS) == 0) return &G_AUDIO_PORTS;
  if (strcmp(id, CLAP_EXT_NOTE_PORTS) == 0) return &G_NOTE_PORTS;
  return NULL;
}

static void g_on_main_thread(const clap_plugin_t *p) { (void)p; }

/* ---- descriptor, factory, entry --------------------------------------------------------- */

static const char *G_FEATURES[] = {OMX_TEST_NOTE_ECHO_CLAP_FEATURES, NULL};

static const clap_plugin_descriptor_t G_DESC = {
    CLAP_VERSION_INIT, OMX_TEST_NOTE_ECHO_CLAP_ID, OMX_TEST_NOTE_ECHO_NAME, OMX_TEST_NOTE_ECHO_VENDOR,
    OMX_TEST_NOTE_ECHO_URL, "", "", OMX_TEST_NOTE_ECHO_VERSION, OMX_TEST_NOTE_ECHO_DESCRIPTION, G_FEATURES};

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
const clap_plugin_entry_t omx_clap_entry_test_note_echo = {CLAP_VERSION_INIT, e_init, e_deinit, e_factory};

#ifndef OMX_CLAP_STATIC
/* A standalone .clap for the oracle: the same entry under CLAP's exported name. */
CLAP_EXPORT const clap_plugin_entry_t clap_entry = {CLAP_VERSION_INIT, e_init, e_deinit, e_factory};
#endif
