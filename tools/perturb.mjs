#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * perturb.mjs <copied plugins/<stem>> <scratch dir> — move the first parameter's default where it
 * is DECLARED: in the copied declaration when it types its travels, or in a copy of omx-contract's
 * data when it is by reference. Prints the environment the consumer must then read with
 * (`OMX_CONTRACT_DIR=<copy>`, or nothing). Exit 3: there is no parameter to move.
 */
import { readFileSync, writeFileSync } from 'node:fs';
import { basename, join, resolve } from 'node:path';
import { ROOT, locateContract, paramKernel, perturbCopy } from './omx-contract.mjs';

const [pdir, scratch] = process.argv.slice(2);
const file = join(pdir, `${basename(resolve(pdir))}.decl.json`);
const d = JSON.parse(readFileSync(file, 'utf8'));
const p = d.params?.[0];
if (!p) process.exit(3);
if (p.ref === undefined) {
  p.def = p.def === p.max ? p.min : p.max;
  writeFileSync(file, `${JSON.stringify(d, null, 2)}\n`);
} else {
  const where = locateContract(ROOT);
  if (!where.dir) {
    console.error(`perturb: ${where.why}`);
    process.exit(2);
  }
  perturbCopy(where.dir, scratch, paramKernel(d, p), p);
  console.log(`OMX_CONTRACT_DIR=${scratch}`);
}
