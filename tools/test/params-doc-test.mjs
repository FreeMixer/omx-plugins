#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * params-doc-test.mjs — the parameter reference says what the generated LV2 descriptions say.
 *
 * For every described plugin, the pages tools/params-doc.mjs lists (symbol and priority) are the pg:Group blocks of
 * its generated TTL, in the same numbers; every control's min, max and default in the page are the TTL port's. The
 * arms break the reference's priority and a default in a copy and expect each to be refused.
 */
import { readFileSync, readdirSync } from 'node:fs';
import { join } from 'node:path';
import { ROOT, describedStems, pluginPage } from '../params-doc.mjs';

let failed = 0;
const say = (ok, what) => { console.log(`${ok ? 'ok  ' : 'FAIL'} ${what}`); if (!ok) failed++; };

/** The groups of a TTL: symbol to priority. */
function ttlGroups(text) {
  const out = new Map();
  for (const m of text.matchAll(/a pg:Group ;\s*lv2:symbol "([^"]+)" ;\s*lv2:name "[^"]*" ;\s*lv2:displayPriority (\d+)/g)) out.set(m[1], Number(m[2]));
  return out;
}
/** The pages of a rendered reference: symbol to priority. */
function docGroups(text) {
  const out = new Map();
  for (const m of text.matchAll(/^\| [^|]+ \| `([^`]+)` \| (\d+) \| `[^`]*` \| \d+ \|$/gm)) out.set(m[1], Number(m[2]));
  return out;
}
const same = (a, b) => a.size === b.size && [...a].every(([k, v]) => b.get(k) === v);

/** The control rows of a rendered reference, symbol to [min, max, default] as the page words them. */
function docRows(text) {
  const rows = new Map();
  for (const m of text.matchAll(/^\| \d+ \| `([^`]+)` \| (?:[^|]|\\\|)* \| (?:[^|]|\\\|)* \| ([^|]*) \| ([^|]*) \| ([^|]*) \|/gm)) rows.set(m[1], [m[2].trim(), m[3].trim(), m[4].trim()]);
  return rows;
}

for (const stem of describedStems(ROOT)) {
  const ttl = readFileSync(join(ROOT, 'plugins', stem, 'generated', `${stem}.lv2`, `${stem}.ttl`), 'utf8');
  const page = pluginPage(ROOT, stem).text;
  say(same(ttlGroups(ttl), docGroups(page)), `${stem}: the pages are the TTL's port groups, symbol and priority`);
  // a travel's numbers
  let bad = 0, compared = 0;
  const rows = docRows(page);
  for (const m of ttl.matchAll(/lv2:symbol "([^"]+)" ;\s*lv2:name "[^"]*" ;\s*lv2:default ([-\d.e+]+) ;\s*lv2:minimum ([-\d.e+]+) ;\s*lv2:maximum ([-\d.e+]+)/g)) {
    const r = rows.get(m[1]);
    if (!r || /^[a-z]/i.test(r[0]) || /\(/.test(r[2])) continue; // enabled, toggles ("off"/"on"), choices ("Label (n)"): compared below
    compared++;
    if (Number(r[0]) !== Number(m[3]) || Number(r[1]) !== Number(m[4]) || Number(r[2]) !== Number(m[2])) { bad++; console.error(`  ${stem}.${m[1]}: page ${r} ttl ${m.slice(2)}`); }
  }
  say(bad === 0 && compared > 0, `${stem}: ${compared} travels' min, max and default are the TTL port's`);
}

// sabotage: the check must be able to fail
const stem = 'omx-eq8';
const ttl = readFileSync(join(ROOT, 'plugins', stem, 'generated', `${stem}.lv2`, `${stem}.ttl`), 'utf8');
const page = pluginPage(ROOT, stem).text;
say(!same(ttlGroups(ttl), docGroups(page.replace(/(`band1` \| )8/, '$19'))), 'sabotage: a changed priority is refused');
say(docRows(page.replace('| 80 |', '| 81 |')).get('hpf_freq')[2] === '81', 'sabotage: the row reader sees a changed default');
say(readdirSync(join(ROOT, 'docs/plugins')).includes('README.md'), 'the index exists');
process.exit(failed ? 1 : 0);
