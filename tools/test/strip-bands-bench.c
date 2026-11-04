// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * strip-bands-bench.c — what omx-strip's EQ costs by its band count (FreeMixer/omx-plugins#308): the built
 * CLAP face, the same host calls a console makes, timed in this process's CPU clock.
 *
 *   strip-bands-bench <omx-strip.clap> [<rate> ...]      (default 48000 96000)
 *
 * For each rate and each count of 4, 8, 16 and 32 bands: Bands set to the count, every band up to it on as a
 * bell at +6 dB (so none is parked at 0 dB and skipped), Q 1, centres a third of an octave apart from 50 Hz,
 * every band past it on and boosted too (the count alone keeps them out); the input, the filters and the EQ on,
 * the gate and the compressor off (the EQ alone is measured, over the strip's fixed cost). Ten seconds of a
 * stereo noise burst in blocks of 128 frames, after a second of warm-up, best of five runs. Prints, per row,
 * the CPU time per block in microseconds and the share of one core the strip takes in real time (the block's
 * CPU time over its duration), and the row with the EQ switched off: the strip's cost without its EQ.
 *
 * A plain process clock (CLOCK_PROCESS_CPUTIME_ID), not a real-time thread: a figure for the band count's
 * growth on one machine, to set beside the Pi's and the desk's, never a real-time budget by itself.
 */
#include <dlfcn.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <clap/clap.h>

#define BLOCK 128u
#define SECONDS 10u
#define WARM 1u
#define RUNS 5
#define MAXP 512

static const void *h_ext(const clap_host_t *h, const char *id) {
  (void)h, (void)id;
  return NULL;
}
static void h_noop(const clap_host_t *h) { (void)h; }
static const clap_host_t HOST = {CLAP_VERSION_INIT, NULL, "strip-bands-bench", "openmixer", "", "1", h_ext, h_noop, h_noop, h_noop};

static const clap_plugin_factory_t *factory;
static uint32_t nparams;
static clap_id ids[MAXP];
static char names[MAXP][CLAP_NAME_SIZE];
static double maxes[MAXP];

static int param(const char *name) {
  for (uint32_t i = 0; i < nparams; i++)
    if (strcmp(names[i], name) == 0) return (int)i;
  return -1;
}

typedef struct {
  clap_event_param_value_t ev[MAXP];
  uint32_t n;
} Events;
static uint32_t ev_size(const clap_input_events_t *l) { return ((const Events *)l->ctx)->n; }
static const clap_event_header_t *ev_get(const clap_input_events_t *l, uint32_t i) { return &((const Events *)l->ctx)->ev[i].header; }
static bool ev_push(const clap_output_events_t *l, const clap_event_header_t *e) { (void)l, (void)e; return true; }
static void set(Events *e, const char *name, double v) {
  const int i = param(name);
  if (i < 0) fprintf(stderr, "no parameter '%s'\n", name), exit(2);
  clap_event_param_value_t *x = &e->ev[e->n++];
  memset(x, 0, sizeof *x);
  x->header = (clap_event_header_t){sizeof *x, 0, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_PARAM_VALUE, 0};
  x->param_id = ids[i], x->note_id = -1, x->port_index = -1, x->channel = -1, x->key = -1, x->value = v;
}

static double now(void) {
  struct timespec t;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);
  return (double)t.tv_sec + 1e-9 * (double)t.tv_nsec;
}

/* CPU seconds for SECONDS of audio at `sr` through one instance set to `bands` (0: the EQ switched off). */
static double run(double sr, int bands, int maxb, const float *il, const float *ir, float *ol, float *orr) {
  const clap_plugin_t *p = factory->create_plugin(factory, &HOST, factory->get_plugin_descriptor(factory, 0)->id);
  if (!p || !p->init(p) || !p->activate(p, sr, BLOCK, BLOCK) || !p->start_processing(p)) fprintf(stderr, "plugin\n"), exit(2);
  Events evs = {.n = 0};
  char s[64];
  set(&evs, "EQ", bands ? 1.0 : 0.0);
  set(&evs, "Gate", 0.0);
  set(&evs, "Comp", 0.0);
  set(&evs, "EQ Bands", bands ? bands : 4);
  for (int b = 1; b <= maxb; b++) {
    snprintf(s, sizeof s, "EQ %d", b), set(&evs, s, 1.0);
    snprintf(s, sizeof s, "EQ %d Type", b), set(&evs, s, 0.0);
    snprintf(s, sizeof s, "EQ %d Gain", b), set(&evs, s, 6.0);
    snprintf(s, sizeof s, "EQ %d Q", b), set(&evs, s, 1.0);
    snprintf(s, sizeof s, "EQ %d Freq", b), set(&evs, s, round(50.0 * pow(2.0, (b - 1) / 3.0)));
  }
  clap_input_events_t in = {&evs, ev_size, ev_get}, none = {&(Events){.n = 0}, ev_size, ev_get};
  clap_output_events_t out = {NULL, ev_push};
  const uint32_t blocks = (uint32_t)(sr / BLOCK) * SECONDS, warm = (uint32_t)(sr / BLOCK) * WARM;
  double t0 = 0;
  for (uint32_t k = 0; k < warm + blocks; k++) {
    if (k == warm) t0 = now();
    const uint32_t at = (k % 64u) * BLOCK;
    float *ib[2] = {(float *)il + at, (float *)ir + at}, *ob[2] = {ol, orr};
    clap_audio_buffer_t ai = {ib, NULL, 2, 0, 0}, ao = {ob, NULL, 2, 0, 0};
    clap_process_t pr = {.steady_time = (int64_t)k * BLOCK, .frames_count = BLOCK, .audio_inputs = &ai, .audio_outputs = &ao,
                         .audio_inputs_count = 1, .audio_outputs_count = 1, .in_events = k ? &none : &in, .out_events = &out};
    if (p->process(p, &pr) == CLAP_PROCESS_ERROR) fprintf(stderr, "process\n"), exit(2);
  }
  const double t = now() - t0;
  p->stop_processing(p);
  p->deactivate(p);
  p->destroy(p);
  return t;
}

