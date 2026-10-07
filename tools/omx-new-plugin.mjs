#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * omx-new-plugin.mjs — `omx new plugin`: a new plugin, every artifact of the plugin recipe
 * (recipes/plugin.recipe.json) written BEFORE any hand work starts.
 *
 *   node tools/omx-new-plugin.mjs --answers <file.json> [--root <tree>] [--plan-json <file>] [--no-run]
 *   node tools/omx-new-plugin.mjs [--save-answers <file.json>] ...      (interactive, on a terminal)
 *
 * 1. The answers are the declaration less what the recipe derives (`derived`: name, vendor, url,
 *    version, CLAP id, LV2 URI), plus the recipe's own questions (`questions`). The questions are the
 *    schema's titles and descriptions and the recipe's; none is kept here.
 * 2. Every answer is validated before a file is written: the schema; the stem, CLAP id and LV2 URI
 *    unique among the plugins; every parameter BY REFERENCE (a typed travel is refused); the kernel
 *    in omx-contract at the pin (or at the release the answers name), else NEW, which means the
 *    kernel recipe in omx-contract and omx-dsp runs first; every reference resolving there; the
 *    panel naming declared parameters.
 * 3. It writes every artifact from the recipe's templates: the declaration, the Makefile, the CLAP
 *    and LV2 faces, the kernel binding and the kernel-identity test as RED-FIRST stubs (each marked
 *    OMX_WIZARD_STUB, each failing a named test until it is written), the packaging lines, the
 *    README row and section and the CHANGELOG entry; then the generated files through tools/gen.mjs.
 * 4. It prints the COMMIT PLAN: one concern per commit, in the recipe's layer order, each with its
 *    files and a plain message (tools/commit-plan-check.mjs holds a range to the same order).
 * 5. It runs `gen.mjs --check`, the plugin's `make test` and the completeness test for the plugin,
 *    and prints the remaining checklist: the red tests and the hand-written artifacts left.
 *
 * Exit 0 the plugin was written (its red tests are expected); 1 the answers were refused, each
 * reason on its own line, nothing written; 2 usage.
 */
import { execFileSync, spawnSync } from 'node:child_process';
import { existsSync, mkdirSync, readFileSync, writeFileSync } from 'node:fs';
import { dirname, join, relative, resolve } from 'node:path';
import { createInterface } from 'node:readline/promises';
import { fileURLToPath } from 'node:url';
import { compareVersions, contractPin, declKernels, kernelExists, locateContract, paramKernel, resolveParam, PIN_FILE } from './omx-contract.mjs';
import {
  ROOT,
  checkPlugin,
  fill,
  gapLines,
  layerOfPath,
  loadRecipe,
  loadSchema,
  omxdspInclude,
  pluginFacts,
  pluginStems,
  render,
  validate,
} from './plugin-recipe.mjs';

// ---- the derived answers -------------------------------------------------------------------

/** How each pointer the recipe declares as derived is computed. A pointer the recipe derives and
 * this table lacks throws: the recipe and the wizard cannot drift. */
const DERIVERS = {
  '/name': (a) => `omx ${a.stem.replace(/^omx-/, '')}`,
  '/vendor': (_a, ctx) => ctx.schema.$defs.plugin.properties.vendor.const,
  '/url': (_a, ctx) => ctx.schema.$defs.plugin.properties.url.const,
  '/version': (_a, ctx) => ctx.version,
  '/clap/id': (a) => `org.openmixer.${a.stem.replace(/^omx-/, '')}`,
  '/lv2/uri': (a) => `urn:openmixer:${a.stem.replace(/^omx-/, '')}`,
  '/$comment': (a) => `THE declaration of ${a.stem}. Every face and the parameter header are generated from this file by tools/gen.mjs. Its parameters are by reference to omx-contract's ${a.kernel} kernel. Parameter order is append-only.`,
};

