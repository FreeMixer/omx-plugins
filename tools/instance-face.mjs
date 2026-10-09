// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * instance-face.mjs — omx-dsp's instance face of one kernel (<omxdsp/fx/omx_<kernel>_instance.h>),
 * read for the generated binding: its prototypes, and each `resolve` argument bound BY NAME to the
 * declaration's parameter whose contract control it is (spec 2026-10-09-plugin-from-contract §2).
 *
 * The shape the generated binding calls:
 *
 *   int  omx_<k>_instance_init(Omx<K>Instance *s, float|double sr);
 *   void omx_<k>_instance_resolve(Omx<K>Instance *s, int bypass, <float|int> <control>, ...);
 *   void omx_<k>_instance_run(Omx<K>Instance *s, const float *in_l, const float *in_r,
 *                             float *out_l, float *out_r, uint32_t n);
 *   latency: omx_<k>_instance_latency(const Omx<K>Instance *s), else OMX_<K>_INSTANCE_LATENCY_FRAMES
 *
 * A face of another shape (ring buffers handed to init, a ports struct, an array of gains) is not
 * refused as wrong: it is not this binding's, and the plugin keeps a hand-written one.
 *
 * Binding: argument `a` takes the parameter whose control name, in snake case, is `a`
 * (`attackDb` → `attack_db`). An argument that still carries an older short name binds when its
 * words are, in order, a subset of exactly one control's words (`attack_ms` of `attack_time_ms`);
 * it is reported as a RENAME owed by omx-dsp's name conformance, and OMX_BINDING_STRICT=1 refuses it.
 */
import { existsSync, readFileSync } from 'node:fs';
import { join } from 'node:path';

const stripC = (s) => s.replace(/\/\*[\s\S]*?\*\//g, ' ').replace(/\/\/[^\n]*/g, ' ');

/** The instance header of `kernel` under omx-dsp's include directory, or undefined. */
export function instanceHeader(inc, kernel) {
  if (!inc) return undefined;
  const base = existsSync(join(inc, 'omxdsp')) ? join(inc, 'omxdsp') : inc;
  const p = join(base, 'fx', `omx_${kernel}_instance.h`);
  return existsSync(p) ? p : undefined;
}

const argsOf = (list) =>
  list.split(',').map((a) => a.trim()).filter((a) => a && a !== 'void').map((a) => {
    const m = a.match(/^(.*?)([A-Za-z_]\w*)\s*(\[[^\]]*\])?$/);
    return { type: m ? `${m[1].trim()}${m[3] ?? ''}`.replace(/\s+/g, ' ').replace(/\s*\*\s*/g, ' *').trim() : a, name: m ? m[2] : a };
  });

/**
 * The face's prototypes in the generated binding's shape: `{ type, srType, args: [{type, name}],
 * latency: { fn } | { macro } }`, or `{ error }` naming what does not fit.
 */
export function parseFace(text, kernel) {
  const src = stripC(text);
  const K = kernel.toUpperCase();
  const fn = (name) => {
    const m = src.match(new RegExp(`static\\s+inline\\s+[\\w\\s*]+?\\bomx_${kernel}_instance_${name}\\s*\\(([^)]*)\\)\\s*\\{`));
    return m ? argsOf(m[1]) : undefined;
  };
  const init = fn('init'), resolve = fn('resolve'), run = fn('run'), lat = fn('latency');
  const missing = [['init', init], ['resolve', resolve], ['run', run]].filter(([, a]) => !a).map(([n]) => `omx_${kernel}_instance_${n}`);
  if (missing.length) return { error: `the face defines no ${missing.join(', ')}` };
  const T = init[0]?.type.replace(/ \*$/, '');
  if (!T || !/^Omx\w+Instance$/.test(T)) return { error: `omx_${kernel}_instance_init's first argument is not an instance pointer (${init[0]?.type})` };
  if (init.length !== 2 || !/^(float|double)$/.test(init[1].type))
    return { error: `omx_${kernel}_instance_init takes (${init.map((a) => a.type).join(', ')}), not (${T} *, float sr): the face holds memory the caller hands in` };
  if (resolve.length < 2 || resolve[0].type !== `${T} *` || resolve[1].type !== 'int')
    return { error: `omx_${kernel}_instance_resolve does not begin (${T} *, int bypass)` };
  const scalar = resolve.slice(2);
  const odd = scalar.filter((a) => !/^(float|int)$/.test(a.type));
  if (odd.length) return { error: `omx_${kernel}_instance_resolve takes ${odd.map((a) => `${a.type} ${a.name}`).join(', ')}, not one scalar per control` };
  const runWant = [`${T} *`, 'const float *', 'const float *', 'float *', 'float *', 'uint32_t'];
  if (run.map((a) => a.type).join('|') !== runWant.join('|')) return { error: `omx_${kernel}_instance_run takes (${run.map((a) => a.type).join(', ')}), not (${runWant.join(', ')})` };
  let latency;
  if (lat && lat.length === 1 && lat[0].type === `const ${T} *`) latency = { fn: `omx_${kernel}_instance_latency` };
  else if (new RegExp(`#\\s*define\\s+OMX_${K}_INSTANCE_LATENCY_FRAMES\\b`).test(src)) latency = { macro: `OMX_${K}_INSTANCE_LATENCY_FRAMES` };
  else return { error: `the face publishes no latency (omx_${kernel}_instance_latency or OMX_${K}_INSTANCE_LATENCY_FRAMES)` };
  return { type: T, srType: init[1].type, args: scalar, latency };
}

