// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * descriptor.mjs — a plugin of ours in omx-contract's descriptor words (omx-contract docs/descriptors.md;
 * docs/declaration-as-descriptor.md):
 *
 *   - descriptorOf(d): the `authored` descriptor of a declaration as tools/gen.mjs loads it: the plugin's
 *     identity, one control per parameter, its ports, its latency. A plugin folder holding
 *     <stem>.descriptor.json keeps it equal to this render (gen.mjs writes it, --check refuses it stale),
 *     so the travels it states are always what its links resolve to.
 *   - viewOf(descriptor, extras): the declaration tools/gen.mjs resolves, from the descriptor and the
 *     extras its <stem>.decl.json keeps: what the descriptor has no word for.
 *
 * tools/panel-description.mjs lays a panel out from a descriptor of any origin; for ours it reads this one.
 */
import { choicesOf, scaleOf } from './gen.mjs';
import { installedContractDir, locateContract, ownersOf, packagingRange } from './omx-contract.mjs';

/** The descriptor file of a plugin folder. */
export const descriptorFile = (stem) => `${stem}.descriptor.json`;

/** The schema the descriptor words are in, and the release that added them. */
export const DESCRIPTOR_SCHEMA = 'https://github.com/FreeMixer/omx-contract/raw/v3/schema/omx-contract.schema.json';
export const DESCRIPTOR_SCHEMA_FLOOR = '2.15.0';

/** The plugin features the descriptor names; a declaration's other CLAP features have no word there. */
export const FEATURES = ['audio-effect', 'instrument', 'note-effect', 'analyzer'];

/** A panel role -> the descriptor meaning it plays, where the two names differ. A role whose name is
 * a meaning plays that meaning; a role in neither has no meaning word and is bound only by a panel. */
export const MEANING_OF_ROLE = Object.freeze({
  mix: 'wetDryRatio', time: 'delay', cutoff: 'cutoffFrequency',
  band_freq: 'frequency', band_gain: 'gain', band_q: 'q',
  hpf_freq: 'lowCut', low_cut: 'lowCut', lpf_freq: 'highCut', high_cut: 'highCut',
  hpf_slope: 'slope', lpf_slope: 'slope', key_listen: 'listen',
});

/** The descriptor's closed list of meanings (omx-contract docs/descriptors.md, Meanings), `unknown` aside. */
export const MEANINGS = Object.freeze([
  'gain', 'threshold', 'ratio', 'attack', 'release', 'hold', 'decay', 'frequency', 'cutoffFrequency', 'resonance', 'bypass',
  'wetDryRatio', 'delay', 'enabled',
  'range', 'knee', 'hysteresis', 'makeup', 'lookahead', 'ceiling', 'output', 'q', 'slope', 'width', 'drive', 'tone', 'rate',
  'depth', 'feedback', 'spread', 'predelay', 'size', 'damping', 'lowCut', 'highCut', 'shift', 'fine', 'sync', 'division', 'listen',
]);

/** The meaning role `role` plays, or undefined when the descriptor has no word for it. */
export function meaningOfRole(role) {
  const m = MEANING_OF_ROLE[role] ?? role;
  return MEANINGS.includes(m) ? m : undefined;
}

/** A reference link, `<file>#<NAME>[.<field>]`, as its parts. */
const LINK = /^(data\/[A-Za-z0-9_./-]+\.json)#([A-Za-z0-9_]+)(?:\.([A-Za-z0-9_]+))?$/;
export function parseLink(link) {
  const m = LINK.exec(link ?? '');
  if (!m) return undefined;
  const kernel = /^data\/kernels\/([a-z0-9_]+)\.json$/.exec(m[1])?.[1];
  return { file: m[1], ref: m[2], field: m[3], kernel };
}

/** The per-control extras a declaration keeps: how a value is read, not the value. */
export const CONTROL_EXTRAS = ['forKind', 'defaultRef', 'defaultId', 'defaultBand', 'perBand', 'unit', 'values', 'own'];

