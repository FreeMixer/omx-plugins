// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * panel-description.mjs — org.openmixer.panel-description/1 of a plugin, from its declaration's
 * `panel`: the family, the sections in order, one control per listed parameter, the roles its family
 * takes, a display per section `graph` the extension names, and the canonical serialisation whose
 * SHA-256 is the fingerprint. tools/gen.mjs writes the rows a face serves (include/
 * omx_panel_description.h); tools/panel-check.mjs reads what a built face serves back against the
 * same declaration.
 *
 * The vocabulary below is the extension's: its families, each family's roles with the units a role
 * accepts, and the displays a family draws in closed form. A declaration's role the family has no
 * place for, or whose parameter's unit the role does not take, is not published (a host would drop
 * it and the fingerprint would no longer be what it reads); the generated header names each one.
 */
import { createHash } from 'node:crypto';

export const PANEL_ID = 'org.openmixer.panel-description/1';

/** The extension's families, by enum value. */
export const FAMILIES = ['none', 'gate', 'dynamics', 'limiter', 'deesser', 'eq', 'filter', 'drive', 'modulation', 'delay', 'reverb', 'pitch'];

/** The widgets, by enum value. A toggle is a switch, a labelled integer AUTO (a selector needs
 * CLAP_PARAM_IS_ENUM, which the faces do not set; a declared `choice` is AUTO too), any other travel a
 * knob; a declared `widgets` entry wins. */
export const WIDGETS = ['auto', 'knob', 'fader', 'switch', 'selector'];
const WIDGET_OF = { knob: 'knob', fader: 'fader', switch: 'switch', choice: 'auto' };

/** The display kinds and sources, by enum value. */
export const DISPLAY_KINDS = ['gain_reduction_bar', 'transfer_curve', 'eq_response', 'meter'];
export const DISPLAY_SOURCES = ['family', 'meters', 'gain_adjustment', 'curve'];
export const PLACEMENTS = ['insert', 'return', 'bus', 'master'];

/** A section `graph` -> the display the extension names for it; a graph with none here is the
 * renderer's own (delay taps, a decay envelope, an LFO). */
const DISPLAY_OF_GRAPH = { 'eq-curve': 'eq_response', 'dynamics-curve': 'transfer_curve', 'transfer-curve': 'transfer_curve' };
/** The displays a host computes from a family's roles in closed form. */
const CLOSED_FORMS = { gate: ['transfer_curve'], dynamics: ['transfer_curve'], eq: ['eq_response'], drive: ['transfer_curve'] };

/** A declared unit -> the unit a role is checked against. */
const UNIT_OF = { '': 'none', dB: 'db', dBFS: 'dbfs', Hz: 'hz', ms: 'ms', s: 's', '%': 'pc', x: 'coef', oct: 'oct', cents: 'cent', st: 'semitone12TET' };

const T = ['ms', 's'], LVL = ['db', 'dbfs'], DB = ['db'], FREQ = ['hz', 'khz'], PLAIN = ['none', 'pc', 'coef'], RATIO = ['ratio', 'coef'], CHOICE = ['none'];
const WET = { mix: ['pc', 'coef', 'none'], output: DB };
/** Each family's roles and the units each accepts. */
export const ROLES = {
  none: {},
  gate: { threshold: LVL, range: DB, attack: T, release: T, hold: T, knee_start: DB, knee_end: DB, hysteresis: DB, ratio: RATIO, key_listen: CHOICE },
  dynamics: { threshold: LVL, ratio: RATIO, attack: T, release: T, knee: DB, makeup: DB, lookahead: ['ms'], ...WET },
  limiter: { ceiling: LVL, release: T, lookahead: ['ms'], threshold: LVL, ...WET },
  deesser: { frequency: FREQ, threshold: LVL, range: DB, width: ['oct'], ratio: RATIO, attack: T, release: T, knee: DB, mode: CHOICE, listen: CHOICE, ...WET },
  eq: { band_type: CHOICE, band_freq: FREQ, band_gain: ['db', 'coef'], band_q: ['none', 'coef', 'oct'], band_on: CHOICE, hpf_freq: FREQ, hpf_slope: ['none', 'db'], hpf_on: CHOICE, lpf_freq: FREQ, lpf_slope: ['none', 'db'], lpf_on: CHOICE, output: DB },
  filter: { cutoff: FREQ, resonance: PLAIN, filter_type: CHOICE, ...WET },
  drive: { drive: DB, tone: PLAIN, model: CHOICE, ...WET },
  modulation: { rate: ['hz', 'bpm'], depth: PLAIN, feedback: PLAIN, voices: CHOICE, spread: PLAIN, shape: CHOICE, ...WET },
  delay: { time: T, feedback: PLAIN, tone: PLAIN, sync: CHOICE, division: CHOICE, pingpong: CHOICE, ...WET },
  reverb: { size: PLAIN, decay: T, algorithm: CHOICE, predelay: ['ms'], damping: PLAIN, width: PLAIN, low_cut: FREQ, high_cut: FREQ, ...WET },
  pitch: { shift: ['semitone12TET', 'cent'], fine: ['cent'], ...WET },
};

