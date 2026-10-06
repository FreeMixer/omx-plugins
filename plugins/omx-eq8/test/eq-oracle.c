// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * eq-oracle.c — what an EQ MUST do, checked on the BUILT plugin at 44.1, 48, 96 and 192 kHz.
 *
 *   eq-oracle clap <omx-eq8.clap>
 *   eq-oracle lv2 <bundle-dir> <uri>
 *
 * The answers are computed here from the definition of the filters, never from omx-dsp (this file
 * includes no DSP header): a sine through a bell of gain G dB is scaled by exactly G dB at the
 * bell's centre; a Butterworth pass filter is 3.01 dB down at its cutoff whatever its order, and an
 * octave away it is 10 log10(1 + 2^2n) down: 12.30 dB for 12 dB/oct (n = 2), 24.10 dB for 24 dB/oct (n = 4); far from every band a sine passes unchanged; a racked instance, a band that is
 * switched off, a switched-off EQ and the host's bypass are the identity, byte for byte. The gain
 * is the amplitude of the sine's own quadrature component over a whole number of cycles, so a
 * filter's ringing and the noise of nothing else can hide in it. Parameters are found BY NAME (CLAP) or
 * BY SYMBOL (LV2, through lilv), so a renumbered face cannot pass by accident.
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
#define BLOCK 512u

#define MAX_SET 24
typedef struct {
  const char *sym[MAX_SET];
  double val[MAX_SET];
  int n;
  double bypass;
} Setting;

static void put(Setting *s, const char *sym, double v) {
  s->sym[s->n] = sym;
  s->val[s->n++] = v;
}

/** One face, driven: run `frames` of stereo input through a fresh instance under `s`. */
typedef bool (*RunFn)(void *face, double sr, const Setting *s, const float *il, const float *ir, float *ol, float *or_,
                      uint32_t frames);

static int failures;

static void check(bool ok, const char *face, double sr, const char *what) {
  if (!ok) failures++;
  printf("%s %s %s @ %.0f Hz\n", ok ? "PASS" : "FAIL", face, what, sr);
}

static bool identical(const float *a, const float *b, uint32_t n) { return memcmp(a, b, n * sizeof(float)) == 0; }

/** The gain in dB of a sine of frequency `f` and amplitude `a` through `run` under `s`: the
 * quadrature amplitude of the output over the last 100 cycles (to the nearest sample), after a settle. Returns -999
 * when the face failed. */
static double gain_db(RunFn run, void *h, double sr, const Setting *s, double f, double a, float *out_leg_r_gain_db) {
  const uint32_t win = (uint32_t)floor(100.0 * sr / f + 0.5), settle = (uint32_t)(0.3 * sr), n = settle + win;
  float *il = calloc(n, sizeof(float)), *ir = calloc(n, sizeof(float)), *ol = calloc(n, sizeof(float)),
        *or_ = calloc(n, sizeof(float));
  if (!il || !ir || !ol || !or_) abort();
  for (uint32_t i = 0; i < n; i++) {
    const double ph = 2.0 * M_PI * f * (double)i / sr;
    il[i] = (float)(a * sin(ph));
    ir[i] = (float)(0.5 * a * sin(ph));
  }
  double g = -999.0;
  if (run(h, sr, s, il, ir, ol, or_, n)) {
    double sl = 0, cl = 0, sr_ = 0, cr = 0;
    for (uint32_t i = settle; i < n; i++) {
      const double ph = 2.0 * M_PI * f * (double)i / sr;
      sl += ol[i] * sin(ph), cl += ol[i] * cos(ph), sr_ += or_[i] * sin(ph), cr += or_[i] * cos(ph);
    }
    const double al = 2.0 * sqrt(sl * sl + cl * cl) / win, ar = 2.0 * sqrt(sr_ * sr_ + cr * cr) / win;
    g = 20.0 * log10(al / a);
    *out_leg_r_gain_db = (float)(20.0 * log10(ar / (0.5 * a)));
  }
  free(il), free(ir), free(ol), free(or_);
  return g;
}

/** `want` within `tol` dB on BOTH legs (the right leg carries half the amplitude: a linear filter
 * does not care). */
static void gain_check(RunFn run, void *h, const char *face, double sr, const Setting *s, double f, double want,
                       double tol, const char *what) {
  float r_gain = 0;
  const double g = gain_db(run, h, sr, s, f, 0.25, &r_gain);
  char line[200];
  snprintf(line, sizeof line, "%s: %.4f dB (want %.4f +-%.2f), right leg %.4f dB", what, g, want, tol, (double)r_gain);
  check(fabs(g - want) <= tol && fabs((double)r_gain - want) <= tol, face, sr, line);
}

