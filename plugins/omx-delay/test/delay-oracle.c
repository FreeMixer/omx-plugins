// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * delay-oracle.c — what a delay MUST do, checked on the BUILT plugin at 44.1, 48, 96 and 192 kHz.
 *
 *   delay-oracle clap <omx-delay.clap>
 *   delay-oracle lv2 <bundle-dir> <uri>
 *
 * The answer is computed here from the definition of a feedback delay, never from omx-dsp (this
 * file includes no kernel header): an impulse fed in at frame 0 with the dry path off comes out
 * at d, 2d, 3d, ... frames, d = round(timeMs * rate / 1000), each repeat the previous one times
 * the feedback; with ping-pong the repeats alternate legs; with a mix m the dry impulse is (1 - m)
 * and the first repeat m; bypassed, output is input. Parameters are found BY NAME (CLAP) or BY
 * SYMBOL (LV2, through lilv), so a renumbered face cannot pass by accident.
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

static const double RATES[] = {44100.0, 48000.0, 96000.0, 192000.0};
static const double TIMES_MS[] = {1.0, 250.0, 1234.0, 2000.0};
#define BLOCK 512u
#define TOL 1e-6f

typedef struct {
  double time_ms, feedback, mix, tone, pingpong, bypass;
} Setting;

/** One face, driven: run `frames` of stereo input through a fresh instance under `s`. */
typedef bool (*RunFn)(void *face, double sr, const Setting *s, const float *il, const float *ir, float *ol, float *or_,
                      uint32_t frames);

static int failures;

static void check(bool ok, const char *face, double sr, const char *what) {
  if (!ok) failures++;
  printf("%s %s %s @ %.0f Hz\n", ok ? "PASS" : "FAIL", face, what, sr);
}

/** Every frame of `got` equals `want` within TOL; on a miss, name the first frame. */
static bool same(const float *got, const float *want, uint32_t n, const char *leg) {
  for (uint32_t i = 0; i < n; i++)
    if (!(fabsf(got[i] - want[i]) <= TOL)) {
      fprintf(stderr, "  %s frame %u: got %.9g, want %.9g\n", leg, i, (double)got[i], (double)want[i]);
      return false;
    }
  return true;
}

static void oracle(const char *face, void *h, RunFn run) {
  for (size_t ri = 0; ri < sizeof RATES / sizeof RATES[0]; ri++) {
    const double sr = RATES[ri];
    const uint32_t frames = (uint32_t)(3.0 * 2000.0 * sr / 1000.0) + 64u;
    float *il = calloc(frames, sizeof(float)), *ir = calloc(frames, sizeof(float));
    float *ol = calloc(frames, sizeof(float)), *or_ = calloc(frames, sizeof(float));
    float *wl = calloc(frames, sizeof(float)), *wr = calloc(frames, sizeof(float));
    if (!il || !ir || !ol || !or_ || !wl || !wr) abort();
    char what[160];

    for (size_t ti = 0; ti < sizeof TIMES_MS / sizeof TIMES_MS[0]; ti++) {
      const double t = TIMES_MS[ti];
      const uint32_t d = (uint32_t)floor(t * sr / 1000.0 + 0.5);
      const uint32_t n = 3u * d + 64u;

      /* Echoes: L impulse 1, R impulse 0.5; wet only, feedback 0.5, bright. */
      memset(il, 0, n * sizeof(float)), memset(ir, 0, n * sizeof(float));
      il[0] = 1.0f, ir[0] = 0.5f;
      memset(wl, 0, n * sizeof(float)), memset(wr, 0, n * sizeof(float));
      for (uint32_t k = 1; k * d < n; k++) wl[k * d] = powf(0.5f, (float)(k - 1)), wr[k * d] = 0.5f * wl[k * d];
      Setting echo = {t, 0.5, 1.0, 0.0, 0.0, 0.0};
      snprintf(what, sizeof what, "echoes at %u, %u, %u frames (%.0f ms)", d, 2 * d, 3 * d, t);
      check(run(h, sr, &echo, il, ir, ol, or_, n) && same(ol, wl, n, "L") && same(or_, wr, n, "R"), face, sr, what);

      /* Ping-pong: L impulse only; the repeats cross legs. */
      ir[0] = 0.0f;
      memset(wl, 0, n * sizeof(float)), memset(wr, 0, n * sizeof(float));
      for (uint32_t k = 1; k * d < n; k++) (k % 2 ? wl : wr)[k * d] = powf(0.5f, (float)(k - 1));
      Setting pp = {t, 0.5, 1.0, 0.0, 1.0, 0.0};
      snprintf(what, sizeof what, "ping-pong L %u, R %u, L %u (%.0f ms)", d, 2 * d, 3 * d, t);
      check(run(h, sr, &pp, il, ir, ol, or_, n) && same(ol, wl, n, "L") && same(or_, wr, n, "R"), face, sr, what);
    }

    /* Mix: dry (1 - m) at 0, the first repeat m at d, nothing after with no feedback. */
    {
      const uint32_t d = (uint32_t)floor(250.0 * sr / 1000.0 + 0.5), n = 3u * d;
      memset(il, 0, n * sizeof(float)), memset(ir, 0, n * sizeof(float));
      il[0] = 1.0f, ir[0] = 1.0f;
      memset(wl, 0, n * sizeof(float));
      wl[0] = 0.75f, wl[d] = 0.25f;
      Setting mix = {250.0, 0.0, 0.25, 0.0, 0.0, 0.0};
      check(run(h, sr, &mix, il, ir, ol, or_, n) && same(ol, wl, n, "L") && same(or_, wl, n, "R"), face, sr,
            "mix 0.25: dry 0.75, repeat 0.25");
    }

    /* Bypass: output is input, on a busy signal. */
    {
      const uint32_t n = 4096u;
      uint32_t seed = 0x6f6d78u;
      for (uint32_t i = 0; i < n; i++) {
        seed = seed * 1664525u + 1013904223u;
        il[i] = (float)(seed >> 8) / 16777216.0f - 0.5f;
        ir[i] = -il[i];
      }
      Setting by = {250.0, 0.9, 1.0, 0.3, 1.0, 1.0};
      check(run(h, sr, &by, il, ir, ol, or_, n) && same(ol, il, n, "L") && same(or_, ir, n, "R"), face, sr,
            "bypass is the identity");
    }
    free(il), free(ir), free(ol), free(or_), free(wl), free(wr);
  }
}

