#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * modgui-gen.mjs — a MOD modgui for every plugin whose declaration carries a `panel` block, so MOD
 * and Zynthian draw our plugins properly.
 *
 * ONE declaration: plugins/<stem>/<stem>.decl.json, read by tools/gen.mjs's loadDecl. Its `panel`
 * block gives the family and the sections; every control's symbol, travel, default and label is its
 * LV2 port's, from gen.mjs's lv2Ports exactly as the TTL is. The colours and the font are the dark
 * theme's `openmixer` look, resolved once into tools/look.json. This file types no travel and no colour.
 *
 * Written into plugins/<stem>/generated/<stem>.lv2/: `modgui.ttl` and `modgui/` (template,
 * stylesheet, screenshot, thumbnail, knob film strip). The images are rasterised here from the
 * template's own layout (raster.mjs) — no text, each knob at its declared default, byte-deterministic.
 *
 * Usage: `node tools/modgui-gen.mjs [--check|--emit-stdout] [plugins/<stem> ...]` — `--check` writes
 * nothing and refuses a stale, missing or extra file byte for byte; `--emit-stdout` prints every text
 * file and one digest line per image, writing nothing.
 */
import { createHash } from 'node:crypto';
import { existsSync, mkdirSync, readdirSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { dirname, join, relative, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { loadDecl, lv2Ports, pluginDirs } from './gen.mjs';
import { canvas, disc, encodePng, rgbOf, roundRect, segment } from './raster.mjs';

const TOOLS = dirname(fileURLToPath(import.meta.url));
export const LOOK_JSON = join(TOOLS, 'look.json');
export const MODGUI_TTL = 'modgui.ttl';
export const MODGUI_DIR = 'modgui';
/** An integer travel with more steps than this is a continuous knob; fewer is a selector. */
export const MAX_SELECTOR_STEPS = 16;

/** The geometry, in CSS px: one set of numbers the template, the stylesheet and the raster read. */
export const GEOMETRY = Object.freeze({
  pad: 16, headH: 40, sectionTop: 52, sectionGap: 10, sectionPad: 8, sectionTitleH: 18,
  cellW: 72, cellH: 76, knob: 48, knobTop: 4, switchW: 40, switchH: 20, switchTop: 18,
  radius: 10, sectionRadius: 6, filmFrames: 65, thumbScale: 0.5,
});

const esc = (s) => String(s).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');
const ttlStr = (s) => `"${String(s).replace(/\\/g, '\\\\').replace(/"/g, '\\"')}"`;

/**
 * The widget AUTO resolves to for a declared parameter: a toggle is a switch; an integer with
 * a wide travel (more than MAX_SELECTOR_STEPS steps, e.g. a time in ms) is a knob; an enumeration,
 * or an integer with a few steps, would be a selector, which this export does not draw: refused.
 */
export function widgetOf(p) {
  if (p.kind === 'toggle') return 'switch';
  const steps = p.max - p.min;
  if (p.enumeration || p.scalePoints || (p.kind === 'integer' && steps <= MAX_SELECTOR_STEPS)) {
    throw new Error(`modgui-gen: '${p.symbol}' is a selector, which the modgui export does not draw`);
  }
  return 'knob';
}

/** Where a value sits on its travel, 0..1 (every declared travel is linear). */
export function travelFraction(p, v) {
  return p.max === p.min ? 0 : (v - p.min) / (p.max - p.min);
}

/** The panel of a declaration, resolved: each section with its controls, each control its LV2 port. */
export function resolvePanel(d) {
  if (!d.panel) throw new Error(`modgui-gen: '${d.stem}' declares no panel`);
  const ports = lv2Ports(d);
  const seen = new Set();
  const sections = d.panel.sections.map((s) => ({
    key: s.key,
    label: s.label,
    controls: s.controls.map((symbol) => {
      if (seen.has(symbol)) throw new Error(`modgui-gen: '${symbol}' is in two sections`);
      seen.add(symbol);
      const port = ports.find((p) => p.symbol === symbol);
      if (!port) throw new Error(`modgui-gen: panel names '${symbol}', which '${d.stem}' has no port for`);
      if (!port.param || !/lv2:ControlPort/.test(port.a) || !/lv2:InputPort/.test(port.a)) {
        throw new Error(`modgui-gen: '${symbol}' is not a declared control input with a travel`);
      }
      const q = port.param;
      return { symbol, name: port.name, index: port.index, widget: widgetOf(q), min: q.min, max: q.max, def: q.def };
    }),
  }));
  if (sections.length === 0 || sections[0].controls.length === 0) throw new Error(`modgui-gen: '${d.stem}' has an empty first section`);
  return { uri: d.lv2.uri, stem: d.stem, name: d.name, brand: d.vendor, family: d.panel.family, sections };
}

/** Every box of the face, in px at scale 1: what the template positions and the raster paints. */
export function layout(panel) {
  const g = GEOMETRY;
  const most = Math.max(...panel.sections.map((s) => s.controls.length));
  const sectionW = most * g.cellW + 2 * g.sectionPad;
  const sectionH = g.sectionTitleH + g.cellH + g.sectionPad / 2;
  const sections = panel.sections.map((s, i) => {
    const y = g.sectionTop + i * (sectionH + g.sectionGap);
    return {
      ...s, x: g.pad, y, w: sectionW, h: sectionH,
      controls: s.controls.map((c, j) => ({ ...c, x: g.sectionPad + j * g.cellW, y: g.sectionTitleH })),
    };
  });
  const last = sections[sections.length - 1];
  return { w: sectionW + 2 * g.pad, h: last.y + last.h + g.pad, sections };
}

/** The look: tools/look.json, every colour a plain #rrggbb. */
export function loadLook() {
  const look = JSON.parse(readFileSync(LOOK_JSON, 'utf8'));
  const need = ['--surface', '--surface-2', '--surface-3', '--border-strong', '--ink', '--ink-dim', '--accent', '--font-ui'];
  const tokens = {};
  for (const k of need) {
    const v = look.tokens?.[k];
    if (typeof v !== 'string' || !v) throw new Error(`modgui-gen: tools/look.json resolves no ${k}`);
    if (k !== '--font-ui') rgbOf(v);
    tokens[k] = v;
  }
  return { theme: look.theme, personality: look.personality, tokens };
}

/** The literal colours and font the face wears. */
export const lookTokens = () => loadLook().tokens;

// ------------------------------------------------------------------------------------ drawing

/** One knob at travel fraction `f`: rim, body, pointer over a 270° sweep from the lower left. */
function drawKnob(c, cx, cy, r, f, col) {
  disc(c, cx, cy, r, col.rim);
  disc(c, cx, cy, r - Math.max(1, r * 0.08), col.body);
  const th = (-135 + 270 * Math.min(1, Math.max(0, f))) * (Math.PI / 180);
  const sx = Math.sin(th), sy = -Math.cos(th); // the segment snaps its ends
  segment(c, cx + sx * r * 0.28, cy + sy * r * 0.28, cx + sx * r * 0.78, cy + sy * r * 0.78, Math.max(0.75, r * 0.08), col.accent);
}

function drawSwitch(c, x, y, w, h, on, col) {
  roundRect(c, x, y, w, h, h / 2, on ? col.accent : col.body);
  const r = h / 2 - h * 0.15;
  disc(c, on ? x + w - h / 2 : x + h / 2, y + h / 2, r, on ? col.ink : col.inkDim);
}

const palette = (tok) => ({
  surface: rgbOf(tok['--surface']), section: rgbOf(tok['--surface-2']), body: rgbOf(tok['--surface-3']),
  rim: rgbOf(tok['--border-strong']), ink: rgbOf(tok['--ink']), inkDim: rgbOf(tok['--ink-dim']), accent: rgbOf(tok['--accent']),
});

/** The face at `scale`, every control at its declared default. */
export function renderFace(lay, tok, scale) {
  const g = GEOMETRY, col = palette(tok), s = scale;
  const c = canvas(Math.round(lay.w * s), Math.round(lay.h * s));
  roundRect(c, 0, 0, lay.w * s, lay.h * s, g.radius * s, col.rim);
  roundRect(c, s, s, (lay.w - 2) * s, (lay.h - 2) * s, (g.radius - 1) * s, col.surface);
  roundRect(c, s, (g.headH - 2) * s, (lay.w - 2) * s, 2 * s, 0, col.accent);
  disc(c, (lay.w - g.pad - 10) * s, (g.headH / 2) * s, 5 * s, col.accent);
  for (const sec of lay.sections) {
    roundRect(c, sec.x * s, sec.y * s, sec.w * s, sec.h * s, g.sectionRadius * s, col.section);
    for (const k of sec.controls) {
      const x = sec.x + k.x, y = sec.y + k.y;
      if (k.widget === 'switch') {
        drawSwitch(c, (x + (g.cellW - g.switchW) / 2) * s, (y + g.switchTop) * s, g.switchW * s, g.switchH * s, k.def >= 0.5, col);
      } else {
        drawKnob(c, (x + g.cellW / 2) * s, (y + g.knobTop + g.knob / 2) * s, (g.knob / 2) * s, travelFraction(k, k.def), col);
      }
    }
  }
  return encodePng(c);
}

/** MOD's film strip: `filmFrames` square frames left to right, the knob from minimum to maximum. */
export function renderFilm(tok) {
  const g = GEOMETRY, col = palette(tok), n = g.filmFrames;
  const c = canvas(g.knob * n, g.knob);
  for (let i = 0; i < n; i++) drawKnob(c, i * g.knob + g.knob / 2, g.knob / 2, g.knob / 2 - 1, i / (n - 1), col);
  return encodePng(c, g.knob);
}

// ------------------------------------------------------------------------------------- text

const banner = (lead, source) => [
  `${lead}SPDX-License-Identifier: GPL-3.0-or-later`,
  `${lead}Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>`,
  `${lead}GENERATED — DO NOT EDIT BY HAND. Produced by tools/modgui-gen.mjs from`,
  `${lead}${source}`,
  `${lead}Regenerate: \`node tools/modgui-gen.mjs\`, then commit the result.`,
];

const files = (stem) => ({
  template: `${MODGUI_DIR}/icon-${stem}.html`,
  stylesheet: `${MODGUI_DIR}/stylesheet-${stem}.css`,
  screenshot: `${MODGUI_DIR}/screenshot-${stem}.png`,
  thumbnail: `${MODGUI_DIR}/thumbnail-${stem}.png`,
  film: `${MODGUI_DIR}/knobs/omx-knob.png`,
});

/** `modgui.ttl`: the gui resource, and `modgui:port` for every control of the FIRST section;
 * a modgui:port's lv2:index is its order on the pedal, its symbol and name the LV2 port's. */
export function emitModguiTtl(panel, source) {
  const f = files(panel.stem);
  const ports = panel.sections[0].controls
    .map((k, i) => `            lv2:index ${i} ;\n            lv2:symbol "${k.symbol}" ;\n            lv2:name ${ttlStr(k.name)} ;\n`)
    .join('        ] , [\n');
  return `${banner('# ', source).join('\n')}
@prefix lv2:    <http://lv2plug.in/ns/lv2core#> .
@prefix modgui: <http://moddevices.com/ns/modgui#> .

<${panel.uri}>
    modgui:gui [
        modgui:resourcesDirectory <${MODGUI_DIR}> ;
        modgui:iconTemplate <${f.template}> ;
        modgui:stylesheet <${f.stylesheet}> ;
        modgui:screenshot <${f.screenshot}> ;
        modgui:thumbnail <${f.thumbnail}> ;
        modgui:brand ${ttlStr(panel.brand)} ;
        modgui:label ${ttlStr(panel.name)} ;
        modgui:port [
${ports}        ] ;
    ] .
`;
}

/** The icon template, in MOD's documented format: every section and every control, laid out. */
export function emitTemplate(panel, lay, source) {
  const g = GEOMETRY;
  const control = (k) => {
    const data = `data-symbol="${k.symbol}" data-min="${k.min}" data-max="${k.max}" data-default="${k.def}"`;
    const image = k.widget === 'switch'
      ? `<div class="omx-switch-image" mod-role="input-control-port" mod-port-symbol="${k.symbol}" mod-widget="switch"></div>`
      : `<div class="omx-knob-image" mod-role="input-control-port" mod-port-symbol="${k.symbol}"></div>`;
    return `            <div class="omx-control omx-${k.widget}" style="left:${k.x}px;top:${k.y}px" title="${esc(k.name)}" ${data}>
                ${image}
                <span class="omx-control-title">${esc(k.name)}</span>
            </div>`;
  };
  const sections = lay.sections.map((s) => `        <div class="omx-section" data-section="${s.key}" style="left:${s.x}px;top:${s.y}px;width:${s.w}px;height:${s.h}px">
            <h2 class="omx-section-title">${esc(s.label)}</h2>
${s.controls.map(control).join('\n')}
        </div>`).join('\n');
  const jacks = (dir) => ['audio', 'midi', 'cv'].map((kind) => `        {{#effect.ports.${kind}.${dir}}}
        <div class="mod-${dir} mod-${dir}-disconnected" title="{{name}}" mod-role="${dir}-${kind}-port" mod-port-symbol="{{symbol}}">
            <div class="mod-pedal-${dir}-image"></div>
        </div>
        {{/effect.ports.${kind}.${dir}}}`).join('\n');
  return `<div class="omx-pedal{{{cns}}} omx-family-${panel.family}" style="width:${lay.w}px;height:${lay.h}px">
${banner('    <!-- ', source).map((l) => `${l} -->`).join('\n')}
    <!-- Template format: MOD Devices' modgui (mod-ui, https://github.com/moddevices/mod-ui, and the
         MOD SDK's icon-template documentation) — Mustache, mod-role / mod-port-symbol, the {{{cns}}}
         and {{{ns}}} scoping, MOD's own drag-handle, bypass and jack roles. Credited, not copied. -->
    <div mod-role="drag-handle" class="mod-drag-handle"></div>
    <div class="omx-head" style="height:${g.headH}px">
        <span class="omx-brand">{{brand}}</span>
        <span class="omx-label">{{label}}</span>
        <div class="omx-bypass" mod-role="bypass"><div class="omx-bypass-light" mod-role="bypass-light"></div></div>
    </div>
${sections}
    <div class="mod-pedal-input">
${jacks('input')}
    </div>
    <div class="mod-pedal-output">
${jacks('output')}
    </div>
</div>
`;
}

/** The stylesheet: the GEOMETRY and the look's resolved tokens, scoped by MOD's {{{cns}}}. */
export function emitStylesheet(panel, look, source) {
  const g = GEOMETRY, f = files(panel.stem), tok = look.tokens;
  const P = '.omx-pedal{{{cns}}}';
  const film = `/resources/${relative(MODGUI_DIR, f.film)}{{{ns}}}`;
  return `/*
${banner(' * ', source).join('\n')}
 * Colours and font: the ${look.theme} theme's "${look.personality}" look of openmixer's web-ui tokens.css, resolved in tools/look.json.
 */
${P} { position: relative; box-sizing: border-box; background: ${tok['--surface']}; border: 1px solid ${tok['--border-strong']}; border-radius: ${g.radius}px; color: ${tok['--ink']}; font-family: ${tok['--font-ui']}; }
${P} .omx-head { position: absolute; left: 0; right: 0; top: 0; border-bottom: 2px solid ${tok['--accent']}; }
${P} .omx-brand { position: absolute; left: ${g.pad}px; top: 5px; font-size: 10px; letter-spacing: 0.08em; text-transform: uppercase; color: ${tok['--ink-dim']}; }
${P} .omx-label { position: absolute; left: ${g.pad}px; top: 17px; font-size: 14px; font-weight: 600; }
${P} .omx-bypass { position: absolute; right: ${g.pad}px; top: ${g.headH / 2 - 10}px; width: 20px; height: 20px; cursor: pointer; }
${P} .omx-bypass-light { width: 10px; height: 10px; margin: 5px; border-radius: 50%; background: ${tok['--surface-3']}; }
${P} .omx-bypass-light.on { background: ${tok['--accent']}; }
${P} .omx-section { position: absolute; box-sizing: border-box; background: ${tok['--surface-2']}; border-radius: ${g.sectionRadius}px; }
${P} .omx-section-title { margin: 0; padding: 3px ${g.sectionPad}px; height: ${g.sectionTitleH}px; box-sizing: border-box; font-size: 10px; font-weight: 600; text-transform: uppercase; color: ${tok['--ink-dim']}; }
${P} .omx-control { position: absolute; width: ${g.cellW}px; height: ${g.cellH}px; text-align: center; }
${P} .omx-knob-image { width: ${g.knob}px; height: ${g.knob}px; margin: ${g.knobTop}px auto 0; background-image: url(${film}); background-repeat: no-repeat; background-size: auto ${g.knob}px; cursor: pointer; }
${P} .omx-switch-image { position: relative; width: ${g.switchW}px; height: ${g.switchH}px; margin: ${g.switchTop}px auto ${g.knob + g.knobTop - g.switchTop - g.switchH}px; border-radius: ${g.switchH / 2}px; background: ${tok['--surface-3']}; cursor: pointer; }
${P} .omx-switch-image::after { content: ''; position: absolute; top: 3px; left: 3px; width: ${g.switchH - 6}px; height: ${g.switchH - 6}px; border-radius: 50%; background: ${tok['--ink-dim']}; }
${P} .omx-switch-image.on { background: ${tok['--accent']}; }
${P} .omx-switch-image.on::after { left: ${g.switchW - g.switchH + 3}px; background: ${tok['--ink']}; }
${P} .omx-control-title { display: block; margin-top: 6px; font-size: 10px; color: ${tok['--ink-dim']}; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
`;
}

/** Every file of a declaration's modgui, as [path relative to the LV2 bundle, bytes], in a fixed order. */
export function generateModgui(d, look = loadLook()) {
  const panel = resolvePanel(d);
  const lay = layout(panel);
  const source = `plugins/${d.stem}/${d.stem}.decl.json (its panel block) and tools/look.json`;
  const f = files(panel.stem);
  return [
    [MODGUI_TTL, Buffer.from(emitModguiTtl(panel, source))],
    [f.template, Buffer.from(emitTemplate(panel, lay, source))],
    [f.stylesheet, Buffer.from(emitStylesheet(panel, look, source))],
    [f.screenshot, renderFace(lay, look.tokens, 1)],
    [f.thumbnail, renderFace(lay, look.tokens, GEOMETRY.thumbScale)],
    [f.film, renderFilm(look.tokens)],
  ];
}

/** The LV2 bundle directory a declaration's modgui is written into. */
export const bundleDir = (d) => join(d.dir, 'generated', `${d.stem}.lv2`);

/** Every file under `dir`, relative to `base`, sorted. */
function walk(dir, base) {
  if (!existsSync(dir)) return [];
  return readdirSync(dir, { withFileTypes: true }).flatMap((e) =>
    e.isDirectory() ? walk(join(dir, e.name), base) : [relative(base, join(dir, e.name))]).sort();
}

function main(argv) {
  const check = argv.includes('--check');
  const toStdout = argv.includes('--emit-stdout');
  const dirs = argv.filter((a) => !a.startsWith('--'));
  const look = loadLook();
  let stale = 0;
  for (const pdir of dirs.length ? dirs : pluginDirs()) {
    const d = loadDecl(pdir);
    if (!d.panel) continue;
    const dir = bundleDir(d);
    const shown = (p) => relative(process.cwd(), join(dir, p));
    const out = generateModgui(d, look);
    if (toStdout) {
      for (const [p, bytes] of out) {
        if (p.endsWith('.png')) process.stdout.write(`# ${shown(p)}: ${bytes.length} bytes, sha256 ${createHash('sha256').update(bytes).digest('hex')}\n`);
        else process.stdout.write(bytes);
      }
    } else if (check) {
      for (const [p, bytes] of out) {
        const at = join(dir, p);
        if (!existsSync(at) || !readFileSync(at).equals(bytes)) {
          console.error(`modgui: STALE ${shown(p)} (run \`node tools/modgui-gen.mjs\`)`);
          stale++;
        }
      }
      const want = new Set(out.map(([p]) => p));
      for (const p of walk(join(dir, MODGUI_DIR), dir)) {
        if (!want.has(p)) {
          console.error(`modgui: EXTRA ${shown(p)} is not generated — a stale modgui file; delete it`);
          stale++;
        }
      }
    } else {
      rmSync(join(dir, MODGUI_DIR), { recursive: true, force: true });
      for (const [p, bytes] of out) {
        mkdirSync(dirname(join(dir, p)), { recursive: true });
        writeFileSync(join(dir, p), bytes);
        console.log(`modgui: wrote ${shown(p)}`);
      }
    }
  }
  if (check) {
    if (stale) process.exit(1);
    console.log('modgui: every generated file is fresh');
  }
}

if (process.argv[1] && resolve(process.argv[1]) === fileURLToPath(import.meta.url)) main(process.argv.slice(2));
