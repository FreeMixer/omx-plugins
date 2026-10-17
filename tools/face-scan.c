// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * face-scan.c — what ANY built plugin states about itself, read as a host reads it and nothing more:
 * the scan a third-party plugin gets, run over our own faces. It knows no plugin, only CLAP and LV2.
 *
 *   face-scan clap <x.clap>
 *       `plugin <id> <name> <vendor> <version>`, `feature <name>` per CLAP feature,
 *       `param <id> <name> <module> <min> <max> <default> <flags>` per clap_plugin_params entry,
 *       `audio <in|out> <index> <id> <name> <channels> <flags>` per clap_plugin_audio_ports entry,
 *       `latency <0|1>` whether the plugin has the latency extension.
 *   face-scan lv2 <bundle> <uri>
 *       `plugin <uri> <name>`, then per port in index order
 *       `port <index> <audio|control|other> <in|out> <symbol> <name> <min> <max> <default> <unit>
 *        <properties> <group label> <designation>` (`-` where a port states none; properties a
 *       comma list of toggled, integer, enumeration, logarithmic, sidechain), and
 *       `point <index> <value> <label>` per scale point.
 *
 * TAB-separated, one record per line; tools/face-descriptor-check.mjs builds the descriptor the face
 * states from it and compares it with the authored one. Exit 1 when the plugin does not load.
 */
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <lilv/lilv.h>

#include "clap/entry.h"
#include "clap/ext/audio-ports.h"
#include "clap/ext/latency.h"
#include "clap/ext/params.h"
#include "clap/factory/plugin-factory.h"
#include "clap/host.h"

static const void *host_get_extension(const clap_host_t *h, const char *id) {
  (void)h, (void)id;
  return NULL;
}
static void host_noop(const clap_host_t *h) { (void)h; }

static const clap_host_t HOST = {
    CLAP_VERSION_INIT, NULL, "omx face-scan", "openmixer", "", "0.1",
    host_get_extension, host_noop, host_noop, host_noop,
};

/* A text field as one TAB-separated column: a TAB or a newline in it becomes a space, empty is `-`. */
static void field(const char *s) {
  if (!s || !*s) {
    fputs("\t-", stdout);
    return;
  }
  putchar('\t');
  for (; *s; s++) putchar(*s == '\t' || *s == '\n' ? ' ' : *s);
}

static int scan_clap(const char *path) {
  void *so = dlopen(path, RTLD_NOW | RTLD_LOCAL);
  if (!so) return fprintf(stderr, "dlopen %s: %s\n", path, dlerror()), 1;
  const clap_plugin_entry_t *entry = (const clap_plugin_entry_t *)dlsym(so, "clap_entry");
  if (!entry || !entry->init(path)) return fprintf(stderr, "no clap_entry\n"), 1;
  const clap_plugin_factory_t *fac = (const clap_plugin_factory_t *)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
  if (!fac || fac->get_plugin_count(fac) != 1) return fprintf(stderr, "factory\n"), 1;
  const clap_plugin_descriptor_t *d = fac->get_plugin_descriptor(fac, 0);
  fputs("plugin", stdout);
  field(d->id), field(d->name), field(d->vendor), field(d->version);
  putchar('\n');
  for (const char *const *f = d->features; f && *f; f++) fputs("feature", stdout), field(*f), putchar('\n');
  const clap_plugin_t *p = fac->create_plugin(fac, &HOST, d->id);
  if (!p || !p->init(p)) return fprintf(stderr, "create/init\n"), 1;
  const clap_plugin_params_t *params = (const clap_plugin_params_t *)p->get_extension(p, CLAP_EXT_PARAMS);
  for (uint32_t i = 0, n = params ? params->count(p) : 0; i < n; i++) {
    clap_param_info_t info;
    if (!params->get_info(p, i, &info)) return fprintf(stderr, "get_info %u\n", i), 1;
    printf("param\t%u", info.id);
    field(info.name), field(info.module);
    printf("\t%.17g\t%.17g\t%.17g\t%u\n", info.min_value, info.max_value, info.default_value, (unsigned)info.flags);
  }
  const clap_plugin_audio_ports_t *ports = (const clap_plugin_audio_ports_t *)p->get_extension(p, CLAP_EXT_AUDIO_PORTS);
  for (int in = 1; in >= 0; in--) {
    for (uint32_t i = 0, n = ports ? ports->count(p, in) : 0; i < n; i++) {
      clap_audio_port_info_t info;
      if (!ports->get(p, i, in, &info)) return fprintf(stderr, "audio port %u\n", i), 1;
      printf("audio\t%s\t%u\t%u", in ? "in" : "out", i, info.id);
      field(info.name);
      printf("\t%u\t%u\n", info.channel_count, (unsigned)info.flags);
    }
  }
  printf("latency\t%d\n", p->get_extension(p, CLAP_EXT_LATENCY) != NULL);
  p->destroy(p);
  entry->deinit();
  dlclose(so);
  return 0;
}

