// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * param-contract.mjs — org.openmixer.param-contract/1 of a plugin, from its declaration: each
 * parameter's symbol, unit, scale and step, the host's bypass last, and the canonical serialisation
 * whose SHA-256 is the fingerprint. tools/gen.mjs writes the rows a CLAP face serves
 * (include/omx_param_contract.h); tools/clap-ext-check.mjs reads what a built face serves back.
 *
 * The contract's version is 1: its first publication, with no migration step. A later change of a
 * symbol, unit, scale, travel or stepping changes the fingerprint and needs the next version.
 */
import { createHash } from 'node:crypto';

export const CONTRACT_ID = 'org.openmixer.param-contract/1';
export const CONTRACT_VERSION = 1;

/** A declared unit -> the contract's unit: its token (the LV2 local name, or the vendor block's)
 * and its enum value. A unit with no token here is NONE, its text left to value_to_text. */
export const UNITS = {
  '': ['none', 0], s: ['s', 1], ms: ['ms', 2], min: ['min', 3], dB: ['db', 13], '%': ['pc', 14], x: ['coef', 15],
  Hz: ['hz', 16], oct: ['oct', 20], cents: ['cent', 21], st: ['semitone12TET', 22], dBFS: ['dbfs', 0x100], dBu: ['dbu', 0x101],
  LUFS: ['lufs', 0x103],
};
export const SCALES = ['linear', 'log', 'stepped'];
const STEPPED = 0x1, BYPASS = 0x10;

/** Each parameter's contract, in id order, the host's bypass last: `{ symbol, unit, unitEnum, scale,
 * min, max, def, step, flags }` with min, max and def as the face's float travel reports them. */
export function contractOf(d, scaleOf) {
  const where = `plugins/${d.stem}`;
  const rows = d.params.map((p) => {
    const [unit, unitEnum] = UNITS[p.unit] ?? UNITS[''];
    const scale = p.kind ? 'stepped' : scaleOf(p) === 'log' ? 'log' : 'linear';
    if (scale === 'log' && !(p.min > 0 && p.max > 0)) throw new Error(`${where}: '${p.symbol}' is logarithmic over ${p.min}..${p.max}; a log scale needs a positive travel`);
    return { symbol: p.symbol, unit, unitEnum, scale, min: Math.fround(p.min), max: Math.fround(p.max), def: Math.fround(p.def), step: scale === 'stepped' ? 1 : 0, flags: p.kind ? STEPPED : 0 };
  });
  if (rows.some((r) => r.symbol === 'bypass')) throw new Error(`${where}: 'bypass' is the host's bypass parameter's symbol; a declared parameter cannot take it`);
  rows.push({ symbol: 'bypass', unit: 'none', unitEnum: 0, scale: 'stepped', min: 0, max: 1, def: 0, step: 1, flags: STEPPED | BYPASS });
  return rows;
}

/** The shortest decimal that round-trips to the same binary64, -0 written 0. */
const shortest = (n) => (Object.is(n, -0) ? '0' : String(n));

/** The canonical serialisation: the id line, then `id symbol unit scale min max default step flags`
 * per parameter, TAB-separated, LF after every line. */
export const contractCanonical = (rows) =>
  `${[CONTRACT_ID, ...rows.map((r, id) => [id, r.symbol, r.unit, r.scale, shortest(r.min), shortest(r.max), shortest(r.def), shortest(r.step), r.flags.toString(16)].join('\t'))].join('\n')}\n`;

export const contractFingerprint = (rows) => createHash('sha256').update(contractCanonical(rows)).digest('hex');
