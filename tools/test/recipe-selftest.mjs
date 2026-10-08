#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * recipe-selftest.mjs — the plugin recipe's machinery, sabotaged arm by arm (`make selftest`).
 *
 *   1. The wizard, from recipes/examples/omx-tremolo.answers.json against a stand-in omx-contract
 *      release that carries the tremolo kernel: it writes every artifact, its declaration is by
 *      reference only, and the generated table carries the kernel file's numbers.
 *   2. Its commit plan, applied literally in a throwaway repository, keeps the commit protocol
 *      (tools/commit-plan-check.mjs); two commits swapped, or two concerns in one commit, break it.
 *   3. Every checker family of the completeness test: break the artifact on a copy of the tree
 *      (omx-drive), and the test names that entry and its wizard step; restore, and it is green.
 *   4. The recipe: a new required entry makes every plugin red; an entry naming a checker that does
 *      not exist breaks the recipe.
 *   5. The wizard refuses, before writing, a taken stem, a typed travel, an unresolved reference, a
 *      reference into another kernel, a derived field answered wrong and a panel it cannot draw.
 *   6. The port hints (tools/port-hints.mjs): a TTL that drops a hint, a declaration that drops a
 *      band type's labels or declares a frequency linear, and a plugin with no pin are each named;
 *      the whole tree is green.
 *
 * Works in build/selftest/ (removed first). Needs git, and omx-dsp's headers as the build does.
 */
