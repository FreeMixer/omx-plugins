#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * recipe-selftest.mjs — the plugin recipe's machinery, sabotaged arm by arm (`make selftest`).
 *
 *   1. The wizard, `--from-contract wobble` against a stand-in omx-contract release carrying the
 *      wobble kernel (at the pin) and a stand-in omx-dsp instance face: the draft takes one
 *      parameter per resolve() argument, by reference, and marks every design choice REVIEW; a
 *      settled draft generates the whole folder, the binding passing each parameter to the
 *      argument it names; only the package description (prose) is left.
 *   1b. The generators: `gen.mjs --check` is green on that tree and red, naming the file or the
 *      argument, for a hand edit to the generated binding, a declaration that moved, a face whose
 *      argument no control names, and a hand edit to a shared file; each restored, green.
 *   2. Its commit plan, applied literally in a throwaway repository, keeps the commit protocol
 *      (tools/commit-plan-check.mjs); two commits swapped, or two concerns in one commit, break it.
 *   3. Every checker family of the completeness test: break the artifact on a copy of the tree
 *      (omx-drive), and the test names that entry and its wizard step; restore, and it is green.
 *   4. The recipe: a new required entry makes every plugin red; an entry naming a checker that does
 *      not exist breaks the recipe.
 *   5. The wizard refuses, before writing: a kernel omx-contract lacks, a kernel with no instance
 *      face or one of another shape, a REVIEW mark left, a folder of another kernel, a typed travel,
 *      an unresolved reference, a reference into another kernel, a derived field written wrong, a
 *      parameter the face takes no argument for, a panel or console naming no parameter, a panel
 *      it cannot draw; and a strict build refuses an argument that binds only by its words.
 *   6. The port hints (tools/port-hints.mjs): a TTL that drops a hint, a declaration that drops a
 *      band type's labels or declares a frequency linear, and a plugin with no pin are each named;
 *      the whole tree is green.
 *
 * Works in build/selftest/ (removed first). Needs git, and omx-dsp's headers as the build does.
 * Exit 1 when any check failed, including an arm that stopped early.
 */
