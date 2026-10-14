#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * manual-perturb.mjs — a declaration's manual follows omx-contract (`make manual`, part of `make test`).
 *
 * For every shipped plugin whose declaration carries a `manual`, and every travel its parameters
 * read from omx-contract (each ref and field once), two moves, each in a fresh copy of the pinned
 * release read through OMX_CONTRACT_DIR (tools/omx-contract.mjs perturbCopy):
 *
 *   1. the travel's upper limit goes out by one step: where the plugin's README section carried the
 *      parameter's range ("<min> to <max>"), it must carry it fewer times against the copy;
 *   2. the travel's default goes to its other end, for a parameter whose default is the travel's
 *      own (no defaultId, defaultRef or defaultBand): where the section carried the default
 *      ("<default> <unit>"), it must carry it fewer times.
 *
 * A manual that types a range or a default instead of reading it ({range:<symbol>},
 * {default:<symbol>}, tools/gen.mjs fillManual) keeps the old words, and fails. A move that leaves
 * the resolved parameter where it was (a travel another choice widens past the moved limit) has
 * nothing to follow and is skipped.
 *
 * Needs omx-contract at the pin (tools/omx-contract.mjs locateContract). Exit 1 on any failure.
 */
import { mkdtempSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { ROOT, SHARED, loadDecl, paramRow, pluginDirs } from '../gen.mjs';
import { findName, locateContract, paramKernel, perturbCopy } from '../omx-contract.mjs';

const where = locateContract(ROOT);
if (!where.dir) {
  console.error(`manual-perturb: ${where.why}`);
  process.exit(1);
}
const pin = where.dir;
const section = (d) => SHARED['README.md'].sections([d]).join('\n');
/** How many times `words` stand on their own in `text` (not inside a longer number). */
const times = (text, words) => text.match(new RegExp(`(?<![\\w.-])${words.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')}(?![\\w]|\\.\\d)`, 'g'))?.length ?? 0;
const rangeWords = (p) => `${p.min} to ${p.max}`;
const defaultWords = (p) => paramRow(p).default;

let failed = 0;
const fail = (m) => {
  failed++;
  console.log(`FAIL ${m}`);
};

/** The plugin at `dir` loaded against a copy of the pin's release with `p`'s travel moved. */
function against(dir, d, p, move) {
  const scratch = mkdtempSync(join(tmpdir(), 'manual-perturb-'));
  try {
    perturbCopy(pin, scratch, paramKernel(d, p, pin), p, { move });
    process.env.OMX_CONTRACT_DIR = scratch;
    return loadDecl(dir);
  } finally {
    delete process.env.OMX_CONTRACT_DIR;
    rmSync(scratch, { recursive: true, force: true });
  }
}

let plugins = 0;
let followed = 0;
for (const dir of pluginDirs()) {
  process.env.OMX_CONTRACT_DIR = pin;
  const d = loadDecl(dir);
  delete process.env.OMX_CONTRACT_DIR;
  if (!d.manual) continue;
  plugins++;
  const was = section(d);
  const seen = new Set();
  for (const p of d.params) {
    if (p.ref === undefined || findName(pin, paramKernel(d, p, pin), p.ref).entry?.kind !== 'travels') continue;
    const own = p.defaultId === undefined && p.defaultRef === undefined && p.defaultBand === undefined;
    for (const [move, words] of [['max', rangeWords], ...(own ? [['default', defaultWords]] : [])]) {
      const key = `${p.ref}.${p.field ?? ''}:${move}`;
      if (seen.has(key)) continue;
      seen.add(key);
      const before = times(was, words(p));
      if (!before) continue;
      const moved = against(dir, d, p, move);
      const q = moved.params.find((x) => x.symbol === p.symbol);
      if (words(q) === words(p)) continue;
      const after = times(section(moved), words(p));
      if (after >= before) fail(`${d.stem}: ${p.ref}${p.field ? `.${p.field}` : ''} ${move} moved ('${p.symbol}' now ${words(q)}), yet its manual still says "${words(p)}" ${after} time(s): typed, not read`);
      else followed++;
    }
  }
}

if (!plugins) fail('no shipped declaration carries a manual: nothing was checked');
console.log(failed ? `manual-perturb: ${failed} failed` : `PASS manual-perturb: ${plugins} manuals follow omx-contract (${followed} moves of a range or a default they carry)`);
process.exit(failed ? 1 : 0);
