#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * answers-from-contract.mjs — a DRAFT of the wizard's answers for an effect whose kernel is already in
 * omx-dsp and whose controls are already in omx-contract (the FX migration). The wizard
 * (tools/omx-new-plugin.mjs --answers) stays the one gate: it validates every answer and writes every
 * artifact; this only removes the typing.
 *
 *   node tools/answers-from-contract.mjs <kernel> [--out <file>]     (default recipes/answers/omx-<kernel>.answers.json)
 *
 * From the pinned omx-contract (.github/pins.txt): one parameter per control of data/kernels/<kernel>.json,
 * each BY REFERENCE: a travel, a field of a fields table when the kernel has no single travels, a set
 * (a choice). Scalars, lists, sheets and the aggregate *_TRAVELS index are not controls. Scale is left
 * out: the schema's default (hertz logarithmic, everything else linear) is the port-hints law.
 * From the pinned omx-dsp: the description, the kernel header's own file summary; omxdspMin, the pin.
 * The CLAP feature and the LV2 class are the standard vocabulary's term for the effect's kind; a kernel
 * not in that table gets none and is reported REVIEW. Prints one line: ANSWERS <kernel> OK|REVIEW|FAIL ...
 */
import { execFileSync } from 'node:child_process';
import { existsSync, mkdirSync, readFileSync, readdirSync, writeFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { ROOT, contractPin, locateContract, pinOf } from './omx-contract.mjs';

/** The standard CLAP feature and LV2 class for each effect kind (clap/plugin-features.h, the LV2 core ontology). */
export const KIND = {
  flanger: ['flanger', 'lv2:FlangerPlugin'],
  reverb: ['reverb', 'lv2:ReverbPlugin'],
  limiter: ['limiter', 'lv2:LimiterPlugin'],
  phaser: ['phaser', 'lv2:PhaserPlugin'],
  pitch: ['pitch-shifter', 'lv2:PitchPlugin'],
  geq: ['equalizer', 'lv2:EQPlugin'],
  tremolo: ['tremolo', 'lv2:ModulatorPlugin'],
  transient: ['transient-shaper', 'lv2:DynamicsPlugin'],
};

/** Controls repeated once per entry of a contract list. Names contract entries only; no number here. */
export const EXPANSIONS = { geq: { ref: 'GEQ_BAND_RANGE', over: 'ISO_THIRD_OCTAVE_CENTRES_HZ' } };
const hzName = (hz) => (hz >= 1000 ? `${+(hz / 1000).toFixed(2)} kHz` : `${hz} Hz`);

/** The kernel header's own summary: its leading comment block, up to the first full stop. */
export function fileSummary(text) {
  const raw = text.split('\n');
  let lead = 0;
  while (lead < raw.length && /^\s*(\/\*|\*|\/\/|$)/.test(raw[lead])) lead++;
  const lines = raw.slice(0, lead).map((l) => l.replace(/^\s*\/?\*+\/?\s?/, '').trimEnd());
  let i = lines.findIndex((l) => /^(mix|omx)_[a-z0-9_]+\.h — /.test(l));
  let first;
  if (i >= 0) first = lines[i].replace(/^(mix|omx)_[a-z0-9_]+\.h — /, '');
  else {
    i = lines.findIndex((l) => l.startsWith('@brief '));
    if (i < 0) return null;
    first = lines[i].slice('@brief '.length);
  }
  const parts = [first.trim()];
  for (let j = i + 1; j < lines.length && !/\.(\s|$)/.test(parts.join(' ')) && lines[j].trim() && !lines[j].trim().startsWith('@'); j++) parts.push(lines[j].trim());
  const t = parts.join(' ').replace(/\s+/g, ' ');
  const stop = t.search(/\.(\s|$)/);
  return stop >= 0 ? t.slice(0, stop + 1) : t;
}

function omxdspInclude() {
  if (process.env.OMXDSP_INCLUDE) return process.env.OMXDSP_INCLUDE;
  return execFileSync('pkg-config', ['--variable=includedir', 'omxdsp'], { encoding: 'utf8' }).trim();
}
function findHeader(dir, name) {
  for (const e of readdirSync(dir, { withFileTypes: true })) {
    const p = join(dir, e.name);
    if (e.isDirectory()) { const f = findHeader(p, name); if (f) return f; }
    else if (e.name === name) return p;
  }
  return null;
}

/** TREMOLO_RATE_RANGE -> rate; a set's plural loses its s (TREMOLO_MODES -> mode). */
export function symbolOf(name, kernel, isSet) {
  let s = name.replace(/_RANGE$/, '');
  const k = kernel.toUpperCase() + '_';
  if (s.startsWith(k)) s = s.slice(k.length);
  s = s.toLowerCase().replace(/_([a-z0-9])/g, (_, c) => c.toUpperCase());
  return isSet ? s.replace(/s$/, '') : s;
}
const titleOf = (sym) => sym.replace(/([a-z0-9])([A-Z])/g, '$1 $2').replace(/^./, (c) => c.toUpperCase());

export function draft(kernel) {
  const loc = locateContract();
  if (!loc.dir) throw new Error(loc.why);
  const file = join(loc.dir, 'data', 'kernels', `${kernel}.json`);
  if (!existsSync(file)) throw new Error(`omx-contract ${contractPin()} has no data/kernels/${kernel}.json`);
  const decls = JSON.parse(readFileSync(file, 'utf8'));
  delete decls.$schema;
  const singles = Object.values(decls).some((e) => e && e.kind === 'travels' && e.travel);
  const params = [];
  const labelled = [];
  const exp = EXPANSIONS[kernel];
  if (exp) {
    // One control per entry of a contract list, all on the same travel (geq: a band per ISO centre).
    const lists = loc.dir ? Object.assign({}, ...readdirSync(join(loc.dir, 'data', 'kernels')).filter((n) => n.endsWith('.json'))
      .map((n) => JSON.parse(readFileSync(join(loc.dir, 'data', 'kernels', n), 'utf8')))) : {};
    const list = lists[exp.over];
    if (!list || list.kind !== 'list' || !Array.isArray(list.values)) throw new Error(`${exp.over} is not a list in omx-contract`);
    list.values.forEach((hz, i) => params.push({ symbol: `band${String(i + 1).padStart(2, '0')}`, name: hzName(hz), ref: exp.ref }));
  }
  for (const [name, e] of Object.entries(decls)) {
    if (!e || typeof e !== 'object') continue;
    if (exp && name === exp.ref) continue;
    if (e.kind === 'travels' && e.travel) {
      const symbol = symbolOf(name, kernel, false);
      params.push({ symbol, name: titleOf(symbol), ref: name });
    } else if (e.kind === 'travels' && e.fields && !singles) {
      for (const f of Object.keys(e.fields)) params.push({ symbol: f, name: titleOf(f), ref: name, field: f });
    } else if (e.kind === 'set') {
      // The labels a host shows are a design choice: drafted from the ids, reported for review.
      const symbol = symbolOf(name, kernel, true);
      params.push({ symbol, name: titleOf(symbol), ref: name, values: e.ids.map((id) => titleOf(id.replace(/[-_]([a-z0-9])/g, (_, c) => c.toUpperCase()))) });
      labelled.push(symbol);
    }
  }
  if (!params.length) throw new Error(`data/kernels/${kernel}.json declares no controls`);
  const h = findHeader(join(omxdspInclude(), 'omxdsp'), `omx_${kernel}.h`);
  if (!h) throw new Error(`omx-dsp has no omx_${kernel}.h`);
  const description = fileSummary(readFileSync(h, 'utf8'));
  if (!description) throw new Error(`${h} carries no file summary`);
  const kind = KIND[kernel];
  const answers = {
    stem: `omx-${kernel}`,
    kernel,
    description,
    clap: { features: ['audio-effect', ...(kind ? [kind[0]] : []), 'stereo'] },
    lv2: { class: kind ? kind[1] : 'lv2:Plugin' },
    params,
    omxdspMin: pinOf('omx-dsp'),
    contractRelease: contractPin(),
  };
  const review = [...(kind ? [] : ['clap feature', 'lv2 class']), ...labelled.map((s) => `labels of ${s}`)];
  return { answers, review };
}

if (import.meta.url === `file://${process.argv[1]}`) {
  const kernel = process.argv[2];
  try {
    if (!kernel || !/^[a-z][a-z0-9_]*$/.test(kernel)) throw new Error('usage: answers-from-contract.mjs <kernel> [--out <file>]');
    const oi = process.argv.indexOf('--out');
    const out = oi > 0 ? process.argv[oi + 1] : join(ROOT, 'recipes', 'answers', `omx-${kernel}.answers.json`);
    const { answers, review } = draft(kernel);
    mkdirSync(dirname(out), { recursive: true });
    writeFileSync(out, JSON.stringify(answers, null, 2) + '\n');
    const rel = out.startsWith(ROOT) ? out.slice(ROOT.length + 1) : out;
    console.log(`ANSWERS ${kernel} ${review.length ? `REVIEW ${review.join(', ')}` : 'OK'} ${answers.params.length} params -> ${rel}`);
  } catch (e) {
    console.log(`ANSWERS ${kernel ?? '?'} FAIL ${e.message}`);
    process.exit(1);
  }
}
