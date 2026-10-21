// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * param-units-check.mjs — the units tools/param-contract.mjs writes are the ones
 * include/omx_param_contract.h serves: each unit's token is the LV2 unit its face writes (plugin-hostd's LV2 table
 * read backwards) and names an OMX_CLAP_UNIT_<TOKEN> (the token upper-cased is the enum name without its underscores:
 * semitone12TET is OMX_CLAP_UNIT_SEMITONE_12TET); the units the declarations shipped with before the table was read
 * keep their token and value; and the vendor block is the one openmixer's CONTRACT_UNITS ends with (dbfs, dbu, ratio,
 * lufs from 0x100, append-only). `make units`, part of `make test`.
 */
import { readFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { contractUnits } from '../param-contract.mjs';

const root = join(dirname(fileURLToPath(import.meta.url)), '..', '..');
const header = readFileSync(process.env.OMX_PARAM_CONTRACT_H ?? join(root, 'include/omx_param_contract.h'), 'utf8');
const body = header.match(/enum omx_clap_param_unit \{([\s\S]*?)\};/)?.[1] ?? '';
const enumOf = new Map([...body.matchAll(/OMX_CLAP_UNIT_(\w+)\s*=\s*(0x[0-9a-fA-F]+|\d+)/g)].map((m) => [m[1].replaceAll('_', ''), Number(m[2])]));
const VENDOR = ['dbfs', 'dbu', 'ratio', 'lufs'];
/** The table tools/param-contract.mjs held before it read plugin-hostd's (FreeMixer/omx-plugins#210): unchanged. */
const SHIPPED = {
  '': ['none', 0], s: ['s', 1], ms: ['ms', 2], min: ['min', 3], dB: ['db', 13], '%': ['pc', 14], x: ['coef', 15],
  Hz: ['hz', 16], oct: ['oct', 20], cents: ['cent', 21], st: ['semitone12TET', 22], dBFS: ['dbfs', 0x100], dBu: ['dbu', 0x101],
  LUFS: ['lufs', 0x103],
};
const UNITS = contractUnits();

const errors = [];
if (enumOf.size === 0) errors.push('include/omx_param_contract.h: no enum omx_clap_param_unit');
for (const [declared, want] of Object.entries(SHIPPED)) if (JSON.stringify(UNITS[declared]) !== JSON.stringify(want)) errors.push(`unit '${declared}' writes ${JSON.stringify(UNITS[declared])}, it shipped as ${JSON.stringify(want)}`);
for (const [declared, [token, code]] of Object.entries(UNITS)) {
  if (token === 'none') continue;
  const got = enumOf.get(token.toUpperCase());
  if (got !== code) errors.push(`unit '${declared}' writes ${token} = ${code}, the header has ${got === undefined ? 'no OMX_CLAP_UNIT_' + token.toUpperCase() : got}`);
}
const vendor = [...enumOf].filter(([, v]) => v >= 0x100).sort((a, b) => a[1] - b[1]);
const want = VENDOR.map((t, i) => [t.toUpperCase(), 0x100 + i]);
if (JSON.stringify(vendor) !== JSON.stringify(want)) {
  errors.push(`vendor block is ${vendor.map(([n, v]) => `${n}=0x${v.toString(16)}`).join(', ')}; expected ${want.map(([n, v]) => `${n}=0x${v.toString(16)}`).join(', ')}`);
}
if (errors.length) {
  for (const e of errors) console.error(`param-units: ${e}`);
  process.exit(1);
}
console.log(`param-units: ${Object.keys(UNITS).length} declared units and the vendor block (${VENDOR.join(', ')}) match the header`);
