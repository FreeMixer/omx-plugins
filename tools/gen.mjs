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
 * and, for a plugin whose declaration says `"binding": "instance"`, the rest of its folder (the
 * declaration is then the folder's one hand-written file), from recipes/templates/plugin/:
 *
 *   plugins/<stem>/omx_<kernel>_core.h               the binding: each parameter to the resolve()
 *                                                    argument of omx-dsp's instance face it names
 *   plugins/<stem>/test/<kernel>-oracle.c            the kernel-identity test against that face
 *   plugins/<stem>/omx_<kernel>_clap.c, _lv2.c       the CLAP and LV2 faces
 *   plugins/<stem>/Makefile                          the build and `make test`
 *
 * and, from every plugin folder at once, the regions of the shared files between a `BEGIN
 * GENERATED <name>` line and its `END GENERATED <name>` line (SHARED below): the README's catalogue
 * and sections, the RPM %files lists and CI's installed-file lists. Adding a plugin touches only its
 * folder; two plugins' changes merge in either order.
 *
 * and the hint view tools/test/port-hints.json (git-ignored), assembled from every plugin's own
 * plugins/<stem>/port-hints.json for tools/port-hints.mjs.
 *
 * The MOD GUI of the same bundle is tools/modgui-gen.mjs's, which reads the port list from here.
 *
 * Usage: `node tools/gen.mjs [--check] [plugins/<stem> ...]` — `--check` writes nothing and fails on a
 * stale, missing or changed file. Named plugins are generated alone; with none, every plugin and
 * the shared files. No clock and no absolute path reach the output.
 */
import { createHash } from 'node:crypto';
import { existsSync, mkdirSync, readFileSync, readdirSync, writeFileSync } from 'node:fs';
import { basename, dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { faceBinding } from './instance-face.mjs';
import { PIN_FILE, controlOf, findName, locateContract, paramKernel, pinOf, resolveParam } from './omx-contract.mjs';
import { bandsMacro, baseOf, expandVariant, variantStems } from './variants.mjs';
import { omxdspInclude, render } from './template.mjs';

export const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..');

const BANNER = '// SPDX-License-Identifier: GPL-3.0-or-later\n// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>\n';
const TTL_BANNER = '# SPDX-License-Identifier: GPL-3.0-or-later\n# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>\n';

/** Read and validate a plugin's declaration. Everything downstream trusts what this returns. */
export function loadDecl(dir) {
  const stem = basename(resolve(dir));
  let file = join(dir, `${stem}.decl.json`);
  let d;
  if (existsSync(file) || !baseOf(dir)) d = JSON.parse(readFileSync(file, 'utf8'));
  else {
    // a variant's folder holds generated files only: its declaration is its base's, expanded
    const base = baseOf(dir);
    file = base.file;
    const where = locateContract(treeOf(file));
    if (!where.dir) throw new Error(`${file}: its variant ${stem} is expanded from omx-contract and ${where.why}`);
    d = expandVariant(where.dir, base.decl, base.decl.variants.of.find((v) => v.stem === stem));
  }
  if (d.variants) throw new Error(`${file}: the base of ${variantStems(d).join(', ')} is no plugin of its own; load one of its variants`);
  if (d.stem !== stem) throw new Error(`${file}: stem '${d.stem}' is not the directory's '${stem}'`);
  if (!/^[a-z][a-z0-9_]*$/.test(d.kernel)) throw new Error(`${file}: kernel '${d.kernel}' is not a C identifier`);
  const { params, controls } = resolveParams(file, d);
  d.params = params;
  if (d.binding === 'instance') d.controls = controls();
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
  d.panel ??= defaultPanel(d);
  return { ...d, dir: resolve(dir), tree: treeOf(file) };
}

/** The panel of a plugin generated from its instance face that declares none: one section, every
 * parameter in declaration order, so every generated plugin has its MOD GUI. A hand-written plugin
 * with no panel still owes none. */
export function defaultPanel(d) {
  if (d.binding !== 'instance') return undefined;
  const label = d.name.replace(/^omx /, '');
  return { family: d.kernel, roles: {}, sections: [{ key: d.kernel, label: label[0].toUpperCase() + label.slice(1), controls: d.params.map((p) => p.symbol) }] };
}

/** The tree a declaration sits in; a declaration copied out of its tree (a test's scratch copy)
 * reads this tree's pin. */
const treeOf = (file) => {
  const tree = resolve(dirname(file), '..', '..');
  return existsSync(join(tree, PIN_FILE)) ? tree : ROOT;
};

/** The travel fields a parameter BY REFERENCE reads from omx-contract and must never retype. */
export const TRAVEL_FIELDS = ['min', 'max', 'def', 'unit', 'kind'];

/** Each parameter with its travel: read from omx-contract at the pin through its `ref`
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
  const controls = () => d.params.map((p) => (p.ref === undefined ? undefined : controlOf(contract(), paramKernel(d, p, contract()), p)));
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
    const out = { ...p, min: t.min, max: t.max, def: t.def, unit: t.unit };
    // a control whose travel another choice widens (eq's q, notchQ while the band is a notch): the
    // port spans every travel it may take, and the face clamps by the choice
    const c = d.binding === 'instance' ? controlOf(dir, kernel, p) : undefined;
    for (const w of c?.when ?? []) {
      const u = resolveParam(dir, kernel, { symbol: p.symbol, ref: w.global });
      out.min = Math.min(out.min, u.min);
      out.max = Math.max(out.max, u.max);
    }
    if (t.kind) out.kind = t.kind;
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

/** The LV2 unit of a declared unit, where the units extension names one. */
const LV2_UNITS = { Hz: 'hz', dB: 'db', ms: 'ms', s: 's', '%': 'pc', oct: 'oct' };

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
#include "omx_plugin_param.h"
${d.variantOf ? `
/* The band count of this variant of plugins/${d.variantOf.stem}: the instance face's compile-time count. */
#define OMX_${(d.face ?? d.kernel).toUpperCase()}_INSTANCE_BANDS ${bandsMacro(d)}
` : ''}
enum {
${d.params.map((p, i) => `  ${P}_${macro(p.symbol)} = ${i},`).join('\n')}
  ${P}_COUNT = ${d.params.length}
};

static const omx_plugin_param OMX_${K}_PARAMS[${P}_COUNT] = {
${d.params.map((p) => `  { "${p.symbol}", "${p.name}", "${p.unit}", ${cf(p.min)}, ${cf(p.max)}, ${cf(p.def)}, ${flags(p)} },`).join('\n')}
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

/* LV2 port indices (tools/gen.mjs lv2Ports): audio, then parameter i at FIRST_PARAM + i, then these. */
${ports.filter((p) => !p.param).map((p) => `#define OMX_${K}_LV2_PORT_${macro(p.symbol)} ${p.index}u`).join('\n')}
#define OMX_${K}_LV2_PORT_FIRST_PARAM ${firstParam}u
#define OMX_${K}_LV2_PORT_COUNT ${ports.length}u

#endif /* OMX_${K}_PARAMS_H */
`;
}

const ttlNum = (n) => (Number.isInteger(n) ? `${n}` : `${n}`);
const ttlStr = (s) => `"${s.replace(/\\/g, '\\\\').replace(/"/g, '\\"')}"`;

export function emitManifest(d, { modgui = false } = {}) {
  return `${TTL_BANNER}# GENERATED by tools/gen.mjs from plugins/${d.stem}/${d.stem}.decl.json — DO NOT EDIT BY HAND.
@prefix lv2:  <http://lv2plug.in/ns/lv2core#> .
@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .

<${d.lv2.uri}>
    a lv2:Plugin ;
    lv2:binary <${d.stem}.so> ;
    rdfs:seeAlso <${d.stem}.ttl>${modgui ? ' , <modgui.ttl>' : ''} .
`;
}

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
      if (LV2_UNITS[q.unit]) lines.push(`        units:unit units:${LV2_UNITS[q.unit]} ;`);
      if (choices) {
        const points = choices.map((c) => `            [ rdfs:label ${ttlStr(c.label)} ; rdf:value ${c.value} ]`);
        lines.push(`        lv2:scalePoint\n${points.join(' ,\n')} ;`);
      }
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
${d.params.some((q) => scaleOf(q) === 'log') ? '@prefix pprops: <http://lv2plug.in/ns/ext/port-props#> .\n' : ''}${d.params.some((q) => choicesOf(q)) ? '@prefix rdf:   <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n' : ''}@prefix rdfs:  <http://www.w3.org/2000/01/rdf-schema#> .
@prefix units: <http://lv2plug.in/ns/extensions/units#> .

<${d.url}>
    a doap:Project ;
    doap:name "omx-plugins" ;
    doap:maintainer [ a foaf:Person ; foaf:name "Pau Aliagas" ] .

<${d.lv2.uri}>
    a lv2:Plugin , ${d.lv2.class} ;
    doap:name ${ttlStr(d.name)} ;
    doap:license <https://www.gnu.org/licenses/gpl-3.0.html> ;
    lv2:project <${d.url}> ;
    lv2:minorVersion ${d.version.split('.')[1]} ;
    lv2:microVersion ${d.version.split('.')[2]} ;
    rdfs:comment ${ttlStr(d.description)} ;
    lv2:optionalFeature lv2:hardRTCapable ;
    lv2:port
${lv2Ports(d).map(port).join(' ,\n')} .
`;
}

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

/** The view every plugin template renders from. */
export function templateView(d) {
  return {
    stem: d.stem, kernel: d.kernel, face: d.face ?? d.kernel, K: d.kernel.toUpperCase(), Kernel: d.kernel.replace(/(^|_)([a-z])/g, (_m, _u, c) => c.toUpperCase()),
    name: d.name, uri: d.lv2.uri, clapId: d.clap.id, description: d.description,
    omxdspMin: pinOf('omx-dsp', d.tree ?? ROOT), panel: Boolean(d.panel), params: d.params.map(paramRow),
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

export function oraclePlan(params, controls = []) {
  const held = new Set(params.map((_, i) => i).filter((i) => controls[i]?.rearms));
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
  const defaults = params.map((p) => p.def);
  const release = [row(0, BURST_MS, SIGNAL.burst, defaults, 0), row(0, tail, SIGNAL.tail, defaults, 0)];
  const choices = [];
  params.forEach((p, c) => {
    const vals = valuesOf(p);
    if (!vals || held.has(c)) return;
    for (const value of vals) {
      const v = defaults.map((d, i) => (i === c ? value : d));
      const moved = stepped(params, v, new Set([...held, c]));
      choices.push(row(0, BURST_MS, SIGNAL.burst, v, 0), row(0, tail, SIGNAL.tail, v, 0), row(0, tail, SIGNAL.tail, moved, 0));
    }
  });
  return [...moves, ...release, ...choices];
}

/** The parameters the plan holds at their default, for the oracle's comment: `[{ symbol }]`. */
const heldOf = (d) => d.params.filter((p, i) => d.controls?.[i]?.rearms).map((p) => ({ symbol: p.symbol }));

/** One step of each parameter, the sabotage arm's move: 1 % of a travel, the next value of a choice. */
const stepOf = (p) => (p.kind === 'toggle' || p.kind === 'integer' ? 1 : (p.max - p.min) / 100);

/** The binding of `d` to its kernel's instance face, or a thrown error naming what is missing. */
export function bindingOf(d) {
  const b = faceBinding(omxdspInclude(), d.face ?? d.kernel, d.params, d.controls ?? []);
  if (b.errors.length) throw new Error(`plugins/${d.stem}: binding: instance, but ${b.errors.join('; ')}`);
  return b;
}

/** The files of a `binding: instance` plugin beyond generated/: path relative to the plugin dir -> text. */
export function generateInstance(d) {
  const { face, binding, renames } = bindingOf(d);
  const K = d.kernel.toUpperCase();
  const macro_ = (sym) => `OMX_${K}_PARAM_${macro(sym)}`;
  const one = (type, sym) => (type === 'int' ? `(int)lrintf(values[${macro_(sym)}])` : `values[${macro_(sym)}]`);
  // a per-band array: a compound literal of its parameters, in declaration order
  const value = (b) => (b.extent ? `(const ${b.elem}[${b.extent}]){${b.params.map((p) => one(b.elem, p.symbol)).join(', ')}}` : one(b.type, b.param.symbol));
  const lat = (self) => (face.latency.fn ? `${face.latency.fn}(${self})` : face.latency.macro);
  const view = {
    ...templateView(d),
    faceType: face.type, srType: face.srType,
    binding: binding.map((b) => ({ arg: b.arg, value: value(b) })),
    arrays: binding.filter((b) => b.extent).map((b) => ({ arg: b.arg, count: b.params.length, extent: b.extent })),
    renames: renames.map((b) => ({ arg: b.arg, rename: b.rename })),
    latency: lat('&c->inst'), refLatency: lat('&inst'),
    plan: oraclePlan(d.params, d.controls ?? []), held: heldOf(d), steps: d.params.map((p) => cfloat(stepOf(p))).join(', '),
  };
  return {
    Makefile: withBanner(render(template('Makefile.tmpl'), view), d, '#'),
    [`omx_${d.kernel}_clap.c`]: withBanner(render(template('clap.c.tmpl'), view), d, '//'),
    [`omx_${d.kernel}_lv2.c`]: withBanner(render(template('lv2.c.tmpl'), view), d, '//'),
    [`omx_${d.kernel}_core.h`]: render(template('core.h.tmpl'), view),
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
  };
}

/** Every plugin's folder: each plugins/<stem>/ holding <stem>.decl.json, a base of variants standing
 * for its variants' folders (tools/variants.mjs). */
export const pluginDirs = (root = ROOT) =>
  readdirSync(join(root, 'plugins'))
    .map((n) => join(root, 'plugins', n))
    .filter((p) => existsSync(join(p, `${basename(p)}.decl.json`)))
    .flatMap((p) => {
      const raw = JSON.parse(readFileSync(join(p, `${basename(p)}.decl.json`), 'utf8'));
      return raw.variants ? variantStems(raw).map((s) => join(root, 'plugins', s)) : [p];
    })
    .sort();

// ---- the shared files, generated from every plugin folder ---------------------------------------

/** A plugin ships (is built, packaged, listed) when its folder has a Makefile or generates one. */
export const ships = (d) => d.binding === 'instance' || existsSync(join(d.dir, 'Makefile'));

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
      ...(d.manual ?? ['| Parameter | Range | Default |', '|---|---|---|', ...d.params.map(paramRow).map((r) => `| ${r.name} | ${r.range} | ${r.default} |`), '', "Plus the host's bypass."]),
    ]).concat(['']),
  },
  'packaging/omx-plugins.spec': {
    'clap-files': (ds) => ds.map((d) => `/usr/lib/clap/${d.stem}.clap`),
    'lv2-files': (ds) => ds.map((d) => `%{_libdir}/lv2/${d.stem}.lv2/`),
  },
  '.github/workflows/ci.yml': {
    plugins: (ds) => [`shorts="${ds.map((d) => d.stem.replace(/^omx-/, '')).join(' ')}"`, `kernels="${ds.map((d) => d.kernel).join(' ')}"`],
  },
};

/** `text` with each `BEGIN GENERATED <name>` .. `END GENERATED <name>` region refilled; the inner
 * lines take the BEGIN line's indentation. A region missing, unclosed or unknown throws. */
export function fillRegions(file, text, producers, ds) {
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
    out.push(...producers[name](ds).map((l) => (l ? `${indent}${l}` : l)));
    out.push(lines[end]);
    seen.add(name);
    i = end;
  }
  const absent = Object.keys(producers).filter((n) => !seen.has(n));
  if (absent.length) throw new Error(`${file}: no region ${absent.map((n) => `'${n}'`).join(', ')}`);
  return out.join('\n');
}

/** Every shared file: path relative to the tree -> text. */
export function generateShared(root = ROOT, decls = pluginDirs(root).map((p) => loadDecl(p))) {
  const ds = decls.filter(ships).sort(byStem);
  return Object.fromEntries(Object.entries(SHARED).map(([file, producers]) => [file, fillRegions(file, readFileSync(join(root, file), 'utf8'), producers, ds)]));
}

/** The shared view the hint check reads: every plugin's own plugins/<stem>/port-hints.json by stem.
 * A build product (tools/test/port-hints.json, git-ignored), never hand-edited. */
export const HINTS_VIEW = 'tools/test/port-hints.json';
export function hintsView(root = ROOT) {
  const view = {};
  for (const dir of pluginDirs(root)) {
    const stem = basename(dir), file = join(dir, 'port-hints.json');
    if (existsSync(file)) view[stem] = JSON.parse(readFileSync(file, 'utf8'));
  }
  return view;
}
export const hintsViewText = (root = ROOT) => `${JSON.stringify(hintsView(root), null, 2)}\n`;

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
  const decls = (dirs.length ? dirs : pluginDirs()).map((dir) => loadDecl(dir));
  for (const d of decls) {
    for (const [rel, text] of Object.entries(generate(d))) put(join(d.dir, rel), join('plugins', d.stem, rel), text, `make -C plugins/${d.stem} gen`);
  }
  if (!dirs.length) for (const [rel, text] of Object.entries(generateShared(ROOT, decls))) put(join(ROOT, rel), rel, text, 'node tools/gen.mjs');
  if (!dirs.length) {
    const viewPath = join(ROOT, HINTS_VIEW);
    // absent is fine (it is built by `make hints`); present must be fresh, so a hand edit goes stale
    const text = hintsViewText(), now = existsSync(viewPath) ? readFileSync(viewPath, 'utf8') : null;
    if (check && now !== null && now !== text) {
      console.error(`gen: STALE ${HINTS_VIEW} (run \`node tools/gen.mjs\`)`);
      stale++;
    } else if (!check && now !== text) writeFileSync(viewPath, text); // a build product, not a file the wizard commits
  }
  if (stale) process.exit(1);
  if (check) console.log(`gen: every generated file is fresh${dirs.length ? '' : ', the shared files too'}`);
}

if (process.argv[1] && resolve(process.argv[1]) === fileURLToPath(import.meta.url)) main(process.argv.slice(2));
