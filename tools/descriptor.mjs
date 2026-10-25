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
 * omx-panel lays a panel out from a descriptor of any origin (tools/omx-panel.mjs); for ours it reads this one.
 */
import { choicesOf, scaleOf } from './gen.mjs';
import { clapFeatureField, clapFeatureValue } from './format-tables.mjs';
import { installedContractDir, locateContract, ownersOf, packagingRange } from './omx-contract.mjs';
import { panelLayout } from './omx-panel.mjs';

/** The descriptor file of a plugin folder. */
export const descriptorFile = (stem) => `${stem}.descriptor.json`;

/** The schema the descriptor words are in, and the release that added them. */
export const DESCRIPTOR_SCHEMA = 'https://github.com/FreeMixer/omx-contract/raw/v3/schema/omx-contract.schema.json';
export const DESCRIPTOR_SCHEMA_FLOOR = '3.0.0';

/** The plugin features the descriptor names; a declaration's other CLAP features have no word there. */
export const FEATURES = ['audio-effect', 'instrument', 'note-effect', 'analyzer'];

/** The extension bag's source fields our faces write (omx-contract docs/descriptors.md, The extension bag): CLAP's
 * open features, in the face's order, and the LV2 face's types as its .ttl states them. */
export const CLAP_FEATURES_FIELD = 'clap_plugin_descriptor.features';
export const LV2_TYPE_FIELD = 'rdf:type';
const LV2_PLUGIN = 'lv2:Plugin';

/** What every LV2 face states it can use (tools/gen.mjs writes lv2:optionalFeature lv2:hardRTCapable), the
 * descriptor's needs.extensions.optional. */
export const LV2_HARD_RT = 'lv2:hardRTCapable';

/** The bag of a declaration: `{ clap?, lv2 }`. */
export function extensionsOf(d) {
  const open = d.clap.features.filter((f) => !FEATURES.includes(f));
  return {
    ...(open.length ? { clap: { [CLAP_FEATURES_FIELD]: open.length === 1 ? clapFeatureField(open[0]) : open.map(clapFeatureField) } } : {}),
    lv2: { [LV2_TYPE_FIELD]: d.lv2.class === LV2_PLUGIN ? LV2_PLUGIN : [LV2_PLUGIN, d.lv2.class] },
  };
}

/** The declaration's CLAP features and LV2 class, read back from a descriptor's words and bag. */
function bagOf(descriptor, file) {
  const ext = descriptor.extensions ?? {};
  const open = [ext.clap?.[CLAP_FEATURES_FIELD] ?? []].flat();
  const types = [ext.lv2?.[LV2_TYPE_FIELD] ?? []].flat();
  if (!Array.isArray(open) || open.some((f) => FEATURES.includes(clapFeatureValue(f)))) throw new Error(`${file}: extensions.clap["${CLAP_FEATURES_FIELD}"] holds CLAP's open features, a list without the ones plugin.features names`);
  if (!types.includes(LV2_PLUGIN) || types.length > 2) throw new Error(`${file}: extensions.lv2["${LV2_TYPE_FIELD}"] is the LV2 face's types, "${LV2_PLUGIN}" and at most its class`);
  return { features: [...descriptor.plugin.features, ...open.map(clapFeatureValue)], class: types.find((t) => t !== LV2_PLUGIN) ?? LV2_PLUGIN };
}

/** A descriptor's unit for a travel whose link resolves to `unit`: the item list's "" is a ratio or a fraction,
 * and a ratio (n:1) states `ratio`, the one word the descriptor says beyond its link (gap 4). */
const RATIO = 'ratio';

/** An integer travel's scale (PARAM_SCALES): what CLAP_PARAM_IS_STEPPED, lv2:integer and the parameter contract say. */
const STEPPED = 'stepped';
export const travelUnit = (unit, authored) => (unit === '' && authored === RATIO ? RATIO : unit);

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
      c.travel = { min: p.min, max: p.max, step: p.kind === 'integer' ? 1 : 0, unit: travelUnit(p.unit, d.descriptor?.controls[i]?.travel?.unit), default: p.def, scale: p.kind === 'integer' ? STEPPED : scaleOf(p) };
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
    plugin: { id: d.clap.id, format: 'clap', uri: d.lv2.uri, name: d.name, vendor: d.vendor, version: d.version, doc: d.description, features: d.clap.features.filter((f) => FEATURES.includes(f)) },
    controls,
    ports,
    needs: { extensions: { optional: [LV2_HARD_RT] } },
    latency: { reports: true },
    extensions: extensionsOf(d),
    source: { impl: face },
  };
  return { descriptor, extras: { choiceUnits, byKind } };
}

