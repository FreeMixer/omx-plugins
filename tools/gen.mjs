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
 * The MOD GUI of the same bundle is tools/modgui-gen.mjs's, which reads the port list from here.
 *
 * Usage: `node tools/gen.mjs [--check] [plugins/<stem> ...]` — `--check` writes nothing and fails on a
 * stale, missing or changed file. No clock and no absolute path reach the output.
 */
import { createHash } from 'node:crypto';
import { existsSync, mkdirSync, readFileSync, readdirSync, writeFileSync } from 'node:fs';
import { basename, dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { PIN_FILE, locateContract, paramKernel, resolveParam } from './omx-contract.mjs';

export const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..');

const BANNER = '// SPDX-License-Identifier: GPL-3.0-or-later\n// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>\n';
const TTL_BANNER = '# SPDX-License-Identifier: GPL-3.0-or-later\n# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>\n';

/** Read and validate a plugin's declaration. Everything downstream trusts what this returns. */
export function loadDecl(dir) {
  const stem = basename(resolve(dir));
  const file = join(dir, `${stem}.decl.json`);
  const d = JSON.parse(readFileSync(file, 'utf8'));
  if (d.stem !== stem) throw new Error(`${file}: stem '${d.stem}' is not the directory's '${stem}'`);
  if (!/^[a-z][a-z0-9_]*$/.test(d.kernel)) throw new Error(`${file}: kernel '${d.kernel}' is not a C identifier`);
  d.params = resolveParams(file, d);
  const seen = new Set();
  for (const p of d.params) {
    if (!/^[A-Za-z][A-Za-z0-9_]*$/.test(p.symbol)) throw new Error(`${file}: symbol '${p.symbol}' is not an LV2 symbol`);
    if (seen.has(p.symbol)) throw new Error(`${file}: symbol '${p.symbol}' declared twice`);
    seen.add(p.symbol);
    if (!(p.min <= p.def && p.def <= p.max && p.min < p.max)) throw new Error(`${file}: '${p.symbol}' travel ${p.min}..${p.max} default ${p.def}`);
    if (p.kind && p.kind !== 'integer' && p.kind !== 'toggle') throw new Error(`${file}: '${p.symbol}' kind '${p.kind}'`);
    if (p.kind === 'toggle' && (p.min !== 0 || p.max !== 1)) throw new Error(`${file}: toggle '${p.symbol}' is not 0..1`);
    if (p.values && (p.kind !== 'integer' || p.values.length !== p.max - p.min + 1)) throw new Error(`${file}: '${p.symbol}' values name every step of an integer travel`);
  }
  if (d.sidechain && (seen.has(d.sidechain.symbol) || FIXED_PORTS.includes(d.sidechain.symbol))) throw new Error(`${file}: sidechain symbol '${d.sidechain.symbol}' is taken`);
  return { ...d, dir: resolve(dir) };
}

/** The travel fields a parameter BY REFERENCE reads from omx-contract and must never retype. */
export const TRAVEL_FIELDS = ['min', 'max', 'def', 'unit', 'kind'];

/** Each parameter with its travel: as typed (a declaration from before references), or read from
 * omx-contract at the pin through its `ref` (tools/omx-contract.mjs). */
function resolveParams(file, d) {
  if (!d.params.some((p) => p.ref !== undefined)) return d.params;
  // the tree the declaration sits in; a declaration copied out of its tree (a test's scratch copy)
  // reads this tree's pin
  const tree = resolve(dirname(file), '..', '..');
  const where = locateContract(existsSync(join(tree, PIN_FILE)) ? tree : ROOT);
  if (!where.dir) throw new Error(`${file}: its parameters are by reference and ${where.why}`);
  return d.params.map((p) => {
    if (p.ref === undefined) throw new Error(`${file}: '${p.symbol}' has no ref; a declaration is by reference throughout or not at all`);
    const typed = TRAVEL_FIELDS.filter((k) => k in p);
    if (typed.length) throw new Error(`${file}: '${p.symbol}' is by reference and retypes ${typed.join(', ')}; omx-contract holds them`);
    const t = resolveParam(where.dir, paramKernel(d, p), p);
    const out = { ...p, min: t.min, max: t.max, def: t.def, unit: t.unit };
    if (t.kind) out.kind = t.kind;
    return out;
  });
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
      if (q.kind === 'integer') lines.push(`        lv2:portProperty lv2:integer${q.values ? ' , lv2:enumeration' : ''} ;`);
      if (q.scale === 'log') lines.push('        lv2:portProperty pprops:logarithmic ;');
      if (q.kind === 'toggle') lines.push('        lv2:portProperty lv2:toggled ;');
      if (q.unit === 'ms') lines.push('        units:unit units:ms ;');
      if (q.values) {
        const points = q.values.map((label, k) => `            [ rdfs:label ${ttlStr(label)} ; rdf:value ${q.min + k} ]`);
        lines.push(`        lv2:scalePoint\n${points.join(' ,\n')} ;`);
      }
    } else if (p.sidechain) {
      // optional: a host that routes nothing to the key gets the effect's own detector
      lines.push('        lv2:portProperty lv2:isSideChain , lv2:connectionOptional ;');
    } else if (p.enabled) {
      lines.push('        lv2:default 1 ;', '        lv2:minimum 0 ;', '        lv2:maximum 1 ;', '        lv2:designation lv2:enabled ;', '        lv2:portProperty lv2:toggled ;');
    } else if (p.latency) {
      lines.push('        lv2:designation lv2:latency ;', '        lv2:portProperty lv2:reportsLatency , lv2:integer ;');
    }
    return `    [\n${lines.join('\n').replace(/ ;$/, '')}\n    ]`;
  };
  return `${TTL_BANNER}# GENERATED by tools/gen.mjs from plugins/${d.stem}/${d.stem}.decl.json — DO NOT EDIT BY HAND.
@prefix doap:  <http://usefulinc.com/ns/doap#> .
@prefix foaf:  <http://xmlns.com/foaf/0.1/> .
@prefix lv2:   <http://lv2plug.in/ns/lv2core#> .
${d.params.some((q) => q.scale === 'log') ? '@prefix pprops: <http://lv2plug.in/ns/ext/port-props#> .\n' : ''}${d.params.some((q) => q.values) ? '@prefix rdf:   <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n' : ''}@prefix rdfs:  <http://www.w3.org/2000/01/rdf-schema#> .
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

/** Every generated file of one plugin: path relative to the plugin dir -> text. */
export function generate(d) {
  const lv2 = `generated/${d.stem}.lv2`;
  return {
    [`generated/omx_${d.kernel}_params.h`]: emitParamsHeader(d),
    [`${lv2}/manifest.ttl`]: emitManifest(d, { modgui: Boolean(d.panel) }),
    [`${lv2}/${d.stem}.ttl`]: emitPluginTtl(d),
  };
}

export const pluginDirs = () =>
  readdirSync(join(ROOT, 'plugins')).map((n) => join(ROOT, 'plugins', n)).filter((p) => existsSync(join(p, `${basename(p)}.decl.json`)));

function main(argv) {
  const check = argv.includes('--check');
  const dirs = argv.filter((a) => !a.startsWith('--'));
  let stale = 0;
  for (const dir of dirs.length ? dirs : pluginDirs()) {
    const d = loadDecl(dir);
    for (const [rel, text] of Object.entries(generate(d))) {
      const path = join(d.dir, rel);
      const now = existsSync(path) ? readFileSync(path, 'utf8') : null;
      if (now === text) continue;
      if (check) {
        console.error(`gen: STALE ${join('plugins', d.stem, rel)} (run \`make -C plugins/${d.stem} gen\`)`);
        stale++;
      } else {
        mkdirSync(dirname(path), { recursive: true });
        writeFileSync(path, text);
        console.log(`gen: wrote ${join('plugins', d.stem, rel)}`);
      }
    }
  }
  if (stale) process.exit(1);
  if (check) console.log('gen: every generated file is fresh');
}

if (process.argv[1] && resolve(process.argv[1]) === fileURLToPath(import.meta.url)) main(process.argv.slice(2));
