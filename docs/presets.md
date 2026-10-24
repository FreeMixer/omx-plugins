<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
<!-- Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com> -->
# Factory presets in every host

A plugin's factory presets are useful only if a host can list and load them. They are files,
`presets/<plugin id>/<preset id>.json` (`org.openmixer.preset/1`), installed read only under
`/usr/share/omx/presets/`. A host reads presets through its plugin format, so each face serves the
same files the way its format asks (plugin-hostd#131, 'PRESETS IN THE HOST').

## One source

The JSON files are the one source. `tools/gen-presets.mjs` writes them and, from them, each LV2
bundle's `presets.ttl`; `tools/gen.mjs` lists the same presets in the bundle's `manifest.ttl`.
Nothing about a preset is kept by hand, and `make presets` fails on a stale or missing file.

A preset carries parameters only (`"carries": "params"`): no plugin here needs a state body.

## CLAP

Every CLAP face exports, beside its plugin factory:

- `clap.preset-discovery-factory/2` (and the `/draft-2` id older hosts ask for), one provider,
  `<clap id>.presets`. Its `init` declares the file type `json` (`OpenMixer preset`), the soundpack
  `<clap id>` (factory content) and the location `/usr/share/omx/presets/<clap id>/`, of kind
  file, factory content, in that soundpack. `get_metadata` reads one file of that location and
  reports one preset: the file's `name`, no load key, the plugin id `clap:<clap id>`, the
  soundpack and the factory flag. A file that names another plugin or is no preset reports none.
- `clap.preset-load` (and `clap.preset-load.draft/2`): `from_location` of kind file reads the
  file and applies each of its `params` as `clap.state`'s load does: a known symbol's value
  clamped to the parameter's declared range, an unknown symbol ignored, a parameter the file does
  not name left as it was. It then asks the host to read the values again (`clap.params`
  rescan) and tells the host's `clap.preset-load` (`loaded`, or `on_error` for a file it cannot
  read). It refuses while processing, as the state load does.

The JSON reader is `include/omx_preset_file.h`, shared by every face.

## LV2

Every bundle carries `presets.ttl`: one `pset:Preset` per file, `<lv2 uri>#preset-<preset id>`,
with `rdfs:label` (the name), `lv2:appliesTo` (the plugin) and one `lv2:port [ lv2:symbol;
pset:value ]` per parameter of the file. The manifest types each preset and points to the file,
so lilv lists them without loading the plugin's description.

## Tests

`make test` of every plugin runs `tools/preset-probe` over the built faces and
`tools/test/presets-host-check.mjs` over what it printed, against the presets `tools/gen-presets.mjs`
builds:

- the discovery factory declares the location above, and its provider, crawling that location
  (the probe maps `/usr/share/omx/presets` to the tree's `presets/`), lists exactly the plugin's
  presets, by name and plugin id;
- each preset, loaded through `clap.preset-load` and read back through `clap.params`, gives the
  file's values exactly (as the plugin's float holds them);
- lilv lists the bundle's presets, the same count, each with the file's values.

`--self-test` drops a file from a copy of the directory, changes a value in another copy, drops a
preset from a copy of the bundle's manifest and changes a value in a copy of its `presets.ttl`, and
each goes red.

A new plugin has no presets until they are declared in `tools/gen-presets.mjs` (`DEFINITIONS`, by
the plugin's CLAP id) and generated (`node tools/gen-presets.mjs`, then `node tools/gen.mjs
plugins/<stem>` for the bundle's manifest); until then `tools/gen-presets.mjs` refuses the tree and
the plugin's `make test` is red, naming the plugin.
