#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * gen.mjs — every file a plugin's faces read, generated from its ONE declaration
 * (plugins/<stem>/<stem>.decl.json):
 *
 *   plugins/<stem>/generated/omx_<kernel>_params.h   the parameter table the CLAP and LV2 faces read
 *   plugins/<stem>/generated/<stem>.lv2/manifest.ttl  the LV2 bundle's manifest
 *   plugins/<stem>/generated/<stem>.lv2/<stem>.ttl    the LV2 plugin description
 *
 * and, for a plugin whose declaration says `"binding": "instance"` (or `"chain"`: a composite whose
 * `chain` runs several kernels' faces in order), the rest of its folder (the declaration
 * is then the folder's one hand-written file), from recipes/templates/plugin/:
 *
 *   plugins/<stem>/omx_<kernel>_core.h               the binding: each parameter to the resolve()
 *                                                    argument of omx-dsp's instance face it names
 *   plugins/<stem>/test/<kernel>-oracle.c            the kernel-identity test against that face
 *   plugins/<stem>/omx_<kernel>_clap.c, _lv2.c       the CLAP and LV2 faces
 *   plugins/<stem>/Makefile                          the build and `make test`
 *
 * and, from every plugin folder at once, the regions of the shared files between a `BEGIN
 * GENERATED <name>` line and its `END GENERATED <name>` line (SHARED below): the README's catalogue
 * and sections, the RPM %files lists, CI's installed-file lists and libomxplugins' table of CLAP
 * entries (include/omx_clap_static.h, lib/omx_clap_static.c). Adding a plugin touches only its
 * folder; two plugins' changes merge in either order.
 *
 * and the hint view tools/test/port-hints.json (git-ignored), assembled from every declaration's
 * `portHints` (the pin of its LV2 port hints) for tools/port-hints.mjs.
 *
 * The MOD GUI of the same bundle is omx-panel's (tools/panel-file.mjs hands it the declared panel).
 *
 * Usage: `node tools/gen.mjs [--check] [plugins/<stem> ...]` — `--check` writes nothing and fails on a
 * stale, missing or changed file. Named plugins are generated alone; with none, every plugin and
 * the shared files. No clock and no absolute path reach the output.
 */
import { execFileSync } from 'node:child_process';
import { createHash } from 'node:crypto';
import { existsSync, mkdirSync, readFileSync, readdirSync, writeFileSync } from 'node:fs';
import { basename, dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { releaseVersion } from './release-version.mjs';
import { faceBinding } from './instance-face.mjs';
import { descriptorFile, descriptorOf, descriptorText, isWhole, panelDescription, portGroupsOf, travelUnit, viewOf } from './descriptor.mjs';
import { lv2Designation, lv2UnitNames, lv2UnitRef } from './format-tables.mjs';
import { DISPLAY_KINDS, DISPLAY_SOURCES, FAMILIES, PLACEMENTS, WIDGETS, pagesOf, panelFingerprint } from './omx-panel.mjs';
import { CONTRACT_VERSION, contractFingerprint, contractOf, unitEnumLines } from './param-contract.mjs';
import { SPEC_FILE, controlOf, eqDefaultBands, findName, kernelControls, locateContract, packagingRange, paramKernel, resolveParam } from './omx-contract.mjs';
import { bandsMacro, baseOf, expandVariant, variantStems } from './variants.mjs';
import { omxdspInclude, render } from './template.mjs';
import { generateTest, isTestDir, loadTestDecl, testPluginDirs } from './test-plugins.mjs';
import { emitHintsC, hintPrefixes, hintTriples, statedHints } from './resource-hints.mjs';
import { presetsManifest, presetsOf } from './preset-lib.mjs';

export const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..');

const BANNER = '// SPDX-License-Identifier: GPL-3.0-or-later\n// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>\n';
const TTL_BANNER = '# SPDX-License-Identifier: GPL-3.0-or-later\n# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>\n';

/** Read and validate a plugin's declaration. Everything downstream trusts what this returns. */
export function loadDecl(dir) {
  const stem = basename(resolve(dir));
  let file = join(dir, `${stem}.decl.json`);
  let d, whole;
  if (existsSync(file) || !baseOf(dir)) {
    d = JSON.parse(readFileSync(file, 'utf8'));
    whole = isWhole(d);
    // a plugin described in omx-contract's words: its declaration is the descriptor's view, the
    // extras of <stem>.decl.json folded in (tools/descriptor.mjs)
    const described = join(dir, descriptorFile(stem));
    if (existsSync(described) && !whole) {
      const descriptor = JSON.parse(readFileSync(described, 'utf8'));
      d = { ...viewOf(descriptor, d, file), described: true, descriptor };
    }
  } else {
    // a variant's folder holds generated files only: its declaration is its base's, expanded
    const base = baseOf(dir);
    file = base.file;
    whole = isWhole(base.decl);
    const where = locateContract(treeOf(file));
    if (!where.dir) throw new Error(`${file}: its variant ${stem} is expanded from omx-contract and ${where.why}`);
    d = expandVariant(where.dir, base.decl, base.decl.variants.of.find((v) => v.stem === stem));
  }
  d.version = releaseVersion(treeOf(file));
  if (d.descriptor) d.descriptor.plugin.version = d.version;
  const prepared = prepareDecl(file, d, stem, dir);
  // a whole declaration (a base of variants, a chain) is rendered as the descriptor its faces are held to, as a
  // described plugin's is read from it: the faces state the designations, units and modules it states
  return prepared.described || !whole ? prepared : { ...prepared, described: true, descriptor: descriptorOf(prepared).descriptor };
}

/** A declaration read from `file` (or about to be written there: the wizard's plan), validated and
 * resolved for the plugin folder `dir`, whose name is `stem`. */
export function prepareDecl(file, d, stem, dir = dirname(file)) {
  if (d.variants) throw new Error(`${file}: the base of ${variantStems(d).join(', ')} is no plugin of its own; load one of its variants`);
  // a variant's sections had their `subgroups` expanded (tools/variants.mjs); a band is a variant's word only
  for (const s of d.panel?.sections ?? []) if (s.subgroups !== undefined) throw new Error(`${file}: section '${s.key}' says "subgroups", a word of a declaration with variants (its bands)`);
  if (d.subgroups !== undefined && !d.variantOf) throw new Error(`${file}: "subgroups" is what tools/variants.mjs writes for a variant (its bands); a declaration without variants carries none`);
  if (d.stem !== stem) throw new Error(`${file}: stem '${d.stem}' is not the directory's '${stem}'`);
  if (!/^[a-z][a-z0-9_]*$/.test(d.kernel)) throw new Error(`${file}: kernel '${d.kernel}' is not a C identifier`);
  if (d.binding === 'chain') {
    d.params = flattenChain(file, d);
    d.kernels = chainKernels(file, d);
  } else if (d.chain !== undefined || d.order !== undefined || d.key !== undefined) throw new Error(`${file}: "chain", "order" and "key" belong to a "binding": "chain" declaration`);
  const { params, controls } = resolveParams(file, d);
  d.params = params;
  // the resource hints, by word, from the contract's RESOURCE_HINTS (tools/resource-hints.mjs)
  const hints = statedHints(file, d, () => {
    const where = locateContract(treeOf(file));
    if (!where.dir) throw new Error(`${file}: its "resourceHints" are words of omx-contract's RESOURCE_HINTS and ${where.why}`);
    return where.dir;
  });
  d.hints = hints.stated;
  d.hintVocabulary = hints.vocabulary;
  if (generatedBinding(d)) d.controls = controls();
  const seen = new Set();
  for (const p of d.params) {
    if (!/^[A-Za-z][A-Za-z0-9_]*$/.test(p.symbol)) throw new Error(`${file}: symbol '${p.symbol}' is not an LV2 symbol`);
    if (seen.has(p.symbol)) throw new Error(`${file}: symbol '${p.symbol}' declared twice`);
    seen.add(p.symbol);
    if (!(p.min <= p.def && p.def <= p.max && p.min < p.max)) throw new Error(`${file}: '${p.symbol}' travel ${p.min}..${p.max} default ${p.def}`);
    if (p.kind && p.kind !== 'integer' && p.kind !== 'toggle') throw new Error(`${file}: '${p.symbol}' kind '${p.kind}'`);
    if (p.kind === 'toggle' && (p.min !== 0 || p.max !== 1)) throw new Error(`${file}: toggle '${p.symbol}' is not 0..1`);
    if (p.values && (p.kind !== 'integer' || p.values.length !== p.max - p.min + 1)) throw new Error(`${file}: '${p.symbol}' values name every step of an integer travel`);
    if (p.points) {
      const v = p.points.map((x) => x.value);
      if (p.kind !== 'integer' || p.values) throw new Error(`${file}: '${p.symbol}' points label an integer travel that has no values`);
      if (v[0] !== p.min || v[v.length - 1] !== p.max || v.some((x, k) => !Number.isInteger(x) || (k && x <= v[k - 1])) || !v.includes(p.def))
        throw new Error(`${file}: '${p.symbol}' points rise from min to max in whole steps and include the default`);
    }
  }
  if (d.sidechain && (seen.has(d.sidechain.symbol) || FIXED_PORTS.includes(d.sidechain.symbol))) throw new Error(`${file}: sidechain symbol '${d.sidechain.symbol}' is taken`);
  if (d.binding === 'chain') checkShipped(file, d);
  if (d.meters && d.binding !== 'instance') throw new Error(`${file}: "meters" are read from an instance face; this declaration's binding is ${d.binding ?? 'its own'}`);
  for (const o of d.oracles ?? []) {
    if (o === `${d.kernel}-oracle`) throw new Error(`${file}: oracle '${o}' is the generated identity test's name`);
    if (!existsSync(join(dir, 'test', `${o}.c`))) throw new Error(`${file}: oracle '${o}' names test/${o}.c, which does not exist`);
  }
  return { ...d, dir: resolve(dir), tree: treeOf(file) };
}

/** The panel family (schema `$defs.family`, the console's closed set) of a kernel whose own name is
 * none of them; a kernel named after a family is that family, any other kernel `none`. */
export const FAMILY_OF_KERNEL = Object.freeze({
  comp: 'dynamics', transient: 'dynamics', chorus: 'modulation', flanger: 'modulation', phaser: 'modulation',
  tremolo: 'modulation', rotary: 'modulation', geq: 'eq',
});

/** The panel family a kernel's plugin draws as: FAMILY_OF_KERNEL's, else the family of its own name. */
export function familyOfKernel(kernel) {
  const families = JSON.parse(readFileSync(join(ROOT, 'schema', 'plugin.decl.schema.json'), 'utf8')).$defs.family.enum;
  const family = FAMILY_OF_KERNEL[kernel] ?? kernel;
  return families.includes(family) ? family : 'none';
}

/** The panel the wizard drafts for a plugin generated from its instance face: one section, every
 * parameter in declaration order, so the MOD GUI draws every parameter until a person groups them;
 * its family the kernel's, its roles left to a person. */
export function defaultPanel(d) {
  if (d.binding === 'chain') {
    // one section per element, by its id (the elements are drawn, never placed, apart)
    const sections = d.chain.map((el) => ({ key: el.id, label: elementName(el), controls: [`${el.id}On`, ...el.params.map((p) => p.symbol)] }));
    if (d.order === 'permutable') sections.push({ key: 'order', label: 'Order', controls: ['order'] });
    return { family: familyOfKernel(d.kernel), roles: {}, sections };
  }
  const label = d.name.replace(/^omx /, '');
  return { family: familyOfKernel(d.kernel), roles: {}, sections: [{ key: d.kernel, label: label[0].toUpperCase() + label.slice(1), controls: d.params.map((p) => p.symbol) }] };
}

/** A plugin whose folder tools/gen.mjs writes whole from the declaration: over one instance face,
 * or a chain of them. */
export const generatedBinding = (d) => d.binding === 'instance' || d.binding === 'chain';

// ---- a composite: a chain of kernel faces -----------

/** An element's name, the label of its switch and of its panel section: its `name`, else its id. */
const elementName = (el) => el.name ?? `${el.id[0].toUpperCase()}${el.id.slice(1)}`;

/** n! */
const factorial = (n) => (n <= 1 ? 1 : n * factorial(n - 1));

/** Every order of `n` elements, lexicographic: row k is the order the `order` parameter's value k runs
 * (0 the declared order). */
export function permutations(n) {
  const out = [];
  const walk = (row, left) => {
    if (!left.length) return out.push(row);
    for (const x of left) walk([...row, x], left.filter((y) => y !== x));
  };
  walk([], [...Array(n).keys()]);
  return out;
}

/** A parameter as its element names it: the symbol without the element's id in front (`eqHpfSlope`
 * -> `hpfSlope`), what omx-contract matches a control shared by two names against. */
const localParam = (p) => {
  if (!p.element || !p.symbol.startsWith(p.element) || !/^[A-Z]/.test(p.symbol.slice(p.element.length))) return p;
  const rest = p.symbol.slice(p.element.length);
  return { ...p, symbol: `${rest[0].toLowerCase()}${rest.slice(1)}` };
};

/**
 * The flattened parameter list of a chain: element by element in chain order, each element's
 * generated switch `<id>On` (its default the element's `on`) and then its parameters, each read from
 * the element's kernel; then, for `"order": "permutable"`, the plugin's own `order` over the
 * permutations of the elements.
 */
function flattenChain(file, d) {
  if (d.params !== undefined) throw new Error(`${file}: a chain's parameters are generated from "chain"; declare no "params"`);
  if (!Array.isArray(d.chain) || d.chain.length < 2) throw new Error(`${file}: "binding": "chain" runs two or more elements, listed in "chain"`);
  const ids = new Set();
  const out = [];
  for (const el of d.chain) {
    if (!/^[a-z][a-z0-9]*$/.test(el.id ?? '')) throw new Error(`${file}: chain element id '${el.id}' is not a lower-case word`);
    if (ids.has(el.id)) throw new Error(`${file}: chain element '${el.id}' declared twice`);
    ids.add(el.id);
    if (!/^[a-z][a-z0-9_]*$/.test(el.kernel ?? '')) throw new Error(`${file}: chain element '${el.id}' names no kernel`);
    if (typeof el.on !== 'boolean') throw new Error(`${file}: chain element '${el.id}': "on" (its switch's default) is true or false`);
    if (!Array.isArray(el.params) || !el.params.length) throw new Error(`${file}: chain element '${el.id}' has no "params"`);
    out.push({ symbol: `${el.id}On`, name: elementName(el), unit: '', min: 0, max: 1, def: el.on ? 1 : 0, kind: 'toggle', own: 'switch', element: el.id });
    for (const p of el.params) {
      if (p.ref === undefined) throw new Error(`${file}: chain element '${el.id}': '${p.symbol}' has no ref; an element's parameters are its kernel's contract controls`);
      if (p.kernel !== undefined && p.kernel !== el.kernel) throw new Error(`${file}: chain element '${el.id}': '${p.symbol}' names kernel '${p.kernel}', not the element's '${el.kernel}'`);
      out.push({ ...p, kernel: el.kernel, element: el.id });
    }
  }
  if (d.order === 'permutable') out.push({ symbol: 'order', name: 'Order', unit: '', min: 0, max: factorial(d.chain.length) - 1, def: 0, kind: 'integer', own: 'stage-order' });
  else if (d.order !== undefined) throw new Error(`${file}: "order" is "permutable" or left out (the declared order)`);
  if (d.key !== undefined) {
    if (!ids.has(d.key)) throw new Error(`${file}: "key" names '${d.key}', no element of the chain`);
    if (!d.sidechain) throw new Error(`${file}: "key" hands the plugin's sidechain to '${d.key}'; declare it in "sidechain"`);
  } else if (d.sidechain) throw new Error(`${file}: a chain's "sidechain" goes to one element, named by "key"`);
  return out;
}

/** The macro of an element's band count, `bands` the key of its kernel's count sheet (eq's
 * EQ_BAND_COUNTS: strip, eq8, ...): OMX_EQ_BAND_COUNTS_STRIP_MAX. */
/** The count sheet key an element's face is compiled at: `faceBands` (a count the face takes where
 * the element exposes fewer bands), else `bands`. */
export const faceBandsOf = (el) => el.faceBands ?? el.bands;

export function chainBandsMacro(d, el) {
  const where = locateContract(d.tree ?? ROOT);
  if (!where.dir) throw new Error(`plugins/${d.stem}: element '${el.id}' counts its bands in omx-contract and ${where.why}`);
  const sheet = kernelControls(where.dir, el.kernel).find((c) => c.count)?.count;
  if (!sheet) throw new Error(`plugins/${d.stem}: element '${el.id}' declares "bands", and the ${el.kernel} kernel has no per-band control`);
  const e = findName(where.dir, el.kernel, sheet).entry;
  const key = faceBandsOf(el);
  if (!Number.isInteger(e?.value?.[key]?.max)) throw new Error(`plugins/${d.stem}: element '${el.id}': ${sheet} has no '${key}' with a max`);
  return `OMX_${sheet}_${key.toUpperCase()}_MAX`;
}

/** The kernels a chain reads, in chain order, each once; a declared `kernels` must be the same. */
function chainKernels(file, d) {
  const ks = [...new Set(d.chain.map((el) => el.kernel))];
  if (d.kernels !== undefined && JSON.stringify(d.kernels) !== JSON.stringify(ks)) throw new Error(`${file}: "kernels" is ${JSON.stringify(d.kernels)}, the chain's are ${JSON.stringify(ks)}; leave it out`);
  return ks;
}

/** The parameter symbols a released plugin shipped: its generated header at the git tag `tag`, or
 * undefined where the tree has no git or no such tag (a tarball build). */
function shippedSymbols(d, tag) {
  try {
    const text = execFileSync('git', ['-C', d.tree, 'show', `${tag}:plugins/${d.stem}/generated/omx_${d.kernel}_params.h`], { encoding: 'utf8', stdio: ['ignore', 'pipe', 'ignore'] });
    // the rows of the parameter table alone: the header holds other tables of strings after it
    const table = text.slice(text.indexOf(`OMX_${d.kernel.toUpperCase()}_PARAMS[`)).split('\n};')[0];
    return [...table.matchAll(/^\s*\{ "([A-Za-z0-9_]+)", "/gm)].map((m) => m[1]);
  } catch {
    return undefined;
  }
}

/** The differences between a released list of symbols and the flattened one, index by index. */
export function symbolDiff(was, now) {
  const out = [];
  for (let i = 0; i < Math.max(was.length, now.length); i++) {
    if (was[i] === now[i]) continue;
    out.push(i >= was.length ? `+${i} ${now[i]}` : i >= now.length ? `-${i} ${was[i]}` : `${i} ${was[i]} -> ${now[i]}`);
  }
  return out;
}

/** A chain converted from a released plugin (`shipped`: its release tag) keeps that plugin's ids
 * and symbols, or names every difference in `shippedDiff`: CLAP ids are indices and LV2
 * symbols are saved state, so a difference is a decision, never a side effect. */
function checkShipped(file, d) {
  if (d.shipped === undefined) return;
  const was = shippedSymbols({ ...d, tree: treeOf(file) }, d.shipped);
  if (!was) {
    // a tarball or shallow build has no tag to read: said, never silent
    console.error(`gen: NOTE ${d.stem}: ${d.shipped} is not readable here (no git or no such tag); its parameter list is not checked against that release`);
    return;
  }
  const diff = symbolDiff(was, d.params.map((p) => p.symbol));
  const named = d.shippedDiff ?? [];
  if (JSON.stringify(diff) !== JSON.stringify(named))
    throw new Error(`${file}: the flattened parameters differ from ${d.stem} ${d.shipped}'s, and "shippedDiff" does not name exactly the differences; they are:\n  ${diff.join('\n  ') || '(none: drop "shippedDiff")'}`);
}

/** The tree a declaration sits in; a declaration copied out of its tree (a test's scratch copy)
 * reads this tree's packaging. */
const treeOf = (file) => {
  const tree = resolve(dirname(file), '..', '..');
  return existsSync(join(tree, SPEC_FILE)) ? tree : ROOT;
};

/** The travel fields a parameter BY REFERENCE reads from omx-contract and must never retype. */
export const TRAVEL_FIELDS = ['min', 'max', 'def', 'unit', 'kind'];

/** Each parameter with its travel: read from the installed omx-contract through its `ref`
 * (tools/omx-contract.mjs), or typed and marked `own` with the reason omx-contract declares no such
 * control (a switch of the plugin's face, the strip's stage order). A parameter is one or the other. */
function resolveParams(file, d) {
  let where;
  const contract = () => {
    if (!where) {
      where = locateContract(treeOf(file));
      if (!where.dir) throw new Error(`${file}: its parameters are by reference and ${where.why}`);
    }
    return where.dir;
  };
  // each parameter's contract control ({ name }), or undefined for an own one: what the generated
  // binding matches resolve() arguments against
  const controls = () => d.params.map((p) => (p.ref === undefined ? undefined : controlOf(contract(), paramKernel(d, p, contract()), localParam(p))));
  const params = d.params.map((p) => {
    if (p.ref === undefined) {
      if (typeof p.own !== 'string' || !p.own) throw new Error(`${file}: '${p.symbol}' has no ref and no "own" reason; a parameter is by reference to omx-contract or says why it is the plugin's own`);
      return p;
    }
    if (p.own !== undefined) throw new Error(`${file}: '${p.symbol}' is by reference and "own"; it is one or the other`);
    const dir = contract();
    const kernel = paramKernel(d, p, dir);
    // a set's unit is face text the set does not declare (a slope's dB/oct); a travel's is the contract's
    const isSet = findName(dir, kernel, p.ref).entry?.kind === 'set';
    const typed = TRAVEL_FIELDS.filter((k) => k in p && !(isSet && k === 'unit'));
    if (typed.length) throw new Error(`${file}: '${p.symbol}' is by reference and retypes ${typed.join(', ')}; omx-contract holds them`);
    const t = resolveParam(dir, kernel, p);
    const out = { ...p, min: t.min, max: t.max, def: t.def, unit: t.unit, from: t.from };
    // a control whose travel another choice widens (eq's q, notchQ while the band is a notch): the
    // port spans every travel it may take, and the face clamps by the choice
    const c = generatedBinding(d) ? controlOf(dir, kernel, localParam(p)) : undefined;
    const widest = [];
    for (const w of c?.when ?? []) {
      const u = resolveParam(dir, kernel, { symbol: p.symbol, ref: w.global });
      out.min = Math.min(out.min, u.min);
      out.max = Math.max(out.max, u.max);
      widest.push(u);
    }
    // the travel the port states is the one its link resolves to (tools/descriptor.mjs): the link is the
    // widening travel itself, which must be the whole extent the port spans
    if (widest.length) {
      const whole = widest.find((u) => u.min === out.min && u.max === out.max);
      if (!whole) throw new Error(`${file}: '${p.symbol}' spans ${out.min}..${out.max}, which no single travel it may take states; its link could not resolve to it`);
      out.from = whole.from;
    }
    if (t.kind) out.kind = t.kind;
    if (t.byKind) out.byKind = t.byKind;
    if (t.points) out.points = t.points;
    if (t.values) out.values = t.values;
    return out;
  });
  return { params, controls };
}

/** SHA-256 of the resolved parameter list: the org.openmixer.declaration/1 digest, the formula the
 * OpenMixer engine computes for its stale-parameter check, so it reads ours unchanged. */
/** The declaration's plain description for org.openmixer.declaration/1: each parameter's name and
 * travel, in declaration order. */
export function sourceOf(d) {
  return `${d.name}: ${d.params.map((p) => `${p.name} ${p.min} to ${p.max}${p.unit ? ` ${p.unit}` : ''}`).join(', ')}`;
}

export function digestOf(params) {
  const rows = params.map((p) => [p.symbol, p.unit, p.min, p.max, p.def, p.kind ? 1 : 0, p.kind === 'toggle']);
  return createHash('sha256').update(JSON.stringify(rows)).digest('hex');
}

/** The plugin-defined units, by name (urn:openmixer:unit#<name>): defined (units:Unit) in the plugin's own TTL, so a
 * host that reads units:unit finds the unit and how to write it. */
export const LV2_CUSTOM_UNITS = {
  dBFS: { label: 'decibels (full scale)', render: '%f dBFS', symbol: 'dBFS' },
  dBu: { label: 'decibels (unloaded)', render: '%f dBu', symbol: 'dBu' },
  LUFS: { label: 'loudness units (full scale)', render: '%f LUFS', symbol: 'LUFS' },
  dBoct: { label: 'decibels per octave', render: '%f dB/oct', symbol: 'dB/oct' },
  ratio: { label: 'ratio', render: '%f:1', symbol: ':1' },
};

export { lv2Designation, lv2UnitNames, lv2UnitRef };

/** How a host draws a travel: as declared, else a frequency logarithmic (the console's every
 * frequency control is), else linear. */
export const scaleOf = (p) => p.scale ?? (p.unit === 'Hz' ? 'log' : 'linear');

/** The labelled values of an integer travel, `{value, label}` from min up: one per step (`values`)
 * or only the ones it takes (`points`). Either makes the LV2 port an enumeration. */
export const choicesOf = (p) => p.points ?? p.values?.map((label, k) => ({ value: p.min + k, label }));

/** The port symbols every plugin has, which no parameter or key may take. */
const FIXED_PORTS = ['in_l', 'in_r', 'out_l', 'out_r', 'enabled', 'latency'];

/** The LV2 port list, in index order: audio in (and the sidechain key, when declared), audio out,
 * the declared parameters, enabled, latency; `lv2.enabledPort: "first"` puts enabled before the
 * parameters. One rule, read by the TTL, the MOD GUI and (as macros) the LV2 C face. */
export function lv2Ports(d) {
  const IN = 'lv2:AudioPort , lv2:InputPort', OUT = 'lv2:AudioPort , lv2:OutputPort';
  const audio = [
    { symbol: 'in_l', name: 'In L', a: IN }, { symbol: 'in_r', name: 'In R', a: IN },
    ...(d.sidechain ? [{ symbol: d.sidechain.symbol, name: d.sidechain.name, a: IN, sidechain: true }] : []),
    { symbol: 'out_l', name: 'Out L', a: OUT }, { symbol: 'out_r', name: 'Out R', a: OUT },
  ];
  const params = d.params.map((p) => ({ symbol: p.symbol, name: p.name, a: 'lv2:ControlPort , lv2:InputPort', param: p }));
  const enabled = { symbol: 'enabled', name: 'Enabled', a: 'lv2:ControlPort , lv2:InputPort', enabled: true };
  const first = d.lv2.enabledPort === 'first';
  return [
    ...audio,
    ...(first ? [enabled] : []),
    ...params,
    ...(first ? [] : [enabled]),
    { symbol: 'latency', name: 'Latency', a: 'lv2:ControlPort , lv2:OutputPort', latency: true },
  ].map((p, index) => ({ ...p, index }));
}

const cf = (n) => `${Number.isInteger(n) ? `${n}.0` : `${n}`}f`;
const macro = (s) => s.replace(/([a-z0-9])([A-Z])/g, '$1_$2').toUpperCase();
const cstr = (s) => `"${s.replace(/\\/g, '\\\\').replace(/"/g, '\\"')}"`;

/** The rows of org.openmixer.panel-description/1 in the parameter header: each table of
 * omx-panel's layout as C, and OMX_<K>_PANEL, the description a face serves. */
function emitPanelRows(d) {
  const K = d.kernel.toUpperCase();
  const x = panelDescription(d);
  const NONE = 'OMX_CLAP_PANEL_INVALID_ID';
  const id = (n) => (n === null || n === undefined ? NONE : `${n}u`);
  const enumOf = (prefix, list, v) => `OMX_CLAP_${prefix}_${list[list.indexOf(v)].toUpperCase()}`;
  const tables = [
    ['SECTIONS', 'section', x.sections.map((s) => `{ ${cstr(s.key)}, ${cstr(s.labelKey)}, ${cstr(s.label)} }`)],
    ['CONTROLS', 'control', x.controls.map((c) => `{ ${c.param}u, ${c.section}u, ${enumOf('WIDGET', WIDGETS, c.widget)}, ${cstr(c.labelKey)}, ${cstr(c.label)}, ${id(c.visible)}, 0x${c.mask.toString(16)}u }`)],
    ['ROLES', 'role', x.roles.map((r) => `{ ${cstr(r.role)}, ${r.index}u, ${r.param}u }`)],
    ['DISPLAYS', 'display', x.displays.map((v) => `{ ${enumOf('DISPLAY', DISPLAY_KINDS, v.kind)}, ${v.section}u, OMX_CLAP_DISPLAY_FROM_${DISPLAY_SOURCES[DISPLAY_SOURCES.indexOf(v.source)].toUpperCase()}, { ${id(v.meters[0])}, ${id(v.meters[1])} }, ${v.curve ?? 0}u, ${NONE}, 0.0, ${cstr(v.labelKey)}, ${cstr(v.label)} }`)],
    ['DEFAULTS', 'default', x.defaults.map((p) => `{ ${enumOf('PLACEMENT', PLACEMENTS, p.placement)}, ${p.param}u, ${p.value} }`)],
  ];
  const array = ([T, row, rows]) => (rows.length ? `static const struct omx_panel_${row}_row OMX_${K}_PANEL_${T}[] = {\n${rows.map((r) => `  ${r},`).join('\n')}\n};\n` : '');
  const ref = ([T, , rows]) => (rows.length ? `OMX_${K}_PANEL_${T}, ${rows.length}u` : 'NULL, 0u');
  const unpublished = x.unpublished.map((u) => ` *   ${u.role} (${u.symbol}): ${u.why}`).join('\n');
  return `/* org.openmixer.panel-description/1 (tools/panel-description.mjs): the declaration's panel as the rows
 * include/omx_panel_description.h serves, parameter ids the declaration's. Family ${x.family}.${unpublished ? `
 * Declared roles not published:
${unpublished}` : ''} */
#define OMX_${K}_PANEL_FINGERPRINT "${panelFingerprint(x)}"
${tables.map(array).join('')}/* clap.remote-controls/2: the pages of the page rule over the sections. */
static const struct omx_panel_page_row OMX_${K}_PANEL_PAGES[] = {
${pagesOf(x, d).map((g) => `  { ${cstr(g.sectionName)}, ${cstr(g.pageName)}, { ${g.ids.map(id).join(', ')} } },`).join('\n')}
};
#define OMX_${K}_PANEL_PAGE_COUNT ${pagesOf(x, d).length}u
static const struct omx_panel_rows OMX_${K}_PANEL = {
  OMX_${K}_PANEL_FINGERPRINT, OMX_CLAP_FAMILY_${FAMILIES[FAMILIES.indexOf(x.family)].toUpperCase()},
${tables.map((t) => `  ${ref(t)},`).join('\n')}
};
`;
}

export function emitParamsHeader(d) {
  const K = d.kernel.toUpperCase();
  const P = `OMX_${K}_PARAM`;
  const flags = (p) => (p.kind === 'integer' ? 'OMX_PLUGIN_PARAM_INTEGER' : p.kind === 'toggle' ? 'OMX_PLUGIN_PARAM_TOGGLE' : '0u');
  const ports = lv2Ports(d);
  const firstParam = ports.find((p) => p.param).index;
  return `${BANNER}#ifndef OMX_${K}_PARAMS_H
#define OMX_${K}_PARAMS_H
/*
 * GENERATED — DO NOT EDIT BY HAND.
 * Produced by tools/gen.mjs from plugins/${d.stem}/${d.stem}.decl.json.
 * Regenerate: \`make -C plugins/${d.stem} gen\`, then commit the result. Order is append-only.
 * The include guard is omx-dsp's own <omxdsp/params/omx_${d.kernel}_params.h>: included first, this
 * table is the one the kernel's instance header reads.
 */
#include <stddef.h>

#include <omxcontract/omx_resource_hints.h>

#include "omx_panel_description.h"
#include "omx_param_contract.h"
#include "omx_plugin_param.h"
${d.variantOf ? `
/* The band count of this variant of plugins/${d.variantOf.stem}: the instance face's compile-time count. */
#define OMX_${(d.face ?? d.kernel).toUpperCase()}_INSTANCE_BANDS ${bandsMacro(d)}
` : ''}${(d.chain ?? []).filter((el, i, all) => faceBandsOf(el) && all.findIndex((o) => o.kernel === el.kernel) === i).map((el) => `
/* The band count of the chain's ${el.id}: its instance face's compile-time count, omx-contract's ${chainBandsMacro(d, el)}. */
#define OMX_${el.kernel.toUpperCase()}_INSTANCE_BANDS ${chainBandsMacro(d, el)}
`).join('')}
enum {
${d.params.map((p, i) => `  ${P}_${macro(p.symbol)} = ${i},`).join('\n')}
  ${P}_COUNT = ${d.params.length}
};

static const omx_plugin_param OMX_${K}_PARAMS[${P}_COUNT] = {
${d.params.map((p) => `  { "${p.symbol}", "${p.name}", "${p.unit}", ${cf(p.min)}, ${cf(p.max)}, ${cf(p.def)}, ${flags(p)} },`).join('\n')}
};

/* Each parameter's CLAP module (clap_param_info.module): its descriptor control's group as a module path, "" for a
 * plugin with no descriptor. */
static const char *const OMX_${K}_PARAM_MODULES[${P}_COUNT] = {
${d.params.map((p, i) => `  ${cstr(moduleOf(d, i))},`).join('\n')}
};

/* One macro per declared bound: what a C face reads where a constant is needed. */
${d.params.map((p) => [['MIN', p.min], ['MAX', p.max], ['DEFAULT', p.def]].map(([k, v]) => `#define ${P}_${macro(p.symbol)}_${k} ${cf(v)}`).join('\n')).join('\n')}

/* The identity every face publishes. */
#define OMX_${K}_NAME "${d.name}"
#define OMX_${K}_VENDOR "${d.vendor}"
#define OMX_${K}_URL "${d.url}"
#define OMX_${K}_VERSION "${d.version}"
#define OMX_${K}_DESCRIPTION "${d.description.replace(/"/g, '\\"')}"
#define OMX_${K}_CLAP_ID "${d.clap.id}"
#define OMX_${K}_CLAP_FEATURES ${d.clap.features.map((f) => `"${f}"`).join(', ')}
#define OMX_${K}_LV2_URI "${d.lv2.uri}"
/* org.openmixer.declaration/1: the declaration's plain description and the digest of its parameters. */
#define OMX_${K}_DECL_SOURCE "${sourceOf(d).replace(/"/g, '\\"')}"
#define OMX_${K}_DECL_DIGEST "${digestOf(d.params)}"

/* org.openmixer.param-contract/1 (tools/param-contract.mjs): each parameter's unit, scale and step, in
 * id order; the host's bypass, the last id, is the face's. */
#define OMX_${K}_PARAM_CONTRACT_VERSION ${CONTRACT_VERSION}u
#define OMX_${K}_PARAM_CONTRACT_FINGERPRINT "${contractFingerprint(contractOf(d, scaleOf))}"
static const struct omx_param_contract_row OMX_${K}_PARAM_CONTRACT[${P}_COUNT] = {
${contractOf(d, scaleOf).slice(0, -1).map((r) => `  { ${r.unitEnum}u, OMX_CLAP_SCALE_${r.scale.toUpperCase()}, ${r.step}.0 }, /* ${r.symbol} ${r.unit} */`).join('\n')}
};

${emitHintsC(K, d.hints ?? [])}
${emitPanelRows(d)}
/* LV2 port indices (tools/gen.mjs lv2Ports): audio, then parameter i at FIRST_PARAM + i, then these. */
${ports.filter((p) => !p.param).map((p) => `#define OMX_${K}_LV2_PORT_${macro(p.symbol)} ${p.index}u`).join('\n')}
#define OMX_${K}_LV2_PORT_FIRST_PARAM ${firstParam}u
#define OMX_${K}_LV2_PORT_COUNT ${ports.length}u

#endif /* OMX_${K}_PARAMS_H */
`;
}

const ttlNum = (n) => (Number.isInteger(n) ? `${n}` : `${n}`);
const ttlStr = (s) => `"${s.replace(/\\/g, '\\\\').replace(/"/g, '\\"')}"`;

/** The manifest: the plugin, and each factory preset of presets/<clap id>/ (its pset:Preset, in the
 * presets.ttl tools/gen-presets.mjs writes from the same files; docs/presets.md). */
export function emitManifest(d, { modgui = false } = {}) {
  const presets = d.clap?.id ? presetsOf(d.clap.id) : [];
  return `${TTL_BANNER}# GENERATED by tools/gen.mjs from plugins/${d.stem}/${d.stem}.decl.json — DO NOT EDIT BY HAND.
@prefix lv2:  <http://lv2plug.in/ns/lv2core#> .
${presets.length ? '@prefix pset: <http://lv2plug.in/ns/ext/presets#> .\n' : ''}@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .

<${d.lv2.uri}>
    a lv2:Plugin ;
    lv2:binary <${d.stem}.so> ;
    rdfs:seeAlso ${modgui ? '<modgui.ttl> , ' : ''}<${d.stem}.ttl> .
${presetsManifest(d.lv2.uri, presets)}`;
}

/** What a face states of parameter `i` beyond the declaration: its descriptor control's unit (a ratio's `ratio`, which
 * its link resolves to "") and meaning, for a plugin described in omx-contract's words; the declared unit otherwise. */
const faceUnit = (d, i) => travelUnit(d.params[i].unit, d.descriptor?.controls[i]?.travel?.unit);
const faceDesignation = (d, i) => lv2Designation(d.descriptor?.controls[i]?.meaning);

export function emitPluginTtl(d) {
  const port = (p) => {
    const lines = [`        a ${p.a} ;`, `        lv2:index ${p.index} ;`, `        lv2:symbol "${p.symbol}" ;`, `        lv2:name "${p.name}" ;`];
    if (p.param) {
      const q = p.param;
      lines.push(`        lv2:default ${ttlNum(q.def)} ;`, `        lv2:minimum ${ttlNum(q.min)} ;`, `        lv2:maximum ${ttlNum(q.max)} ;`);
      const choices = choicesOf(q);
      if (q.kind === 'integer') lines.push(`        lv2:portProperty lv2:integer${choices ? ' , lv2:enumeration' : ''} ;`);
      if (scaleOf(q) === 'log') lines.push('        lv2:portProperty pprops:logarithmic ;');
      if (q.kind === 'toggle') lines.push('        lv2:portProperty lv2:toggled ;');
      const i = d.params.indexOf(q);
      if (lv2UnitRef(faceUnit(d, i))) lines.push(`        units:unit ${lv2UnitRef(faceUnit(d, i))} ;`);
      if (faceDesignation(d, i)) lines.push(`        lv2:designation ${faceDesignation(d, i)} ;`);
      if (choices) {
        const points = choices.map((c) => `            [ rdfs:label ${ttlStr(c.label)} ; rdf:value ${c.value} ]`);
        lines.push(`        lv2:scalePoint\n${points.join(' ,\n')} ;`);
      }
      const group = groupOf(d, q.symbol);
      if (group) lines.push(`        pg:group <${groupUri(d, group)}> ;`);
    } else if (p.sidechain) {
      // optional: a host that routes nothing to the key gets the effect's own detector
      lines.push('        lv2:portProperty lv2:isSideChain , lv2:connectionOptional ;');
    } else if (p.enabled) {
      lines.push('        lv2:default 1 ;', '        lv2:minimum 0 ;', '        lv2:maximum 1 ;', '        lv2:designation lv2:enabled ;', '        lv2:portProperty lv2:toggled ;');
    } else if (p.latency) {
      lines.push('        lv2:designation lv2:latency ;', '        lv2:portProperty lv2:reportsLatency , lv2:integer ;', '        units:unit units:frame ;');
    }
    return `    [\n${lines.join('\n').replace(/ ;$/, '')}\n    ]`;
  };
  return `${TTL_BANNER}# GENERATED by tools/gen.mjs from plugins/${d.stem}/${d.stem}.decl.json — DO NOT EDIT BY HAND.
@prefix doap:  <http://usefulinc.com/ns/doap#> .
@prefix foaf:  <http://xmlns.com/foaf/0.1/> .
@prefix lv2:   <http://lv2plug.in/ns/lv2core#> .
${hintPrefix(d, (p) => p !== 'xsd')}${d.params.some((q, i) => faceDesignation(d, i)?.startsWith('param:')) ? '@prefix param: <http://lv2plug.in/ns/ext/parameters#> .\n' : ''}${d.panel ? '@prefix pg:    <http://lv2plug.in/ns/ext/port-groups#> .\n' : ''}${d.params.some((q) => scaleOf(q) === 'log') ? '@prefix pprops: <http://lv2plug.in/ns/ext/port-props#> .\n' : ''}${d.params.some((q) => choicesOf(q)) ? '@prefix rdf:   <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n' : ''}@prefix rdfs:  <http://www.w3.org/2000/01/rdf-schema#> .
@prefix units: <http://lv2plug.in/ns/extensions/units#> .
${hintPrefix(d, (p) => p === 'xsd')}
<${d.url}>
    a doap:Project ;
    doap:name "omx-plugins" ;
    doap:maintainer [ a foaf:Person ; foaf:name "Pau Aliagas" ; foaf:homepage <https://github.com/linuxnow> ] .

<${d.lv2.uri}>
    a lv2:Plugin , ${d.lv2.class} ;
    doap:name ${ttlStr(d.name)} ;
    doap:license <https://www.gnu.org/licenses/gpl-3.0.html> ;
    lv2:project <${d.url}> ;
    lv2:minorVersion ${d.version.split('.')[1]} ;
    lv2:microVersion ${d.version.split('.')[2]} ;
    rdfs:comment ${ttlStr(d.description)} ;
    lv2:optionalFeature lv2:hardRTCapable ;
${hintTriples(d.hintVocabulary, d.hints ?? []).map((l) => `${l}\n`).join('')}    lv2:port
${lv2Ports(d).map(port).join(' ,\n')} .
${portGroupsOf(d).length ? `
# lv2:displayPriority on a port group is the term Zynthian reads to order a plugin's groups (zynthian_lv2.py), and the
# one FreeMixer/omx-plugins#257 rules; LV2 defines none in lv2core (port-props' pprops:displayPriority is a port's), so
# it is stated here, for a validator, as what it is.
lv2:displayPriority
    a <http://www.w3.org/1999/02/22-rdf-syntax-ns#Property> ;
    rdfs:label "display priority" ;
    rdfs:comment "A port group's rank among its plugin's groups, higher first, as Zynthian reads it." .
` : ''}${portGroupsOf(d).map((g) => `
<${groupUri(d, g)}>
    a pg:Group ;
    lv2:symbol "${g.key}" ;
    lv2:name ${ttlStr(g.label)} ;
    lv2:displayPriority ${g.priority} ;
    rdfs:label ${ttlStr(g.label)} .
`).join('')}${customUnitsOf(d).map((name) => `
<urn:openmixer:unit#${name}>
    a units:Unit ;
    rdfs:label ${ttlStr(LV2_CUSTOM_UNITS[name].label)} ;
    units:render ${ttlStr(LV2_CUSTOM_UNITS[name].render)} ;
    units:symbol ${ttlStr(LV2_CUSTOM_UNITS[name].symbol)} .
`).join('')}`;
}

/** The @prefix lines of the stated resource hints (tools/resource-hints.mjs) that `pick` keeps, in the TTL's column. */
const hintPrefix = (d, pick) => hintPrefixes(d.hintVocabulary, d.hints ?? []).filter(([p]) => pick(p)).map(([p, ns]) => `@prefix ${`${p}:`.padEnd(6)} <${ns}> .\n`).join('');

/** The plugin-defined units this plugin's ports use, in a fixed order. */
const customUnitsOf = (d) => Object.keys(LV2_CUSTOM_UNITS).filter((name) => d.params.some((q, i) => lv2UnitRef(faceUnit(d, i)) === `<urn:openmixer:unit#${name}>`));

/** A control's group as a CLAP module path (plugin-hostd's group-path, written: the labels joined by "/"). */
const moduleOf = (d, i) => (d.descriptor?.controls[i]?.group ?? []).join('/');

/** A parameter's LV2 port group (pg:Group): the panel section it is drawn in, or the section's subgroup holding it
 * (tools/descriptor.mjs portGroupsOf). */
const groupOf = (d, symbol) => portGroupsOf(d).find((g) => g.controls.includes(symbol));
const groupUri = (d, section) => `${d.lv2.uri}#${section.key}`;

// ---- a plugin generated from its instance face (`binding: instance`) ---------------------------

const TEMPLATES = join(ROOT, 'recipes', 'templates', 'plugin');
const template = (name) => readFileSync(join(TEMPLATES, name), 'utf8');

/** The C float literal of a number: always with a point, never an exponent a float cannot hold. */
const cfloat = (n) => {
  const t = `${Number(n.toPrecision(9))}`;
  return `${/[.e]/.test(t) ? t : `${t}.0`}f`;
};

/** How a README row and section read a resolved parameter: its range and its default, in words. */
export function paramRow(p) {
  const unit = p.unit ? ` ${p.unit}` : '';
  if (p.kind === 'toggle') return { name: p.name, range: 'off / on', default: p.def ? 'on' : 'off' };
  const choices = choicesOf(p);
  if (choices) return { name: p.name, range: choices.map((c) => c.label).join(' / '), default: choices.find((c) => c.value === p.def)?.label ?? `${p.def}` };
  return { name: p.name, range: `${p.min} to ${p.max}${unit}${p.kind === 'integer' ? ', whole steps' : ''}`, default: `${p.def}${unit}` };
}

/** The manual's own words around the numbers: each `{range:<symbol>}`, `{default:<symbol>}`,
 * `{min:<symbol>}` and `{max:<symbol>}` in a line of `manual` reads that resolved parameter (its
 * range and default as the parameter table words them, or its bare limit), so the README never
 * retypes a number omx-contract or the declaration holds. A symbol no parameter has, or any other
 * `{word:...}`, is refused. */
export function fillManual(d, line) {
  const out = line.replace(/\{(range|default|min|max):([A-Za-z][A-Za-z0-9_]*)\}/g, (_m, what, symbol) => {
    const p = d.params.find((q) => q.symbol === symbol);
    if (!p) throw new Error(`plugins/${d.stem}: its manual reads '${symbol}', which is none of its parameters`);
    return what === 'min' || what === 'max' ? `${p[what]}` : paramRow(p)[what];
  });
  const left = out.match(/\{[a-z]+:[^}]*\}/);
  if (left) throw new Error(`plugins/${d.stem}: its manual's ${left[0]} is none of {range|default|min|max:<symbol>}`);
  return out;
}

/** The view every plugin template renders from. */
export function templateView(d) {
  return {
    stem: d.stem, kernel: d.kernel, face: d.face ?? d.kernel, K: d.kernel.toUpperCase(), Kernel: d.kernel.replace(/(^|_)([a-z])/g, (_m, _u, c) => c.toUpperCase()),
    name: d.name, uri: d.lv2.uri, clapId: d.clap.id, description: d.description,
    omxdspMin: packagingRange('omx-dsp-devel', d.tree ?? ROOT).floor, panel: Boolean(d.panel), params: d.params.map(paramRow),
    oracles: (d.oracles ?? []).map((oracle) => ({ oracle })),
  };
}

/** A templated file with the GENERATED line under its SPDX header (`//` for C, `#` for make). */
function withBanner(text, d, comment) {
  const lines = text.split('\n');
  const at = lines.findIndex((l) => l.startsWith(`${comment} Copyright`)) + 1;
  lines.splice(at, 0, `${comment} GENERATED by tools/gen.mjs from plugins/${d.stem}/${d.stem}.decl.json — DO NOT EDIT BY HAND.`);
  return lines.join('\n');
}

/**
 * The block plan of the identity test, three parts:
 *
 *   1. MOVES: twelve uneven blocks, fixed in frames, every parameter moved across its travel between
 *      them, the bypass toggled.
 *   2. RELEASE: at the defaults, a full-scale burst and then a tail that falls from full scale onto
 *      a quiet bed (about -60 dBFS, under any gate's default threshold, each threshold crossed at its
 *      own time), both in milliseconds so every rate gets the same time. The tail
 *      lasts the sum of every time travel's default (ms or s): the longest release or decay, with
 *      the predelay, hold or window that comes before it (a gated reverb holds 120 ms, then releases),
 *      and at least TAIL_FLOOR_MS. A dynamics kernel engages on the burst and releases in the tail, so
 *      its release is seen.
 *   3. CHOICES: for every choice (a set's values, or a toggle), each of its values in turn, held over
 *      its own burst and two tails: every other parameter at its default for the first, stepped once
 *      for the second. A control that is read only under one value of a choice (a reverb's plate
 *      depth under the plate algorithm) is then in the plan for long enough to be seen.
 *
 * A parameter whose contract control `rearms` (`controls[i].rearms`: changing it re-arms the kernel's
 * state) is held at its default in every block: moving it would test the re-arm, not the identity
 * of the faces and the kernel. The sabotage arm still moves it, by one step.
 *
 * A row is `frames, ms, signal, {values}, bypass`. `ms` > 0 means the block lasts that long at every
 * rate and `frames` is unused. `signal` is 0 for the moving stimulus, 1 for the full-scale burst and
 * 2 for the quiet tail.
 */
const PLAN_FRAMES = [64, 1, 333, 512, 17, 480, 129, 1024, 7, 2500, 600, 3000];
const PLAN_BYPASS = [0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0];
const PLAN_AT = ['def', 0.25, 1, 0, 0.75, 'def', 0.5, 0.9, 0.1, 0.6, 0.35, 'def'];
export const BURST_MS = 10;
export const TAIL_FLOOR_MS = 100;
const SIGNAL = { moves: 0, burst: 1, tail: 2 };

/** The quiet tail of a release section, ms: the sum of every time travel's default, at least TAIL_FLOOR_MS. */
export function tailMs(params) {
  const sum = params
    .filter((p) => p.unit === 'ms' || p.unit === 's')
    .reduce((t, p) => t + (p.unit === 's' ? p.def * 1000 : p.def), 0);
  return Math.max(TAIL_FLOOR_MS, Math.ceil(sum));
}

/** A choice's values (a set's ids by index, a toggle's 0 and 1), or undefined for a travel. */
const valuesOf = (p) => (p.kind === 'toggle' ? [0, 1] : choicesOf(p)?.map((c) => c.value));

/** A parameter's value at `at` (a fraction of its travel, or 'def'). */
function valueAt(p, at) {
  if (at === 'def') return p.def;
  const choices = choicesOf(p);
  if (choices) return choices[Math.round(at * (choices.length - 1))].value;
  const x = p.min + at * (p.max - p.min);
  return p.kind === 'integer' ? Math.round(x) : x;
}

/** One step of every parameter but those in `keep`: a travel to three quarters of its travel (a
 * quarter when that is its default), a choice to its next value, a toggle flipped. */
function stepped(params, v, keep) {
  return params.map((p, i) => {
    if (keep.has(i)) return v[i];
    const vals = valuesOf(p);
    if (vals) return vals[(vals.indexOf(v[i]) + 1) % vals.length];
    const x = valueAt(p, 0.75);
    return x === v[i] ? valueAt(p, 0.25) : x;
  });
}

export function oraclePlan(params, controls = [], { engaged = [] } = {}) {
  const held = new Set(params.map((_, i) => i).filter((i) => controls[i]?.rearms));
  // a chain's element switches: on in the release and choice sections, so each element is heard
  // there (an EQ switched off by the stepped row would hide every band's Q), and never stepped
  const on = new Set(engaged);
  const row = (frames, ms, signal, v, bypass) => ({ row: `${frames}, ${cfloat(ms)}, ${signal}, {${v.map(cfloat).join(', ')}}, ${bypass}` });
  const moves = PLAN_FRAMES.map((frames, b) => {
    const v = params.map((p, i) => {
      if (held.has(i)) return p.def;
      if (p.kind === 'toggle') return (b + i) % 2;
      return valueAt(p, PLAN_AT[(b + 2 * i) % PLAN_AT.length]);
    });
    return row(frames, 0, SIGNAL.moves, v, PLAN_BYPASS[b]);
  });
  const tail = tailMs(params);
  const defaults = params.map((p, i) => (on.has(i) ? 1 : p.def));
  const release = [row(0, BURST_MS, SIGNAL.burst, defaults, 0), row(0, tail, SIGNAL.tail, defaults, 0)];
  const choices = [];
  params.forEach((p, c) => {
    const vals = valuesOf(p);
    if (!vals || held.has(c)) return;
    for (const value of vals) {
      const v = defaults.map((d, i) => (i === c ? value : d));
      const moved = stepped(params, v, new Set([...held, ...on, c]));
      choices.push(row(0, BURST_MS, SIGNAL.burst, v, 0), row(0, tail, SIGNAL.tail, v, 0), row(0, tail, SIGNAL.tail, moved, 0));
    }
  });
  return [...moves, ...release, ...choices];
}

/** The declaration's latency checks in the oracle's shape (`latencyChecks` in the schema): each a row of
 * every parameter's value, the named ones set and the rest at their defaults, then what the faces
 * report (-1: where an impulse lands) and the set values in words. */
export function latencyRows(d) {
  return (d.latencyChecks ?? []).map((c, k) => {
    for (const [sym, x] of Object.entries(c.values)) {
      const p = d.params.find((q) => q.symbol === sym);
      if (!p) throw new Error(`plugins/${d.stem}: latencyChecks[${k}] sets '${sym}', which is no parameter`);
      if (!(p.min <= x && x <= p.max)) throw new Error(`plugins/${d.stem}: latencyChecks[${k}] sets '${sym}' to ${x}, outside ${p.min}..${p.max}`);
    }
    const v = d.params.map((p) => (p.symbol in c.values ? c.values[p.symbol] : p.def));
    const label = Object.entries(c.values).map(([sym, x]) => `${sym} ${x}`).join(', ') || 'the defaults';
    return { row: `{${v.map(cfloat).join(', ')}}, ${c.reports === 'impulse' ? -1 : c.reports}, ${JSON.stringify(label)}` };
  });
}

/** clap.meters/1's kind of each meter kind a declaration may name. */
const METER_KINDS = { gain_reduction: 'CLAP_METER_KIND_GAIN_REDUCTION', level: 'CLAP_METER_KIND_LEVEL' };
/** The most values a plugin's meters carry together: include/omx_meter_ring.h's OMX_METER_RING_MAX. */
const METER_VALUES_MAX = 16;
/** A C double literal: always with a point. */
const cdouble = (n) => (typeof n === 'string' || /[.e]/.test(`${n}`) ? `${n}` : `${n}.0`);

/** The declaration's meters in the faces' shape (`meters` in the schema, docs/meters.md): each its CLAP
 * id (its index), its name, its kind, its channels and where they start among the plugin's meter
 * values (the order the face publishes them in, one block at a time), and its display range. A
 * gain reduction reads the instance field it names, one channel, from the low end of its `floor`
 * parameter's travel to 0 dB, or over the range omx-contract declares for the kernel's `readout`
 * of that name (OMX_<KERNEL>_READOUT_<readout>_MIN/_MAX, read by the compiler); a level is the stereo pair's peak in dBFS at the plugin's `input` or
 * `output`, its range as declared. */
export function meterRows(d, face) {
  const taken = new Set([...d.params.map((p) => p.symbol), ...FIXED_PORTS]);
  const K = (d.face ?? d.kernel);
  let offset = 0;
  const rows = (d.meters ?? []).map((m, k) => {
    const where = `plugins/${d.stem}: meters[${k}]`;
    if (!(m.kind in METER_KINDS)) throw new Error(`${where}: kind '${m.kind}' is none of ${Object.keys(METER_KINDS).join(', ')}`);
    if (!/^[A-Za-z][A-Za-z0-9_]*$/.test(m.id ?? '')) throw new Error(`${where}: id '${m.id}' is not a symbol`);
    if (taken.has(m.id)) throw new Error(`${where}: id '${m.id}' is taken`);
    taken.add(m.id);
    // a reading of the kernel's: a gain reduction, or a level the kernel reads itself (its detector's)
    const reads = m.kind === 'gain_reduction' || 'read' in m;
    const at = 'read' in m ? 'read' : 'field';
    const own = m.kind === 'gain_reduction' ? [at, 'readout' in m ? 'readout' : 'floor'] : reads ? ['read', 'readout'] : ['source', 'min', 'max'];
    for (const key of ['field', 'read', 'floor', 'readout', 'source', 'min', 'max']) {
      if (own.includes(key) && !(key in m)) throw new Error(`${where}: a ${m.kind} meter names its '${key}'`);
      if (!own.includes(key) && key in m) throw new Error(`${where}: a ${m.kind} meter ${reads && m.kind === 'level' ? 'the kernel reads ' : ''}takes no '${key}'`);
    }
    let row;
    if (reads) {
      // where the reading is: a member of the instance, or the face's reading function, banded or not
      let core, ref;
      if (at === 'field') {
        if (!/^[a-z_][a-z0-9_]*(\.[a-z_][a-z0-9_]*)*$/.test(m.field)) throw new Error(`${where}: field '${m.field}' is not a member path`);
        core = `c->inst.${m.field}`;
        ref = `inst.${m.field}`;
      } else {
        const r = m.read.match(/^([a-z][a-z0-9_]*)(?:\(([0-9]+)\))?$/);
        if (!r) throw new Error(`${where}: read '${m.read}' is not <reading> or <reading>(<band>)`);
        const reading = face?.readings?.[r[1]];
        if (face && !reading) throw new Error(`${where}: omx-dsp's <omxdsp/fx/omx_${K}_instance.h> has no float omx_${K}_instance_${r[1]}(const ${face.type} *${r[2] === undefined ? '' : ', uint32_t'})`);
        if (reading && reading.banded !== (r[2] !== undefined)) throw new Error(`${where}: omx_${K}_instance_${r[1]} ${reading.banded ? 'takes' : 'takes no'} band index`);
        const call = (self) => `omx_${K}_instance_${r[1]}(${self}${r[2] === undefined ? '' : `, ${r[2]}u`})`;
        core = call('&c->inst');
        ref = call('&inst');
      }
      if ('readout' in m) {
        if (!/^[A-Z][A-Z0-9_]*$/.test(m.readout)) throw new Error(`${where}: readout '${m.readout}' is not a readout name`);
        const r = `OMX_${d.kernel.toUpperCase()}_READOUT_${m.readout}`;
        row = { channels: 1, min: `(double)${r}_MIN`, max: `(double)${r}_MAX`, core, ref };
      } else {
        const p = d.params.find((q) => q.symbol === m.floor);
        if (!p) throw new Error(`${where}: floor '${m.floor}' is no parameter`);
        if (!(p.min < 0)) throw new Error(`${where}: floor '${m.floor}' starts at ${p.min}, not below 0 dB`);
        row = { channels: 1, min: p.min, max: 0, core, ref };
      }
    } else {
      if (m.source !== 'input' && m.source !== 'output') throw new Error(`${where}: source '${m.source}' is not input or output`);
      if (!(Number.isFinite(m.min) && Number.isFinite(m.max) && m.min < m.max)) throw new Error(`${where}: the range ${m.min}..${m.max} is not a range`);
      row = { channels: 2, min: m.min, max: m.max, core: '', ref: '' };
    }
    const out = {
      meterIndex: k, meterName: m.id, meterKind: METER_KINDS[m.kind], meterReadCore: row.core, meterReadRef: row.ref,
      meterChannels: row.channels, meterOffset: offset, meterRight: offset + 1, meterMin: cdouble(row.min), meterMax: cdouble(row.max),
      isGr: m.kind === 'gain_reduction', isRead: reads, isIn: m.source === 'input', isOut: m.source === 'output',
      // what a bypassed kernel's stale reading publishes: no reduction, or a level's display minimum
      meterBypass: m.kind === 'gain_reduction' ? '0.0' : `G_METER_INFO[${k}].min`, meterBypassRef: m.kind === 'gain_reduction' ? '0.0' : `METERS[${k}].min`,
    };
    offset += row.channels;
    return out;
  });
  if (offset > METER_VALUES_MAX) throw new Error(`plugins/${d.stem}: its meters carry ${offset} values, more than the ${METER_VALUES_MAX} one read holds`);
  return rows;
}

/** The template's view of the meters: the rows, how many, how many values, which levels are read. */
function meterView(d, face) {
  const meters = meterRows(d, face);
  return {
    meters, metered: meters.length > 0, meterCount: meters.length,
    meterValues: meters.reduce((n, m) => n + m.meterChannels, 0),
    meterGr: meters.some((m) => m.isGr), meterIn: meters.some((m) => m.isIn), meterOut: meters.some((m) => m.isOut),
    meterLevel: meters.some((m) => m.isIn || m.isOut),
    meterRead: meters.some((m) => m.isRead), meterKernelLevel: meters.some((m) => m.isRead && !m.isGr),
  };
}

/** The parameters the plan holds at their default, for the oracle's comment: `[{ symbol }]`. */
const heldOf = (d) => d.params.filter((p, i) => d.controls?.[i]?.rearms).map((p) => ({ symbol: p.symbol }));

/** How many times the identity test runs each block of the plan in a row: 1, or for a keyed face 64,
 * so a detector has the time its declared release travel spans (up to seconds) to fall through
 * every threshold of the plan: a wrong ratio or release is then seen, not hidden by a gate held open. */
export const oracleStretch = (face) => (face.keyed ? 64 : 1);

/** One step of each parameter, the sabotage arm's move: 1 % of a travel, the next value of a choice. */
const stepOf = (p) => (p.kind === 'toggle' || p.kind === 'integer' ? 1 : (p.max - p.min) / 100);

/**
 * The controls of `kernel` a declaration (or a chain element) `holds`: no parameter, no port, each
 * passed at its kernel's declared default by the default's named macro, OMX_<ref>_DEFAULT (a face
 * argument the plugin does not expose yet). `[{ p: { symbol, held }, c: { name } }]`, each symbol
 * `<prefix>.<control>`, the errors pushed on `errors`; `who` names the holder in them.
 */
function heldControls(d, kernel, names, prefix, who, errors) {
  if (names === undefined) return [];
  if (!Array.isArray(names) || names.some((n) => typeof n !== 'string')) {
    errors.push(`${who}'s "held" lists control names`);
    return [];
  }
  const where = locateContract(d.tree ?? ROOT);
  if (!where.dir) {
    errors.push(`${who} holds ${names.join(', ')} and ${where.why}`);
    return [];
  }
  return names.flatMap((name) => {
    const c = kernelControls(where.dir, kernel).find((x) => x.name === name);
    if (!c) errors.push(`${who} holds '${name}', no control of the ${kernel} kernel`);
    else if (c.field || c.count) errors.push(`${who} holds '${name}', a field of ${c.ref} with no default macro of its own: pass it as "fixed"`);
    else return [{ p: { symbol: `${prefix}.${name}`, held: `OMX_${c.ref}_DEFAULT` }, c: { name } }];
    return [];
  });
}

/** The binding of `d` to its kernel's instance face, or a thrown error naming what is missing. */
export function bindingOf(d) {
  const errors = [];
  const held = heldControls(d, d.face ?? d.kernel, d.held, d.stem, d.stem, errors);
  // the controls of the face this plugin does not expose: each passed at its `fixed` value, or held at its kernel's default
  const fixed = fixedArgs(d, { id: d.kernel, kernel: d.face ?? d.kernel, fixed: d.fixed }, d.controls ?? [], errors);
  const extra = [...fixed, ...held];
  const b = faceBinding(omxdspInclude(), d.face ?? d.kernel, [...d.params, ...extra.map((f) => f.p)], [...(d.controls ?? []), ...extra.map((f) => f.c)]);
  errors.push(...b.errors);
  if (errors.length) throw new Error(`plugins/${d.stem}: binding: instance, but ${errors.join('; ')}`);
  return b;
}

/** A held argument's value at the call site: its default's macro, cast to the argument's type. */
const heldValue = (type, p) => `(${type})${p.held}`;

/** The files of a `binding: instance` plugin beyond generated/: path relative to the plugin dir -> text. */
export function generateInstance(d) {
  const { face, binding, renames } = bindingOf(d);
  if (face.keyed && !d.sidechain) throw new Error(`plugins/${d.stem}: omx_${d.kernel}_instance_run takes a key; the declaration names its port in "sidechain"`);
  if (!face.keyed && !face.keyable && d.sidechain) throw new Error(`plugins/${d.stem}: declares a "sidechain", and omx_${d.kernel}_instance_run takes no key (nor has it a run_keyed)`);
  // keyed: the plugin's key port reaches the face, through run (the key first) or run_keyed (the
  // mono key on key_l, key_r NULL)
  const keyed = face.keyed || (face.keyable && Boolean(d.sidechain));
  const K = d.kernel.toUpperCase();
  const macro_ = (sym) => `OMX_${K}_PARAM_${macro(sym)}`;
  // a fixed control is its value, a parameter its port's
  const one = (type, p) => (p.fixed !== undefined ? (type === 'int' ? `${p.fixed}` : cfloat(p.fixed)) : type === 'int' ? `(int)lrintf(values[${macro_(p.symbol)}])` : `values[${macro_(p.symbol)}]`);
  // a per-band array: a compound literal of its parameters, in declaration order
  const value = (b) => (b.extent ? `(const ${b.elem}[${b.extent}]){${b.params.map((p) => one(b.elem, p)).join(', ')}}` : b.param.held ? heldValue(b.type, b.param) : one(b.type, b.param));
  const lat = (self) => (face.latency.fn ? `${face.latency.fn}(${self})` : face.latency.macro);
  const view = {
    ...templateView(d),
    faceType: face.type, srType: face.srType,
    binding: binding.map((b) => ({ arg: b.arg, value: value(b) })),
    arrays: binding.filter((b) => b.extent).map((b) => ({ arg: b.arg, count: b.params.length, extent: b.extent })),
    renames: renames.map((b) => ({ arg: b.arg, rename: b.rename })),
    heldArgs: binding.filter((b) => b.param.held).map((b) => ({ arg: b.arg, at: b.param.held })),
    fixed: Object.entries(d.fixed ?? {}).map(([name, v]) => ({ control: name, at: JSON.stringify(v) })),
    latency: lat('&c->inst'), refLatency: lat('&inst'),
    keyed, keyPort: keyed ? macro(d.sidechain.symbol) : '', keySymbol: keyed ? d.sidechain.symbol : '',
    runFn: keyed && !face.keyed ? 'run_keyed' : 'run', keyTail: keyed && !face.keyed ? ', NULL' : '',
    plan: oraclePlan(d.params, d.controls ?? []), held: heldOf(d), stretch: oracleStretch(face), steps: d.params.map((p) => cfloat(stepOf(p))).join(', '),
    chain: false,
    latencyChecks: latencyRows(d), measuresLatency: Boolean(d.latencyChecks),
    ...meterView(d, face),
  };
  return {
    Makefile: withBanner(render(template('Makefile.tmpl'), view), d, '#'),
    [`omx_${d.kernel}_clap.c`]: withBanner(render(template('clap.c.tmpl'), view), d, '//'),
    [`omx_${d.kernel}_lv2.c`]: withBanner(render(template('lv2.c.tmpl'), view), d, '//'),
    [`omx_${d.kernel}_core.h`]: render(template('core.h.tmpl'), view),
    [`test/${d.kernel}-oracle.c`]: render(template('oracle.c.tmpl'), view),
  };
}

/** The value a chain element's `fixed` control is passed at: a set's id as its index (a numeric
 * set's id as itself), a travel's number as given, `"default"` the control's declared default (a
 * travel that declares none is refused), `{ "defaultRef": "<SHEET>.<field>" }` the default a sheet
 * of omx-contract holds for it (EQ_PASS_FILTER_DEFAULTS.hpfFreqHz). */
function fixedValue(d, el, control, v) {
  const where = locateContract(d.tree ?? ROOT);
  if (!where.dir) throw new Error(`plugins/${d.stem}: element '${el.id}' fixes '${control.name}' and ${where.why}`);
  const as = { symbol: `${el.id}.${control.name}`, ref: control.ref, ...(control.field ? { field: control.field } : {}) };
  const e = findName(where.dir, el.kernel, control.ref).entry;
  if (v && typeof v === 'object') return resolveParam(where.dir, el.kernel, { ...as, defaultRef: v.defaultRef }).def;
  if (v === 'default') {
    const t = e?.kind === 'travels' ? (control.field ? e.value?.[control.field] : e.value) : undefined;
    if (t && t.default === undefined) throw new Error(`plugins/${d.stem}: element '${el.id}' fixes '${control.name}' at "default", and ${control.ref} declares no default; name the sheet that holds it, { "defaultRef": "<SHEET>.<field>" }`);
    return resolveParam(where.dir, el.kernel, as).def;
  }
  if (e?.kind === 'set') {
    if (e.value.every((x) => typeof x === 'number')) {
      if (!e.value.includes(v)) throw new Error(`plugins/${d.stem}: element '${el.id}' fixes '${control.name}' at ${JSON.stringify(v)}, not one of ${control.ref}'s ids ${JSON.stringify(e.value)}`);
      return v;
    }
    const k = e.value.indexOf(v);
    if (k < 0) throw new Error(`plugins/${d.stem}: element '${el.id}' fixes '${control.name}' at ${JSON.stringify(v)}, not one of ${control.ref}'s ids ${JSON.stringify(e.value)}`);
    return k;
  }
  if (typeof v !== 'number') throw new Error(`plugins/${d.stem}: element '${el.id}' fixes the travel '${control.name}' at ${JSON.stringify(v)}, not a number, "default" or { "defaultRef" }`);
  return v;
}

/** The values of a per-band control an element fixes, for the bands of its face it does not expose:
 * from band `exposed` (the element's own parameters of that control come first) to the face's
 * compiled count. `"default"` gives each band its own default by omx-contract's one default rule at
 * that count (its type, centre, gain and Q; a control the rule does not name, its declared default);
 * any other value is the one value for every band. */
function fixedBands(d, el, control, v, exposed) {
  const where = locateContract(d.tree ?? ROOT);
  if (!faceBandsOf(el)) throw new Error(`plugins/${d.stem}: element '${el.id}' fixes '${control.name}', a control taken once per band; it declares its "bands" (or "faceBands")`);
  const n = findName(where.dir, el.kernel, control.count).entry?.value?.[faceBandsOf(el)]?.max;
  if (!Number.isInteger(n)) throw new Error(`plugins/${d.stem}: element '${el.id}': ${control.count} has no '${faceBandsOf(el)}' with a max`);
  const rest = [...Array(n).keys()].slice(exposed);
  if (v !== 'default') return rest.map(() => fixedValue(d, el, control, v));
  const rule = eqDefaultBands(where.dir, n);
  const name = control.name.toLowerCase();
  return rest.map((i) => {
    const band = rule[i];
    const key = Object.keys(band).find((k) => k.toLowerCase() === name || k.toLowerCase().startsWith(name) || name.endsWith(k.toLowerCase()));
    return key === undefined ? fixedValue(d, el, control, 'default') : fixedValue(d, el, control, band[key]);
  });
}

/**
 * The controls a face is handed at a fixed value, never a port: a chain element's `fixed`, or an
 * instance plugin's own (`el` is then the plugin, its id its kernel): `[{ p, c }]`, each bound like a
 * parameter (`p.fixed` the value) to its control `c`. `controls` are the exposed parameters' controls;
 * a refusal is pushed to `errors`.
 */
function fixedArgs(d, el, controls, errors) {
  const where = locateContract(d.tree ?? ROOT);
  return Object.entries(el.fixed ?? {}).flatMap(([name, v]) => {
    const c = where.dir ? kernelControls(where.dir, el.kernel).find((x) => x.name === name) : undefined;
    if (!c) {
      errors.push(`element '${el.id}' fixes '${name}', no control of the ${el.kernel} kernel`);
      return [];
    }
    try {
      // a per-band control: one fixed value per band, bound as the face's array argument
      if (c.count) {
        const exposed = controls.filter((x) => x?.name === name).length;
        return fixedBands(d, el, c, v, exposed).map((x, k) => ({ p: { symbol: `${el.id}.${name}[${exposed + k}]`, fixed: x }, c: { name } }));
      }
      return [{ p: { symbol: `${el.id}.${name}`, fixed: fixedValue(d, el, c, v) }, c: { name } }];
    } catch (e) {
      errors.push(e.message);
      return [];
    }
  });
}

/**
 * Each element of a chain bound to its own kernel's instance face, exactly as a single plugin's
 * parameters are (bindFace, by name): `[{ el, face, binding, renames, fixed }]`, or a thrown error
 * naming every element that does not bind. An element's `fixed` controls (`{ control: id | number }`)
 * are the ones it does not expose (a keyed face's key source, in an element that is not the chain's
 * `key`): passed at that value, they bind like parameters and are never a port.
 */
export function chainBindings(d) {
  const inc = omxdspInclude();
  const where = locateContract(d.tree ?? ROOT);
  const errors = [];
  for (const el of d.chain) {
    const other = d.chain.find((o) => o !== el && o.kernel === el.kernel && faceBandsOf(o) !== faceBandsOf(el));
    if (other) errors.push(`elements '${el.id}' and '${other.id}' run one ${el.kernel} face, compiled once: they declare the same face band count ("faceBands", else "bands")`);
  }
  const out = d.chain.map((el) => {
    const idx = d.params.map((p, i) => i).filter((i) => d.params[i].element === el.id && d.params[i].own === undefined);
    const params = idx.map((i) => d.params[i]), controls = idx.map((i) => d.controls[i]);
    if (el.bands && where.dir) {
      const want = findName(where.dir, el.kernel, kernelControls(where.dir, el.kernel).find((x) => x.count)?.count ?? '').entry?.value?.[el.bands]?.max;
      for (const c of kernelControls(where.dir, el.kernel).filter((x) => x.count)) {
        const have = controls.filter((x) => x?.name === c.name).length;
        if (have && have !== want) errors.push(`element '${el.id}' exposes ${have} '${c.name}' parameters, and its "bands" '${el.bands}' counts ${want}`);
      }
    }
    const fixed = fixedArgs(d, el, controls, errors);
    const held = heldControls(d, el.kernel, el.held, el.id, `element '${el.id}'`, errors);
    const extra = [...fixed, ...held];
    const b = faceBinding(inc, el.kernel, [...params, ...extra.map((f) => f.p)], [...controls, ...extra.map((f) => f.c)]);
    for (const e of b.errors) errors.push(`element '${el.id}' (${el.kernel}): ${e}`);
    if (b.face && !b.face.keyed && d.key === el.id) errors.push(`"key" names '${el.id}', and omx_${el.kernel}_instance_run takes no key`);
    return { el, ...b };
  });
  if (errors.length) throw new Error(`plugins/${d.stem}: binding: chain, but ${errors.join('; ')}`);
  return out;
}

/** The files of a `binding: chain` plugin beyond generated/: path relative to the plugin dir -> text.
 * The faces and the Makefile are a single plugin's; the binding is the chain core (copy in to out
 * once, each element's face in place in the chosen order, no arithmetic of its own) and the oracle's
 * reference is the same chain of faces called directly. */
export function generateChain(d) {
  const bound = chainBindings(d);
  const K = d.kernel.toUpperCase();
  const macro_ = (sym) => `OMX_${K}_PARAM_${macro(sym)}`;
  const one = (type, p) => (p.held ? heldValue(type, p) : p.fixed !== undefined ? (type === 'int' ? `${p.fixed}` : cfloat(p.fixed)) : type === 'int' ? `(int)lrintf(values[${macro_(p.symbol)}])` : `values[${macro_(p.symbol)}]`);
  const value = (b) => (b.extent ? `(const ${b.elem}[${b.extent}]){${b.params.map((p) => one(b.elem, p)).join(', ')}}` : one(b.type, b.param));
  const lat = (b, self) => (b.face.latency.fn ? `${b.face.latency.fn}(${self})` : b.face.latency.macro);
  const keyed = d.key !== undefined;
  const elements = bound.map((b, index) => ({
    id: b.el.id, index, name: elementName(b.el), face: b.el.kernel, faceType: b.face.type, srType: b.face.srType,
    on: macro_(`${b.el.id}On`),
    binding: b.binding.map((x) => ({ arg: x.arg, value: value(x) })),
    arrays: b.binding.filter((x) => x.extent).map((x) => ({ arg: x.arg, count: x.params.length, extent: x.extent })),
    renames: b.renames.map((x) => ({ arg: x.arg, rename: x.rename })),
    fixed: (b.el.fixed ? Object.entries(b.el.fixed) : []).map(([name, v]) => ({ control: name, at: JSON.stringify(v) })),
    heldArgs: b.binding.filter((x) => x.param.held).map((x) => ({ arg: x.arg, at: x.param.held })),
    keyedFace: b.face.keyed, keyArg: b.face.keyed ? (d.key === b.el.id ? 'key' : 'NULL') : '',
    keyRef: b.face.keyed && d.key === b.el.id ? 'key ? key + at : NULL' : 'NULL',
    latency: lat(b, `&c->${b.el.id}`), refLatency: lat(b, `&${b.el.id}`),
  }));
  const rows = d.order === 'permutable' ? permutations(d.chain.length) : [[...d.chain.keys()]];
  const view = {
    ...templateView(d),
    chain: true, elements, faces: [...new Set(elements.map((e) => e.face))].map((face) => ({ face })),
    length: elements.length, orders: rows.length, orderRows: rows.map((r) => ({ row: r.join(', ') })),
    permutable: d.order === 'permutable', orderParam: d.order === 'permutable' ? macro_('order') : '',
    sequence: elements.map((e) => e.name).join(' > '),
    latency: elements.map((e) => `(float)(${e.latency})`).join(' + '), refLatency: elements.map((e) => `(float)(${e.refLatency})`).join(' + '),
    keyed, keyPort: keyed ? macro(d.sidechain.symbol) : '', keySymbol: keyed ? d.sidechain.symbol : '',
    plan: oraclePlan(d.params, d.controls ?? [], { engaged: d.params.map((p, i) => i).filter((i) => d.params[i].own === 'switch' && d.params[i].element) }),
    held: heldOf(d), stretch: keyed ? 64 : 1, steps: d.params.map((p) => cfloat(stepOf(p))).join(', '),
    latencyChecks: latencyRows(d), measuresLatency: Boolean(d.latencyChecks),
    // the single-face fields the shared templates read, empty for a chain
    faceType: '', srType: '', binding: [], arrays: [], renames: [], heldArgs: [],
  };
  return {
    Makefile: withBanner(render(template('Makefile.tmpl'), view), d, '#'),
    [`omx_${d.kernel}_clap.c`]: withBanner(render(template('clap.c.tmpl'), view), d, '//'),
    [`omx_${d.kernel}_lv2.c`]: withBanner(render(template('lv2.c.tmpl'), view), d, '//'),
    [`omx_${d.kernel}_core.h`]: render(template('chain-core.h.tmpl'), view),
    [`test/${d.kernel}-oracle.c`]: render(template('oracle.c.tmpl'), view),
  };
}

/** Every generated file of one plugin: path relative to the plugin dir -> text. */
export function generate(d) {
  const lv2 = `generated/${d.stem}.lv2`;
  return {
    [`generated/omx_${d.kernel}_params.h`]: emitParamsHeader(d),
    [`${lv2}/manifest.ttl`]: emitManifest(d, { modgui: Boolean(d.panel) }),
    [`${lv2}/${d.stem}.ttl`]: emitPluginTtl(d),
    ...(d.binding === 'instance' ? generateInstance(d) : {}),
    ...(d.binding === 'chain' ? generateChain(d) : {}),
    ...(d.descriptor ? { [descriptorFile(d.stem)]: descriptorText(d) } : {}),
  };
}

/** Every plugin's folder: each plugins/<stem>/ holding <stem>.decl.json, a base of variants standing
 * for its variants' folders (tools/variants.mjs). A test plugin's folder (`"role": "test"`,
 * tools/test-plugins.mjs) is none of them: testPluginDirs() lists those. */
export const pluginDirs = (root = ROOT) =>
  readdirSync(join(root, 'plugins'))
    .map((n) => join(root, 'plugins', n))
    .filter((p) => existsSync(join(p, `${basename(p)}.decl.json`)) && !isTestDir(p))
    .flatMap((p) => {
      const raw = JSON.parse(readFileSync(join(p, `${basename(p)}.decl.json`), 'utf8'));
      return raw.variants ? variantStems(raw).map((s) => join(root, 'plugins', s)) : [p];
    })
    .sort();

// ---- the shared files, generated from every plugin folder ---------------------------------------

/** A plugin ships (is built, packaged, listed) when its folder has a Makefile or generates one. */
export const ships = (d) => generatedBinding(d) || existsSync(join(d.dir, 'Makefile'));

/** Plugin order everywhere a list is generated: by stem, numbers by value (eq8 before eq16). */
const byStem = (a, b) => a.stem.localeCompare(b.stem, 'en', { numeric: true });

/** Each region's lines, from the shipped plugins' declarations. */
export const SHARED = {
  'README.md': {
    catalogue: (ds) => [
      '',
      '| Plugin | CLAP id | LV2 URI | What it does |',
      '|---|---|---|---|',
      ...ds.map((d) => `| **${d.name}** | \`${d.clap.id}\` | \`${d.lv2.uri}\` | ${d.summary ?? d.description} |`),
      '',
    ],
    sections: (ds) => ds.flatMap((d) => [
      '',
      `### ${d.name}`,
      '',
      ...(d.manual?.map((l) => fillManual(d, l)) ?? ['| Parameter | Range | Default |', '|---|---|---|', ...d.params.map(paramRow).map((r) => `| ${r.name} | ${r.range} | ${r.default} |`), '', "Plus the host's bypass."]),
    ]).concat(['']),
  },
  'packaging/omx-plugins.spec': {
    'clap-files': (ds) => ds.map((d) => `/usr/lib/clap/${d.stem}.clap`),
    'lv2-files': (ds) => ds.map((d) => `%{_libdir}/lv2/${d.stem}.lv2/`),
    // the test plugins (`"role": "test"`): omx-plugins-test, outside every CLAP search path (`make install-test`)
    'clap-test-files': (ds, ts = []) => ts.map((d) => `%{_libexecdir}/omx-plugins/test/${d.stem}.clap`),
  },
  '.github/workflows/ci.yml': {
    // the test plugins' ids too: static-table-host expects them after `--test`, and a hand-written
    // list in CI hides a new test plugin
    plugins: (ds, ts = []) => [
      `shorts="${ds.map((d) => d.stem.replace(/^omx-/, '')).join(' ')}"`,
      `kernels="${ds.map((d) => d.kernel).join(' ')}"`,
      `tests="${ts.map((d) => d.clap.id).join(' ')}"`,
      `teststems="${ts.map((d) => d.stem).join(' ')}"`,
    ],
  },
  // the param-contract extension's units, from their declaration (contract/param-units.json), one source for the
  // header and the file a runtime reader reads in the contract directory
  'include/omx_param_contract.h': {
    'param-units': () => unitEnumLines(),
  },
  // libomxplugins: each face's entry, defined by omx_<kernel>_clap.c, listed by its CLAP id
  'include/omx_clap_static.h': {
    'clap-entries': (ds) => ds.map((d) => `extern const clap_plugin_entry_t omx_clap_entry_${d.kernel};`),
    'clap-test-entries': (ds, ts = []) => ts.map((d) => `extern const clap_plugin_entry_t omx_clap_entry_${d.kernel};`),
  },
  'lib/omx_clap_static.c': {
    'clap-table': (ds) => ds.map((d) => `{"${d.clap.id}", &omx_clap_entry_${d.kernel}},`),
    // the test plugins (`"role": "test"`): linked and found by id, never listed with the shipped ones
    'clap-test-table': (ds, ts = []) => ts.map((d) => `{"${d.clap.id}", &omx_clap_entry_${d.kernel}},`),
  },
};

/** `text` with each `BEGIN GENERATED <name>` .. `END GENERATED <name>` region refilled; the inner
 * lines take the BEGIN line's indentation. A region missing, unclosed or unknown throws. */
export function fillRegions(file, text, producers, ds, ts = []) {
  const lines = text.split('\n');
  const out = [];
  const seen = new Set();
  for (let i = 0; i < lines.length; i++) {
    const m = lines[i].match(/^(\s*).*\bBEGIN GENERATED ([a-z][a-z0-9-]*)\b/);
    out.push(lines[i]);
    if (!m) continue;
    const [, indent, name] = m;
    if (!producers[name]) throw new Error(`${file}: line ${i + 1} opens a region '${name}' tools/gen.mjs does not fill`);
    const end = lines.findIndex((l, j) => j > i && new RegExp(`\\bEND GENERATED ${name}\\b`).test(l));
    if (end < 0) throw new Error(`${file}: region '${name}' (line ${i + 1}) is never closed`);
    out.push(...producers[name](ds, ts).map((l) => (l ? `${indent}${l}` : l)));
    out.push(lines[end]);
    seen.add(name);
    i = end;
  }
  const absent = Object.keys(producers).filter((n) => !seen.has(n));
  if (absent.length) throw new Error(`${file}: no region ${absent.map((n) => `'${n}'`).join(', ')}`);
  return out.join('\n');
}

/** Every shared file: path relative to the tree -> text. */
export function generateShared(root = ROOT, decls = pluginDirs(root).map((p) => loadDecl(p)), tests = testPluginDirs(root).map((p) => loadTestDecl(root, p))) {
  const ds = decls.filter(ships).sort(byStem);
  const ts = [...tests].sort(byStem);
  return Object.fromEntries(Object.entries(SHARED).map(([file, producers]) => [file, fillRegions(file, readFileSync(join(root, file), 'utf8'), producers, ds, ts)]));
}

/** A declaration's pinned port hints (its `portHints`, tools/port-hints.mjs) in the view's shape,
 * `{props, unit, points}` by port symbol in LV2 port order, then any symbol no port has (which the
 * hint check names as gone); undefined when it pins none. */
export function hintsOf(d) {
  if (!d.portHints) return undefined;
  const out = {};
  for (const sym of new Set([...lv2Ports(d).map((p) => p.symbol), ...Object.keys(d.portHints)])) {
    const h = d.portHints[sym];
    if (h) out[sym] = { props: h.props ?? [], unit: h.unit ?? null, points: h.points ?? [] };
  }
  return out;
}

/** The shared view the hint check reads: every plugin's pinned hints, from its declaration, by stem.
 * A build product (tools/test/port-hints.json, git-ignored), never hand-edited. */
export const HINTS_VIEW = 'tools/test/port-hints.json';
export function hintsView(root = ROOT, decls = pluginDirs(root).map((p) => loadDecl(p))) {
  const view = {};
  for (const d of decls) {
    const h = hintsOf(d);
    if (h) view[d.stem] = h;
  }
  return view;
}
export const hintsViewText = (root = ROOT, decls) => `${JSON.stringify(hintsView(root, decls), null, 2)}\n`;

function main(argv) {
  const check = argv.includes('--check');
  const dirs = argv.filter((a) => !a.startsWith('--'));
  let stale = 0;
  const put = (path, shown, text, how) => {
    const now = existsSync(path) ? readFileSync(path, 'utf8') : null;
    if (now === text) return;
    if (check) {
      console.error(`gen: STALE ${shown} (run \`${how}\`)`);
      stale++;
    } else {
      mkdirSync(dirname(path), { recursive: true });
      writeFileSync(path, text);
      console.log(`gen: wrote ${shown}`);
    }
  };
  const decls = (dirs.length ? dirs.filter((dir) => !isTestDir(dir)) : pluginDirs()).map((dir) => loadDecl(dir));
  const tests = (dirs.length ? dirs.filter((dir) => isTestDir(dir)) : testPluginDirs(ROOT)).map((dir) => loadTestDecl(ROOT, dir));
  for (const d of decls) {
    for (const [rel, text] of Object.entries(generate(d))) put(join(d.dir, rel), join('plugins', d.stem, rel), text, `make -C plugins/${d.stem} gen`);
  }
  for (const d of tests) {
    for (const [rel, text] of Object.entries(generateTest(d))) put(join(d.dir, rel), join('plugins', d.stem, rel), text, 'node tools/gen.mjs');
  }
  if (!dirs.length) for (const [rel, text] of Object.entries(generateShared(ROOT, decls, tests))) put(join(ROOT, rel), rel, text, 'node tools/gen.mjs');
  if (!dirs.length) {
    // the pin is the declaration's `portHints`: a pin file beside it would be a second home
    for (const n of readdirSync(join(ROOT, 'plugins')).sort()) {
      if (!existsSync(join(ROOT, 'plugins', n, 'port-hints.json'))) continue;
      console.error(`gen: EXTRA plugins/${n}/port-hints.json: the pin of the port hints is the declaration's "portHints"; fold it in and delete the file`);
      stale++;
    }
    const viewPath = join(ROOT, HINTS_VIEW);
    // absent is fine (it is built by `make hints`); present must be fresh, so a hand edit goes stale
    const text = hintsViewText(ROOT, decls), now = existsSync(viewPath) ? readFileSync(viewPath, 'utf8') : null;
    if (check && now !== null && now !== text) {
      console.error(`gen: STALE ${HINTS_VIEW} (run \`node tools/gen.mjs\`)`);
      stale++;
    } else if (!check && now !== text) writeFileSync(viewPath, text); // a build product, not a file the wizard commits
  }
  if (stale) process.exit(1);
  if (check) console.log(`gen: every generated file is fresh${dirs.length ? '' : ', the shared files too'}`);
}

if (process.argv[1] && resolve(process.argv[1]) === fileURLToPath(import.meta.url)) main(process.argv.slice(2));
