// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * preset-lib.mjs — what a preset file is checked against: each shipped plugin's parameters as its
 * generated header declares them (symbol, unit, travel, integer steps) and the labels of its choices,
 * read from the generated LV2 description. Shared by tools/gen-presets.mjs (which writes the files)
 * and tools/test/presets-check.mjs (which holds them to the travels).
 */
import { existsSync, readdirSync, readFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

export const ROOT = join(dirname(fileURLToPath(import.meta.url)), '..');
export const FORMAT = 'org.openmixer.preset/1';
export const PRESET_DIR = 'presets';

/** The shipped plugins that have a parameter header: `{ stem, id, params, labels }`. */
export function plugins(root = ROOT) {
  const out = [];
  for (const stem of readdirSync(join(root, 'plugins')).sort()) {
    const gen = join(root, 'plugins', stem, 'generated');
    if (!existsSync(gen)) continue;
    const header = readdirSync(gen).find((f) => f.endsWith('_params.h'));
    if (!header) continue;
    out.push({ stem, id: `org.openmixer.${stem.replace(/^omx-/, '')}`, params: paramsOf(readFileSync(join(gen, header), 'utf8'), header), labels: labelsOf(join(gen, `${stem}.lv2`, `${stem}.ttl`)) });
  }
  return out;
}

/** The rows of the OMX_<X>_PARAMS table: `{ symbol, name, unit, min, max, def, integer, toggle }`. */
function paramsOf(text, file) {
  const table = text.match(/OMX_\w+_PARAMS\[[^\]]*\]\s*=\s*\{([\s\S]*?)\n\};/);
  if (!table) throw new Error(`${file}: no parameter table`);
  const rows = [...table[1].matchAll(/\{\s*"([^"]+)",\s*"([^"]*)",\s*"([^"]*)",\s*(-?[\d.]+)f,\s*(-?[\d.]+)f,\s*(-?[\d.]+)f,\s*([^}]+?)\s*\}/g)];
  return rows.map((m) => ({ symbol: m[1], name: m[2], unit: m[3], min: Number(m[4]), max: Number(m[5]), def: Number(m[6]), integer: /INTEGER|TOGGLE/.test(m[7]), toggle: /TOGGLE/.test(m[7]) }));
}

/** symbol -> labels by value, from the LV2 description's scale points. */
function labelsOf(ttl) {
  const text = existsSync(ttl) ? readFileSync(ttl, 'utf8') : '';
  const out = new Map();
  for (const part of text.split(/lv2:symbol "/).slice(1)) {
    const symbol = part.slice(0, part.indexOf('"'));
    const points = [...part.matchAll(/\[\s*rdfs:label "([^"]*)"\s*;\s*rdf:value (-?\d+)\s*\]/g)];
    if (points.length) out.set(symbol, new Map(points.map((m) => [Number(m[2]), m[1]])));
  }
  return out;
}

/** Every preset file: `{ file, dir, preset }`. */
export function presetFiles(root = ROOT) {
  const base = join(root, PRESET_DIR);
  if (!existsSync(base)) return [];
  return readdirSync(base).sort().flatMap((dir) => readdirSync(join(base, dir)).filter((f) => f.endsWith('.json')).sort().map((f) => ({ file: `${PRESET_DIR}/${dir}/${f}`, dir, preset: JSON.parse(readFileSync(join(base, dir, f), 'utf8')) })));
}

/** The problems with one preset against its plugin; an empty list is a clean file. */
export function problems(file, dir, preset, plugin) {
  const bad = [];
  const at = (m) => bad.push(`${file}: ${m}`);
  if (preset.format !== FORMAT) at(`format is ${JSON.stringify(preset.format)}, expected ${FORMAT}`);
  if (!plugin) return [...bad, `${file}: no shipped plugin has id ${dir}`];
  if (preset.plugin !== plugin.id) at(`plugin is ${preset.plugin}, the directory is ${plugin.id}`);
  if (!/^[a-z0-9]+(-[a-z0-9]+)*$/.test(preset.id ?? '')) at(`id ${JSON.stringify(preset.id)} is not a lower-case dash id`);
  if (file.split('/').pop() !== `${preset.id}.json`) at(`the file name is not ${preset.id}.json`);
  if (typeof preset.name !== 'string' || !preset.name) at('no name');
  if (preset.carries !== 'params') at(`carries is ${JSON.stringify(preset.carries)}, a factory preset carries params`);
  if (!('copyOf' in preset)) at('no copyOf');
  for (const k of ['created', 'modified']) if (Number.isNaN(Date.parse(preset[k]))) at(`${k} is not a date`);
  if (!Array.isArray(preset.params) || preset.params.length === 0) return [...bad, `${file}: no params`];
  const seen = new Set();
  for (const p of preset.params) {
    const decl = plugin.params.find((d) => d.symbol === p.symbol);
    if (!decl) { at(`'${p.symbol}' is not a parameter of ${plugin.id}`); continue; }
    if (seen.has(p.symbol)) at(`'${p.symbol}' is set twice`);
    seen.add(p.symbol);
    if (p.unit !== decl.unit) at(`'${p.symbol}' unit is ${JSON.stringify(p.unit)}, declared ${JSON.stringify(decl.unit)}`);
    if (typeof p.value !== 'number' || !Number.isFinite(p.value)) { at(`'${p.symbol}' value is not a number`); continue; }
    if (p.value < decl.min || p.value > decl.max) at(`'${p.symbol}' = ${p.value} is outside ${decl.min}..${decl.max}`);
    if (decl.integer && !Number.isInteger(p.value) && p.value !== decl.def) at(`'${p.symbol}' = ${p.value} is not a whole step`);
    const label = plugin.labels.get(p.symbol)?.get(p.value);
    if ((p.label ?? undefined) !== label) at(`'${p.symbol}' label is ${JSON.stringify(p.label)}, the plugin names ${p.value} ${JSON.stringify(label)}`);
  }
  return bad;
}