const KEY = /^[a-z][a-z0-9_]*(\.[a-z0-9_]+)*$/;
const KEY_MAX = 63, LABEL_MAX = 255;

/** camelCase -> snake_case, the key grammar's spelling of a symbol or section key. */
export const snake = (s) => s.replace(/([a-z0-9])([A-Z])/g, '$1_$2').toLowerCase();

/**
 * The description of declaration `d` (as tools/gen.mjs loads it): every table in published order,
 * each parameter by its symbol and its declared id. Throws on what a host would refuse whole.
 */
export function panelDescription(d) {
  const where = `plugins/${d.stem}`;
  const panel = d.panel;
  // the declaration's family is one of the extension's (the schema's $defs.family is the same closed
  // set); a family the extension has no place for is none: no house view
  const family = FAMILIES.includes(panel.family) ? panel.family : 'none';
  const ns = `fx.${d.face ?? d.kernel}`;
  const id = new Map(d.params.map((p, i) => [p.symbol, i]));
  const param = (sym) => d.params[id.get(sym)];
  const key = (k, what) => {
    if (!KEY.test(k) || k.length > KEY_MAX) throw new Error(`${where}: ${what} '${k}' is not a key of ${PANEL_ID} ([a-z][a-z0-9_]*, dotted, at most ${KEY_MAX} bytes)`);
    return k;
  };
  const label = (s, what) => {
    if (Buffer.byteLength(s) > LABEL_MAX) throw new Error(`${where}: ${what} label is longer than ${LABEL_MAX} bytes`);
    return s;
  };

  const sections = panel.sections.map((s) => {
    const k = key(snake(s.key), 'section key');
    return { key: k, labelKey: key(`${ns}.section.${k}`, 'section label key'), label: label(s.label, `section '${s.key}'`) };
  });
  if (new Set(sections.map((s) => s.key)).size !== sections.length) throw new Error(`${where}: two panel sections share a key`);

  const seen = new Set();
  const controls = panel.sections.flatMap((s, section) =>
    s.controls.map((sym) => {
      const p = param(sym);
      if (!p) throw new Error(`${where}: panel section '${s.key}' lists '${sym}', no parameter`);
      if (seen.has(sym)) throw new Error(`${where}: '${sym}' is a control twice`);
      seen.add(sym);
      const declared = panel.widgets?.[sym];
      const widget = declared ? WIDGET_OF[declared] : p.kind === 'toggle' ? 'switch' : p.points || p.values ? 'auto' : 'knob';
      if (widget === 'switch' && p.kind !== 'toggle') throw new Error(`${where}: '${sym}' is drawn as a switch and is not a toggle`);
      return { symbol: sym, param: id.get(sym), section, sectionKey: sections[section].key, widget, labelKey: key(`${ns}.${snake(sym)}`, 'control label key'), label: label(p.name, `'${sym}'`), visible: null, mask: 0n };
    }),
  );

  const table = ROLES[family];
  const roles = [], unpublished = [];
  // an indexed role (an EQ band's) names every band's symbol in order: one row per band, its index the band's
  for (const [role, syms] of Object.entries(panel.roles ?? {})) {
    [syms].flat().forEach((sym, index) => {
      const p = param(sym);
      if (!p) throw new Error(`${where}: role '${role}' names '${sym}', no parameter`);
      const unit = UNIT_OF[p.unit];
      if (!table[role]) unpublished.push({ role, symbol: sym, why: `not a role of the ${family} family` });
      else if (!table[role].includes(unit)) unpublished.push({ role, symbol: sym, why: `the ${role} role takes ${table[role].join(', ')}, not ${p.unit || 'no unit'}` });
      else roles.push({ role, index, symbol: sym, param: id.get(sym) });
    });
  }

  const displays = panel.sections.flatMap((s, section) => {
    const kind = DISPLAY_OF_GRAPH[s.graph];
    if (!kind) return [];
    if (!CLOSED_FORMS[family]?.includes(kind)) throw new Error(`${where}: section '${s.key}' draws ${s.graph}, and the ${family} family has no ${kind} a host computes`);
    return [{ kind, section, sectionKey: sections[section].key, source: 'family', meters: [null, null], curve: null, floor: null, labelKey: key(`${ns}.display.${kind}`, 'display label key'), label: label(s.label, `section '${s.key}'`) }];
  });

  // the contract's per-kind defaults: each channel kind the placement it is, the value inside the travel
  const defaults = d.params.flatMap((p, i) =>
    Object.entries(p.byKind ?? {}).map(([kind, value]) => {
      const placement = PLACEMENT_OF_KIND[kind];
      if (!placement) throw new Error(`${where}: '${p.symbol}' comes up differently on a ${kind} channel, which has no placement of ${PANEL_ID}; add it to PLACEMENT_OF_KIND`);
      if (!(value >= p.min && value <= p.max) || (p.kind && !Number.isInteger(value))) throw new Error(`${where}: '${p.symbol}' on ${kind} comes up at ${value}, outside its travel`);
      return { placement, symbol: p.symbol, param: i, value };
    }),
  );

  return { family, sections, controls, roles, unpublished, displays, defaults };
}

