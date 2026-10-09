// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * transient-oracle.c — omx-transient's kernel-identity test: both built faces, sample for sample, against
 * omx-dsp's transient kernel called DIRECTLY on the same blocks, at every declared rate, engaged and
 * bypassed — no tolerance.
 *
 *   transient-oracle clap <omx-transient.clap>
 *   transient-oracle lv2 <bundle-dir> <uri>
 *
 * The signal is deterministic drum-like bursts in a stereo bed, run in uneven blocks (1 to 1024 frames)
 * with every parameter and the bypass changing between blocks, through ONE instance of the face. The
 * reference is the kernel itself — omx_transient_resolve() and omx_transient_process() of
 * <omxdsp/fx/omx_transient.h> on a copy of the same blocks, its state cleared where the face re-engages
 * from bypass — and the output is compared with memcmp. Parameters are found BY NAME (CLAP) or BY SYMBOL
 * (LV2, through lilv), so a renumbered face cannot pass by accident. The published latency is the kernel's.
 */
#include <dlfcn.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <clap/clap.h>
#include <lilv/lilv.h>
#include <omxdsp/fx/omx_transient.h>

#include "omx_transient_params.h"

#define NP OMX_TRANSIENT_PARAM_COUNT
#define MAXB 16u

/* One block: its length, the five declared parameters (declaration order) and the host's bypass. */
typedef struct {
  uint32_t frames;
  float v[NP];
  int bypass;
} Block;

static const Block PLAN[] = {
    {64, {0.0f, 0.0f, 10.0f, 250.0f, 0.0f}, 0},       {1, {12.0f, -6.0f, 5.0f, 400.0f, 0.0f}, 0},
    {333, {12.0f, -6.0f, 5.0f, 400.0f, 0.0f}, 0},     {512, {-18.0f, 18.0f, 30.0f, 100.0f, 3.0f}, 0},
    {17, {-18.0f, 18.0f, 30.0f, 100.0f, 3.0f}, 1},    {480, {24.0f, 24.0f, 2.0f, 2000.0f, -6.0f}, 1},
    {129, {24.0f, 24.0f, 2.0f, 2000.0f, -6.0f}, 0},   {1024, {24.0f, -24.0f, 50.0f, 50.0f, 12.0f}, 0},
    {7, {-24.0f, 24.0f, 20.0f, 1000.0f, -24.0f}, 0},  {2500, {9.0f, 0.0f, 8.0f, 250.0f, 0.0f}, 0},
    {600, {0.0f, -12.0f, 10.0f, 500.0f, 1.5f}, 0},    {3000, {12.0f, 12.0f, 3.0f, 1500.0f, 0.0f}, 0},
};
#define NBLOCKS (sizeof PLAN / sizeof PLAN[0])

static int failures;

static void check(bool ok, const char *face, double sr, const char *what) {
  if (!ok) failures++;
  printf("%s %s %s @ %.0f Hz\n", ok ? "PASS" : "FAIL", face, what, sr);
}

static uint32_t total_frames(void) {
  uint32_t n = 0;
  for (size_t i = 0; i < NBLOCKS; i++) n += PLAN[i].frames;
  return n;
}

/* Bursts of decaying noise on a quiet bed, the legs unequal: real onsets and decays for both contrasts. */
static void signal_make(float *l, float *r, uint32_t n, double sr) {
  uint32_t seed = 0x6f6d78u;
  const uint32_t period = (uint32_t)(0.09 * sr);
  for (uint32_t i = 0; i < n; i++) {
    seed = seed * 1664525u + 1013904223u;
    const float a = (float)(seed >> 8) / 16777216.0f - 0.5f;
    seed = seed * 1664525u + 1013904223u;
    const float b = (float)(seed >> 8) / 16777216.0f - 0.5f;
    const float env = expf(-(float)(i % period) / (0.012f * (float)sr));
    l[i] = 0.9f * env * a + 0.02f * b;
    r[i] = 0.7f * env * b + 0.02f * a;
  }
}

