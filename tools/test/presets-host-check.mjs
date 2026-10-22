#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * presets-host-check.mjs — one plugin's factory presets as a host finds and loads them
 * (docs/presets.md), read through tools/preset-probe.c from the built faces, against the presets
 * tools/gen-presets.mjs builds (the one source; `make presets` holds the files to it):
 *
 *   - CLAP: the discovery factory declares /usr/share/omx/presets/<clap id>/ (factory content, file
 *     type json, the plugin's soundpack); crawled (from the tree's presets/), its provider lists
 *     exactly the plugin's presets, each by its name and the plugin id; each, loaded through
 *     clap.preset-load into a fresh instance and read back through clap.params, gives every value
 *     of the file exactly (as the plugin's float holds it), and the host hears `loaded` once;
 *   - LV2: lilv lists the bundle's presets, as many as the files, each with its label and exactly
 *     the file's port values.
 *
 * Usage: `node tools/test/presets-host-check.mjs [--self-test] <preset-probe> <plugin dir>` after the
 * plugin is built (`make test` of every plugin). `--self-test` also drops a file from a copy of the
 * presets directory, changes a value in another copy, drops a preset from a copy of the bundle's
 * manifest and changes a value in a copy of its presets.ttl, and wants each red.
 */
import { execFileSync } from 'node:child_process';
import { cpSync, mkdtempSync, readFileSync, readdirSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { basename, join, resolve } from 'node:path';
import { build } from '../gen-presets.mjs';
import { PRESET_DIR, ROOT, plugins, presetUri } from '../preset-lib.mjs';

const INSTALLED = '/usr/share/omx/presets';
const FACTORY = 1; // CLAP_PRESET_DISCOVERY_IS_FACTORY_CONTENT

const args = process.argv.slice(2).filter((a) => a !== '--self-test');
if (args.length !== 2) {
  console.error('usage: presets-host-check.mjs [--self-test] <preset-probe> <plugin dir>');
  process.exit(2);
}
const [probe, dir] = args.map((a) => resolve(a));
const stem = basename(dir);
const plugin = plugins().find((p) => p.stem === stem);
if (!plugin) {
  console.error(`presets-host-check: ${stem} is no shipped plugin`);
  process.exit(2);
}
const want = [...build()].filter(([path]) => path.startsWith(`${PRESET_DIR}/${plugin.id}/`)).map(([path, text]) => ({ file: basename(path), preset: JSON.parse(text) }));
const same = (printed, value) => Math.fround(Number(printed)) === Math.fround(value);

const run = (...a) => {
  try {
    return { out: execFileSync(probe, a, { encoding: 'utf8', stdio: ['ignore', 'pipe', 'pipe'] }), err: '' };
  } catch (e) {
    return { out: e.stdout ?? '', err: `${e.stderr ?? ''}probe exit ${e.status}` };
  }
};
const rows = (out, tag) => out.split('\n').filter((l) => l.startsWith(`${tag}\t`)).map((l) => l.split('\t').slice(1));

/** The CLAP face's problems, its presets read from `tree` (the installed root's stand-in). */
function clapProblems(tree) {
  const bad = [];
  const { out, err } = run('clap', join(dir, 'build', `${stem}.clap`), INSTALLED, tree);
  if (err) bad.push(`clap: ${err.trim()}`);
  const location = `${INSTALLED}/${plugin.id}/`;
  const locs = rows(out, 'location');
  if (locs.length !== 1 || locs[0][1] !== location || Number(locs[0][0]) !== FACTORY) bad.push(`clap: declared locations ${JSON.stringify(locs)}, want one factory location ${location}`);
  if (!rows(out, 'filetype').some((r) => r[0] === 'json')) bad.push('clap: no json file type');
  if (!rows(out, 'soundpack').some((r) => r[0] === plugin.id)) bad.push(`clap: no soundpack ${plugin.id}`);
  const listed = rows(out, 'preset');
  const names = listed.map((r) => r[0]).sort().join(' ');
  const expected = want.map((w) => w.file).sort().join(' ');
  if (names !== expected) bad.push(`clap: discovery lists [${names}], the presets are [${expected}]`);
  for (const [file, id, flags, name] of listed) {
    const w = want.find((x) => x.file === file);
    if (!w) continue;
    if (name !== w.preset.name) bad.push(`clap: ${file} is named ${JSON.stringify(name)}, the file says ${JSON.stringify(w.preset.name)}`);
    if (id !== `clap:${plugin.id}`) bad.push(`clap: ${file} is for ${id}`);
    if (Number(flags) !== FACTORY) bad.push(`clap: ${file} flags ${flags}, not factory content`);
  }
  const values = rows(out, 'value');
  const loaded = rows(out, 'loaded').map((r) => r[0]);
  for (const { file, preset } of want) {
    if (loaded.filter((f) => f === file).length !== 1) bad.push(`clap: the host heard ${file} loaded ${loaded.filter((f) => f === file).length} times`);
    for (const { symbol, value } of preset.params) {
      const id = plugin.params.findIndex((p) => p.symbol === symbol);
      const got = values.find((v) => v[0] === file && Number(v[1]) === id)?.[2];
      if (got === undefined || !same(got, value)) bad.push(`clap: ${file} '${symbol}' reads back ${got}, the file says ${value}`);
    }
  }
  return bad;
}

/** The LV2 bundle's problems, read from `bundle`. */
function lv2Problems(bundle) {
  const bad = [];
  const { out, err } = run('lv2', bundle, plugin.uri);
  if (err) bad.push(`lv2: ${err.trim()}`);
  const listed = rows(out, 'lv2-preset');
  if (listed.length !== want.length) bad.push(`lv2: lilv lists ${listed.length} presets, the files are ${want.length}`);
  const values = rows(out, 'lv2-value');
  for (const { preset } of want) {
    const uri = presetUri(plugin.uri, preset);
    const row = listed.find((r) => r[0] === uri);
    if (!row) { bad.push(`lv2: lilv lists no ${uri}`); continue; }
    if (row[1] !== preset.name) bad.push(`lv2: ${uri} is labelled ${JSON.stringify(row[1])}, the file says ${JSON.stringify(preset.name)}`);
    const mine = values.filter((v) => v[0] === uri);
    if (mine.length !== preset.params.length) bad.push(`lv2: ${uri} sets ${mine.length} ports, the file ${preset.params.length}`);
    for (const { symbol, value } of preset.params) {
      const got = mine.find((v) => v[1] === symbol)?.[2];
      if (got === undefined || !same(got, value)) bad.push(`lv2: ${uri} '${symbol}' is ${got}, the file says ${value}`);
    }
  }
  return bad;
}

const tree = join(ROOT, PRESET_DIR);
const bundle = join(dir, 'build', `${stem}.lv2`);
const errors = [...clapProblems(tree), ...lv2Problems(bundle)];

if (process.argv.includes('--self-test')) {
  const tmp = mkdtempSync(join(tmpdir(), 'presets-host-'));
  try {
    const copyTree = (name) => { const t = join(tmp, name); cpSync(join(tree, plugin.id), join(t, plugin.id), { recursive: true }); return t; };
    const copyBundle = (name) => { const b = join(tmp, name); cpSync(bundle, b, { recursive: true }); return b; };
    const first = want[0];
    const moved = (p) => { const d = plugin.params.find((x) => x.symbol === p.symbol); return p.value === d.max ? d.min : d.max; };
    const arms = {
      'a preset file dropped from the directory': () => {
        const t = copyTree('dropped');
        rmSync(join(t, plugin.id, first.file));
        return clapProblems(t);
      },
      'a value changed in a preset file': () => {
        const t = copyTree('changed');
        const f = join(t, plugin.id, first.file);
        const p = JSON.parse(readFileSync(f, 'utf8'));
        p.params[0].value = moved(p.params[0]);
        writeFileSync(f, JSON.stringify(p, null, 2));
        return clapProblems(t);
      },
      'a preset dropped from the bundle manifest': () => {
        const b = copyBundle('dropped.lv2');
        const m = join(b, 'manifest.ttl');
        const uri = presetUri(plugin.uri, first.preset);
        const text = readFileSync(m, 'utf8');
        const at = text.indexOf(`\n<${uri}>\n`);
        const end = text.indexOf('<presets.ttl> .\n', at) + '<presets.ttl> .\n'.length;
        if (at < 0 || end <= at) throw new Error(`self-test: ${uri} is not in the manifest`);
        writeFileSync(m, text.slice(0, at + 1) + text.slice(end));
        return lv2Problems(b);
      },
      'a value changed in the bundle presets.ttl': () => {
        const b = copyBundle('changed.lv2');
        const t = join(b, 'presets.ttl');
        const q = first.preset.params[0];
        const text = readFileSync(t, 'utf8');
        const at = text.indexOf(`<${presetUri(plugin.uri, first.preset)}>`);
        const was = `[ lv2:symbol "${q.symbol}" ; pset:value `;
        if (at < 0 || text.indexOf(was, at) < 0) throw new Error(`self-test: ${q.symbol} is not in presets.ttl`);
        const k = text.indexOf(was, at) + was.length;
        writeFileSync(t, `${text.slice(0, k)}${moved(q)}.0${text.slice(text.indexOf(' ]', k))}`);
        return lv2Problems(b);
      },
    };
    if (readdirSync(join(tree, plugin.id)).length < 1) errors.push('self-test: no presets to break');
    for (const [name, arm] of Object.entries(arms)) if (arm().length === 0) errors.push(`self-test: ${name} stays green`);
  } finally {
    rmSync(tmp, { recursive: true, force: true });
  }
}

if (errors.length) {
  for (const e of errors) console.error(`presets-host-check: ${stem}: ${e}`);
  process.exit(1);
}
console.log(`presets-host-check: ${stem}: ${want.length} presets listed and loaded through CLAP and lilv, exact${process.argv.includes('--self-test') ? '; 4 sabotages red' : ''}`);
