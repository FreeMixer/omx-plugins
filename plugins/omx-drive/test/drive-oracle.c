// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * drive-oracle.c — what a drive stage MUST do, checked on the BUILT plugin at 44.1, 48, 96 and
 * 192 kHz. Unlike a delay's impulse response, a waveshaper has no closed-form "expected sample"
 * independent of its own curve — so this is not a from-the-definition oracle (delay-oracle.c's
 * kind). It checks the stage's DOCUMENTED, kernel-level laws (mix_drive.h's own header, carried
 * unchanged into omxdsp's installed <omxdsp/fx/omx_drive.h>) hold through the shell:
 *
 *   drive-oracle clap <omx-drive.clap>
 *   drive-oracle lv2 <bundle-dir> <uri>
 *
 *   - bypassed, output is input, bytes.
 *   - at mix 0 and trim 0 dB, output is input delayed by exactly the plugin's own published
 *     latency, bytes — "mix = 0 stays bit-identical dry at every factor and every band".
 *   - the declared `bandHz` control is LIVE: two bandHz settings a decade apart, drive and
 *     character both away from their no-op points, give a DIFFERENT output — the one way the
 *     shell could silently drop a parameter without any of the above catching it.
 *   - at the full declared travel (36 dB drive, +-1 character, 100% mix, +12 dB trim) on a
 *     full-scale tone, every output sample is finite — the curve's own bound (|out| < 1 before
 *     trim) carried through a shell that is free to compute trim and mix however it likes.
 *
 * Parameters are found BY NAME (CLAP) or BY SYMBOL (LV2, through lilv), so a renumbered face
 * cannot pass by accident. This file includes no DSP header of its own.
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

static const double RATES[] = OMX_ORACLE_FLOOR_RATES_INIT; /* the four rates every kernel is judged at */
#define BLOCK 512u
#define FRAMES 8192u

typedef struct {
  double drive_db, character, band_hz, mix, trim_db, bypass;
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
  /* run `frames` of stereo input through a FRESH instance under `s`; `*latency` is read back. */
  bool (*run)(void *face, double sr, const Setting *s, const float *il, const float *ir, float *ol, float *orr,
             uint32_t frames, uint32_t *latency);
  void *face;
};

/* ---- CLAP ---------------------------------------------------------------------------------- */

static const void *h_ext(const clap_host_t *h, const char *id) {
  (void)h, (void)id;
  return NULL;
}
static void h_noop(const clap_host_t *h) { (void)h; }
static const clap_host_t HOST = {CLAP_VERSION_INIT, NULL, "drive-oracle", "openmixer", "", "0.1", h_ext, h_noop, h_noop, h_noop};

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

