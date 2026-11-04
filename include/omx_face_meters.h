// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * omx_face_meters.h — one block's meter values, the same for every face (lib/omx_clap_face.c publishes
 * them through clap.meters/1, lib/omx_lv2_face.c writes them to its output control ports): a level is
 * the block's peak of each leg at the inputs (before the kernel runs) or the outputs (after), a reading
 * the kernel's own after the block (docs/meters.md). Arithmetic on the caller's array: no allocation,
 * no lock, no call into the system.
 */
#ifndef OMX_FACE_METERS_H
#define OMX_FACE_METERS_H

#include <stdint.h>

#include "omx_plugin_face.h"
#include <omxdsp/mix/omx_meter.h> /* omx_meter_level_db: a level meter's reading */

/** The meters of `source` (OMX_FACE_METER_IN or _OUT): each leg's level of `l` and `r` into `mv`. */
static inline void omx_face_meter_levels(const struct omx_face *f, uint32_t source, const float *l, const float *r,
                                         uint32_t frames, double *mv) {
  for (uint32_t m = 0; m < f->meter_count; ++m) {
    const struct omx_face_meter *row = &f->meters[m];
    if (row->source != source) continue;
    mv[row->at] = omx_meter_level_db(l, frames, row->min);
    mv[row->at + 1] = omx_meter_level_db(r, frames, row->min);
  }
}

/** The kernel's readings of the block just run into `mv`; a bypassed kernel's state is stale, and reads 0
 * (a level the kernel reads, its display minimum). */
static inline void omx_face_meter_readings(const struct omx_face *f, const void *core, int bypassed, double *mv) {
  for (uint32_t m = 0; m < f->meter_count; ++m) {
    const struct omx_face_meter *row = &f->meters[m];
    if (row->source == OMX_FACE_METER_READ)
      mv[row->at] = bypassed ? (row->kind == OMX_FACE_METER_KIND_GAIN_REDUCTION ? 0.0 : row->min)
                             : (double)f->core->meter(core, m);
  }
}

#endif /* OMX_FACE_METERS_H */