import { execFileSync, spawnSync } from 'node:child_process';
import { cpSync, existsSync, mkdirSync, readFileSync, rmSync, unlinkSync, writeFileSync } from 'node:fs';
import { join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { checkCommits, rangeCommits } from '../commit-plan-check.mjs';
import { commitPlan, draftDeclaration, kernelSources, planPlugin, reviewMarks, writePlugin } from '../omx-new-plugin.mjs';
import { bindFace, parseFace } from '../instance-face.mjs';
import { locateContract } from '../omx-contract.mjs';
import { omxdspInclude } from '../template.mjs';
import { emitPluginTtl, loadDecl } from '../gen.mjs';
import { hintErrors } from '../port-hints.mjs';
import { checkPlugin, debtGrowth, debtVerdict, gapLines, loadDebt, loadRecipe, pluginStems, recipeErrors } from '../plugin-recipe.mjs';

const ROOT = resolve(fileURLToPath(import.meta.url), '..', '..', '..');
const WORK = join(ROOT, 'build', 'selftest');
let fails = 0;
const pass = (what) => console.log(`PASS ${what}`);
const fail = (what) => {
  console.log(`FAIL ${what}`);
  fails++;
};
const expect = (cond, what) => (cond ? pass(what) : fail(what));

/** A copy of this tree's files (tracked and new, never ignored) at `dst`. */
function copyTree(dst) {
  mkdirSync(dst, { recursive: true });
  // listed first, so a git that cannot read the tree fails here rather than copying nothing
  const files = execFileSync('git', ['ls-files', '-co', '--exclude-standard', '-z'], { cwd: ROOT });
  if (!files.length) throw new Error('git lists no file in this tree');
  execFileSync('sh', ['-c', 'xargs -0 tar -cf - | tar -xf - -C "$1"', 'sh', dst], { cwd: ROOT, input: files });
}

/** A stand-in omx-contract release: the pinned release's resolved render, with the items of each
 * kernel given in place of that kernel's own (the rest of the tree still resolves against it). */
function fakeContract(dst, kernels) {
  const real = locateContract(ROOT);
  if (!real.dir) throw new Error(`the stand-in starts from the pinned omx-contract: ${real.why}`);
  mkdirSync(join(dst, 'share', 'omx-contract'), { recursive: true });
  cpSync(join(real.dir, 'lib'), join(dst, 'lib'), { recursive: true });
  cpSync(join(real.dir, 'package.json'), join(dst, 'package.json'));
  const render = JSON.parse(readFileSync(join(real.dir, 'share', 'omx-contract', 'omx-contract.json'), 'utf8'));
  for (const [k, v] of Object.entries(kernels)) {
    for (const [name, e] of Object.entries(render.items)) if (e.rel === `data/kernels/${k}.json`) delete render.items[name];
    for (const [name, e] of Object.entries(v)) render.items[name] = { rel: `data/kernels/${k}.json`, ...e };
  }
  writeFileSync(join(dst, 'share', 'omx-contract', 'omx-contract.json'), JSON.stringify(render, null, 2));
}

const travel = (min, max, step, unit, def) => ({ kind: 'travels', shape: 'travel', value: { min, max, step, unit, default: def, defaultFrom: 'desk' } });
const WOBBLE = {
  WOBBLE_RATE_RANGE: travel(0.1, 20, 0.01, 'Hz', 4),
  WOBBLE_DEPTH_RANGE: travel(0, 100, 0.1, '%', 50),
  WOBBLE_MIX_RANGE: travel(0, 100, 0.1, '%', 100),
  WOBBLE_MODES: { kind: 'set', value: ['wobble', 'pan'], default: 'wobble' },
  // the aggregate: its field names are the controls' names (rateHz, not rate)
  WOBBLE_TRAVELS: { kind: 'travels', shape: 'table', value: { rateHz: travel(0.1, 20, 0.01, 'Hz', 4).value, depth: travel(0, 100, 0.1, '%', 50).value, mix: travel(0, 100, 0.1, '%', 100).value } },
};

/** A stand-in omx-dsp: the real headers, plus a wobble instance face of the generated shape. */
function fakeOmxdsp(dst, args = 'float rate_hz, float depth, float mix, int mode') {
  const real = omxdspInclude();
  if (!real) throw new Error('no omx-dsp headers (pkg-config omxdsp, or OMXDSP_INCLUDE)');
  if (!existsSync(dst)) cpSync(real, dst, { recursive: true });
  const fx = join(dst, existsSync(join(dst, 'omxdsp')) ? 'omxdsp' : '', 'fx', 'omx_wobble_instance.h');
  writeFileSync(fx, `#ifndef OMX_WOBBLE_INSTANCE_H
#define OMX_WOBBLE_INSTANCE_H
#include <stdint.h>
typedef struct { float sr; } OmxWobbleInstance;
#define OMX_WOBBLE_INSTANCE_LATENCY_FRAMES 0.0f
static inline int omx_wobble_instance_init(OmxWobbleInstance *s, float sr) { s->sr = sr; return 1; }
static inline void omx_wobble_instance_resolve(OmxWobbleInstance *s, int bypass, ${args}) { (void)s; (void)bypass; }
static inline void omx_wobble_instance_run(OmxWobbleInstance *s, const float *in_l, const float *in_r, float *out_l,
                                            float *out_r, uint32_t n) { (void)s; (void)in_l; (void)in_r; (void)out_l; (void)out_r; (void)n; }
#endif
`);
  return fx;
}

const git = (repo, ...args) => execFileSync('git', ['-C', repo, '-c', 'user.name=selftest', '-c', 'user.email=selftest@invalid', '-c', 'commit.gpgsign=false', ...args], { encoding: 'utf8' });

/** A throwaway repository holding `base`, then each commit of `commits` copied from `from`. */
function applyPlan(repo, base, from, commits) {
  copyTree(repo);
  git(repo, 'init', '-q');
  git(repo, 'add', '-A');
  git(repo, 'commit', '-q', '-m', 'base');
  for (const c of commits) {
    for (const f of c.files) {
      mkdirSync(join(repo, f, '..'), { recursive: true });
      cpSync(join(from, f), join(repo, f));
    }
    git(repo, 'add', ...c.files);
    git(repo, 'commit', '-q', '-m', c.message);
  }
  return checkCommits(loadRecipe(ROOT), rangeCommits(repo, 'HEAD~' + commits.length + '..HEAD'));
}

async function main() {
  rmSync(WORK, { recursive: true, force: true });
  mkdirSync(WORK, { recursive: true });
  const recipe = loadRecipe(ROOT);
  const realInc = omxdspInclude();
  const contract = join(WORK, 'omx-contract');
  fakeContract(contract, { wobble: WOBBLE });
  const inc = join(WORK, 'omxdsp-include');
  const face = fakeOmxdsp(inc);
  process.env.OMX_CONTRACT_DIR = contract;
  process.env.OMXDSP_INCLUDE = inc;

  // ---- 1. the wizard -------------------------------------------------------------------------
  const tree = join(WORK, 'wizard');
  copyTree(tree);
  const src = kernelSources('wobble', { root: tree });
  expect(!src.refusals.length, `the wizard finds the wobble kernel and its face${src.refusals.length ? `: ${src.refusals.map((r) => r.reason).join('; ')}` : ''}`);
  if (src.refusals.length) return;
  const draft = draftDeclaration('wobble', src, { root: tree, recipe });
  expect(draft.decl?.params.map((p) => `${p.symbol}:${p.name}`).join(' ') === 'rateHz:Rate depth:Depth mix:Mix mode:Mode',
    `the draft has one parameter per resolve() argument, named by the contract (${draft.decl?.params.map((p) => `${p.symbol}:${p.name}`).join(' ')})`);
  expect(draft.decl?.params.every((p) => p.ref && !('min' in p) && !('def' in p)), 'the draft is by reference only');
  const marks = reviewMarks(draft.decl);
  expect(marks.join(' ') === '/description /clap/features/1 /lv2/class /params/3/values/0 /params/3/values/1',
    `the draft marks every design choice REVIEW (${marks.join(' ')})`);
  const unsettled = await planPlugin(draft.decl, src, { root: tree, recipe });
  expect(!unsettled.ok && unsettled.refusals.length === marks.length, `a draft with REVIEW marks is refused, each named (${unsettled.refusals.map((r) => r.field).join(' ')})`);
  const settled = structuredClone(draft.decl);
  settled.description = 'The console\'s wobble: the level, or the balance, moved by one LFO.';
  settled.clap.features[1] = 'wobble';
  settled.lv2.class = 'lv2:ModulatorPlugin';
  settled.params[3].values = ['Wobble', 'Pan'];
  const answers = settled; // what section 5 mutates
  const plan = await planPlugin(settled, src, { root: tree, recipe });
  expect(plan.ok, `the settled draft is accepted${plan.ok ? '' : `: ${plan.refusals.map((r) => `${r.field} ${r.reason}`).join('; ')}`}`);
  if (!plan.ok) return;
  const written = writePlugin(plan, { root: tree });
  const P_ = 'plugins/omx-wobble';
  for (const f of ['omx-wobble.decl.json', 'Makefile', 'omx_wobble_clap.c', 'omx_wobble_lv2.c', 'omx_wobble_core.h', 'test/wobble-oracle.c', 'generated/omx_wobble_params.h'])
    expect(written.includes(`${P_}/${f}`), `the wizard wrote ${P_}/${f}`);
  for (const f of ['README.md', 'packaging/omx-plugins.spec', '.github/workflows/ci.yml']) expect(written.includes(f), `the shared file ${f} lists the new plugin`);
  const header = readFileSync(join(tree, P_, 'generated/omx_wobble_params.h'), 'utf8');
  expect(header.includes('{ "rateHz", "Rate", "Hz", 0.1f, 20.0f, 4.0f, 0u }'), 'the generated table carries the kernel file\'s rate travel');
  const core = readFileSync(join(tree, P_, 'omx_wobble_core.h'), 'utf8');
  expect(core.includes('/* rate_hz */ values[OMX_WOBBLE_PARAM_RATE_HZ]') && core.includes('/* mode */ (int)lrintf(values[OMX_WOBBLE_PARAM_MODE])') && !/OMX_WIZARD_STUB/.test(core),
    'the generated binding passes each parameter to the resolve() argument it names, a choice as an int');
  // The scratch tree's package prose may already name wobble (the real catalogue does); take it out so the arm
  // proves a new plugin's missing description is caught.
  for (const f of ['debian/control', 'packaging/omx-plugins.spec']) {
    const p = join(tree, f);
    writeFileSync(p, readFileSync(p, 'utf8').replace(/(?<![-_])\bwobble\b,?[ \t]?/gi, ''));
  }
  const owed = gapLines(await checkPlugin(tree, recipe, 'omx-wobble'));
  expect(owed.length === 1 && owed[0].startsWith('omx-wobble: package-description missing'), `a generated plugin owes only the package description, prose a person writes (${owed.map((g) => g.split(' (')[0]).join('; ')})`);

  // ---- 1b. the generators, sabotaged input by input --------------------------------------------
  const check = () => spawnSync('node', [join(tree, 'tools', 'gen.mjs'), '--check'], { cwd: tree, encoding: 'utf8' });
  const green = check();
  expect(green.status === 0, `gen.mjs --check is green on the wizard's tree${green.status ? `: ${green.stderr.trim().split('\n')[0]}` : ''}`);
  const sabotageGen = (what, file, mutate, needle) => {
    const at = join(file.startsWith('/') ? '' : tree, file);
    const was = readFileSync(at, 'utf8');
    writeFileSync(at, mutate(was));
    const red = check();
    writeFileSync(at, was);
    const back = check();
    const out = `${red.stdout}${red.stderr}`;
    expect(red.status !== 0 && out.includes(needle) && back.status === 0, `sabotage gen: ${what} goes red naming ${needle}, green restored${red.status ? '' : ' (STAYED GREEN)'}`);
  };
  sabotageGen('a hand edit to the generated binding', `${P_}/omx_wobble_core.h`, (t) => t.replace('/* depth */', '/* depth (tuned) */'), `STALE ${P_}/omx_wobble_core.h`);
  sabotageGen('a hand edit to the generated identity test', `${P_}/test/wobble-oracle.c`, (t) => t.replace('no tolerance', 'a little tolerance'), `STALE ${P_}/test/wobble-oracle.c`);
  sabotageGen('a declaration that moved a parameter\'s name', `${P_}/omx-wobble.decl.json`, (t) => t.replace('"name": "Depth"', '"name": "Depth Amount"'), `STALE ${P_}/generated/omx_wobble_params.h`);
  sabotageGen('a face whose argument no contract control names', face, (t) => t.replace('float depth,', 'float intensity,'), "resolve argument 'intensity'");
  sabotageGen('a hand edit to a shared file', 'README.md', (t) => t.replace('| **omx wobble** |', '| **omx wobble (beta)** |'), 'STALE README.md');
  {
    const was = readFileSync(join(tree, P_, 'omx_wobble_core.h'), 'utf8');
    writeFileSync(join(tree, P_, 'omx_wobble_core.h'), `${was}\n`);
    const r = gapLines(await checkPlugin(tree, recipe, 'omx-wobble'));
    writeFileSync(join(tree, P_, 'omx_wobble_core.h'), was);
    expect(r.some((l) => l.startsWith('omx-wobble: instance-files missing')), `sabotage completeness: a stale generated binding is the gap instance-files (${r.length} gaps)`);
  }

  // ---- 2. the commit plan, applied literally ------------------------------------------------
  const commits = commitPlan(recipe, plan, written);
  const order = recipe.layers.map((l) => l.id);
  expect(commits.every((c, i) => i === 0 || order.indexOf(c.layer) > order.indexOf(commits[i - 1].layer)), `the plan is in layer order (${commits.map((c) => c.layer).join(' → ')})`);
  const good = applyPlan(join(WORK, 'plan-ok'), ROOT, tree, commits);
  expect(good.ok, `the plan applied literally keeps the commit protocol${good.ok ? '' : `: ${good.violations.join('; ')}`}`);
  const iContract = commits.findIndex((c) => c.layer === 'contract');
  const iCode = commits.findIndex((c) => c.layer === 'code');
  const swapped = [...commits];
  [swapped[iContract], swapped[iCode]] = [swapped[iCode], swapped[iContract]];
  const bad = applyPlan(join(WORK, 'plan-swapped'), ROOT, tree, swapped);
  expect(!bad.ok && bad.violations.some((v) => /layer contract after code/.test(v)), `sabotage: faces before the declaration is refused (${bad.violations[0] ?? 'no violation'})`);
  const merged = [{ ...commits[iContract], files: [...commits[iContract].files, ...commits[iCode].files] }, ...commits.filter((_c, i) => i !== iContract && i !== iCode)];
  const two = applyPlan(join(WORK, 'plan-merged'), ROOT, tree, merged);
  expect(!two.ok && two.violations.some((v) => /2 concerns in one commit/.test(v)), 'sabotage: declaration and faces in one commit is refused');

  const none = checkCommits(recipe, rangeCommits(join(WORK, 'plan-ok'), 'HEAD..HEAD'));
  expect(!none.ok && none.violations.some((v) => /holds no commit/.test(v)), 'sabotage: an empty commit range fails the protocol check');

  // ---- 3. every checker family, broken on a copy of omx-drive --------------------------------
  delete process.env.OMX_CONTRACT_DIR;
  delete process.env.OMXDSP_INCLUDE;
  const t = join(WORK, 'sabotage');
  copyTree(t);
  const P = 'plugins/omx-drive';
  const base = await checkPlugin(t, recipe, 'omx-drive');
  expect(gapLines(base).length === 0, `baseline: omx-drive is complete${gapLines(base).length ? `: ${gapLines(base).join('; ')}` : ''}`);
  const edit = (f, fn) => {
    const at = join(t, f);
    const was = readFileSync(at, 'utf8');
    writeFileSync(at, fn(was));
    return () => writeFileSync(at, was);
  };
  const add = (f, text) => {
    writeFileSync(join(t, f), text);
    return () => unlinkSync(join(t, f));
  };
  const remove = (f) => {
    const was = readFileSync(join(t, f));
    unlinkSync(join(t, f));
    return () => writeFileSync(join(t, f), was);
  };
  const dspName = (() => {
    const inc = realInc;
    const h = readFileSync(join(inc, 'omxdsp', 'omx_lfo.h'), 'utf8');
    return h.match(/static inline \w+ (omx_lfo_[a-z_]+)\s*\(/)[1];
  })();
  const SABOTAGE = [
    ['declValid', 'declaration', () => edit(`${P}/omx-drive.decl.json`, (s) => s.replace('"org.openmixer.drive"', '"org.openmixer.delay"'))],
    ['generatedFresh', 'params-header', () => edit(`${P}/generated/omx_drive_params.h`, (s) => `${s}\n`)],
    ['generatedFresh', 'lv2-description', () => edit(`${P}/generated/omx-drive.lv2/omx-drive.ttl`, (s) => s.replace('lv2:default 0 ;', 'lv2:default 1 ;'))],
    ['modguiFresh', 'modgui', () => add(`${P}/generated/omx-drive.lv2/modgui/stray.css`, '')],
    ['makefile', 'makefile', () => edit(`${P}/Makefile`, (s) => s.replace('urn:openmixer:drive', 'urn:openmixer:drives'))],
    ['face', 'clap-face', () => edit(`${P}/omx_drive_clap.c`, (s) => s.replace('#include "omx_drive_params.h"', ''))],
    ['face', 'lv2-face', () => edit(`${P}/omx_drive_lv2.c`, (s) => s.replace('lv2_descriptor(uint32_t', 'lv2_descriptor_gone(uint32_t'))],
    ['kernelBinding', 'kernel-binding', () => edit(`${P}/omx_drive_lv2.c`, (s) => `#define OMX_WIZARD_STUB 1\n${s}`)],
    ['kernelBinding', 'kernel-binding', () => {
      // only omx_denormal.h left: an omx-dsp header, but not the plugin's own kernel
      const undo = ['omx_drive_clap.c', 'omx_drive_lv2.c'].map((f) => edit(`${P}/${f}`, (s) => s.replace(/#include <omxdsp\/fx\/omx_drive_instance\.h>\n/, '')));
      return () => undo.forEach((u) => u());
    }],
    ['kernelBinding', 'kernel-binding', () => edit(`${P}/omx-drive.decl.json`, (s) => s.replace('"kernel": "drive",', '"kernel": "drive",\n  "kernels": ["drive", "delay"],'))],
    ['makeTestRuns', 'parameter-test', () => edit(`${P}/Makefile`, (s) => s.replace(/^.*clap-params-check\.mjs.*\n/m, ''))],
    ['oracle', 'kernel-identity-test', () => remove(`${P}/test/drive-oracle.c`)],
    ['sharedListed', 'spec-files', () => edit('packaging/omx-plugins.spec', (s) => s.replace('%{_libdir}/lv2/omx-drive.lv2/\n', ''))],
    ['installCovers', 'deb-install', () => edit('debian/omx-plugins-clap.install', () => 'usr/lib/clap/omx-delay.clap\n')],
    ['namedIn', 'package-description', () => edit('debian/control', (s) => s.replace(/delay, drive,/g, 'delay,'))],
    ['sharedListed', 'ci-installed-files', () => edit('.github/workflows/ci.yml', (s) => s.replaceAll(' drive ', ' '))],
    ['sharedListed', 'catalogue-row', () => edit('README.md', (s) => s.replace('`org.openmixer.drive`', '`org.openmixer.x`'))],
    ['sharedListed', 'manual-section', () => edit('README.md', (s) => s.replace('### omx drive\n', '### drive\n'))],
    ['noFiles', 'no-cpp', () => add(`${P}/shell.cpp`, '// SPDX-License-Identifier: GPL-3.0-or-later\n')],
    ['noText', 'no-dpf', () => add(`${P}/dpf_shell.h`, '#include "DistrhoPlugin.hpp"\n')],
    ['noCopiedDsp', 'no-copied-dsp', () => add(`${P}/copied.h`, `static inline float ${dspName}(float x) {\n  return x;\n}\n`)],
    ['noCopiedDsp', 'no-copied-dsp', () => add(`${P}/engine.h`, '#include "mix_drive.h"\n')],
    ['paramsByReference', 'params-by-reference', () => edit(`${P}/omx-drive.decl.json`, (s) => s.replace('"ref": "DRIVE_AMOUNT_RANGE"', '"ref": "DRIVE_AMOUNT_RANGE", "min": 0'))],
  ];
  for (const [family, id, breakIt] of SABOTAGE) {
    const restore = breakIt();
    const r = await checkPlugin(t, recipe, 'omx-drive');
    const entry = r.results.find((x) => x.id === id);
    const line = gapLines(r).find((l) => l.startsWith(`omx-drive: ${entry?.kind === 'law' ? 'law ' : ''}${id} missing (wizard step '${entry?.step}'`));
    restore();
    const back = gapLines(await checkPlugin(t, recipe, 'omx-drive'));
    expect(line && back.length === 0, `sabotage ${family}: ${id} goes red naming step '${entry?.step}', green restored${line ? ` — ${line.slice(0, 140)}` : ''}`);
  }

  // ---- 3b. the debt ratchet, both directions ---------------------------------------------------
  const ratchet = async () => {
    const reports = [];
    for (const st of pluginStems(t)) reports.push(await checkPlugin(t, recipe, st));
    return debtVerdict(reports, loadDebt(t));
  };
  const whole = await ratchet();
  expect(!whole.fresh.length && !whole.stale.length && whole.held.length === loadDebt(t).length,
    `the debt holds today's gaps exactly (${whole.held.length} held, ${whole.fresh.length} new, ${whole.stale.length} stale)`);
  let undo = edit('README.md', (s) => s.replace('### omx drive\n', '### drive\n'));
  const unheld = await ratchet();
  undo();
  expect(unheld.fresh.includes("omx-drive manual-section"), `sabotage: a gap the debt does not hold fails the ratchet (${unheld.fresh.join(", ")})`);
  undo = add('plugins/omx-chorus/Makefile', readFileSync(join(t, 'plugins/omx-drive/Makefile'), 'utf8').replace(/omx-drive/g, 'omx-chorus').replace(/urn:openmixer:drive/g, 'urn:openmixer:chorus').replace(/org\.openmixer\.drive/g, 'org.openmixer.chorus'));
  const paid = await ratchet();
  undo();
  expect(paid.stale.some((x) => x.startsWith('omx-chorus makefile')), `sabotage: a debt entry now satisfied fails the ratchet as stale (${paid.stale.join(', ')})`);

  const ghost = await (async () => {
    const reports = [];
    for (const st of pluginStems(t)) reports.push(await checkPlugin(t, recipe, st));
    return debtVerdict(reports, [...loadDebt(t), { plugin: 'omx-ghost', entry: 'makefile', owedBy: 'nobody' }], pluginStems(t));
  })();
  expect((ghost.unknown ?? []).length === 1 && ghost.unknown[0].startsWith('omx-ghost makefile'), `sabotage: a debt entry for a plugin that does not exist fails the ratchet (${(ghost.unknown ?? []).join(', ')})`);
  {
    // an empty plugin set: the run itself fails, rather than passing over nothing
    const empty = join(WORK, 'empty');
    mkdirSync(join(empty, 'plugins'), { recursive: true });
    cpSync(join(ROOT, 'recipes'), join(empty, 'recipes'), { recursive: true });
    cpSync(join(ROOT, 'schema'), join(empty, 'schema'), { recursive: true });
    mkdirSync(join(empty, '.github'), { recursive: true });
    cpSync(join(ROOT, '.github', 'pins.txt'), join(empty, '.github', 'pins.txt'));
    const run = spawnSync('node', [join(ROOT, 'tools', 'plugin-recipe.mjs')], { env: { ...process.env, OMX_PLUGINS_ROOT: empty }, encoding: 'utf8' });
    expect(run.status === 1 && /no plugin to check/.test(run.stdout + run.stderr), `sabotage: a run over no plugin fails (exit ${run.status})`);
  }

  {
    // the debt against its merge-base: a throwaway repository whose origin/main holds today's debt
    const repo = join(WORK, 'growth');
    copyTree(repo);
    git(repo, 'init', '-q');
    git(repo, 'add', '-A');
    git(repo, 'commit', '-q', '-m', 'main');
    git(repo, 'update-ref', 'refs/remotes/origin/main', 'HEAD');
    git(repo, 'checkout', '-q', '-b', 'topic');
    const level = debtGrowth(repo, loadDebt(repo));
    expect(level.base === loadDebt(repo).length && !level.grown, `the debt as main holds it has not grown (${level.base} entries at the merge-base)`);
    const more = [...loadDebt(repo), { plugin: 'omx-drive', entry: 'manual-section', owedBy: 'sabotage' }];
    const grew = debtGrowth(repo, more);
    expect(grew.grown && grew.added.join() === 'omx-drive manual-section', `sabotage: a debt entry added since main fails the growth check (${grew.added.join()})`);
    const fewer = debtGrowth(repo, loadDebt(repo).slice(1));
    expect(!fewer.grown, 'a debt that shrank passes the growth check');
    git(repo, 'update-ref', '-d', 'refs/remotes/origin/main');
    const none = debtGrowth(repo, loadDebt(repo));
    expect(none.base === undefined && /no merge-base/.test(none.why), `no main to compare to is reported, never a silent pass (${none.why?.slice(0, 60)})`);
  }

  // ---- 4. the recipe itself ----------------------------------------------------------------
  const grown = JSON.parse(JSON.stringify(recipe));
  grown.artifacts.push({ id: 'sabotage-notes', what: 'a section every plugin now owes', step: 'docs', layer: 'docs', made: 'HAND-WRITTEN', target: 'README.md', paths: [], questions: [], template: null, when: 'always', checker: { fn: 'sharedListed', file: 'README.md', region: 'sections', needle: '### {name} notes' } });
  let allRed = true;
  for (const s of ['omx-delay', 'omx-drive', 'omx-eq8', 'omx-strip']) {
    if (!gapLines(await checkPlugin(t, grown, s)).some((l) => l.includes('sabotage-notes missing'))) allRed = false;
  }
  expect(allRed, 'sabotage: one new required entry in the recipe makes every plugin red');
  const broken = JSON.parse(JSON.stringify(recipe));
  broken.artifacts[0].checker.fn = 'noSuchChecker';
  expect(recipeErrors(broken).some((e) => e.includes('noSuchChecker')), 'sabotage: an entry naming a missing checker breaks the recipe');
  const unruled = JSON.parse(JSON.stringify(recipe));
  unruled.laws[0].when = 'someday';
  expect(recipeErrors(unruled).some((e) => e.includes("'someday'")), 'sabotage: an entry naming an undeclared rule breaks the recipe');

  // ---- 5. the wizard refuses before writing ------------------------------------------------------
  process.env.OMX_CONTRACT_DIR = contract;
  process.env.OMXDSP_INCLUDE = inc;
  const fresh = join(WORK, 'refusals');
  copyTree(fresh);
  const srcFresh = kernelSources('wobble', { root: fresh });
  const refused = async (what, mutate, field) => {
    const a = structuredClone(answers);
    mutate(a);
    const p = await planPlugin(a, srcFresh, { root: fresh, recipe });
    const r = p.refusals.find((x) => x.field.startsWith(field));
    expect(!p.ok && r, `wizard refuses ${what}: ${r ? `${r.field} ${r.reason}` : 'NOT REFUSED'}`);
  };
  const sourceRefused = (what, kernel, field, needle) => {
    const r = kernelSources(kernel, { root: fresh }).refusals.find((x) => x.field === field);
    expect(r && r.reason.includes(needle), `wizard refuses ${what}: ${r ? r.reason.slice(0, 120) : 'NOT REFUSED'}`);
  };
  sourceRefused('a kernel omx-contract lacks', 'nosuch', 'omx-contract', 'has no data/kernels/nosuch.json');
  sourceRefused('a kernel with no instance face', 'delay', 'omx-dsp', 'has no <omxdsp/fx/omx_delay_instance.h>');
  {
    const chorus = join(realInc, existsSync(join(realInc, 'omxdsp')) ? 'omxdsp' : '', 'fx', 'omx_chorus_instance.h');
    const f = existsSync(chorus) ? parseFace(readFileSync(chorus, 'utf8'), 'chorus') : { error: 'no chorus face to read' };
    expect(/hands in/.test(f.error ?? ''), `a face of another shape is named, not guessed (chorus: ${f.error ?? 'ACCEPTED'})`);
  }
  await refused('a REVIEW mark left', (a) => { a.lv2.class = 'REVIEW: the LV2 class'; }, '/lv2/class');
  await refused('a plugin not generated from the face', (a) => { delete a.binding; }, '/binding');
  await refused('a folder that is another kernel\'s', (a) => { a.stem = 'omx-drive'; delete a.clap.id; delete a.lv2.uri; delete a.name; }, '/stem');
  await refused('another plugin\'s LV2 URI', (a) => { a.stem = 'omx-trem'; a.lv2.uri = 'urn:openmixer:drive'; delete a.clap.id; delete a.name; }, '/lv2/uri');
  await refused('a typed travel', (a) => { a.params[0].min = 0.1; }, '/params/0');
  await refused('an unresolved reference', (a) => { a.params[1].ref = 'WOBBLE_SPEED_RANGE'; }, '/params/1/ref');
  await refused('a reference into another kernel', (a) => { a.params[0].ref = 'FX_DELAY_TIME_RANGE'; }, '/params/0/ref');
  await refused('a derived field written wrong', (a) => { a.clap.id = 'org.openmixer.trem'; }, '/clap/id');
  await refused('a face argument no parameter binds', (a) => { a.params.splice(2, 1); }, '/params');
  await refused('a panel naming no parameter', (a) => { a.panel = { family: 'modulation', roles: {}, sections: [{ key: 'wobble', label: 'Wobble', controls: ['rateHz', 'speed'] }] }; }, '/panel/sections');
  await refused('a console chip naming no parameter', (a) => { a.console = { placement: { strips: ['input'], group: 'insert' }, chip: '{rateHz} · {speed}' }; }, '/console/chip');
  await refused('a panel that leaves a parameter off the MOD GUI', (a) => {
    a.panel = { family: 'modulation', roles: {}, sections: [{ key: 'wobble', label: 'Wobble', controls: ['rateHz', 'depth', 'mix'] }] };
  }, '/panel');
  {
    const f = parseFace(`static inline int omx_wobble_instance_init(OmxWobbleInstance *s, float sr) { return 1; }
static inline void omx_wobble_instance_resolve(OmxWobbleInstance *s, int bypass, float rate, float depth) { }
static inline void omx_wobble_instance_run(OmxWobbleInstance *s, const float *in_l, const float *in_r, float *out_l, float *out_r, uint32_t n) { }
#define OMX_WOBBLE_INSTANCE_LATENCY_FRAMES 0.0f`, 'wobble');
    const ps = [{ symbol: 'rateHz' }, { symbol: 'depth' }];
    const loose = bindFace(f, ps, [{ name: 'rateHz' }, { name: 'depth' }], { strict: false });
    const strict = bindFace(f, ps, [{ name: 'rateHz' }, { name: 'depth' }], { strict: true });
    expect(!loose.errors.length && loose.renames[0]?.rename === 'rate_hz' && strict.errors.some((e) => e.includes("'rate'")),
      `an argument that binds only by its words is a rename owed, refused when strict (${loose.renames.map((r) => `${r.arg}->${r.rename}`).join(', ')}; ${strict.errors[0] ?? 'NOT REFUSED'})`);
  }
  {
    const face = (arg) => parseFace(`static inline int omx_wobble_instance_init(OmxWobbleInstance *s, float sr) { return 1; }
static inline void omx_wobble_instance_resolve(OmxWobbleInstance *s, int bypass, ${arg}, float mix) { }
static inline void omx_wobble_instance_run(OmxWobbleInstance *s, const float *in_l, const float *in_r, float *out_l, float *out_r, uint32_t n) { }
#define OMX_WOBBLE_INSTANCE_LATENCY_FRAMES 0.0f`, 'wobble');
    const ps = [{ symbol: 'band1' }, { symbol: 'mix' }, { symbol: 'band2' }, { symbol: 'band3' }];
    const cs = [{ name: 'band' }, { name: 'mix' }, { name: 'band' }, { name: 'band' }];
    const b = bindFace(face('const float band[OMX_WOBBLE_BANDS]'), ps, cs);
    const arr = b.binding.find((x) => x.arg === 'band');
    expect(!b.errors.length && arr?.extent === 'OMX_WOBBLE_BANDS' && arr.params.map((p) => p.symbol).join() === 'band1,band2,band3',
      `a per-band array argument binds every parameter of its control, in declaration order (${arr?.params?.map((p) => p.symbol).join() ?? b.errors[0]})`);
    const literal = face('const float band[3]');
    expect(/not one scalar/.test(literal.error ?? ''), `a per-band array of literal extent is named, not bound (${literal.error ?? 'ACCEPTED'})`);
  }

  // ---- 6. the port hints ---------------------------------------------------------------------
  delete process.env.OMX_CONTRACT_DIR; // the real tree reads the release it pins
  delete process.env.OMXDSP_INCLUDE;
  {
    const hints = join(WORK, 'hints');
    copyTree(hints);
    const named = (what, needle) => {
      const e = hintErrors(hints);
      expect(e.some((x) => x.includes(needle)), `port hints: ${what} is named${e.length ? ` (${e[0]})` : ' (NOTHING NAMED)'}`);
    };
    expect(hintErrors(hints).length === 0, 'port hints: the whole tree keeps every pinned hint');
    const eq16 = join(hints, 'plugins/omx-eq16/generated/omx-eq16.lv2/omx-eq16.ttl');
    const ttl16 = readFileSync(eq16, 'utf8');
    writeFileSync(eq16, ttl16.replace('pprops:logarithmic ;', '').replace('units:unit units:db', 'units:unit units:pc'));
    named('a TTL that drops a frequency\'s logarithmic travel', 'omx-eq16: port "hpf_freq" lost pprops:logarithmic');
    named('a TTL that drops a gain\'s unit', 'omx-eq16: port "b1_gain" lost units:unit units:db');
    writeFileSync(eq16, ttl16);
    const dir8 = join(hints, 'plugins/omx-eq8'), decl8 = join(dir8, 'omx-eq8.decl.json'), ttl8 = join(dir8, 'generated/omx-eq8.lv2/omx-eq8.ttl');
    const d8 = readFileSync(decl8, 'utf8');
    const regen = (mutate) => {
      const d = JSON.parse(d8);
      mutate(d.params);
      writeFileSync(decl8, JSON.stringify(d, null, 2));
      writeFileSync(ttl8, emitPluginTtl(loadDecl(dir8)));
    };
    regen((ps) => ps.splice(ps.findIndex((p) => p.symbol === 'b1_type'), 1, { symbol: 'b1_type', name: 'Band 1 Type', own: 'switch', unit: '', min: 0, max: 5, def: 0, kind: 'integer' }));
    named('a declaration that drops a band type\'s labels', 'omx-eq8: port "b1_type" lost the scale point 0 Bell');
    regen((ps) => (ps.find((p) => p.symbol === 'hpf_freq').scale = 'linear'));
    named('a frequency declared linear', 'omx-eq8: port "hpf_freq" lost pprops:logarithmic');
    regen(() => {});
    expect(hintErrors(hints).length === 0, 'port hints: restored, green again');
    cpSync(join(hints, 'plugins/omx-eq8'), join(hints, 'plugins/omx-eq9'), { recursive: true });
    for (const [from, to] of [['omx-eq8.decl.json', 'omx-eq9.decl.json'], ['generated/omx-eq8.lv2', 'generated/omx-eq9.lv2'], ['generated/omx-eq9.lv2/omx-eq8.ttl', 'generated/omx-eq9.lv2/omx-eq9.ttl']])
      execFileSync('mv', [join(hints, 'plugins/omx-eq9', from), join(hints, 'plugins/omx-eq9', to)]);
    const d9 = JSON.parse(readFileSync(join(hints, 'plugins/omx-eq9/omx-eq9.decl.json'), 'utf8'));
    d9.stem = 'omx-eq9';
    writeFileSync(join(hints, 'plugins/omx-eq9/omx-eq9.decl.json'), JSON.stringify(d9, null, 2));
    named('a plugin with no pin', 'omx-eq9: no pinned hints');
  }

}

// an arm that stops early has failed already: the verdict and the exit come after main, always
await main();
console.log(fails ? `recipe-selftest: ${fails} check(s) failed` : 'recipe-selftest: every arm red when broken, green when whole');
process.exit(fails ? 1 : 0);
