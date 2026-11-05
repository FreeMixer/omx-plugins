#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * descriptor-features-check.mjs — the descriptor's feature words (audio-effect and its kin) are omx-contract's, the
 * enum of its schema's `$defs.plugin` features, and this tree reads them from the installed schema
 * (tools/descriptor.mjs FEATURES) instead of spelling them: FEATURES is what the schema says, read here on its own,
 * and no tool lists two or more of the words in one array literal, which is how the copy came back once.
 * `make descriptor-features`, part of `make test`.
 *
 *   node tools/test/descriptor-features-check.mjs               the check
 *   node tools/test/descriptor-features-check.mjs --self-test   the check, then a source with the list planted,
 *                                                               FEATURES with one word changed and a schema copy
 *                                                               without the enum: each red
 */
import { existsSync, readdirSync, readFileSync } from 'node:fs';
import { basename, dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { DESCRIPTOR_SCHEMA_FILE, FEATURES, descriptorFeatures, descriptorSchemaFile } from '../descriptor.mjs';

const root = join(dirname(fileURLToPath(import.meta.url)), '..', '..');
const SELF = basename(fileURLToPath(import.meta.url));

/** The lines of `text` where one array literal quotes two or more of `words`. */
export function literalsIn(text, words) {
  const out = [];
  const quoted = (w) => new RegExp(`['"\`]${w.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')}['"\`]`);
  for (const m of text.matchAll(/\[[^[\]]*\]/g)) {
    const n = words.filter((w) => quoted(w).test(m[0])).length;
    if (n >= 2) out.push(`line ${text.slice(0, m.index).split('\n').length}: ${m[0].trim().slice(0, 60)}`);
  }
  return out;
}

/** The refusals of `features` (FEATURES as a tool reads it) against `want` (the schema's enum). */
export function drift(features, want) {
  return JSON.stringify(features) === JSON.stringify(want) ? [] : [`FEATURES is ${JSON.stringify(features)}; the schema's enum is ${JSON.stringify(want)}`];
}

const schemaFile = descriptorSchemaFile();
const schema = JSON.parse(readFileSync(schemaFile, 'utf8'));
const want = schema.$defs?.plugin?.properties?.features?.items?.enum;
const errors = [];
if (!Array.isArray(want) || want.length < 2) errors.push(`${schemaFile}: no $defs.plugin.properties.features.items.enum of two or more words`);
else errors.push(...drift(FEATURES, want));
const sources = ['tools', 'tools/test'].flatMap((d) => readdirSync(join(root, d)).filter((f) => f.endsWith('.mjs') && f !== SELF).map((f) => join(d, f)));
let read = 0;
for (const rel of sources) {
  const text = readFileSync(join(root, rel), 'utf8');
  read++;
  for (const hit of literalsIn(text, want ?? FEATURES)) errors.push(`${rel}: lists the descriptor's feature words (${hit}); they are read from ${DESCRIPTOR_SCHEMA_FILE}`);
}
if (!existsSync(join(root, 'tools', 'descriptor.mjs')) || read < 2) errors.push('no tool source was read');
if (errors.length) {
  for (const e of errors) console.error(`descriptor-features: ${e}`);
  process.exit(1);
}
console.log(`descriptor-features: FEATURES (${FEATURES.join(', ')}) is ${DESCRIPTOR_SCHEMA_FILE}'s enum, and none of ${read} tools lists it`);

if (process.argv.includes('--self-test')) {
  let fail = 0;
  const red = (arm, out) => {
    if (out.length) console.log(`SABOTAGE ${arm}: red (${out[0]})`);
    else {
      console.error(`SABOTAGE ${arm}: STAYED GREEN`);
      fail++;
    }
  };
  const planted = `export const F = [${want.map((w) => `'${w}'`).join(', ')}];\n`;
  red('literal-planted', literalsIn(readFileSync(join(root, 'tools', 'descriptor.mjs'), 'utf8') + planted, want));
  red('literal-planted-reordered', literalsIn(`const x = ["${want[1]}", '${want[0]}'];`, want));
  if (literalsIn(`const one = ['${want[0]}', 'stereo'];`, want).length) {
    console.error('self-test: one word beside another list is not the enum, and was refused');
    fail++;
  } else console.log('self-test: a declaration naming one feature is not a copy of the list');
  red('word-changed', drift([...want.slice(0, -1), 'other'], want));
  red('word-dropped', drift(want.slice(1), want));
  const copy = structuredClone(schema);
  delete copy.$defs.plugin.properties.features.items.enum;
  let threw;
  try {
    descriptorFeatures(copy);
    threw = false;
  } catch (e) {
    threw = e.message;
  }
  red('schema-without-enum', threw ? [threw] : []);
  if (fail) process.exit(1);
  console.log('descriptor-features self-test: every sabotage red');
}
