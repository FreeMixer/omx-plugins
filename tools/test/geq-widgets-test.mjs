#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * geq-widgets-test.mjs — omx-geq's 31 bands are drawn as faders (FreeMixer/omx-plugins#231: the declaration named
 * no widget, so every band was served as a knob although the summary promises one fader per band). Compared by value:
 *
 *   1. the declaration's panel names the fader widget for each of the 31 band parameters, and no other;
 *   2. the panel description omx-panel derives from the declaration gives every band control the fader widget;
 *   3. the panel rows of the COMMITTED generated header, the description every face serves, are those 31 faders,
 *      in band order, and no knob.
 *
 * `make geq-widgets`, part of `make test`. It reads the declaration and the committed header: a stale header is
 * gen.mjs --check's.
 */
import { readFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { panelDescription } from '../descriptor.mjs';
import { loadDecl } from '../gen.mjs';

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..', '..');
const DIR = join(ROOT, 'plugins', 'omx-geq');
const d = loadDecl(DIR);
const bands = Array.from({ length: 31 }, (_, i) => `band${String(i + 1).padStart(2, '0')}`);

let bad = 0, n = 0;
const check = (ok, what, detail = '') => {
  n++;
  if (!ok) bad++;
  console.log(`${ok ? 'PASS' : 'FAIL'} geq-widgets ${what}${ok ? '' : `: ${detail}`}`);
};
const same = (a, b) => JSON.stringify(a) === JSON.stringify(b);

const declared = d.panel.widgets ?? {};
check(same(Object.keys(declared), bands) && Object.values(declared).every((w) => w === 'fader'), 'the declaration names the fader for the 31 bands', JSON.stringify(declared));

const described = panelDescription(d).controls.map((c) => [c.symbol, c.widget]);
check(same(described, bands.map((b) => [b, 'fader'])), 'the panel description draws every band as a fader', JSON.stringify(described));

const header = readFileSync(join(DIR, 'generated', 'omx_geq_params.h'), 'utf8');
const rows = [...header.matchAll(/OMX_CLAP_WIDGET_(\w+), "fx\.geq\.(band\d+)"/g)].map((m) => [m[2], m[1]]);
check(same(rows, bands.map((b) => [b, 'FADER'])), 'the generated header serves 31 faders in band order and no knob', JSON.stringify(rows));

console.log(`geq-widgets: ${n - bad}/${n} pass`);
process.exit(bad ? 1 : 0);
