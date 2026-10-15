// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * meter-ring-stress.c — include/omx_meter_ring.h under contention: one audio thread publishing, many
 * readers copying, and every value of one publication equal to that publication's number, so a read
 * that mixes two blocks is seen at once (its values differ), and so is a read older than one the same
 * reader already had (its number went down).
 *
 *   meter-ring-stress [seconds per phase] [readers]
 *
 * Two phases: PACED, a publication every 3 µs (an audio thread at a small block), then HAMMER, one
 * publication after another with no pause. The readers outnumber the cores two to one, so the
 * scheduler preempts them in the middle of a copy and the audio thread laps them: the case the
 * ring's re-check exists for. Exit 1 on any torn or backwards read, or when no read was accepted.
 * Build with OMX_METER_RING_H_PATH to test another copy of the header (tools/meter-ring-check.sh's
 * sabotage arms); the first torn read stops the run.
 */
#define _GNU_SOURCE
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define OMX_METER_RING_MAX 64u
#ifdef OMX_METER_RING_H_PATH
#include OMX_METER_RING_H_PATH
#else
#include "omx_meter_ring.h"
#endif

#define N OMX_METER_RING_MAX

static struct omx_meter_ring ring;
static atomic_int stop, torn;
static int paced;

typedef struct {
  unsigned long long reads, accepted, refused, backwards, distinct;
  double first_bad[2];
} Tally;

static double now_us(void) {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return (double)t.tv_sec * 1e6 + (double)t.tv_nsec / 1e3;
}

static void *writer(void *arg) {
  unsigned long long *published = arg;
  double v[N], next = now_us();
  for (unsigned long long k = 1; !atomic_load_explicit(&stop, memory_order_relaxed); k++) {
    for (unsigned i = 0; i < N; i++) v[i] = (double)k;
    if (paced) {
      while (now_us() < next) {
      }
      next += 3.0;
    }
    omx_meter_ring_publish(&ring, v, N);
    *published = k;
  }
  return NULL;
}

static void *reader(void *arg) {
  Tally *t = arg;
  double v[N], last = 0.0;
  while (!atomic_load_explicit(&stop, memory_order_relaxed)) {
    t->reads++;
    if (omx_meter_ring_read(&ring, 0, v, N) != N) {
      t->refused++;
      continue;
    }
    t->accepted++;
    for (unsigned i = 1; i < N; i++)
      if (v[i] != v[0]) {
        if (atomic_fetch_add(&torn, 1) == 0) t->first_bad[0] = v[0], t->first_bad[1] = v[i];
        atomic_store(&stop, 1);
        return NULL;
      }
    if (v[0] < last) t->backwards++;
    if (v[0] != last) t->distinct++;
    last = v[0];
  }
  return NULL;
}

static int phase(const char *name, int pace, double seconds, unsigned nreaders) {
  omx_meter_ring_init(&ring);
  atomic_store(&stop, 0);
  atomic_store(&torn, 0);
  paced = pace;
  Tally *t = calloc(nreaders, sizeof *t);
  pthread_t *th = calloc(nreaders, sizeof *th), w;
  unsigned long long published = 0;
  if (!t || !th) abort();
  pthread_create(&w, NULL, writer, &published);
  for (unsigned r = 0; r < nreaders; r++) pthread_create(&th[r], NULL, reader, &t[r]);
  const double end = now_us() + seconds * 1e6;
  while (now_us() < end && !atomic_load(&torn)) usleep(1000);
  atomic_store(&stop, 1);
  pthread_join(w, NULL);
  Tally sum = {0};
  double bad[2] = {0, 0};
  for (unsigned r = 0; r < nreaders; r++) {
    pthread_join(th[r], NULL);
    sum.reads += t[r].reads, sum.accepted += t[r].accepted, sum.refused += t[r].refused;
    sum.backwards += t[r].backwards, sum.distinct += t[r].distinct;
    if (t[r].first_bad[0] != t[r].first_bad[1]) bad[0] = t[r].first_bad[0], bad[1] = t[r].first_bad[1];
  }
  const int n_torn = atomic_load(&torn);
  printf("%s %s: %llu publications, %u readers, %llu reads: %llu whole, %llu refused after %u tries, %d torn, %llu "
         "backwards, %llu changes seen\n",
         n_torn || sum.backwards || !sum.accepted ? "FAIL" : "PASS", name, published, nreaders, sum.reads,
         sum.accepted, sum.refused, OMX_METER_RING_TRIES, n_torn, sum.backwards, sum.distinct);
  if (n_torn) printf("  a read mixed publication %.0f with publication %.0f\n", bad[0], bad[1]);
  free(t);
  free(th);
  return n_torn || sum.backwards || !sum.accepted;
}

int main(int argc, char **argv) {
  const double seconds = argc > 1 ? atof(argv[1]) : 1.0;
  long cpus = sysconf(_SC_NPROCESSORS_ONLN);
  const unsigned readers = argc > 2 ? (unsigned)atoi(argv[2]) : (unsigned)(cpus > 1 ? 2 * cpus : 2);
  int bad = phase("paced (3 us)", 1, seconds, readers);
  bad |= phase("hammer", 0, seconds, readers);
  return bad ? 1 : 0;
}
