// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * port-groups-probe.c — the LV2 port groups of ANY built omx plugin, as a host reads them through
 * lilv: it knows no plugin, only LV2 and the port-groups extension.
 *
 *   port-groups-probe <bundle> <uri>
 *
 * One TAB-separated line per port, in port index order:
 * `port <symbol> <group symbol> <group label> <group name> <group display priority>`, the group fields `-`
 * for a port in no group (pg:group). tools/port-groups-check.mjs reads it back against the declaration's
 * port groups. Exit 1 when the plugin does not load, or a port names a group with no lv2:symbol,
 * rdfs:label, lv2:name or lv2:displayPriority (Zynthian reads the symbol, the name and the priority).
 */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <lilv/lilv.h>

int main(int argc, char **argv) {
  if (argc != 3) return fprintf(stderr, "usage: port-groups-probe <bundle> <uri>\n"), 2;
  char dir[4096];
  snprintf(dir, sizeof dir, "%s%s", argv[1], argv[1][0] && argv[1][strlen(argv[1]) - 1] == '/' ? "" : "/");
  LilvWorld *world = lilv_world_new();
  LilvNode *b = lilv_new_file_uri(world, NULL, dir), *u = lilv_new_uri(world, argv[2]);
  LilvNode *group = lilv_new_uri(world, "http://lv2plug.in/ns/ext/port-groups#group");
  LilvNode *symbol = lilv_new_uri(world, "http://lv2plug.in/ns/lv2core#symbol");
  LilvNode *label = lilv_new_uri(world, "http://www.w3.org/2000/01/rdf-schema#label");
  LilvNode *name = lilv_new_uri(world, "http://lv2plug.in/ns/lv2core#name");
  LilvNode *priority = lilv_new_uri(world, "http://lv2plug.in/ns/lv2core#displayPriority");
  lilv_world_load_bundle(world, b);
  const LilvPlugin *plugin = lilv_plugins_get_by_uri(lilv_world_get_all_plugins(world), u);
  int rc = 0;
  if (!plugin) {
    fprintf(stderr, "%s: no plugin %s\n", dir, argv[2]);
    rc = 1;
  }
  for (uint32_t i = 0; plugin && i < lilv_plugin_get_num_ports(plugin); ++i) {
    const LilvPort *port = lilv_plugin_get_port_by_index(plugin, i);
    const char *sym = lilv_node_as_string(lilv_port_get_symbol(plugin, port));
    LilvNodes *gs = lilv_port_get_value(plugin, port, group);
    if (!gs || lilv_nodes_size(gs) == 0) printf("port\t%s\t-\t-\t-\t-\n", sym);
    else if (lilv_nodes_size(gs) > 1) {
      fprintf(stderr, "%s: port %s is in %u groups\n", argv[2], sym, lilv_nodes_size(gs));
      rc = 1;
    } else {
      const LilvNode *g = lilv_nodes_get_first(gs);
      LilvNode *gsym = lilv_world_get(world, g, symbol, NULL), *glabel = lilv_world_get(world, g, label, NULL);
      LilvNode *gname = lilv_world_get(world, g, name, NULL), *gprio = lilv_world_get(world, g, priority, NULL);
      if (!gsym || !glabel || !gname || !gprio || !lilv_node_is_int(gprio)) {
        fprintf(stderr, "%s: port %s names group %s, which has no lv2:symbol, rdfs:label, lv2:name or integer lv2:displayPriority\n", argv[2], sym, lilv_node_as_uri(g));
        rc = 1;
      } else
        printf("port\t%s\t%s\t%s\t%s\t%d\n", sym, lilv_node_as_string(gsym), lilv_node_as_string(glabel), lilv_node_as_string(gname), lilv_node_as_int(gprio));
      lilv_node_free(gsym), lilv_node_free(glabel), lilv_node_free(gname), lilv_node_free(gprio);
    }
    lilv_nodes_free(gs);
  }
  lilv_node_free(priority), lilv_node_free(name), lilv_node_free(label), lilv_node_free(symbol), lilv_node_free(group), lilv_node_free(u), lilv_node_free(b);
  lilv_world_free(world);
  return rc;
}