int main(int argc, char **argv) {
  if (argc < 2) return fprintf(stderr, "usage: strip-bands-bench <omx-strip.clap> [<rate> ...]\n"), 2;
  void *lib = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
  const clap_plugin_entry_t *e = lib ? (const clap_plugin_entry_t *)dlsym(lib, "clap_entry") : NULL;
  if (!e || !e->init(argv[1])) return fprintf(stderr, "cannot load %s\n", argv[1]), 2;
  factory = (const clap_plugin_factory_t *)e->get_factory(CLAP_PLUGIN_FACTORY_ID);
  const clap_plugin_t *p = factory->create_plugin(factory, &HOST, factory->get_plugin_descriptor(factory, 0)->id);
  if (!p || !p->init(p)) return 2;
  const clap_plugin_params_t *pp = (const clap_plugin_params_t *)p->get_extension(p, CLAP_EXT_PARAMS);
  nparams = pp->count(p);
  for (uint32_t i = 0; i < nparams && i < MAXP; i++) {
    clap_param_info_t info;
    pp->get_info(p, i, &info);
    ids[i] = info.id, maxes[i] = info.max_value;
    snprintf(names[i], sizeof names[i], "%s", info.name);
  }
  p->destroy(p);
  const int bands = param("EQ Bands");
  if (bands < 0) return fprintf(stderr, "%s has no 'EQ Bands'\n", argv[1]), 2;
  /* every band slot the plugin has: the count's maximum */
  const int maxb = (int)maxes[bands];
  float *il = malloc(64u * BLOCK * sizeof(float)), *ir = malloc(64u * BLOCK * sizeof(float)), ol[BLOCK], orr[BLOCK];
  uint32_t seed = 0x62656e63u;
  for (uint32_t i = 0; i < 64u * BLOCK; i++) {
    seed = seed * 1664525u + 1013904223u;
    il[i] = 0.5f * ((float)(seed >> 8) / 16777216.0f - 0.5f);
    seed = seed * 1664525u + 1013904223u;
    ir[i] = 0.5f * ((float)(seed >> 8) / 16777216.0f - 0.5f);
  }
  static const int COUNTS[] = {0, 4, 8, 16, 32};
  if (maxb < 32) return fprintf(stderr, "%s holds %d bands, fewer than the 32 measured\n", argv[1], maxb), 2;
  printf("rate\tbands\tus/block(%u)\t%%core\n", BLOCK);
  for (int r = 2; r < argc + (argc == 2 ? 2 : 0); r++) {
    const double sr = argc == 2 ? (r == 2 ? 48000.0 : 96000.0) : atof(argv[r]);
    for (size_t c = 0; c < sizeof COUNTS / sizeof *COUNTS; c++) {
      double best = 1e30;
      for (int k = 0; k < RUNS; k++) {
        const double t = run(sr, COUNTS[c], maxb, il, ir, ol, orr);
        if (t < best) best = t;
      }
      const double blocks = (double)((uint32_t)(sr / BLOCK) * SECONDS);
      const double us = 1e6 * best / blocks, pct = 100.0 * best / (blocks * BLOCK / sr);
      if (COUNTS[c]) printf("%.0f\t%d\t%.2f\t%.3f\n", sr, COUNTS[c], us, pct);
      else printf("%.0f\tEQ off\t%.2f\t%.3f\n", sr, us, pct);
    }
  }
  free(il), free(ir);
  return 0;
}