/* ---- CLAP ----------------------------------------------------------------------------------- */

static const void *h_ext(const clap_host_t *h, const char *id) {
  (void)h, (void)id;
  return NULL;
}
static void h_noop(const clap_host_t *h) { (void)h; }
static const clap_host_t HOST = {CLAP_VERSION_INIT, NULL, "delay-oracle", "openmixer", "", "0.1", h_ext, h_noop, h_noop, h_noop};

typedef struct {
  clap_event_param_value_t ev[16];
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

/** The param whose name is `name`, or the bypass param when `name` is NULL. */
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

static bool clap_run(void *face, double sr, const Setting *s, const float *il, const float *ir, float *ol, float *or_,
                     uint32_t frames) {
  ClapFace *c = (ClapFace *)face;
  const clap_plugin_descriptor_t *desc = c->factory->get_plugin_descriptor(c->factory, 0);
  const clap_plugin_t *p = c->factory->create_plugin(c->factory, &HOST, desc->id);
  if (!p || !p->init(p)) return false;
  const clap_plugin_params_t *pp = (const clap_plugin_params_t *)p->get_extension(p, CLAP_EXT_PARAMS);
  bool ok = pp && p->activate(p, sr, 1, BLOCK) && p->start_processing(p);
  Events evs = {.n = 0};
  const char *names[] = {"Time", "Feedback", "Mix", "Tone", "Pingpong", NULL};
  const double vals[] = {s->time_ms, s->feedback, s->mix, s->tone, s->pingpong, s->bypass};
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
    float *ib[2] = {(float *)il + f, (float *)ir + f}, *ob[2] = {ol + f, or_ + f};
    clap_audio_buffer_t ai = {ib, NULL, 2, 0, 0}, ao = {ob, NULL, 2, 0, 0};
    clap_process_t pr = {.steady_time = f, .frames_count = n, .audio_inputs = &ai, .audio_outputs = &ao,
                         .audio_inputs_count = 1, .audio_outputs_count = 1, .in_events = f ? &in_none : &in,
                         .out_events = &out};
    ok = p->process(p, &pr) != CLAP_PROCESS_ERROR;
  }
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
  oracle("clap", &c, clap_run);
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

static bool lv2_run(void *face, double sr, const Setting *s, const float *il, const float *ir, float *ol, float *or_,
                    uint32_t frames) {
  Lv2Face *l = (Lv2Face *)face;
  LilvInstance *inst = lilv_plugin_instantiate(l->plugin, sr, NULL);
  if (!inst) return false;
  const char *audio[] = {"in_l", "in_r", "out_l", "out_r"};
  const char *ctl[] = {"timeMs", "feedback", "mix", "tone", "pingpong", "enabled", "latency"};
  float cv[7] = {(float)s->time_ms, (float)s->feedback, (float)s->mix, (float)s->tone, (float)s->pingpong,
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
    lilv_instance_connect_port(inst, (uint32_t)ap[3], or_ + f);
    lilv_instance_run(inst, n);
  }
  if (ok) lilv_instance_deactivate(inst);
  lilv_instance_free(inst);
  return ok && cv[6] == 0.0f;
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
  oracle("lv2", &l, lv2_run);
  lilv_node_free(u), lilv_node_free(b);
  lilv_world_free(l.world);
  return failures ? 1 : 0;
}

int main(int argc, char **argv) {
  int rc;
  if (argc == 3 && strcmp(argv[1], "clap") == 0) rc = clap_main(argv[2]);
  else if (argc == 4 && strcmp(argv[1], "lv2") == 0) rc = lv2_main(argv[2], argv[3]);
  else return fprintf(stderr, "usage: delay-oracle clap <x.clap> | lv2 <bundle-dir> <uri>\n"), 2;
  printf("%s %s: %d failure(s)\n", rc ? "ORACLE-RED" : "ORACLE-GREEN", argv[1], failures);
  return rc;
}
