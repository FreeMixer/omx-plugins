// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * meter-held-face.c — a built face holds its meters until the host reads them (docs/meters.md): a minimal CLAP
 * host runs the face over silence, one 10 ms burst of samples over full scale and twenty blocks of silence
 * again, and reads the meters once, after the silence. What the face gave must still show the burst: the input
 * level's peak, the gain reduction's depth and the clip word's run. A second read after more silence must have
 * recovered: the floor, no reduction, no run. Exit 1 on the first value that is not what the stimulus says.
 *
 *   meter-held-face <x.clap>
 *
 * The face is a compressor (omx-comp): meters compGr, compIn, compOut, compKey. Used by tools/meter-held-check.sh.
 */
#include <dlfcn.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <omx-clap-host/clap_meters.h>

#include "clap/entry.h"
#include "clap/factory/plugin-factory.h"
#include "clap/host.h"
#include "clap/process.h"
#include "omx_clap_face.h"

#define RATE 48000.0
#define BLOCK 480u /* 10 ms */

static const void *host_get_extension(const clap_host_t *h, const char *id) { (void)h, (void)id; return NULL; }
static void host_noop(const clap_host_t *h) { (void)h; }
static const clap_host_t HOST = {CLAP_VERSION_INIT, NULL, "omx meter-held-face", "openmixer", "", "0.1",
                                 host_get_extension, host_noop, host_noop, host_noop};
static uint32_t ev_size(const clap_input_events_t *l) { (void)l; return 0; }
static const clap_event_header_t *ev_get(const clap_input_events_t *l, uint32_t i) { (void)l, (void)i; return NULL; }
static bool ev_push(const clap_output_events_t *l, const clap_event_header_t *e) { (void)l, (void)e; return true; }

static const clap_plugin_t *P;
static int bad;

static void run(float amp, unsigned blocks) {
  static float il[BLOCK], ir[BLOCK], ol[BLOCK], or_[BLOCK];
  float *in[2] = {il, ir}, *out[2] = {ol, or_};
  clap_audio_buffer_t ai = {in, NULL, 2, 0, 0}, ao = {out, NULL, 2, 0, 0};
  const clap_input_events_t ev_in = {NULL, ev_size, ev_get};
  const clap_output_events_t ev_out = {NULL, ev_push};
  for (unsigned b = 0; b < blocks; b++) {
    for (unsigned i = 0; i < BLOCK; i++) il[i] = ir[i] = amp;
    clap_process_t pr = {0};
    pr.frames_count = BLOCK;
    pr.audio_inputs = &ai, pr.audio_inputs_count = 1;
    pr.audio_outputs = &ao, pr.audio_outputs_count = 1;
    pr.in_events = &ev_in, pr.out_events = &ev_out;
    if (P->process(P, &pr) == CLAP_PROCESS_ERROR) fprintf(stderr, "process failed\n"), exit(1);
  }
}

static void expect(int ok, const char *what, double got) {
  printf("%s %s (%.6g)\n", ok ? "ok  " : "FAIL", what, got);
  if (!ok) bad = 1;
}

int main(int argc, char **argv) {
  if (argc != 2) return fprintf(stderr, "usage: meter-held-face <x.clap>\n"), 2;
  void *so = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
  if (!so) return fprintf(stderr, "dlopen %s: %s\n", argv[1], dlerror()), 1;
  const clap_plugin_entry_t *entry = (const clap_plugin_entry_t *)dlsym(so, "clap_entry");
  if (!entry || !entry->init(argv[1])) return fprintf(stderr, "no clap_entry\n"), 1;
  const clap_plugin_factory_t *fac = (const clap_plugin_factory_t *)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
  if (!fac || fac->get_plugin_count(fac) != 1) return fprintf(stderr, "factory\n"), 1;
  P = fac->create_plugin(fac, &HOST, fac->get_plugin_descriptor(fac, 0)->id);
  if (!P || !P->init(P)) return fprintf(stderr, "create/init\n"), 1;
  const clap_plugin_meters_t *m = (const clap_plugin_meters_t *)P->get_extension(P, CLAP_EXT_METERS);
  const omx_clap_plugin_meter_clip_t *clip = (const omx_clap_plugin_meter_clip_t *)P->get_extension(P, OMX_CLAP_EXT_METER_CLIP);
  if (!m || !clip) return fprintf(stderr, "no clap.meters/1 or no clip word\n"), 1;
  if (!P->activate(P, RATE, 1, BLOCK) || !P->start_processing(P)) return fprintf(stderr, "activate\n"), 1;
  /* meter ids by name: the face lists compGr, compIn, compOut, compKey */
  int gr = -1, in = -1;
  for (uint32_t k = 0; k < m->count(P); k++) {
    clap_meter_info_t info;
    if (m->get_info(P, k, &info)) {
      if (strcmp(info.name, "compGr") == 0) gr = (int)k;
      if (strcmp(info.name, "compIn") == 0) in = (int)k;
    }
  }
  if (gr < 0 || in < 0) return fprintf(stderr, "meters not found\n"), 1;

  double v[2], runs[2];
  uint32_t nv, nr;
  run(0.0f, 3);
  nv = m->get_values(P, (clap_id)in, v, 2);
  expect(nv == 2, "a read after the first blocks", nv);
  run(0.0f, 3);
  run(1.2f, 1); /* the burst: 10 ms, 1.2 on both legs: +1.58 dBFS and 480 samples over full scale */
  run(0.0f, 20);
  nv = m->get_values(P, (clap_id)in, v, 2);
  expect(nv == 2 && fabs(v[0] - 20.0 * log10(1.2)) < 0.01 && fabs(v[1] - 20.0 * log10(1.2)) < 0.01,
         "the input level still shows the burst after twenty silent blocks", v[0]);
  nr = clip->get(P, (clap_id)in, runs, 2);
  expect(nr == 2 && runs[0] == (double)BLOCK && runs[1] == (double)BLOCK, "the clip word holds the burst's run", runs[0]);
  nv = m->get_values(P, (clap_id)gr, v, 1);
  const double deepest = v[0];
  expect(nv == 1 && deepest < -0.1, "the gain reduction holds its deepest", deepest);
  run(0.0f, 3);
  nv = m->get_values(P, (clap_id)in, v, 2);
  expect(nv == 2 && v[0] < -50.0 && v[1] < -50.0, "the next read has recovered: the floor", v[0]);
  nr = clip->get(P, (clap_id)in, runs, 2);
  expect(nr == 2 && runs[0] == 0.0 && runs[1] == 0.0, "the clip word recovered", runs[0]);
  nv = m->get_values(P, (clap_id)gr, v, 1);
  expect(nv == 1 && v[0] > deepest + 0.5 && v[0] < 0.1, "the gain reduction is shallower: the release, not the burst's deepest", v[0]);
  P->stop_processing(P);
  P->deactivate(P);
  P->destroy(P);
  return bad;
}
