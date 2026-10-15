// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * param-units-check.mjs — the units tools/param-contract.mjs writes are the ones
 * include/omx_param_contract.h serves: every declared unit's token names an OMX_CLAP_UNIT_<TOKEN> with
 * the same value, and the vendor block is the one openmixer's CONTRACT_UNITS ends with (dbfs, dbu,
 * ratio, lufs from 0x100, append-only). `make units`, part of `make test`.
 */
import { readFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { UNITS } from '../param-contract.mjs';

const root = join(dirname(fileURLToPath(import.meta.url)), '..', '..');
const header = readFileSync(process.env.OMX_PARAM_CONTRACT_H ?? join(root, 'include/omx_param_contract.h'), 'utf8');
const body = header.match(/enum omx_clap_param_unit \{([\s\S]*?)\};/)?.[1] ?? '';
const enumOf = new Map([...body.matchAll(/OMX_CLAP_UNIT_(\w+)\s*=\s*(0x[0-9a-fA-F]+|\d+)/g)].map((m) => [m[1], Number(m[2])]));
const VENDOR = ['dbfs', 'dbu', 'ratio', 'lufs'];

const errors = [];
if (enumOf.size === 0) errors.push('include/omx_param_contract.h: no enum omx_clap_param_unit');
for (const [declared, [token, code]] of Object.entries(UNITS)) {
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
