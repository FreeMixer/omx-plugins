// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * clap-pages-follow-probe.c — a built CLAP face's clap.remote-controls pages, read back as a host reads them, as one
 * governing parameter moves (omx-strip's eqBands, #308): the pages at the come-up value, then after each value
 * the host sets. A minimal host that offers clap.remote-controls counts the face's changed() calls, and runs the
 * face's on_main_thread when the face asked for a callback (a host's deferral to its main thread).
 *
 *   clap-pages-follow-probe <x.clap> <param id> <value>...
 *
 * Output, tab separated: `step <value|init> <changed calls so far> <page count>`, then one `page <k> <section> <name>
 * <id,id,...>` line a page (an empty slot 4294967295).
 */
#include <dlfcn.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clap/entry.h"
#include "clap/events.h"
#include "clap/ext/params.h"
#include "clap/ext/remote-controls.h"
#include "clap/factory/plugin-factory.h"
#include "clap/host.h"

static unsigned g_changed, g_asked;

static void pages_changed(const clap_host_t *h) { (void)h, ++g_changed; }
static void pages_suggest(const clap_host_t *h, clap_id id) { (void)h, (void)id; }
static const clap_host_remote_controls_t HOST_PAGES = {pages_changed, pages_suggest};
static const void *host_get_extension(const clap_host_t *h, const char *id) {
  (void)h;
  return strcmp(id, CLAP_EXT_REMOTE_CONTROLS) == 0 ? &HOST_PAGES : NULL;
}
static void host_noop(const clap_host_t *h) { (void)h; }
static void host_callback(const clap_host_t *h) { (void)h, ++g_asked; }
static const clap_host_t HOST = {
    CLAP_VERSION_INIT, NULL, "omx clap-pages-follow-probe", "openmixer", "", "0.1", host_get_extension, host_noop, host_noop, host_callback,
};

typedef struct {
  clap_event_param_value_t ev;
  uint32_t n;
} One;
static uint32_t ev_size(const clap_input_events_t *l) { return ((const One *)l->ctx)->n; }
static const clap_event_header_t *ev_get(const clap_input_events_t *l, uint32_t i) { return i ? NULL : &((const One *)l->ctx)->ev.header; }
static bool ev_push(const clap_output_events_t *l, const clap_event_header_t *e) { return (void)l, (void)e, true; }

static void show(const char *what, const clap_plugin_t *p, const clap_plugin_remote_controls_t *rc) {
  const uint32_t n = rc->count(p);
  printf("step\t%s\t%u\t%" PRIu32 "\n", what, g_changed, n);
  for (uint32_t k = 0; k < n; ++k) {
    clap_remote_controls_page_t page;
    if (!rc->get(p, k, &page)) {
      printf("page\t%" PRIu32 "\tUNSERVED\n", k);
      continue;
    }
    printf("page\t%" PRIu32 "\t%s\t%s\t", k, page.section_name, page.page_name);
    for (uint32_t s = 0; s < CLAP_REMOTE_CONTROLS_COUNT; ++s) printf("%s%" PRIu32, s ? "," : "", page.param_ids[s]);
    printf("\n");
  }
}

int main(int argc, char **argv) {
  if (argc < 4) return fprintf(stderr, "usage: clap-pages-follow-probe <x.clap> <param id> <value>...\n"), 2;
  void *so = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
  if (!so) return fprintf(stderr, "dlopen %s: %s\n", argv[1], dlerror()), 1;
  const clap_plugin_entry_t *entry = (const clap_plugin_entry_t *)dlsym(so, "clap_entry");
  if (!entry || !entry->init(argv[1])) return fprintf(stderr, "no clap_entry\n"), 1;
  const clap_plugin_factory_t *f = (const clap_plugin_factory_t *)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
  if (!f || f->get_plugin_count(f) != 1) return fprintf(stderr, "factory\n"), 1;
  const clap_plugin_t *p = f->create_plugin(f, &HOST, f->get_plugin_descriptor(f, 0)->id);
  if (!p || !p->init(p)) return fprintf(stderr, "create/init\n"), 1;
  const clap_plugin_params_t *params = (const clap_plugin_params_t *)p->get_extension(p, CLAP_EXT_PARAMS);
  const clap_plugin_remote_controls_t *rc = (const clap_plugin_remote_controls_t *)p->get_extension(p, CLAP_EXT_REMOTE_CONTROLS);
  if (!params || !rc) return fprintf(stderr, "missing clap.params or clap.remote-controls\n"), 1;
  if (!p->activate(p, 48000.0, 1, 4096)) return fprintf(stderr, "activate\n"), 1;
  show("init", p, rc);
  for (int a = 3; a < argc; ++a) {
    One one;
    memset(&one, 0, sizeof one);
    one.n = 1;
    one.ev.header = (clap_event_header_t){sizeof one.ev, 0, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_PARAM_VALUE, 0};
    one.ev.param_id = (clap_id)strtoul(argv[2], NULL, 10);
    one.ev.note_id = -1, one.ev.port_index = -1, one.ev.channel = -1, one.ev.key = -1;
    one.ev.value = strtod(argv[a], NULL);
    const clap_input_events_t in = {&one, ev_size, ev_get};
    const clap_output_events_t out = {NULL, ev_push};
    params->flush(p, &in, &out);
    if (g_asked) g_asked = 0, p->on_main_thread(p); /* the host's main thread runs the callback the face asked for */
    show(argv[a], p, rc);
  }
  p->deactivate(p);
  p->destroy(p);
  return 0;
}
