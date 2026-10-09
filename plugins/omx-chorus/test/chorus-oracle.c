// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * chorus-oracle.c — omx-chorus's kernel-identity test: both built faces, sample for sample, against
 * omx-dsp's chorus kernel called DIRECTLY on the same blocks, at every declared rate, engaged and
 * bypassed — no tolerance.
 *
 *   chorus-oracle clap <omx-chorus.clap>
 *   chorus-oracle lv2 <omx-chorus.lv2> <uri>
 *
 * The signal is deterministic noise in blocks of BLOCK frames; the parameters change at SWITCH_AT, in
 * the middle of the stream, so the resolve path is exercised on a running instance. The reference is
 * omx-dsp's chorus instance (<omxdsp/fx/omx_chorus_instance.h>) with rings of its own, resolved and
 * run block by block with the same values; the face's output must equal it bit for bit (memcmp).
 * Parameters are found BY NAME (CLAP) or BY SYMBOL (LV2, through lilv), so a renumbered face cannot
 * pass by accident.
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
#include <omxcontract/omx_contract_limits.h>
#include <omxdsp/fx/omx_chorus_instance.h>

static const double RATES[] = OMX_ORACLE_FLOOR_RATES_INIT; /* the four rates every kernel is judged at */
#define BLOCK 512u
#define FRAMES 8192u
#define SWITCH_AT (4u * BLOCK) /* the parameters change here, mid-stream */

typedef struct {
  double rate_hz, depth_ms, voices, mix, spread, bypass;
} Setting;

static int failures;

typedef struct Driver Driver;
static void run_checks(const char *face, Driver *d, double sr);

static void check(bool ok, const char *face, double sr, const char *what) {
  if (!ok) failures++;
  printf("%s %s %s @ %.0f Hz\n", ok ? "PASS" : "FAIL", face, what, sr);
}

static void noise(float *l, float *r, uint32_t n, uint32_t seed) {
  for (uint32_t i = 0; i < n; i++) {
    seed = seed * 1664525u + 1013904223u;
    l[i] = ((float)(seed >> 8) / 16777216.0f - 0.5f) * 1.8f;
    seed = seed * 1664525u + 1013904223u;
    r[i] = ((float)(seed >> 8) / 16777216.0f - 0.5f) * 1.8f;
  }
}

/* ---- the two faces, behind one interface ------------------------------------------------- */

struct Driver {
  /* run `frames` of stereo input through a FRESH instance under s[0], then s[1] from SWITCH_AT; `*latency` is read back. */
  bool (*run)(void *face, double sr, const Setting s[2], const float *il, const float *ir, float *ol, float *orr,
             uint32_t frames, uint32_t *latency);
  void *face;
};

/* ---- CLAP ---------------------------------------------------------------------------------- */

static const void *h_ext(const clap_host_t *h, const char *id) {
  (void)h, (void)id;
  return NULL;
}
static void h_noop(const clap_host_t *h) { (void)h; }
static const clap_host_t HOST = {CLAP_VERSION_INIT, NULL, "chorus-oracle", "openmixer", "", "0.1", h_ext, h_noop, h_noop, h_noop};

