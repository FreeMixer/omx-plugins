#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * meter-words-check.mjs — every meter of every plugin descriptor states the contract's meter words `held` and `clip`
 * (spec 7.9), and they are the ones the generated face rows carry.
 *
 *   OMX_CONTRACT_SCHEMA=<omx-contract.schema.json> node tools/test/meter-words-check.mjs [--require]
 *
 * It waits on omx-contract's meter words (lane 18a, branch cluster/omx-contract-meter-words): while that schema's
 * descriptorMeter has no `held` and `clip` the check says it is pending and exits 0, and `--require` makes it exit 1
 * (the red it will be once the words land and the descriptors do not carry them). Not part of `make test` until then.
 */
import { readFileSync, readdirSync } from 'node:fs';
import { join } from 'node:path';
import { ROOT } from '../omx-contract.mjs';

const require_ = process.argv.includes('--require');
const schemaPath = process.env.OMX_CONTRACT_SCHEMA;
const props = schemaPath ? JSON.parse(readFileSync(schemaPath, 'utf8')).$defs?.descriptorMeter?.properties ?? {} : {};
if (!('held' in props && 'clip' in props)) {
  console.log(`pending meter-words: ${schemaPath ?? 'no OMX_CONTRACT_SCHEMA'} has no held and clip in descriptorMeter (omx-contract branch cluster/omx-contract-meter-words carries no commit past main)`);
  process.exit(require_ ? 1 : 0);
}
let bad = 0;
for (const stem of readdirSync(join(ROOT, 'plugins'))) {
  let d;
  try {
    d = JSON.parse(readFileSync(join(ROOT, 'plugins', stem, `${stem}.descriptor.json`), 'utf8'));
  } catch {
    continue;
  }
  for (const m of d.meters ?? []) {
    const ok = typeof m.held === 'boolean' && typeof m.clip === 'boolean';
    if (!ok) bad++, console.error(`FAIL ${stem}: meter ${m.symbol} has no boolean held and clip`);
  }
}
console.log(bad ? `FAIL meter-words: ${bad} meter(s) without the words` : 'ok meter-words: every descriptor meter states held and clip');
process.exit(bad ? 1 : 0);
