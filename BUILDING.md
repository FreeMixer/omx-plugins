<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
<!-- Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com> -->
# Building omx-plugins

## Dependencies

| What | Fedora | Debian | Used for |
|---|---|---|---|
| omx-dsp ≥ 0.4.1 (`pkg-config omxdsp`, which must require the omx-contract `.github/pins.txt` names) | `omx-dsp-devel` (channel) | `libomxdsp-dev` (channel) | the DSP: every kernel lives there, none here |
| openmixer CLAP extensions | `omx-clap-core-devel` (channel) | `libomx-clap-core-dev` (channel) | `<omx-clap-host/omx_clap_ext.h>` |
| CLAP 1.2 headers | `clap-devel` | `clap-headers` (built from the pinned CLAP, see FreeMixer/.github) | the CLAP face |
| LV2 headers | `lv2-devel` | `lv2-dev` | the LV2 face |
| C compiler, make, pkg-config | `gcc make pkgconf-pkg-config` | `build-essential pkgconf` | |
| lilv | `lilv lilv-devel` | `lilv-utils liblilv-dev` | tests: the oracles' LV2 host, `lv2ls`, `lv2info` |
| Node.js ≥ 18 | `nodejs` | `nodejs` | tests and `make gen`: the generators |
| sord | `sord` | `sordi` | tests: `sord_validate` on the LV2 bundle |
| omx-clap-host | `omx-clap-host` (channel) | `omx-clap-host` (channel) | tests: `omx-clap-scan` (the check says SKIP when it is absent; CI and the package builds install it) |

The channel is FreeMixer's package repository: `https://freemixer.github.io/rpm/freemixer.repo` for
dnf, `https://freemixer.github.io/deb/debian/<release> ./` (key `https://freemixer.github.io/deb/freemixer.asc`)
for apt.

## Build, test, install

```sh
make                      # every plugin: plugins/<name>/build/
make test                 # every plugin's tests, then the static table's (make lib-test)
make install DESTDIR=/tmp/stage LIBDIR=/usr/lib64   # <CLAPDIR>/*.clap (default /usr/lib/clap, any LIBDIR), <LIBDIR>/lv2/*.lv2/
make install-devel DESTDIR=/tmp/stage LIBDIR=/usr/lib64   # headers, <LIBDIR>/libomxplugins.a, <LIBDIR>/pkgconfig/omxplugins.pc
make version              # the release version
```

`make` also builds `build/lib/libomxplugins.a`: every shipped plugin's CLAP face, compiled with
`-DOMX_CLAP_STATIC` (the face then defines only its `omx_clap_entry_<kernel>`, not CLAP's
`clap_entry`, so all of them link into one program), and `lib/omx_clap_static.c`, the table of those
entries by CLAP id that `include/omx_clap_static.h` declares. A host links it through
`pkg-config omxplugins` and opens any plugin in-process, with no `.clap` file and no `dlopen`.
`make lib-test` installs the devel files into `build/lib/stage`, builds
`tools/test/static-table-host.c` from the staged `omxplugins.pc` alone and runs it: every plugin's
CLAP id is in the table once, and each one is created through its factory, activated and run.

`make test` for a plugin runs, in order:

1. **freshness**: every generated file matches what its declaration generates today;
2. **parameters**: the built CLAP plugin's parameters, read through `clap_plugin_params`, are the
   declaration's, in order, with the host's bypass last;
3. **plugin probe** (`test/plugin-probe.c`): the CLAP and LV2 builds, run over a deterministic
   signal in uneven blocks with parameter changes, are bit-identical to the omx-dsp kernel called
   directly, at every declared rate, engaged, bypassed and re-activated;
4. **delay oracle** (`test/delay-oracle.c`): at 44.1, 48, 96 and 192 kHz, on both builds, the
   echoes, ping-pong, mix and bypass computed from the definition of a delay, not from omx-dsp;
5. **MOD GUI**: fresh, carrying every declared control with its travel and default, valid under
   `sord_validate`, and stale when the declaration moves;
6. **host check**: `omx-clap-scan` loads the CLAP file, `lv2ls`/`lv2info` read the LV2 bundle;
7. **port groups** (`tools/port-groups-probe.c`, `tools/port-groups-check.mjs`): the LV2 bundle,
   read through lilv, holds one `pg:Group` per panel section (its `lv2:symbol` the section's key, its
   `rdfs:label` the section's label), and each parameter a section lists carries `pg:group` naming
   it; no other port is grouped;
8. **panel description** (`tools/panel-probe.c`, `tools/panel-check.mjs`): each built face is opened
   as a host opens it (the `.clap` through `clap_entry`, the LV2 bundle through lilv) and its
   `org.openmixer.panel-description/1` read back: the family, the sections, every listed parameter as
   a control in its section and order, the roles and displays equal the declaration's panel, and the
   fingerprint is the SHA-256 of what it serves.
8. **CLAP extensions** (`tools/clap-ext-probe.c`, `tools/clap-ext-check.mjs`): the CLAP face's
   `org.openmixer.param-contract/1` (each parameter's symbol, unit, scale and step, version 1, the
   fingerprint of what it serves), its `clap.remote-controls` pages (each section's controls, eight
   a page) and its `clap.track-info`: under each placement, before and after activation, every
   parameter comes up at the contract's default for that kind of channel (a delay's or reverb's mix
   fully wet on a return), else its own, and a value the host set stays.

