<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
<!-- Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com> -->
# The declaration as a view of the descriptor

A plugin of ours is described in omx-contract's words, like anyone's (omx-contract `docs/descriptors.md`):
`plugins/<stem>/<stem>.descriptor.json` is its `authored` descriptor. `plugins/<stem>/<stem>.decl.json` keeps only
what the descriptor has no word for and only this repository needs: how the faces are built, the presets'
source, the panel's hints, the docs and the console's placement. `schema/plugin.decl.schema.json` is the shape
of the declaration a generator reads, the descriptor's view with those extras folded in; for a described plugin
the extras file carries only the extras, and `viewOf` (`tools/descriptor.mjs`) refuses a field the descriptor
says (the identity, the parameters, the CLAP features, the LV2 class, the description).

`tools/gen.mjs`'s `loadDecl` reads the two and gives every generator the declaration it gave before, so the
headers, both faces, the panel description, the MOD GUI and its rasters do not change by a byte when a
plugin moves to a descriptor (`node tools/gen.mjs --check`, `node tools/modgui-gen.mjs --check`,
`node tools/look.mjs --check`, `make panel-layout`).

A link carries the release it is read from, `at`: `omx-contract@<floor>`, the packaging's floor.

## Where a host reads it

`make install-devel` (through `make install-contract`) installs each shipped plugin's descriptor, byte for byte,
into omx-contract's contract directory as `<contractdir>/3/omx-plugins/plugins/<stem>.json`, with a `MANIFEST.json`
naming the release (omx-contract `docs/contract-dir.md`); the -devel and -dev packages carry it. A host reads it
there through omx-contract's reader, never from this tree. `make contract-dir` holds the installed tree to the
plugin folders' descriptors, one per shipped plugin that has one, and reads it back through the reader.

## No number is typed

A control's travel is still read by reference. Its `source.from` is the link into the kernel's definition,
`data/kernels/<kernel>.json#<NAME>` (a field: `#<NAME>.<field>`), or into omx-contract's shared files
(`data/primitives.json#<NAME>`). The descriptor states the travel the link resolves to, because the
descriptor words always state it; `node tools/gen.mjs` writes those numbers from the link, and
`node tools/gen.mjs --check` refuses a descriptor whose stated travel, default, unit or choice is not what its
link resolves to. A number edited by hand is stale, never a declaration.

## Where each declaration field goes