/** A channel kind whose default a contract travel states (byKind) -> the placement it is. */
const PLACEMENT_OF_KIND = { fxReturn: 'return', main: 'master' };

/** The surface's slots per page, CLAP_REMOTE_CONTROLS_COUNT. */
export const PAGE_SIZE = 8;

/** The pages of clap.remote-controls/2 by the page rule: each section's controls in order, cut into
 * pages of eight, a section of several pages numbering them; a short page's slots are null. */
export function pagesOf(x, d) {
  return d.panel.sections.flatMap((s, section) => {
    const ids = x.controls.filter((c) => c.section === section).map((c) => c.param);
    const count = Math.ceil(ids.length / PAGE_SIZE);
    return [...Array(count).keys()].map((k) => {
      const slots = ids.slice(k * PAGE_SIZE, (k + 1) * PAGE_SIZE);
      while (slots.length < PAGE_SIZE) slots.push(null);
      return { section, sectionName: s.label, pageName: count > 1 ? `${s.label} ${k + 1}` : s.label, ids: slots };
    });
  });
}

/** The shortest decimal that round-trips to the same binary64, -0 written 0. */
const shortest = (n) => (Object.is(n, -0) ? '0' : String(n));
const byCodeUnit = (a, b) => (a < b ? -1 : a > b ? 1 : 0);

/** The canonical serialisation: UTF-8, LF after every line, TAB-separated, parameters by symbol, the
 * fallback labels left out. */
export function panelCanonical(x) {
  const lines = [PANEL_ID, `family\t${x.family}`];
  x.sections.forEach((s, i) => lines.push(['section', i, s.key, s.labelKey].join('\t')));
  for (const c of x.controls) lines.push(['control', c.symbol, c.sectionKey, c.widget, c.labelKey, c.visible ?? '-', c.mask.toString(16)].join('\t'));
  for (const r of [...x.roles].sort((a, b) => byCodeUnit(a.role, b.role) || a.index - b.index)) lines.push(['role', r.role, r.index, r.symbol].join('\t'));
  for (const v of x.displays) {
    const floor = v.floor === null ? '-' : typeof v.floor === 'number' ? shortest(v.floor) : v.floor;
    const meter = (m) => (m === null ? '-' : String(m));
    lines.push(['display', v.kind, v.sectionKey, v.source, meter(v.meters[0]), meter(v.meters[1]), v.curve ?? '-', floor, v.labelKey].join('\t'));
  }
  const order = (p) => PLACEMENTS.indexOf(p);
  for (const p of [...x.defaults].sort((a, b) => order(a.placement) - order(b.placement) || byCodeUnit(a.symbol, b.symbol))) lines.push(['default', p.placement, p.symbol, shortest(p.value)].join('\t'));
  return `${lines.join('\n')}\n`;
}

/** The fingerprint: lowercase hex SHA-256 of the canonical serialisation. */
export const panelFingerprint = (x) => createHash('sha256').update(panelCanonical(x)).digest('hex');
