// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
//
// Reads one clap-validator JSON report and prints a line per test that failed or crashed:
//   node tools/clap-validate-summary.mjs <plugin> <report.json>
// Exit 1 when any test failed or crashed, or the report cannot be read; a report with no tests at
// all is unreadable, not clean.
import { readFileSync } from 'node:fs';

const [name, path] = process.argv.slice(2);
let report;
try {
  report = JSON.parse(readFileSync(path, 'utf8'));
} catch (e) {
  console.error(`FAIL ${name}: no readable report (${e.message})`);
  process.exit(1);
}
const counts = {};
const failures = [];
// report.results[]: {test: {<kind>: {test, ...}}, status: {code: "success"|"failed"|"crashed"|"skipped"|"warning", details}}
for (const r of Array.isArray(report.results) ? report.results : []) {
  const code = r?.status?.code;
  const [kind, t] = Object.entries(r?.test ?? {})[0] ?? ['?', {}];
  if (typeof code !== 'string') {
    console.error(`FAIL ${name}: a result has no status code`);
    process.exit(1);
  }
  counts[code] = (counts[code] ?? 0) + 1;
  if (code === 'failed' || code === 'crashed') failures.push(`${kind}/${t.test ?? '?'}: ${code}${r.status.details ? ': ' + String(r.status.details).replace(/\s+/g, ' ').slice(0, 160) : ''}`);
}
const total = Object.values(counts).reduce((a, b) => a + b, 0);
if (total === 0) {
  console.error(`FAIL ${name}: the report holds no test`);
  process.exit(1);
}
console.log(`${name}: ${Object.entries(counts).map(([k, v]) => `${v} ${k}`).join(', ')}`);
for (const f of failures) console.log(`  FAIL ${name} / ${f}`);
process.exit(failures.length ? 1 : 0);
