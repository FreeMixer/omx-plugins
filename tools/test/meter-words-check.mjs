#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * meter-words-check.mjs — every meter of every plugin descriptor states omx-contract's meter words `held` and `clip`
 * (descriptorMeter, schema 3.10.0; spec 7.9), as the declaration spells them and the generated face rows carry them:
 * held unless the declaration says not, a clip word on a level only (true on a held one the face measures; a gain
 * reduction states none, the contract refuses it there). A descriptor whose meter
 * lacks a word, or carries one the declaration does not, is red. The check's own arm: a descriptor with the word
 * stripped is refused. Part of `make test`.
 *
 *   node tools/test/meter-words-check.mjs
 */
import { readFileSync, readdirSync } from 'node:fs';
import { join } from 'node:path';
import { ROOT } from '../omx-contract.mjs';
import { loadDecl, metersOf } from '../gen.mjs';

/** The faults of descriptor `d` against the words of declaration `decl`: one line each, none when it holds. */
export function meterWordFaults(stem, d, decl) {
  const want = metersOf(decl);
  const out = [];
  for (const m of d.meters ?? []) {
    const w = want.find((x) => x.id === m.symbol);
    if (!w) {
      out.push(`${stem}: meter ${m.symbol} is none of the declaration's`);
      continue;
    }
    if (typeof m.held !== 'boolean') out.push(`${stem}: meter ${m.symbol} states no held word`);
    else if (m.held !== w.held) out.push(`${stem}: meter ${m.symbol} says held ${m.held}, the declaration ${w.held}`);
    if (m.kind !== 'level') {
      if ('clip' in m) out.push(`${stem}: meter ${m.symbol} is a ${m.kind} with a clip word (omx-contract refuses it)`);
    } else if (typeof m.clip !== 'boolean') out.push(`${stem}: meter ${m.symbol} states no clip word`);
    else if (m.clip !== w.clip) out.push(`${stem}: meter ${m.symbol} says clip ${m.clip}, the declaration ${w.clip}`);
    if ((m.measurement ?? w.measurement) !== w.measurement) out.push(`${stem}: meter ${m.symbol} measures ${m.measurement}, the declaration ${w.measurement}`);
  }
  for (const w of want) if (!(d.meters ?? []).some((m) => m.symbol === w.id)) out.push(`${stem}: the declaration's meter ${w.id} is not in the descriptor`);
  return out;
}

let faults = [];
let meters = 0;
let held = 0;
let arm;
for (const stem of readdirSync(join(ROOT, 'plugins')).sort()) {
  let d;
  try {
    d = JSON.parse(readFileSync(join(ROOT, 'plugins', stem, `${stem}.descriptor.json`), 'utf8'));
  } catch {
    continue;
  }
  const decl = loadDecl(join(ROOT, 'plugins', stem));
  faults.push(...meterWordFaults(stem, d, decl));
  meters += (d.meters ?? []).length;
  held += (d.meters ?? []).filter((m) => m.held === true).length;
  if (!arm && (d.meters ?? []).some((m) => m.held === true)) {
    // the arm: a held meter's descriptor without the word is red
    const stripped = JSON.parse(JSON.stringify(d));
    delete stripped.meters.find((m) => m.held === true).held;
    arm = meterWordFaults(stem, stripped, decl).some((f) => f.includes('states no held word')) ? 'red' : `not red (${stem})`;
  }
}
for (const f of faults) console.error(`FAIL meter-words: ${f}`);
if (!meters) faults.push('no descriptor states a meter');
if (arm !== 'red') faults.push(`arm: a held meter's descriptor without the word is ${arm ?? 'not tried: no held meter'}`);
console.log(faults.length ? `FAIL meter-words: ${faults.length} fault(s)` : `ok   meter-words: ${meters} descriptor meters state held and clip as declared (${held} held); a stripped word is red`);
process.exit(faults.length ? 1 : 0);
