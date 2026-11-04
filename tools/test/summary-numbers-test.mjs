#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * summary-numbers-test.mjs — a plugin summary reads its numbers from the declaration (FreeMixer/omx-plugins#174).
 *
 * A number typed in a summary goes stale silently when omx-contract moves the travel it states. Two parts:
 *   1. NO TYPED LIMIT. A number in a shipped summary that is a limit or the default of one of the plugin's own
 *      parameters must be a `{max|min|default|range:<symbol>}` word (tools/gen.mjs summaryOf), not typed.
 *   2. FOLLOWS THE CONTRACT. For every summary that reads a parameter's limit, the travel's upper limit is moved
 *      out by one step in a copy of the installed release (OMX_CONTRACT_DIR): the catalogue row must then
 *      carry the moved limit, and no longer the old one.
 * `--self-test`: a summary with its limit typed again is refused by part 1, and part 2's reading of an unmoved
 * copy finds the row unchanged (so a typed summary could not pass it).
 *
 * `make summary-numbers`, part of `make test`. Needs the installed omx-contract. Exit 1 on any failure.
 */
import { mkdtempSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { ROOT, SHARED, loadDecl, pluginDirs, summaryOf } from '../gen.mjs';
import { findName, locateContract, paramKernel, perturbCopy } from '../omx-contract.mjs';

const where = locateContract(ROOT);
if (!where.dir) {
  console.error(`summary-numbers: ${where.why}`);
  process.exit(1);
}
const installed = where.dir;
let failed = 0;
const fail = (m) => {
  failed++;
  console.log(`FAIL ${m}`);
};

/** The numbers a summary types, outside its `{...}` words. */
const typed = (text) => [...text.replace(/\{[^}]*\}/g, ' ').matchAll(/(?<![\w.])\d+(?:\.\d+)?(?![\w])/g)].map((m) => Number(m[0]));
/** The typed numbers of `d`'s summary that are a limit or a default of one of its parameters. */
function typedLimits(d, summary = d.summary) {
  const limits = new Set(d.params.filter((p) => p.kind !== 'toggle').flatMap((p) => [p.min, p.max, p.def]).filter((x) => Number.isFinite(x)));
  return typed(summary ?? '').filter((n) => limits.has(n));
}
const row = (d) => SHARED['docs/catalogue.md'].catalogue([d]).find((l) => l.startsWith(`| **${d.name}**`));

function load(dir, contract) {
  process.env.OMX_CONTRACT_DIR = contract;
  try { return loadDecl(dir); } finally { delete process.env.OMX_CONTRACT_DIR; }
}

let summaries = 0;
let followed = 0;
for (const dir of pluginDirs()) {
  const d = load(dir, installed);
  if (d.summary === undefined) continue;
  summaries++;
  const bad = typedLimits(d);
  if (bad.length) fail(`${d.stem}: its summary types ${bad.join(', ')}, a limit or default of its own parameters; read it with {max|min|default|range:<symbol>}`);
  const words = [...d.summary.matchAll(/\{(?:max|range):([A-Za-z][A-Za-z0-9_]*)\}/g)].map((m) => m[1]);
  for (const symbol of new Set(words)) {
    const p = d.params.find((q) => q.symbol === symbol);
    if (p.ref === undefined || findName(installed, paramKernel(d, p, installed), p.ref).entry?.kind !== 'travels') continue;
    const scratch = mkdtempSync(join(tmpdir(), 'summary-numbers-'));
    try {
      perturbCopy(installed, scratch, paramKernel(d, p, installed), p, { move: 'max' });
      const moved = load(dir, scratch);
      const q = moved.params.find((x) => x.symbol === symbol);
      if (q.max === p.max) continue;
      if (!row(moved).includes(`${q.max}`) || row(moved) === row(d)) fail(`${d.stem}: '${symbol}' max moved ${p.max} -> ${q.max}, yet the catalogue row says "${summaryOf(moved)}"`);
      else followed++;
    } finally {
      rmSync(scratch, { recursive: true, force: true });
    }
  }
}
if (!summaries) fail('no shipped declaration carries a summary: nothing was checked');

if (process.argv.includes('--self-test')) {
  // a delay summary with its limit typed again is refused by part 1
  const d = load(pluginDirs().find((x) => x.endsWith('omx-delay')), installed);
  const limit = d.params.find((p) => p.symbol === 'fxDelayTime').max;
  const typedAgain = d.summary.replace('{max:fxDelayTime}', `${limit}`);
  if (typedLimits(d, typedAgain).join() !== `${limit}`) fail(`self-test: "${typedAgain}" was not caught as typing ${limit}`);
  else console.log(`PASS self-test: a summary typing ${limit} is refused`);
  if (typedLimits(d).length) fail('self-test: the shipped delay summary reads as typing a limit');
}

console.log(failed ? `summary-numbers: ${failed} failed` : `PASS summary-numbers: ${summaries} summaries type no limit of their own parameters (${followed} limits followed a moved contract)`);
process.exit(failed ? 1 : 0);