const getPtr = (obj, ptr) => ptr.split('/').slice(1).reduce((o, k) => o?.[k], obj);
function setPtr(obj, ptr, v) {
  const keys = ptr.split('/').slice(1);
  let o = obj;
  for (const k of keys.slice(0, -1)) o = o[k] ??= {};
  o[keys.at(-1)] = v;
}

/** The tree's release version, from the file and variable the recipe names. */
function treeVersion(root, recipe) {
  const { file, var: v } = recipe.tree.versionFrom;
  const m = readFileSync(join(root, file), 'utf8').match(new RegExp(`^${v}\\s*:?=\\s*(\\S+)`, 'm'));
  if (!m) throw new Error(`${file}: no ${v}`);
  return m[1];
}

function omxdspMinOf(root, recipe) {
  const { file, pattern } = recipe.tree.omxdspMinFrom;
  const m = readFileSync(join(root, file), 'utf8').match(new RegExp(pattern, 'm'));
  return m?.[1];
}

/** The declaration in the schema's key order, at every depth (the canonical form every declaration
 * is written in). */
function canonical(schema, decl) {
  const deref = (node) => (node?.$ref ? deref(node.$ref.replace(/^#\//, '').split('/').reduce((n, k) => n?.[k], schema)) : node);
  const order = (node, value) => {
    const s = deref(node);
    if (Array.isArray(value)) return value.map((v) => order(s?.items, v));
    if (value === null || typeof value !== 'object' || !s?.properties) return value;
    const keys = [...Object.keys(s.properties).filter((k) => k in value), ...Object.keys(value).filter((k) => !(k in s.properties))];
    return Object.fromEntries(keys.map((k) => [k, order(s.properties[k], value[k])]));
  };
  return order(schema.$defs.plugin, decl);
}

// ---- the plan: validate everything, write nothing ---------------------------------------------

export async function planPlugin(answers, { root = ROOT, recipe = loadRecipe(root) } = {}) {
  const schema = loadSchema(root, recipe);
  const refusals = [];
  const refuse = (field, reason) => refusals.push({ field, reason });
  const notes = [];

  // the recipe's own questions, apart from the declaration
  const extra = {};
  const decl = {};
  for (const [k, v] of Object.entries(structuredClone(answers))) {
    if (k in recipe.questions) extra[k] = v;
    else decl[k] = v;
  }
  for (const [k, q] of Object.entries(recipe.questions)) {
    if (extra[k] !== undefined) for (const e of validate(q, q, extra[k], `#/questions/${k}`)) refuse(`#/questions/${k}`, e);
  }
  extra.omxdspMin ??= omxdspMinOf(root, recipe);

  // the derived answers: filled when absent, checked when given
  const ctx = { schema, version: treeVersion(root, recipe) };
  if (typeof decl.stem === 'string' && typeof decl.kernel === 'string') {
    for (const ptr of Object.keys(recipe.derived)) {
      const derive = DERIVERS[ptr];
      if (!derive) throw new Error(`${recipe.item} recipe derives ${ptr}, which the wizard cannot compute`);
      const want = derive(decl, ctx);
      const given = getPtr(decl, ptr);
      if (given === undefined) setPtr(decl, ptr, want);
      else if (ptr !== '/$comment' && given !== want) refuse(ptr, `'${given}' is derived: it is '${want}'`);
    }
  }
  if (typeof decl.kernel === 'string') decl.kernels ??= [decl.kernel];
  for (const e of validate(schema, schema, decl)) refuse(e.split(':')[0], `schema: ${e.slice(e.indexOf(':') + 2)}`);
  if (refusals.length) return { ok: false, refusals, notes };

  // unique among the plugins of the tree
  const stems = pluginStems(root);
  if (stems.includes(decl.stem) || existsSync(join(root, 'plugins', decl.stem))) refuse('/stem', `plugins/${decl.stem} already exists`);
  for (const s of stems) {
    const o = pluginFacts(root, s);
    if (o.clapId === decl.clap.id) refuse('/clap/id', `'${decl.clap.id}' is ${s}'s`);
    if (o.uri === decl.lv2.uri) refuse('/lv2/uri', `'${decl.lv2.uri}' is ${s}'s`);
    if (o.kernel === decl.kernel) notes.push(`kernel '${decl.kernel}' is also ${s}'s: both faces then share its file in omx-contract`);
  }

  // parameters: by reference, unique, and the panel names them
  const symbols = new Set();
  decl.params.forEach((p, i) => {
    if (symbols.has(p.symbol)) refuse(`/params/${i}/symbol`, `'${p.symbol}' declared twice`);
    symbols.add(p.symbol);
    if (p.ref === undefined) refuse(`/params/${i}`, `'${p.symbol}' has no ref: a parameter is by reference to omx-contract, never typed`);
    const typed = ['min', 'max', 'def', 'unit', 'kind'].filter((k) => k in p);
    if (typed.length) refuse(`/params/${i}`, `'${p.symbol}' retypes ${typed.join(', ')}: omx-contract holds them`);
  });
  for (const s of decl.panel?.sections ?? []) for (const c of s.controls) if (!symbols.has(c)) refuse('/panel/sections', `section '${s.key}' names '${c}', which is no parameter`);
  for (const [role, sym] of Object.entries(decl.panel?.roles ?? {})) if (!symbols.has(sym)) refuse(`/panel/roles/${role}`, `'${sym}' is no parameter`);

  // the kernels: all at the pin, all at the release the answers name, or NEW
  const kernels = declKernels(decl);
  const pin = contractPin(root);
  const where = locateContract(root);
  let kernel = { state: 'new', at: undefined, dir: undefined };
  const missingAt = (dir) => kernels.filter((k) => !kernelExists(dir, k));
  if (where.dir && !missingAt(where.dir).length) kernel = { state: 'existing', at: `omx-contract ${pin}`, dir: where.dir };
  else if (extra.contractRelease) {
    if (compareVersions(extra.contractRelease, pin) <= 0) refuse('#/questions/contractRelease', `${extra.contractRelease} is not after the pin ${pin}`);
    const env = process.env.OMX_CONTRACT_DIR;
    let v;
    try {
      v = env && JSON.parse(readFileSync(join(env, 'package.json'), 'utf8')).version;
    } catch {
      v = undefined;
    }
    if (env && v === extra.contractRelease && !missingAt(env).length) kernel = { state: 'released', at: `omx-contract ${v} (OMX_CONTRACT_DIR)`, dir: env };
    else kernel = { state: 'new', at: `omx-contract ${extra.contractRelease}, to be cut by the kernel recipe`, dir: undefined };
  }
  const newKernels = where.dir ? missingAt(where.dir) : kernels;
  if (!where.dir && kernel.state === 'new') notes.push(`no omx-contract ${pin} reachable (${where.why}): the kernels count as NEW until they are checked`);

  // every reference resolves in its kernel's file (or is owed by the kernel recipe)
  const resolved = [];
  const owedNames = new Map(kernels.map((k) => [k, new Set()]));
  for (const [i, p] of decl.params.entries()) {
    let k;
    try {
      k = paramKernel(decl, p);
    } catch (e) {
      refuse(`/params/${i}/kernel`, e.message);
      continue;
    }
    if (kernel.dir) {
      try {
        resolved.push({ ...p, ...resolveParam(kernel.dir, k, p) });
      } catch (e) {
        refuse(`/params/${i}/ref`, e.message);
      }
    } else {
      owedNames.get(k).add(p.ref);
      if (p.defaultRef) owedNames.get(k).add(p.defaultRef);
    }
  }

  // the panel, drawn as the MOD GUI generator will draw it (a selector it cannot draw is refused now)
  if (decl.panel && resolved.length === decl.params.length) {
    const mg = await import(join(root, 'tools', 'modgui-gen.mjs'));
    try {
      mg.resolvePanel({ ...decl, params: resolved });
      const drawn = new Set(decl.panel.sections.flatMap((x) => x.controls));
      const undrawn = decl.params.filter((p) => !drawn.has(p.symbol)).map((p) => p.symbol);
      if (undrawn.length) throw new Error(`the MOD GUI draws every parameter (tools/modgui-test.sh), and the panel leaves out ${undrawn.join(', ')}`);
    } catch (e) {
      refuse('/panel', `${e.message}; leave the panel out until the MOD GUI draws it, or keep that parameter off it`);
    }
  }
  if (kernel.state === 'new') {
    for (const k of newKernels.length ? newKernels : kernels) {
      notes.push(`kernel '${k}': NEW — not in omx-contract ${pin}. The kernel recipe in omx-contract and omx-dsp runs first ` +
        `(\`omx new kernel ${k}\`): data/kernels/${k}.json must export ${[...owedNames.get(k)].join(', ') || '(no reference yet)'}, then omx-contract is released and ${PIN_FILE} moves to it.`);
    }
  } else notes.push(`kernel${kernels.length > 1 ? 's' : ''} ${kernels.map((k) => `'${k}'`).join(', ')}: in ${kernel.at}; every reference resolves there`);
  const inc = omxdspInclude();
  if (inc) {
    for (const k of kernels) {
      const fx = join(existsSync(join(inc, 'omxdsp')) ? join(inc, 'omxdsp') : inc, 'fx', `omx_${k}.h`);
      notes.push(existsSync(fx) ? `omx-dsp has <omxdsp/fx/omx_${k}.h>: the binding calls it` : `omx-dsp has no <omxdsp/fx/omx_${k}.h>: the kernel recipe moves it into omx-dsp first`);
    }
  }

  return { ok: refusals.length === 0, refusals, notes, decl: canonical(schema, decl), extra, kernel, resolved, pin };
}

// ---- writing ------------------------------------------------------------------------------

/** The template view of a plan. */
function viewOf(plan) {
  const d = plan.decl;
  const K = d.kernel.toUpperCase();
  const Kernel = d.kernel.replace(/(^|_)([a-z])/g, (_m, _u, c) => c.toUpperCase());
  const num = (n) => `${n}`;
  const params = (plan.resolved.length ? plan.resolved : d.params).map((p) => {
    const unit = p.unit ? ` ${p.unit}` : '';
    if (p.min === undefined) return { name: p.name, range: `(${p.ref})`, default: `(${p.defaultRef ?? p.ref})` };
    if (p.kind === 'toggle') return { name: p.name, range: 'off / on', default: p.def ? 'on' : 'off' };
    if (p.values) return { name: p.name, range: p.values.join(' / '), default: p.values[p.def] };
    return { name: p.name, range: `${num(p.min)} to ${num(p.max)}${unit}${p.kind === 'integer' ? ', whole steps' : ''}`, default: `${num(p.def)}${unit}` };
  });
  return {
    stem: d.stem, kernel: d.kernel, K, Kernel, name: d.name, uri: d.lv2.uri, clapId: d.clap.id,
    description: d.description, omxdspMin: plan.extra.omxdspMin, panel: Boolean(d.panel), params,
  };
}

/** Insert one rendered block into a file by the recipe's anchor; returns the new text. */
function insertBlock(textIn, block, tpl) {
  const lines = textIn.split('\n');
  const body = block.replace(/\n+$/, '').split('\n');
  if (tpl.afterLast) {
    const re = new RegExp(tpl.afterLast);
    let at = -1;
    lines.forEach((l, i) => { if (re.test(l)) at = i; });
    if (at < 0) throw new Error(`no line matches ${tpl.afterLast}`);
    lines.splice(at + 1, 0, ...body);
  } else if (tpl.before) {
    const at = lines.findIndex((l) => new RegExp(tpl.before).test(l));
    if (at < 0) throw new Error(`no line matches ${tpl.before}`);
    lines.splice(at, 0, ...body, '');
  } else if (tpl.after) {
    let at = lines.findIndex((l) => new RegExp(tpl.after).test(l));
    if (at < 0 && tpl.create) {
      // the section is not there yet: open it before the first line `createBefore` matches
      const before = lines.findIndex((l) => new RegExp(tpl.createBefore).test(l));
      if (before < 0) throw new Error(`no line matches ${tpl.createBefore}`);
      lines.splice(before, 0, tpl.create, '');
      at = before;
    }
    if (at < 0) throw new Error(`no line matches ${tpl.after}`);
    lines.splice(at + 1, 0, '', ...body);
  }
  return lines.join('\n').replace(/\n{3,}/g, '\n\n');
}

/** Insert each line among its siblings, in sorted order. */
function insertLines(textIn, entries, facts) {
  const lines = textIn.split('\n');
  for (const { line, among } of entries) {
    const want = fill(line, facts);
    if (lines.includes(want)) continue;
    const re = new RegExp(among);
    const sib = lines.map((l, i) => [l, i]).filter(([l]) => re.test(l));
    if (!sib.length) throw new Error(`no line matches ${among}`);
    const after = sib.filter(([l]) => l < want).at(-1);
    lines.splice(after ? after[1] + 1 : sib[0][1], 0, want);
  }
  return lines.join('\n');
}

export function writePlugin(plan, { root = ROOT, recipe = loadRecipe(root) } = {}) {
  const d = plan.decl;
  const view = viewOf(plan);
  const facts = { stem: d.stem, kernel: d.kernel, name: d.name, uri: d.lv2.uri, clapId: d.clap.id, short: d.stem.replace(/^omx-/, '') };
  const written = [];
  const put = (rel, content) => {
    mkdirSync(dirname(join(root, rel)), { recursive: true });
    writeFileSync(join(root, rel), content);
    written.push(rel);
  };
  const tdir = join(root, recipe.templates);
  for (const a of recipe.artifacts) {
    if (a.made === 'DECLARED') put(fill(a.target, facts), `${JSON.stringify(d, null, 2)}\n`);
    else if (a.id === 'kernel-file') {
      if (plan.extra.contractRelease) {
        const pinFile = join(root, PIN_FILE);
        const pin = JSON.parse(readFileSync(pinFile, 'utf8'));
        pin.version = plan.extra.contractRelease;
        put(PIN_FILE, `${JSON.stringify(pin, null, 2)}\n`);
      }
    } else if (typeof a.template === 'string' && a.template.endsWith('.tmpl')) {
      if (written.includes(fill(a.target.split('#')[0], facts))) continue; // one file, several entries
      put(fill(a.target.split('#')[0], facts), render(readFileSync(join(tdir, a.template), 'utf8'), view));
    } else if (a.template && typeof a.template === 'object') {
      const file = a.template.file;
      const before = readFileSync(join(root, file), 'utf8');
      const after = a.template.lines
        ? insertLines(before, a.template.lines, facts)
        : insertBlock(before, render(readFileSync(join(tdir, a.template.insert), 'utf8'), view), a.template);
      if (after !== before) {
        writeFileSync(join(root, file), after);
        if (!written.includes(file)) written.push(file);
      }
    }
  }
  // the generated files, when the references resolve (a NEW kernel's wait for its release)
  const generated = [];
  if (plan.kernel.dir) {
    const env = { ...process.env, ...(plan.kernel.state === 'released' ? { OMX_CONTRACT_DIR: plan.kernel.dir } : {}) };
    const pdir = join(root, 'plugins', d.stem);
    const tools = [join(root, 'tools', 'gen.mjs'), ...(d.panel ? [join(root, 'tools', 'modgui-gen.mjs')] : [])];
    for (const t of tools) {
      const out = execFileSync('node', [t, pdir], { env, cwd: root, encoding: 'utf8' });
      for (const m of out.matchAll(/wrote (\S+)/g)) generated.push(relative(root, resolve(root, m[1])));
    }
  }
  return { written, generated };
}

// ---- the commit plan -------------------------------------------------------------------------

/** One commit per layer that has files, in the recipe's layer order. */
export function commitPlan(recipe, plan, files) {
  const facts = { name: plan.decl.name, kernel: plan.decl.kernel, contractRelease: plan.extra.contractRelease ?? '(the release the kernel recipe cuts)' };
  const byLayer = new Map(recipe.layers.map((l) => [l.id, []]));
  const unmapped = [];
  for (const f of files) {
    const layer = layerOfPath(recipe, f);
    if (layer) byLayer.get(layer).push(f);
    else unmapped.push(f);
  }
  const commits = [];
  for (const l of recipe.layers) {
    const fs = [...new Set(byLayer.get(l.id))].sort();
    if (l.id === 'kernel' && !fs.length && plan.kernel.state === 'new') {
      commits.push({ layer: l.id, files: [PIN_FILE], message: fill(l.message, facts), pending: `after the kernel recipe releases omx-contract with ${declKernels(plan.decl).map((k) => `data/kernels/${k}.json`).join(', ')}: move ${PIN_FILE} to it` });
      continue;
    }
    if (l.id === 'generated' && !fs.length && !plan.kernel.dir) {
      commits.push({ layer: l.id, files: [`plugins/${plan.decl.stem}/generated/`], message: fill(l.message, facts), pending: `after the pin moves: make -C plugins/${plan.decl.stem} gen` });
      continue;
    }
    if (fs.length) commits.push({ layer: l.id, files: fs, message: fill(l.message, facts) });
  }
  if (unmapped.length) throw new Error(`the wizard wrote files no recipe artifact maps to a layer: ${unmapped.join(', ')}`);
  return commits;
}

// ---- the end: generators, tests, checklist -----------------------------------------------------

function run(cmd, args, opts) {
  const r = spawnSync(cmd, args, { encoding: 'utf8', ...opts });
  return { code: r.status, out: `${r.stdout ?? ''}${r.stderr ?? ''}` };
}

async function finish(plan, root, recipe, env) {
  const stem = plan.decl.stem;
  const lines = [];
  const gen = run('node', [join(root, 'tools', 'gen.mjs'), '--check', join(root, 'plugins', stem)], { env });
  lines.push(`gen.mjs --check: ${gen.code === 0 ? 'fresh' : `RED\n${gen.out.trim()}`}`);
  const make = run('make', ['-C', join(root, 'plugins', stem), 'test'], { env });
  const red = make.out.split('\n').filter((l) => /^FAIL /.test(l));
  lines.push(`make -C plugins/${stem} test: ${make.code === 0 ? 'green' : `RED (expected while the stubs stand)${red.length ? `\n  ${red.join('\n  ')}` : `\n  ${make.out.trim().split('\n').slice(-3).join('\n  ')}`}`}`);
  const report = await checkPlugin(root, recipe, stem);
  const gaps = gapLines(report);
  lines.push(`completeness for ${stem}: ${gaps.length} owed entries left`);
  return { lines, gaps, report };
}

// ---- the command ---------------------------------------------------------------------------

function usage(msg) {
  if (msg) process.stderr.write(`omx-new-plugin: ${msg}\n`);
  process.stderr.write('usage: omx-new-plugin.mjs --answers <file.json> [--root <tree>] [--plan-json <file>] [--no-run]\n'
    + '       omx-new-plugin.mjs [--save-answers <file.json>]   (interactive)\n');
  process.exit(2);
}

/** The questions, asked from the schema's titles and descriptions and the recipe's own. */
async function ask(recipe, schema) {
  const rl = createInterface({ input: process.stdin, output: process.stdout });
  const q = async (prop, title, desc, def) => {
    const a = (await rl.question(`${title}${def ? ` [${def}]` : ''}\n  ${desc}\n> `)).trim();
    return a || def;
  };
  const P = schema.$defs.plugin.properties;
  const derived = new Set(Object.keys(recipe.derived));
  const answers = {};
  for (const [k, s] of Object.entries(P)) {
    if (derived.has(`/${k}`) || k.startsWith('$') || k === 'params' || k === 'panel') continue;
    if (s.type === 'object') {
      answers[k] = {};
      for (const [k2, s2] of Object.entries(s.properties)) {
        if (derived.has(`/${k}/${k2}`)) continue;
        const v = await q(k2, s2.title, s2.description, k2 === 'features' ? 'audio-effect,stereo' : k2 === 'class' ? 'lv2:Plugin' : undefined);
        answers[k][k2] = s2.type === 'array' ? v.split(',').map((x) => x.trim()) : v;
      }
    } else answers[k] = await q(k, s.title, s.description);
  }
  answers.params = [];
  const PP = schema.$defs.param.properties;
  for (;;) {
    const symbol = await q('symbol', `Parameter ${answers.params.length + 1}: ${PP.symbol.title} (empty to finish)`, PP.symbol.description);
    if (!symbol) break;
    const p = { symbol };
    for (const k of ['name', 'ref', 'field', 'forKind', 'defaultRef']) {
      const v = await q(k, PP[k].title, PP[k].description);
      if (v) p[k] = v;
    }
    answers.params.push(p);
  }
  for (const [k, s] of Object.entries(recipe.questions)) {
    const v = await q(k, s.title, s.description);
    if (v) answers[k] = v;
  }
  rl.close();
  return answers;
}

async function main(argv) {
  const opt = (name) => {
    const i = argv.indexOf(name);
    return i >= 0 ? argv[i + 1] : undefined;
  };
  const root = resolve(opt('--root') ?? ROOT);
  const recipe = loadRecipe(root);
  let answers;
  const answersPath = opt('--answers');
  if (answersPath) answers = JSON.parse(readFileSync(answersPath, 'utf8'));
  else if (process.stdin.isTTY) answers = await ask(recipe, loadSchema(root, recipe));
  else usage('--answers <file.json> is required when not on a terminal');
  if (opt('--save-answers')) writeFileSync(opt('--save-answers'), `${JSON.stringify(answers, null, 2)}\n`);

  const plan = await planPlugin(answers, { root, recipe });
  for (const n of plan.notes) console.log(`note: ${n}`);
  if (!plan.ok) {
    for (const r of plan.refusals) console.error(`REFUSED ${r.field}: ${r.reason}`);
    process.exit(1);
  }

  const { written, generated } = writePlugin(plan, { root, recipe });
  console.log(`\nwritten (${written.length}):`);
  for (const f of written) console.log(`  ${f}`);
  console.log(`generated (${generated.length}):${generated.length ? '' : ' none yet: the references wait for the kernel\'s release'}`);
  for (const f of generated) console.log(`  ${f}`);

  const commits = commitPlan(recipe, plan, [...written, ...generated]);
  console.log('\nCOMMIT PLAN (one concern per commit, in layer order; tools/commit-plan-check.mjs holds the range to it):');
  commits.forEach((c, i) => {
    console.log(`  ${i + 1}. [${c.layer}] ${c.message}${c.pending ? `  (PENDING: ${c.pending})` : ''}`);
    for (const f of c.files) console.log(`       ${f}`);
  });
  if (opt('--plan-json')) writeFileSync(opt('--plan-json'), `${JSON.stringify(commits, null, 2)}\n`);

  if (argv.includes('--no-run')) return;
  const env = { ...process.env, ...(plan.kernel.state === 'released' ? { OMX_CONTRACT_DIR: plan.kernel.dir } : {}) };
  const end = await finish(plan, root, recipe, env);
  console.log('');
  for (const l of end.lines) console.log(l);
  console.log('\nREMAINING CHECKLIST (what is left to do):');
  for (const g of end.gaps) console.log(`  [ ] ${g}`);
  const templated = recipe.artifacts.filter((a) => a.made === 'TEMPLATED').map((a) => `${a.id} (${fill(a.target, { stem: plan.decl.stem, kernel: plan.decl.kernel, name: plan.decl.name })})`);
  console.log(`  [ ] review the TEMPLATED artifacts, now the plugin's own: ${templated.join(', ')}`);
}

if (process.argv[1] && resolve(process.argv[1]) === fileURLToPath(import.meta.url)) await main(process.argv.slice(2));