static int scan_lv2(const char *bundle, const char *uri) {
  char dir[4096];
  snprintf(dir, sizeof dir, "%s%s", bundle, bundle[0] && bundle[strlen(bundle) - 1] == '/' ? "" : "/");
  LilvWorld *w = lilv_world_new();
  LilvNode *b = lilv_new_file_uri(w, NULL, dir), *u = lilv_new_uri(w, uri);
  lilv_world_load_bundle(w, b);
  const LilvPlugin *pl = lilv_plugins_get_by_uri(lilv_world_get_all_plugins(w), u);
  if (!pl) return fprintf(stderr, "no plugin %s in %s\n", uri, bundle), 1;
#define NODE(var, s) LilvNode *var = lilv_new_uri(w, s)
  NODE(audio, LILV_URI_AUDIO_PORT);
  NODE(control, LILV_URI_CONTROL_PORT);
  NODE(input, LILV_URI_INPUT_PORT);
  NODE(unit, "http://lv2plug.in/ns/extensions/units#unit");
  NODE(group, "http://lv2plug.in/ns/ext/port-groups#group");
  NODE(label, "http://www.w3.org/2000/01/rdf-schema#label");
  NODE(designation, "http://lv2plug.in/ns/lv2core#designation");
  NODE(toggled, "http://lv2plug.in/ns/lv2core#toggled");
  NODE(integer, "http://lv2plug.in/ns/lv2core#integer");
  NODE(enumeration, "http://lv2plug.in/ns/lv2core#enumeration");
  NODE(logarithmic, "http://lv2plug.in/ns/ext/port-props#logarithmic");
  NODE(sidechain, "http://lv2plug.in/ns/lv2core#isSideChain");
  LilvNode *name = lilv_plugin_get_name(pl);
  fputs("plugin", stdout);
  field(uri), field(name ? lilv_node_as_string(name) : NULL);
  putchar('\n');
  lilv_node_free(name);
  const char *local = NULL;
  for (uint32_t i = 0, n = lilv_plugin_get_num_ports(pl); i < n; i++) {
    const LilvPort *port = lilv_plugin_get_port_by_index(pl, i);
    const char *cls = lilv_port_is_a(pl, port, audio) ? "audio" : lilv_port_is_a(pl, port, control) ? "control" : "other";
    printf("port\t%u\t%s\t%s", i, cls, lilv_port_is_a(pl, port, input) ? "in" : "out");
    field(lilv_node_as_string(lilv_port_get_symbol(pl, port)));
    LilvNode *pn = lilv_port_get_name(pl, port);
    field(pn ? lilv_node_as_string(pn) : NULL);
    lilv_node_free(pn);
    LilvNode *def = NULL, *min = NULL, *max = NULL;
    lilv_port_get_range(pl, port, &def, &min, &max);
    LilvNode *range[3] = {min, max, def};
    for (int k = 0; k < 3; k++) {
      if (range[k]) printf("\t%.17g", (double)lilv_node_as_float(range[k]));
      else fputs("\t-", stdout);
      lilv_node_free(range[k]);
    }
    LilvNodes *units = lilv_port_get_value(pl, port, unit);
    const char *us = units && lilv_nodes_size(units) ? lilv_node_as_uri(lilv_nodes_get_first(units)) : NULL;
    local = us ? strrchr(us, '#') : NULL;
    field(local ? local + 1 : us);
    lilv_nodes_free(units);
    char props[128] = "";
    const struct { LilvNode *node; const char *word; } P[] = {
        {toggled, "toggled"}, {integer, "integer"}, {enumeration, "enumeration"}, {logarithmic, "logarithmic"}, {sidechain, "sidechain"},
    };
    for (size_t k = 0; k < sizeof P / sizeof P[0]; k++) {
      if (!lilv_port_has_property(pl, port, P[k].node)) continue;
      if (*props) strncat(props, ",", sizeof props - strlen(props) - 1);
      strncat(props, P[k].word, sizeof props - strlen(props) - 1);
    }
    field(props);
    LilvNodes *groups = lilv_port_get_value(pl, port, group);
    const char *gl = NULL;
    LilvNodes *labels = NULL;
    if (groups && lilv_nodes_size(groups)) {
      labels = lilv_world_find_nodes(w, lilv_nodes_get_first(groups), label, NULL);
      if (labels && lilv_nodes_size(labels)) gl = lilv_node_as_string(lilv_nodes_get_first(labels));
    }
    field(gl);
    lilv_nodes_free(labels);
    lilv_nodes_free(groups);
    LilvNodes *des = lilv_port_get_value(pl, port, designation);
    const char *ds = des && lilv_nodes_size(des) ? lilv_node_as_uri(lilv_nodes_get_first(des)) : NULL;
    local = ds ? strrchr(ds, '#') : NULL;
    field(local ? local + 1 : ds);
    lilv_nodes_free(des);
    putchar('\n');
    LilvScalePoints *points = lilv_port_get_scale_points(pl, port);
    LILV_FOREACH(scale_points, it, points) {
      const LilvScalePoint *sp = lilv_scale_points_get(points, it);
      printf("point\t%u\t%.17g", i, (double)lilv_node_as_float(lilv_scale_point_get_value(sp)));
      field(lilv_node_as_string(lilv_scale_point_get_label(sp)));
      putchar('\n');
    }
    lilv_scale_points_free(points);
  }
  LilvNode *all[] = {audio, control, input, unit, group, label, designation, toggled, integer, enumeration, logarithmic, sidechain, b, u};
  for (size_t k = 0; k < sizeof all / sizeof all[0]; k++) lilv_node_free(all[k]);
  lilv_world_free(w);
  return 0;
}

int main(int argc, char **argv) {
  if (argc == 3 && strcmp(argv[1], "clap") == 0) return scan_clap(argv[2]);
  if (argc == 4 && strcmp(argv[1], "lv2") == 0) return scan_lv2(argv[2], argv[3]);
  fprintf(stderr, "usage: face-scan clap <x.clap> | face-scan lv2 <bundle> <uri>\n");
  return 2;
}
