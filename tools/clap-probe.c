// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * clap-probe.c — a minimal CLAP host over ANY built omx plugin: it knows no plugin, only CLAP.
 *
 *   clap-probe params <x.clap>
 *       one line per parameter, as clap_plugin_params.get_info reports it:
 *       `id<TAB>name<TAB>min<TAB>max<TAB>default<TAB>flags` — tools/clap-params-check.mjs compares
 *       it with the declaration.
 *   clap-probe live <x.clap>
 *       every parameter but the host's bypass MOVES the output: a deterministic stereo signal (noise
 *       bursts with quiet gaps) is run
 *       with the parameter at its default and at the far end of its travel, in four settings of
 *       the others until one shows a difference: (a) all at their defaults; (b) every other toggle
 *       on (a filter's frequency moves nothing while the filter is off); (c) every other parameter
 *       at the far end of its travel (a bell's frequency moves nothing at 0 dB); (d) every other
 *       toggle on and every other travel at its far end; (e) every other choice (stepped from 0
 *       to at most 15, not a toggle) at its far value, the rest at their defaults: a choice that selects which of
 *       several controls is heard (a speed switch) hides them until it is moved, and a far
 *       value of another travel (a mix at 0) can hide them again. A parameter whose output is
 *       byte-identical in all five, and under every value of every other choice or toggle in turn (the
 *       rest at their defaults: a control read only under one mode), and with every other toggle on and
 *       each other travel in turn at either end of it (the rest at their defaults: a strip's gate hold is
 *       heard only once its trim lowers the quiet bed under the gate's threshold), is one the face does
 *       not deliver.
 *       `PASS live <name>` / `FAIL live <name>` per parameter. A face still on the wizard's stub
 *       binding is red here, by design. Every input port past the main one (a sidechain key) is
 *       connected too, fed a signal of its own: square-wave bursts the main signal does not have, so a
 *       parameter that picks what the detector listens to can move the output.
 */
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clap/entry.h"
#include "clap/ext/audio-ports.h"
#include "clap/ext/params.h"
#include "clap/factory/plugin-factory.h"
#include "clap/host.h"

#define RATE 48000.0
#define BLOCK 256u
#define FRAMES (282u * BLOCK) /* 1.5 s at 48 kHz in whole blocks: a delay's second repeat */
#define MAX_PARAMS 256u
#define MAX_SIDE 8u /* channels over every input port past the main one */

static const void *host_get_extension(const clap_host_t *h, const char *id) {
  (void)h, (void)id;
  return NULL;
}
static void host_noop(const clap_host_t *h) { (void)h; }

static const clap_host_t HOST = {
    CLAP_VERSION_INIT, NULL, "omx clap-probe", "openmixer", "", "0.1",
    host_get_extension, host_noop, host_noop, host_noop,
};

typedef struct {
  clap_event_param_value_t ev[MAX_PARAMS];
  uint32_t n;
} EvList;

static uint32_t ev_size(const clap_input_events_t *l) { return ((const EvList *)l->ctx)->n; }
static const clap_event_header_t *ev_get(const clap_input_events_t *l, uint32_t i) {
  return &((const EvList *)l->ctx)->ev[i].header;
}
static bool ev_push(const clap_output_events_t *l, const clap_event_header_t *e) {
  (void)l, (void)e;
  return true;
}

typedef struct {
  void *so;
  const clap_plugin_entry_t *entry;
  const clap_plugin_t *plugin;
  const clap_plugin_params_t *params;
} Clap;

static int clap_open(Clap *c, const char *path) {
  memset(c, 0, sizeof *c);
  c->so = dlopen(path, RTLD_NOW | RTLD_LOCAL);
  if (!c->so) return fprintf(stderr, "dlopen %s: %s\n", path, dlerror()), 0;
  c->entry = (const clap_plugin_entry_t *)dlsym(c->so, "clap_entry");
  if (!c->entry || !c->entry->init(path)) return fprintf(stderr, "no clap_entry\n"), 0;
  const clap_plugin_factory_t *fac = (const clap_plugin_factory_t *)c->entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
  if (!fac || fac->get_plugin_count(fac) != 1) return fprintf(stderr, "factory\n"), 0;
  const clap_plugin_descriptor_t *d = fac->get_plugin_descriptor(fac, 0);
  c->plugin = fac->create_plugin(fac, &HOST, d->id);
  if (!c->plugin || !c->plugin->init(c->plugin)) return fprintf(stderr, "create/init\n"), 0;
  c->params = (const clap_plugin_params_t *)c->plugin->get_extension(c->plugin, CLAP_EXT_PARAMS);
  if (!c->params) return fprintf(stderr, "no params extension\n"), 0;
  return 1;
}

static void clap_close(Clap *c) {
  if (c->plugin) c->plugin->destroy(c->plugin);
  if (c->entry) c->entry->deinit();
  if (c->so) dlclose(c->so);
}

static int params_dump(const char *path) {
  Clap c;
  if (!clap_open(&c, path)) return 1;
  uint32_t n = c.params->count(c.plugin);
  for (uint32_t i = 0; i < n; i++) {
    clap_param_info_t info;
    if (!c.params->get_info(c.plugin, i, &info)) return fprintf(stderr, "get_info %u\n", i), 1;
    printf("%u\t%s\t%.17g\t%.17g\t%.17g\t%u\n", info.id, info.name, info.min_value, info.max_value,
           info.default_value, (unsigned)info.flags);
  }
  clap_close(&c);
  return 0;
}

/** Deterministic stereo noise with sparse impulses, the legs different, in 0.25 s bursts each followed
 * by 0.5 s of a quiet bed: a gate closes, a dynamics stage releases, a reverb's tail is heard. */
static void make_signal(float *l, float *r) {
  uint32_t seed = 0x2545F491u;
  for (uint32_t i = 0; i < FRAMES; i++) {
    const float g = (i % 36000u) < 12000u ? 1.0f : 0.025f;
    seed = seed * 1664525u + 1013904223u;
    l[i] = ((float)(seed >> 8) / 16777216.0f - 0.5f) * 0.8f * g;
    seed = seed * 1664525u + 1013904223u;
    r[i] = ((float)(seed >> 8) / 16777216.0f - 0.5f) * 0.6f * g;
    if (i % 4800u == 0 && g == 1.0f) l[i] = r[i] = 0.9f;
  }
}

/** The side inputs' signal: a 1 kHz square wave at half scale, on for the first 0.1 s of every
 * second and silent for the rest, long enough for a detector's slowest release to tell. */
static void make_side(float *k) {
  for (uint32_t i = 0; i < FRAMES; i++) k[i] = (i % 48000u) < 4800u ? 0.5f * (float)((i / 24u) % 2u ? 1 : -1) : 0.0f;
}

static float *side;

/** One run from a fresh instance, the `n` (id, value) pairs set before the first block. */
static int render(const char *path, const clap_id *ids, const double *vals, uint32_t n, const float *in_l,
                  const float *in_r, float *out_l, float *out_r) {
  Clap c;
  if (!clap_open(&c, path)) return 0;
  if (!c.plugin->activate(c.plugin, RATE, 1, BLOCK) || !c.plugin->start_processing(c.plugin)) {
    clap_close(&c);
    return fprintf(stderr, "activate\n"), 0;
  }
  /* The input ports the plugin declares past the main one, each channel fed the side signal. */
  const clap_plugin_audio_ports_t *ap = (const clap_plugin_audio_ports_t *)c.plugin->get_extension(c.plugin, CLAP_EXT_AUDIO_PORTS);
  uint32_t nin = ap ? ap->count(c.plugin, true) : 1u, nside = 0;
  clap_audio_buffer_t ain[1 + MAX_SIDE];
  float *side_ch[MAX_SIDE];
  uint32_t side_first[MAX_SIDE];
  if (nin < 1u) nin = 1u;
  for (uint32_t k = 1; k < nin && k <= MAX_SIDE; k++) {
    clap_audio_port_info_t pi;
    uint32_t ch = ap->get(c.plugin, k, true, &pi) ? pi.channel_count : 0u;
    side_first[k - 1] = nside;
    for (; ch > 0 && nside < MAX_SIDE; ch--) nside++;
    ain[k] = (clap_audio_buffer_t){side_ch + side_first[k - 1], NULL, nside - side_first[k - 1], 0, 0};
  }
  if (nin > 1u + MAX_SIDE) nin = 1u + MAX_SIDE;
  static EvList evs;
  for (uint32_t f = 0; f < FRAMES; f += BLOCK) {
    for (uint32_t k = 0; k < nside; k++) side_ch[k] = side + f;
    evs.n = 0;
    for (uint32_t k = 0; f == 0 && k < n; k++) {
      clap_event_param_value_t *e = &evs.ev[evs.n++];
      memset(e, 0, sizeof *e);
      e->header.size = sizeof *e;
      e->header.space_id = CLAP_CORE_EVENT_SPACE_ID;
      e->header.type = CLAP_EVENT_PARAM_VALUE;
      e->param_id = ids[k];
      e->note_id = -1;
      e->port_index = -1;
      e->channel = -1;
      e->key = -1;
      e->value = vals[k];
    }
    clap_input_events_t in_ev = {&evs, ev_size, ev_get};
    clap_output_events_t out_ev = {NULL, ev_push};
    float *ins[2] = {(float *)in_l + f, (float *)in_r + f};
    float *outs[2] = {out_l + f, out_r + f};
    ain[0] = (clap_audio_buffer_t){ins, NULL, 2, 0, 0};
    clap_audio_buffer_t aout = {outs, NULL, 2, 0, 0};
    clap_process_t pr;
    memset(&pr, 0, sizeof pr);
    pr.steady_time = f;
    pr.frames_count = BLOCK;
    pr.audio_inputs = ain;
    pr.audio_outputs = &aout;
    pr.audio_inputs_count = nin;
    pr.audio_outputs_count = 1;
    pr.in_events = &in_ev;
    pr.out_events = &out_ev;
    if (c.plugin->process(c.plugin, &pr) == CLAP_PROCESS_ERROR) {
      clap_close(&c);
      return fprintf(stderr, "process error\n"), 0;
    }
  }
  c.plugin->stop_processing(c.plugin);
  c.plugin->deactivate(c.plugin);
  clap_close(&c);
  return 1;
}

static double far_of(const clap_param_info_t *p) { return p->default_value == p->max_value ? p->min_value : p->max_value; }
/** A choice: stepped, counting from 0 to at most 15 (a mode, a speed), not a toggle. A stepped travel in
 * whole units (a balance in percent) is not one. */
static int is_choice(const clap_param_info_t *p) {
  return (p->flags & CLAP_PARAM_IS_STEPPED) && p->min_value == 0.0 && p->max_value >= 2.0 && p->max_value <= 15.0;
}
static int is_toggle(const clap_param_info_t *p) {
  return (p->flags & CLAP_PARAM_IS_STEPPED) && p->min_value == 0.0 && p->max_value == 1.0;
}

static float *in_l, *in_r, *a_l, *a_r, *b_l, *b_r;

/** Does moving parameter `i` from its default to its far value change the output, the others set by
 * `setting` (0 defaults, 1 toggles on, 2 at their far values, 3 toggles on and the rest far, 4 the
 * other stepped choices far and the rest at their defaults, 5 the other choice or toggle `k` at
 * `kv` and the rest at their defaults, 6 every other toggle on and the travel `k` at `kv`, the rest at
 * their defaults)? -1 on a host error. */
static int moves(const char *path, const clap_param_info_t *info, uint32_t n, uint32_t i, int setting, uint32_t k_at,
                 double kv) {
  static clap_id ids[MAX_PARAMS];
  static double vals[MAX_PARAMS];
  uint32_t m = 0;
  for (uint32_t k = 0; k < n; k++) {
    if (k == i || (info[k].flags & CLAP_PARAM_IS_BYPASS)) continue;
    if (setting == 1 && is_toggle(&info[k])) ids[m] = info[k].id, vals[m++] = 1.0;
    if (setting == 2) ids[m] = info[k].id, vals[m++] = far_of(&info[k]);
    if (setting == 3) ids[m] = info[k].id, vals[m++] = is_toggle(&info[k]) ? 1.0 : far_of(&info[k]);
    if (setting == 4 && is_choice(&info[k])) ids[m] = info[k].id, vals[m++] = far_of(&info[k]);
    if (setting == 5 && k == k_at) ids[m] = info[k].id, vals[m++] = kv;
    if (setting == 6 && (k == k_at || is_toggle(&info[k]))) ids[m] = info[k].id, vals[m++] = k == k_at ? kv : 1.0;
  }
  ids[m] = info[i].id;
  vals[m] = info[i].default_value;
  if (!render(path, ids, vals, m + 1, in_l, in_r, a_l, a_r)) return -1;
  vals[m] = far_of(&info[i]);
  if (!render(path, ids, vals, m + 1, in_l, in_r, b_l, b_r)) return -1;
  return memcmp(a_l, b_l, FRAMES * sizeof(float)) != 0 || memcmp(a_r, b_r, FRAMES * sizeof(float)) != 0;
}

static int live(const char *path) {
  Clap c;
  if (!clap_open(&c, path)) return 1;
  const uint32_t n = c.params->count(c.plugin);
  if (n > MAX_PARAMS) return fprintf(stderr, "%u parameters, more than %u\n", n, MAX_PARAMS), 1;
  clap_param_info_t *info = calloc(n, sizeof *info);
  for (uint32_t i = 0; i < n; i++) c.params->get_info(c.plugin, i, &info[i]);
  clap_close(&c);

  float **bufs[] = {&in_l, &in_r, &a_l, &a_r, &b_l, &b_r, &side};
  for (size_t k = 0; k < 7; k++) *bufs[k] = malloc(FRAMES * sizeof(float));
  make_signal(in_l, in_r);
  make_side(side);
  static const char *const SETTING[] = {"the others at their defaults", "every other toggle on", "the others at their far ends",
                                        "every other toggle on, the rest at their far ends",
                                        "the other choices at their far values"};
  int fails = 0, checked = 0;
  for (uint32_t i = 0; i < n; i++) {
    if (info[i].flags & CLAP_PARAM_IS_BYPASS) continue;
    checked++;
    int setting = 0, r = 0;
    for (; setting < 5; setting++) {
      r = moves(path, info, n, i, setting, 0, 0.0);
      if (r != 0) break;
    }
    /* then each value of every other choice or toggle in turn: a control read only under one value
     * (a reverb's plate depth under the plate algorithm) is moved where it is read */
    uint32_t k_at = 0;
    double kv = 0.0;
    for (uint32_t k = 0; r == 0 && k < n; k++) {
      if (k == i || !(is_choice(&info[k]) || is_toggle(&info[k]))) continue;
      for (double v = info[k].min_value; r == 0 && v <= info[k].max_value; v += 1.0) {
        if (v == info[k].default_value) continue;
        r = moves(path, info, n, i, 5, k, v);
        k_at = k, kv = v;
      }
    }
    /* then, every other toggle on, each other travel in turn at either end of it */
    int ends = 0;
    for (uint32_t k = 0; r == 0 && k < n; k++) {
      if (k == i || is_choice(&info[k]) || is_toggle(&info[k]) || (info[k].flags & CLAP_PARAM_IS_BYPASS)) continue;
      const double end[2] = {info[k].min_value, info[k].max_value};
      for (int e = 0; r == 0 && e < 2; e++) {
        if (end[e] == info[k].default_value) continue;
        r = moves(path, info, n, i, 6, k, end[e]);
        k_at = k, kv = end[e], ends = 1;
      }
    }
    if (r < 0) return 1;
    if (r && ends) {
      printf("PASS live %s moves the output (every other toggle on, %s at %g, the rest at their defaults)\n", info[i].name, info[k_at].name, kv);
    } else if (r && setting < 5) {
      printf("PASS live %s moves the output (%s)\n", info[i].name, SETTING[setting]);
    } else if (r) {
      printf("PASS live %s moves the output (%s at %g, the rest at their defaults)\n", info[i].name, info[k_at].name, kv);
    } else {
      printf("FAIL live %s: %g leaves the output byte-identical in every setting; the face does not deliver it to the kernel\n",
             info[i].name, far_of(&info[i]));
      fails++;
    }
  }
  free(info);
  for (size_t k = 0; k < 7; k++) free(*bufs[k]);
  if (checked == 0) return printf("FAIL live: no parameter to move\n"), 1;
  return fails ? 1 : 0;
}

int main(int argc, char **argv) {
  if (argc == 3 && strcmp(argv[1], "params") == 0) return params_dump(argv[2]);
  if (argc == 3 && strcmp(argv[1], "live") == 0) return live(argv[2]);
  fprintf(stderr, "usage: clap-probe params|live <x.clap>\n");
  return 2;
}
