/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
 *
 * omx_test_transport_probe.h — the extension of omx test-transport-probe, a TEST plugin (role test:
 * linked into libomxplugins.a, created by id, never listed or installed): the transport its last
 * block was handed, whole, for a host's own tests.
 *
 *   const omx_test_transport_probe_t *x = plugin->get_extension(plugin, OMX_TEST_TRANSPORT_PROBE_ID);
 *   struct omx_test_transport_record rec;
 *   if (x && x->last(plugin, &rec)) ...   rec.transport is the block's clap_event_transport_t, byte for byte
 *
 * The audio thread writes the record at the end of each process() call under a sequence counter
 * (odd while it writes); last() copies it from any thread, retrying while a write is in progress, and
 * answers false when no block has run yet or no consistent copy was had in a bounded number of tries.
 */
#ifndef OMX_TEST_TRANSPORT_PROBE_H
#define OMX_TEST_TRANSPORT_PROBE_H

#include <stdbool.h>
#include <stdint.h>

#include <clap/events.h>
#include <clap/plugin.h>

#ifdef __cplusplus
extern "C" {
#endif

/** The extension's id. */
#define OMX_TEST_TRANSPORT_PROBE_ID "org.openmixer.test-transport-probe/1"

/** What the probe recorded of its last block. */
struct omx_test_transport_record {
  uint64_t blocks;                   /**< process() calls since activate(), this one included */
  uint64_t steady_time;              /**< the block's clap_process_t.steady_time */
  uint32_t frames;                   /**< the block's frames_count */
  uint32_t has_transport;            /**< 1 when the block was handed a transport, else 0 */
  clap_event_transport_t transport;  /**< the transport, byte for byte; all zero when none was handed */
};

/** The extension. */
typedef struct omx_test_transport_probe {
  /** Copy the last block's record into `out`; false when no block has run or no consistent copy was had. */
  bool (*last)(const clap_plugin_t *plugin, struct omx_test_transport_record *out);
} omx_test_transport_probe_t;

#ifdef __cplusplus
}
#endif

#endif /* OMX_TEST_TRANSPORT_PROBE_H */
