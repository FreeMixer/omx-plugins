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
  const seen = new Set();
  for (const p of d.params) {
    if (!/^[A-Za-z][A-Za-z0-9_]*$/.test(p.symbol)) throw new Error(`${file}: symbol '${p.symbol}' is not an LV2 symbol`);
    if (seen.has(p.symbol)) throw new Error(`${file}: symbol '${p.symbol}' declared twice`);
    seen.add(p.symbol);
    if (!(p.min <= p.def && p.def <= p.max && p.min < p.max)) throw new Error(`${file}: '${p.symbol}' travel ${p.min}..${p.max} default ${p.def}`);
    if (p.kind && p.kind !== 'integer' && p.kind !== 'toggle') throw new Error(`${file}: '${p.symbol}' kind '${p.kind}'`);
    if (p.kind === 'toggle' && (p.min !== 0 || p.max !== 1)) throw new Error(`${file}: toggle '${p.symbol}' is not 0..1`);
  }
  return { ...d, dir: resolve(dir) };
}

/** SHA-256 of the resolved parameter list: the org.openmixer.declaration/1 digest, the SAME formula
 * openmixer's clap-gen.mjs uses, so the engine's stale-parameter check reads ours unchanged. */
export function digestOf(params) {
  const rows = params.map((p) => [p.symbol, p.unit, p.min, p.max, p.def, p.kind ? 1 : 0, p.kind === 'toggle']);
  return createHash('sha256').update(JSON.stringify(rows)).digest('hex');
}

/** The LV2 port list, in index order: audio, the declared parameters, enabled, latency. One rule,
 * read by the TTL, the MOD GUI and (as macros) the LV2 C face. */
export function lv2Ports(d) {
  const audio = [
    ['in_l', 'In L', 'lv2:AudioPort , lv2:InputPort'], ['in_r', 'In R', 'lv2:AudioPort , lv2:InputPort'],
    ['out_l', 'Out L', 'lv2:AudioPort , lv2:OutputPort'], ['out_r', 'Out R', 'lv2:AudioPort , lv2:OutputPort'],
  ].map(([symbol, name, a]) => ({ symbol, name, a }));
  const params = d.params.map((p) => ({ symbol: p.symbol, name: p.name, a: 'lv2:ControlPort , lv2:InputPort', param: p }));
  return [
    ...audio,
    ...params,
    { symbol: 'enabled', name: 'Enabled', a: 'lv2:ControlPort , lv2:InputPort', enabled: true },
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
 * Produced by tools/gen.mjs from plugins/${d.stem}/${d.stem}.decl.json (openmixer's
 * ${d.source}).
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
/* org.openmixer.declaration/1: the source expression and the digest of the resolved parameters. */
#define OMX_${K}_DECL_SOURCE "${d.source.replace(/"/g, '\\"')}"
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
      if (q.kind === 'integer') lines.push('        lv2:portProperty lv2:integer ;');
      if (q.kind === 'toggle') lines.push('        lv2:portProperty lv2:toggled ;');
      if (q.unit === 'ms') lines.push('        units:unit units:ms ;');
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
@prefix rdfs:  <http://www.w3.org/2000/01/rdf-schema#> .
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
