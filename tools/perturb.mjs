#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * perturb.mjs <copied plugins/<stem>> <scratch dir> — move the first parameter's default where it
 * is DECLARED: in the copied declaration when it types its travels (a chain's first parameter is its
 * first element's switch, declared by that element's `on`), or in a copy of omx-contract's data when
 * it is by reference. Prints the environment the consumer must then read with
 * (`OMX_CONTRACT_DIR=<copy>`, or nothing). Exit 3: there is no parameter to move.
 */
import { existsSync, readFileSync, writeFileSync } from 'node:fs';
import { basename, join, resolve } from 'node:path';
import { descriptorFile, viewOf } from './descriptor.mjs';
import { RESOLVED, ROOT, locateContract, paramKernel, perturbCopy } from './omx-contract.mjs';
import { baseOf } from './variants.mjs';

const [pdir, scratch] = process.argv.slice(2);
// a variant's parameters are declared in its base (tools/variants.mjs), copied beside it
const own = join(pdir, `${basename(resolve(pdir))}.decl.json`);
const file = existsSync(own) ? own : (baseOf(pdir)?.file ?? own);
const raw = JSON.parse(readFileSync(file, 'utf8'));
// a plugin described in omx-contract's words: its parameters are its descriptor's controls
const descFile = join(pdir, descriptorFile(basename(resolve(pdir))));
const desc = existsSync(descFile) ? JSON.parse(readFileSync(descFile, 'utf8')) : undefined;
const d = desc ? viewOf(desc, raw, file) : raw;
if (d.binding === 'chain' && d.chain?.length) {
  raw.chain[0].on = !raw.chain[0].on;
  writeFileSync(file, `${JSON.stringify(raw, null, 2)}\n`);
  process.exit(0);
}
const p = d.params?.[0];
if (!p) process.exit(3);
if (p.ref === undefined) {
  if (desc) {
    // the face's own control: its travel is typed in the descriptor
    const c = desc.controls[0];
    if (c.kind === 'toggle') c.default = !c.default;
    else c.travel.default = c.travel.default === c.travel.max ? c.travel.min : c.travel.max;
    writeFileSync(descFile, `${JSON.stringify(desc, null, 2)}\n`);
  } else {
    p.def = p.def === p.max ? p.min : p.max;
    writeFileSync(file, `${JSON.stringify(d, null, 2)}\n`);
  }
} else if (p.defaultId !== undefined) {
  // a set whose default the declaration names (the contract names none): it moves there, to another id
  const where = locateContract(ROOT);
  if (!where.dir) {
    console.error(`perturb: ${where.why}`);
    process.exit(2);
  }
  const ids = JSON.parse(readFileSync(join(where.dir, RESOLVED), 'utf8')).items[p.ref]?.value;
  if (!Array.isArray(ids) || ids.length < 2) process.exit(3);
  const moved = ids.find((x) => x !== p.defaultId);
  if (desc) raw.controls[p.symbol].defaultId = moved;
  else p.defaultId = moved;
  writeFileSync(file, `${JSON.stringify(raw, null, 2)}\n`);
} else {
  const where = locateContract(ROOT);
  if (!where.dir) {
    console.error(`perturb: ${where.why}`);
    process.exit(2);
  }
  perturbCopy(where.dir, scratch, paramKernel(d, p, where.dir), p);
  console.log(`OMX_CONTRACT_DIR=${scratch}`);
}
