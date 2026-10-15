/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
 *
 * omx_meter_ring.h — the values of a plugin's meters, handed from the audio thread to any number of
 * readers so that one read never mixes two blocks: what clap.meters/1's CLAP_METER_IS_COHERENT
 * promises (free-audio/clap#521). Plain C11, header only, no allocation, no lock; the audio thread
 * and every reader finish in a bounded number of steps.
 *
 * The recipe the draft's header documents: three copies of the value array and one atomic word
 * naming the current copy. After each block the audio thread fills a copy no reader can hold and
 * then stores the word; a reader loads the word and copies that array. One step more makes the copy
 * provably whole: the reader loads the word again after copying and keeps the copy only if the
 * audio thread published at most once in between.
 *
 * WHY THAT CHECK IS ENOUGH (the three-copy argument). The word carries a publication count `s` and
 * the copy `c` that publication filled; copies are filled in turn, 0 1 2 0 1 2 …, so publication
 * s + k fills copy (c + k) mod 3, and the first one to write copy `c` again is s + 3. The audio
 * thread starts filling for s + 3 only after it stored the word of s + 2 (program order, and the
 * release fence at the start of publish() keeps that store ahead of the fill for every reader that
 * sees any value of the fill). A reader that saw word s, copied, issued an acquire fence and then
 * loaded word s' <= s + 1 has therefore not seen any store of s + 3 or later: if it had, the fence
 * pair would make the word of s + 2 visible to its second load. And it sees every value of s, which
 * the acquire load of the word ordered before its copy. So an accepted copy is exactly publication
 * s, whole. With two copies the overwrite would start after s + 1 and no publication at all could
 * land during a read; with three the reader survives one whole publication and a second one in
 * progress, which is two blocks of slack for a copy of a few doubles.
 *
 * A reader that finds s' >= s + 2 (it was preempted across two blocks) copies again, at most
 * OMX_METER_RING_TRIES times, and then reports nothing rather than a torn copy: both sides stay
 * wait-free. The count is 30 bits and skips 0 on wrapping, so 0 can mean "nothing published"; a
 * reader stalled for 2^30 blocks (hours, at the smallest block at the highest rate) could in theory
 * be fooled, and no other reader can.
 */
#ifndef OMX_METER_RING_H
#define OMX_METER_RING_H

#include <stdatomic.h>
#include <stdint.h>
#include <string.h>

/** The most values one ring carries: every channel of every meter of one plugin. */
#ifndef OMX_METER_RING_MAX
#define OMX_METER_RING_MAX 16u
#endif
/** How many times a reader copies before it gives up on a read (see above). */
#ifndef OMX_METER_RING_TRIES
#define OMX_METER_RING_TRIES 4u
#endif
#define OMX_METER_RING_COPIES 3u
#define OMX_METER_RING_COUNT_MASK 0x3fffffffu

_Static_assert(ATOMIC_INT_LOCK_FREE == 2 && ATOMIC_LLONG_LOCK_FREE == 2, "the ring needs lock-free atomics");
_Static_assert(sizeof(double) == sizeof(unsigned long long), "a value travels as its 64 bits");

struct omx_meter_ring {
  _Atomic unsigned int word;  /* (count << 2) | copy of the last publication; 0: none since clear() */
  unsigned int count, next;   /* the audio thread's: the last count it published, the copy it fills next */
  _Atomic unsigned long long copy[OMX_METER_RING_COPIES][OMX_METER_RING_MAX]; /* the values, as bits */
};

/** Before any other call, with no reader yet. */
static inline void omx_meter_ring_init(struct omx_meter_ring *r) {
  memset(r, 0, sizeof *r);
}

/** Nothing published: readers get no values until the next publish(). [main thread, audio thread stopped]
 * The count and the copy in turn go on, so a reader that straddles a clear() is still judged right. */
static inline void omx_meter_ring_clear(struct omx_meter_ring *r) {
  atomic_store_explicit(&r->word, 0u, memory_order_release);
}

/** One block's values, `n` of them (n <= OMX_METER_RING_MAX). [audio thread, one writer] */
static inline void omx_meter_ring_publish(struct omx_meter_ring *r, const double *values, unsigned int n) {
  const unsigned int c = r->next;
  /* the previous word stays ahead of this fill, for any reader that sees a value of it */
  atomic_thread_fence(memory_order_release);
  for (unsigned int i = 0; i < n; i++) {
    unsigned long long bits;
    memcpy(&bits, &values[i], sizeof bits);
    atomic_store_explicit(&r->copy[c][i], bits, memory_order_relaxed);
  }
  r->count = (r->count + 1u) & OMX_METER_RING_COUNT_MASK;
  if (r->count == 0u) r->count = 1u;
  atomic_store_explicit(&r->word, (r->count << 2) | c, memory_order_release);
  r->next = (c + 1u) % OMX_METER_RING_COPIES;
}

/** Values `first` .. `first + n - 1` of the last publication, all of the same block, into `out`;
 * returns n, or 0 when nothing was published since clear() or no whole copy could be read in
 * OMX_METER_RING_TRIES tries. [any thread, any number of readers] */
static inline unsigned int omx_meter_ring_read(struct omx_meter_ring *r, unsigned int first, double *out,
                                               unsigned int n) {
  for (unsigned int t = 0; t < OMX_METER_RING_TRIES; t++) {
    const unsigned int w = atomic_load_explicit(&r->word, memory_order_acquire);
    if (w == 0u) return 0;
    for (unsigned int i = 0; i < n; i++) {
      const unsigned long long bits = atomic_load_explicit(&r->copy[w & 3u][first + i], memory_order_relaxed);
      memcpy(&out[i], &bits, sizeof bits);
    }
    atomic_thread_fence(memory_order_acquire);
    const unsigned int again = atomic_load_explicit(&r->word, memory_order_relaxed);
    if (again != 0u && (((again >> 2) - (w >> 2)) & OMX_METER_RING_COUNT_MASK) <= 1u) return n;
  }
  return 0;
}

#endif /* OMX_METER_RING_H */
