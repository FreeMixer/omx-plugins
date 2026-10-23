#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * modgui-import.mjs — a MOD modgui bundle in, a skin package out: the console's skin package
 * format, read in reverse from the format tools/modgui-gen.mjs writes.
 *
 * The format read is MOD Devices' modgui, as MOD documents it (the `modgui:` vocabulary,
 * http://moddevices.com/ns/modgui#, in mod-sdk and the MOD wiki's "MOD SDK / modgui" pages): a
 * `modgui:gui` node beside an LV2 plugin naming its resources directory, icon template,
 * stylesheet, screenshot, thumbnail, brand, label, model, panel, colour and knob, and one
 * `modgui:port` per control keyed by `lv2:symbol`. Credit for the format and every skin it reads
 * is MOD Devices' and each bundle's authors'; this tool copies their files only under a licence
 * that permits it, and records that licence in the package.
 *
 * The package (§6.2, §6.3): `skin.json` with the plugin, the licence and where it came from, one
 * section in `lv2:index` order, each port's `lv2:name` as its label, the screenshot as the
 * backdrop under `art/`; the modgui's whole resources directory and its TTL under `modgui/`. No
 * custom view (§6.6: a modgui icon template is not a component of the skin API), no `family` and
 * no `roles` (a contract unit a modgui does not carry): the roles the port symbols suggest are
 * PRINTED for the operator to confirm.
 *
 * The licence map, the modgui vocabulary and the §6.3 override schema are this file's own copy
 * (omx-plugins carries no dependency on the console's library); the console keeps its own for its
 * loader, the two kept in step by hand, one issue each.
 *
 *   node tools/modgui-import.mjs <bundle.lv2> --out <skins dir> [--plugin <uri>]
 *
 * prints one `MODGUI-IMPORT-JSON {…}` line; exit 0 on a package written, 3 on a refusal, 2 on usage.
 */
import { cpSync, existsSync, mkdirSync, readdirSync, readFileSync, realpathSync, renameSync, rmSync, statSync, writeFileSync } from 'node:fs';
import { basename, join, relative, resolve, sep } from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';
import { parseTurtle } from './turtle-read.mjs';

const RDF_TYPE = 'http://www.w3.org/1999/02/22-rdf-syntax-ns#type';
const LV2_PROJECT = 'http://lv2plug.in/ns/lv2core#project';

export const MODGUI_NS = 'http://moddevices.com/ns/modgui#';
export const LV2_NS = 'http://lv2plug.in/ns/lv2core#';
export const DOAP_NS = 'http://usefulinc.com/ns/doap#';

/** The `modgui:` properties the importer knows on a `modgui:gui` node; any other is refused. */
export const MODGUI_GUI_PROPERTIES = ['resourcesDirectory', 'iconTemplate', 'stylesheet', 'screenshot', 'thumbnail', 'brand', 'label', 'model', 'panel', 'color', 'knob', 'port'];

/** The `lv2:` properties the importer knows on a `modgui:port` node; any other is refused. */
export const MODGUI_PORT_PROPERTIES = ['index', 'symbol', 'name'];

/** The importer's refusals (§6.6), each naming the file it read; nothing is written on any of them. */
export const MODGUI_IMPORT_REFUSALS = [
  'MODGUI_NOT_FOUND',
  'MODGUI_TTL_INVALID',
  'MODGUI_NO_LICENCE',
  'MODGUI_LICENCE_UNMAPPED',
  'MODGUI_LICENCE_REFUSED',
  'MODGUI_UNKNOWN_PROPERTY',
  'MODGUI_PATH_ESCAPE',
  'MODGUI_TARGET_EXISTS',
];

/** The licences an imported modgui may carry (§12 Q6). GPL-2.0-only and every other licence is refused until the operator adds it here. */
export const SKIN_LICENCE_ALLOW_LIST = ['GPL-2.0-or-later', 'GPL-3.0-or-later', 'LGPL-2.1-or-later', 'LGPL-3.0-or-later', 'MIT', 'BSD-2-Clause', 'BSD-3-Clause', 'ISC', 'Zlib', 'CC0-1.0', 'CC-BY-4.0'];

export const isSkinLicenceAllowed = (spdx) => SKIN_LICENCE_ALLOW_LIST.includes(spdx);

/** DOAP's unversioned licence IRIs and the SPDX id each is read as (an unversioned GPL lets the recipient choose any published version). */
const DOAP_UNVERSIONED = {
  'http://usefulinc.com/doap/licenses/gpl': 'GPL-2.0-or-later',
  'http://usefulinc.com/doap/licenses/lgpl': 'LGPL-2.1-or-later',
};

/** The deprecated `+` and bare-version SPDX spellings, each read as its current id. */
const SPDX_DEPRECATED = {
  'GPL-2.0+': 'GPL-2.0-or-later',
  'GPL-3.0+': 'GPL-3.0-or-later',
  'LGPL-2.1+': 'LGPL-2.1-or-later',
  'LGPL-3.0+': 'LGPL-3.0-or-later',
  'GPL-2.0': 'GPL-2.0-only',
  'GPL-3.0': 'GPL-3.0-only',
  'LGPL-2.1': 'LGPL-2.1-only',
  'LGPL-3.0': 'LGPL-3.0-only',
};

/** An SPDX id as written, with the deprecated `+` and bare-version forms read as their current id. */
function spdxId(id) {
  const known = [...SKIN_LICENCE_ALLOW_LIST, ...Object.keys(SPDX_DEPRECATED)].find((k) => k.toLowerCase() === id.toLowerCase());
  const canon = known ?? id;
  return SPDX_DEPRECATED[canon] ?? canon;
}

/**
 * The SPDX expression a `doap:license` value names, or `null` when it names none this map reads
 * (§6.6): an `spdx.org/licenses/<id>` IRI or a bare SPDX literal as that id, an
 * `opensource.org/licenses/<id>` IRI as the SPDX id it names, DOAP's unversioned gpl/lgpl as
 * GPL-2.0-or-later/LGPL-2.1-or-later. Mapping is not permission: {@link isSkinLicenceAllowed} decides.
 */
export function spdxFromDoapLicence(value) {
  if (value.literal !== undefined) {
    const lit = value.literal.trim();
    return /^[A-Za-z0-9.+-]+$/.test(lit) ? spdxId(lit) : null;
  }
  const iri = value.iri;
  if (iri === undefined) return null;
  const doap = DOAP_UNVERSIONED[iri.replace(/^https:/, 'http:').replace(/\/$/, '')];
  if (doap) return doap;
  const m = /^https?:\/\/(?:www\.)?(spdx\.org|opensource\.org)\/licenses\/([A-Za-z0-9.+-]+?)(?:\.html|\.php)?\/?$/.exec(iri);
  return m ? spdxId(m[2]) : null;
}

// ─── §6.1 the directory and §6.3 the override file ────────────────────────────────────────────

const KEPT_BYTE = /^[A-Za-z0-9._-]$/;

/**
 * The directory name of a plugin id's skin package: the id as it is when it holds only
 * `[A-Za-z0-9._-]`, else percent-encoded byte by byte (uppercase hex). `''`, `.` and `..` are
 * never a name and throw.
 */
export function skinPackageDirName(pluginId) {
  let out = '';
  for (const byte of new TextEncoder().encode(pluginId)) {
    const ch = String.fromCharCode(byte);
    out += byte < 0x80 && KEPT_BYTE.test(ch) ? ch : `%${byte.toString(16).toUpperCase().padStart(2, '0')}`;
  }
  if (out === '' || out === '.' || out === '..') throw new Error(`skinPackageDirName: '${pluginId}' is not a package name`);
  return out;
}

export const SKIN_SCHEMA_VERSION = 1;
export const SKIN_OVERRIDE_FILE = 'skin.json';
const SKIN_SOURCE_KINDS = ['own', 'generated', 'modgui'];
const OVERRIDE_FIELDS = new Set(['schema', 'plugin', 'licence', 'source', 'fingerprint', 'family', 'roles', 'sections', 'labels', 'visibleWhen', 'displays', 'houseView', 'look', 'backdrop', 'view']);
const ROLE_KEY = /^[a-z][a-z_]*(\[\d+\])?$/;
const isPackagePath = (p) => p.split('/').every((s) => s !== '' && s !== '.' && s !== '..');
const isObj = (v) => typeof v === 'object' && v !== null && !Array.isArray(v);
const isStr = (v) => typeof v === 'string' && v.length > 0;
const optStr = (v) => v === undefined || isStr(v);
const isLabel = (v) => isObj(v) && optStr(v.labelKey) && optStr(v.label);

/** The first schema violation of an override object, or `null` when it conforms to §6.3. */
function overrideViolation(o) {
  if (!isObj(o)) return 'not an object';
  for (const k of Object.keys(o)) if (!OVERRIDE_FIELDS.has(k)) return `unknown field '${k}'`;
  if (o.schema !== SKIN_SCHEMA_VERSION) return `schema is ${JSON.stringify(o.schema)}, not ${SKIN_SCHEMA_VERSION}`;
  if (!isStr(o.plugin)) return 'plugin is not a string';
  if (!isStr(o.licence)) return 'licence is required';
  const s = o.source;
  if (!isObj(s) || !SKIN_SOURCE_KINDS.includes(s.kind)) return 'source.kind is not own|generated|modgui';
  for (const k of Object.keys(s)) if (!['kind', 'bundle', 'licenceEvidence'].includes(k)) return `unknown field 'source.${k}'`;
  if (!optStr(s.bundle) || !optStr(s.licenceEvidence)) return 'source.bundle/licenceEvidence is not a string';
  if (!optStr(o.fingerprint)) return 'fingerprint is not a string';
  if (o.family !== undefined && !isPanelFamilyName(o.family)) return `family '${String(o.family)}' is not a family`;
  if (o.houseView !== undefined && !isPanelFamilyName(o.houseView)) return `houseView '${String(o.houseView)}' is not a family`;
  if (o.roles !== undefined) {
    if (!isObj(o.roles)) return 'roles is not an object';
    for (const [k, v] of Object.entries(o.roles)) if (!ROLE_KEY.test(k) || !isStr(v)) return `roles['${k}'] is not role → symbol`;
  }
  if (o.sections !== undefined) {
    if (!Array.isArray(o.sections)) return 'sections is not an array';
    for (const sec of o.sections) {
      if (!isLabel(sec) || !isStr(sec.key)) return 'a section has no key';
      if (!Array.isArray(sec.controls) || !sec.controls.every(isStr)) return `section '${String(sec.key)}' controls are not symbols`;
    }
  }
  if (o.labels !== undefined && (!isObj(o.labels) || !Object.values(o.labels).every(isLabel))) return 'labels is not symbol → { labelKey?, label? }';
  if (o.visibleWhen !== undefined) {
    if (!isObj(o.visibleWhen)) return 'visibleWhen is not an object';
    for (const v of Object.values(o.visibleWhen)) {
      if (!isObj(v) || !isStr(v.param) || !Array.isArray(v.values) || !v.values.every(Number.isInteger)) return 'visibleWhen is not symbol → { param, values: int[] }';
    }
  }
  if (o.displays !== undefined && (!Array.isArray(o.displays) || !o.displays.every(isObj))) return 'displays is not an array of objects';
  if (!optStr(o.look)) return 'look is not a string';
  if (o.backdrop !== undefined && !(isStr(o.backdrop) && o.backdrop.startsWith('art/') && isPackagePath(o.backdrop))) return 'backdrop is not a path under art/';
  if (o.view !== undefined) {
    const v = o.view;
    if (!isObj(v) || !isStr(v.entry) || !isPackagePath(v.entry) || !isStr(v.api)) return 'view is not { entry, api }';
  }
  return null;
}

/** Parse and check one `skin.json` (§6.3): a file that does not parse or fails the schema is refused whole. */
export function parseSkinOverride(text) {
  let o;
  try {
    o = JSON.parse(text);
  } catch (e) {
    return { ok: false, code: 'SKIN_OVERRIDE_INVALID', reason: `does not parse: ${e.message}` };
  }
  const why = overrideViolation(o);
  return why === null ? { ok: true, file: o } : { ok: false, code: 'SKIN_OVERRIDE_INVALID', reason: why };
}

// ─── §4 the panel families, for the roles a modgui's symbols suggest ──────────────────────────

const WET_ROLES = ['mix', 'output'];

export const PANEL_FAMILIES = {
  gate: { required: [['threshold'], ['range'], ['attack'], ['release']], optional: ['hold', 'knee_start', 'knee_end', 'hysteresis', 'ratio', 'key_listen'], wet: false },
  dynamics: { required: [['threshold'], ['ratio'], ['attack'], ['release']], optional: ['knee', 'makeup', 'mix', 'lookahead'], wet: false },
  limiter: { required: [['ceiling'], ['release']], optional: ['lookahead', 'threshold'], wet: false },
  deesser: { required: [['frequency'], ['threshold'], ['range']], optional: ['width', 'ratio', 'attack', 'release', 'knee', 'mode', 'listen'], wet: false },
  eq: { required: [['band_freq'], ['band_gain']], optional: ['band_type', 'band_q', 'band_on', 'hpf_freq', 'hpf_slope', 'hpf_on', 'lpf_freq', 'lpf_slope', 'lpf_on', 'output'], wet: false },
  filter: { required: [['cutoff']], optional: ['resonance', 'filter_type', 'mix'], wet: true },
  drive: { required: [['drive']], optional: ['tone', 'model', 'output', 'mix'], wet: true },
  modulation: { required: [['rate'], ['depth']], optional: ['feedback', 'voices', 'spread', 'shape', 'mix'], wet: true },
  delay: { required: [['time'], ['feedback']], optional: ['tone', 'sync', 'division', 'pingpong', 'mix'], wet: true },
  reverb: { required: [['size', 'decay'], ['mix']], optional: ['algorithm', 'predelay', 'damping', 'width', 'low_cut', 'high_cut'], wet: true },
  pitch: { required: [['shift']], optional: ['fine', 'mix'], wet: true },
  none: { required: [], optional: [], wet: false },
};

export const PANEL_FAMILY_NAMES = Object.keys(PANEL_FAMILIES);
export const isPanelFamilyName = (v) => typeof v === 'string' && Object.hasOwn(PANEL_FAMILIES, v);

/** Every role a family accepts: its required names, its optional ones and, if wet, {@link WET_ROLES}. */
export function familyRoleNames(family) {
  const f = PANEL_FAMILIES[family];
  return [...new Set([...f.required.flat(), ...f.optional, ...(f.wet ? WET_ROLES : [])])];
}

// ─── the importer ──────────────────────────────────────────────────────────────────────────────

class Refusal extends Error {
  constructor(code, file, detail) {
    super(`${code}: ${file}: ${detail}`);
    this.code = code;
    this.file = file;
    this.detail = detail;
  }
}

const key = (term) => `${term.t}:${term.v}`;
const show = (term) => (term.t === 'iri' ? `<${term.v}>` : term.t === 'lit' ? JSON.stringify(term.v) : `_:${term.v}`);

/** Every `*.ttl` at the bundle's root, parsed; each triple carries the file it was read from. */
function readBundle(dir) {
  const triples = [];
  for (const f of readdirSync(dir).filter((n) => n.endsWith('.ttl')).sort()) {
    const path = join(dir, f);
    let parsed;
    try {
      parsed = parseTurtle(readFileSync(path, 'utf8'), pathToFileURL(path).href);
    } catch (e) {
      throw new Refusal('MODGUI_TTL_INVALID', path, e.message);
    }
    // Blank nodes are local to their file.
    const local = (t) => (t.t === 'bnode' ? { t: 'bnode', v: `${f}#${t.v}` } : t);
    for (const t of parsed) triples.push({ s: local(t.s), p: t.p, o: local(t.o), file: path });
  }
  return triples;
}

/** A path inside the bundle, or a PATH_ESCAPE refusal; a missing file is MODGUI_NOT_FOUND. */
function insideBundle(bundle, path, file) {
  if (!existsSync(path)) throw new Refusal('MODGUI_NOT_FOUND', file, `names ${path}, which does not exist`);
  const real = realpathSync(path);
  if (real !== bundle && !real.startsWith(bundle + sep)) throw new Refusal('MODGUI_PATH_ESCAPE', file, `${path} resolves outside the bundle, to ${real}`);
  return real;
}

/** Every entry under `dir`, each checked to resolve inside the bundle (a symlink out is refused). */
function checkTree(bundle, dir, file) {
  for (const name of readdirSync(dir)) {
    const p = join(dir, name);
    if (statSync(insideBundle(bundle, p, file)).isDirectory()) checkTree(bundle, p, file);
  }
}

/** The roles the port symbols suggest, against the §4 family table; `null` when no family's required roles are all present. */
export function suggestRoles(symbols) {
  const norm = (s) => s.toLowerCase().replace(/[^a-z0-9]/g, '');
  const bySymbol = new Map(symbols.map((s) => [norm(s), s]));
  let best = null;
  for (const family of PANEL_FAMILY_NAMES) {
    const f = PANEL_FAMILIES[family];
    if (f.required.length === 0) continue;
    const roles = {};
    for (const role of familyRoleNames(family)) {
      const sym = bySymbol.get(norm(role));
      if (sym !== undefined) roles[role] = sym;
    }
    if (!f.required.every((alts) => alts.some((r) => r in roles))) continue;
    const n = Object.keys(roles).length;
    if (best === null || n > best.n) best = { n, family, roles, tie: false };
    else if (n === best.n) best.tie = true;
  }
  if (best === null || best.tie) return null;
  const bound = new Set(Object.values(best.roles));
  return { family: best.family, roles: best.roles, unmatched: symbols.filter((s) => !bound.has(s)) };
}

/**
 * Import one bundle into `<out>/<skinPackageDirName(plugin)>/`. Returns the written package or
 * throws a {@link Refusal}; on a refusal nothing exists under `out` that was not there before.
 */
export async function importModgui({ bundle, out, plugin }) {
  const NS = MODGUI_NS;
  const LV2 = LV2_NS;
  if (!existsSync(bundle) || !statSync(bundle).isDirectory()) throw new Refusal('MODGUI_NOT_FOUND', bundle, 'is not a bundle directory');
  const dir = realpathSync(bundle);
  const triples = readBundle(dir);
  const about = (s) => triples.filter((t) => key(t.s) === key(s));
  const one = (s, p) => about(s).filter((t) => t.p.v === p);

  // The plugin that carries the modgui.
  const guis = triples.filter((t) => t.p.v === `${NS}gui` && (plugin === undefined || t.s.v === plugin));
  if (guis.length === 0) throw new Refusal('MODGUI_NOT_FOUND', dir, plugin ? `no modgui:gui on <${plugin}>` : 'no modgui:gui in any TTL of the bundle');
  if (new Set(guis.map((t) => t.s.v)).size > 1 || guis.length > 1) {
    throw new Refusal('MODGUI_NOT_FOUND', dir, `${guis.length} modgui:gui nodes (${guis.map((t) => show(t.s)).join(', ')}); name one with --plugin`);
  }
  const [{ s: pluginTerm, o: gui, file: guiFile }] = guis;
  if (pluginTerm.t !== 'iri') throw new Refusal('MODGUI_TTL_INVALID', guiFile, 'the modgui:gui subject is not a plugin IRI');

  // Every modgui property it does not know is refused (§6.6), on the gui node, its ports, or anywhere.
  const knownGui = new Set(MODGUI_GUI_PROPERTIES.map((p) => `${NS}${p}`));
  const knownPort = new Set(MODGUI_PORT_PROPERTIES.map((p) => `${LV2}${p}`));
  for (const t of about(gui)) if (!knownGui.has(t.p.v)) throw new Refusal('MODGUI_UNKNOWN_PROPERTY', t.file, `modgui:gui carries ${show(t.p)}`);
  for (const t of triples) {
    if (t.p.v.startsWith(NS) && t.p.v !== `${NS}gui` && !knownGui.has(t.p.v)) throw new Refusal('MODGUI_UNKNOWN_PROPERTY', t.file, `${show(t.p)} is not a modgui property this importer knows`);
    if (t.p.v === RDF_TYPE && t.o.t === 'iri' && t.o.v.startsWith(NS)) throw new Refusal('MODGUI_UNKNOWN_PROPERTY', t.file, `a node is typed ${show(t.o)}`);
  }
  const ports = one(gui, `${NS}port`).map((t) => {
    for (const q of about(t.o)) if (!knownPort.has(q.p.v)) throw new Refusal('MODGUI_UNKNOWN_PROPERTY', q.file, `a modgui:port carries ${show(q.p)}`);
    const val = (p) => one(t.o, `${LV2}${p}`)[0]?.o.v;
    const index = Number(val('index'));
    const symbol = val('symbol');
    if (!Number.isInteger(index) || typeof symbol !== 'string' || symbol === '') throw new Refusal('MODGUI_TTL_INVALID', t.file, 'a modgui:port has no integer lv2:index or no lv2:symbol');
    return { index, symbol, name: val('name') ?? symbol };
  });
  ports.sort((a, b) => a.index - b.index);
  const lit = (p) => one(gui, `${NS}${p}`)[0]?.o.v;
  const res = (p) => {
    const t = one(gui, `${NS}${p}`)[0];
    if (!t) return undefined;
    if (t.o.t !== 'iri' || !t.o.v.startsWith('file:')) throw new Refusal('MODGUI_PATH_ESCAPE', t.file, `modgui:${p} ${show(t.o)} is not a file of the bundle`);
    return insideBundle(dir, fileURLToPath(t.o.v), t.file);
  };

  // The licence: the plugin's doap:license, else its lv2:project's (§6.6; a modgui carries none).
  let licTriples = one(pluginTerm, `${DOAP_NS}license`);
  if (licTriples.length === 0) for (const pr of one(pluginTerm, LV2_PROJECT)) licTriples = licTriples.concat(one(pr.o, `${DOAP_NS}license`));
  const pluginFile = triples.find((t) => key(t.s) === key(pluginTerm) && t.p.v === RDF_TYPE)?.file ?? guiFile;
  if (licTriples.length === 0) throw new Refusal('MODGUI_NO_LICENCE', pluginFile, `<${pluginTerm.v}> carries no doap:license, nor does its lv2:project`);
  const licences = licTriples.map((t) => {
    const spdx = spdxFromDoapLicence(t.o.t === 'iri' ? { iri: t.o.v } : { literal: t.o.v });
    if (spdx === null) throw new Refusal('MODGUI_LICENCE_UNMAPPED', t.file, `doap:license ${show(t.o)} names no SPDX licence`);
    if (!isSkinLicenceAllowed(spdx)) throw new Refusal('MODGUI_LICENCE_REFUSED', t.file, `doap:license ${show(t.o)} is ${spdx}, outside the allow-list (${SKIN_LICENCE_ALLOW_LIST.join(', ')})`);
    return { spdx, evidence: `${basename(t.file)} doap:license ${show(t.o)}` };
  });

  // The files: every resource inside the bundle, every entry of the resources directory too.
  const resourcesDir = res('resourcesDirectory');
  if (resourcesDir === undefined || !statSync(resourcesDir).isDirectory()) throw new Refusal('MODGUI_NOT_FOUND', guiFile, 'modgui:resourcesDirectory names no directory');
  checkTree(dir, resourcesDir, guiFile);
  const named = ['iconTemplate', 'stylesheet', 'screenshot', 'thumbnail'].map((p) => [p, res(p)]).filter(([, f]) => f !== undefined);
  const screenshot = named.find(([p]) => p === 'screenshot')?.[1];

  const id = pluginTerm.v;
  const name = skinPackageDirName(id);
  const outDir = resolve(out);
  const target = join(outDir, name);
  if (existsSync(target)) throw new Refusal('MODGUI_TARGET_EXISTS', target, 'the package is already there; nothing is overwritten');
  const ttlCopy = basename(guiFile);
  if (existsSync(join(resourcesDir, ttlCopy))) throw new Refusal('MODGUI_TARGET_EXISTS', join(resourcesDir, ttlCopy), `the resources directory already holds a ${ttlCopy}`);

  const label = lit('label');
  const skin = {
    schema: SKIN_SCHEMA_VERSION,
    plugin: id,
    licence: [...new Set(licences.map((l) => l.spdx))].join(' AND '),
    source: { kind: 'modgui', bundle: dir, licenceEvidence: licences.map((l) => l.evidence).join('; ') },
    sections: [{ key: 'modgui', ...(label !== undefined ? { label } : {}), controls: ports.map((p) => p.symbol) }],
    labels: Object.fromEntries(ports.map((p) => [p.symbol, { label: p.name }])),
    ...(screenshot !== undefined ? { backdrop: `art/${basename(screenshot)}` } : {}),
  };
  const checked = parseSkinOverride(JSON.stringify(skin));
  if (!checked.ok) throw new Error(`modgui-import: the package it built fails §6.3: ${checked.reason}`);

  // Write beside the target and rename, so a failure leaves nothing behind.
  mkdirSync(outDir, { recursive: true });
  const tmp = join(outDir, `.${name}.importing-${process.pid}`);
  try {
    cpSync(resourcesDir, join(tmp, 'modgui'), { recursive: true, dereference: true });
    for (const [, f] of named) {
      const rel = relative(resourcesDir, f);
      if (rel.startsWith('..')) cpSync(f, join(tmp, 'modgui', relative(dir, f)));
    }
    cpSync(guiFile, join(tmp, 'modgui', ttlCopy));
    if (screenshot !== undefined) {
      mkdirSync(join(tmp, 'art'));
      cpSync(screenshot, join(tmp, 'art', basename(screenshot)));
    }
    writeFileSync(join(tmp, SKIN_OVERRIDE_FILE), `${JSON.stringify(skin, null, 2)}\n`);
    renameSync(tmp, target);
  } catch (e) {
    rmSync(tmp, { recursive: true, force: true });
    throw e;
  }
  return { dir: target, skin, brand: lit('brand'), label, suggestedRoles: suggestRoles(ports.map((p) => p.symbol)) };
}

async function main() {
  const argv = process.argv.slice(2);
  const opt = (flag) => {
    const k = argv.indexOf(flag);
    return k >= 0 ? argv[k + 1] : undefined;
  };
  const bundle = argv.find((a, k) => !a.startsWith('--') && !['--out', '--plugin'].includes(argv[k - 1]));
  const out = opt('--out');
  if (!bundle || !out) {
    console.error('usage: modgui-import.mjs <bundle.lv2> --out <skins dir> [--plugin <uri>]');
    process.exit(2);
  }
  try {
    const r = await importModgui({ bundle, out, plugin: opt('--plugin') });
    console.log(`MODGUI-IMPORT-JSON ${JSON.stringify({ verdict: 'written', dir: r.dir, licence: r.skin.licence, brand: r.brand, label: r.label, controls: r.skin.sections[0].controls, suggestedRoles: r.suggestedRoles })}`);
    if (r.suggestedRoles) console.error(`suggested (not written; confirm into skin.json): family ${r.suggestedRoles.family}, roles ${JSON.stringify(r.suggestedRoles.roles)}`);
  } catch (e) {
    if (!(e instanceof Refusal)) throw e;
    console.log(`MODGUI-IMPORT-JSON ${JSON.stringify({ verdict: 'refused', code: e.code, file: e.file, detail: e.detail })}`);
    console.error(`refused ${e.code}: ${e.file}: ${e.detail} (nothing written)`);
    process.exit(3);
  }
}

if (process.argv[1] && resolve(process.argv[1]) === fileURLToPath(import.meta.url)) await main();
