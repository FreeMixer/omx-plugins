// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * preset-probe.c — the factory presets of ANY built omx plugin, as a host finds and loads them
 * (docs/presets.md): it knows no plugin, only CLAP's preset discovery, preset-load and params, and
 * LV2's presets through lilv.
 *
 *   preset-probe clap <plugin.clap> <installed root> <tree root>
 *   preset-probe lv2 <bundle> <uri>
 *
 * clap: every provider of clap.preset-discovery-factory is created and initialised; each location
 * it declares is crawled as an indexer crawls it, for files of each declared extension, the
 * installed root (/usr/share/omx/presets) read from the tree root (the source's presets/) so the
 * probe needs no install; get_metadata of each file prints its presets. Each preset is then
 * loaded into a fresh instance through clap.preset-load and every parameter read back through
 * clap.params. TAB-separated lines:
 *
 *   filetype <extension>         soundpack <id>          location <flags> <declared location>
 *   preset <file> <plugin id> <flags> <name>
 *   loaded <file>                (the host's clap.preset-load `loaded`, once per load)
 *   rescan <file> <flags>        (the host's clap.params `rescan`, during that file's load)
 *   value <file> <param id> <value, %.9g>
 *
 * lv2: every pset:Preset lilv relates to the plugin, loaded and read as a state:
 *
 *   lv2-preset <preset uri> <label>
 *   lv2-value <preset uri> <symbol> <value, %.9g>
 *
 * tools/test/presets-host-check.mjs reads it back against the presets tools/gen-presets.mjs builds.
 * Exit 1 when the plugin does not load or a step it must offer is missing.
 */
#include <dirent.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clap/entry.h"
#include "clap/ext/params.h"
#include "clap/ext/preset-load.h"
#include "clap/factory/plugin-factory.h"
#include "clap/factory/preset-discovery.h"
#include "clap/host.h"

#include <lilv/lilv.h>
#include <lv2/urid/urid.h>

/* ---- clap -------------------------------------------------------------------------------- */

#define MAX_ITEMS 64
static char g_ext[MAX_ITEMS][32];
static int g_exts;
static char g_loc[MAX_ITEMS][1024];
static int g_locs;
static const char *g_file; /* the file being loaded or read, for the printed lines */

static void h_loaded(const clap_host_t *h, uint32_t kind, const char *location, const char *key) {
  (void)h, (void)kind, (void)location, (void)key;
  printf("loaded\t%s\n", g_file);
}
static void h_error(const clap_host_t *h, uint32_t kind, const char *location, const char *key, int32_t status, const char *msg) {
  (void)h, (void)kind, (void)key, (void)status;
  fprintf(stderr, "preset-load error %s: %s\n", location, msg ? msg : "");
}
static const clap_host_preset_load_t HOST_PRESET_LOAD = {h_error, h_loaded};
static void h_rescan(const clap_host_t *h, clap_param_rescan_flags flags) {
  (void)h;
  printf("rescan\t%s\t%u\n", g_file ? g_file : "-", (unsigned)flags);
}
static void h_clear(const clap_host_t *h, clap_id id, clap_param_clear_flags flags) { (void)h, (void)id, (void)flags; }
static void h_flush(const clap_host_t *h) { (void)h; }
static const clap_host_params_t HOST_PARAMS = {h_rescan, h_clear, h_flush};
static const void *h_ext(const clap_host_t *h, const char *id) {
  (void)h;
  if (strcmp(id, CLAP_EXT_PARAMS) == 0) return &HOST_PARAMS;
  return strcmp(id, CLAP_EXT_PRESET_LOAD) == 0 ? &HOST_PRESET_LOAD : NULL;
}
static void h_noop(const clap_host_t *h) { (void)h; }
static const clap_host_t HOST = {CLAP_VERSION_INIT, NULL, "preset-probe", "openmixer", "", "1", h_ext, h_noop, h_noop, h_noop};

static bool ix_filetype(const clap_preset_discovery_indexer_t *ix, const clap_preset_discovery_filetype_t *t) {
  (void)ix;
  if (g_exts < MAX_ITEMS) snprintf(g_ext[g_exts++], sizeof g_ext[0], "%s", t->file_extension ? t->file_extension : "");
  printf("filetype\t%s\n", t->file_extension ? t->file_extension : "");
  return true;
}
static bool ix_location(const clap_preset_discovery_indexer_t *ix, const clap_preset_discovery_location_t *l) {
  (void)ix;
  printf("location\t%u\t%s\n", l->flags, l->location ? l->location : "-");
  if (l->kind == CLAP_PRESET_DISCOVERY_LOCATION_FILE && l->location && g_locs < MAX_ITEMS)
    snprintf(g_loc[g_locs++], sizeof g_loc[0], "%s", l->location);
  return true;
}
static bool ix_soundpack(const clap_preset_discovery_indexer_t *ix, const clap_preset_discovery_soundpack_t *s) {
  (void)ix;
  printf("soundpack\t%s\n", s->id);
  return true;
}
static const void *ix_ext(const clap_preset_discovery_indexer_t *ix, const char *id) {
  (void)ix, (void)id;
  return NULL;
}
static const clap_preset_discovery_indexer_t INDEXER = {CLAP_VERSION_INIT, "preset-probe", "openmixer", "", "1", NULL,
                                                        ix_filetype, ix_location, ix_soundpack, ix_ext};

/* The metadata of one file: kept until the preset ends, then printed. */
static char m_name[256], m_plugin[256];
static uint32_t m_flags;
static int m_open;
static void m_flush(void) {
  if (m_open) printf("preset\t%s\t%s\t%u\t%s\n", g_file, m_plugin, m_flags, m_name);
  m_open = 0;
}
static void r_error(const clap_preset_discovery_metadata_receiver_t *r, int32_t s, const char *msg) {
  (void)r, (void)s;
  fprintf(stderr, "metadata error %s: %s\n", g_file, msg ? msg : "");
}
static bool r_begin(const clap_preset_discovery_metadata_receiver_t *r, const char *name, const char *key) {
  (void)r;
  m_flush();
  if (key) fprintf(stderr, "%s: a load key (%s) for a one-preset file\n", g_file, key);
  snprintf(m_name, sizeof m_name, "%s", name ? name : "");
  m_plugin[0] = 0;
  m_flags = 0;
  m_open = 1;
  return true;
}
static void r_plugin(const clap_preset_discovery_metadata_receiver_t *r, const clap_universal_plugin_id_t *id) {
  (void)r;
  snprintf(m_plugin, sizeof m_plugin, "%s:%s", id->abi, id->id);
}
static void r_pack(const clap_preset_discovery_metadata_receiver_t *r, const char *id) { (void)r, (void)id; }
static void r_flags(const clap_preset_discovery_metadata_receiver_t *r, uint32_t f) { (void)r, m_flags = f; }
static void r_str(const clap_preset_discovery_metadata_receiver_t *r, const char *s) { (void)r, (void)s; }
static void r_times(const clap_preset_discovery_metadata_receiver_t *r, clap_timestamp a, clap_timestamp b) { (void)r, (void)a, (void)b; }
static void r_extra(const clap_preset_discovery_metadata_receiver_t *r, const char *k, const char *v) { (void)r, (void)k, (void)v; }
static const clap_preset_discovery_metadata_receiver_t RECEIVER = {NULL, r_error, r_begin, r_plugin, r_pack, r_flags,
                                                                   r_str, r_str, r_times, r_str, r_extra};

static int by_name(const void *a, const void *b) { return strcmp(*(char *const *)a, *(char *const *)b); }

static int clap_mode(const char *path, const char *installed, const char *tree) {
  void *so = dlopen(path, RTLD_NOW | RTLD_LOCAL);
  if (!so) return fprintf(stderr, "dlopen %s: %s\n", path, dlerror()), 1;
  const clap_plugin_entry_t *entry = (const clap_plugin_entry_t *)dlsym(so, "clap_entry");
  if (!entry || !entry->init(path)) return fprintf(stderr, "%s: no clap_entry\n", path), 1;
  const clap_preset_discovery_factory_t *df = (const clap_preset_discovery_factory_t *)entry->get_factory(CLAP_PRESET_DISCOVERY_FACTORY_ID);
  const clap_plugin_factory_t *pf = (const clap_plugin_factory_t *)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
  if (!df) return fprintf(stderr, "%s: no %s\n", path, CLAP_PRESET_DISCOVERY_FACTORY_ID), 1;
  if (!pf || pf->get_plugin_count(pf) != 1) return fprintf(stderr, "%s: not one plugin\n", path), 1;
  const char *plugin_id = pf->get_plugin_descriptor(pf, 0)->id;
  char *files[512];
  int nfiles = 0, rc = 0;
  for (uint32_t k = 0; k < df->count(df); ++k) {
    const clap_preset_discovery_provider_t *pv = df->create(df, &INDEXER, df->get_descriptor(df, k)->id);
    if (!pv || !pv->init(pv)) return fprintf(stderr, "%s: provider %u does not start\n", path, k), 1;
    for (int l = 0; l < g_locs; ++l) {
      size_t n = strlen(installed);
      char dir[2048];
      if (strncmp(g_loc[l], installed, n) == 0) snprintf(dir, sizeof dir, "%s%s", tree, g_loc[l] + n);
      else snprintf(dir, sizeof dir, "%s", g_loc[l]);
      DIR *d = opendir(dir);
      if (!d) { fprintf(stderr, "%s: declared location %s (%s) does not open\n", path, g_loc[l], dir); rc = 1; continue; }
      int first = nfiles;
      for (struct dirent *e; (e = readdir(d)) && nfiles < 512;) {
        const char *dot = strrchr(e->d_name, '.');
        for (int t = 0; dot && e->d_name[0] != '.' && t < g_exts; ++t) {
          if (strcmp(dot + 1, g_ext[t]) != 0) continue;
          char *f = malloc(strlen(dir) + strlen(e->d_name) + 2);
          sprintf(f, "%s%s%s", dir, dir[strlen(dir) - 1] == '/' ? "" : "/", e->d_name);
          files[nfiles++] = f;
          break;
        }
      }
      closedir(d);
      qsort(files + first, (size_t)(nfiles - first), sizeof *files, by_name);
    }
    for (int i = 0; i < nfiles; ++i) {
      g_file = strrchr(files[i], '/') + 1;
      if (!pv->get_metadata(pv, CLAP_PRESET_DISCOVERY_LOCATION_FILE, files[i], &RECEIVER)) {
        fprintf(stderr, "%s: get_metadata fails\n", files[i]);
        rc = 1;
      }
      m_flush();
    }
    pv->destroy(pv);
  }
  for (int i = 0; i < nfiles; ++i) {
    g_file = strrchr(files[i], '/') + 1;
    const clap_plugin_t *p = pf->create_plugin(pf, &HOST, plugin_id);
    if (!p || !p->init(p)) return fprintf(stderr, "%s: the plugin does not start\n", path), 1;
    const clap_plugin_preset_load_t *pl = (const clap_plugin_preset_load_t *)p->get_extension(p, CLAP_EXT_PRESET_LOAD);
    const clap_plugin_params_t *pp = (const clap_plugin_params_t *)p->get_extension(p, CLAP_EXT_PARAMS);
    if (!pl || !pp) return fprintf(stderr, "%s: no %s or params\n", path, CLAP_EXT_PRESET_LOAD), 1;
    if (!pl->from_location(p, CLAP_PRESET_DISCOVERY_LOCATION_FILE, files[i], NULL)) {
      fprintf(stderr, "%s: from_location fails\n", files[i]);
      rc = 1;
    }
    for (uint32_t k = 0; k < pp->count(p); ++k) {
      clap_param_info_t info;
      double v;
      if (pp->get_info(p, k, &info) && pp->get_value(p, info.id, &v)) printf("value\t%s\t%u\t%.9g\n", g_file, info.id, v);
    }
    p->destroy(p);
    free(files[i]);
  }
  entry->deinit();
  return rc;
}

/* ---- lv2 --------------------------------------------------------------------------------- */

static char *g_uris[256];
static LV2_URID u_map(LV2_URID_Map_Handle h, const char *uri) {
  (void)h;
  for (LV2_URID i = 0; i < 256; ++i) {
    if (!g_uris[i]) return (g_uris[i] = strdup(uri)), i + 1;
    if (strcmp(g_uris[i], uri) == 0) return i + 1;
  }
  return 0;
}
static const char *u_unmap(LV2_URID_Unmap_Handle h, LV2_URID u) {
  (void)h;
  return u && u <= 256 ? g_uris[u - 1] : NULL;
}

static const char *g_preset;
static void lv2_value(const char *symbol, void *data, const void *value, uint32_t size, uint32_t type) {
  (void)data;
  const char *t = u_unmap(NULL, type);
  if (t && strcmp(t, "http://lv2plug.in/ns/ext/atom#Float") == 0 && size == sizeof(float)) printf("lv2-value\t%s\t%s\t%.9g\n", g_preset, symbol, (double)*(const float *)value);
  else if (t && strcmp(t, "http://lv2plug.in/ns/ext/atom#Double") == 0 && size == sizeof(double)) printf("lv2-value\t%s\t%s\t%.9g\n", g_preset, symbol, *(const double *)value);
  else if (t && strcmp(t, "http://lv2plug.in/ns/ext/atom#Int") == 0 && size == sizeof(int32_t)) printf("lv2-value\t%s\t%s\t%d\n", g_preset, symbol, *(const int32_t *)value);
  else fprintf(stderr, "%s: port %s has a value of type %s\n", g_preset, symbol, t ? t : "?");
}

static int lv2_mode(const char *bundle, const char *uri) {
  char dir[4096];
  snprintf(dir, sizeof dir, "%s%s", bundle, bundle[0] && bundle[strlen(bundle) - 1] == '/' ? "" : "/");
  LilvWorld *world = lilv_world_new();
  LilvNode *b = lilv_new_file_uri(world, NULL, dir), *u = lilv_new_uri(world, uri);
  LilvNode *preset = lilv_new_uri(world, "http://lv2plug.in/ns/ext/presets#Preset");
  lilv_world_load_bundle(world, b);
  const LilvPlugin *plugin = lilv_plugins_get_by_uri(lilv_world_get_all_plugins(world), u);
  if (!plugin) return fprintf(stderr, "%s: no plugin %s\n", dir, uri), 1;
  LV2_URID_Map map = {NULL, u_map};
  LV2_URID_Unmap unmap = {NULL, u_unmap};
  LilvNodes *presets = lilv_plugin_get_related(plugin, preset);
  int rc = 0;
  LILV_FOREACH (nodes, i, presets) {
    const LilvNode *p = lilv_nodes_get(presets, i);
    g_preset = lilv_node_as_uri(p);
    lilv_world_load_resource(world, p);
    LilvState *s = lilv_state_new_from_world(world, &map, p);
    if (!s) { fprintf(stderr, "%s: no state\n", g_preset); rc = 1; continue; }
    printf("lv2-preset\t%s\t%s\n", g_preset, lilv_state_get_label(s) ? lilv_state_get_label(s) : "");
    lilv_state_emit_port_values(s, lv2_value, NULL);
    lilv_state_free(s);
  }
  lilv_nodes_free(presets);
  (void)unmap;
  lilv_node_free(preset), lilv_node_free(u), lilv_node_free(b);
  lilv_world_free(world);
  return rc;
}

int main(int argc, char **argv) {
  if (argc == 5 && strcmp(argv[1], "clap") == 0) return clap_mode(argv[2], argv[3], argv[4]);
  if (argc == 4 && strcmp(argv[1], "lv2") == 0) return lv2_mode(argv[2], argv[3]);
  fprintf(stderr, "usage: preset-probe clap <plugin.clap> <installed root> <tree root>\n"
                  "       preset-probe lv2 <bundle> <uri>\n");
  return 2;
}
