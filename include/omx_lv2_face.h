// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * omx_lv2_face.h — the LV2 face every plugin shares (lib/omx_lv2_face.c), over one plugin's struct
 * omx_face. A plugin's omx_<kernel>_lv2.c holds its `struct omx_lv2_face` (OMX_LV2_FACE) and its
 * lv2_descriptor(), which answers &<it>.lv2.
 */
#ifndef OMX_LV2_FACE_H
#define OMX_LV2_FACE_H

#include <stdbool.h>
#include <stdint.h>

#include <lv2/core/lv2.h>

#include "omx_plugin_face.h"

/** One plugin's LV2 descriptor, the descriptor first: instantiate() finds the plugin from it. */
struct omx_lv2_face {
  LV2_Descriptor lv2;
  const struct omx_face *face;
};

LV2_Handle omx_lv2_face_instantiate(const LV2_Descriptor *d, double rate, const char *bundle,
                                    const LV2_Feature *const *features);
void omx_lv2_face_connect_port(LV2_Handle h, uint32_t port, void *data);
void omx_lv2_face_activate(LV2_Handle h);
void omx_lv2_face_run(LV2_Handle h, uint32_t frames);
void omx_lv2_face_cleanup(LV2_Handle h);
const void *omx_lv2_face_extension_data(const char *uri);

#define OMX_LV2_FACE(face_, uri_)                                                                               \
  {                                                                                                             \
    {uri_, omx_lv2_face_instantiate, omx_lv2_face_connect_port, omx_lv2_face_activate, omx_lv2_face_run, NULL, \
     omx_lv2_face_cleanup, omx_lv2_face_extension_data},                                                        \
        face_                                                                                                   \
  }

/* The port indices generated/omx_<kernel>_params.h names for every plugin (a keyed plugin's key its own). */
#define OMX_FACE_LV2_PORTS(K, key_)                                                                              \
  .lv2 = {OMX_##K##_LV2_PORT_IN_L, OMX_##K##_LV2_PORT_IN_R, key_, OMX_##K##_LV2_PORT_OUT_L, OMX_##K##_LV2_PORT_OUT_R, \
          OMX_##K##_LV2_PORT_ENABLED, OMX_##K##_LV2_PORT_LATENCY, OMX_##K##_LV2_PORT_FIRST_PARAM}

#endif /* OMX_LV2_FACE_H */