/* The reference: the kernel, called directly, on the same blocks. */
static void reference(double sr, const float *il, const float *ir, float *ol, float *orr) {
  struct omx_transient atom;
  struct omx_transient_state st;
  omx_transient_state_init(&st);
  int engaged_before = 0;
  uint32_t at = 0;
  for (size_t b = 0; b < NBLOCKS; b++) {
    const Block *k = &PLAN[b];
    const int engaged = !k->bypass;
    if (engaged && !engaged_before) omx_transient_state_init(&st);
    engaged_before = engaged;
    omx_transient_resolve(&atom, k->bypass, k->v[OMX_TRANSIENT_PARAM_ATTACK_DB], k->v[OMX_TRANSIENT_PARAM_SUSTAIN_DB],
                          k->v[OMX_TRANSIENT_PARAM_ATTACK_TIME_MS], k->v[OMX_TRANSIENT_PARAM_SUSTAIN_TIME_MS],
                          k->v[OMX_TRANSIENT_PARAM_OUTPUT_DB], (float)sr);
    memcpy(ol + at, il + at, k->frames * sizeof(float));
    memcpy(orr + at, ir + at, k->frames * sizeof(float));
    omx_transient_process(ol + at, orr + at, k->frames, &atom, &st);
    at += k->frames;
  }
}

typedef struct Driver Driver;
struct Driver {
  /* the whole plan through ONE fresh instance; `*latency` is read back */
  bool (*run)(void *face, double sr, const float *il, const float *ir, float *ol, float *orr, uint32_t *latency);
  void *face;
};

static void run_checks(const char *face, Driver *d, double sr) {
  const uint32_t n = total_frames();
  float *buf = calloc((size_t)n * 6u, sizeof(float));
  if (!buf) abort();
  float *il = buf, *ir = buf + n, *ol = buf + 2u * n, *orr = buf + 3u * n, *wl = buf + 4u * n, *wr = buf + 5u * n;
  signal_make(il, ir, n, sr);
  omx_denormals_off(); /* the faces run with flush-to-zero; so does the reference */
  reference(sr, il, ir, wl, wr);
  uint32_t lat = 99;
  bool ran = d->run(d->face, sr, il, ir, ol, orr, &lat);
  bool same = ran && memcmp(ol, wl, (size_t)n * sizeof(float)) == 0 && memcmp(orr, wr, (size_t)n * sizeof(float)) == 0;
  for (uint32_t i = 0; ran && !same && i < n; i++)
    if (memcmp(ol + i, wl + i, sizeof(float)) != 0 || memcmp(orr + i, wr + i, sizeof(float)) != 0) {
      fprintf(stderr, "  first difference at frame %u: got %.9g/%.9g, kernel %.9g/%.9g\n", i, (double)ol[i],
              (double)orr[i], (double)wl[i], (double)wr[i]);
      break;
    }
  check(same, face, sr, "output is the kernel's, bit for bit");
  /* the reference must have moved the signal, or identity would be vacuous */
  bool moved = false;
  for (uint32_t i = 0; i < n && !moved; i++) moved = memcmp(wl + i, il + i, sizeof(float)) != 0;
  check(moved, face, sr, "the kernel shapes the signal");
  check(ran && lat == (uint32_t)omx_transient_latency(), face, sr, "published latency is the kernel's");
  free(buf);
}

/* ---- CLAP ---------------------------------------------------------------------------------- */

static const void *h_ext(const clap_host_t *h, const char *id) {
  (void)h, (void)id;
  return NULL;
}
static void h_noop(const clap_host_t *h) { (void)h; }
static const clap_host_t HOST = {CLAP_VERSION_INIT, NULL, "transient-oracle", "openmixer", "", "0.1", h_ext, h_noop,
                                 h_noop, h_noop};

typedef struct {
  clap_event_param_value_t ev[NP + 1];
  uint32_t n;
} Events;
static uint32_t ev_size(const clap_input_events_t *l) { return ((const Events *)l->ctx)->n; }
static const clap_event_header_t *ev_get(const clap_input_events_t *l, uint32_t i) {
  return &((const Events *)l->ctx)->ev[i].header;
}
static bool ev_push(const clap_output_events_t *l, const clap_event_header_t *e) {
  (void)l, (void)e;
  return true;
}

typedef struct {
  const clap_plugin_entry_t *entry;
  const clap_plugin_factory_t *factory;
} ClapFace;

