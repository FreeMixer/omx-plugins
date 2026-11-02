#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * param-tie-test.mjs — a parameter's symbol is tied to the control its ref names (FreeMixer/omx-plugins#58).
 *
 * Without the tie, swapping the refs of two parameters whose travels differ (tremolo's rateHz and depth) was
 * accepted: gen --check stayed fresh, the identity oracle follows the declaration and the parameter check
 * compares the face with the same declaration. Two parts, both by value:
 *   1. symbolTie over the cases that matter: the control's own words, a part of them, a band prefix, the frozen
 *      symbol that says its `control`, and the refusals (the other control's words, a wrong `control`, a stranger).
 *   2. the shipped declarations in a scratch copy: omx-tremolo's two refs swapped is refused naming both symbols,
 *      omx-keyed-gate without its `control` extras is refused for key_external, key_hp and key_lp, and the
 *      unchanged copies load.
 *
 * `make param-tie`, part of `make test`. Reads files; needs the contract the generator reads.
 */
import { cpSync, mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { loadDecl, symbolTie } from '../gen.mjs';

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..', '..');
let failed = 0;
const check = (ok, what, detail = '') => {
  console.log(`${ok ? 'PASS' : 'FAIL'} ${what}${ok ? '' : ` ${detail}`}`);
  if (!ok) failed++;
};
const ctl = (...names) => names.map((name) => ({ name }));

// 1. the rule
const KERNEL = ctl('rateHz', 'depth', 'bandOn', 'hpfOn', 'thresholdDb', 'attackMs', 'attackCurve');
const tie = (symbol, name, extra = {}) => symbolTie({ symbol, ref: 'REF', ...extra }, { name }, KERNEL);
check(tie('rateHz', 'rateHz').length === 0, 'a symbol that is its control passes');
check(tie('rate_hz', 'rateHz').length === 0, 'its snake spelling passes');
check(tie('threshold', 'thresholdDb').length === 0, 'a part of the control words in order passes');
check(tie('b3_on', 'bandOn').length === 0, 'a band prefix is dropped');
check(tie('tap2Gain', 'tap2Gain').length === 0, 'digits do not tell controls apart');
check(tie('depth', 'rateHz').length === 1, 'depth on rate_hz is refused', JSON.stringify(tie('depth', 'rateHz')));
check(tie('rateHz', 'depth').length === 1, 'rateHz on depth is refused');
check(/is the words of 'hpfOn'/.test(tie('hpfOn', 'bandOn')[0] ?? ''), 'the exact words of another control are that control');
check(tie('key_external', 'keySource').length === 1, 'a frozen symbol with no `control` is refused');
check(tie('key_external', 'keySource', { control: 'keySource' }).length === 0, 'a frozen symbol that says its control passes');
check(tie('key_external', 'keySource', { control: 'hpHz' }).length === 1, 'a `control` that is not the ref\'s is refused');

// 2. the declarations
const scratch = mkdtempSync(join(tmpdir(), 'param-tie-'));
try {
  const copy = (stem, edit) => {
    const dir = join(scratch, 'plugins', stem);
    cpSync(join(ROOT, 'plugins', stem), dir, { recursive: true, filter: (s) => !/\/(build|generated)(\/|$)/.test(s) });
    edit?.(dir);
    return dir;
  };
  const load = (dir) => {
    try { return { d: loadDecl(dir) }; } catch (e) { return { e: String(e.message) }; }
  };
  const tremolo = load(copy('omx-tremolo'));
  check(tremolo.d !== undefined, 'omx-tremolo loads as shipped', tremolo.e);
  const swapped = load(copy('omx-tremolo', (dir) => {
    const f = join(dir, 'omx-tremolo.descriptor.json');
    const j = JSON.parse(readFileSync(f, 'utf8'));
    const [a, b] = ['rateHz', 'depth'].map((s) => j.controls.find((c) => c.symbol === s));
    [a.source, b.source] = [b.source, a.source];
    writeFileSync(f, `${JSON.stringify(j, null, 2)}\n`);
  }));
  check(swapped.e !== undefined && /'rateHz'/.test(swapped.e) && /'depth'/.test(swapped.e), 'omx-tremolo with rateHz and depth swapped is refused naming both', swapped.e ?? 'it loaded');
  const gate = load(copy('omx-keyed-gate'));
  check(gate.d !== undefined, 'omx-keyed-gate loads as shipped', gate.e);
  const bare = load(copy('omx-keyed-gate', (dir) => {
    const f = join(dir, 'omx-keyed-gate.decl.json');
    const j = JSON.parse(readFileSync(f, 'utf8'));
    for (const k of Object.keys(j.controls)) delete j.controls[k].control;
    writeFileSync(f, `${JSON.stringify(j, null, 2)}\n`);
  }));
  check(bare.e !== undefined && ['key_external', 'key_hp', 'key_lp'].every((s) => bare.e.includes(`'${s}'`)), 'omx-keyed-gate without its `control` extras is refused for its three frozen symbols', bare.e ?? 'it loaded');
} finally {
  rmSync(scratch, { recursive: true, force: true });
}
process.exit(failed ? 1 : 0);
