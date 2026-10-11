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
make test                 # every plugin's tests
make install DESTDIR=/tmp/stage LIBDIR=/usr/lib64   # <CLAPDIR>/*.clap (default /usr/lib/clap, any LIBDIR), <LIBDIR>/lv2/*.lv2/
make version              # the release version
```

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
6. **host check**: `omx-clap-scan` loads the CLAP file, `lv2ls`/`lv2info` read the LV2 bundle.

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
include/                  shared C shapes (no DSP)
tools/                    gen.mjs (header + TTL), modgui-gen.mjs (MOD GUI), the test helpers,
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
parameters), the RPM `%files` lists and CI's installed-file lists. The package descriptions are
prose; the completeness test checks they name every shipped plugin. The older plugins (delay,
drive, the EQs, strip, keyed gate) keep their hand-written faces and bindings.

`make completeness` holds every plugin to the same recipe and names the wizard step for each gap;
`node tools/commit-plan-check.mjs <base>..<head>` holds a plugin's commits to the plan's order.

A directory with a declaration and no Makefile (omx-chorus and omx-deesser today) is not built or
packaged yet: `make completeness` lists what it still needs. `node tools/gen.mjs --check` still
keeps its generated files fresh.

## Packages

`packaging/omx-plugins.spec` builds `omx-plugins-clap`, `omx-plugins-lv2` and the `omx-plugins`
metapackage that requires both, and runs `make test` in `%check`; `debian/` builds the same three
packages for bookworm and trixie and runs `make test` unless `nocheck` is set. The version lives in three places that CI keeps equal: the top-level
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