static bool clap_param(const clap_plugin_params_t *pp, const clap_plugin_t *p, const char *name, clap_id *id) {
  for (uint32_t i = 0; i < pp->count(p); i++) {
    clap_param_info_t info;
    if (!pp->get_info(p, i, &info)) continue;
    if (name ? strcmp(info.name, name) == 0 : (info.flags & CLAP_PARAM_IS_BYPASS) != 0) {
      *id = info.id;
      return true;
    }
  }
  fprintf(stderr, "  no CLAP parameter named %s\n", name ? name : "(bypass)");
  return false;
}

static bool clap_run(void *face, double sr, const float *il, const float *ir, float *ol, float *orr,
                     uint32_t *latency) {
  ClapFace *c = (ClapFace *)face;
  const clap_plugin_descriptor_t *desc = c->factory->get_plugin_descriptor(c->factory, 0);
  const clap_plugin_t *p = c->factory->create_plugin(c->factory, &HOST, desc->id);
  if (!p || !p->init(p)) return false;
  const clap_plugin_params_t *pp = (const clap_plugin_params_t *)p->get_extension(p, CLAP_EXT_PARAMS);
  const clap_plugin_latency_t *lat = (const clap_plugin_latency_t *)p->get_extension(p, CLAP_EXT_LATENCY);
  bool ok = pp && lat && p->activate(p, sr, 1, 4096) && p->start_processing(p);
  clap_id ids[NP + 1];
  for (uint32_t i = 0; ok && i < NP; i++) ok = clap_param(pp, p, OMX_TRANSIENT_PARAMS[i].name, &ids[i]);
  ok = ok && clap_param(pp, p, NULL, &ids[NP]);
  clap_output_events_t out = {NULL, ev_push};
  uint32_t at = 0;
  for (size_t b = 0; ok && b < NBLOCKS; b++) {
    const Block *k = &PLAN[b];
    Events evs = {.n = 0};
    for (uint32_t i = 0; i <= NP; i++) {
      clap_event_param_value_t *e = &evs.ev[evs.n++];
      memset(e, 0, sizeof *e);
      e->header = (clap_event_header_t){sizeof *e, 0, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_PARAM_VALUE, 0};
      e->param_id = ids[i], e->note_id = -1, e->port_index = -1, e->channel = -1, e->key = -1;
      e->value = i < NP ? (double)k->v[i] : (k->bypass ? 1.0 : 0.0);
    }
    clap_input_events_t in = {&evs, ev_size, ev_get};
    float *ib[2] = {(float *)il + at, (float *)ir + at}, *ob[2] = {ol + at, orr + at};
    clap_audio_buffer_t ai = {ib, NULL, 2, 0, 0}, ao = {ob, NULL, 2, 0, 0};
    clap_process_t pr = {.steady_time = at, .frames_count = k->frames, .audio_inputs = &ai, .audio_outputs = &ao,
                         .audio_inputs_count = 1, .audio_outputs_count = 1, .in_events = &in, .out_events = &out};
    ok = p->process(p, &pr) != CLAP_PROCESS_ERROR;
    at += k->frames;
  }
  if (ok && latency) *latency = lat->get(p);
  p->stop_processing(p);
  p->deactivate(p);
  p->destroy(p);
  return ok;
}

static int clap_main(const char *path) {
  void *lib = dlopen(path, RTLD_NOW | RTLD_LOCAL);
  if (!lib) return fprintf(stderr, "FAIL dlopen %s: %s\n", path, dlerror()), 1;
  ClapFace c = {(const clap_plugin_entry_t *)dlsym(lib, "clap_entry"), NULL};
  if (!c.entry || !c.entry->init(path)) return fprintf(stderr, "FAIL clap_entry %s\n", path), 1;
  c.factory = (const clap_plugin_factory_t *)c.entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
  if (!c.factory || c.factory->get_plugin_count(c.factory) != 1) return fprintf(stderr, "FAIL factory\n"), 1;
  Driver d = {clap_run, &c};
  for (uint32_t i = 0; i < OMX_DECLARED_RATE_COUNT; i++) run_checks("clap", &d, (double)OMX_DECLARED_RATES[i]);
  c.entry->deinit();
  dlclose(lib);
  return failures ? 1 : 0;
}

/* ---- LV2 ------------------------------------------------------------------------------------ */

typedef struct {
  LilvWorld *world;
  const LilvPlugin *plugin;
} Lv2Face;