For omx-strip, steps 3 and 4 are one **strip oracle** (`test/strip-oracle.c`): at 44.1, 48, 96
and 192 kHz, on both builds, in each of the 24 stage orders and at the defaults, the output equals
omx-dsp's strip modules called in sequence by the oracle itself; bypass is the identity. omx-strip
has no MOD GUI yet (its declaration carries no panel).

## Layout

```
plugins/<name>/
  <name>.decl.json        THE declaration: parameters, identity, panel
  omx_<kernel>_clap.c     the CLAP face, plain C
  omx_<kernel>_lv2.c      the LV2 face, plain C
  generated/              written by `make gen`, committed: the parameter header and the LV2 bundle
  test/                   the probe and the oracle
include/                  shared C shapes (no DSP): omx_panel_description.h and omx_param_contract.h,
                          the openmixer extensions and their rows; omx_clap_static.h, libomxplugins' table
lib/                      omx_clap_static.c (the table's rows are generated) and omxplugins.pc.in
tools/                    gen.mjs (header + TTL), panel-description.mjs and param-contract.mjs (the
                          extensions' rows),
                          modgui-gen.mjs (MOD GUI), the test helpers,
                          omx-new-plugin.mjs (the wizard), plugin-recipe.mjs (completeness)
recipes/                  plugin.recipe.json, the templates tools/gen.mjs writes from
schema/                   plugin.decl.schema.json, the shape of a declaration
```

## Changing a plugin

Edit the declaration, run `make -C plugins/<name> gen`, commit the declaration and `generated/`
together. Parameter order is append-only: hosts save sessions by parameter id. DSP changes go to
omx-dsp, never here.

## Adding a plugin

A plugin over an omx-dsp effect kernel starts with the wizard, never by hand
([docs/new-plugin-from-contract.md](docs/new-plugin-from-contract.md)):

```sh
node tools/omx-new-plugin.mjs --from-contract <kernel>   # drafts plugins/omx-<kernel>/omx-<kernel>.decl.json
$EDITOR plugins/omx-<kernel>/omx-<kernel>.decl.json      # settle every REVIEW mark
node tools/omx-new-plugin.mjs --from-contract <kernel>   # validates, generates everything, runs make test
```

The declaration is the folder's one hand-written file. Every parameter names its travel BY
REFERENCE into the kernel's file in omx-contract (`"ref"`, `"field"`), at the release `omx-contract`
names in `.github/pins.txt`; no number is typed in a declaration. The release is read through its
JSON render from `$OMX_CONTRACT_DIR`, a checkout of omx-contract beside this one, or the release
tarball (fetched once into `build/omx-contract-<version>/`, which needs `curl`). The kernel must
have its instance face in omx-dsp (`<omxdsp/fx/omx_<kernel>_instance.h>`): each parameter binds to
the `resolve()` argument named after its contract control, and `"binding": "instance"` makes
`tools/gen.mjs` write the binding, the kernel-identity test, both faces and the Makefile. A kernel
missing from omx-contract, or without a face of that shape, is refused with the work named.

`tools/pins-check.sh` holds the packaging's requirement on omx-dsp, and the omx-contract the
installed omx-dsp requires, equal to `.github/pins.txt`.

`node tools/gen.mjs` also refills the shared files' generated regions from every plugin folder: the
README's catalogue and sections (a declaration's `summary` and `manual`, else its description and
parameters), the RPM `%files` lists, CI's installed-file lists and libomxplugins' table of CLAP
entries. The package descriptions are
prose; the completeness test checks they name every shipped plugin. The older plugins (delay,
drive, the EQs, strip, keyed gate) keep their hand-written faces and bindings.

`make completeness` holds every plugin to the same recipe and names the wizard step for each gap;
`node tools/commit-plan-check.mjs <base>..<head>` holds a plugin's commits to the plan's order.

A directory with a declaration and no Makefile (omx-chorus and omx-deesser today) is not built or
packaged yet: `make completeness` lists what it still needs. `node tools/gen.mjs --check` still
keeps its generated files fresh.

## Packages

`packaging/omx-plugins.spec` builds `omx-plugins-clap`, `omx-plugins-lv2`, the `omx-plugins`
metapackage that requires both, and `omx-plugins-devel` (the headers, `libomxplugins.a` and
`omxplugins.pc`), and runs `make test` in `%check`; `debian/` builds the same packages for bookworm
and trixie (`omx-plugins-dev`, with the archive in the multiarch directory) and runs `make test`
unless `nocheck` is set. The version lives in three places that CI keeps equal: the top-level
`Makefile` (`make version`), the spec's `Version:` and the top of `debian/changelog`.

A `v<version>` tag runs `.github/workflows/release.yml`: RPMs for Fedora 44 (x86_64, aarch64)
and DEBs for bookworm and trixie (amd64, arm64), signed and published into the FreeMixer package
channel and attached to the GitHub release. Beside them goes
`omx-plugins-zynthian-<version>-arm64.tar.gz`: the files of the bookworm arm64 `omx-plugins-lv2` and
`omx-plugins-clap` packages, repacked by `tools/zynthian-bundle.sh` as `lv2/` and `clap/` directories for
a Zynthian's plugin directories, after checking every ELF (architecture, glibc no newer than bookworm's
2.36, no library a bare bookworm lacks, `clap_entry` in every CLAP) and running every LV2 plugin
through `lv2bench` on arm64. Pull requests run the same workflow as a dry run that signs and publishes
nothing; there the tarball is packed and checked from the amd64 build.
