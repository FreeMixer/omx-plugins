#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * meter-values-check.mjs <got> <want> — the meters that are not level meters (a gain reduction), read
 * by tools/test/meter-level-digest.c --others, against the values committed in
 * tools/test/<face>.meter-others.txt. Each line is `<block> <meter> <value>`. The same lines must be
 * there in the same order, and each value must be within TOLERANCE_DB of the committed one: a gain
 * reduction is computed in float through libm's float functions, and C libraries round those last bits
 * differently (glibc 2.36 and 2.41 give readings 2e-6 dB apart), so the check tests the value, not its
 * bits. A NaN must stay a NaN, an infinity the same infinity, and `none` (no value that block) `none`.
 *
 *   meter-values-check.mjs --self-test   a copy passes, a move under the tolerance passes, and a move
 *                                        over it, a missing line, a NaN and a moved block each go red
 * Exit: 0 the values agree, 1 they do not, 2 a file could not be read.
 */
import { mkdtempSync, readFileSync, rmSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";

/** dB: 50 times the toolchain spread measured, far below a meter's displayed resolution. */
export const TOLERANCE_DB = 1e-4;

function lines(path) {
  return readFileSync(path, "utf8").split("\n").filter((l) => l !== "" && !l.startsWith("#"));
}

/** printf's %g spelling as a number: nan, -nan, inf and -inf as well. */
function num(text) {
  if (/^[-+]?nan$/i.test(text)) return NaN;
  if (/^\+?inf$/i.test(text)) return Infinity;
  if (/^-inf$/i.test(text)) return -Infinity;
  return Number(text);
}

/** The first disagreement, or null. */
export function compare(got, want, tol = TOLERANCE_DB) {
  if (got.length !== want.length) return `${got.length} values, ${want.length} committed`;
  for (let i = 0; i < want.length; i++) {
    const [gb, gm, gv] = got[i].split(" ");
    const [wb, wm, wv] = want[i].split(" ");
    const at = `line ${i + 1} (block ${wb}, meter ${wm})`;
    if (gb !== wb || gm !== wm) return `${at}: read block ${gb}, meter ${gm}`;
    if (wv === "none" || gv === "none") {
      if (gv !== wv) return `${at}: ${gv}, committed ${wv}`;
      continue;
    }
    const g = num(gv), w = num(wv);
    if (Number.isNaN(w) || Number.isNaN(g)) {
      if (!(Number.isNaN(w) && Number.isNaN(g))) return `${at}: ${gv}, committed ${wv}`;
      continue;
    }
    if (!Number.isFinite(w) || !Number.isFinite(g)) {
      if (g !== w) return `${at}: ${gv}, committed ${wv}`;
      continue;
    }
    if (Math.abs(g - w) > tol) return `${at}: ${gv}, committed ${wv}, ${Math.abs(g - w)} dB apart (tolerance ${tol})`;
  }
  return null;
}

function selfTest() {
  const dir = mkdtempSync(join(tmpdir(), "meter-values-"));
  try {
    const want = ["0 0 0", "1 0 -2.96562195", "2 0 nan", "3 0 none", "4 0 -inf"];
    const cases = [
      ["a copy passes", want, true],
      ["a move under the tolerance passes", ["0 0 0", "1 0 -2.96562219", "2 0 nan", "3 0 none", "4 0 -inf"], true],
      ["a move over the tolerance is red", ["0 0 0", "1 0 -2.96582195", "2 0 nan", "3 0 none", "4 0 -inf"], false],
      ["a missing line is red", want.slice(0, 4), false],
      ["a NaN where a number was is red", ["0 0 nan", ...want.slice(1)], false],
      ["a number where a NaN was is red", ["0 0 0", "1 0 -2.96562195", "2 0 0", "3 0 none", "4 0 -inf"], false],
      ["a value where none was is red", ["0 0 0", "1 0 -2.96562195", "2 0 nan", "3 0 0", "4 0 -inf"], false],
      ["+inf where -inf was is red", ["0 0 0", "1 0 -2.96562195", "2 0 nan", "3 0 none", "4 0 inf"], false],
      ["a moved block is red", ["0 0 0", "2 0 -2.96562195", "2 0 nan", "3 0 none", "4 0 -inf"], false],
    ];
    let fail = 0;
    for (const [what, got, pass] of cases) {
      writeFileSync(join(dir, "got"), `${got.join("\n")}\n`);
      writeFileSync(join(dir, "want"), `${want.join("\n")}\n`);
      const r = compare(lines(join(dir, "got")), lines(join(dir, "want")));
      if ((r === null) === pass) console.log(`ok   ${what}`);
      else { console.log(`FAIL ${what}: ${r ?? "passed"}`); fail = 1; }
    }
    if (fail === 0) console.log("meter-values-check self-test: every case as wanted");
    return fail;
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
}

const args = process.argv.slice(2);
if (args[0] === "--self-test") process.exit(selfTest());
if (args.length !== 2) {
  console.error("usage: meter-values-check.mjs <got> <want> | --self-test");
  process.exit(2);
}
let got, want;
try {
  got = lines(args[0]);
  want = lines(args[1]);
} catch (e) {
  console.error(`meter-values-check: ${e.message}`);
  process.exit(2);
}
const r = compare(got, want);
if (r !== null) {
  console.error(`FAIL meter-values ${args[1]}: ${r}`);
  process.exit(1);
}
console.log(`ok   meter-values ${args[1]}: ${want.length} values within ${TOLERANCE_DB} dB`);