| Declaration field | Descriptor word | Or the extra, in `<stem>.decl.json` |
|---|---|---|
| `name`, `vendor`, `version` | `plugin.name`, `.vendor`, `.version` | |
| `clap.id` | `plugin.id` (`format: clap`) | |
| `lv2.uri` | `plugin.uri` | |
| `clap.features` | `plugin.features`: the four the descriptor names (`audio-effect`, `instrument`, `note-effect`, `analyzer`); CLAP's other features (`delay`, `stereo`, ...) in the extension bag, `extensions.clap["clap_plugin_descriptor.features"]`, in the face's order | |
| `lv2.class` | the extension bag, `extensions.lv2["rdf:type"]`: the face's types as its `.ttl` states them, `["lv2:Plugin", <class>]` (`"lv2:Plugin"` alone when that is the class) | |
| `lv2.enabledPort` | | kept: how the LV2 face is written |
| `description` | `plugin.doc`: the one-line description, CLAP `description` and LV2 `rdfs:comment` | |
| `summary`, `noun`, `manual`, `url` | | kept: what a host and the README show |
| `stem`, `kernel`, `kernels`, `binding`, `face`, `variants`, `variantOf`, `chain`, `order`, `key`, `shipped`, `shippedDiff`, `role` | | kept: the kind of face and how it is built |
| `sidechain` (`symbol`, `name`) | a `ports[]` entry: `kind: audio`, `direction: in`, `role: sidechain`, `channels: 1`, `layout: mono`, named as the CLAP face names it (the symbol) | `sidechain`, whole: the LV2 port's symbol and name |
| the audio pair | `ports[]`: `in` and `out` (the CLAP face's names), `role: main`, `channels: 2`, `layout: stereo` | |
| `latencyChecks` | `latency.reports: true`; `latency.frames` when every check reports one constant | kept: the checks the oracles run |
| `oracles`, `portHints`, `meters` | | kept: tests, the LV2 hint pin, the clap.meters/1 rows |
| `panel.sections[].label` | each listed control's `group` | `panel` (family, roles, section keys, order, widgets, graphs): the panel's hints |
| `panel.roles` | each bound control's `meaning`, where the role has a meaning word (docs/panel-layout.md) | kept: the roles themselves |
| `console` | | kept: the console's placement, card and chip |
| `params[i]` | `controls[i]`, `id: i` | |
| `params[].symbol` | `controls[].symbol` | |
| `params[].name` | `controls[].name` | |
| `params[].ref`, `.field`, `.kernel` | `controls[].source.from`: `data/kernels/<kernel>.json#<ref>[.<field>]` | |
| resolved `min`, `max`, `def`, `unit`, `kind` | `kind` (`travel`, `toggle`, `choice`), `travel.min`, `.max`, `.default`, `.unit`, `.step` (1 for an integer), or a toggle's `default`; `.unit` is `ratio` where the link resolves to `""` and the control is a ratio (n:1), the one word the descriptor states beyond its link | |
| `params[].scale` | `travel.scale` (left out of the declaration: `log` for Hz, else `linear`) | |
| resolved `values`, `points` | a `choice`: `ids` (each value as text), `labels`, `values` | |
| `params[].forKind`, `.defaultRef`, `.defaultId`, `.defaultBand`, `.perBand`, `.values` | the come-up value and the labels they give, stated | kept, by symbol (`controls`): how the value is read, not the value |
| resolved `byKind` | | kept as the read (gap 2) |
| a set's `unit` (`dB/oct`) | | kept, by symbol (gap 1) |
| `params[].own` (a face's switch, a stage order) | the control, its travel typed, `source.impl` naming the face | `own`, the reason, by symbol |

## The gaps

What the descriptor has no word for, found laying out our panels, and where each goes. A gap marked
"contract" is proposed to omx-contract as a schema minor; until it lands the extra stays here.

| # | Gap | Goes to | Until then |
|---|---|---|---|
| 1 | A choice's unit: our EQ slopes, 12 and 24 `dB/oct` | contract: an optional `unit` on a choice, and `dB/oct` among the descriptor units | the set's unit, an extra by symbol |
| 2 | The come-up value per kind of channel (a delay's mix fully wet on a return): the kernel files say it as `byKind` | contract: `travel.byKind`, the kernel word, on a descriptor travel | read through the link at load |
| 3 | Meanings for a rotor's speeds and speed switch, an LFO's shape, a drive's model, a reverb's algorithm, a de-esser's mode, a filter or band type, a band's on switch, a ping-pong switch | contract: `speed`, `shape`, `model`, `algorithm`, `mode`, `type`, `on`, `pingpong` | the declared roles bind them |
| 4 | A ratio's unit: LV2 has none, the item list has only `x` | contract: `ratio` (n:1) among the descriptor units (omx-contract 3.0.0); the LV2 face writes it as `urn:openmixer:unit#ratio` | done: omx-comp `ratio`, omx-deesser `deessRatio`, omx-keyed-gate `ratio` state `ratio` |
| 5 | What a meter measures | contract: `measurement` on a read-only control: `peak`, `rms`, `vu`, `ppm`, `lufs`, `gainReduction` | a LUFS reading is `lufs`, any other `null` |
| 6 | `dBu`, which param-contract serves | contract: `dBu` among the descriptor units | the unit stays on our declarations only |
| 7 | An xy pad (two controls drawn as one) | omx-plugins: a panel hint, `panel.widgets` naming the pair | none of ours draws one |
| 8 | A momentary control | none: a `trigger` is momentary | |
| 9 | CLAP's open features (`delay`, `stereo`) and the LV2 class | contract: the extension bag, `extensions` (omx-contract 3.0.0) | done: each descriptor's bag carries them |
| 10 | The plugin's one-line description (CLAP `description`, LV2 `rdfs:comment`) | contract: `plugin.doc` (omx-contract 3.0.0) | done |

## The check of our own faces

The authored descriptor is also what our built faces must serve. `tools/face-descriptor-check.mjs` builds
the descriptor each face states (the CLAP face through `clap_plugin_params` and its audio ports, the LV2
bundle through lilv, as a scan reads anyone's plugin) and compares it with the authored one, control by
control: id, symbol, name, kind, range, default, unit where the format carries one, the ports. Any
difference fails with `contract.descriptor.face-differs`, naming the plugin, the face and the control.
