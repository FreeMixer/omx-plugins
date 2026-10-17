// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * preset-figures.c — runs a factory preset's parameter values through omx-dsp's instance face (the kernel the
 * plugin's CLAP and LV2 faces call) on a measuring signal and prints what it measured, one `key value` line each.
 * tools/test/preset-figures.mjs builds it once per kernel, passes the preset's values, and holds the figures.
 *
 *   preset-figures name=value ...        (the build picks the kernel: -DFIG_EQ=8, -DFIG_EQ=16, -DFIG_DRIVE, -DFIG_CHORUS)
 *
 * eq     gain_<f>_hz: the level change at tone <f> Hz (dB) for each f in argv `probe=<f>` (EQ on against the same tone through the flat kernel's default)
 * drive  h1_dbfs, h2_dbc, h3_dbc: a 1 kHz sine at -12 dBFS, the fundamental and its 2nd and 3rd harmonics (dBc)
 * chorus lr_null_db, lr_rms_db: a mono sine on both legs; the RMS of L - R against the RMS of L (dB), and L against R (dB)
 *
 * Every tone is a whole number of cycles in the window read, so a single DFT bin reads it exactly.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(FIG_EQ) && FIG_EQ == 8
#include "omx_eq8_params.h"
#define TABLE OMX_EQ8_PARAMS
#define COUNT OMX_EQ8_PARAM_COUNT
#include <omxdsp/fx/omx_eq_instance.h>
#elif defined(FIG_EQ) && FIG_EQ == 16
#include "omx_eq16_params.h"
#define TABLE OMX_EQ16_PARAMS
#define COUNT OMX_EQ16_PARAM_COUNT
#include <omxdsp/fx/omx_eq_instance.h>
#elif defined(FIG_DRIVE)
#include "omx_drive_params.h"
#define TABLE OMX_DRIVE_PARAMS
#define COUNT OMX_DRIVE_PARAM_COUNT
#include <omxdsp/fx/omx_drive_instance.h>
#elif defined(FIG_CHORUS)
#include "omx_chorus_params.h"
#define TABLE OMX_CHORUS_PARAMS
#define COUNT OMX_CHORUS_PARAM_COUNT
#include <omxdsp/fx/omx_chorus_instance.h>
#else
#error "build with -DFIG_EQ=8, -DFIG_EQ=16, -DFIG_DRIVE or -DFIG_CHORUS"
#endif

#define SR 48000.0
#define BLOCK 512
#define SETTLE 48000 /* 1 s the processor settles, unread */
#define WINDOW 48000 /* 1 s read: whole cycles of every tone used (1 kHz, 2.5 kHz, 250 Hz) */

static float values[COUNT];

static int index_of(const char *symbol) {
  for (int i = 0; i < COUNT; i++) if (!strcmp(TABLE[i].symbol, symbol)) return i;
  fprintf(stderr, "preset-figures: no parameter '%s'\n", symbol);
  exit(2);
}
static float v(const char *symbol) { return values[index_of(symbol)]; }

/* the DFT bin of x[0..WINDOW) at `hz`, as a magnitude */
__attribute__((unused)) static double bin(const float *x, double hz) {
  double re = 0, im = 0;
  for (int n = 0; n < WINDOW; n++) {
    const double ph = 2.0 * M_PI * hz * n / SR;
    re += x[n] * cos(ph);
    im += x[n] * sin(ph);
  }
  return 2.0 * hypot(re, im) / WINDOW;
}
__attribute__((unused)) static double rms(const float *x) {
  double s = 0;
  for (int n = 0; n < WINDOW; n++) s += (double)x[n] * x[n];
  return sqrt(s / WINDOW);
}
static double db(double a) { return 20.0 * log10(a > 1e-30 ? a : 1e-30); }

static void tone(float *x, int n, double hz, double amp) {
  for (int i = 0; i < n; i++) x[i] = (float)(amp * sin(2.0 * M_PI * hz * i / SR));
}

#define TOTAL (SETTLE + WINDOW)
static float in[TOTAL], inr[TOTAL], outl[TOTAL], outr[TOTAL];

#if defined(FIG_EQ)
static OmxEqInstance inst;
static void resolve(void) {
  int type[OMX_EQ_INSTANCE_BANDS], on[OMX_EQ_INSTANCE_BANDS];
  float freq[OMX_EQ_INSTANCE_BANDS], gain[OMX_EQ_INSTANCE_BANDS], q[OMX_EQ_INSTANCE_BANDS];
  char name[32];
  for (int b = 0; b < OMX_EQ_INSTANCE_BANDS; b++) {
    snprintf(name, sizeof name, "b%d_type", b + 1); type[b] = (int)lrintf(v(name));
    snprintf(name, sizeof name, "b%d_freq", b + 1); freq[b] = v(name);
    snprintf(name, sizeof name, "b%d_gain", b + 1); gain[b] = v(name);
    snprintf(name, sizeof name, "b%d_q", b + 1); q[b] = v(name);
    snprintf(name, sizeof name, "b%d_on", b + 1); on[b] = (int)lrintf(v(name));
  }
  omx_eq_instance_resolve(&inst, 0, (int)lrintf(v("hpf_on")), v("hpf_freq"), (int)lrintf(v("hpf_slope")), (int)lrintf(v("lpf_on")), v("lpf_freq"), (int)lrintf(v("lpf_slope")), type, freq, gain, q, on);
}
static void run(int n) {
  for (int at = 0; at < n; at += BLOCK) {
    const int f = n - at < BLOCK ? n - at : BLOCK;
    omx_eq_instance_run(&inst, in + at, inr + at, outl + at, outr + at, f);
  }
}
static void init(void) { (void)omx_eq_instance_init(&inst, (float)SR); }
#elif defined(FIG_DRIVE)
static OmxDriveInstance inst;
static void resolve(void) {
  omx_drive_instance_resolve(&inst, 0, v("amount"), v("character"), v("bandFreq"), v("mix"), v("trim"), (int)lrintf(v("curve")), (int)lrintf(v("band")), (int)lrintf(v("autoGain")), (int)lrintf(v("stereoLink")), (int)lrintf(v("hfRolloff")), (int)lrintf(v("oversample")));
}
static void run(int n) {
  for (int at = 0; at < n; at += BLOCK) {
    const int f = n - at < BLOCK ? n - at : BLOCK;
    omx_drive_instance_run(&inst, in + at, inr + at, outl + at, outr + at, f);
  }
}
static void init(void) { (void)omx_drive_instance_init(&inst, (float)SR); }
#else
static OmxChorusInstance inst;
static void resolve(void) {
  omx_chorus_instance_resolve(&inst, 0, v("spread"), v("rate"), v("depth"), v("voices"), v("mix"));
}
static void run(int n) {
  for (int at = 0; at < n; at += BLOCK) {
    const int f = n - at < BLOCK ? n - at : BLOCK;
    omx_chorus_instance_run(&inst, in + at, inr + at, outl + at, outr + at, f);
  }
}
static void init(void) { (void)omx_chorus_instance_init(&inst, (float)SR); }
#endif

/* One tone through a fresh instance at the given values; the window read is the last second. */
static void through(double hz, double amp) {
  init();
  resolve();
  tone(in, TOTAL, hz, amp);
  memcpy(inr, in, sizeof in);
  run(TOTAL);
}

int main(int argc, char **argv) {
  for (int i = 0; i < COUNT; i++) values[i] = TABLE[i].def;
  double probes[8];
  int nprobes = 0;
  for (int a = 1; a < argc; a++) {
    char *eq = strchr(argv[a], '=');
    if (!eq) { fprintf(stderr, "preset-figures: '%s' is not name=value\n", argv[a]); return 2; }
    *eq = 0;
    if (!strcmp(argv[a], "probe") && nprobes < 8) probes[nprobes++] = atof(eq + 1);
    else values[index_of(argv[a])] = (float)atof(eq + 1);
  }
#if defined(FIG_EQ)
  /* the same tone with every band off is the flat reference: the kernel's own pass-through level */
  float saved[COUNT];
  memcpy(saved, values, sizeof values);
  for (int p = 0; p < nprobes; p++) {
    char name[32];
    memcpy(values, saved, sizeof values);
    through(probes[p], 0.1);
    const double with = rms(outl + SETTLE);
    for (int b = 1; b <= OMX_EQ_INSTANCE_BANDS; b++) { snprintf(name, sizeof name, "b%d_on", b); values[index_of(name)] = 0; }
    values[index_of("hpf_on")] = 0;
    values[index_of("lpf_on")] = 0;
    through(probes[p], 0.1);
    const double flat = rms(outl + SETTLE);
    printf("gain_%g_hz %.4f\n", probes[p], db(with) - db(flat));
  }
#elif defined(FIG_DRIVE)
  (void)probes; (void)nprobes;
  through(1000.0, pow(10.0, -12.0 / 20.0));
  const double h1 = bin(outl + SETTLE, 1000.0);
  printf("h1_dbfs %.4f\nh2_dbc %.4f\nh3_dbc %.4f\n", db(h1), db(bin(outl + SETTLE, 2000.0) / h1), db(bin(outl + SETTLE, 3000.0) / h1));
#else
  (void)probes; (void)nprobes;
  through(1000.0, 0.25);
  static float diff[WINDOW];
  for (int n = 0; n < WINDOW; n++) diff[n] = outl[SETTLE + n] - outr[SETTLE + n];
  printf("lr_null_db %.4f\nlr_rms_db %.4f\n", db(rms(diff) / rms(outl + SETTLE)), db(rms(outl + SETTLE)) - db(rms(outr + SETTLE)));
#endif
  return 0;
}
