#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * strip-pages-follow-test.mjs — omx-strip's CLAP remote-control pages follow its band count (#308). The face serves
 * clap.remote-controls pages cut over the controls Bands shows; when Bands changes it rebuilds them and tells the host
 * (the CLAP remote-controls extension's changed()), so bands 5 to 32 are reachable from a control surface. Read back
 * through the extension by tools/test/clap-pages-follow-probe.c over the BUILT face, against omx-panel's page rule
 * (pagesOf) over the declaration at the same value:
 *
 *   1. at the come-up count (4) the pages are pagesOf at 4, and the host has not been told of a change;
 *   2. Bands set to 32: the pages are pagesOf at 32 (every band on a page), the host told once;
 *   3. Bands set to 9, then 9 again: the pages are pagesOf at 9, told once for the move, not for the repeat;
 *   4. Bands back to 4: the pages are the come-up pages again.
 *
 * Usage: strip-pages-follow-test.mjs <probe-output-file>   (the probe run with: 4 32 9 9 4, eqBands' id)
 * `make strip-pages-follow` builds the face and the probe and runs it. Needs omx-contract, omx-dsp, omx-panel.
 */
import { readFileSync } from 'node:fs';
import { join } from 'node:path';
import { panelDescription } from '../descriptor.mjs';
import { loadDecl } from '../gen.mjs';
import { pagesOf } from '../omx-panel.mjs';

const ROOT = join(import.meta.dirname, '..', '..');
const NONE = 4294967295;
const d = loadDecl(join(ROOT, 'plugins', 'omx-strip'));
const x = panelDescription(d);

/** The probe's steps: `{ value, changed, pages: [{ section, name, ids }] }`. */
export function parse(text) {
  const steps = [];
  for (const line of text.split('\n')) {
    const f = line.split('\t');
    if (f[0] === 'step') steps.push({ value: f[1], changed: Number(f[2]), count: Number(f[3]), pages: [] });
    else if (f[0] === 'page') steps.at(-1).pages.push({ section: f[2], name: f[3], ids: f[4]?.split(',').map(Number) });
  }
  return steps;
}
const want = (bands) => pagesOf(x, { values: { eqBands: bands } }).map((g) => ({ section: g.sectionName, name: g.pageName, ids: g.ids.map((i) => i ?? NONE) }));

let bad = 0, n = 0;
function check(what, got, wanted) {
  n++;
  const ok = JSON.stringify(got) === JSON.stringify(wanted);
  if (!ok) bad++;
  console.log(`${ok ? 'PASS' : 'FAIL'} strip-pages-follow ${what}${ok ? '' : `: got ${JSON.stringify(got).slice(0, 300)}, want ${JSON.stringify(wanted).slice(0, 300)}`}`);
}

const steps = parse(readFileSync(process.argv[2], 'utf8'));
const [init, s32, s9, s9again, back] = [steps[0], steps[2], steps[3], steps[4], steps[5]];
check('the probe ran init and five values', steps.map((s) => s.value), ['init', '4', '32', '9', '9', '4']);
check('the pages at 4 bands hold fewer pages than at 32', want(4).length < want(32).length, true);
check('init: the pages are the come-up pages (4 bands), no change told', [init?.pages, init?.changed], [want(4), 0]);
check('Bands 4 set at 4: no change told', steps[1]?.changed, 0);
check('Bands 32: every band is on a page', s32?.pages, want(32));
check('Bands 32: the host was told once', s32?.changed, 1);
check('Bands 32: the last page holds band 32 (eq32On)', s32?.pages.at(-1).ids.some((id) => id !== NONE), true);
check('Bands 9: the pages are the pages at 9', s9?.pages, want(9));
check('Bands 9: the host was told again', s9?.changed, 2);
check('Bands 9 again: not told for a repeat', s9again?.changed, 2);
check('Bands back to 4: the come-up pages', back?.pages, want(4));
check('Bands back to 4: told once more', back?.changed, 3);
console.log(`strip-pages-follow: ${n - bad}/${n} passed`);
process.exit(bad ? 1 : 0);