import { execFileSync, spawnSync } from 'node:child_process';
import { cpSync, existsSync, mkdirSync, readFileSync, rmSync, unlinkSync, writeFileSync } from 'node:fs';
import { join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { checkCommits, rangeCommits } from '../commit-plan-check.mjs';
import { commitPlan, planPlugin, writePlugin } from '../omx-new-plugin.mjs';
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

/** A stand-in omx-contract release: the version, and the items of each kernel given, as the resolved
 * render (share/omx-contract/omx-contract.json) carries them. */
function fakeContract(dst, version, kernels) {
  mkdirSync(join(dst, 'share', 'omx-contract'), { recursive: true });
  mkdirSync(join(dst, 'lib'), { recursive: true });
  writeFileSync(join(dst, 'package.json'), JSON.stringify({ name: '@openmixer/omx-contract', version }));
  writeFileSync(join(dst, 'lib', 'eq-defaults.mjs'), '// the stand-in has no EQ\n');
  const items = {};
  for (const [k, v] of Object.entries(kernels)) for (const [name, e] of Object.entries(v)) items[name] = { rel: `data/kernels/${k}.json`, ...e };
  writeFileSync(join(dst, 'share', 'omx-contract', 'omx-contract.json'), JSON.stringify({ name: '@openmixer/omx-contract', version, items }, null, 2));
}

const travel = (min, max, step, unit, def) => ({ kind: 'travels', shape: 'travel', value: { min, max, step, unit, default: def, defaultFrom: 'desk' } });
const TREMOLO = {
  TREMOLO_RATE_RANGE: travel(0.1, 20, 0.01, 'Hz', 4),
  TREMOLO_DEPTH_RANGE: travel(0, 100, 0.1, '%', 50),
  TREMOLO_MIX_RANGE: travel(0, 100, 0.1, '%', 100),
  TREMOLO_MODES: { kind: 'set', value: ['tremolo', 'pan'], default: 'tremolo' },
};
const DELAY = { FX_DELAY_TIME_RANGE: travel(0, 2000, 1, 'ms', 300) };

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
  const answers = JSON.parse(readFileSync(join(ROOT, 'recipes', 'examples', 'omx-tremolo.answers.json'), 'utf8'));
  const contract = join(WORK, 'omx-contract-1.4.0');
  fakeContract(contract, '1.4.0', { tremolo: TREMOLO, delay: DELAY });
  process.env.OMX_CONTRACT_DIR = contract;

  // ---- 1. the wizard -------------------------------------------------------------------------
  const tree = join(WORK, 'wizard');
  copyTree(tree);
  const plan = await planPlugin(answers, { root: tree, recipe });
  expect(plan.ok, `wizard accepts the tremolo answers${plan.ok ? '' : `: ${plan.refusals.map((r) => `${r.field} ${r.reason}`).join('; ')}`}`);
  if (!plan.ok) return;
  const { written, generated } = writePlugin(plan, { root: tree, recipe });
  const decl = JSON.parse(readFileSync(join(tree, 'plugins/omx-tremolo/omx-tremolo.decl.json'), 'utf8'));
  expect(decl.params.every((p) => p.ref && !('min' in p) && !('def' in p)), 'the written declaration is by reference only');
  const header = readFileSync(join(tree, 'plugins/omx-tremolo/generated/omx_tremolo_params.h'), 'utf8');
  expect(header.includes('{ "rateHz", "Rate", "Hz", 0.1f, 20.0f, 4.0f, 0u }'), 'the generated table carries the kernel file\'s rate travel');
  for (const a of recipe.artifacts.filter((x) => x.made === 'TEMPLATED' || x.made === 'HAND-WRITTEN')) {
    if (!a.template) continue;
    const at = typeof a.template === 'object' ? a.template.file : a.target.replace(/#.*/, '').replace(/\{stem\}/g, 'omx-tremolo').replace(/\{kernel\}/g, 'tremolo');
    expect(existsSync(join(tree, at)) && [...written, ...generated].includes(at), `the wizard wrote ${a.id} (${at})`);
  }
  const red = await checkPlugin(tree, recipe, 'omx-tremolo');
  const redIds = gapLines(red).join('\n');
  expect(/kernel-binding missing \(wizard step 'faces'/.test(redIds) && /kernel-identity-test missing \(wizard step 'tests'/.test(redIds),
    'red first: the stub binding and the stub oracle are named gaps');

  // ---- 2. the commit plan, applied literally ------------------------------------------------
  const commits = commitPlan(recipe, plan, [...written, ...generated]);
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
  const { omxdspInclude } = await import('../plugin-recipe.mjs');
  const dspName = (() => {
    const inc = omxdspInclude();
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
    ['linesPresent', 'spec-files', () => edit('packaging/omx-plugins.spec', (s) => s.replace('%{_libdir}/lv2/omx-drive.lv2/\n', ''))],
    ['installCovers', 'deb-install', () => edit('debian/omx-plugins-clap.install', () => 'usr/lib/clap/omx-delay.clap\n')],
    ['namedIn', 'package-description', () => edit('debian/control', (s) => s.replace(/delay, drive,/g, 'delay,'))],
    ['ciCovers', 'ci-installed-files', () => edit('.github/workflows/ci.yml', (s) => s.replaceAll('/usr/lib64/clap/omx-drive.clap', ''))],
    ['catalogueRow', 'catalogue-row', () => edit('README.md', (s) => s.replace('`org.openmixer.drive`', '`org.openmixer.x`'))],
    ['heading', 'manual-section', () => edit('README.md', (s) => s.replace('### omx drive\n', '### drive\n'))],
    ['namedIn', 'changelog', () => edit('CHANGELOG.md', (s) => s.replace('delay, drive,', 'delay,'))],
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
  undo = edit('CHANGELOG.md', (s) => `${s}\n- The chorus, a note for the test.\n`);
  const paid = await ratchet();
  undo();
  expect(paid.stale.some((x) => x.startsWith('omx-chorus changelog')), `sabotage: a debt entry now satisfied fails the ratchet as stale (${paid.stale.join(', ')})`);

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
    const more = [...loadDebt(repo), { plugin: 'omx-drive', entry: 'changelog', owedBy: 'sabotage' }];
    const grew = debtGrowth(repo, more);
    expect(grew.grown && grew.added.join() === 'omx-drive changelog', `sabotage: a debt entry added since main fails the growth check (${grew.added.join()})`);
    const fewer = debtGrowth(repo, loadDebt(repo).slice(1));
    expect(!fewer.grown, 'a debt that shrank passes the growth check');
    git(repo, 'update-ref', '-d', 'refs/remotes/origin/main');
    const none = debtGrowth(repo, loadDebt(repo));
    expect(none.base === undefined && /no merge-base/.test(none.why), `no main to compare to is reported, never a silent pass (${none.why?.slice(0, 60)})`);
  }

  // ---- 4. the recipe itself ----------------------------------------------------------------
  const grown = JSON.parse(JSON.stringify(recipe));
  grown.artifacts.push({ id: 'sabotage-notes', what: 'a section every plugin now owes', step: 'docs', layer: 'docs', made: 'HAND-WRITTEN', target: 'README.md', paths: [], questions: [], template: null, when: 'always', checker: { fn: 'heading', file: 'README.md', heading: '### {name} notes' } });
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
  const fresh = join(WORK, 'refusals');
  copyTree(fresh);
  const refused = async (what, mutate, field) => {
    const a = JSON.parse(JSON.stringify(answers));
    mutate(a);
    const p = await planPlugin(a, { root: fresh, recipe });
    const r = p.refusals.find((x) => x.field.startsWith(field));
    expect(!p.ok && r, `wizard refuses ${what}: ${r ? `${r.field} ${r.reason}` : 'NOT REFUSED'}`);
  };
  await refused('a taken stem', (a) => { a.stem = 'omx-drive'; a.kernel = 'drive'; }, '/stem');
  await refused('a typed travel', (a) => { a.params[0].min = 0.1; }, '/params/0');
  await refused('an unresolved reference', (a) => { a.params[1].ref = 'TREMOLO_SPEED_RANGE'; }, '/params/1/ref');
  await refused('a reference into another kernel', (a) => { a.params[0].ref = 'FX_DELAY_TIME_RANGE'; }, '/params/0/ref');
  await refused('a derived field answered wrong', (a) => { a.clap.id = 'org.openmixer.trem'; }, '/clap/id');
  await refused('a composite parameter that names no kernel', (a) => { a.kernels = ['tremolo', 'delay']; }, '/params/0/kernel');
  await refused('a parameter naming a kernel the plugin does not declare', (a) => { a.params[0].kernel = 'delay'; }, '/params/0/kernel');
  {
    const a = JSON.parse(JSON.stringify(answers));
    a.kernels = ['tremolo', 'delay'];
    for (const p of a.params) p.kernel = 'tremolo';
    a.params.push({ symbol: 'timeMs', name: 'Time', kernel: 'delay', ref: 'FX_DELAY_TIME_RANGE', scale: 'linear' });
    const p = await planPlugin(a, { root: fresh, recipe });
    const time = p.resolved?.find((x) => x.symbol === 'timeMs');
    const rate = p.resolved?.find((x) => x.symbol === 'rateHz');
    expect(p.ok && time?.max === 2000 && rate?.max === 20, `a composite resolves each parameter in its own kernel (timeMs from delay: ${time?.max}, rateHz from tremolo: ${rate?.max})`);
  }
  await refused('a panel the MOD GUI cannot draw', (a) => {
    a.panel = { family: 'modulation', roles: { mode: 'mode' }, sections: [{ key: 'tremolo', label: 'Tremolo', controls: ['rateHz', 'depth', 'mix', 'mode'] }] };
  }, '/panel');

  // ---- 6. the port hints ---------------------------------------------------------------------
  delete process.env.OMX_CONTRACT_DIR; // the real tree reads the release it pins
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

  console.log(fails ? `recipe-selftest: ${fails} check(s) failed` : 'recipe-selftest: every arm red when broken, green when whole');
  process.exit(fails ? 1 : 0);
}

await main();
