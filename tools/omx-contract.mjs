// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * omx-contract.mjs — where a plugin's parameter references are read from: omx-contract's data at
 * the version this repository pins (omx-contract.pin.json), and nowhere else.
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
 * The data is found, in order: $OMX_CONTRACT_DIR (a directory holding package.json and data/);
 * else a sibling git checkout ../omx-contract, read at $OMX_CONTRACT_REF, or the tag v<pin>, or
 * origin/main, and extracted once into build/omx-contract-<pin>/. In every case the package.json
 * version must equal the pin.
 */
import { execFileSync } from 'node:child_process';
import { existsSync, mkdirSync, readFileSync, readdirSync, writeFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

export const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..');
export const PIN_FILE = 'omx-contract.pin.json';

/** The omx-contract version a tree pins. */
export function contractPin(root = ROOT) {
  const pin = JSON.parse(readFileSync(join(root, PIN_FILE), 'utf8'));
  if (!/^\d+\.\d+\.\d+$/.test(pin.version ?? '')) throw new Error(`${PIN_FILE}: version '${pin.version}' is not x.y.z`);
  return pin.version;
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

/**
 * The directory omx-contract's data is read from, at the pin: `{ dir, at }`, or `{ dir: undefined,
 * why }` when no omx-contract at the pinned version is reachable.
 */
export function locateContract(root = ROOT) {
  const pin = contractPin(root);
  const env = process.env.OMX_CONTRACT_DIR;
  if (env) {
    const v = versionOf(env);
    if (!existsSync(join(env, 'data'))) return { dir: undefined, pin, why: `OMX_CONTRACT_DIR=${env} holds no data/` };
    if (v !== pin) return { dir: undefined, pin, why: `OMX_CONTRACT_DIR=${env} is omx-contract ${v}, the pin is ${pin}` };
    return { dir: env, pin, at: `${env} (omx-contract ${v})` };
  }
  const cache = join(root, 'build', `omx-contract-${pin}`);
  if (versionOf(cache) === pin && existsSync(join(cache, 'data'))) return { dir: cache, pin, at: `${cache} (omx-contract ${pin})` };
  const sibling = join(root, '..', 'omx-contract');
  if (!existsSync(join(sibling, '.git'))) {
    return { dir: undefined, pin, why: `no omx-contract ${pin}: set OMX_CONTRACT_DIR, or check omx-contract out beside this tree` };
  }
  const refs = [process.env.OMX_CONTRACT_REF, `v${pin}`, 'origin/main'].filter(Boolean);
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
    execFileSync('sh', ['-c', 'git -C "$1" archive "$2" package.json data | tar -x -C "$3"', 'sh', sibling, ref, cache]);
    return { dir: cache, pin, at: `${sibling} at ${ref} (omx-contract ${pin})` };
  }
  return { dir: undefined, pin, why: `${sibling}: none of ${refs.join(', ')} is omx-contract ${pin}` };
}

const readJson = (f) => JSON.parse(readFileSync(f, 'utf8'));
const names = (obj) => Object.keys(obj).filter((k) => !k.startsWith('$'));

/** The files a name may resolve in, for one kernel: its own file first, then the shared files. */
export function contractFiles(dir, kernel) {
  const kernels = readdirSync(join(dir, 'data', 'kernels')).filter((f) => f.endsWith('.json')).map((f) => f.slice(0, -5));
  return {
    own: kernels.includes(kernel) ? `data/kernels/${kernel}.json` : undefined,
    others: kernels.filter((k) => k !== kernel).map((k) => `data/kernels/${k}.json`),
    shared: ['data/primitives.json', 'data/rates.json'].filter((f) => existsSync(join(dir, f))),
  };
}

/** Does the kernel have its file in omx-contract at `dir`? */
export const kernelExists = (dir, kernel) => existsSync(join(dir, 'data', 'kernels', `${kernel}.json`));

/** Where one name resolves: `{ file, entry }` or a refusal `{ error }`. */
export function findName(dir, kernel, name) {
  const f = contractFiles(dir, kernel);
  const hits = [f.own, ...f.shared].filter(Boolean).filter((file) => names(readJson(join(dir, file))).includes(name));
  const foreign = f.others.filter((file) => names(readJson(join(dir, file))).includes(name));
  if (foreign.length && !hits.length) return { error: `'${name}' is in ${foreign.join(', ')}, another kernel's file: a plugin reads only its own kernel (${kernel})` };
  if (hits.length === 0) return { error: `'${name}' is in neither data/kernels/${kernel}.json nor the shared files` };
  if (hits.length > 1) return { error: `'${name}' is in ${hits.join(' and ')}: a name lives in one place` };
  return { file: hits[0], entry: readJson(join(dir, hits[0]))[name] };
}

const num = (v, what) => {
  if (typeof v === 'number') return v;
  throw new Error(`${what} is not a plain number (${JSON.stringify(v)}); a parameter's travel must be one`);
};

/**
 * One parameter's travel, read from its reference: `{ min, max, def, unit, kind? }`. A `travels`
 * entry gives min/max/default/unit (from `field` when the entry has fields; the default for
 * `forKind` from its byKind); a boolean `scalar` is a toggle; a `list` is an integer index into its
 * values, its default named by `defaultRef` (a scalar holding one of the values) or the first.
 */
export function resolveParam(dir, kernel, p) {
  const where = findName(dir, kernel, p.ref);
  if (where.error) throw new Error(`param '${p.symbol}': ${where.error}`);
  const e = where.entry;
  const at = `${where.file}#${p.ref}${p.field ? `.${p.field}` : ''}`;
  if (e.kind === 'travels') {
    let t = e.travel;
    if (e.fields) {
      if (!p.field) throw new Error(`param '${p.symbol}': ${where.file}#${p.ref} has fields (${Object.keys(e.fields).join(', ')}); name one with "field"`);
      t = e.fields[p.field];
      if (!t) throw new Error(`param '${p.symbol}': ${where.file}#${p.ref} has no field '${p.field}'`);
    } else if (p.field) throw new Error(`param '${p.symbol}': ${where.file}#${p.ref} has no fields; drop "field"`);
    let def = t.default ?? t.min;
    // a kind the travel does not override comes up at the travel's own default
    if (p.forKind && t.byKind && p.forKind in t.byKind) def = t.byKind[p.forKind];
    const out = { min: num(t.min, at), max: num(t.max, at), def: num(def, at), unit: t.unit ?? '', from: at };
    if (typeof t.step === 'number' && t.step >= 1 && Number.isInteger(out.min) && Number.isInteger(out.max)) out.kind = 'integer';
    return out;
  }
  if (e.kind === 'scalar' && typeof e.value === 'boolean') return { min: 0, max: 1, def: e.value ? 1 : 0, unit: '', kind: 'toggle', from: at };
  if (e.kind === 'list' && Array.isArray(e.values)) {
    let def = 0;
    if (p.defaultRef) {
      const d = findName(dir, kernel, p.defaultRef);
      if (d.error) throw new Error(`param '${p.symbol}' defaultRef: ${d.error}`);
      def = e.values.indexOf(d.entry.value);
      if (def < 0) throw new Error(`param '${p.symbol}': ${p.defaultRef}'s value ${JSON.stringify(d.entry.value)} is not one of ${p.ref}'s values`);
    }
    return { min: 0, max: e.values.length - 1, def, unit: '', kind: 'integer', values: e.values, from: at };
  }
  throw new Error(`param '${p.symbol}': ${at} is a ${e.kind}${e.kind === 'scalar' ? ` of ${JSON.stringify(e.value)}` : ''}, not a travel, a toggle or a list`);
}

/**
 * The perturbation proof's move: copy omx-contract's data at `src` to `dst` and move the default of
 * the travel parameter `p` (of `kernel`) references to its other end. The copy is what a consumer
 * reads with OMX_CONTRACT_DIR=dst; every generated file must then follow, or be refused as stale.
 */
export function perturbCopy(src, dst, kernel, p) {
  execFileSync('cp', ['-R', join(src, 'data'), join(src, 'package.json'), dst]);
  const where = findName(dst, kernel, p.ref);
  if (where.error) throw new Error(where.error);
  const file = join(dst, where.file);
  const data = readJson(file);
  const e = data[p.ref];
  if (e.kind === 'travels') {
    const t = e.fields ? e.fields[p.field] : e.travel;
    const def = t.default ?? t.min;
    t.default = def === t.max ? t.min : t.max;
    if (p.forKind && t.byKind && p.forKind in t.byKind) t.byKind[p.forKind] = t.default;
  } else if (e.kind === 'scalar' && typeof e.value === 'boolean') e.value = !e.value;
  else throw new Error(`${p.ref}: a ${e.kind} has no default to move`);
  writeFileSync(file, `${JSON.stringify(data, null, 2)}\n`);
  return where.file;
}

/** The omx-contract kernels a declaration's parameters reference: `kernels`, or `[kernel]`. */
export const declKernels = (d) => (Array.isArray(d.kernels) && d.kernels.length ? d.kernels : [d.kernel]);

/**
 * The kernel one parameter's reference is read from: its own `kernel` (which must be one of the
 * plugin's kernels), or the plugin's only kernel. A composite's parameter must name its kernel.
 */
export function paramKernel(d, p) {
  const ks = declKernels(d);
  if (p.kernel !== undefined) {
    if (!ks.includes(p.kernel)) throw new Error(`param '${p.symbol}': kernel '${p.kernel}' is not one of the plugin's kernels (${ks.join(', ')})`);
    return p.kernel;
  }
  if (ks.length > 1) throw new Error(`param '${p.symbol}': a composite (${ks.join(', ')}) names each parameter's kernel`);
  return ks[0];
}
