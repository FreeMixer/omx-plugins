#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * strip-visible-when-test.mjs — omx-strip's panel follows its band count (#308): the declaration's
 * `visibleWhen` gives band N's five controls `{ param: eqBands, atLeast: N }`, and omx-panel's layout carries it
 * into the panel description. Read by value, at eqBands 4 (the come-up count) and at 9:
 *
 *   1. every control of bands 1..4 is shown and every control of bands 5..32 hidden, and at 9 the cut moves to 9;
 *   2. each band control's rule names eqBands, and every other control has none (always shown);
 *   3. the pages of eight at eqBands 4 hold each control of bands 1..4 and none of bands 5..32.
 *
 * With `visibleWhen` taken out of the declaration every band control is shown at any count, and 1 and 3 go red.
 *
 * `make strip-visible-when`, part of `make test`. It loads the declaration, so it needs omx-contract, omx-dsp and an
 * omx-panel whose layout reads `visibleWhen` (OMX_PANEL_DIR).
 */
import { join } from 'node:path';
import { panelDescription } from '../descriptor.mjs';
import { loadDecl } from '../gen.mjs';
import { isVisible, pagesOf } from '../omx-panel.mjs';

const ROOT = join(import.meta.dirname, '..', '..');
const BANDS = 32;
const PARTS = ['Type', 'Freq', 'Gain', 'Q', 'On'];

let bad = 0, n = 0;
const same = (a, b) => JSON.stringify(a) === JSON.stringify(b);
function check(what, got, want) {
  n++;
  const ok = same(got, want);
  if (!ok) bad++;
  console.log(`${ok ? 'PASS' : 'FAIL'} strip-visible-when ${what}${ok ? '' : `: got ${JSON.stringify(got)}, want ${JSON.stringify(want)}`}`);
}

const d = loadDecl(join(ROOT, 'plugins', 'omx-strip'));
const x = panelDescription(d);
const bySymbol = new Map(x.controls.map((c) => [c.symbol, c]));
const band = (b) => PARTS.map((p) => `eq${b}${p}`);
const bands = (from, to) => Array.from({ length: to - from + 1 }, (_v, i) => band(from + i)).flat();
const all = bands(1, BANDS);

check(`the description holds every control of the ${BANDS} bands`, all.filter((s) => !bySymbol.has(s)), []);

// 1. shown and hidden by the count
for (const count of [4, 9]) {
  const shown = (syms) => syms.filter((s) => bySymbol.has(s) && isVisible(bySymbol.get(s), { eqBands: count }));
  check(`eqBands ${count}: bands 1..${count} shown`, shown(bands(1, count)), bands(1, count));
  check(`eqBands ${count}: bands ${count + 1}..${BANDS} hidden`, shown(bands(count + 1, BANDS)), []);
}
// the come-up count is the declared default, 4: a description read with no values shows bands 1..4
check('come-up: bands 1..4 shown, the rest hidden', all.filter((s) => isVisible(bySymbol.get(s))), bands(1, 4));

// 2. the rules: each band control governed by eqBands, nothing else governed
check('every band control is governed by eqBands', all.filter((s) => bySymbol.get(s)?.visibleWhen?.symbol !== 'eqBands'), []);
check('no other control is governed', x.controls.filter((c) => !all.includes(c.symbol) && c.visible !== null).map((c) => c.symbol), []);

// 3. the pages of eight at eqBands 4
const ids = new Map(x.controls.map((c) => [c.param, c.symbol]));
const paged = pagesOf(x, { values: { eqBands: 4 } }).flatMap((g) => g.ids).filter((id) => id !== null).map((id) => ids.get(id));
check('eqBands 4: the pages hold bands 1..4', bands(1, 4).filter((s) => !paged.includes(s)), []);
check('eqBands 4: no page holds a band past 4', paged.filter((s) => bands(5, BANDS).includes(s)), []);

console.log(`strip-visible-when: ${n - bad}/${n} passed`);
process.exit(bad ? 1 : 0);