static bool clap_run(void *face, double sr, const Setting *s, const float *il, const float *ir, float *ol, float *orr,
                     uint32_t frames, uint32_t *latency) {
  ClapFace *c = (ClapFace *)face;
  const clap_plugin_descriptor_t *desc = c->factory->get_plugin_descriptor(c->factory, 0);
  const clap_plugin_t *p = c->factory->create_plugin(c->factory, &HOST, desc->id);
  if (!p || !p->init(p)) return false;
  const clap_plugin_params_t *pp = (const clap_plugin_params_t *)p->get_extension(p, CLAP_EXT_PARAMS);
  const clap_plugin_latency_t *lat = (const clap_plugin_latency_t *)p->get_extension(p, CLAP_EXT_LATENCY);
  bool ok = pp && lat && p->activate(p, sr, 1, BLOCK) && p->start_processing(p);
  Events evs = {.n = 0};
  const char *names[] = {"Drive", "Character", "Band", "Mix", "Trim", NULL};
  const double vals[] = {s->drive_db, s->character, s->band_hz, s->mix, s->trim_db, s->bypass};
  for (size_t i = 0; ok && i < 6; i++) {
    clap_id id;
    ok = clap_param(pp, p, names[i], &id);
    clap_event_param_value_t *e = &evs.ev[evs.n++];
    memset(e, 0, sizeof *e);
    e->header = (clap_event_header_t){sizeof *e, 0, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_PARAM_VALUE, 0};
    e->param_id = ok ? id : 0, e->note_id = -1, e->port_index = -1, e->channel = -1, e->key = -1, e->value = vals[i];
  }
  clap_input_events_t in = {&evs, ev_size, ev_get};
  Events none = {.n = 0};
  clap_input_events_t in_none = {&none, ev_size, ev_get};
  clap_output_events_t out = {NULL, ev_push};
  for (uint32_t f = 0; ok && f < frames; f += BLOCK) {
    const uint32_t n = frames - f < BLOCK ? frames - f : BLOCK;
    float *ib[2] = {(float *)il + f, (float *)ir + f}, *ob[2] = {ol + f, orr + f};
    clap_audio_buffer_t ai = {ib, NULL, 2, 0, 0}, ao = {ob, NULL, 2, 0, 0};
    clap_process_t pr = {.steady_time = f, .frames_count = n, .audio_inputs = &ai, .audio_outputs = &ao,
                         .audio_inputs_count = 1, .audio_outputs_count = 1, .in_events = f ? &in_none : &in,
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

static bool lv2_run(void *face, double sr, const Setting *s, const float *il, const float *ir, float *ol, float *orr,
                    uint32_t frames, uint32_t *latency) {
  Lv2Face *l = (Lv2Face *)face;
  LilvInstance *inst = lilv_plugin_instantiate(l->plugin, sr, NULL);
  if (!inst) return false;
  const char *audio[] = {"in_l", "in_r", "out_l", "out_r"};
  const char *ctl[] = {"driveDb", "character", "bandHz", "mix", "trimDb", "enabled", "latency"};
  float cv[7] = {(float)s->drive_db, (float)s->character, (float)s->band_hz, (float)s->mix, (float)s->trim_db,
                 s->bypass > 0.5 ? 0.0f : 1.0f, 0.0f};
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

/* ---- the checks, written once against the Driver interface ------------------------------- */

static void run_checks(const char *face, Driver *d, double sr) {
  float *il = calloc(FRAMES, sizeof(float)), *ir = calloc(FRAMES, sizeof(float));
  float *ol = calloc(FRAMES, sizeof(float)), *orr = calloc(FRAMES, sizeof(float));
  if (!il || !ir || !ol || !orr) abort();
  noise(il, ir, FRAMES, 0x6f6d78u);

  /* bypass: output is input. */
  {
    Setting by = {18.0, 0.5, 2000.0, 70.0, 3.0, 1.0};
    uint32_t lat = 0;
    bool ok = d->run(d->face, sr, &by, il, ir, ol, orr, FRAMES, &lat) &&
              memcmp(ol, il, FRAMES * sizeof(float)) == 0 && memcmp(orr, ir, FRAMES * sizeof(float)) == 0;
    check(ok, face, sr, "bypass is the identity");
  }

  /* mix 0, trim 0 dB: output is input delayed by the plugin's own published latency. */
  {
    Setting dry = {24.0, 0.7, 500.0, 0.0, 0.0, 0.0};
    uint32_t lat = 0;
    bool ok = d->run(d->face, sr, &dry, il, ir, ol, orr, FRAMES, &lat) && lat < FRAMES;
    if (ok) {
      ok = memcmp(ol + lat, il, (FRAMES - lat) * sizeof(float)) == 0 &&
           memcmp(orr + lat, ir, (FRAMES - lat) * sizeof(float)) == 0;
    }
    char what[80];
    snprintf(what, sizeof what, "mix 0 is the delayed identity (latency %u)", lat);
    check(ok, face, sr, what);
  }

  /* the declared bandHz control is live. */
  {
    Setting lo = {24.0, 0.8, 80.0, 100.0, 0.0, 0.0};
    Setting hi = lo;
    hi.band_hz = 12000.0;
    float *ol2 = calloc(FRAMES, sizeof(float)), *or2 = calloc(FRAMES, sizeof(float));
    uint32_t lat = 0;
    bool ok = ol2 && or2 && d->run(d->face, sr, &lo, il, ir, ol, orr, FRAMES, &lat) &&
              d->run(d->face, sr, &hi, il, ir, ol2, or2, FRAMES, &lat) &&
              (memcmp(ol, ol2, FRAMES * sizeof(float)) != 0 || memcmp(orr, or2, FRAMES * sizeof(float)) != 0);
    check(ok, face, sr, "bandHz changes the output");
    free(ol2), free(or2);
  }

  /* full travel: every output sample is finite. */
  {
    const uint32_t n = 4096u;
    for (uint32_t i = 0; i < n; i++) {
      il[i] = sinf(2.0f * 3.14159265f * 300.0f * (float)i / (float)sr);
      ir[i] = -il[i];
    }
    Setting hot = {36.0, 1.0, 20000.0, 100.0, 12.0, 0.0};
    uint32_t lat = 0;
    bool ok = d->run(d->face, sr, &hot, il, ir, ol, orr, n, &lat);
    for (uint32_t i = 0; ok && i < n; i++) ok = isfinite(ol[i]) && isfinite(orr[i]);
    check(ok, face, sr, "full travel stays finite");
  }

  free(il), free(ir), free(ol), free(orr);
}

int main(int argc, char **argv) {
  int rc;
  if (argc == 3 && strcmp(argv[1], "clap") == 0) rc = clap_main(argv[2]);
  else if (argc == 4 && strcmp(argv[1], "lv2") == 0) rc = lv2_main(argv[2], argv[3]);
  else return fprintf(stderr, "usage: drive-oracle clap <x.clap> | lv2 <bundle-dir> <uri>\n"), 2;
  printf("%s %s: %d failure(s)\n", rc ? "ORACLE-RED" : "ORACLE-GREEN", argv[1], failures);
  return rc;
}
