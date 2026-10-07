#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * port-hints.mjs — no LV2 face loses a port hint (`make hints`, run by `make test`).
 *
 *   node tools/port-hints.mjs            check every plugin's generated TTL
 *   node tools/port-hints.mjs --write    pin the hints the TTLs carry now (commit the result)
 *
 * A hint is what a host draws a control from: a port property (logarithmic, integer, enumeration,
 * toggled, ...), a unit, a labelled scale point. Two checks, each naming the plugin, the port and
 * the hint:
 *
 *   - the PIN, tools/test/port-hints.json: every hint pinned for a port is still on it. A face
 *     may gain hints; losing one is a capability regression, so it is red until the pin is
 *     rewritten on purpose. A plugin with no pin is red too: a new face pins its hints.
 *   - the LAW, from the declaration: a frequency is logarithmic in hertz unless declared linear,
 *     every unit the LV2 units extension names is on its port, and a parameter with labelled
 *     values or points is an enumeration that lists them all.
 */
import { readdirSync, readFileSync, writeFileSync, existsSync } from 'node:fs';
import { join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { choicesOf, loadDecl, scaleOf } from './gen.mjs';

const ROOT = resolve(fileURLToPath(import.meta.url), '..', '..');
const UNITS = { Hz: 'hz', dB: 'db', ms: 'ms', s: 's', '%': 'pc', oct: 'oct' };

/** The hints of each port of a generated TTL, by symbol: `props`, `unit`, `points` ["v label"]. */
export function ttlHints(text) {
  const ports = {};
  // a port opens with its class (`[ a lv2:...`), a scale point with its label
  for (const block of text.split(/\]\s*,\s*\[(?=\s*a\s)/)) {
    const sym = /lv2:symbol\s+"([^"]+)"/.exec(block);
    if (!sym) continue;
    const props = [];
    for (const m of block.matchAll(/lv2:portProperty\s+([^;\]]+)/g)) props.push(...m[1].split(',').map((s) => s.trim()).filter(Boolean));
    const unit = /units:unit\s+(units:\w+)/.exec(block)?.[1] ?? null;
    const points = [...block.matchAll(/\[\s*rdfs:label\s+"((?:[^"\\]|\\.)*)"\s*;\s*rdf:value\s+(-?[\d.]+)\s*\]/g)].map((m) => `${Number(m[2])} ${m[1]}`);
    ports[sym[1]] = { props: [...new Set(props)].sort(), unit, points };
  }
  return ports;
}

export function pluginStems(root = ROOT) {
  return readdirSync(join(root, 'plugins'), { withFileTypes: true })
    .filter((e) => e.isDirectory() && existsSync(join(root, 'plugins', e.name, `${e.name}.decl.json`)))
    .map((e) => e.name)
    .sort();
}

const ttlOf = (root, stem) => readFileSync(join(root, 'plugins', stem, 'generated', `${stem}.lv2`, `${stem}.ttl`), 'utf8');
const pinPath = (root) => join(root, 'tools', 'test', 'port-hints.json');

/** Every lost or missing hint, one line each; empty when every face keeps them all. */
export function hintErrors(root = ROOT) {
  const errors = [];
  const pin = existsSync(pinPath(root)) ? JSON.parse(readFileSync(pinPath(root), 'utf8')) : {};
  for (const stem of pluginStems(root)) {
    const have = ttlHints(ttlOf(root, stem));
    if (!pin[stem]) errors.push(`${stem}: no pinned hints (node tools/port-hints.mjs --write, then commit tools/test/port-hints.json)`);
    for (const [sym, want] of Object.entries(pin[stem] ?? {})) {
      const got = have[sym];
      if (!got) {
        errors.push(`${stem}: port "${sym}" is gone`);
        continue;
      }
      for (const p of want.props) if (!got.props.includes(p)) errors.push(`${stem}: port "${sym}" lost ${p}`);
      if (want.unit && got.unit !== want.unit) errors.push(`${stem}: port "${sym}" lost units:unit ${want.unit}`);
      for (const p of want.points) if (!got.points.includes(p)) errors.push(`${stem}: port "${sym}" lost the scale point ${p}`);
    }
    const d = loadDecl(join(root, 'plugins', stem));
    for (const q of d.params) {
      const got = have[q.symbol];
      if (!got) {
        errors.push(`${stem}: declared parameter "${q.symbol}" has no port`);
        continue;
      }
      if (scaleOf(q) === 'log' && !got.props.includes('pprops:logarithmic')) errors.push(`${stem}: port "${q.symbol}" is not logarithmic`);
      if (UNITS[q.unit] && got.unit !== `units:${UNITS[q.unit]}`) errors.push(`${stem}: port "${q.symbol}" has no units:${UNITS[q.unit]}`);
      const choices = choicesOf(q);
      if (choices) {
        if (!got.props.includes('lv2:enumeration')) errors.push(`${stem}: port "${q.symbol}" is not an enumeration`);
        for (const c of choices) if (!got.points.includes(`${c.value} ${c.label}`)) errors.push(`${stem}: port "${q.symbol}" lists no "${c.label}" (${c.value})`);
      }
    }
  }
  return errors;
}

/** The pin: every plugin's hints as its TTL carries them now. */
export function writePin(root = ROOT) {
  const pin = {};
  for (const stem of pluginStems(root)) {
    pin[stem] = {};
    for (const [sym, h] of Object.entries(ttlHints(ttlOf(root, stem)))) if (h.props.length || h.unit || h.points.length) pin[stem][sym] = h;
  }
  writeFileSync(pinPath(root), `${JSON.stringify(pin, null, 2)}\n`);
}

if (process.argv[1] && resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  if (process.argv.includes('--write')) {
    writePin();
    console.log('port-hints: wrote tools/test/port-hints.json');
  } else {
    const errors = hintErrors();
    for (const e of errors) console.log(`FAIL ${e}`);
    console.log(errors.length ? `port-hints: ${errors.length} hint(s) lost` : `PASS port-hints: every face keeps its hints (${pluginStems().length} plugins)`);
    process.exit(errors.length ? 1 : 0);
  }
}
