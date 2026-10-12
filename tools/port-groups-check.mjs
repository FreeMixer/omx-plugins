#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * port-groups-check.mjs — the LV2 port groups of a BUILT bundle, as tools/port-groups-probe.c read
 * them through lilv, against the plugin's DECLARATION: every parameter a panel section lists is in
 * that section's group (its symbol the section's key, its label the section's label), and no other
 * port is in a group.
 *
 * Usage: node tools/port-groups-check.mjs plugins/<stem> <probe output>
 */
import { readFileSync } from 'node:fs';
import { loadDecl } from './gen.mjs';

const [dir, dump] = process.argv.slice(2);
const d = loadDecl(dir);
const served = new Map(readFileSync(dump, 'utf8').split('\n').filter((l) => l.startsWith('port\t')).map((l) => {
  const [, sym, group, label] = l.split('\t');
  return [sym, group === '-' ? null : { group, label }];
}));
const want = new Map([...served.keys()].map((sym) => [sym, null]));
for (const s of d.panel.sections) for (const sym of s.controls) want.set(sym, { group: s.key, label: s.label });

let bad = 0;
for (const [sym, w] of want) {
  const g = served.get(sym);
  const ok = served.has(sym) && JSON.stringify(g ?? null) === JSON.stringify(w);
  if (!ok) bad++;
  if (!ok || w) console.log(`${ok ? 'PASS' : 'FAIL'} port-groups ${sym}: ${w ? `in group ${w.group} (${w.label})` : 'in no group'}${ok ? '' : `, read ${served.has(sym) ? JSON.stringify(g) : 'no such port'}`}`);
}
const groups = d.panel.sections.length;
if (!bad) console.log(`PASS port-groups ${d.stem}: ${groups} group(s), every other port in none`);
process.exit(bad ? 1 : 0);