static void oracle(const char *face, void *h, RunFn run) {
  for (size_t ri = 0; ri < sizeof RATES / sizeof RATES[0]; ri++) {
    const double sr = RATES[ri];
    Setting s;

    /* A racked instance is a wire: every band off, the pass filters off, the EQ on. */
    {
      const uint32_t n = 8192u;
      float *il = calloc(n, sizeof(float)), *ir = calloc(n, sizeof(float)), *ol = calloc(n, sizeof(float)),
            *or_ = calloc(n, sizeof(float));
      if (!il || !ir || !ol || !or_) abort();
      uint32_t seed = 0x6f6d78u;
      for (uint32_t i = 0; i < n; i++) {
        seed = seed * 1664525u + 1013904223u;
        il[i] = (float)(seed >> 8) / 16777216.0f - 0.5f;
        ir[i] = -il[i];
      }
      memset(&s, 0, sizeof s);
      check(run(h, sr, &s, il, ir, ol, or_, n) && identical(ol, il, n) && identical(or_, ir, n), face, sr,
            "a racked instance is the identity");

      /* A band at +12 dB that is switched off, a bell whose EQ is switched off, and the bypass. */
      memset(&s, 0, sizeof s);
      put(&s, "b1_type", 0), put(&s, "b1_freq", 1000), put(&s, "b1_gain", 12), put(&s, "b1_q", 1), put(&s, "b1_on", 0);
      check(run(h, sr, &s, il, ir, ol, or_, n) && identical(ol, il, n) && identical(or_, ir, n), face, sr,
            "a +12 dB band that is off is the identity");
      memset(&s, 0, sizeof s);
      put(&s, "on", 0), put(&s, "hpf_on", 1), put(&s, "hpf_freq", 500), put(&s, "b1_gain", 12), put(&s, "b1_on", 1);
      check(run(h, sr, &s, il, ir, ol, or_, n) && identical(ol, il, n) && identical(or_, ir, n), face, sr,
            "the EQ switched off is the identity");
      memset(&s, 0, sizeof s);
      put(&s, "hpf_on", 1), put(&s, "hpf_freq", 500), put(&s, "lpf_on", 1), put(&s, "lpf_freq", 2000), put(&s, "b1_gain", 12), put(&s, "b1_on", 1);
      s.bypass = 1.0;
      check(run(h, sr, &s, il, ir, ol, or_, n) && identical(ol, il, n) && identical(or_, ir, n), face, sr,
            "bypass is the identity on a busy bank");
      free(il), free(ir), free(ol), free(or_);
    }

    /* A bell is exactly its gain at its centre, and nothing a decade away. */
    memset(&s, 0, sizeof s);
    put(&s, "b1_type", 0), put(&s, "b1_freq", 1000), put(&s, "b1_gain", 6), put(&s, "b1_q", 1), put(&s, "b1_on", 1);
    gain_check(run, h, face, sr, &s, 1000.0, 6.0206, 0.1, "bell +6 dB at 1 kHz, Q 1, at its centre");
    gain_check(run, h, face, sr, &s, 10000.0, 0.0, 0.5, "the same bell at 10 kHz");
    memset(&s, 0, sizeof s);
    put(&s, "b3_type", 0), put(&s, "b3_freq", 250), put(&s, "b3_gain", -9), put(&s, "b3_q", 2), put(&s, "b3_on", 1);
    gain_check(run, h, face, sr, &s, 250.0, -9.0, 0.1, "bell -9 dB at 250 Hz, Q 2, at its centre");

    /* A band that is not the first one, with the first one off: the slots close up. */
    memset(&s, 0, sizeof s);
    put(&s, "b8_type", 0), put(&s, "b8_freq", 1000), put(&s, "b8_gain", -6), put(&s, "b8_q", 1), put(&s, "b8_on", 1);
    gain_check(run, h, face, sr, &s, 1000.0, -6.0206, 0.1, "band 8 alone, -6 dB at 1 kHz");

    /* Butterworth pass filters: -3.01 dB at the cutoff, and one octave out 10 log10(1 + 2^2n) down. */
    memset(&s, 0, sizeof s);
    put(&s, "hpf_on", 1), put(&s, "hpf_freq", 100), put(&s, "hpf_slope", 12);
    gain_check(run, h, face, sr, &s, 100.0, -3.0103, 0.1, "HPF 100 Hz 12 dB/oct at its cutoff");
    gain_check(run, h, face, sr, &s, 50.0, -12.3045, 0.2, "the same HPF an octave down");
    gain_check(run, h, face, sr, &s, 10000.0, 0.0, 0.1, "the same HPF at 10 kHz");
    memset(&s, 0, sizeof s);
    put(&s, "hpf_on", 1), put(&s, "hpf_freq", 100), put(&s, "hpf_slope", 24);
    gain_check(run, h, face, sr, &s, 100.0, -3.0103, 0.1, "HPF 100 Hz 24 dB/oct at its cutoff");
    gain_check(run, h, face, sr, &s, 50.0, -24.0988, 0.3, "the same HPF an octave down");
    memset(&s, 0, sizeof s);
    put(&s, "lpf_on", 1), put(&s, "lpf_freq", 1000), put(&s, "lpf_slope", 12);
    gain_check(run, h, face, sr, &s, 1000.0, -3.0103, 0.1, "LPF 1 kHz 12 dB/oct at its cutoff");
    gain_check(run, h, face, sr, &s, 2000.0, -12.3045, 0.3, "the same LPF an octave up");
    gain_check(run, h, face, sr, &s, 100.0, 0.0, 0.1, "the same LPF at 100 Hz");
    memset(&s, 0, sizeof s);
    put(&s, "lpf_on", 1), put(&s, "lpf_freq", 1000), put(&s, "lpf_slope", 24);
    gain_check(run, h, face, sr, &s, 1000.0, -3.0103, 0.1, "LPF 1 kHz 24 dB/oct at its cutoff");
    gain_check(run, h, face, sr, &s, 2000.0, -24.0988, 0.4, "the same LPF an octave up");
  }
}

