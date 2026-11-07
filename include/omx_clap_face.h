// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * omx_clap_face.h — the CLAP face every plugin shares (lib/omx_clap_face.c), over one plugin's
 * struct omx_face. A plugin's omx_<kernel>_clap.c holds its `struct omx_clap_face` (OMX_CLAP_FACE) and
 * its entry, whose factory getter is omx_clap_face_factory over it:
 *
 *   static const struct omx_clap_face CLAP = OMX_CLAP_FACE(&FACE, FEATURES, OMX_<K>_CLAP_ID, OMX_<K>_NAME, ...);
 *   static const void *e_factory(const char *id) { return omx_clap_face_factory(&CLAP, id); }
 *   const clap_plugin_entry_t omx_clap_entry_<kernel> = {CLAP_VERSION_INIT, omx_clap_face_init, omx_clap_face_deinit, e_factory};
 */
#ifndef OMX_CLAP_FACE_H
#define OMX_CLAP_FACE_H

#include <stdbool.h>
#include <stdint.h>

#include "clap/entry.h"
#include "clap/factory/plugin-factory.h"
#include "clap/factory/preset-discovery.h"
#include "clap/plugin.h"
#include "clap/version.h"

#include "omx_plugin_face.h"
#include "omx_preset_file.h"

/** org.openmixer.meter-clip/1: the clip word beside clap.meters/1's values. clap.meters/1 has no word for it, so a
 * meter that declares `clip` serves it here: the longest run of samples over full scale, per channel, held since the
 * host's last get_values() of that meter and taken by it (docs/meters.md). [the thread that reads the meters] */
#define OMX_CLAP_EXT_METER_CLIP "org.openmixer.meter-clip/1"
typedef struct omx_clap_plugin_meter_clip {
  /** The clip words of `meter_id`'s channels the last get_values() of it took, up to `capacity`; returns how many
   * it wrote, 0 for an unknown id or a meter that declares no clip word. */
  uint32_t (*get)(const clap_plugin_t *plugin, clap_id meter_id, double *runs, uint32_t capacity);
} omx_clap_plugin_meter_clip_t;

/** One plugin's CLAP factories and descriptors; the factories' callbacks find the plugin from them. */
struct omx_clap_face {
  clap_plugin_factory_t factory;
  clap_preset_discovery_factory_t presets;
  clap_plugin_descriptor_t desc;
  clap_preset_discovery_provider_descriptor_t provider;
  const char *preset_location; /* OMX_PRESET_ROOT "/" <clap id> "/" */
  const struct omx_face *face;
};

uint32_t omx_clap_face_count(const clap_plugin_factory_t *f);
const clap_plugin_descriptor_t *omx_clap_face_desc(const clap_plugin_factory_t *f, uint32_t i);
const clap_plugin_t *omx_clap_face_create(const clap_plugin_factory_t *f, const clap_host_t *host, const char *id);
uint32_t omx_clap_face_presets_count(const clap_preset_discovery_factory_t *f);
const clap_preset_discovery_provider_descriptor_t *omx_clap_face_presets_desc(const clap_preset_discovery_factory_t *f,
                                                                             uint32_t i);
const clap_preset_discovery_provider_t *omx_clap_face_presets_create(const clap_preset_discovery_factory_t *f,
                                                                     const clap_preset_discovery_indexer_t *ix,
                                                                     const char *id);

/** The plugin's factories by CLAP id (clap_plugin_entry.get_factory over one plugin). */
const void *omx_clap_face_factory(const struct omx_clap_face *f, const char *id);
bool omx_clap_face_init(const char *path);
void omx_clap_face_deinit(void);

/* A plugin's struct omx_clap_face: its identity strings are its generated header's macros, so the
 * preset location and the provider's id and name are joined at compile time. */
#define OMX_CLAP_FACE(face_, features_, id_, name_, vendor_, url_, version_, description_)                        \
  {                                                                                                               \
    {omx_clap_face_count, omx_clap_face_desc, omx_clap_face_create},                                              \
        {omx_clap_face_presets_count, omx_clap_face_presets_desc, omx_clap_face_presets_create},                  \
        {CLAP_VERSION_INIT, id_, name_, vendor_, url_, "", "", version_, description_, features_},                \
        {CLAP_VERSION_INIT, id_ ".presets", name_ " factory presets", vendor_}, OMX_PRESET_ROOT "/" id_ "/", face_ \
  }

#endif /* OMX_CLAP_FACE_H */
