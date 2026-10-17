#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * descriptor-view-test.mjs — a plugin described in omx-contract's words (docs/declaration-as-descriptor.md):
 * its <stem>.descriptor.json and the extras of its <stem>.decl.json make the declaration tools/gen.mjs
 * resolves. Held by value over every described plugin, each rule with an arm that breaks it on a copy:
 *
 *   1. the descriptor is fresh: it is what its declaration renders, and it holds to the descriptor words of
 *      the installed omx-contract's schema (`$defs.descriptor`; a release before them is named);
 *   2. its links are the references: a travel is read from the link, never from the number it states (a
 *      number edited by hand is stale), and a link moved reads the other travel;
 *   3. the descriptor and the extras never both say one thing: an extras file naming the plugin is
 *      refused, and so is a control with no link and no reason of its own, and an id out of order.
 *
 * The generated files the view gives are the ones committed: `node tools/gen.mjs --check` holds that.
 * `make descriptor-view`, part of `make test`; it reads omx-contract as `make test` does.
 */
import { cpSync, existsSync, mkdirSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { join } from 'node:path';
import { DESCRIPTOR_SCHEMA_FLOOR, descriptorFile, descriptorText } from '../descriptor.mjs';
import { ROOT, loadDecl, pluginDirs } from '../gen.mjs';
import { installedDataDir, locateContract } from '../omx-contract.mjs';
import { validate } from '../schema-validate.mjs';

let bad = 0, n = 0;
function check(what, got, want) {
  n++;
  const ok = JSON.stringify(got) === JSON.stringify(want);
  if (!ok) bad++;
  console.log(`${ok ? 'PASS' : 'FAIL'} descriptor-view ${what}${ok ? '' : `: got ${JSON.stringify(got)}, want ${JSON.stringify(want)}`}`);
}
const refusal = (f) => {
  try {
    f();
    return 'accepted';
  } catch (e) {
    return e.message.replace(/^.*?\.decl\.json: /, '');
  }
};

const described = pluginDirs().filter((dir) => existsSync(join(dir, descriptorFile(dir.split('/').pop()))));
check('at least one plugin is described', described.length > 0, true);
for (const dir of described) {
  const stem = dir.split('/').pop();
  check(`${stem}: the descriptor is what its declaration renders`, descriptorText(loadDecl(dir)) === readFileSync(join(dir, descriptorFile(stem)), 'utf8'), true);
}

// the descriptor words: the installed omx-contract's schema
const where = locateContract();
// beside the render read, else in the installed data directory (a union render carries no schema)
const schemaFile = [where.dir, installedDataDir()].filter(Boolean).map((d) => join(d, 'omx-contract.schema.json')).find((f) => existsSync(f));
const schema = schemaFile && existsSync(schemaFile) ? JSON.parse(readFileSync(schemaFile, 'utf8')) : undefined;
if (schema?.$defs?.descriptor) {
  for (const dir of described) {
    const stem = dir.split('/').pop();
    check(`${stem}: the descriptor holds to omx-contract's descriptor words`, validate(schema, schema.$defs.descriptor, JSON.parse(readFileSync(join(dir, descriptorFile(stem)), 'utf8'))), []);
  }
  const broken = JSON.parse(readFileSync(join(described[0], descriptorFile(described[0].split('/').pop())), 'utf8'));
  broken.controls[0].ids = ['a'];
  check('arm: a travel carrying choice ids breaks the descriptor words', validate(schema, schema.$defs.descriptor, broken).length > 0, true);
} else console.log(`NOTE descriptor-view: ${where.at ?? 'omx-contract'} predates the descriptor words (${DESCRIPTOR_SCHEMA_FLOOR}); its schema holds no descriptor`);

// the arms, on a copy of the first described plugin
const dir = described[0], stem = dir.split('/').pop();
const scratch = join(ROOT, 'build', 'tmp', 'descriptor-view');
const copy = join(scratch, stem);
const fresh = () => {
  rmSync(scratch, { recursive: true, force: true });
  mkdirSync(scratch, { recursive: true });
  cpSync(dir, copy, { recursive: true, filter: (src) => !src.includes('/build') });
};
const editJson = (name, f) => {
  const file = join(copy, name);
  const x = JSON.parse(readFileSync(file, 'utf8'));
  f(x);
  writeFileSync(file, `${JSON.stringify(x, null, 2)}\n`);
};
const DESC = descriptorFile(stem), DECL = `${stem}.decl.json`;
const travelled = JSON.parse(readFileSync(join(dir, DESC), 'utf8')).controls.find((c) => c.kind === 'travel' && c.source?.from);
const want = loadDecl(dir).params[travelled.id];

fresh();
editJson(DESC, (x) => { x.controls[travelled.id].travel.max += 1; });
const edited = loadDecl(copy);
check(`arm: ${travelled.symbol}'s max edited by hand is not read`, edited.params[travelled.id].max, want.max);
check(`arm: ${travelled.symbol}'s max edited by hand is stale`, descriptorText(edited) === readFileSync(join(copy, DESC), 'utf8'), false);

const other = JSON.parse(readFileSync(join(dir, DESC), 'utf8')).controls.find((c) => c.kind === 'travel' && c.source?.from && c.id !== travelled.id);
fresh();
editJson(DESC, (x) => { x.controls[travelled.id].source = other.source; });
const moved = loadDecl(copy).params[travelled.id];
check(`arm: ${travelled.symbol}'s link moved to ${other.symbol}'s reads that travel`, [moved.min, moved.max, moved.def], [loadDecl(dir).params[other.id].min, loadDecl(dir).params[other.id].max, loadDecl(dir).params[other.id].def]);

fresh();
editJson(DECL, (x) => { x.name = 'omx elsewhere'; });
check('arm: an extras file naming the plugin is refused', refusal(() => loadDecl(copy)), `"name" is the descriptor's to say (${DESC})`);

fresh();
editJson(DESC, (x) => { delete x.controls[travelled.id].source; });
check('arm: a control with no link and no reason of its own is refused', refusal(() => loadDecl(copy)), `'${travelled.symbol}' has no source link and no "own" reason`);

fresh();
editJson(DESC, (x) => { [x.controls[0], x.controls[1]] = [x.controls[1], x.controls[0]]; });
check('arm: a control out of the parameter order is refused', refusal(() => loadDecl(copy)), `the descriptor's control 0 has id 1; our ids are the parameter order`);

rmSync(scratch, { recursive: true, force: true });
console.log(`descriptor-view: ${n - bad}/${n} pass`);
process.exit(bad ? 1 : 0);
