// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * phaser-oracle.c — omx-phaser's kernel-identity test: both built faces, sample for sample, against
 * omx-dsp's phaser kernel called DIRECTLY on the same blocks, at every declared rate, engaged and
 * bypassed — no tolerance.
 *
 *   phaser-oracle clap <omx-phaser.clap>
 *   phaser-oracle lv2 <omx-phaser.lv2> <uri>
 *
 * The face is reached the way a host reaches it (dlopen and clap_entry, or lilv), never by including
 * it. A deterministic stereo signal runs through uneven blocks while the controls move between
 * them: the declared defaults, the extremes of every travel, an odd Stages (the kernel runs the even
 * count below it), a Mix of 0 and of 100, and bypass on and off. The reference calls
 * omx_phaser_process() from <omxdsp/fx/omx_phaser.h> on the atom written out here from the control
 * values, one block at a time, on one state carried through the programme. Equal bytes prove the
 * face calls the kernel, every control lands on the same atom, and the state carries across a host's
 * block boundary. The published latency is the kernel's: none.
 *
 * The parameters are found BY NAME (CLAP) or BY SYMBOL (LV2), so a renumbered face cannot pass by
 * accident.
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

#include "omx_phaser_params.h"
#include <omxdsp/fx/omx_phaser.h>

static const double RATES[] = OMX_ORACLE_FLOOR_RATES_INIT; /* the rates every kernel is judged at */
#define NP OMX_PHASER_PARAM_COUNT

typedef struct {
  float v[NP];
  int bypass;
  uint32_t frames; /* this segment's length, in frames */
} Segment;

/* Rate, Base, Depth, Stages, Feedback, Mix — in declaration order. */
static const Segment PROGRAMME[] = {
    {{0.5f, 200.0f, 4.0f, 6.0f, 0.4f, 50.0f}, 0, 2000},    /* the declared defaults */
    {{5.0f, 2000.0f, 6.0f, 12.0f, 0.9f, 100.0f}, 0, 3000}, /* the top of every travel */
    {{0.05f, 50.0f, 0.0f, 2.0f, -0.9f, 100.0f}, 0, 3000},  /* the bottom, feedback deepest negative */
    {{1.3f, 700.0f, 2.5f, 7.0f, -0.45f, 73.0f}, 0, 4000},  /* an odd Stages: the kernel runs 6 */
    {{2.0f, 300.0f, 3.0f, 8.0f, 0.6f, 100.0f}, 1, 1500},   /* bypassed */
    {{2.0f, 300.0f, 3.0f, 8.0f, 0.6f, 100.0f}, 0, 3000},   /* engaged again, same controls */
    {{0.8f, 120.0f, 5.0f, 4.0f, 0.7f, 0.0f}, 0, 1500},     /* Mix 0: bit-identical dry */
    {{0.8f, 120.0f, 5.0f, 4.0f, 0.7f, 35.0f}, 0, 3000},    /* and back */
};
#define SEGMENTS (sizeof PROGRAMME / sizeof PROGRAMME[0])
static const uint32_t BLOCKS[] = {37, 256, 101, 512, 64, 333, 1, 480}; /* uneven; cycled */
#define NBLOCKS (sizeof BLOCKS / sizeof BLOCKS[0])

static int failures, checks;
struct Driver;
static void run_checks(const char *face, struct Driver *d, double sr);

static void check(bool ok, const char *face, double sr, const char *what) {
  checks++;
  if (!ok) failures++;
  printf("%s kernel-identity %s %s @ %.1f Hz\n", ok ? "PASS" : "FAIL", face, what, sr);
}

static uint32_t total_frames(void) {
  uint32_t t = 0;
  for (size_t s = 0; s < SEGMENTS; s++) t += PROGRAMME[s].frames;
  return t;
}

static void signal(float *l, float *r, uint32_t n, double sr) {
  uint32_t seed = 0x70686173u;
  for (uint32_t i = 0; i < n; i++) {
    seed = seed * 1664525u + 1013904223u;
    const float nz = ((float)(seed >> 8) / 16777216.0f - 0.5f) * 0.4f;
    const float t = (float)((double)i / sr);
    l[i] = 0.5f * sinf(6.2831853f * 220.0f * t) + nz;
    r[i] = 0.4f * sinf(6.2831853f * 330.0f * t + 0.7f) - nz;
  }
}

/* ---- the reference: the kernel, called directly ----------------------------------------- */

