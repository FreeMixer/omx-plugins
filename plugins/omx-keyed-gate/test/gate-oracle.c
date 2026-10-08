// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * gate-oracle.c — omx-keyed-gate's kernel-identity test: both BUILT faces, sample for sample,
 * against omx-dsp's gate called DIRECTLY, at 44.1, 48, 96 and 192 kHz, with no tolerance.
 *
 *   gate-oracle clap <omx-keyed-gate.clap>
 *   gate-oracle lv2 <omx-keyed-gate.lv2> <uri>
 *
 * The face is reached the way a host reaches it (dlopen and clap_entry, or lilv), never by
 * including it. It runs in 256-frame quanta through a programme of seven segments with the
 * controls moving between them (key on and off, threshold, ratio, range, attack, release, the key
 * left unconnected, bypass on and off). Two references run each segment as ONE block:
 *
 *   - the instance core, <omxdsp/fx/omx_gate_instance.h>, called directly: the face's binding
 *     must deliver every control and the key to it unchanged;
 *   - the console's gate as the console's own keyed gate bundle tested it: omx_dynamics_keyed on
 *     the atom the console resolves (below mode, peak detector, hard knee, unity make-up,
 *     oversampling on auto), written out here from the control values.
 *
 * Equal output proves the face calls the console's kernel, the controls land on the same atom,
 * the key and only the key drives the detector when it is connected and chosen, and the state
 * carries across a host's block boundary as it does inside one block. Then the measured laws, on
 * the face's own audio: a quiet key closes the gate on a loud signal, a loud key opens it on a
 * quiet one, Self closes it again, bypass is the input, the latency is the 4x detector's while a
 * fast attack engages it; and a face whose threshold is left alone opens at the declared default.
 *
 * The LV2 face is also held to the console bundle's identity (urn:openmixer:keyed-gate): every
 * port's index and symbol, every control's default and travel, the key a sidechain.
 *
 * HOLD is not exercised: the gate has none (omx_gate.h: the console's resolved gate carries none).
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
#include <lv2/core/lv2.h>

#include "omx_gate_params.h"
#include <omxdsp/fx/omx_gate_instance.h>
#include <omxdsp/omx_denormal.h>
#include <omxcontract/omx_contract_limits.h>

static const float RATES[] = OMX_ORACLE_FLOOR_RATES_INIT; /* the four rates every kernel is judged at */
#define QUANTUM 256u
#define SEGMENTS 7u
#define SEG_FRAMES (QUANTUM * 150u) /* 38 400 frames: 0.2 s at 192 kHz, 0.87 s at 44.1 kHz */
#define FRAMES (SEG_FRAMES * SEGMENTS)

static int failures, checks;

static void check(bool ok, const char *face, const char *what) {
  checks++;
  if (!ok) failures++;
  printf("%s %s %s\n", ok ? "PASS" : "FAIL", face, what);
}

/* One control programme per segment. `key_port` 0 is the host routing nothing to the key. */
typedef struct {
  float enabled, key_external, threshold, ratio, range, attack, release;
  int key_port;
  const char *name;
} Ctl;
static const Ctl PROGRAMME[SEGMENTS] = {
    {1, 1, -30, 16, -90, 1.0f, 10, 1, "loud main, quiet key: closed"},
    {1, 1, -30, 16, -90, 1.0f, 10, 1, "quiet main, loud key: open"},
    {1, 0, -30, 16, -90, 1.0f, 10, 1, "key set to Self, quiet main: closed"},
    {1, 1, -50, 16, -20, 0.2f, 300, 1, "thr -50, range -20, attack 0.2 ms (4x), keyed bursts"},
    {0, 1, -50, 16, -20, 0.2f, 300, 1, "bypassed"},
    {1, 1, -20, 4, -40, 10.0f, 1000, 1, "back on, ratio 4, range -40, 10/1000 ms"},
    {1, 1, -20, 4, -40, 10.0f, 1000, 0, "key unconnected: self-keyed"},
};

static float in_l[FRAMES], in_r[FRAMES], key[FRAMES];
static float out_l[FRAMES], out_r[FRAMES];
static float ref_l[FRAMES], ref_r[FRAMES], con_l[FRAMES], con_r[FRAMES];
static float latency_seen[SEGMENTS];

static uint32_t lcg;
static float noise(void) {
  lcg = lcg * 1664525u + 1013904223u;
  return (float)(int32_t)lcg * (1.0f / 2147483648.0f);
}

/* Sines in the first three segments, noise after; the key a tone at -80 dBFS, then -6 dBFS, then
 * bursts, then a modulated tone. */
static void make_input(float rate) {
  lcg = 1u;
  const float tau = 6.283185307f;
  for (uint32_t i = 0; i < FRAMES; i++) {
    const uint32_t s = i / SEG_FRAMES;
    const float t = (float)i / rate;
    float amp, k;
    switch (s) {
    case 0: amp = 0.5f, k = 1.0e-4f * sinf(tau * 440.0f * t); break;
    case 1:
    case 2: amp = 0.01f, k = 0.5f * sinf(tau * 440.0f * t); break;
    case 3:
    case 4: amp = 0.25f, k = (i % 4800u) < 2400u ? 0.3f * sinf(tau * 220.0f * t) : 0.0f; break;
    default: amp = 0.3f, k = 0.1f * sinf(tau * 3.0f * t) * sinf(tau * 300.0f * t); break;
    }
    if (s >= 3u) {
      in_l[i] = amp * noise();
      in_r[i] = amp * noise();
    } else {
      in_l[i] = amp * sinf(tau * 1000.0f * t);
      in_r[i] = amp * sinf(tau * 1500.0f * t);
    }
    key[i] = k;
  }
}

/* ---- the references --------------------------------------------------------------------- */

/** The instance core called directly, one block per segment. */
static void reference_instance(float rate) {
  static OmxGateInstance g;
  omx_gate_instance_init(&g, rate);
  memcpy(ref_l, in_l, sizeof ref_l);
  memcpy(ref_r, in_r, sizeof ref_r);
  for (uint32_t s = 0; s < SEGMENTS; s++) {
    const Ctl *c = &PROGRAMME[s];
    const uint32_t off = s * SEG_FRAMES;
    omx_gate_instance_resolve(&g, c->enabled < 0.5f, c->key_external >= 0.5f, c->threshold, c->ratio, c->range,
                              c->attack, c->release);
    omx_gate_instance_run(&g, c->key_port ? key + off : NULL, ref_l + off, ref_r + off, ref_l + off, ref_r + off,
                          SEG_FRAMES);
  }
}

/** The console's gate: omx_dynamics_keyed on the atom the console resolves, written out here. */
static void reference_console(float rate) {
  static struct omx_dyn_state st;
  omx_dyn_state_init(&st, 1u);
  memcpy(con_l, in_l, sizeof con_l);
  memcpy(con_r, in_r, sizeof con_r);
  for (uint32_t s = 0; s < SEGMENTS; s++) {
    const Ctl *c = &PROGRAMME[s];
    struct omx_dyn p;
    memset(&p, 0, sizeof p);
    p.enabled = c->enabled >= 0.5f;
    p.gc.mode = OMX_DYN_BELOW;
    p.detect = OMX_DETECT_PEAK;
    p.gc.thresh_db = c->threshold;
    p.gc.ratio = c->ratio;
    p.gc.knee_db = 0.0f;
    p.gc.range_db = c->range;
    p.gc.makeup_lin = 1.0f;
    p.ovs_mode = OMX_DYN_OVS_AUTO;
    p.attack_ms = c->attack;
    p.attack_coeff = omx_pole_from_time_ms(c->attack, rate);
    p.release_coeff = omx_pole_from_time_ms(c->release, rate);
    const uint32_t off = s * SEG_FRAMES;
    const float *k = c->key_port && c->key_external >= 0.5f ? key + off : NULL;
    omx_dynamics_keyed(con_l + off, con_r + off, k, SEG_FRAMES, &p, &st);
  }
}

/* ---- the faces ---------------------------------------------------------------------------- */

/** Run the programme through a fresh instance at `rate` into out_l/out_r; latency_seen per segment
 * (NAN where the face publishes none per block). */
typedef bool (*RunFn)(void *face, float rate);

/* CLAP */

static const void *h_ext(const clap_host_t *h, const char *id) {
  (void)h, (void)id;
  return NULL;
}
static void h_noop(const clap_host_t *h) { (void)h; }
static const clap_host_t HOST = {CLAP_VERSION_INIT, NULL, "gate-oracle", "openmixer", "", "0.1", h_ext, h_noop, h_noop, h_noop};

typedef struct {
  clap_event_param_value_t ev[OMX_GATE_PARAM_COUNT + 1];
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
static void ev_add(Events *e, clap_id id, double v) {
  clap_event_param_value_t *x = &e->ev[e->n++];
  memset(x, 0, sizeof *x);
  x->header = (clap_event_header_t){sizeof *x, 0, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_PARAM_VALUE, 0};
  x->param_id = id, x->note_id = -1, x->port_index = -1, x->channel = -1, x->key = -1, x->value = v;
}

typedef struct {
  void *lib;
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

/* The CLAP names of the programme's controls, in Ctl order after `enabled` (the bypass). */
static const char *const CLAP_NAMES[6] = {"Key", "Threshold", "Ratio", "Range", "Attack", "Release"};

static const clap_plugin_t *clap_new(ClapFace *c, const clap_plugin_params_t **pp) {
  const clap_plugin_t *p = c->factory->create_plugin(c->factory, &HOST, c->factory->get_plugin_descriptor(c->factory, 0)->id);
  if (!p || !p->init(p)) return NULL;
  *pp = (const clap_plugin_params_t *)p->get_extension(p, CLAP_EXT_PARAMS);
  return *pp ? p : NULL;
}

static bool clap_process(const clap_plugin_t *p, uint32_t f, Events *evs, int key_port) {
  clap_input_events_t in = {evs, ev_size, ev_get};
  clap_output_events_t out = {NULL, ev_push};
  float *ib[2] = {in_l + f, in_r + f}, *kb[1] = {key + f}, *ob[2] = {out_l + f, out_r + f};
  clap_audio_buffer_t ai[2] = {{ib, NULL, 2, 0, 0}, {kb, NULL, 1, 0, 0}}, ao = {ob, NULL, 2, 0, 0};
  clap_process_t pr = {.steady_time = f, .frames_count = QUANTUM, .audio_inputs = ai, .audio_outputs = &ao,
                       .audio_inputs_count = key_port ? 2u : 1u, .audio_outputs_count = 1, .in_events = &in,
                       .out_events = &out};
  return p->process(p, &pr) != CLAP_PROCESS_ERROR;
}

static bool clap_run(void *face, float rate) {
  ClapFace *c = (ClapFace *)face;
  const clap_plugin_params_t *pp;
  const clap_plugin_t *p = clap_new(c, &pp);
  if (!p) return false;
  clap_id ids[7];
  bool ok = clap_param(pp, p, NULL, &ids[0]);
  for (int i = 0; i < 6; i++) ok = clap_param(pp, p, CLAP_NAMES[i], &ids[1 + i]) && ok;
  ok = ok && p->activate(p, rate, 1, QUANTUM) && p->start_processing(p);
  for (uint32_t s = 0; ok && s < SEGMENTS; s++) {
    const Ctl *k = &PROGRAMME[s];
    const float v[7] = {k->enabled < 0.5f ? 1.0f : 0.0f, k->key_external, k->threshold, k->ratio, k->range, k->attack, k->release};
    for (uint32_t f = s * SEG_FRAMES; ok && f < (s + 1u) * SEG_FRAMES; f += QUANTUM) {
      Events evs = {.n = 0};
      if (f == s * SEG_FRAMES)
        for (int i = 0; i < 7; i++) ev_add(&evs, ids[i], v[i]);
      ok = clap_process(p, f, &evs, k->key_port);
    }
    latency_seen[s] = NAN;
  }
  p->stop_processing(p);
  p->deactivate(p);
  p->destroy(p);
  return ok;
}

/** The latency a fresh CLAP instance reports after activation with `attack_ms` set before it. */
static int64_t clap_latency(ClapFace *c, float rate, double attack_ms) {
  const clap_plugin_params_t *pp;
  const clap_plugin_t *p = clap_new(c, &pp);
  const clap_plugin_latency_t *lat = p ? (const clap_plugin_latency_t *)p->get_extension(p, CLAP_EXT_LATENCY) : NULL;
  clap_id id;
  int64_t frames = -1;
  if (lat && clap_param(pp, p, "Attack", &id)) {
    Events evs = {.n = 0};
    ev_add(&evs, id, attack_ms);
    clap_input_events_t in = {&evs, ev_size, ev_get};
    clap_output_events_t out = {NULL, ev_push};
    pp->flush(p, &in, &out);
    if (p->activate(p, rate, 1, QUANTUM)) {
      frames = lat->get(p);
      p->deactivate(p);
    }
  }
  if (p) p->destroy(p);
  return frames;
}

/** A self-keyed sine, the threshold never sent: its level a measured 1.5 dB above the default. */
static bool clap_default_threshold(void *face, float rate, const float *sine, float *out, uint32_t n) {
  ClapFace *c = (ClapFace *)face;
  const clap_plugin_params_t *pp;
  const clap_plugin_t *p = clap_new(c, &pp);
  clap_id id;
  bool ok = p && clap_param(pp, p, "Key", &id) && p->activate(p, rate, 1, QUANTUM) && p->start_processing(p);
  for (uint32_t f = 0; ok && f + QUANTUM <= n; f += QUANTUM) {
    Events evs = {.n = 0};
    if (f == 0) ev_add(&evs, id, 0.0);
    clap_input_events_t in = {&evs, ev_size, ev_get};
    clap_output_events_t o = {NULL, ev_push};
    float *ib[2] = {(float *)sine + f, (float *)sine + f}, *ob[2] = {out + f, out_r + f};
    clap_audio_buffer_t ai = {ib, NULL, 2, 0, 0}, ao = {ob, NULL, 2, 0, 0};
    clap_process_t pr = {.steady_time = f, .frames_count = QUANTUM, .audio_inputs = &ai, .audio_outputs = &ao,
                         .audio_inputs_count = 1, .audio_outputs_count = 1, .in_events = &in, .out_events = &o};
    ok = p->process(p, &pr) != CLAP_PROCESS_ERROR;
  }
  if (p) {
    p->stop_processing(p);
    p->deactivate(p);
    p->destroy(p);
  }
  return ok;
}

/* LV2 */

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

/* The control symbols, in Ctl field order. */
static const char *const LV2_CONTROLS[7] = {"enabled", "key_external", "threshold", "ratio", "range", "attack", "release"};

static bool lv2_run(void *face, float rate) {
  Lv2Face *l = (Lv2Face *)face;
  LilvInstance *inst = lilv_plugin_instantiate(l->plugin, rate, NULL);
  if (!inst) return false;
  Ctl ctl = PROGRAMME[0];
  float *cv[7] = {&ctl.enabled, &ctl.key_external, &ctl.threshold, &ctl.ratio, &ctl.range, &ctl.attack, &ctl.release};
  float latency = -1.0f;
  const int32_t pil = port_of(l, "in_l"), pir = port_of(l, "in_r"), pk = port_of(l, "key"), pol = port_of(l, "out_l"),
                por = port_of(l, "out_r"), plat = port_of(l, "latency");
  bool ok = pil >= 0 && pir >= 0 && pk >= 0 && pol >= 0 && por >= 0 && plat >= 0;
  for (int i = 0; i < 7; i++) {
    const int32_t idx = port_of(l, LV2_CONTROLS[i]);
    ok = idx >= 0 && ok;
    if (idx >= 0) lilv_instance_connect_port(inst, (uint32_t)idx, cv[i]);
  }
  if (ok) {
    lilv_instance_connect_port(inst, (uint32_t)plat, &latency);
    lilv_instance_activate(inst);
  }
  for (uint32_t s = 0; ok && s < SEGMENTS; s++) {
    ctl = PROGRAMME[s];
    for (uint32_t f = s * SEG_FRAMES; f < (s + 1u) * SEG_FRAMES; f += QUANTUM) {
      lilv_instance_connect_port(inst, (uint32_t)pil, in_l + f);
      lilv_instance_connect_port(inst, (uint32_t)pir, in_r + f);
      lilv_instance_connect_port(inst, (uint32_t)pk, ctl.key_port ? key + f : NULL);
      lilv_instance_connect_port(inst, (uint32_t)pol, out_l + f);
      lilv_instance_connect_port(inst, (uint32_t)por, out_r + f);
      latency = -1.0f;
      lilv_instance_run(inst, QUANTUM);
    }
    latency_seen[s] = latency;
  }
  if (ok) lilv_instance_deactivate(inst);
  lilv_instance_free(inst);
  return ok;
}

static bool lv2_default_threshold(void *face, float rate, const float *sine, float *out, uint32_t n) {
  Lv2Face *l = (Lv2Face *)face;
  LilvInstance *inst = lilv_plugin_instantiate(l->plugin, rate, NULL);
  if (!inst) return false;
  float enabled = 1.0f, self = 0.0f, latency = 0.0f;
  const int32_t pe = port_of(l, "enabled"), pke = port_of(l, "key_external"), plat = port_of(l, "latency"),
                pil = port_of(l, "in_l"), pir = port_of(l, "in_r"), pk = port_of(l, "key"), pol = port_of(l, "out_l"),
                por = port_of(l, "out_r");
  const bool ok = pe >= 0 && pke >= 0 && plat >= 0 && pil >= 0 && pir >= 0 && pk >= 0 && pol >= 0 && por >= 0;
  if (ok) {
    /* threshold and the other controls left unconnected */
    lilv_instance_connect_port(inst, (uint32_t)pe, &enabled);
    lilv_instance_connect_port(inst, (uint32_t)pke, &self);
    lilv_instance_connect_port(inst, (uint32_t)plat, &latency);
    lilv_instance_connect_port(inst, (uint32_t)pk, NULL);
    lilv_instance_activate(inst);
    for (uint32_t f = 0; f + QUANTUM <= n; f += QUANTUM) {
      lilv_instance_connect_port(inst, (uint32_t)pil, (void *)(sine + f));
      lilv_instance_connect_port(inst, (uint32_t)pir, (void *)(sine + f));
      lilv_instance_connect_port(inst, (uint32_t)pol, out + f);
      lilv_instance_connect_port(inst, (uint32_t)por, out_r + f);
      lilv_instance_run(inst, QUANTUM);
    }
    lilv_instance_deactivate(inst);
  }
  lilv_instance_free(inst);
  return ok;
}

/* The console's keyed gate bundle, port for port: what a session saved against it reconnects to. */
static const struct {
  const char *symbol;
  float def, min, max; /* NAN for an audio port or the latency output */
} BUNDLE[] = {
    {"in_l", NAN, NAN, NAN},      {"in_r", NAN, NAN, NAN},        {"key", NAN, NAN, NAN},
    {"out_l", NAN, NAN, NAN},     {"out_r", NAN, NAN, NAN},       {"enabled", 1, 0, 1},
    {"key_external", 1, 0, 1},    {"threshold", -40, -80, 0},     {"ratio", 16, 1, 100},
    {"range", -90, -90, 0},       {"attack", 1, 0, 500},          {"release", 100, 0, 5000},
    {"latency", NAN, NAN, NAN},
};

static void lv2_identity(Lv2Face *l, const char *uri) {
  char what[200];
  check(strcmp(uri, "urn:openmixer:keyed-gate") == 0, "lv2", "the URI is the console bundle's, urn:openmixer:keyed-gate");
  const uint32_t n = sizeof BUNDLE / sizeof BUNDLE[0];
  snprintf(what, sizeof what, "%u ports, as the console bundle has", n);
  check(lilv_plugin_get_num_ports(l->plugin) == n, "lv2", what);
  for (uint32_t i = 0; i < n && i < lilv_plugin_get_num_ports(l->plugin); i++) {
    const LilvPort *port = lilv_plugin_get_port_by_index(l->plugin, i);
    const char *sym = lilv_node_as_string(lilv_port_get_symbol(l->plugin, port));
    bool ok = strcmp(sym, BUNDLE[i].symbol) == 0;
    if (!isnan(BUNDLE[i].def)) {
      LilvNode *def, *min, *max;
      lilv_port_get_range(l->plugin, port, &def, &min, &max);
      ok = ok && def && min && max && lilv_node_as_float(def) == BUNDLE[i].def && lilv_node_as_float(min) == BUNDLE[i].min &&
           lilv_node_as_float(max) == BUNDLE[i].max;
      lilv_node_free(def), lilv_node_free(min), lilv_node_free(max);
      snprintf(what, sizeof what, "port %u is \"%s\", default %g in %g..%g", i, BUNDLE[i].symbol, (double)BUNDLE[i].def,
               (double)BUNDLE[i].min, (double)BUNDLE[i].max);
    } else {
      snprintf(what, sizeof what, "port %u is \"%s\"", i, BUNDLE[i].symbol);
    }
    check(ok, "lv2", what);
  }
  LilvNode *side = lilv_new_uri(l->world, LV2_CORE_PREFIX "isSideChain");
  LilvNode *opt = lilv_new_uri(l->world, LV2_CORE_PREFIX "connectionOptional");
  LilvNode *sym = lilv_new_string(l->world, "key");
  const LilvPort *kp = lilv_plugin_get_port_by_symbol(l->plugin, sym);
  check(kp && lilv_port_has_property(l->plugin, kp, side) && lilv_port_has_property(l->plugin, kp, opt), "lv2",
        "the key is lv2:isSideChain and lv2:connectionOptional");
  lilv_node_free(side), lilv_node_free(opt), lilv_node_free(sym);
  LilvNode *reports = lilv_new_uri(l->world, LV2_CORE_PREFIX "reportsLatency");
  LilvNode *lsym = lilv_new_string(l->world, "latency");
  const LilvPort *lp = lilv_plugin_get_port_by_symbol(l->plugin, lsym);
  check(lp && lilv_port_has_property(l->plugin, lp, reports), "lv2", "the latency port reports the latency");
  lilv_node_free(reports), lilv_node_free(lsym);
}

/* ---- the oracle --------------------------------------------------------------------------- */

static double db_rms(const float *x, uint32_t n) {
  double acc = 0.0;
  for (uint32_t i = 0; i < n; i++) acc += (double)x[i] * (double)x[i];
  return 10.0 * log10(acc / (double)n + 1e-30);
}

/* A segment's settled level: its last half, the left leg. */
static double seg_db(const float *x, uint32_t s) { return db_rms(x + s * SEG_FRAMES + SEG_FRAMES / 2u, SEG_FRAMES / 2u); }

typedef bool (*DefaultFn)(void *face, float rate, const float *sine, float *out, uint32_t n);

static void oracle(const char *face, void *h, RunFn run, DefaultFn dflt) {
  char what[240];
  for (size_t ri = 0; ri < sizeof RATES / sizeof RATES[0]; ri++) {
    const float rate = RATES[ri];
    make_input(rate);
    reference_instance(rate);
    reference_console(rate);
    memset(out_l, 0, sizeof out_l), memset(out_r, 0, sizeof out_r);
    snprintf(what, sizeof what, "@ %.0f Hz: the programme runs", (double)rate);
    if (!run(h, rate)) {
      check(false, face, what);
      continue;
    }

    /* Bit for bit, per segment, so a red names where. */
    for (uint32_t s = 0; s < SEGMENTS; s++) {
      const uint32_t off = s * SEG_FRAMES;
      uint32_t di = 0, dc = 0;
      for (uint32_t i = off; i < off + SEG_FRAMES; i++) {
        di += out_l[i] != ref_l[i] || out_r[i] != ref_r[i];
        dc += out_l[i] != con_l[i] || out_r[i] != con_r[i];
      }
      snprintf(what, sizeof what, "@ %.0f Hz segment %u (%s): face == omx_gate_instance bit for bit (%u frames differ)",
               (double)rate, s, PROGRAMME[s].name, di);
      check(di == 0, face, what);
      snprintf(what, sizeof what, "@ %.0f Hz segment %u: face == the console's omx_dynamics_keyed bit for bit (%u frames differ)",
               (double)rate, s, dc);
      check(dc == 0, face, what);
    }

    /* The laws, measured on the face's own audio. */
    const double in0 = seg_db(in_l, 0), o0 = seg_db(out_l, 0), in1 = seg_db(in_l, 1), o1 = seg_db(out_l, 1);
    const double in2 = seg_db(in_l, 2), o2 = seg_db(out_l, 2);
    snprintf(what, sizeof what, "@ %.0f Hz: a quiet key closes the gate on a loud signal (%.1f -> %.1f dBFS)", (double)rate, in0, o0);
    check(in0 > -12.0 && in0 - o0 >= 60.0, face, what);
    snprintf(what, sizeof what, "@ %.0f Hz: a loud key opens the gate on a quiet signal (%.2f -> %.2f dBFS)", (double)rate, in1, o1);
    check(in1 < -40.0 && fabs(in1 - o1) <= 0.01, face, what);
    snprintf(what, sizeof what, "@ %.0f Hz: Self, the same quiet signal closes it (%.1f -> %.1f dBFS)", (double)rate, in2, o2);
    check(in2 - o2 >= 40.0, face, what);
    uint32_t moved3 = 0, moved4 = 0;
    for (uint32_t i = 3u * SEG_FRAMES; i < 4u * SEG_FRAMES; i++) moved3 += out_l[i] != in_l[i] || out_r[i] != in_r[i];
    for (uint32_t i = 4u * SEG_FRAMES; i < 5u * SEG_FRAMES; i++) moved4 += out_l[i] != in_l[i] || out_r[i] != in_r[i];
    snprintf(what, sizeof what, "@ %.0f Hz: the keyed bursts move the audio (%u frames)", (double)rate, moved3);
    check(moved3 > SEG_FRAMES / 2u, face, what);
    snprintf(what, sizeof what, "@ %.0f Hz: bypassed is exactly the input (%u frames differ)", (double)rate, moved4);
    check(moved4 == 0u, face, what);

    /* The latency: the 4x detector's own figure while a fast attack engages it, else 0. */
    for (uint32_t s = 0; s < SEGMENTS; s++) {
      if (isnan(latency_seen[s])) continue;
      const Ctl *c = &PROGRAMME[s];
      const int x4 = c->enabled >= 0.5f && c->attack > 0.0f && c->attack < OMX_DYN_OVS_AUTO_MS;
      const float want = x4 ? (float)omx_oversampler_latency_for(4u) : 0.0f;
      snprintf(what, sizeof what, "@ %.0f Hz segment %u: the latency port reads %.0f frames (read %.0f)", (double)rate, s,
               (double)want, (double)latency_seen[s]);
      check(latency_seen[s] == want, face, what);
    }

    /* The threshold left alone is the declared default: a sine 1.5 dB above it passes open. */
    const uint32_t n = (uint32_t)rate / QUANTUM * QUANTUM;
    const float level_db = OMX_GATE_PARAM_THRESHOLD_DEFAULT + 1.5f, amp = powf(10.0f, level_db / 20.0f);
    for (uint32_t i = 0; i < n; i++) key[i] = amp * sinf(6.283185307f * 1000.0f * (float)i / rate);
    const bool ran = dflt(h, rate, key, out_l, n);
    const double drop = db_rms(key + n / 2u, n / 2u) - db_rms(out_l + n / 2u, n / 2u);
    snprintf(what, sizeof what, "@ %.0f Hz: threshold left alone, a %.1f dBFS sine passes open (drop %.2f dB, want <= 0.1)",
             (double)rate, (double)level_db, drop);
    check(ran && drop <= 0.1, face, what);
  }
}

static int clap_main(const char *path) {
  ClapFace c = {dlopen(path, RTLD_NOW | RTLD_LOCAL), NULL, NULL};
  if (!c.lib) return fprintf(stderr, "FAIL dlopen %s: %s\n", path, dlerror()), 1;
  c.entry = (const clap_plugin_entry_t *)dlsym(c.lib, "clap_entry");
  if (!c.entry || !c.entry->init(path)) return fprintf(stderr, "FAIL clap_entry %s\n", path), 1;
  c.factory = (const clap_plugin_factory_t *)c.entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
  if (!c.factory || c.factory->get_plugin_count(c.factory) != 1) return fprintf(stderr, "FAIL factory\n"), 1;
  oracle("clap", &c, clap_run, clap_default_threshold);
  char what[160];
  for (size_t ri = 0; ri < sizeof RATES / sizeof RATES[0]; ri++) {
    const int64_t base = clap_latency(&c, RATES[ri], 1.0), x4 = clap_latency(&c, RATES[ri], 0.2);
    snprintf(what, sizeof what, "@ %.0f Hz: the latency after activation is 0 at a 1 ms attack, %u at 0.2 ms (read %lld, %lld)",
             (double)RATES[ri], omx_oversampler_latency_for(4u), (long long)base, (long long)x4);
    check(base == 0 && x4 == (int64_t)omx_oversampler_latency_for(4u), "clap", what);
  }
  c.entry->deinit();
  dlclose(c.lib);
  return failures ? 1 : 0;
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
  lv2_identity(&l, uri);
  check(lilv_plugin_instantiate(l.plugin, 0.0, NULL) == NULL, "lv2", "a zero rate is refused");
  oracle("lv2", &l, lv2_run, lv2_default_threshold);
  lilv_node_free(u), lilv_node_free(b);
  lilv_world_free(l.world);
  return failures ? 1 : 0;
}

int main(int argc, char **argv) {
  /* The faces flush denormals on every callback; the references run under the same mode. */
  omx_denormals_off();
  int rc;
  if (argc == 3 && strcmp(argv[1], "clap") == 0) rc = clap_main(argv[2]);
  else if (argc == 4 && strcmp(argv[1], "lv2") == 0) rc = lv2_main(argv[2], argv[3]);
  else return fprintf(stderr, "usage: gate-oracle clap <x.clap> | lv2 <bundle-dir> <uri>\n"), 2;
  printf("%s %s: %d checks, %d failure(s)\n", rc ? "ORACLE-RED" : "ORACLE-GREEN", argv[1], checks, failures);
  return rc;
}
