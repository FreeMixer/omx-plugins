// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * variants.mjs — ONE declaration for several plugins that differ in a band count only.
 *
 * The channel EQ ships as omx-eq8, omx-eq16 and omx-eq32: one kernel, one instance face, three
 * numbers. By the one-home rule (spec 2026-10-09-plugin-from-contract §6, §7) the only fact that
 * differs between them is the band count, and its home is omx-contract's count sheet
 * (EQ_BAND_COUNTS.eq8.max, ...). So the plugins have ONE hand-written declaration,
 * plugins/<base>/<base>.decl.json, carrying
 *
 *   "variants": { "count": "EQ_BAND_COUNTS", "of": [{ "stem": "omx-eq8", "count": "eq8" }, ...] }
 *
 * and parameters of two sorts: the plugin's own (written once) and the per-band ones, marked
 * `"perBand": true`, written once and repeated for every band of a variant, in band order, each
 * band's parameters together: symbol `b<n>_<symbol>`, name `Band <n> <name>`, and a `defaultBand`
 * given only its `of` takes the variant's strip and the band's index. `{bands}` in the prose
 * (`noun`, `description`, `summary`, `manual`) reads the variant's count.
 *
 * Each variant is a plugin of its own, in plugins/<variant stem>/, which holds generated files
 * only. Its declaration is the base's, expanded here: stem, name, CLAP id and LV2 URI derived from
 * the variant's stem as for any plugin; `kernel`, the C name of its files, the variant's short name
 * (eq8); `face`, the omx-dsp instance face it binds, the base's kernel (eq); `variantOf`, the base,
 * the count sheet entry and the band count, which the parameter header passes to the face as
 * OMX_<FACE>_INSTANCE_BANDS.
 */
import { existsSync, readFileSync, readdirSync } from 'node:fs';
import { basename, join, resolve } from 'node:path';
import { items } from './omx-contract.mjs';

/** The base declaration (raw) whose `variants` names `stem`, searched beside `dir`'s folder, or undefined. */
export function baseOf(dir) {
  const stem = basename(resolve(dir));
  const plugins = resolve(dir, '..');
  if (!existsSync(plugins)) return undefined;
  for (const n of readdirSync(plugins)) {
    const file = join(plugins, n, `${n}.decl.json`);
    if (!existsSync(file)) continue;
    let d;
    try {
      d = JSON.parse(readFileSync(file, 'utf8'));
    } catch {
      continue;
    }
    if (d.variants?.of?.some((v) => v.stem === stem)) return { file, decl: d };
  }
  return undefined;
}

/** The variant stems a base declaration names. */
export const variantStems = (decl) => (decl.variants?.of ?? []).map((v) => v.stem);

/** A sheet entry's number: a number, or `{ref}` to a scalar. */
function sheetNumber(contractDir, v, what) {
  if (typeof v === 'number') return v;
  if (v && typeof v.ref === 'string') {
    const e = items(contractDir)[v.ref];
    if (e && typeof e.value === 'number') return e.value;
  }
  throw new Error(`${what} is no number omx-contract states`);
}

/** The band count of `variant` (`{stem, count}`) in the base's count sheet. */
export function variantBands(contractDir, base, variant) {
  const sheet = items(contractDir)[base.variants.count];
  if (!sheet || sheet.kind !== 'sheet') throw new Error(`variants: ${base.variants.count} is no sheet in omx-contract`);
  const entry = sheet.value?.[variant.count];
  if (!entry) throw new Error(`variants: ${base.variants.count} has no '${variant.count}'`);
  return sheetNumber(contractDir, entry.max, `${base.variants.count}.${variant.count}.max`);
}

const fillBands = (s, n) => (typeof s === 'string' ? s.replaceAll('{bands}', String(n)) : s);

/** The raw declaration of one variant of `base`, its prose filled and its per-band parameters repeated. */
export function expandVariant(contractDir, base, variant) {
  const n = variantBands(contractDir, base, variant);
  const short = variant.stem.replace(/^omx-/, '');
  const own = base.params.filter((p) => !p.perBand);
  const per = base.params.filter((p) => p.perBand);
  const bands = [];
  for (let b = 1; b <= n; b++) {
    for (const p of per) {
      const { perBand: _perBand, ...q } = p;
      q.symbol = `b${b}_${p.symbol}`;
      q.name = `Band ${b} ${p.name}`;
      if (p.defaultBand) q.defaultBand = { strip: variant.count, index: b - 1, ...p.defaultBand };
      bands.push(q);
    }
  }
  const { variants: _variants, $comment: _comment, ...rest } = base;
  const d = {
    ...rest,
    $comment: `The ${variant.stem} variant of plugins/${base.stem}/${base.stem}.decl.json (tools/variants.mjs): its ${n} bands, nothing hand-written here.`,
    stem: variant.stem,
    kernel: short.replace(/-/g, '_'),
    face: base.kernel,
    kernels: base.kernels ?? [base.kernel],
    variantOf: { stem: base.stem, count: base.variants.count, key: variant.count, bands: n },
    name: `omx ${short}`,
    clap: { ...base.clap, id: `org.openmixer.${short}` },
    lv2: { ...base.lv2, uri: `urn:openmixer:${short}` },
    params: [...own, ...bands],
  };
  for (const k of ['noun', 'description', 'summary']) if (d[k] !== undefined) d[k] = fillBands(d[k], n);
  if (d.manual) d.manual = d.manual.map((l) => fillBands(l, n));
  return d;
}

/** The C macro the face reads for the band count: OMX_<SHEET>_<KEY>_MAX of the contract's render. */
export const bandsMacro = (d) => `OMX_${d.variantOf.count}_${d.variantOf.key.toUpperCase()}_MAX`;