/**
 * `{ descriptor, extras }` of declaration `d`, every parameter resolved. Each control's id is the
 * parameter's index; its symbol and name the parameter's; a toggle is a toggle, an integer with labelled
 * values a choice (ids the values, as text), any other parameter a travel with the declared unit and
 * scale; its group the label of the panel section listing it, its meaning the one its declared role
 * plays; its source the link its travel is read from, or the face for a parameter of the face's own.
 * `extras` carries what the descriptor has no word for: a choice's unit and the per-kind come-up values.
 */
export function descriptorOf(d) {
  const groupOf = new Map();
  for (const s of d.panel?.sections ?? []) for (const sym of s.controls) groupOf.set(sym, [s.label]);
  const meaningOf = new Map();
  for (const [role, syms] of Object.entries(d.panel?.roles ?? {})) {
    const m = meaningOfRole(role);
    if (m) for (const sym of [syms].flat()) if (!meaningOf.has(sym)) meaningOf.set(sym, m);
  }
  // the release a link is read from: its owner (the contract directory's manifests, from omx-contract 2.15.0
  // on; omx-contract itself before) at the floor the packaging builds against, else the installed version
  let owners;
  const atOf = (rel) => {
    if (owners === undefined) {
      // the owners beside the render read (owners.json: a union render and every copy of it carry
      // one, OMX_CONTRACT_DIR's included), else the installed contract directory's manifests
      const where = locateContract(d.tree);
      owners = ownersOf(where.contractdir ?? where.dir);
      if (!owners.size) owners = ownersOf(installedContractDir());
    }
    const owner = owners.get(rel.split('#')[0]) ?? { package: 'omx-contract' };
    try {
      return `${owner.package}@${packagingRange(`${owner.package}-devel`, d.tree).floor}`;
    } catch {
      return owner.version ? `${owner.package}@${owner.version}` : null;
    }
  };
  const face = `plugins/${d.stem}/omx_${d.kernel}_clap.c#clap_entry`;
  const choiceUnits = {}, byKind = {};
  const controls = d.params.map((p, i) => {
    const c = { id: i, symbol: p.symbol, name: p.name };
    const choices = choicesOf(p);
    if (p.kind === 'toggle') Object.assign(c, { kind: 'toggle', default: p.def !== 0 });
    else if (choices) {
      Object.assign(c, { kind: 'choice', ids: choices.map((x) => String(x.value)), labels: choices.map((x) => x.label), values: choices.map((x) => x.value), default: String(p.def) });
      if (p.unit) choiceUnits[p.symbol] = p.unit;
    } else {
      c.kind = 'travel';
      c.travel = { min: p.min, max: p.max, step: p.kind === 'integer' ? 1 : 0, unit: p.unit, default: p.def, scale: scaleOf(p) };
    }
    if (groupOf.has(p.symbol)) c.group = groupOf.get(p.symbol);
    if (meaningOf.has(p.symbol)) c.meaning = meaningOf.get(p.symbol);
    c.automatable = true;
    c.source = p.from ? { from: atOf(p.from) ? { link: p.from, at: atOf(p.from) } : p.from } : { impl: face };
    if (p.byKind) byKind[p.symbol] = p.byKind;
    return c;
  });
  const ports = [
    { id: 0, name: 'in', kind: 'audio', direction: 'in', role: 'main', channels: 2, layout: 'stereo' },
    ...(d.sidechain ? [{ id: 1, name: d.sidechain.symbol, kind: 'audio', direction: 'in', role: 'sidechain', channels: 1, layout: 'mono' }] : []),
    { id: 0, name: 'out', kind: 'audio', direction: 'out', role: 'main', channels: 2, layout: 'stereo' },
  ];
  const descriptor = {
    $schema: DESCRIPTOR_SCHEMA,
    requires: { 'omx-contract-schema': `>= ${DESCRIPTOR_SCHEMA_FLOOR}` },
    origin: 'authored',
    plugin: { id: d.clap.id, format: 'clap', uri: d.lv2.uri, name: d.name, vendor: d.vendor, version: d.version, features: d.clap.features.filter((f) => FEATURES.includes(f)) },
    controls,
    ports,
    latency: { reports: true },
    source: { impl: face },
  };
  return { descriptor, extras: { choiceUnits, byKind } };
}

