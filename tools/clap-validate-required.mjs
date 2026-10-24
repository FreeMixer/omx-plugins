// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
//
// The clap-validator tests CI requires, over the reports tools/clap-validate.sh wrote:
//   node tools/clap-validate-required.mjs <reports dir> <test-name prefix>...
// Exit 1 when a test whose name starts with one of the prefixes failed or crashed in any report, or
// when no report holds a passing test of a prefix (a gate that judged nothing is not green).
import { readdirSync, readFileSync } from 'node:fs';
import { join } from 'node:path';

const [dir, ...prefixes] = process.argv.slice(2);
if (!dir || prefixes.length === 0) {
  console.error('usage: node tools/clap-validate-required.mjs <reports dir> <test-name prefix>...');
  process.exit(2);
}
const passed = Object.fromEntries(prefixes.map((p) => [p, 0]));
let bad = 0;
for (const f of readdirSync(dir).filter((f) => f.endsWith('.json')).sort()) {
  let report;
  try {
    report = JSON.parse(readFileSync(join(dir, f), 'utf8'));
  } catch (e) {
    console.log(`FAIL ${f}: no readable report (${e.message})`);
    bad = 1;
    continue;
  }
  for (const r of Array.isArray(report.results) ? report.results : []) {
    const t = Object.values(r?.test ?? {})[0]?.test ?? '';
    const p = prefixes.find((x) => t.startsWith(x));
    if (!p) continue;
    const code = r?.status?.code;
    if (code === 'success') passed[p]++;
    else if (code === 'failed' || code === 'crashed') {
      console.log(`FAIL ${f.replace(/\.json$/, '')} / ${t}: ${code}`);
      bad = 1;
    }
  }
}
for (const [p, n] of Object.entries(passed)) {
  if (n === 0) {
    console.log(`FAIL no report holds a passing ${p}* test`);
    bad = 1;
  } else console.log(`required ${p}*: ${n} passed`);
}
process.exit(bad);
