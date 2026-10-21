#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * face-descriptor-check.mjs — the scan of our own built face against the plugin's authored descriptor
 * (docs/declaration-as-descriptor.md, "The check of our own faces"). tools/face-scan.c reads the face as
 * a host scans anyone's plugin; this builds what the face states, in the descriptor's words, and holds
 * it to plugins/<stem>/<stem>.descriptor.json:
 *
 *   - clap: the plugin's id, name, vendor and version; one parameter per control in order, its clap_id
 *     the control's id, its name, range and default (as the float a face stores), stepped for a toggle,
 *     a choice or an integer travel, automatable as stated; the audio ports, each with its id, name,
 *     channel count and main flag; the latency extension when the descriptor reports latency. The
 *     host's bypass (CLAP_PARAM_IS_BYPASS) is the face's, beside the controls;
 *   - lv2: the plugin's URI and name; one control input per control in order, its symbol and name, its
 *     range and default, its unit where LV2 has one (none where the descriptor's has no LV2 unit),
 *     toggled for a toggle, integer for a step, an enumeration with the choice's values and labels,
 *     logarithmic for a log scale, its port group's label the control's group; the audio inputs and
 *     outputs (a stereo pair is two ports), the side-chain; the latency port when latency is reported.
 *     The `enabled` port (lv2:enabled) is the face's, beside the controls.
 *
 * Any difference fails with contract.descriptor.face-differs, naming the plugin, the face and the
 * control or port.
 *
 * Usage: node tools/face-descriptor-check.mjs plugins/<stem> clap|lv2 <face-scan output> [<descriptor>]
 * (the last argument reads another descriptor file in the folder's place: the sabotage arm's).
 */
import { readFileSync } from 'node:fs';
import { basename, join, resolve } from 'node:path';
import { descriptorFile } from './descriptor.mjs';
import { lv2UnitNames } from './gen.mjs';

export const FACE_DIFFERS = 'contract.descriptor.face-differs';
const IS_STEPPED = 1, IS_BYPASS = 1 << 4, IS_AUTOMATABLE = 1 << 5, PORT_IS_MAIN = 1;

/** A control's travel as a face stores it (a float): `[min, max, default]`. */
function rangeOf(c) {
  if (c.kind === 'toggle') return [0, 1, c.default ? 1 : 0];
  if (c.kind === 'choice') return [Math.min(...c.values), Math.max(...c.values), c.values[c.ids.indexOf(c.default)]];
  return [c.travel.min, c.travel.max, c.travel.default];
}
const f32 = (x) => Math.fround(Number(x));
const stepped = (c) => c.kind !== 'travel' || c.travel.step === 1;

/** The rows of a face-scan dump: `{ <record>: [fields...] [] }`, `-` read as null. */
export function rowsOf(text) {
  const out = {};
  for (const line of text.split('\n').filter(Boolean)) {
    const [kind, ...f] = line.split('\t');
    (out[kind] ??= []).push(f.map((x) => (x === '-' ? null : x)));
  }
  return out;
}

/** Every difference between what `face` states (`rows`) and the authored `desc`: `[{ where, what }]`. */
export function faceDifferences(desc, face, rows) {
  const out = [];
  const differ = (where, what, authored, stated) => {
    if (JSON.stringify(authored) !== JSON.stringify(stated)) out.push({ where, what: `${what}: authored ${JSON.stringify(authored)}, the face states ${JSON.stringify(stated)}` });
  };
  const controls = desc.controls;
  const audio = (dir) => desc.ports.filter((p) => p.kind === 'audio' && p.direction === dir);
  if (face === 'clap') {
    const [id, name, vendor, version] = rows.plugin?.[0] ?? [];
    differ('plugin', 'identity', [desc.plugin.id, desc.plugin.name, desc.plugin.vendor, desc.plugin.version], [id, name, vendor, version]);
    const features = (rows.feature ?? []).map(([f]) => f);
    for (const f of desc.plugin.features) if (!features.includes(f)) out.push({ where: 'plugin', what: `feature ${f}: authored, the face does not state it` });
    const params = (rows.param ?? []).filter((r) => !(Number(r[6]) & IS_BYPASS));
    differ('controls', 'count', controls.length, params.length);
    controls.forEach((c, i) => {
      const r = params[i];
      if (!r) return;
      const [pid, pname, , min, max, def, flags] = r;
      const where = `control ${c.symbol ?? c.name}`;
      differ(where, 'clap_id', c.id, Number(pid));
      differ(where, 'name', c.name, pname);
      differ(where, 'range and default', rangeOf(c).map(f32), [min, max, def].map(f32));
      differ(where, 'stepped', stepped(c), Boolean(Number(flags) & IS_STEPPED));
      differ(where, 'automatable', c.automatable ?? false, Boolean(Number(flags) & IS_AUTOMATABLE));
    });
    for (const dir of ['in', 'out']) {
      const stated = (rows.audio ?? []).filter((r) => r[0] === dir);
      const want = audio(dir);
      differ(`audio ${dir}`, 'port count', want.length, stated.length);
      want.forEach((p, i) => {
        const r = stated[i];
        if (!r) return;
        differ(`audio ${dir} port ${p.name}`, 'id, name, channels, main', [p.id, p.name, p.channels, p.role === 'main'], [Number(r[2]), r[3], Number(r[4]), Boolean(Number(r[5]) & PORT_IS_MAIN)]);
      });
    }
    differ('latency', 'reports', desc.latency?.reports ?? false, rows.latency?.[0]?.[0] === '1');
    return out;
  }
  // lv2
  const [uri, name] = rows.plugin?.[0] ?? [];
  differ('plugin', 'identity', [desc.plugin.uri, desc.plugin.name], [uri, name]);
  const ports = (rows.port ?? []).map(([index, cls, dir, symbol, pname, min, max, def, unit, props, group, designation]) => ({
    index, cls, dir, symbol, name: pname, min, max, def, unit, props: props ? props.split(',') : [], group, designation,
  }));
  // LV2 scale points are a set: read in value order, as a choice lists its values
  const points = (index) => (rows.point ?? []).filter((r) => r[0] === index).map(([, v, l]) => [Number(v), l]).sort((a, b) => a[0] - b[0]);
  const inputs = ports.filter((p) => p.cls === 'control' && p.dir === 'in' && p.designation !== 'enabled');
  differ('controls', 'count', controls.length, inputs.length);
  controls.forEach((c, i) => {
    const p = inputs[i];
    if (!p) return;
    const where = `control ${c.symbol ?? c.name}`;
    const has = (x) => p.props.includes(x);
    differ(where, 'symbol', c.symbol ?? c.id, p.symbol);
    differ(where, 'name', c.name, p.name);
    differ(where, 'range and default', rangeOf(c).map(f32), [p.min, p.max, p.def].map(f32));
    differ(where, 'unit', c.travel ? lv2UnitNames()[c.travel.unit] ?? null : null, p.unit);
    differ(where, 'toggled', c.kind === 'toggle', has('toggled'));
    differ(where, 'integer', c.kind === 'choice' || (c.kind === 'travel' && c.travel.step === 1), has('integer'));
    differ(where, 'enumeration', c.kind === 'choice' ? c.values.map((v, k) => [v, c.labels[k]]) : [], has('enumeration') ? points(p.index) : []);
    differ(where, 'logarithmic', c.travel?.scale === 'log', has('logarithmic'));
    differ(where, 'group', c.group?.[0] ?? null, p.group);
  });
  for (const dir of ['in', 'out']) {
    const stated = ports.filter((p) => p.cls === 'audio' && p.dir === dir);
    const want = audio(dir);
    differ(`audio ${dir}`, 'main channels', want.filter((p) => p.role === 'main').reduce((n, p) => n + p.channels, 0), stated.filter((p) => !p.props.includes('sidechain')).length);
    differ(`audio ${dir}`, 'side-chain channels', want.filter((p) => p.role === 'sidechain').reduce((n, p) => n + p.channels, 0), stated.filter((p) => p.props.includes('sidechain')).length);
  }
  differ('latency', 'reports', desc.latency?.reports ?? false, ports.some((p) => p.designation === 'latency'));
  return out;
}

function main([dir, face, dump, other]) {
  if (!dir || !['clap', 'lv2'].includes(face) || !dump) {
    console.error('usage: face-descriptor-check.mjs plugins/<stem> clap|lv2 <face-scan output> [<descriptor>]');
    process.exit(2);
  }
  const stem = basename(resolve(dir));
  const desc = JSON.parse(readFileSync(other ?? join(dir, descriptorFile(stem)), 'utf8'));
  const diffs = faceDifferences(desc, face, rowsOf(readFileSync(dump, 'utf8')));
  for (const d of diffs) console.log(`FAIL face-descriptor ${stem} ${face} ${d.where}: ${FACE_DIFFERS}: ${d.what}`);
  if (!diffs.length) console.log(`PASS face-descriptor ${stem} ${face}: the face states the authored descriptor (${desc.controls.length} controls, ${desc.ports.length} ports)`);
  process.exit(diffs.length ? 1 : 0);
}

if (process.argv[1] && resolve(process.argv[1]) === resolve(new URL(import.meta.url).pathname)) main(process.argv.slice(2));