/** The descriptor file's text: what `node tools/gen.mjs` writes and `--check` holds equal. */
export const descriptorText = (d) => `${JSON.stringify(descriptorOf(d).descriptor, null, 2)}\n`;

/** The declaration fields the descriptor says: an extras file beside a descriptor carries none of them. */
const SAID = ['name', 'vendor', 'version', 'params'];

/**
 * The declaration `extras` (a <stem>.decl.json beside a descriptor) and `descriptor` make together, as
 * tools/gen.mjs resolves it: identity from the descriptor's plugin block, one parameter per control (its
 * reference read from the control's source link, or its travel typed for a control of the face's own),
 * and each control's extras (`controls`, by symbol) folded in. Throws, naming `file`, on what the two
 * would both say or what neither does.
 */
export function viewOf(descriptor, extras, file) {
  for (const k of SAID) if (k in extras) throw new Error(`${file}: "${k}" is the descriptor's to say (${descriptorFile(extras.stem)})`);
  if (extras.clap?.id !== undefined || extras.lv2?.uri !== undefined) throw new Error(`${file}: the plugin's CLAP id and LV2 URI are the descriptor's to say (${descriptorFile(extras.stem)})`);
  if (descriptor.origin !== 'authored') throw new Error(`${file}: ${descriptorFile(extras.stem)} is ${descriptor.origin}; ours is authored`);
  const known = new Set(descriptor.controls.map((c) => c.symbol));
  for (const sym of Object.keys(extras.controls ?? {})) if (!known.has(sym)) throw new Error(`${file}: "controls" names '${sym}', no control of the descriptor`);
  const { controls: perControl = {}, ...rest } = extras;
  const pl = descriptor.plugin;
  const params = descriptor.controls.map((c, i) => {
    if (c.id !== i) throw new Error(`${file}: the descriptor's control ${i} has id ${JSON.stringify(c.id)}; our ids are the parameter order`);
    const x = perControl[c.symbol] ?? {};
    for (const k of Object.keys(x)) if (!CONTROL_EXTRAS.includes(k)) throw new Error(`${file}: '${c.symbol}' keeps "${k}", which is not an extra (${CONTROL_EXTRAS.join(', ')})`);
    const p = { symbol: c.symbol, name: c.name };
    const from = c.source?.from;
    if (from !== undefined) {
      const link = parseLink(typeof from === 'string' ? from : from.link);
      if (!link) throw new Error(`${file}: '${c.symbol}' reads its travel from ${JSON.stringify(from)}, not a link <file>#<NAME>[.<field>]`);
      p.ref = link.ref;
      if (link.field) p.field = link.field;
      if (link.kernel && (rest.kernels ?? [rest.kernel]).length > 1) p.kernel = link.kernel;
      if (c.travel && c.travel.scale !== scaleOf({ unit: c.travel.unit })) p.scale = c.travel.scale;
    } else {
      // a control of the face's own: its travel is typed, the reason kept with it
      if (!x.own) throw new Error(`${file}: '${c.symbol}' has no source link and no "own" reason`);
      if (c.kind === 'toggle') Object.assign(p, { unit: '', min: 0, max: 1, def: c.default ? 1 : 0, kind: 'toggle' });
      else if (c.kind === 'travel') Object.assign(p, { unit: c.travel.unit, min: c.travel.min, max: c.travel.max, def: c.travel.default, ...(c.travel.step === 1 ? { kind: 'integer' } : {}) });
      else throw new Error(`${file}: '${c.symbol}', a ${c.kind} of the face's own, is not a toggle or a travel`);
    }
    return { ...p, ...x };
  });
  return { ...rest, name: pl.name, vendor: pl.vendor, version: pl.version, clap: { id: pl.id, ...rest.clap }, lv2: { uri: pl.uri, ...rest.lv2 }, params };
}