/** A control name in snake case: attackTimeMs -> attack_time_ms. */
export const snake = (s) => s.replace(/([a-z0-9])([A-Z])/g, '$1_$2').toLowerCase();

const subsequence = (small, big) => {
  let i = 0;
  for (const w of big) if (w === small[i]) i++;
  return i === small.length;
};

/**
 * Bind the face's `resolve` arguments to the declaration's parameters. `controls[i]` is the
 * contract control of parameter i (`{ name }`), or undefined for a parameter that is the plugin's
 * own. Returns `{ binding: [{ arg, type, param, control, rename? }], renames, errors }`, in the
 * face's argument order.
 */
export function bindFace(face, params, controls, { strict = process.env.OMX_BINDING_STRICT === '1' } = {}) {
  const errors = [];
  const own = params.filter((_p, i) => !controls[i]);
  if (own.length) errors.push(`${own.map((p) => `'${p.symbol}'`).join(', ')} ${own.length > 1 ? 'are' : 'is'} no contract control: a generated binding passes the contract's controls only`);
  const wanted = params.map((p, i) => ({ p, i, words: controls[i] ? snake(controls[i].name).split('_') : null })).filter((x) => x.words);
  const taken = new Set();
  const binding = [];
  for (const a of face.args) {
    let hit = wanted.filter((w) => w.words.join('_') === a.name);
    let rename;
    if (hit.length !== 1) {
      hit = wanted.filter((w) => subsequence(a.name.split('_'), w.words));
      if (hit.length === 1) rename = hit[0].words.join('_');
    }
    if (hit.length !== 1) {
      errors.push(`resolve argument '${a.name}' is ${hit.length ? `ambiguous (${hit.map((h) => h.p.symbol).join(', ')})` : 'no contract control of the declaration'}`);
      continue;
    }
    const { p, i } = hit[0];
    if (taken.has(i)) {
      errors.push(`'${p.symbol}' binds two resolve arguments`);
      continue;
    }
    taken.add(i);
    if (rename && strict) errors.push(`resolve argument '${a.name}' is not the contract's '${rename}' (OMX_BINDING_STRICT)`);
    binding.push({ arg: a.name, type: a.type, param: p, index: i, control: controls[i].name, ...(rename ? { rename } : {}) });
  }
  for (const w of wanted) if (!taken.has(w.i)) errors.push(`'${w.p.symbol}' (control ${controls[w.i].name}) is no resolve argument of the face`);
  return { binding, renames: binding.filter((b) => b.rename), errors };
}

/** Read, parse and bind in one step: `{ face, header, binding, renames, errors }`. */
export function faceBinding(inc, kernel, params, controls) {
  const header = instanceHeader(inc, kernel);
  if (!header) return { errors: [`omx-dsp has no <omxdsp/fx/omx_${kernel}_instance.h>${inc ? '' : ' (no omx-dsp include directory: pkg-config omxdsp, or OMXDSP_INCLUDE)'}: its instance face is omx-dsp's work first`] };
  const face = parseFace(readFileSync(header, 'utf8'), kernel);
  if (face.error) return { header, errors: [`<omxdsp/fx/omx_${kernel}_instance.h>: ${face.error}`] };
  return { header, face, ...bindFace(face, params, controls) };
}
