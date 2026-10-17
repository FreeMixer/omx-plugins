// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * descriptor.mjs — a declaration (as tools/gen.mjs loads it) rendered as its `authored` plugin
 * descriptor, in omx-contract's descriptor words (omx-contract docs/descriptors.md): the plugin's
 * identity, one control per parameter, its ports. tools/panel-description.mjs lays a panel out from a
 * descriptor of any origin; for our plugins it reads this one (docs/panel-layout.md).
 *
 * What a declaration states that the descriptor has no word for goes in `extras`, beside it, never
 * into the descriptor: a choice's unit (a slope's dB/oct) and the per-placement come-up values.
 */
import { choicesOf, scaleOf } from './gen.mjs';

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

/**
 * `{ descriptor, extras }` of declaration `d`. Each control's id is the parameter's index, its name the
 * symbol, its label the parameter's name; a toggle is a toggle, an integer with labelled values a
 * choice (ids the values, as text), any other parameter a travel with the declared unit and scale;
 * its group the label of the panel section listing it, its meaning the one its declared role plays.
 */
export function descriptorOf(d) {
  const groupOf = new Map();
  for (const s of d.panel?.sections ?? []) for (const sym of s.controls) groupOf.set(sym, [s.label]);
  const meaningOf = new Map();
  for (const [role, syms] of Object.entries(d.panel?.roles ?? {})) {
    const m = meaningOfRole(role);
    if (m) for (const sym of [syms].flat()) if (!meaningOf.has(sym)) meaningOf.set(sym, m);
  }
  const choiceUnits = {}, byKind = {};
  const controls = d.params.map((p, i) => {
    const c = { id: i, name: p.symbol, label: p.name };
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
    if (p.byKind) byKind[p.symbol] = p.byKind;
    return c;
  });
  const stereo = (id, name, direction) => ({ id, name, kind: 'audio', direction, role: 'main', channels: 2, layout: 'stereo' });
  const ports = [
    stereo(0, 'Input', 'in'),
    ...(d.sidechain ? [{ id: 1, name: d.sidechain.name, kind: 'audio', direction: 'in', role: 'sidechain', channels: 1, layout: 'mono' }] : []),
    stereo(0, 'Output', 'out'),
  ];
  const descriptor = {
    origin: 'authored',
    plugin: { id: d.clap.id, format: 'clap', uri: d.lv2.uri, name: d.name, vendor: d.vendor, version: d.version, features: d.clap.features.filter((f) => FEATURES.includes(f)) },
    controls,
    ports,
    source: { impl: `plugins/${d.stem}/${d.stem}.decl.json` },
  };
  return { descriptor, extras: { choiceUnits, byKind } };
}