static void reference(double sr, const float *il, const float *ir, float *ol, float *orr, uint32_t frames) {
  struct omx_phaser_state st;
  omx_phaser_state_init(&st);
  uint32_t pos = 0, b = 0;
  for (size_t s = 0; s < SEGMENTS; s++) {
    const Segment *g = &PROGRAMME[s];
    struct omx_phaser p = {
        .enabled = !g->bypass,
        .stages = omx_phaser_stages_run((int)lrintf(g->v[OMX_PHASER_PARAM_STAGES])),
        .base_hz = g->v[OMX_PHASER_PARAM_BASE],
        .depth_oct = g->v[OMX_PHASER_PARAM_DEPTH],
        .lfo_inc = omx_lfo_inc(g->v[OMX_PHASER_PARAM_RATE], (float)sr),
        .feedback = g->v[OMX_PHASER_PARAM_FEEDBACK],
        .mix = g->v[OMX_PHASER_PARAM_MIX] * 0.01f,
    };
    for (uint32_t f = 0; f < g->frames && pos < frames; b++) {
      uint32_t n = BLOCKS[b % NBLOCKS];
      if (n > g->frames - f) n = g->frames - f;
      memcpy(ol + pos, il + pos, n * sizeof(float));
      memcpy(orr + pos, ir + pos, n * sizeof(float));
      omx_phaser_process(ol + pos, orr + pos, n, &p, &st, (float)sr);
      pos += n, f += n;
    }
  }
}

/* ---- the two faces, behind one interface ------------------------------------------------ */

typedef struct Driver {
  /* run the programme through a FRESH instance; `*latency` is read back. */
  bool (*run)(void *face, double sr, const float *il, const float *ir, float *ol, float *orr, uint32_t frames,
              uint32_t *latency);
  void *face;
} Driver;

/* ---- CLAP ------------------------------------------------------------------------------- */

static const void *h_ext(const clap_host_t *h, const char *id) {
  (void)h, (void)id;
  return NULL;
}
static void h_noop(const clap_host_t *h) { (void)h; }
static const clap_host_t HOST = {CLAP_VERSION_INIT, NULL, "phaser-oracle", "openmixer", "", "0.1",
                                 h_ext, h_noop, h_noop, h_noop};

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

/* The id of the parameter named `name`, or of the bypass parameter when `name` is NULL. */
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

