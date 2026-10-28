// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * param-units-check.mjs — the units tools/param-contract.mjs writes are the ones
 * include/omx_param_contract.h serves: each unit's token is the LV2 unit its face writes (plugin-hostd's LV2 table
 * read backwards) and names an OMX_CLAP_UNIT_<TOKEN> (the token upper-cased is the enum name without its underscores:
 * semitone12TET is OMX_CLAP_UNIT_SEMITONE_12TET); the units the declarations shipped with before the table was read
 * keep their token and value; and the vendor block is the one openmixer's CONTRACT_UNITS ends with (dbfs, dbu, ratio,
 * lufs from 0x100, append-only). `make units`, part of `make test`.
 *
 * The declaration and the header cannot drift: contract/param-units.json (OMX_PARAM_CONTRACT_UNITS, the file
 * omx-plugins-contract installs) is the source, the header's enum region is what tools/gen.mjs writes from it byte
 * for byte, and the enum names the same code for every token and no other.
 *
 *   node tools/test/param-units-check.mjs               the check
 *   node tools/test/param-units-check.mjs --self-test   the check, then the drift check over copies with one unit
 *                                                       changed in the declaration, one in the header, one added to
 *                                                       the declaration and one dropped from the header: each red
 */
import { mkdtempSync, readdirSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { fillRegions, SHARED } from '../gen.mjs';
import { contractUnits, declaredUnits, headerUnits, PARAM_CONTRACT_H, PARAM_UNITS_JSON, scalesOf, unitEnumLines } from '../param-contract.mjs';
import { locateContract } from '../omx-contract.mjs';

const root = join(dirname(fileURLToPath(import.meta.url)), '..', '..');
const HEADER = 'include/omx_param_contract.h';
const headerFile = process.env.OMX_PARAM_CONTRACT_H ?? PARAM_CONTRACT_H;
const VENDOR = ['dbfs', 'dbu', 'ratio', 'lufs'];
/** The table tools/param-contract.mjs held before it read plugin-hostd's (FreeMixer/omx-plugins#210): unchanged. */
const SHIPPED = {
  '': ['none', 0], s: ['s', 1], ms: ['ms', 2], min: ['min', 3], dB: ['db', 13], '%': ['pc', 14], x: ['coef', 15],
  Hz: ['hz', 16], oct: ['oct', 20], cents: ['cent', 21], st: ['semitone12TET', 22], dBFS: ['dbfs', 0x100], dBu: ['dbu', 0x101],
  LUFS: ['lufs', 0x103],
};

/** The refusals of a declaration `declFile` and a header `hFile` that drift apart. */
export function drift(declFile, hFile) {
  const errors = [];
  const text = readFileSync(hFile, 'utf8');
  const units = declaredUnits(declFile);
  let want;
  try {
    want = fillRegions(HEADER, text, { 'param-units': () => unitEnumLines(units) });
  } catch (e) {
    return [e.message];
  }
  if (Object.keys(SHARED[HEADER]).join() !== 'param-units') errors.push(`tools/gen.mjs fills ${HEADER}'s regions ${Object.keys(SHARED[HEADER]).join(', ')}, not param-units alone`);
  if (want !== text) errors.push(`${HEADER}: the enum region is not what tools/gen.mjs writes from contract/param-units.json (run \`node tools/gen.mjs\`)`);
  const declared = new Map(Object.entries(units).map(([t, v]) => [t.toUpperCase(), v]));
  const header = headerUnits(hFile);
  for (const [name, v] of declared) if (header.get(name) !== v) errors.push(`unit ${name}: the declaration has ${v}, the header ${header.get(name) ?? 'none'}`);
  for (const [name, v] of header) if (!declared.has(name)) errors.push(`unit ${name}: the header has ${v}, the declaration none`);
  return errors;
}

const errors = drift(PARAM_UNITS_JSON, headerFile);
// what `node tools/gen.mjs` itself writes into the header, its own producers
const text = readFileSync(headerFile, 'utf8');
if (fillRegions(HEADER, text, SHARED[HEADER]) !== text) errors.push(`${HEADER}: tools/gen.mjs writes another enum region (run \`node tools/gen.mjs\`)`);
const enumOf = headerUnits(headerFile);
const UNITS = contractUnits();
if (enumOf.size === 0) errors.push(`${HEADER}: no enum omx_clap_param_unit`);
// the scale words: the declaration schema offers a travel the contract's PARAM_SCALES but the one a `kind` already says
const contract = locateContract(root);
let scales = [];
if (!contract.dir) errors.push(`PARAM_SCALES: ${contract.why}`);
else {
  scales = scalesOf(contract.dir);
  // a scale's code is its position in PARAM_SCALES: the header's OMX_CLAP_SCALE_<WORD> values say the same
  const codes = [...text.matchAll(/OMX_CLAP_SCALE_(\w+)\s*=\s*(\d+)/g)].map((m) => [m[1].toLowerCase(), Number(m[2])]);
  if (JSON.stringify(codes) !== JSON.stringify(scales.map((w, i) => [w, i]))) errors.push(`${HEADER} numbers the scales ${JSON.stringify(codes)}; PARAM_SCALES is ${JSON.stringify(scales)}`);
  const schema = JSON.parse(readFileSync(join(root, 'schema/plugin.decl.schema.json'), 'utf8'));
  const offered = schema.$defs.param.properties.scale.enum;
  const want = scales.filter((w) => w !== 'stepped');
  if (JSON.stringify(offered) !== JSON.stringify(want)) errors.push(`schema/plugin.decl.schema.json offers scale ${JSON.stringify(offered)}; PARAM_SCALES minus stepped (a \`kind\` says it) is ${JSON.stringify(want)}`);
}
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
// a summary names no sample-rate span: STANDARD_SAMPLE_RATES is omx-contract's, and prose retyped from it goes stale
const SPAN = /\d+(\.\d+)?\s*(to|-|\u2013)\s*\d+(\.\d+)?\s*kHz/;
const decls = readdirSync(join(root, 'plugins'), { withFileTypes: true }).filter((e) => e.isDirectory()).map((e) => join(root, 'plugins', e.name, `${e.name}.decl.json`));
let summaries = 0;
for (const f of decls) {
  let decl;
  try { decl = JSON.parse(readFileSync(f, 'utf8')); } catch { continue; }
  if (typeof decl.summary !== 'string') continue;
  summaries++;
  if (SPAN.test(decl.summary)) errors.push(`${f.slice(root.length + 1)}: the summary states a sample-rate span ("${decl.summary.match(SPAN)[0]}"); the rates are omx-contract's STANDARD_SAMPLE_RATES`);
}
if (summaries === 0) errors.push('no declaration summary was read: the span check looked at nothing');
if (errors.length) {
  for (const e of errors) console.error(`param-units: ${e}`);
  process.exit(1);
}
console.log(`param-units: ${Object.keys(UNITS).length} declared units and the vendor block (${VENDOR.join(', ')}) match the header, its enum written from contract/param-units.json; the scale words are ${scales.join(', ')}`);

if (process.argv.includes('--self-test')) {
  const dir = mkdtempSync(join(tmpdir(), 'param-units-'));
  const decl = readFileSync(PARAM_UNITS_JSON, 'utf8');
  const head = readFileSync(headerFile, 'utf8');
  const arms = {
    'declaration-changed': [decl.replace('"db": 13,', '"db": 12,'), head],
    'header-changed': [decl, head.replace('OMX_CLAP_UNIT_DB = 13,', 'OMX_CLAP_UNIT_DB = 12,')],
    'declaration-added': [decl.replace('"lufs": 259', '"lufs": 259,\n      "phon": 260'), head],
    'header-dropped': [decl, head.replace(/^ *OMX_CLAP_UNIT_LUFS = 0x103,\n/m, '')],
  };
  let fail = 0;
  for (const [arm, [d, h]] of Object.entries(arms)) {
    if (d === decl && h === head) {
      console.error(`SABOTAGE ${arm}: the sabotage did not land`);
      fail++;
      continue;
    }
    writeFileSync(join(dir, 'param-units.json'), d);
    writeFileSync(join(dir, 'omx_param_contract.h'), h);
    const out = drift(join(dir, 'param-units.json'), join(dir, 'omx_param_contract.h'));
    if (out.length) console.log(`SABOTAGE ${arm}: red (${out[0]})`);
    else {
      console.error(`SABOTAGE ${arm}: STAYED GREEN`);
      fail++;
    }
  }
  writeFileSync(join(dir, 'param-units.json'), decl);
  writeFileSync(join(dir, 'omx_param_contract.h'), head);
  if (drift(join(dir, 'param-units.json'), join(dir, 'omx_param_contract.h')).length) {
    console.error('self-test: the unsabotaged copies are not green');
    fail++;
  }
  rmSync(dir, { recursive: true, force: true });
  if (fail) process.exit(1);
  console.log('param-units self-test: every sabotage red');
}
