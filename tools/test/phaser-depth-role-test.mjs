#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * phaser-depth-role-test.mjs — omx-phaser's GENERATED panel, read as committed, publishes its depth role
 * (FreeMixer/omx-plugins#232): the role row `depth` names the descriptor's depthOct, the declaration gives that
 * control octaves, and no "Declared roles not published" note names depth. Before omx-panel 0.7.0 (omx-panel#80) the
 * modulation family's depth took none, pc and coef, not oct, so a modulation house view had no depth control for
 * the phaser.
 *
 * `--self-test`: the same checks over the header with the depth row taken out, and with the old note put back,
 * must each fail.
 *
 * `make phaser-depth`, part of `make test`. It reads files only: a stale file is gen.mjs --check's.
 */
import { readFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..', '..');
const DIR = join(ROOT, 'plugins', 'omx-phaser');
const header = readFileSync(join(DIR, 'generated', 'omx_phaser_params.h'), 'utf8');
const descriptor = JSON.parse(readFileSync(join(DIR, 'omx-phaser.descriptor.json'), 'utf8'));
const decl = JSON.parse(readFileSync(join(DIR, 'omx-phaser.decl.json'), 'utf8'));

/** the unit the declaration gives a symbol, wherever its face declares it */
function unitOf(node, symbol) {
  if (node === null || typeof node !== 'object') return undefined;
  if (node[symbol] && typeof node[symbol] === 'object' && typeof node[symbol].unit === 'string') return node[symbol].unit;
  for (const v of Object.values(node)) {
    const u = unitOf(v, symbol);
    if (u !== undefined) return u;
  }
  return undefined;
}

/** the failures of the checks over a header text */
function failures(text) {
  const out = [];
  const rows = [...text.matchAll(/\{ "([a-z]+)", (\d+)u, (\d+)u \}/g)].map((m) => ({ role: m[1], index: Number(m[2]), param: Number(m[3]) }));
  const depth = rows.find((r) => r.role === 'depth');
  if (depth === undefined) out.push('no depth role row in OMX_PHASER_PANEL_ROLES');
  else {
    const control = descriptor.controls[depth.param];
    if (control?.symbol !== 'depthOct') out.push(`the depth row names control ${depth.param} (${control?.symbol}), not depthOct`);
    if (control?.meaning !== 'depth') out.push(`depthOct's meaning is ${control?.meaning}, not depth`);
  }
  if (/roles not published:[\s\S]*?\bdepth\b/.test(text)) out.push('a "roles not published" note still names depth');
  if (unitOf(decl, 'depthOct') !== 'units:oct') out.push(`depthOct is declared in ${unitOf(decl, 'depthOct')}, not units:oct`);
  return out;
}

if (process.argv.includes('--self-test')) {
  const noRow = header.replace(/\s*\{ "depth", \d+u, \d+u \},/, '');
  const noted = header.replace('Family modulation. */', 'Family modulation.\n * Declared roles not published:\n *   depth (depthOct): the depth role takes none, pc, coef, not oct */');
  let bad = 0;
  for (const [name, text] of [['the depth row taken out', noRow], ['the old note put back', noted]]) {
    const red = text !== header && failures(text).length > 0;
    console.log(`${red ? 'PASS' : 'FAIL'} phaser-depth self-test: ${name} is ${red ? 'red' : 'NOT red'}`);
    bad += red ? 0 : 1;
  }
  process.exit(bad ? 1 : 0);
}
const f = failures(header);
for (const m of f) console.log(`FAIL phaser-depth ${m}`);
if (f.length === 0) console.log('PASS phaser-depth omx-phaser publishes its depth role on depthOct, in octaves');
process.exit(f.length ? 1 : 0);