/* ---- CLAP ----------------------------------------------------------------------------------- */

static const void *h_ext(const clap_host_t *h, const char *id) {
  (void)h, (void)id;
  return NULL;
}
static void h_noop(const clap_host_t *h) { (void)h; }
static const clap_host_t HOST = {CLAP_VERSION_INIT, NULL, "eq-oracle", "openmixer", "", "0.1", h_ext, h_noop, h_noop, h_noop};

typedef struct {
  clap_event_param_value_t ev[MAX_SET + 2];
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

/** The CLAP parameter name of a declared symbol (tools/gen.mjs names; the oracle spells them out). */
static void clap_name_of(const char *sym, char *out, size_t size) {
  int band;
  char what[16];
  if (sscanf(sym, "b%d_%15s", &band, what) == 2) {
    const char *w = strcmp(what, "type") == 0 ? "Type" : strcmp(what, "freq") == 0 ? "Frequency"
                  : strcmp(what, "gain") == 0 ? "Gain" : strcmp(what, "q") == 0 ? "Q" : "On";
    snprintf(out, size, "Band %d %s", band, w);
  } else if (strcmp(sym, "on") == 0) {
    snprintf(out, size, "EQ On");
  } else {
    char pass[4];
    sscanf(sym, "%3[a-z]_%15s", pass, what);
    for (char *c = pass; *c; c++) *c = (char)(*c - 'a' + 'A');
    snprintf(out, size, "%s %s", pass, strcmp(what, "on") == 0 ? "On" : strcmp(what, "freq") == 0 ? "Frequency" : "Slope");
  }
}

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
  for (int i = 0; ok && i <= s->n; i++) {
    clap_id id;
    char name[48];
    if (i < s->n) clap_name_of(s->sym[i], name, sizeof name);
    ok = clap_param(pp, p, i < s->n ? name : NULL, &id);
    clap_event_param_value_t *e = &evs.ev[evs.n++];
    memset(e, 0, sizeof *e);
    e->header = (clap_event_header_t){sizeof *e, 0, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_PARAM_VALUE, 0};
    e->param_id = ok ? id : 0, e->note_id = -1, e->port_index = -1, e->channel = -1, e->key = -1;
    e->value = i < s->n ? s->val[i] : s->bypass;
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
  int32_t ap[4];
  bool ok = true;
  for (int i = 0; i < 4; i++) ok = (ap[i] = port_of(l, audio[i])) >= 0 && ok;
  /* Every control port starts at its declared default (lv2:default of the port), then the setting. */
  const uint32_t n_ports = lilv_plugin_get_num_ports(l->plugin);
  float *dflt = calloc(n_ports, sizeof(float)), *cv = calloc(n_ports, sizeof(float));
  if (!dflt || !cv) abort();
  lilv_plugin_get_port_ranges_float(l->plugin, NULL, NULL, dflt);
  LilvNode *audio_class = lilv_new_uri(l->world, LILV_URI_AUDIO_PORT);
  for (uint32_t i = 0; i < n_ports; i++) {
    cv[i] = dflt[i];
    if (!lilv_port_is_a(l->plugin, lilv_plugin_get_port_by_index(l->plugin, i), audio_class))
      lilv_instance_connect_port(inst, i, &cv[i]);
  }
  lilv_node_free(audio_class);
  for (int i = 0; i < s->n; i++) {
    const int32_t idx = port_of(l, s->sym[i]);
    ok = idx >= 0 && ok;
    if (idx >= 0) cv[idx] = (float)s->val[i];
  }
  const int32_t enabled = port_of(l, "enabled");
  ok = enabled >= 0 && ok;
  if (enabled >= 0) cv[enabled] = s->bypass > 0.5 ? 0.0f : 1.0f;
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
  free(dflt), free(cv);
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
  oracle("lv2", &l, lv2_run);
  lilv_node_free(u), lilv_node_free(b);
  lilv_world_free(l.world);
  return failures ? 1 : 0;
}

int main(int argc, char **argv) {
  int rc;
  if (argc == 3 && strcmp(argv[1], "clap") == 0) rc = clap_main(argv[2]);
  else if (argc == 4 && strcmp(argv[1], "lv2") == 0) rc = lv2_main(argv[2], argv[3]);
  else return fprintf(stderr, "usage: eq-oracle clap <x.clap> | lv2 <bundle-dir> <uri>\n"), 2;
  printf("%s %s: %d failure(s)\n", rc ? "ORACLE-RED" : "ORACLE-GREEN", argv[1], failures);
  return rc;
}