typedef struct {
  clap_event_param_value_t ev[8];
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

static bool clap_run(void *face, double sr, const Setting s[2], const float *il, const float *ir, float *ol, float *orr,
                     uint32_t frames, uint32_t *latency) {
  ClapFace *c = (ClapFace *)face;
  const clap_plugin_descriptor_t *desc = c->factory->get_plugin_descriptor(c->factory, 0);
  const clap_plugin_t *p = c->factory->create_plugin(c->factory, &HOST, desc->id);
  if (!p || !p->init(p)) return false;
  const clap_plugin_params_t *pp = (const clap_plugin_params_t *)p->get_extension(p, CLAP_EXT_PARAMS);
  const clap_plugin_latency_t *lat = (const clap_plugin_latency_t *)p->get_extension(p, CLAP_EXT_LATENCY);
  bool ok = pp && lat && p->activate(p, sr, 1, BLOCK) && p->start_processing(p);
  const char *names[] = {"Rate", "Depth", "Voices", "Mix", "Spread", NULL};
  Events evs[2] = {{.n = 0}, {.n = 0}};
  for (int k = 0; k < 2; k++) {
    const double vals[] = {s[k].rate_hz, s[k].depth_ms, s[k].voices, s[k].mix, s[k].spread, s[k].bypass};
    for (size_t i = 0; ok && i < 6; i++) {
      clap_id id;
      ok = clap_param(pp, p, names[i], &id);
      clap_event_param_value_t *e = &evs[k].ev[evs[k].n++];
      memset(e, 0, sizeof *e);
      e->header = (clap_event_header_t){sizeof *e, 0, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_PARAM_VALUE, 0};
      e->param_id = ok ? id : 0, e->note_id = -1, e->port_index = -1, e->channel = -1, e->key = -1, e->value = vals[i];
    }
  }
  clap_input_events_t in0 = {&evs[0], ev_size, ev_get}, in1 = {&evs[1], ev_size, ev_get};
  Events none = {.n = 0};
  clap_input_events_t in_none = {&none, ev_size, ev_get};
  clap_output_events_t out = {NULL, ev_push};
  for (uint32_t f = 0; ok && f < frames; f += BLOCK) {
    const uint32_t n = frames - f < BLOCK ? frames - f : BLOCK;
    float *ib[2] = {(float *)il + f, (float *)ir + f}, *ob[2] = {ol + f, orr + f};
    clap_audio_buffer_t ai = {ib, NULL, 2, 0, 0}, ao = {ob, NULL, 2, 0, 0};
    clap_process_t pr = {.steady_time = f, .frames_count = n, .audio_inputs = &ai, .audio_outputs = &ao,
                         .audio_inputs_count = 1, .audio_outputs_count = 1, .in_events = f == 0 ? &in0 : f == SWITCH_AT ? &in1 : &in_none,
                         .out_events = &out};
    ok = p->process(p, &pr) != CLAP_PROCESS_ERROR;
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
  for (size_t i = 0; i < sizeof RATES / sizeof RATES[0]; i++) run_checks("clap", &d, RATES[i]);
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

static bool lv2_run(void *face, double sr, const Setting s[2], const float *il, const float *ir, float *ol, float *orr,
                    uint32_t frames, uint32_t *latency) {
  Lv2Face *l = (Lv2Face *)face;
  LilvInstance *inst = lilv_plugin_instantiate(l->plugin, sr, NULL);
  if (!inst) return false;
  const char *audio[] = {"in_l", "in_r", "out_l", "out_r"};
  const char *ctl[] = {"rateHz", "depthMs", "voices", "mix", "spread", "enabled", "latency"};
  float cv[7] = {0};
#define LV2_SET(k) \
  cv[0] = (float)s[k].rate_hz, cv[1] = (float)s[k].depth_ms, cv[2] = (float)s[k].voices, cv[3] = (float)s[k].mix, \
  cv[4] = (float)s[k].spread, cv[5] = s[k].bypass > 0.5 ? 0.0f : 1.0f
  LV2_SET(0);
  int32_t ap[4];
  bool ok = true;
  for (int i = 0; i < 4; i++) ok = (ap[i] = port_of(l, audio[i])) >= 0 && ok;
  for (int i = 0; i < 7; i++) {
    int32_t idx = port_of(l, ctl[i]);
    ok = idx >= 0 && ok;
    if (idx >= 0) lilv_instance_connect_port(inst, (uint32_t)idx, &cv[i]);
  }
  if (ok) lilv_instance_activate(inst);
  for (uint32_t f = 0; ok && f < frames; f += BLOCK) {
    const uint32_t n = frames - f < BLOCK ? frames - f : BLOCK;
    if (f == SWITCH_AT) LV2_SET(1);
    lilv_instance_connect_port(inst, (uint32_t)ap[0], (void *)(il + f));
    lilv_instance_connect_port(inst, (uint32_t)ap[1], (void *)(ir + f));
    lilv_instance_connect_port(inst, (uint32_t)ap[2], ol + f);
    lilv_instance_connect_port(inst, (uint32_t)ap[3], orr + f);
    lilv_instance_run(inst, n);
  }
  if (ok) lilv_instance_deactivate(inst);
  lilv_instance_free(inst);
  if (ok && latency) *latency = (uint32_t)(cv[6] + 0.5f);
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
  for (size_t i = 0; i < sizeof RATES / sizeof RATES[0]; i++) run_checks("lv2", &d, RATES[i]);
  lilv_node_free(u), lilv_node_free(b);
  lilv_world_free(l.world);
  return failures ? 1 : 0;
}

/* ---- the reference: omx-dsp's chorus instance, called directly -------------------------------- */

/** The kernel's own answer: the same blocks, the same values, rings of the reference's own. */
static bool reference(double sr, const Setting s[2], const float *il, const float *ir, float *ol, float *orr,
                      uint32_t frames) {
  const uint32_t cap = omx_chorus_instance_cap_for((float)sr);
  float *rl = calloc(cap, sizeof(float)), *rr = calloc(cap, sizeof(float));
  OmxChorusInstance inst;
  bool ok = rl && rr && omx_chorus_instance_init(&inst, (float)sr, rl, rr, cap);
  for (uint32_t f = 0; ok && f < frames; f += BLOCK) {
    const uint32_t n = frames - f < BLOCK ? frames - f : BLOCK;
    const Setting *k = f >= SWITCH_AT ? &s[1] : &s[0];
    omx_chorus_instance_resolve(&inst, k->bypass > 0.5, (float)k->voices, (float)k->depth_ms, (float)k->rate_hz,
                                (float)k->mix, (float)k->spread);
    omx_chorus_instance_run(&inst, il + f, ir + f, ol + f, orr + f, n);
  }
  free(rl), free(rr);
  return ok;
}

/** The face's output equals the kernel's, bit for bit, under settings `s`. */
static void identical(const char *face, Driver *d, double sr, const Setting s[2], const char *what, const float *il,
                      const float *ir, float *ol, float *orr, float *wl, float *wr) {
  uint32_t lat = ~0u;
  bool ok = d->run(d->face, sr, s, il, ir, ol, orr, FRAMES, &lat) && reference(sr, s, il, ir, wl, wr, FRAMES) &&
            lat == (uint32_t)OMX_CHORUS_INSTANCE_LATENCY_FRAMES && memcmp(ol, wl, FRAMES * sizeof(float)) == 0 &&
            memcmp(orr, wr, FRAMES * sizeof(float)) == 0;
  check(ok, face, sr, what);
}

static void run_checks(const char *face, Driver *d, double sr) {
  float *il = calloc(FRAMES, sizeof(float)), *ir = calloc(FRAMES, sizeof(float));
  float *ol = calloc(FRAMES, sizeof(float)), *orr = calloc(FRAMES, sizeof(float));
  float *wl = calloc(FRAMES, sizeof(float)), *wr = calloc(FRAMES, sizeof(float));
  if (!il || !ir || !ol || !orr || !wl || !wr) abort();
  noise(il, ir, FRAMES, 0x6f6d78u);

  /* the defaults, then values across the declared travel, with a change mid-stream. */
  const Setting engaged[] = {
    {0.6, 4.0, 3.0, 35.0, 0.0, 0.0},  {0.6, 4.0, 3.0, 35.0, 0.0, 0.0},
    {8.0, 12.0, 4.0, 100.0, 0.5, 0.0}, {0.05, 0.0, 1.0, 0.0, 0.0, 0.0},
    {2.5, 7.5, 2.0, 60.0, 0.25, 0.0},  {5.0, 10.0, 4.0, 80.0, 0.5, 0.0},
  };
  const char *names[] = {"defaults throughout", "fast, deep, 4 voices, wet, wide, then slow, shallow, 1 voice, dry",
                         "mid values, then fast, deep, 4 voices"};
  for (int i = 0; i < 3; i++) identical(face, d, sr, &engaged[2 * i], names[i], il, ir, ol, orr, wl, wr);

  /* bypass, and bypass lifted or set mid-stream (the kernel clears its rings on re-engage). */
  const Setting by[2] = {{2.5, 7.5, 3.0, 60.0, 0.25, 1.0}, {2.5, 7.5, 3.0, 60.0, 0.25, 1.0}};
  identical(face, d, sr, by, "bypassed throughout", il, ir, ol, orr, wl, wr);
  check(memcmp(ol, il, FRAMES * sizeof(float)) == 0 && memcmp(orr, ir, FRAMES * sizeof(float)) == 0, face, sr,
        "bypass is the identity");
  const Setting re[2] = {{2.5, 7.5, 3.0, 60.0, 0.25, 1.0}, {2.5, 7.5, 3.0, 60.0, 0.25, 0.0}};
  identical(face, d, sr, re, "bypassed, then engaged mid-stream", il, ir, ol, orr, wl, wr);
  const Setting out[2] = {{2.5, 7.5, 3.0, 60.0, 0.25, 0.0}, {2.5, 7.5, 3.0, 60.0, 0.25, 1.0}};
  identical(face, d, sr, out, "engaged, then bypassed mid-stream", il, ir, ol, orr, wl, wr);

  free(il), free(ir), free(ol), free(orr), free(wl), free(wr);
}

int main(int argc, char **argv) {
  int rc;
  if (argc == 3 && strcmp(argv[1], "clap") == 0) rc = clap_main(argv[2]);
  else if (argc == 4 && strcmp(argv[1], "lv2") == 0) rc = lv2_main(argv[2], argv[3]);
  else return fprintf(stderr, "usage: chorus-oracle clap <x.clap> | lv2 <bundle-dir> <uri>\n"), 2;
  printf("%s %s: %d failure(s)\n", rc ? "ORACLE-RED" : "ORACLE-GREEN", argv[1], failures);
  return rc;
}
