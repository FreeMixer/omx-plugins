#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * modgui-import-test.mjs — tools/modgui-import.mjs. Over a fixture modgui bundle of our own
 * (tools/test/fixtures/modgui, no MOD art), the package it writes parses under its own §6.3
 * schema check, its labels and order equal the bundle's, its files are the bundle's byte for
 * byte, and every refusal code the declaration lists is produced by a bundle that earns it, with
 * nothing written. `make modgui-import`, part of `make test`.
 */
import { spawnSync } from 'node:child_process';
import { cpSync, existsSync, mkdtempSync, readdirSync, readFileSync, rmSync, symlinkSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { dirname, join, relative } from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';
import {
  importModgui,
  LV2_NS,
  MODGUI_IMPORT_REFUSALS,
  MODGUI_NS,
  SKIN_LICENCE_ALLOW_LIST,
  SKIN_OVERRIDE_FILE,
  isSkinLicenceAllowed,
  parseSkinOverride,
  skinPackageDirName,
  spdxFromDoapLicence,
} from '../modgui-import.mjs';
import { parseTurtle } from '../turtle-read.mjs';

const HERE = dirname(fileURLToPath(import.meta.url));
const FIXTURE = join(HERE, 'fixtures', 'modgui', 'omx-fixture-echo.lv2');
const PLUGIN = 'urn:openmixer:fixture:echo';
const TOOL = join(HERE, '..', 'modgui-import.mjs');

let n = 0, bad = 0;
function check(what, got, want) {
  n++;
  const ok = JSON.stringify(got) === JSON.stringify(want);
  if (!ok) bad++;
  console.log(`${ok ? 'PASS' : 'FAIL'} modgui-import ${what}${ok ? '' : `: got ${JSON.stringify(got)}, want ${JSON.stringify(want)}`}`);
}

function checkTrue(what, got) {
  check(what, got, true);
}

const scratch = mkdtempSync(join(tmpdir(), 'omx-modgui-import-'));
let fresh_n = 0;
const fresh = (name) => join(scratch, `${name}-${fresh_n++}`);

/** A copy of the fixture with one file rewritten, so each refusal is earned by one edit; an edit that does not land is caught. */
function edited(file, edit) {
  const b = fresh('bundle');
  cpSync(FIXTURE, b, { recursive: true });
  if (edit) {
    const before = readFileSync(join(b, file), 'utf8');
    const after = edit(before);
    if (after === before) throw new Error(`the edit of ${file} did not land`);
    writeFileSync(join(b, file), after);
  }
  return b;
}

/** Every file under `dir`, relative, sorted. */
function files(dir) {
  return readdirSync(dir, { recursive: true, withFileTypes: true })
    .filter((d) => d.isFile())
    .map((d) => relative(dir, join(d.parentPath, d.name)))
    .sort();
}

/** The fixture's own ports, read from its modgui.ttl by the reader: the bundle's facts, not the test's. */
function bundlePorts() {
  const path = join(FIXTURE, 'modgui.ttl');
  const t = parseTurtle(readFileSync(path, 'utf8'), pathToFileURL(path).href);
  const of = (s, p) => t.find((x) => x.s.v === s && x.p.v === `${LV2_NS}${p}`).o.v;
  return t.filter((x) => x.p.v === `${MODGUI_NS}port`).map((x) => ({ index: Number(of(x.o.v, 'index')), symbol: of(x.o.v, 'symbol'), name: of(x.o.v, 'name') }));
}

const produced = new Set();
async function refused(bundle, code, opts = {}) {
  const out = opts.out ?? fresh('out');
  const before = existsSync(out) ? files(out) : [];
  let caught;
  try {
    await importModgui({ bundle, out, plugin: opts.plugin });
  } catch (e) {
    caught = e;
  }
  check(`${code}: refused`, caught?.code, code);
  // Nothing written: the out directory holds exactly what it held before, and no temporary.
  const after = existsSync(out) ? files(out) : [];
  check(`${code}: nothing written`, after, before);
  produced.add(code);
}

// ─── the SPDX map, by value, including the deprecated GNU spellings ───────────────────────────

for (const [lit, want] of [
  ['GPL-2.0+', 'GPL-2.0-or-later'],
  ['GPL-3.0+', 'GPL-3.0-or-later'],
  ['LGPL-2.1+', 'LGPL-2.1-or-later'],
  ['LGPL-3.0+', 'LGPL-3.0-or-later'],
  ['GPL-2.0', 'GPL-2.0-only'],
  ['GPL-3.0', 'GPL-3.0-only'],
  ['LGPL-2.1', 'LGPL-2.1-only'],
  ['LGPL-3.0', 'LGPL-3.0-only'],
  ['MIT', 'MIT'],
]) check(`the deprecated SPDX literal '${lit}' reads as ${want}`, spdxFromDoapLicence({ literal: lit }), want);

check('the unversioned DOAP gpl IRI reads as GPL-2.0-or-later', spdxFromDoapLicence({ iri: 'http://usefulinc.com/doap/licenses/gpl' }), 'GPL-2.0-or-later');
check('the unversioned DOAP lgpl IRI reads as LGPL-2.1-or-later', spdxFromDoapLicence({ iri: 'http://usefulinc.com/doap/licenses/lgpl' }), 'LGPL-2.1-or-later');
check('an spdx.org IRI reads as its id', spdxFromDoapLicence({ iri: 'http://spdx.org/licenses/MIT' }), 'MIT');
check('an opensource.org IRI reads as its id', spdxFromDoapLicence({ iri: 'https://opensource.org/licenses/BSD-3-Clause' }), 'BSD-3-Clause');
check('an unrecognised IRI maps to no SPDX id', spdxFromDoapLicence({ iri: 'http://example.org/our-terms' }), null);

checkTrue('GPL-2.0-only is a real SPDX id outside the allow-list (unreusable)', !isSkinLicenceAllowed('GPL-2.0-only'));
checkTrue('GPL-2.0-or-later is on the allow-list', isSkinLicenceAllowed('GPL-2.0-or-later'));

// ─── the fixture bundle becomes a §6 skin package ─────────────────────────────────────────────

{
  const out = fresh('out');
  const r = await importModgui({ bundle: FIXTURE, out });
  const pkg = join(out, skinPackageDirName(PLUGIN));
  const text = readFileSync(join(pkg, SKIN_OVERRIDE_FILE), 'utf8');
  const parsed = parseSkinOverride(text);

  check('writes skin.json under the §6.1 name', r.dir, pkg);
  checkTrue('the §6.1 name percent-encodes the plugin urn', pkg.endsWith('urn%3Aopenmixer%3Afixture%3Aecho'));
  check('skin.json parses under §6.3', parsed.ok, true);

  const ports = bundlePorts().sort((a, b) => a.index - b.index);
  check('its control order is the bundle lv2:index order', parsed.file.sections, [{ key: 'modgui', label: 'Echo', controls: ports.map((p) => p.symbol) }]);
  check('the order is time, feedback, tone, mix', parsed.file.sections[0].controls, ['time', 'feedback', 'tone', 'mix']);
  check('its labels are the bundle lv2:name', parsed.file.labels, Object.fromEntries(ports.map((p) => [p.symbol, { label: p.name }])));
  check('feedback is labelled Repeats', parsed.file.labels.feedback, { label: 'Repeats' });

  check('records the plugin id', parsed.file.plugin, PLUGIN);
  check('records the licence it found', parsed.file.licence, 'GPL-3.0-or-later');
  check('records where it read the licence', parsed.file.source, { kind: 'modgui', bundle: FIXTURE, licenceEvidence: 'echo.ttl doap:license <http://spdx.org/licenses/GPL-3.0-or-later>' });

  const src = join(FIXTURE, 'modgui');
  check('copies the modgui whole, plus its TTL', files(join(pkg, 'modgui')), [...files(src), 'modgui.ttl'].sort());
  let copiedByte = true;
  for (const f of files(src)) if (!readFileSync(join(pkg, 'modgui', f)).equals(readFileSync(join(src, f)))) copiedByte = false;
  checkTrue('every modgui file is copied byte for byte', copiedByte);
  check('the TTL is copied byte for byte', readFileSync(join(pkg, 'modgui/modgui.ttl'), 'utf8'), readFileSync(join(FIXTURE, 'modgui.ttl'), 'utf8'));
  check('the screenshot is the backdrop', parsed.file.backdrop, 'art/screenshot-echo.png');
  checkTrue('the backdrop is copied byte for byte', readFileSync(join(pkg, parsed.file.backdrop)).equals(readFileSync(join(src, 'screenshot-echo.png'))));

  check('writes no view, family or roles', Object.keys(parsed.file).sort(), ['backdrop', 'labels', 'licence', 'plugin', 'schema', 'sections', 'source']);
  checkTrue('no view directory is written', !existsSync(join(pkg, 'view')));
  check('the roles its symbols suggest are returned, not written', r.suggestedRoles, { family: 'delay', roles: { time: 'time', feedback: 'feedback', tone: 'tone', mix: 'mix' }, unmatched: [] });
  check('the brand is read from modgui:brand', r.brand, 'openmixer');
}

// ─── refusals name the code and the file, and write nothing ──────────────────────────────────

await refused(edited('echo.ttl', (t) => t.replace(/^\s*doap:license .*$/m, '')), 'MODGUI_NO_LICENCE');

{
  checkTrue('GPL-2.0-only is outside the allow-list', !SKIN_LICENCE_ALLOW_LIST.includes('GPL-2.0-only'));
  const b = edited('echo.ttl', (t) => t.replace('licenses/GPL-3.0-or-later>', 'licenses/GPL-2.0-only>'));
  let caught;
  try {
    await importModgui({ bundle: b, out: fresh('out') });
  } catch (e) {
    caught = e;
  }
  check('MODGUI_LICENCE_REFUSED: a real unreusable SPDX string names the code', caught?.code, 'MODGUI_LICENCE_REFUSED');
  check('MODGUI_LICENCE_REFUSED: names the file', caught?.file, join(b, 'echo.ttl'));
  checkTrue('MODGUI_LICENCE_REFUSED: the detail names GPL-2.0-only', caught?.detail.includes('GPL-2.0-only'));
  await refused(b, 'MODGUI_LICENCE_REFUSED');
}

await refused(edited('echo.ttl', (t) => t.replace('http://spdx.org/licenses/GPL-3.0-or-later', 'http://example.org/our-terms')), 'MODGUI_LICENCE_UNMAPPED');

await refused(edited('modgui.ttl', (t) => t.replace('modgui:brand', 'modgui:javascript <modgui/icon-echo.html> ;\n        modgui:brand')), 'MODGUI_UNKNOWN_PROPERTY');
await refused(edited('modgui.ttl', (t) => t.replace('lv2:name "Blend" ;', 'lv2:name "Blend" ;\n            lv2:default 0.5 ;')), 'MODGUI_UNKNOWN_PROPERTY');

await refused(edited('modgui.ttl', (t) => t.replace('<modgui/thumbnail-echo.png>', '<file:///etc/hostname>')), 'MODGUI_PATH_ESCAPE');
{
  const b = edited('modgui.ttl');
  symlinkSync(tmpdir(), join(b, 'modgui', 'escape'));
  await refused(b, 'MODGUI_PATH_ESCAPE');
}
await refused(edited('modgui.ttl', (t) => t.replace('<modgui/screenshot-echo.png>', '<../../screenshot.png>')), 'MODGUI_NOT_FOUND');

await refused(edited('modgui.ttl', (t) => t.replace('modgui:brand "openmixer" ;', 'modgui:brand "openmixer ;')), 'MODGUI_TTL_INVALID');

{
  const b = edited('manifest.ttl');
  rmSync(join(b, 'modgui.ttl'));
  await refused(b, 'MODGUI_NOT_FOUND');
}

{
  const out = fresh('out');
  await importModgui({ bundle: FIXTURE, out });
  const skin = join(out, skinPackageDirName(PLUGIN), 'skin.json');
  writeFileSync(skin, 'the operator edited this');
  await refused(FIXTURE, 'MODGUI_TARGET_EXISTS', { out });
  check('an existing package is never overwritten', readFileSync(skin, 'utf8'), 'the operator edited this');
}

{
  const b = edited('echo.ttl', (t) => t.replace('licenses/GPL-3.0-or-later>', 'licenses/GPL-2.0-only>'));
  const out = fresh('out');
  const run = spawnSync('node', [TOOL, b, '--out', out], { encoding: 'utf8' });
  check('the CLI exits 3 on a refusal', run.status, 3);
  const line = run.stdout.split('\n').find((l) => l.startsWith('MODGUI-IMPORT-JSON '));
  const parsed = line && JSON.parse(line.slice('MODGUI-IMPORT-JSON '.length));
  check('the CLI prints the refusal as one JSON line', parsed && { verdict: parsed.verdict, code: parsed.code, file: parsed.file }, { verdict: 'refused', code: 'MODGUI_LICENCE_REFUSED', file: join(b, 'echo.ttl') });
  checkTrue('the CLI writes nothing on a refusal', !existsSync(out));
}

check('every refusal code the declaration lists is produced above', [...produced].sort(), [...MODGUI_IMPORT_REFUSALS].sort());

rmSync(scratch, { recursive: true, force: true });
console.log(`modgui-import: ${n - bad}/${n} pass`);
process.exit(bad ? 1 : 0);
