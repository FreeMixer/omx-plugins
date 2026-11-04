#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * placement-strips-check.mjs — a declaration's `console.placement.strips` is a restriction: the strip kinds a plugin
 * confines itself to, of the kinds the console gives an insert rack (schema/plugin.decl.schema.json enumerates them).
 * Left out, the plugin goes wherever the console puts its group. So a list that names every rack kind restates the
 * console and is refused, and so is a kind no rack hosts (the console's fxSend and mixMinus): the schema's enum
 * refuses it first, this names it. `make placement-strips`, part of `make test`.
 *
 *   node tools/test/placement-strips-check.mjs               the check, over every declaration
 *   node tools/test/placement-strips-check.mjs --self-test   the check, then declarations that list the whole rack
 *                                                            set, the console's eight kinds, a kind without a rack,
 *                                                            a kind twice and an empty list: each red; a real
 *                                                            restriction and no list: each green
 */
import { readdirSync, readFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

const root = join(dirname(fileURLToPath(import.meta.url)), '..', '..');
const SCHEMA = 'schema/plugin.decl.schema.json';

/** The strip kinds the console racks, as the declaration schema enumerates them. */
export function rackKinds(schema) {
  const list = schema?.$defs?.console?.properties?.placement?.properties?.strips?.items?.enum;
  if (!Array.isArray(list) || list.length < 2) throw new Error(`${SCHEMA}: console.placement.strips has no enum of the rack kinds`);
  return list;
}

/** The refusals of one declaration's `console.placement.strips` (undefined when left out) against `rack`. */
export function drift(strips, rack) {
  if (strips === undefined) return [];
  if (!Array.isArray(strips) || !strips.length) return ['strips is not a list of at least one strip kind; leave it out to restrict nothing'];
  const errors = [];
  for (const s of strips) if (!rack.includes(s)) errors.push(`strips names ${JSON.stringify(s)}, which no rack hosts (the rack kinds are ${rack.join(', ')})`);
  if (new Set(strips).size !== strips.length) errors.push('strips names a kind twice');
  if (!errors.length && strips.length >= rack.length) errors.push(`strips lists every rack kind (${rack.join(', ')}): that restates the console; leave it out`);
  return errors;
}

const rack = rackKinds(JSON.parse(readFileSync(join(root, SCHEMA), 'utf8')));
const decls = readdirSync(join(root, 'plugins'), { withFileTypes: true }).filter((e) => e.isDirectory()).map((e) => join('plugins', e.name, `${e.name}.decl.json`));
const errors = [];
let read = 0, restricted = 0, open = 0;
for (const rel of decls) {
  let decl;
  try { decl = JSON.parse(readFileSync(join(root, rel), 'utf8')); } catch { continue; }
  read++;
  const strips = decl.console?.placement?.strips;
  if (strips === undefined) open++;
  else restricted++;
  for (const e of drift(strips, rack)) errors.push(`${rel}: ${e}`);
}
if (read === 0) errors.push('no declaration was read');
if (errors.length) {
  for (const e of errors) console.error(`placement-strips: ${e}`);
  process.exit(1);
}
console.log(`placement-strips: ${read} declarations; ${restricted} restrict their strips to fewer than the ${rack.length} rack kinds, ${open} go wherever their group goes`);

if (process.argv.includes('--self-test')) {
  let fail = 0;
  const want = (arm, strips, red) => {
    const out = drift(strips, rack);
    if (Boolean(out.length) === red) console.log(`SABOTAGE ${arm}: ${red ? `red (${out[0]})` : 'green'}`);
    else {
      console.error(`SABOTAGE ${arm}: ${red ? 'STAYED GREEN' : `went red (${out[0]})`}`);
      fail++;
    }
  };
  want('the whole rack set', [...rack], true);
  want('the whole rack set, reordered', [...rack].reverse(), true);
  want('the console\'s eight kinds', [...rack, 'fxSend', 'mixMinus'], true);
  want('a kind without a rack', ['fxSend'], true);
  want('a kind twice', [rack[0], rack[0]], true);
  want('an empty list', [], true);
  want('a restriction', rack.slice(0, 2), false);
  want('no list', undefined, false);
  if (fail) process.exit(1);
  console.log('placement-strips self-test: every sabotage red, every restriction green');
}
