#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * port-groups-check.mjs — the LV2 port groups of a BUILT bundle, as tools/port-groups-probe.c read
 * them through lilv, against the plugin's DECLARATION (tools/descriptor.mjs portGroupsOf): every
 * parameter a panel section lists is in that section's group, or in the section's subgroup holding it
 * (an EQ's band, FreeMixer/omx-plugins#257), the group's symbol, label, name and display priority the
 * declaration's; no other port is in a group. It then lists the groups as a host showing the higher
 * priority first (Zynthian) orders them.
 *
 * Usage: node tools/port-groups-check.mjs plugins/<stem> <probe output>
 */
import { readFileSync } from 'node:fs';
import { loadDecl } from './gen.mjs';
import { portGroupsOf } from './descriptor.mjs';

const [dir, dump] = process.argv.slice(2);
const d = loadDecl(dir);
const served = new Map(readFileSync(dump, 'utf8').split('\n').filter((l) => l.startsWith('port\t')).map((l) => {
  const [, sym, group, label, name, priority] = l.split('\t');
  return [sym, group === '-' ? null : { group, label, name, priority }];
}));
const want = new Map([...served.keys()].map((sym) => [sym, null]));
const groups = portGroupsOf(d);
for (const g of groups) for (const sym of g.controls) want.set(sym, { group: g.key, label: g.label, name: g.label, priority: String(g.priority) });

let bad = 0;
for (const [sym, w] of want) {
  const g = served.get(sym);
  const ok = served.has(sym) && JSON.stringify(g ?? null) === JSON.stringify(w);
  if (!ok) bad++;
  if (!ok || w) console.log(`${ok ? 'PASS' : 'FAIL'} port-groups ${sym}: ${w ? `in group ${w.group} (${w.label}, priority ${w.priority})` : 'in no group'}${ok ? '' : `, read ${served.has(sym) ? JSON.stringify(g) : 'no such port'}`}`);
}
if (!bad) {
  const shown = [...groups].sort((a, b) => b.priority - a.priority).map((g) => `${g.key} "${g.label}" ${g.priority}`);
  console.log(`PASS port-groups ${d.stem}: ${groups.length} group(s), every other port in none; by priority: ${shown.join(', ')}`);
}
process.exit(bad ? 1 : 0);
