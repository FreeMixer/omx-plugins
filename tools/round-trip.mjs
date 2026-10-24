#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * round-trip.mjs — the authored descriptor against what plugin-hostd scans back from our built faces
 * (docs/declaration-as-descriptor.md, "The round trip"; FreeMixer/omx-plugins#210, ruling B). tools/round-trip.sh
 * generates and builds each described plugin and writes `plugin-hostd-probe describe` of both faces; this compares
 * each scanned descriptor with the authored one, word by word, one line each:
 *
 *   equal <format> <where> <word>             the face states what the descriptor says
 *   lost <format> <where> <word> <authored>   the face does not carry the word: no row of plugin-hostd's <format>
 *                                             table reads it back, or the scan states none (`unknown`, or absent)
 *   differs <format> <where> <word> ...       the face states another value
 *
 * A word the format's table has no row for is lost by name, never filled in; so is a value the scan leaves unknown.
 * What the format itself says otherwise is named here, each once: an LV2 plugin's id is its URI and an LV2 control's
 * id its symbol (omx-contract's descriptor schema; plugin-hostd docs/formats.md), an LV2 face's audio ports are one
 * per channel with the generator's own symbols and names (the descriptor's port id and name are CLAP's), a CLAP face
 * reads back the float it stores. The face's own bypass (CLAP_PARAM_IS_BYPASS, lv2:enabled) stands beside the controls.
 * The bags (`extensions.<format>`) are equal: every source field the authored bag holds, the scanned one holds with the
 * same value (an LV2 field's values a set, as RDF states them).
 *
 * Pass: no `differs`, every bag equal, and the lost words the ones LOSSES names, by count (a new loss is red until it
 * is named there).
 *
 * Usage: node tools/round-trip.mjs <scans dir> [--authored <stem>=<file>]... ; the scans dir holds <stem>.clap.json
 * and <stem>.lv2.json for every described plugin. --authored reads another descriptor in the plugin's place (the
 * sabotage arm's). Exit 1 when it does not pass.
 */
import { execFileSync } from 'node:child_process';
import { existsSync, readFileSync, readdirSync } from 'node:fs';
import { join, resolve } from 'node:path';
import { descriptorFile } from './descriptor.mjs';
import { lookupValues, tableRows, tableWords } from './format-tables.mjs';
import { compareVersions } from './omx-contract.mjs';

/** The installed plugin-hostd's version, as pkg-config reads it, or undefined. */
function hostdVersion() {
  try {
    return execFileSync(process.env.PKG_CONFIG || 'pkg-config', ['--modversion', 'plugin-hostd'], { encoding: 'utf8', stdio: ['ignore', 'pipe', 'ignore'] }).trim() || undefined;
  } catch {
    return undefined;
  }
}

export const ROOT = resolve(new URL('..', import.meta.url).pathname);
export const FORMATS = ['clap', 'lv2'];

/** The losses of this tree, `<format> <word>` -> how many words (OR2: the meaning has a CLAP word for the bypass only
 * and an LV2 term for some meanings; LV2 has no unit for ""). Each other entry is a word the format's table has no
 * row for, or one its face states otherwise, named in the header above. */
export const LOSSES = {
  // OR2: a meaning has a CLAP word only for the bypass, an LV2 term only for some (lv2:designation); LV2 has no unit
  // for "" (a plain number)
  'clap controls[].meaning': 89,
  'lv2 controls[].meaning': 32,
  'lv2 controls[].travel.unit ""': 13,
  // plugin-hostd 0.7.0's LV2 table has no row for urn:openmixer:unit#ratio, the ratio unit our faces write (gap 4)
  'lv2 controls[].travel.unit "ratio"': 3,
  // CLAP: no row reads a unit, a linear or log scale, a step, a choice (our faces set no CLAP_PARAM_IS_ENUM), the
  // session symbol, the LV2 URI, a side-chain's role or what an LV2 host may lend
  'clap controls[].travel.unit': 113,
  'clap controls[].travel.scale': 85,
  'clap controls[].travel.step': 113,
  'clap controls[].kind': 15,
  'clap controls[].ids': 12,
  'clap controls[].values': 12,
  'clap controls[].labels': 12,
  'clap controls[].default': 15,
  'clap controls[].symbol': 128,
  'clap plugin.uri': 15,
  'clap ports[].role': 1,
  'clap needs.extensions.optional': 15,
  // LV2: no row reads a step, automation, the plugin's version or vendor (our .ttl writes no doap:maintainer); a
  // control's id is its symbol and a point's id its label, so the CLAP id and our point ids are no LV2 word; the
  // audio ports are one per channel, with the generator's symbols and names
  'lv2 controls[].travel.step': 113,
  'lv2 controls[].automatable': 128,
  'lv2 controls[].id': 128,
  'lv2 controls[].ids': 12,
  'lv2 plugin.version': 15,
  'lv2 plugin.vendor': 15,
  'lv2 ports[].id': 31,
  'lv2 ports[].name': 31,
  // the three toggles UPSTREAM names, scanned as travels, carry no toggle's default
  'lv2 controls[].default': 3,
};

/** A break in plugin-hostd's wave tip, reported, each `differs` it causes printed with it
 * and not failing; the run fails when one no longer shows, so the entry leaves with the fix (an entry that names its
 * `ports`: when a port outside them shows, or the installed plugin-hostd is past its `through`). `plugin-hostd-probe
 * describe` keeps a word's last value, and an LV2 dump lists lv2:portProperty lv2:toggled before rdf:type
 * lv2:ControlPort, so a toggle's kind is the control port's travel. */
export const UPSTREAM = {
  'lv2 controls[].kind "toggle" "travel"': { count: 3, why: 'plugin-hostd 83e54db describe: lv2:ControlPort overwrites lv2:toggled (src/describe.c get(), the last value)' },
  // plugin-hostd#142: an integer Hz port states lv2:integer (stepped) and pprops:logarithmic (log), and describe keeps
  // whichever of the two sord yields last. That order moves with the bundle's path and contents: the same omx-reverb
  // bundle scans "stepped" from one directory and "log" from another, and CI's make test and RPM jobs flip different
  // ports of the same commit. So the entry names every port that states both words (the only ones that can flip), a
  // flip on any other port fails, and the entry holds through the newest plugin-hostd release seen still flipping
  // (0.9.1, in CI): a newer plugin-hostd fails the run until the entry is retired or moved on. plugin-hostd#142 is still
  // open and 0.9.1's release notes do not touch the scale flip, so the hold moves on from 0.9.0.
  'lv2 controls[].travel.scale "stepped" "log"': {
    ports: ['omx-deesser deessFreq', 'omx-drive bandFreq', 'omx-keyed-gate key_hp', 'omx-keyed-gate key_lp', 'omx-phaser baseHz', 'omx-reverb lowcut', 'omx-reverb highcut'],
    through: '0.9.1',
    why: 'plugin-hostd#142: of lv2:integer and pprops:logarithmic, describe keeps the last sord yields (src/describe.c get())',
  },
};

/** The UPSTREAM entry a difference `k` at `port` (`<stem> <where>`) is, or undefined: an entry that names its ports
 * covers those alone. */
const upstreamOf = (k, port) => (UPSTREAM[k] && (!UPSTREAM[k].ports || UPSTREAM[k].ports.includes(port)) ? UPSTREAM[k] : undefined);

/** The described plugins (a descriptor beside the declaration), and the ones outside the round trip. */
export function described(root = ROOT) {
  const stems = readdirSync(join(root, 'plugins'), { withFileTypes: true }).filter((e) => e.isDirectory()).map((e) => e.name).sort();
  return { inside: stems.filter((s) => existsSync(join(root, 'plugins', s, descriptorFile(s)))), outside: stems.filter((s) => !existsSync(join(root, 'plugins', s, descriptorFile(s)))) };
}

const f32 = (x) => (typeof x === 'number' ? Math.fround(x) : x);
const same = (a, b) => JSON.stringify(a) === JSON.stringify(b);
const sorted = (v) => (Array.isArray(v) ? [...v].sort() : [v]);
const FACE_OWN = { clap: 'bypass', lv2: 'enabled' };

/** The leaf words of `o`, `[path, value]`, arrays of scalars as one value. */
function leaves(o, path = '', out = []) {
  if (o && typeof o === 'object' && !Array.isArray(o)) for (const [k, v] of Object.entries(o)) leaves(v, path ? `${path}.${k}` : k, out);
  else out.push([path, o]);
  return out;
}

/** Every line of one face's comparison: `[{ verdict, format, where, word, authored, scanned }]`. */
export function compareFace(format, authored, scanned) {
  const out = [];
  const words = tableWords(format);
  // a value no row of the table gives the word is describe's own when no row set it (a CLAP control a `travel`, a CLAP
  // port that is not main `aux`): the face stated nothing
  const lookupOnly = (word) => Object.values(tableRows(format)).every((r) => r.word?.word !== word || r.word.transform === 'lookup');
  const defaulted = (word, s) => lookupOnly(word) && words.has(word) && !Object.values(lookupValues(format, word)).some((v) => same(v, s));
  const emit = (where, word, a, s, carried = true) => {
    const verdict = !carried || s === undefined || s === 'unknown' || (!same(a, s) && defaulted(word, s)) ? 'lost' : same(a, s) ? 'equal' : 'differs';
    out.push({ verdict, format, where, word, authored: a, scanned: s });
  };
  const read = (word, base = word) => words.has(base) || words.has(base.replace(/^(controls|ports)\[\]\./, '$1[].'));

  // the plugin: what a face states of it
  const ap = authored.plugin, sp = scanned.plugin;
  for (const [k, a] of Object.entries(ap)) {
    if (k === 'format') continue;
    if (format === 'lv2' && k === 'id') emit('plugin', 'plugin.id', ap.uri, sp.id);
    else if (format === 'lv2' && k === 'uri') emit('plugin', 'plugin.uri', a, sp.uri);
    else emit('plugin', `plugin.${k}`, a, sp[k], read(`plugin.${k}`));
  }

  // the controls, in order, the face's own bypass beside them
  const sc = scanned.controls.filter((c) => c.meaning !== FACE_OWN[format]);
  if (sc.length !== authored.controls.length) out.push({ verdict: 'differs', format, where: 'controls', word: 'controls', authored: authored.controls.length, scanned: sc.length });
  authored.controls.forEach((c, i) => {
    const s = sc[i] ?? {};
    const where = c.symbol ?? String(c.id);
    for (const [path, a] of leaves(c)) {
      if (path.startsWith('source')) continue;
      const word = `controls[].${path}`;
      if (format === 'lv2' && path === 'id') emit(where, word, a, undefined, false);
      else if (format === 'lv2' && path === 'symbol') emit(where, word, a, s.id);
      // a choice's points carry its values and labels; describe names each point by its label, so the default is
      // the value it designates and our ids are no word a face carries
      else if (c.kind === 'choice' && s.kind === 'choice' && path === 'values') emit(where, word, a, s.values);
      else if (c.kind === 'choice' && s.kind === 'choice' && path === 'default') emit(where, word, c.values[c.ids.indexOf(a)], s.values?.[s.ids?.indexOf(s.default)]);
      else if (path === 'ids') emit(where, word, a, undefined, false);
      else {
        const v = path.split('.').reduce((o, k) => o?.[k], s);
        const fl = format === 'clap' && path.startsWith('travel.') && typeof a === 'number';
        emit(where, word, fl ? f32(a) : a, fl ? f32(v) : v, word === 'controls[].symbol' ? false : read(word) || ['controls[].id', 'controls[].name'].includes(word));
      }
    }
  });

  // the ports: by direction, in order
  const sports = [...scanned.ports];
  for (const p of authored.ports) {
    const k = sports.findIndex((q) => q.direction === p.direction);
    const s = k < 0 ? {} : sports.splice(k, 1)[0];
    const where = `port ${p.direction} ${p.name}`;
    for (const [path, a] of leaves(p)) {
      const word = `ports[].${path}`;
      const perChannel = format === 'lv2' && p.kind === 'audio' && (path === 'id' || path === 'name');
      emit(where, word, a, s[path], !perChannel);
    }
  }
  for (const s of sports) out.push({ verdict: 'differs', format, where: `port ${s.direction} ${s.name}`, word: 'ports[]', authored: undefined, scanned: s.id });

  // the rest of the descriptor's words: latency, needs, instrument
  for (const k of ['latency', 'needs', 'instrument']) {
    for (const [path, a] of leaves(authored[k] ?? {}, k)) emit('plugin', path, a, path.split('.').reduce((o, x) => o?.[x], scanned), read(path));
    if (scanned[k] && !authored[k]) out.push({ verdict: 'differs', format, where: 'plugin', word: k, authored: undefined, scanned: scanned[k] });
  }

  // the bag
  const ab = authored.extensions?.[format] ?? {}, sb = scanned.extensions?.[format] ?? {};
  for (const [field, a] of Object.entries(ab)) {
    const s = sb[field];
    // the bag holds a field's one value as it is, two or more as a list (plugin-hostd docs/formats.md)
    const eq = format === 'lv2' ? same(sorted(a), sorted(s)) : same(a, s);
    out.push({ verdict: eq ? 'equal' : 'differs', format, where: 'bag', word: `extensions.${format}.${field}`, authored: a, scanned: s });
  }
  return out;
}

/** The lines of a whole run: `{ lines, losses, fails }`. */
export function roundTrip(scansDir, { root = ROOT, authoredFiles = {} } = {}) {
  const { inside, outside } = described(root);
  const lines = [], losses = {}, upstream = {};
  let fails = 0;
  for (const stem of inside) {
    const authored = JSON.parse(readFileSync(authoredFiles[stem] ?? join(root, 'plugins', stem, descriptorFile(stem)), 'utf8'));
    for (const format of FORMATS) {
      const file = join(scansDir, `${stem}.${format}.json`);
      if (!existsSync(file)) {
        lines.push(`differs ${format} ${stem}: no scan (${file})`);
        fails++;
        continue;
      }
      for (const r of compareFace(format, authored, JSON.parse(readFileSync(file, 'utf8')))) {
        const show = (v) => (v === undefined ? '-' : JSON.stringify(v));
        if (r.verdict === 'equal') lines.push(`equal ${format} ${stem} ${r.where} ${r.word}`);
        else if (r.verdict === 'lost') {
          lines.push(`lost ${format} ${stem} ${r.where} ${r.word} ${show(r.authored)}`);
          const k = r.word === 'controls[].travel.unit' && format === 'lv2' ? `${format} ${r.word} ${show(r.authored)}` : `${format} ${r.word}`;
          losses[k] = (losses[k] ?? 0) + 1;
        } else if (upstreamOf(`${format} ${r.word} ${show(r.authored)} ${show(r.scanned)}`, `${stem} ${r.where}`)) {
          const k = `${format} ${r.word} ${show(r.authored)} ${show(r.scanned)}`;
          lines.push(`differs ${format} ${stem} ${r.where} ${r.word}: authored ${show(r.authored)}, scanned ${show(r.scanned)} (upstream: ${UPSTREAM[k].why})`);
          upstream[k] = (upstream[k] ?? 0) + 1;
        } else {
          lines.push(`differs ${format} ${stem} ${r.where} ${r.word}: authored ${show(r.authored)}, scanned ${show(r.scanned)}`);
          fails++;
        }
      }
    }
  }
  for (const stem of outside) lines.push(`outside ${stem}: no descriptor, outside the round trip`);
  for (const k of [...new Set([...Object.keys(LOSSES), ...Object.keys(losses)])].sort()) {
    const ok = LOSSES[k] === losses[k];
    lines.push(`${ok ? 'named' : 'unnamed'} loss ${k}: ${losses[k] ?? 0} word(s)${ok ? '' : `, LOSSES names ${LOSSES[k] ?? 0}`}`);
    if (!ok) fails++;
  }
  for (const [k, { count, ports, through }] of Object.entries(UPSTREAM)) {
    if (ports) {
      const at = hostdVersion();
      const ok = at !== undefined && compareVersions(at, through) <= 0;
      lines.push(ok ? `upstream ${k}: ${upstream[k] ?? 0} of the ${ports.length} named port(s) flip under plugin-hostd ${at}`
        : `unnamed upstream ${k}: plugin-hostd ${at ?? '(no version: pkg-config --modversion plugin-hostd)'} is past ${through}, the release the entry holds through: run without it, retire it if no named port flips, else move it on`);
      if (!ok) fails++;
      continue;
    }
    const ok = upstream[k] === count;
    lines.push(`${ok ? 'upstream' : 'unnamed upstream'} ${k}: ${upstream[k] ?? 0} word(s)${ok ? '' : `, UPSTREAM names ${count} (fixed upstream? remove the entry)`}`);
    if (!ok) fails++;
  }
  return { lines, losses, fails, inside, outside };
}

if (process.argv[1] && resolve(process.argv[1]) === resolve(new URL(import.meta.url).pathname)) {
  const args = process.argv.slice(2);
  const authoredFiles = {};
  for (let i = 1; i < args.length; i += 2) if (args[i] === '--authored') { const [s, f] = args[i + 1].split('='); authoredFiles[s] = f; }
  const { lines, fails, inside, outside } = roundTrip(args[0], { authoredFiles });
  for (const l of lines) console.log(l);
  const count = (v) => lines.filter((l) => l.startsWith(`${v} `)).length;
  const up = lines.filter((l) => l.startsWith('differs ') && l.includes(' (upstream: ')).length;
  console.log(`round-trip: ${inside.length} plugins, both faces: ${count('equal')} equal, ${count('lost')} lost, ${count('differs') - up} differs, ${up} upstream; ${outside.length} outside (${outside.join(', ')})`);
  if (fails) console.log(`FAIL round-trip: ${fails} difference(s) or unnamed loss(es)`);
  process.exit(fails ? 1 : 0);
}