static int32_t port_of(const Lv2Face *l, const char *symbol) {
  LilvNode *sym = lilv_new_string(l->world, symbol);
  const LilvPort *port = lilv_plugin_get_port_by_symbol(l->plugin, sym);
  lilv_node_free(sym);
  if (!port) fprintf(stderr, "  no LV2 port '%s'\n", symbol);
  return port ? (int32_t)lilv_port_get_index(l->plugin, port) : -1;
}

static bool lv2_run(void *face, double sr, const float *il, const float *ir, float *ol, float *orr,
                    uint32_t *latency) {
  Lv2Face *l = (Lv2Face *)face;
  LilvInstance *inst = lilv_plugin_instantiate(l->plugin, sr, NULL);
  if (!inst) return false;
  const char *audio[] = {"in_l", "in_r", "out_l", "out_r"};
  int32_t ap[4], cp[NP + 2];
  float cv[NP + 2] = {0};
  bool ok = true;
  for (int i = 0; i < 4; i++) ok = (ap[i] = port_of(l, audio[i])) >= 0 && ok;
  for (uint32_t i = 0; i < NP; i++) ok = (cp[i] = port_of(l, OMX_TRANSIENT_PARAMS[i].symbol)) >= 0 && ok;
  ok = (cp[NP] = port_of(l, "enabled")) >= 0 && ok;
  ok = (cp[NP + 1] = port_of(l, "latency")) >= 0 && ok;
  for (uint32_t i = 0; ok && i < NP + 2; i++) lilv_instance_connect_port(inst, (uint32_t)cp[i], &cv[i]);
  if (ok) lilv_instance_activate(inst);
  uint32_t at = 0;
  for (size_t b = 0; ok && b < NBLOCKS; b++) {
    const Block *k = &PLAN[b];
    for (uint32_t i = 0; i < NP; i++) cv[i] = k->v[i];
    cv[NP] = k->bypass ? 0.0f : 1.0f;
    lilv_instance_connect_port(inst, (uint32_t)ap[0], (void *)(il + at));
    lilv_instance_connect_port(inst, (uint32_t)ap[1], (void *)(ir + at));
    lilv_instance_connect_port(inst, (uint32_t)ap[2], ol + at);
    lilv_instance_connect_port(inst, (uint32_t)ap[3], orr + at);
    lilv_instance_run(inst, k->frames);
    at += k->frames;
  }
  if (ok) lilv_instance_deactivate(inst);
  lilv_instance_free(inst);
  if (ok && latency) *latency = (uint32_t)(cv[NP + 1] + 0.5f);
  return ok;
}

static int lv2_main(const char *bundle, const char *uri) {
  Lv2Face l = {lilv_world_new(), NULL};
  char dir[4096];
  snprintf(dir, sizeof dir, "%s%s", bundle, bundle[strlen(bundle) - 1] == '/' ? "" : "/");
  LilvNode *b = lilv_new_file_uri(l.world, NULL, dir);
  lilv_world_load_bundle(l.world, b);
  LilvNode *u = lilv_new_uri(l.world, uri);
  l.plugin = lilv_plugins_get_by_uri(lilv_world_get_all_plugins(l.world), u);
  if (!l.plugin) return fprintf(stderr, "FAIL no LV2 plugin %s in %s\n", uri, dir), 1;
  Driver d = {lv2_run, &l};
  for (uint32_t i = 0; i < OMX_DECLARED_RATE_COUNT; i++) run_checks("lv2", &d, (double)OMX_DECLARED_RATES[i]);
  lilv_node_free(u), lilv_node_free(b);
  lilv_world_free(l.world);
  return failures ? 1 : 0;
}

int main(int argc, char **argv) {
  int rc;
  if (argc == 3 && strcmp(argv[1], "clap") == 0) rc = clap_main(argv[2]);
  else if (argc == 4 && strcmp(argv[1], "lv2") == 0) rc = lv2_main(argv[2], argv[3]);
  else return fprintf(stderr, "usage: transient-oracle clap <x.clap> | lv2 <bundle-dir> <uri>\n"), 2;
  printf("%s %s: %d failure(s)\n", rc ? "ORACLE-RED" : "ORACLE-GREEN", argv[1], failures);
  return rc;
}
