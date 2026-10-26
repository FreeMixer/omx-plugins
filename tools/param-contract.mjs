// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * param-contract.mjs — org.openmixer.param-contract/1 of a plugin, from its declaration: each
 * parameter's symbol, unit, scale and step, the host's bypass last, and the canonical serialisation
 * whose SHA-256 is the fingerprint. tools/gen.mjs writes the rows a CLAP face serves
 * (include/omx_param_contract.h) and the header's unit enum from contract/param-units.json; tools/clap-ext-check.mjs
 * reads what a built face serves back.
 *
 * The contract's version is 1: its first publication, with no migration step. A later change of a
 * symbol, unit, scale, travel or stepping changes the fingerprint and needs the next version.
 */
import { createHash } from 'node:crypto';
import { readFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { lv2UnitNames, lv2UnitRef } from './format-tables.mjs';

export const CONTRACT_ID = 'org.openmixer.param-contract/1';
export const CONTRACT_VERSION = 1;

const ROOT = join(dirname(fileURLToPath(import.meta.url)), '..');

/** The declaration of the contract's units, installed into omx-plugins' contract directory as param-units.json: the
 * sheet OMX_PARAM_CONTRACT_UNITS, each token's code. tools/gen.mjs writes the header's enum from it. */
export const PARAM_UNITS_JSON = join(ROOT, 'contract', 'param-units.json');
export const PARAM_UNITS_ITEM = 'OMX_PARAM_CONTRACT_UNITS';
/** The header the extension's C structures and the unit enum are in. */
export const PARAM_CONTRACT_H = join(ROOT, 'include', 'omx_param_contract.h');

/** The declared units, `{ <token>: <code> }` in code order, of the declaration `file`. */
export function declaredUnits(file = PARAM_UNITS_JSON) {
  const item = JSON.parse(readFileSync(file, 'utf8'))[PARAM_UNITS_ITEM];
  if (item?.kind !== 'sheet' || typeof item.value !== 'object') throw new Error(`${file}: holds no ${PARAM_UNITS_ITEM} sheet`);
  return Object.fromEntries(Object.entries(item.value).sort((a, b) => a[1] - b[1]));
}

/** A token's enum name: OMX_CLAP_UNIT_ and the token upper-cased, a word break before a capital that follows a small
 * letter and before digits that follow a letter (midiNote is MIDI_NOTE, semitone12TET SEMITONE_12TET). */
export const unitEnumName = (token) => `OMX_CLAP_UNIT_${token.replace(/([a-z])([A-Z])/g, '$1_$2').replace(/([A-Za-z])([0-9])/g, '$1_$2').toUpperCase()}`;

/** The enum's lines, one per declared unit, a code past the LV2 block in hex. */
export const unitEnumLines = (units = declaredUnits()) =>
  Object.entries(units).map(([t, v]) => `${unitEnumName(t)} = ${v >= 0x100 ? `0x${v.toString(16)}` : v},`);

/** The units by name without underscores (SEMITONE12TET), as `Map<name, code>`: the declaration's, or with `file` the
 * enum omx_clap_param_unit of that header. */
let declared;
export function headerUnits(file) {
  if (!file) return (declared ??= new Map(Object.entries(declaredUnits()).map(([t, v]) => [t.toUpperCase(), v])));
  const body = readFileSync(file, 'utf8').match(/enum omx_clap_param_unit \{([\s\S]*?)\};/)?.[1] ?? '';
  return new Map([...body.matchAll(/OMX_CLAP_UNIT_(\w+)\s*=\s*(0x[0-9a-fA-F]+|\d+)/g)].map((m) => [m[1].replaceAll('_', ''), Number(m[2])]));
}

/** A declared unit -> the contract's unit, `[token, enum value]`: the token is the LV2 unit the face writes for it
 * (plugin-hostd's LV2 table read backwards, tools/format-tables.mjs: units:db -> db; a plugin-defined unit's name in
 * the vendor block's lower case: urn:openmixer:unit#dBFS -> dbfs), the value the declaration's. A unit it has no
 * value for (dB/oct), and "", is NONE, its text left to value_to_text. */
export function contractUnit(unit) {
  const ref = lv2UnitRef(unit);
  const token = ref && (ref.startsWith('units:') ? ref.slice('units:'.length) : ref.replace(/^<|>$/g, '').split('#').pop().toLowerCase());
  const value = token && headerUnits().get(token.toUpperCase());
  return value === undefined ? ['none', 0] : [token, value];
}

/** The token of a unit the contract serves (one with a header value, or "" for none), else undefined. */
export const servedToken = (unit) => (unit === '' ? 'none' : contractUnit(unit)[1] ? contractUnit(unit)[0] : undefined);

/** Every unit a face can write, with its contract unit: `{ <unit>: [token, value] }`, "" among them. */
export const contractUnits = () => Object.fromEntries(['', ...Object.keys(lv2UnitNames())].map((u) => [u, contractUnit(u)]));
export const SCALES = ['linear', 'log', 'stepped'];
const STEPPED = 0x1, BYPASS = 0x10;

/** Each parameter's contract, in id order, the host's bypass last: `{ symbol, unit, unitEnum, scale,
 * min, max, def, step, flags }` with min, max and def as the face's float travel reports them. */
export function contractOf(d, scaleOf) {
  const where = `plugins/${d.stem}`;
  const rows = d.params.map((p) => {
    const [unit, unitEnum] = contractUnit(p.unit);
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
