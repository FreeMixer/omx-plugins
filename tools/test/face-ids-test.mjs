#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * face-ids-test.mjs — the controls that are the face's own (the host's bypass, LV2 enabled and latency) sit at
 * omx-contract's permanent ids (OMX_PLUGIN_FACE_IDS), in every plugin, and appending a parameter moves none of them.
 * Until 0.19.0 the bypass took the CLAP id after the last parameter and enabled and latency the LV2 ports after the
 * controls, so every appended control moved them, and a host that kept bypass automation by id drove another control.
 *
 *   1. EVERY PLUGIN. For each shipped declaration, the LV2 port list (tools/gen.mjs lv2Ports), the generated header's
 *      OMX_<K>_LV2_PORT_ENABLED / _LATENCY and the generated TTL's ports put enabled and latency at the contract's
 *      indices; the built CLAP face's bypass id is held by tools/clap-params-check.mjs against the same constant.
 *   2. APPEND. A parameter appended to a declaration (and a sidechain added to one) leaves enabled and latency where
 *      they were, and every port that was there keeps its symbol's index, so a host that stored an index still finds it.
 *   3. SYMBOLS. A preset or a session names a port by its symbol: the set of symbols is the old set, the order of the
 *      declared controls among themselves unchanged.
 * `--self-test`: the rule this replaced (enabled and latency after the controls, the bypass the next CLAP id) is red
 * under part 2, so the check cannot pass a face that moves them.
 *
 * `make face-ids`, part of `make test`. Needs the installed omx-contract (>= 3.9.0). Exit 1 on any failure.
 */
import { readFileSync } from 'node:fs';
import { join } from 'node:path';
import { ROOT, loadDecl, lv2Ports, pluginDirs } from '../gen.mjs';
import { faceIds } from '../param-contract.mjs';

const ids = faceIds();
let failed = 0;
const say = (ok, m) => {
  if (!ok) failed++;
  console.log(`${ok ? 'PASS' : 'FAIL'} face-ids ${m}`);
};

/** The rule this replaced: audio, the controls, enabled, latency; the bypass the CLAP id after the last control. */
function oldLv2Ports(d) {
  const audio = ['in_l', 'in_r', ...(d.sidechain ? [d.sidechain.symbol] : []), 'out_l', 'out_r'].map((symbol) => ({ symbol }));
  return [...audio, ...d.params.map((p) => ({ symbol: p.symbol, param: p })), { symbol: 'enabled', enabled: true }, { symbol: 'latency', latency: true }]
    .map((p, index) => ({ ...p, index }));
}
const oldBypassId = (d) => d.params.length;
const newBypassId = () => ids.bypassClapId;

const indexOf = (ports, symbol) => ports.find((p) => p.symbol === symbol)?.index;

/** Part 2 over one rule: what moved when `d` grew into `grown`, as messages (none when nothing moved). */
function moved(d, grown, portsOf, bypassOf) {
  const out = [];
  const a = portsOf(d), b = portsOf(grown);
  for (const s of ['enabled', 'latency']) if (indexOf(a, s) !== indexOf(b, s)) out.push(`${s} moved ${indexOf(a, s)} -> ${indexOf(b, s)}`);
  for (const p of a) if (indexOf(b, p.symbol) !== p.index && !['enabled', 'latency'].includes(p.symbol) && !grown.sidechain) out.push(`${p.symbol} moved ${p.index} -> ${indexOf(b, p.symbol)}`);
  if (bypassOf(d) !== bypassOf(grown)) out.push(`the CLAP bypass id moved ${bypassOf(d)} -> ${bypassOf(grown)}`);
  return out;
}

/** `d` with one more parameter, a copy of its last under a new symbol. */
const appended = (d) => ({ ...d, params: [...d.params, { ...d.params.at(-1), symbol: 'appendedProbe', name: 'Appended Probe' }] });
/** `d` with a sidechain key, when it has none. */
const keyed = (d) => (d.sidechain ? d : { ...d, sidechain: { symbol: 'key_in', name: 'Key In' } });

const selfTest = process.argv.includes('--self-test');
const decls = pluginDirs(ROOT).map((dir) => loadDecl(dir));

if (selfTest) {
  for (const d of decls) {
    const m = moved(d, appended(d), oldLv2Ports, oldBypassId);
    say(m.some((x) => x.startsWith('enabled moved')) && m.some((x) => x.startsWith('latency moved')) && m.some((x) => x.startsWith('the CLAP bypass id moved')),
      `self-test ${d.stem}: the old rule is red on an appended parameter (${m.slice(0, 3).join('; ')})`);
  }
} else {
  for (const d of decls) {
    const K = d.kernel.toUpperCase();
    const ports = lv2Ports(d);
    say(indexOf(ports, 'enabled') === ids.enabledLv2Index && indexOf(ports, 'latency') === ids.latencyLv2Index,
      `${d.stem}: enabled at ${indexOf(ports, 'enabled')}, latency at ${indexOf(ports, 'latency')} (contract ${ids.enabledLv2Index}, ${ids.latencyLv2Index})`);
    const dir = join(ROOT, 'plugins', d.stem);
    const header = readFileSync(join(dir, 'generated', `omx_${d.kernel}_params.h`), 'utf8');
    const def = (n) => Number(header.match(new RegExp(`^#define OMX_${K}_LV2_PORT_${n} (\\d+)u$`, 'm'))?.[1]);
    say(def('ENABLED') === ids.enabledLv2Index && def('LATENCY') === ids.latencyLv2Index, `${d.stem}: the generated header's LV2 enabled and latency are the contract's`);
    const ttl = readFileSync(join(dir, 'generated', `${d.stem}.lv2`, `${d.stem}.ttl`), 'utf8');
    const at = (symbol) => Number(ttl.match(new RegExp(`lv2:index (\\d+) ;\\s*lv2:symbol "${symbol}"`))?.[1]);
    say(at('enabled') === ids.enabledLv2Index && at('latency') === ids.latencyLv2Index, `${d.stem}: the generated TTL's enabled and latency are the contract's`);
    for (const [what, grown] of [['an appended parameter', appended(d)], ['an added sidechain', keyed(d)], ['both', keyed(appended(d))]]) {
      const m = moved(d, grown, lv2Ports, newBypassId);
      say(m.length === 0, `${d.stem}: ${what} moves no face id${m.length ? `: ${m.join('; ')}` : ''}`);
    }
    const old = oldLv2Ports(d).map((p) => p.symbol), now = ports.map((p) => p.symbol);
    const controls = (list) => list.filter((s) => d.params.some((p) => p.symbol === s));
    say([...old].sort().join() === [...now].sort().join() && controls(old).join() === controls(now).join(),
      `${d.stem}: the same port symbols as before, the controls in their order`);
  }
}
console.log(failed ? `face-ids: ${failed} failed` : `face-ids: ${decls.length} plugins${selfTest ? ' (self-test)' : ''}, all held`);
process.exit(failed ? 1 : 0);