static bool clap_run(void *face, double sr, const float *il, const float *ir, float *ol, float *orr, uint32_t frames,
                     uint32_t *latency) {
  ClapFace *c = (ClapFace *)face;
  const clap_plugin_descriptor_t *desc = c->factory->get_plugin_descriptor(c->factory, 0);
  const clap_plugin_t *p = c->factory->create_plugin(c->factory, &HOST, desc->id);
  if (!p || !p->init(p)) return false;
  const clap_plugin_params_t *pp = (const clap_plugin_params_t *)p->get_extension(p, CLAP_EXT_PARAMS);
  const clap_plugin_latency_t *lat = (const clap_plugin_latency_t *)p->get_extension(p, CLAP_EXT_LATENCY);
  bool ok = pp && lat && p->activate(p, sr, 1, 1024) && p->start_processing(p);
  clap_id ids[NP], bypass_id = 0;
  for (uint32_t i = 0; ok && i < NP; i++) ok = clap_param(pp, p, OMX_PHASER_PARAMS[i].name, &ids[i]);
  ok = ok && clap_param(pp, p, NULL, &bypass_id);
  uint32_t pos = 0, b = 0;
  for (size_t s = 0; ok && s < SEGMENTS; s++) {
    const Segment *g = &PROGRAMME[s];
    Events evs = {.n = 0};
    for (uint32_t i = 0; i <= NP; i++) {
      clap_event_param_value_t *e = &evs.ev[evs.n++];
      memset(e, 0, sizeof *e);
      e->header = (clap_event_header_t){sizeof *e, 0, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_PARAM_VALUE, 0};
      e->param_id = i < NP ? ids[i] : bypass_id, e->note_id = -1, e->port_index = -1, e->channel = -1, e->key = -1;
      e->value = i < NP ? (double)g->v[i] : (g->bypass ? 1.0 : 0.0);
    }
    Events none = {.n = 0};
    clap_input_events_t in = {&evs, ev_size, ev_get}, in_none = {&none, ev_size, ev_get};
    clap_output_events_t out = {NULL, ev_push};
    for (uint32_t f = 0; ok && f < g->frames && pos < frames; b++) {
      uint32_t n = BLOCKS[b % NBLOCKS];
      if (n > g->frames - f) n = g->frames - f;
      float *ib[2] = {(float *)il + pos, (float *)ir + pos}, *ob[2] = {ol + pos, orr + pos};
      clap_audio_buffer_t ai = {ib, NULL, 2, 0, 0}, ao = {ob, NULL, 2, 0, 0};
      clap_process_t pr = {.steady_time = pos, .frames_count = n, .audio_inputs = &ai, .audio_outputs = &ao,
                           .audio_inputs_count = 1, .audio_outputs_count = 1,
                           .in_events = f ? &in_none : &in, .out_events = &out};
      ok = p->process(p, &pr) != CLAP_PROCESS_ERROR;
      pos += n, f += n;
    }
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

/* ---- LV2 -------------------------------------------------------------------------------- */

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

static bool lv2_run(void *face, double sr, const float *il, const float *ir, float *ol, float *orr, uint32_t frames,
                    uint32_t *latency) {
  Lv2Face *l = (Lv2Face *)face;
  LilvInstance *inst = lilv_plugin_instantiate(l->plugin, sr, NULL);
  if (!inst) return false;
  const char *audio[] = {"in_l", "in_r", "out_l", "out_r"};
  int32_t ap[4], pp[NP], en_port, lat_port;
  float cv[NP], enabled = 1.0f, lat_out = 0.0f;
  bool ok = true;
  for (int i = 0; i < 4; i++) ok = (ap[i] = port_of(l, audio[i])) >= 0 && ok;
  for (uint32_t i = 0; i < NP; i++) {
    ok = (pp[i] = port_of(l, OMX_PHASER_PARAMS[i].symbol)) >= 0 && ok;
    cv[i] = OMX_PHASER_PARAMS[i].def;
    if (pp[i] >= 0) lilv_instance_connect_port(inst, (uint32_t)pp[i], &cv[i]);
  }
  ok = (en_port = port_of(l, "enabled")) >= 0 && ok;
  ok = (lat_port = port_of(l, "latency")) >= 0 && ok;
  if (ok) {
    lilv_instance_connect_port(inst, (uint32_t)en_port, &enabled);
    lilv_instance_connect_port(inst, (uint32_t)lat_port, &lat_out);
    lilv_instance_activate(inst);
  }
  uint32_t pos = 0, b = 0;
  for (size_t s = 0; ok && s < SEGMENTS; s++) {
    const Segment *g = &PROGRAMME[s];
    for (uint32_t i = 0; i < NP; i++) cv[i] = g->v[i];
    enabled = g->bypass ? 0.0f : 1.0f;
    for (uint32_t f = 0; f < g->frames && pos < frames; b++) {
      uint32_t n = BLOCKS[b % NBLOCKS];
      if (n > g->frames - f) n = g->frames - f;
      lilv_instance_connect_port(inst, (uint32_t)ap[0], (void *)(il + pos));
      lilv_instance_connect_port(inst, (uint32_t)ap[1], (void *)(ir + pos));
      lilv_instance_connect_port(inst, (uint32_t)ap[2], ol + pos);
      lilv_instance_connect_port(inst, (uint32_t)ap[3], orr + pos);
      lilv_instance_run(inst, n);
      pos += n, f += n;
    }
  }
  if (ok) lilv_instance_deactivate(inst);
  lilv_instance_free(inst);
  if (ok && latency) *latency = (uint32_t)(lat_out + 0.5f);
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

/* ---- the checks, written once against the Driver interface ------------------------------ */

static void run_checks(const char *face, Driver *d, double sr) {
  const uint32_t frames = total_frames();
  float *buf = calloc((size_t)frames * 6, sizeof(float));
  if (!buf) abort();
  float *il = buf, *ir = buf + frames, *ol = buf + 2 * (size_t)frames, *orr = buf + 3 * (size_t)frames;
  float *rl = buf + 4 * (size_t)frames, *rr = buf + 5 * (size_t)frames;
  signal(il, ir, frames, sr);
  reference(sr, il, ir, rl, rr, frames);
  uint32_t lat = 99;
  bool ran = d->run(d->face, sr, il, ir, ol, orr, frames, &lat);
  check(ran, face, sr, "the programme runs");
  check(ran && memcmp(ol, rl, (size_t)frames * sizeof(float)) == 0 &&
            memcmp(orr, rr, (size_t)frames * sizeof(float)) == 0,
        face, sr, "output equals omx-dsp's phaser kernel, byte for byte");
  /* the programme must exercise the kernel: engaged output differs from the input, bypass does not. */
  uint32_t eng = 0, byp = PROGRAMME[0].frames + PROGRAMME[1].frames + PROGRAMME[2].frames + PROGRAMME[3].frames;
  check(memcmp(rl + eng, il + eng, PROGRAMME[0].frames * sizeof(float)) != 0, face, sr,
        "the reference moves the signal (the test is not vacuous)");
  check(memcmp(rl + byp, il + byp, PROGRAMME[4].frames * sizeof(float)) == 0, face, sr,
        "the reference is the input while bypassed");
  check(ran && lat == 0, face, sr, "the published latency is the kernel's: none");
  free(buf);
}

int main(int argc, char **argv) {
  int rc;
  if (argc == 3 && strcmp(argv[1], "clap") == 0) rc = clap_main(argv[2]);
  else if (argc == 4 && strcmp(argv[1], "lv2") == 0) rc = lv2_main(argv[2], argv[3]);
  else return fprintf(stderr, "usage: phaser-oracle clap <x.clap> | lv2 <bundle-dir> <uri>\n"), 2;
  printf("%s %s: %d failure(s) of %d check(s)\n", rc ? "ORACLE-RED" : "ORACLE-GREEN", argv[1], failures, checks);
  return rc;
}
