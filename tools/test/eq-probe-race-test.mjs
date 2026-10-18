#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * eq-probe-race-test.mjs — build/eq-default-probe (tools/omx-contract.mjs eqDefaultBands) is run whole
 * while other processes build it. `make -j` runs several node tools at once and each compiles the probe
 * for itself; compiled in place, one process ran the file while another's compiler still had it open
 * (ETXTBSY) or before it was executable (EACCES), and rpmbuild's %check went red (#195). Here a fake
 * compiler opens its output, writes the front of it, waits, writes the rest and only then makes it
 * executable (the window a real cc has), and several readers start a little apart: every one must
 * read the bands, and no temporary build may be left behind.
 *
 * Nothing here needs a compiler, omx-dsp or omx-contract: a fake pkg-config prints no flags, the fake
 * compiler writes a shell script that prints two bands. `make eq-probe-race`, part of `make test` and
 * `make test-faces`.
 */
import { spawn } from 'node:child_process';
import { chmodSync, mkdirSync, mkdtempSync, readdirSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { fileURLToPath } from 'node:url';

const HERE = fileURLToPath(import.meta.url);
const LIB = join(HERE, '..', '..', 'omx-contract.mjs');
const READERS = 3;
const BUILD_MS = 400;
const STAGGER_MS = 200;

/** A throwaway root: a resolved render with the EQ items, a fake pkg-config, a fake compiler. */
function scratch() {
  const root = mkdtempSync(join(tmpdir(), 'eq-probe-race-'));
  const data = join(root, 'data');
  mkdirSync(data);
  writeFileSync(join(data, 'omx-contract.json'), JSON.stringify({
    version: '2.16.0',
    kernels: {},
    items: {
      EQ_BAND_TYPES: { kind: 'list', value: ['bell', 'low-shelf'] },
      EQ_BAND_DEFAULTS: { kind: 'sheet', value: { gainDb: 0, q: 0.7 } },
    },
  }));
  const pkg = join(root, 'pkg-config');
  writeFileSync(pkg, '#!/bin/sh\necho\n');
  chmodSync(pkg, 0o755);
  const cc = join(root, 'fake-cc.mjs');
  writeFileSync(cc, `#!/usr/bin/env node
import { chmodSync, closeSync, openSync, writeSync } from 'node:fs';
const out = process.argv[process.argv.indexOf('-o') + 1];
const body = '#!/bin/sh\\necho "1 100"\\necho "0 1000"\\n';
const fd = openSync(out, 'w', 0o644);
writeSync(fd, body.slice(0, 12));
await new Promise((r) => setTimeout(r, ${BUILD_MS}));
writeSync(fd, body.slice(12));
closeSync(fd);
chmodSync(out, 0o755);
`);
  chmodSync(cc, 0o755);
  return { root, data, pkg, cc };
}

/** One reader process: the default bands of a two-band EQ, the way gen.mjs asks for them. */
function reader(s) {
  const code = `import { eqDefaultBands } from ${JSON.stringify(`file://${LIB}`)};
console.log(JSON.stringify(eqDefaultBands(${JSON.stringify(s.data)}, 2, ${JSON.stringify(s.root)})));`;
  return new Promise((resolve) => {
    const p = spawn(process.execPath, ['--input-type=module', '-e', code], {
      env: { ...process.env, PKG_CONFIG: s.pkg, CC: s.cc },
      stdio: ['ignore', 'pipe', 'pipe'],
    });
    let out = '', err = '';
    p.stdout.on('data', (d) => (out += d));
    p.stderr.on('data', (d) => (err += d));
    p.on('close', (rc) => resolve({ rc, out: out.trim(), err: err.trim().split('\n').find((l) => /Error|code:/.test(l)) ?? err.trim().split('\n')[0] }));
  });
}

const s = scratch();
const runs = [];
for (let i = 0; i < READERS; i++) {
  runs.push(reader(s));
  await new Promise((r) => setTimeout(r, STAGGER_MS));
}
const got = await Promise.all(runs);

let bad = 0;
const check = (what, ok, detail = '') => {
  if (!ok) bad++;
  console.log(`${ok ? 'PASS' : 'FAIL'} eq-probe-race ${what}${ok ? '' : `: ${detail}`}`);
};
const want = JSON.stringify([
  { type: 'low-shelf', freqHz: 100, gainDb: 0, q: 0.7 },
  { type: 'bell', freqHz: 1000, gainDb: 0, q: 0.7 },
]);
got.forEach((g, i) => check(`reader ${i + 1} of ${READERS} reads the bands while the others build the probe`, g.rc === 0 && g.out === want, `rc ${g.rc}, out ${JSON.stringify(g.out)}, ${g.err}`));
const left = readdirSync(join(s.root, 'build')).filter((n) => n !== 'eq-default-probe');
check('no temporary build is left behind', left.length === 0, left.join(', '));

if (!process.env.EQ_PROBE_KEEP) rmSync(s.root, { recursive: true, force: true }); else console.log(s.root);
console.log(bad ? `${bad} FAILED` : 'eq-probe-race: all pass');
process.exit(bad ? 1 : 0);
