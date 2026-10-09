// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * omx-contract.mjs — where a plugin's parameter references are read from: omx-contract's RESOLVED
 * data (share/omx-contract/omx-contract.json: every reference and derivation already worked out) at
 * the version this repository pins (`omx-contract` in .github/pins.txt), and nowhere else.
 *
 * A parameter in plugins/<stem>/<stem>.decl.json names its travel BY REFERENCE (`"ref"`, with an
 * optional `"field"` and `"forKind"`), never by retyped numbers. A name resolves in this order, and
 * it must resolve in exactly one place:
 *
 *   1. the plugin's kernel file, data/kernels/<kernel>.json;
 *   2. omx-contract's shared files, data/primitives.json and data/rates.json.
 *
 * A name found in ANOTHER kernel's file is refused (a plugin reads only its own kernel), as is a
 * name found in more than one place and a name found nowhere.
 *
 * The release is found, in order: $OMX_CONTRACT_DIR (a directory holding package.json and
 * share/omx-contract/omx-contract.json, lib/eq-defaults.mjs: the unpacked npm/release package); else
 * build/omx-contract-<pin>/ (this tool's earlier fetch); else a sibling git checkout ../omx-contract,
 * read at $OMX_CONTRACT_REF or the tag v<pin>; else the release tarball
 * openmixer-omx-contract-<pin>.tgz of github.com/FreeMixer/omx-contract, fetched with curl. In every
 * case the package.json version must equal the pin.
 */
import { execFileSync } from 'node:child_process';
import { existsSync, mkdirSync, readFileSync, writeFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

export const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..');
export const PIN_FILE = '.github/pins.txt';
const RESOLVED = join('share', 'omx-contract', 'omx-contract.json');
const RELEASE_URL = (v) => `https://github.com/FreeMixer/omx-contract/releases/download/v${v}/openmixer-omx-contract-${v}.tgz`;

/** The pinned version of one source in .github/pins.txt (`name url version`), or undefined. */
export function pinOf(name, root = ROOT) {
  const f = join(root, PIN_FILE);
  if (!existsSync(f)) return undefined;
  for (const line of readFileSync(f, 'utf8').split('\n')) {
    const w = line.trim().split(/\s+/);
    if (w[0] === name && !line.trim().startsWith('#')) return w[2];
  }
  return undefined;
}

/** The omx-contract version a tree pins. */
export function contractPin(root = ROOT) {
  const v = pinOf('omx-contract', root);
  if (!/^\d+\.\d+\.\d+$/.test(v ?? '')) throw new Error(`${PIN_FILE}: the omx-contract version '${v}' is not x.y.z`);
  return v;
}

/** -1, 0 or 1, comparing two x.y.z versions. */
export function compareVersions(a, b) {
  const pa = a.split('.').map(Number), pb = b.split('.').map(Number);
  for (let i = 0; i < 3; i++) if (pa[i] !== pb[i]) return pa[i] < pb[i] ? -1 : 1;
  return 0;
}

const versionOf = (dir) => {
  try {
    return JSON.parse(readFileSync(join(dir, 'package.json'), 'utf8')).version;
  } catch {
    return undefined;
  }
};

function gitOk(repo, ...args) {
  try {
    return execFileSync('git', ['-C', repo, ...args], { encoding: 'utf8', stdio: ['ignore', 'pipe', 'ignore'] });
  } catch {
    return undefined;
  }
}

const complete = (dir) => existsSync(join(dir, RESOLVED)) && existsSync(join(dir, 'lib', 'eq-defaults.mjs'));

/**
 * The directory omx-contract's release is read from, at the pin: `{ dir, pin, at }`, or `{ dir:
 * undefined, pin, why }` when no omx-contract at the pinned version is reachable.
 */
export function locateContract(root = ROOT) {
  const pin = contractPin(root);
  const env = process.env.OMX_CONTRACT_DIR;
  if (env) {
    const v = versionOf(env);
    if (!complete(env)) return { dir: undefined, pin, why: `OMX_CONTRACT_DIR=${env} holds no ${RESOLVED} and lib/eq-defaults.mjs` };
    if (v !== pin) return { dir: undefined, pin, why: `OMX_CONTRACT_DIR=${env} is omx-contract ${v}, the pin is ${pin}` };
    return { dir: env, pin, at: `${env} (omx-contract ${v})` };
  }
  const cache = join(root, 'build', `omx-contract-${pin}`);
  if (versionOf(cache) === pin && complete(cache)) return { dir: cache, pin, at: `${cache} (omx-contract ${pin})` };
  const sibling = join(root, '..', 'omx-contract');
  if (existsSync(join(sibling, '.git'))) {
    const refs = [process.env.OMX_CONTRACT_REF, `v${pin}`].filter(Boolean);
    for (const ref of refs) {
      const pkg = gitOk(sibling, 'show', `${ref}:package.json`);
      if (pkg === undefined) continue;
      let v;
      try {
        v = JSON.parse(pkg).version;
      } catch {
        continue;
      }
      if (v !== pin) continue;
      mkdirSync(cache, { recursive: true });
      execFileSync('sh', ['-c', 'git -C "$1" archive "$2" package.json data share lib | tar -x -C "$3"', 'sh', sibling, ref, cache]);
      return { dir: cache, pin, at: `${sibling} at ${ref} (omx-contract ${pin})` };
    }
  }
  try {
    mkdirSync(cache, { recursive: true });
    execFileSync('sh', ['-c', 'curl -fsSL "$1" | tar -xz --strip-components=1 -C "$2"', 'sh', RELEASE_URL(pin), cache], { stdio: ['ignore', 'ignore', 'pipe'] });
  } catch (e) {
    return { dir: undefined, pin, why: `omx-contract ${pin} is not at OMX_CONTRACT_DIR, in a sibling checkout or at ${RELEASE_URL(pin)}: ${String(e.stderr ?? e.message).trim()}` };
  }
  if (versionOf(cache) === pin && complete(cache)) return { dir: cache, pin, at: `${RELEASE_URL(pin)} (omx-contract ${pin})` };
  return { dir: undefined, pin, why: `the release fetched from ${RELEASE_URL(pin)} is not a complete omx-contract ${pin}` };
}

const resolvedCache = new Map();
/** The resolved items of the release at `dir`: name -> { rel, kind, shape, value, ... }. */
export function items(dir) {
  if (!resolvedCache.has(dir)) resolvedCache.set(dir, JSON.parse(readFileSync(join(dir, RESOLVED), 'utf8')).items);
  return resolvedCache.get(dir);
}

const kernelFile = (k) => `data/kernels/${k}.json`;
const SHARED = ['data/primitives.json', 'data/rates.json'];

/** Does the kernel have its file in omx-contract at `dir`? */
export const kernelExists = (dir, kernel) => Object.values(items(dir)).some((e) => e.rel === kernelFile(kernel));

/** Where one name resolves: `{ file, entry }` or a refusal `{ error }`. */
export function findName(dir, kernel, name) {
  const e = items(dir)[name];
  if (!e) return { error: `'${name}' is in neither ${kernelFile(kernel)} nor the shared files` };
  if (e.rel === kernelFile(kernel) || SHARED.includes(e.rel)) return { file: e.rel, entry: e };
  return { error: `'${name}' is in ${e.rel}, another kernel's file: a plugin reads only its own kernel (${kernel})` };
}

const num = (v, what) => {
  if (typeof v === 'number') return v;
  throw new Error(`${what} is not a plain number (${JSON.stringify(v)}); a parameter's travel must be one`);
};

/** A dotted path into a sheet's value: `NAME.a.b` -> the number there. */
function sheetValue(dir, kernel, path, symbol) {
  const [name, ...keys] = path.split('.');
  const d = findName(dir, kernel, name);
  if (d.error) throw new Error(`param '${symbol}' defaultRef: ${d.error}`);
  let v = d.entry.value;
  for (const k of keys) v = v?.[k];
  if (typeof v !== 'number' && typeof v !== 'boolean') throw new Error(`param '${symbol}' defaultRef: ${path} is not a number in ${d.file}`);
  return typeof v === 'boolean' ? (v ? 1 : 0) : v;
}

const eqCache = new Map();
/**
 * The default bands of an EQ with `count` bands, by omx-contract's own rule (lib/eq-defaults.mjs of
 * the release, run in a child node so this stays synchronous): `[{ type, freqHz, gainDb, q }]`.
 */
export function eqDefaultBands(dir, count) {
  const key = `${dir}#${count}`;
  if (!eqCache.has(key)) {
    const script = `import { eqDefaultBands, eqDefaultParams } from ${JSON.stringify(`file://${join(dir, 'lib', 'eq-defaults.mjs')}`)};
import { readFileSync } from 'node:fs';
const items = JSON.parse(readFileSync(${JSON.stringify(join(dir, RESOLVED))}, 'utf8')).items;
console.log(JSON.stringify(eqDefaultBands(${Number(count)}, eqDefaultParams(new Map(Object.entries(items))))));`;
    eqCache.set(key, JSON.parse(execFileSync(process.execPath, ['--input-type=module', '-e', script], { encoding: 'utf8' })));
  }
  return eqCache.get(key);
}

/** The band count of a strip type (`eq16`, ...) in EQ_BAND_COUNTS: its default. */
const bandCount = (dir, strip, symbol) => {
  const c = items(dir).EQ_BAND_COUNTS?.value?.[strip];
  if (!c) throw new Error(`param '${symbol}': EQ_BAND_COUNTS has no strip type '${strip}'`);
  return c.default;
};

/**
 * One parameter's travel, read from its reference: `{ min, max, def, unit, kind?, values?, points? }`.
 * A `travels` entry gives min/max/default/unit (from `field` when the entry is a table; the default
 * for `forKind` from its byKind; `defaultRef` NAME.path names a sheet number to come up at instead,
 * and `defaultBand` {strip, index, of: 'centre'|'type'} takes the default of one band of
 * omx-contract's default EQ rule); a boolean `scalar` is a toggle, and so is a set whose ids are
 * exactly off, on; a `list` is an integer index into
 * its values, its default named by `defaultRef` (a scalar holding one of the values) or the first;
 * a `set` is an integer index into its ids, labelled by its labels (or the declaration's `values`
 * when the set has none), its default the set's own, `defaultRef` or `defaultBand`.
 */
export function resolveParam(dir, kernel, p) {
  const where = findName(dir, kernel, p.ref);
  if (where.error) throw new Error(`param '${p.symbol}': ${where.error}`);
  const e = where.entry;
  const at = `${where.file}#${p.ref}${p.field ? `.${p.field}` : ''}`;
  const band = p.defaultBand ? eqDefaultBands(dir, bandCount(dir, p.defaultBand.strip, p.symbol))[p.defaultBand.index] : undefined;
  if (p.defaultBand && !band) throw new Error(`param '${p.symbol}': ${p.defaultBand.strip} has no band ${p.defaultBand.index}`);
  if (e.kind === 'travels') {
    let t = e.value;
    if (e.shape === 'table') {
      if (!p.field) throw new Error(`param '${p.symbol}': ${where.file}#${p.ref} has fields (${Object.keys(e.value).join(', ')}); name one with "field"`);
      t = e.value[p.field];
      if (!t) throw new Error(`param '${p.symbol}': ${where.file}#${p.ref} has no field '${p.field}'`);
    } else if (p.field) throw new Error(`param '${p.symbol}': ${where.file}#${p.ref} has no fields; drop "field"`);
    let def = t.default ?? t.min;
    // a kind the travel does not override comes up at the travel's own default
    if (p.forKind && t.byKind && p.forKind in t.byKind) def = t.byKind[p.forKind];
    if (p.defaultRef) def = sheetValue(dir, kernel, p.defaultRef, p.symbol);
    if (band) {
      if (p.defaultBand.of !== 'centre') throw new Error(`param '${p.symbol}': a travel's band default is the 'centre'`);
      def = band.freqHz;
    }
    const out = { min: num(t.min, at), max: num(t.max, at), def: num(def, at), unit: t.unit ?? '', from: at };
    if (typeof t.step === 'number' && t.step >= 1 && Number.isInteger(out.min) && Number.isInteger(out.max)) out.kind = 'integer';
    return out;
  }
  if (e.kind === 'scalar' && typeof e.value === 'boolean') return { min: 0, max: 1, def: e.value ? 1 : 0, unit: '', kind: 'toggle', from: at };
  if (e.kind === 'list' && Array.isArray(e.value)) {
    let def = 0;
    if (p.defaultRef) {
      const d = findName(dir, kernel, p.defaultRef);
      if (d.error) throw new Error(`param '${p.symbol}' defaultRef: ${d.error}`);
      def = e.value.indexOf(d.entry.value);
      if (def < 0) throw new Error(`param '${p.symbol}': ${p.defaultRef}'s value ${JSON.stringify(d.entry.value)} is not one of ${p.ref}'s values`);
    }
    return { min: 0, max: e.value.length - 1, def, unit: '', kind: 'integer', values: e.value, from: at };
  }
  if (e.kind === 'set' && Array.isArray(e.value)) {
    const ids = e.value;
    const numeric = ids.every((x) => typeof x === 'number');
    if (p.defaultId !== undefined && e.default !== undefined) throw new Error(`param '${p.symbol}': ${p.ref} names its own default (${JSON.stringify(e.default)}); "defaultId" is for a set that names none`);
    let defId = e.default ?? p.defaultId ?? ids[0];
    if (band) {
      if (p.defaultBand.of !== 'type') throw new Error(`param '${p.symbol}': a set's band default is the 'type'`);
      defId = band.type;
    }
    const def = ids.indexOf(defId);
    if (def < 0) throw new Error(`param '${p.symbol}': ${JSON.stringify(defId)} is not one of ${p.ref}'s ids ${JSON.stringify(ids)}`);
    if (numeric) {
      // a set of numbers (the pass filters' slopes) is the travel from its first to its last, labelled at each id
      const unit = p.unit ?? '';
      return { min: ids[0], max: ids[ids.length - 1], def: ids[def], unit, kind: 'integer', points: ids.map((v) => ({ value: v, label: unit ? `${v} ${unit}` : `${v}` })), from: at };
    }
    // a switch (ids exactly off, on: DELAY_PINGPONGS, DRIVE_AUTO_GAINS, EQ_BAND_ONS) is a toggle,
    // its value the id's index, so a host draws it as one and keeps the toggled hint
    if (ids.length === 2 && ids[0] === 'off' && ids[1] === 'on' && p.values === undefined)
      return { min: 0, max: 1, def, unit: '', kind: 'toggle', from: at };
    const labels = e.labels ?? p.values;
    if (!Array.isArray(labels) || labels.length !== ids.length) throw new Error(`param '${p.symbol}': ${p.ref} has no labels; the declaration's "values" must give one per id (${ids.length})`);
    return { min: 0, max: ids.length - 1, def, unit: '', kind: 'integer', values: labels, from: at };
  }
  throw new Error(`param '${p.symbol}': ${at} is a ${e.kind}${e.kind === 'scalar' ? ` of ${JSON.stringify(e.value)}` : ''}, not a travel, a toggle, a list or a set`);
}

/**
 * The perturbation proof's move: copy omx-contract's release at `src` to `dst` and move the default
 * of the travel parameter `p` (of `kernel`) references to its other end. The copy is what a consumer
 * reads with OMX_CONTRACT_DIR=dst; every generated file must then follow, or be refused as stale.
 */
export function perturbCopy(src, dst, kernel, p) {
  execFileSync('cp', ['-R', join(src, 'share'), join(src, 'lib'), join(src, 'package.json'), dst]);
  const file = join(dst, RESOLVED);
  const data = JSON.parse(readFileSync(file, 'utf8'));
  const where = findNameIn(data.items, kernel, p.ref);
  if (where.error) throw new Error(where.error);
  const e = data.items[p.ref];
  if (e.kind === 'travels') {
    const t = e.shape === 'table' ? e.value[p.field] : e.value;
    const def = t.default ?? t.min;
    t.default = def === t.max ? t.min : t.max;
    if (p.forKind && t.byKind && p.forKind in t.byKind) t.byKind[p.forKind] = t.default;
  } else if (e.kind === 'scalar' && typeof e.value === 'boolean') e.value = !e.value;
  else if (e.kind === 'set' && Array.isArray(e.value) && e.value.length > 1) {
    // a set's default is one of its ids: it moves to the last, or from the last to the first
    const ids = e.value;
    const now = ids.includes(e.default) ? e.default : ids[0];
    e.default = now === ids[ids.length - 1] ? ids[0] : ids[ids.length - 1];
  } else throw new Error(`${p.ref}: a ${e.kind} has no default to move`);
  writeFileSync(file, `${JSON.stringify(data, null, 2)}\n`);
  return where.file;
}

function findNameIn(its, kernel, name) {
  const e = its[name];
  if (!e) return { error: `'${name}' is in neither ${kernelFile(kernel)} nor the shared files` };
  if (e.rel === kernelFile(kernel) || SHARED.includes(e.rel)) return { file: e.rel };
  return { error: `'${name}' is in ${e.rel}, another kernel's file` };
}

/** The omx-contract kernels a declaration's parameters reference: `kernels`, or `[kernel]`. */
export const declKernels = (d) => (Array.isArray(d.kernels) && d.kernels.length ? d.kernels : [d.kernel]);

/**
 * The kernel one parameter's reference is read from: its own `kernel` (which must be one of the
 * plugin's kernels), or the plugin's only kernel. A composite's parameter must name its kernel.
 */
export function paramKernel(d, p, dir) {
  const ks = declKernels(d);
  if (p.kernel !== undefined) {
    if (!ks.includes(p.kernel)) throw new Error(`param '${p.symbol}': kernel '${p.kernel}' is not one of the plugin's kernels (${ks.join(', ')})`);
    return p.kernel;
  }
  // a name in the shared files (primitives, rates) belongs to no kernel: any of the plugin's reads it
  if (ks.length > 1 && dir && SHARED.includes(items(dir)[p.ref]?.rel)) return ks[0];
  if (ks.length > 1) throw new Error(`param '${p.symbol}': a composite (${ks.join(', ')}) names each parameter's kernel`);
  return ks[0];
}

/** The render's `kernels`: kernel -> { controls: [{ name, kind, global, table?, rearms? }] }. */
const kernelsCache = new Map();
export function renderKernels(dir) {
  if (!kernelsCache.has(dir)) kernelsCache.set(dir, JSON.parse(readFileSync(join(dir, RESOLVED), 'utf8')).kernels ?? {});
  return kernelsCache.get(dir);
}

/**
 * The controls of one kernel, in the kernel file's order, as omx-contract's render lists them under
 * `kernels`: `[{ name, ref, field?, kind: 'travel'|'choice', rearms? }]`. A control's `global` is its
 * `ref`; a table field's name is its `field`; `rearms` is true on a control whose change re-arms the
 * kernel's state.
 * @throws when the render lists no such kernel.
 */
export function kernelControls(dir, kernel) {
  const listed = renderKernels(dir)[kernel];
  if (!listed) throw new Error(`omx-contract's render (${join(dir, RESOLVED)}) lists no kernel '${kernel}' under "kernels"`);
  return listed.controls.map((c) => ({
    name: c.name,
    ref: c.global,
    ...(c.table ? { field: c.name } : {}),
    kind: c.kind,
    ...(c.rearms === true ? { rearms: true } : {}),
    ...(c.count ? { count: c.count } : {}),
    ...(c.when ? { when: c.when } : {}),
  }));
}

/** The contract control a by-reference parameter reads: its `field`, or the control its `ref` is. */
export function controlOf(dir, kernel, p) {
  if (p.ref === undefined) return undefined;
  const cs = kernelControls(dir, kernel);
  // two controls may use one set (eq's hpfSlope and lpfSlope, FILTER_SLOPES): the parameter whose
  // symbol is the control's name in snake case (or its base symbol, a per-band b<n>_ prefix off) takes it
  const snake = (x) => x.replace(/([a-z0-9])([A-Z])/g, '$1_$2').toLowerCase();
  const byRef = cs.filter((x) => x.ref === p.ref);
  const sym = snake(p.symbol.replace(/^b\d+_/, ''));
  const c = p.field
    ? cs.find((x) => x.name === p.field && (x.ref === p.ref || !x.field))
    : byRef.length > 1 ? byRef.find((x) => snake(x.name) === sym) ?? byRef[0] : byRef[0];
  return c ? { ...c, ...(p.field ? { name: p.field } : {}) } : undefined;
}
