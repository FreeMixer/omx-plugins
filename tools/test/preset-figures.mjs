#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * preset-figures.mjs — the figures the console measured for its two catalogue presets, held on the factory preset
 * files through the DSP: each file's values are applied to omx-dsp's instance face (the kernel the plugin's
 * CLAP and LV2 faces call) and a measuring signal is run through it (tools/test/preset-figures.c).
 *
 *   amp-input (eq8, eq16)   a bell: +4.00 dB at its 2.5 kHz centre, and a decade below it (250 Hz) under 0.5 dB
 *   amp-tube  (drive)       a 1 kHz tone at -12 dBFS: H2 and H3 within 1 dB of the tube shaper's closed form
 *                           (-15.36 and -19.20 dBc), and 20 dB above the same stage at mix 0 (the dry control)
 *   dimension (chorus)      two voices at spread 0.5: L - R is a null (below -100 dB of L); the same chorus at
 *                           spread 0.25 is the control, L - R above -20 dB of L
 *
 * `make presets-figures`, part of `make test`. `--self-test` breaks a copy of each preset (a bell of 3 dB, a
 * drive of 6 dB, a spread of 0.25) and wants each figure red.
 * Needs omx-dsp (pkg-config omxdsp), a C compiler and the plugins' generated parameter headers.
 */
import { execFileSync } from 'node:child_process';
import { mkdirSync, readFileSync } from 'node:fs';
import { join } from 'node:path';
import { ROOT } from '../preset-lib.mjs';

const OUT = join(ROOT, 'build', 'figs');
const CC = process.env.CC || 'cc';
const pkg = (...a) => execFileSync(process.env.PKG_CONFIG || 'pkg-config', a, { encoding: 'utf8' }).trim().split(/\s+/).filter(Boolean);

const KERNELS = {
  eq8: ['-DFIG_EQ=8', 'omx-eq8'],
  eq16: ['-DFIG_EQ=16', 'omx-eq16'],
  drive: ['-DFIG_DRIVE', 'omx-drive'],
  chorus: ['-DFIG_CHORUS', 'omx-chorus'],
};

function build() {
  mkdirSync(OUT, { recursive: true });
  for (const [name, [flag, stem]] of Object.entries(KERNELS)) {
    execFileSync(CC, ['-std=gnu11', '-Wall', '-Wextra', '-Werror', '-O2', ...pkg('--cflags', 'omxdsp'), '-I' + join(ROOT, 'include'),
      flag, '-I' + join(ROOT, 'plugins', stem, 'generated'), join(ROOT, 'tools/test/preset-figures.c'), '-o', join(OUT, name), ...pkg('--libs', 'omxdsp'), '-lm'], { stdio: 'inherit' });
  }
}

const load = (plugin, id) => JSON.parse(readFileSync(join(ROOT, 'presets', plugin, `${id}.json`), 'utf8'));
const args = (preset, edit = {}) => preset.params.map((p) => `${p.symbol}=${edit[p.symbol] ?? p.value}`);
function measure(kernel, preset, edit, extra = []) {
  const out = execFileSync(join(OUT, kernel), [...args(preset, edit), ...extra], { encoding: 'utf8' });
  return Object.fromEntries(out.trim().split('\n').map((l) => { const [k, v] = l.split(' '); return [k, Number(v)]; }));
}

// the closed form of the tube shaper at 18 dB of drive and character 0.6 (the console's drive-shaper, one cycle's DFT)
const TUBE_H2 = -15.36;
const TUBE_H3 = -19.2;

/** Every figure for the preset files, or with `mutate` applied to a copy of each: the failures, one string each. */
function figures(mutate = {}) {
  const fails = [];
  const expect = (ok, what, got) => { if (!ok) fails.push(`${what}: ${typeof got === 'number' ? got.toFixed(4) : got}`); };
  for (const [kernel, plugin] of [['eq8', 'org.openmixer.eq8'], ['eq16', 'org.openmixer.eq16']]) {
    const m = measure(kernel, load(plugin, 'amp-input'), mutate.bell, ['probe=2500', 'probe=250']);
    expect(Math.abs(m.gain_2500_hz - 4) <= 0.05, `${plugin} amp-input: the bell lifts 2.5 kHz by 4.00 dB`, m.gain_2500_hz);
    expect(Math.abs(m.gain_250_hz) < 0.5, `${plugin} amp-input: 250 Hz, a decade below the bell, moves under 0.5 dB`, m.gain_250_hz);
  }
  const tube = load('org.openmixer.drive', 'amp-tube');
  const t = measure('drive', tube, mutate.tube);
  const off = measure('drive', tube, { ...mutate.tube, mix: 0 });
  expect(Math.abs(t.h2_dbc - TUBE_H2) <= 1, `amp-tube: H2 is ${TUBE_H2} dBc (closed form) within 1 dB`, t.h2_dbc);
  expect(Math.abs(t.h3_dbc - TUBE_H3) <= 1, `amp-tube: H3 is ${TUBE_H3} dBc (closed form) within 1 dB`, t.h3_dbc);
  expect(t.h2_dbc - off.h2_dbc >= 20, 'amp-tube: H2 stands 20 dB above the dry control (mix 0, the stage out of the path)', t.h2_dbc - off.h2_dbc);
  expect(t.h3_dbc - off.h3_dbc >= 20, 'amp-tube: H3 stands 20 dB above the dry control (mix 0, the stage out of the path)', t.h3_dbc - off.h3_dbc);
  const dim = load('org.openmixer.chorus', 'dimension');
  const d = measure('chorus', dim, mutate.dim);
  const ctl = measure('chorus', dim, { ...mutate.dim, spread: 0.25 });
  expect(d.lr_null_db < -100, 'dimension: L - R is a null at two voices, spread 0.5', d.lr_null_db);
  expect(ctl.lr_null_db > -20, 'dimension: at spread 0.25 the legs part (the control)', ctl.lr_null_db);
  return fails;
}

build();
const errors = figures();
if (process.argv.includes('--self-test')) {
  const arms = { 'a bell of 3 dB': { bell: { b1_gain: 3 } }, 'a drive of 6 dB': { tube: { amount: 6 } }, 'a spread of 0.25': { dim: { spread: 0.25 } } };
  if (errors.length) errors.push('self-test: the untouched presets are red');
  for (const [name, mutate] of Object.entries(arms)) if (figures(mutate).length === 0) errors.push(`self-test: ${name} stays green`);
}
if (errors.length) {
  for (const e of errors) console.error(`preset-figures: ${e}`);
  process.exit(1);
}
console.log('preset-figures: amp-input bell +4.00 dB at 2.5 kHz, amp-tube H2/H3 on the closed form, dimension L - R null');