/** The descriptor file's text: what `node tools/gen.mjs` writes and `--check` holds equal. */
export const descriptorText = (d) => `${JSON.stringify(descriptorOf(d).descriptor, null, 2)}\n`;

/**
 * The description of declaration `d` (as tools/gen.mjs loads it): its descriptor laid out by omx-panel under its
 * declared panel. Every table in published order, each parameter by its symbol and its declared id.
 * Throws on what a host would refuse whole.
 */
export function panelDescription(d) {
  // the units the faces serve beside it (the parameter contract, the declaration's): a ratio's descriptor word,
  // ratio, has no param-contract unit in this release, so its role is judged on the unit the face serves
  const { descriptor, extras } = descriptorOf({ ...d, descriptor: undefined });
  return panelLayout(descriptor, { panel: d.panel, ns: `fx.${d.face ?? d.kernel}`, where: `plugins/${d.stem}`, ...extras });
}

/**
 * Whether declaration `decl` is whole: a base of variants (omx-eq) or a chain (omx-strip) keeps its own
 * parameters, identity and prose, and its descriptor is a render of it (gen.mjs writes it, --check holds it),
 * never an input. A single-kernel plugin's <stem>.decl.json is the extras of its descriptor, and carries none
 * of these.
 */
export const isWhole = (decl) => decl.variants !== undefined || decl.binding === 'chain';

/** The declaration fields the descriptor says: an extras file beside a descriptor carries none of them. */
const SAID = ['name', 'vendor', 'version', 'params', 'description'];

/**
 * The declaration `extras` (a <stem>.decl.json beside a descriptor) and `descriptor` make together, as
 * tools/gen.mjs resolves it: identity from the descriptor's plugin block, one parameter per control (its
 * reference read from the control's source link, or its travel typed for a control of the face's own),
 * and each control's extras (`controls`, by symbol) folded in. Throws, naming `file`, on what the two
 * would both say or what neither does.
 */
export function viewOf(descriptor, extras, file) {
  for (const k of SAID) if (k in extras) throw new Error(`${file}: "${k}" is the descriptor's to say (${descriptorFile(extras.stem)})`);
  if (extras.clap !== undefined || extras.lv2?.uri !== undefined || extras.lv2?.class !== undefined) throw new Error(`${file}: the plugin's CLAP id and features, its LV2 URI and class are the descriptor's to say (${descriptorFile(extras.stem)}: plugin and extensions)`);
  if (descriptor.origin !== 'authored') throw new Error(`${file}: ${descriptorFile(extras.stem)} is ${descriptor.origin}; ours is authored`);
  const known = new Set(descriptor.controls.map((c) => c.symbol));
  for (const sym of Object.keys(extras.controls ?? {})) if (!known.has(sym)) throw new Error(`${file}: "controls" names '${sym}', no control of the descriptor`);
  const { controls: perControl = {}, ...rest } = extras;
  const pl = descriptor.plugin;
  const bag = bagOf(descriptor, file);
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
      // an integer travel is stepped, as both faces and the parameter contract state it; its faces' log hint is its unit's
      if (c.travel && c.travel.scale !== (c.travel.step === 1 ? STEPPED : scaleOf({ unit: c.travel.unit }))) p.scale = c.travel.scale;
    } else {
      // a control of the face's own: its travel is typed, the reason kept with it
      if (!x.own) throw new Error(`${file}: '${c.symbol}' has no source link and no "own" reason`);
      if (c.kind === 'toggle') Object.assign(p, { unit: '', min: 0, max: 1, def: c.default ? 1 : 0, kind: 'toggle' });
      else if (c.kind === 'travel') Object.assign(p, { unit: c.travel.unit, min: c.travel.min, max: c.travel.max, def: c.travel.default, ...(c.travel.step === 1 ? { kind: 'integer' } : {}) });
      else throw new Error(`${file}: '${c.symbol}', a ${c.kind} of the face's own, is not a toggle or a travel`);
    }
    return { ...p, ...x };
  });
  return { ...rest, name: pl.name, vendor: pl.vendor, version: pl.version, description: pl.doc, clap: { id: pl.id, features: bag.features }, lv2: { uri: pl.uri, class: bag.class, ...rest.lv2 }, params };
}
