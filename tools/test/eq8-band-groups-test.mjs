#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * eq8-band-groups-test.mjs — omx-eq8's GENERATED descriptor and LV2 TTL, read as committed, hold one port group
 * per band (FreeMixer/omx-plugins#257: a touchscreen cuts a group into screens of four, and one group of forty
 * controls was ten screens straddling the bands): `Band 1` ... `Band 8`, each that band's five controls (type,
 * frequency, gain, Q, on), in band order, with lv2:symbol `band<n>`, lv2:name and a display priority counting down
 * in port order; HPF and LPF keep their groups of three; no group named Bands is left in the TTL, while the panel
 * section stays Bands (the descriptor's group path is [section, group]).
 *
 * Both forms of the word are refused in a declaration without variants (the section's `"subgroups": "band"`, and the
 * top-level `subgroups` tools/variants.mjs writes), each by an arm on a scratch copy of omx-reverb.
 *
 * `make band-groups`, part of `make test`. Its eq8 checks read files only: a stale or hand-edited file is gen.mjs --check's.
 */
import { cpSync, mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { loadDecl } from '../gen.mjs';

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..', '..');
const DIR = join(ROOT, 'plugins', 'omx-eq8');
const descriptor = JSON.parse(readFileSync(join(DIR, 'omx-eq8.descriptor.json'), 'utf8'));
const ttl = readFileSync(join(DIR, 'generated', 'omx-eq8.lv2', 'omx-eq8.ttl'), 'utf8');

const BANDS = 8;
const PER_BAND = ['type', 'freq', 'gain', 'q', 'on'];
let bad = 0, n = 0;
const check = (ok, what, detail = '') => {
  n++;
  if (!ok) bad++;
  console.log(`${ok ? 'PASS' : 'FAIL'} eq8-band-groups ${what}${ok ? '' : `: ${detail}`}`);
};
const same = (a, b) => JSON.stringify(a) === JSON.stringify(b);

// the descriptor: each control's group path, the groups in the order their first control comes
const byGroup = new Map();
for (const c of descriptor.controls) {
  const key = JSON.stringify(c.group ?? null);
  if (!byGroup.has(key)) byGroup.set(key, []);
  byGroup.get(key).push(c.symbol);
}
const wantDescriptor = [
  [['HPF'], ['hpf_on', 'hpf_freq', 'hpf_slope']],
  [['LPF'], ['lpf_on', 'lpf_freq', 'lpf_slope']],
  ...Array.from({ length: BANDS }, (_x, b) => [['Bands', `Band ${b + 1}`], PER_BAND.map((s) => `b${b + 1}_${s}`)]),
];
const gotDescriptor = [...byGroup].map(([k, syms]) => [JSON.parse(k), syms]);
check(gotDescriptor.filter(([p]) => p?.[0] === 'Bands').length === BANDS, `descriptor: ${BANDS} band groups under the Bands section`,
  `read ${gotDescriptor.filter(([p]) => p?.[0] === 'Bands').map(([p, s]) => `${p.join('/')} (${s.length})`).join(', ')}`);
check(same(gotDescriptor, wantDescriptor), 'descriptor: HPF, LPF, then Band 1 ... Band 8, each its five controls in band order',
  JSON.stringify(gotDescriptor).slice(0, 400));

// the TTL: each control port's pg:group, and each group node's symbol, name, label and priority
const portGroup = new Map();
let port;
for (const line of ttl.split('\n')) {
  // a port's own lines are indented eight spaces (a scale point's, deeper, carry no symbol or group)
  const sym = /^ {8}lv2:symbol "([A-Za-z0-9_]+)"/.exec(line), group = /^ {8}pg:group <urn:openmixer:eq8#([A-Za-z0-9_]+)>/.exec(line);
  if (sym) port = sym[1];
  if (group) portGroup.set(port, group[1]);
}
const nodes = new Map();
for (const m of ttl.matchAll(/<urn:openmixer:eq8#([A-Za-z0-9_]+)>\s+a pg:Group ;\s+lv2:symbol "([^"]*)" ;\s+lv2:name "([^"]*)" ;\s+lv2:displayPriority (\d+) ;\s+rdfs:label "([^"]*)" \./g)) {
  nodes.set(m[1], { symbol: m[2], name: m[3], priority: Number(m[4]), label: m[5] });
}
const groupNodes = (ttl.match(/a pg:Group/g) ?? []).length;
check(groupNodes === nodes.size, 'TTL: every pg:Group node states lv2:symbol, lv2:name, lv2:displayPriority and rdfs:label', `${groupNodes} nodes, ${nodes.size} complete`);
const wantTtl = [['hpf', 'HPF', ['hpf_on', 'hpf_freq', 'hpf_slope']], ['lpf', 'LPF', ['lpf_on', 'lpf_freq', 'lpf_slope']],
  ...Array.from({ length: BANDS }, (_x, b) => [`band${b + 1}`, `Band ${b + 1}`, PER_BAND.map((s) => `b${b + 1}_${s}`)])];
check(nodes.size === wantTtl.length && !nodes.has('bands'), `TTL: ${wantTtl.length} groups (HPF, LPF and one per band), none named Bands`, [...nodes.keys()].join(', '));
wantTtl.forEach(([key, label, syms], k) => {
  const node = nodes.get(key);
  const members = [...portGroup].filter(([, g]) => g === key).map(([s]) => s);
  check(!!node && node.symbol === key && node.name === label && node.label === label && node.priority === wantTtl.length - k && same(members, syms),
    `TTL: ${label} is ${key}, priority ${wantTtl.length - k}, holding ${syms.join(' ')}`,
    `read ${JSON.stringify(node)} holding ${members.join(' ') || 'nothing'}`);
});
check(portGroup.size === wantTtl.reduce((a, [, , s]) => a + s.length, 0), 'TTL: every control port is in exactly the groups above, no other port grouped', `${portGroup.size} grouped ports`);

// a band is a variant's word: both its forms are refused in a declaration without variants (a scratch copy of
// omx-reverb under the temporary directory), and the variant loads
const TMP = mkdtempSync(join(tmpdir(), 'eq8-band-groups-'));
const scratch = join(TMP, 'plugins', 'omx-reverb');
try {
  check(loadDecl(DIR).subgroups?.bands?.length === BANDS, `a variant: loadDecl accepts omx-eq8 with its ${BANDS} band subgroups`);
  cpSync(join(ROOT, 'plugins', 'omx-reverb'), scratch, { recursive: true, filter: (src) => !src.includes('/build') });
  const file = join(scratch, 'omx-reverb.decl.json');
  const raw = readFileSync(file, 'utf8');
  check((() => { loadDecl(scratch); return true; })(), 'the scratch omx-reverb loads unchanged (the arms below change one thing)');
  const arms = [
    ['the section form ("subgroups": "band" on a section)', (d) => { d.panel.sections[0].subgroups = 'band'; }, /section 'reverb' says "subgroups"/],
    ['the expanded form (a top-level "subgroups")', (d) => { d.subgroups = { [d.panel.sections[0].key]: [{ key: 'band1', label: 'Band 1', controls: [d.panel.sections[0].controls[0]] }] }; }, /"subgroups" is what tools\/variants\.mjs writes for a variant/],
  ];
  for (const [what, change, want] of arms) {
    const d = JSON.parse(raw);
    change(d);
    writeFileSync(file, JSON.stringify(d, null, 2));
    let err;
    try {
      loadDecl(scratch);
    } catch (e) {
      err = e;
    }
    check(err !== undefined && want.test(err.message), `refused without variants: ${what}`, err ? err.message.slice(0, 300) : 'loadDecl accepted it');
  }
} finally {
  rmSync(TMP, { recursive: true, force: true });
}

console.log(bad ? `eq8-band-groups: ${bad} of ${n} FAILED` : `eq8-band-groups: all ${n} pass`);
process.exit(bad ? 1 : 0);
