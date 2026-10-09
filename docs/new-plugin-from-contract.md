<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
<!-- Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com> -->
# A new plugin from its contract

How a plugin over an omx-dsp effect kernel is born, and what a person writes. It follows
openmixer's spec `2026-10-09-plugin-from-contract.md` (§3, §5, §6, §7, §8, §10).

## The inputs

| Input | Where | Who writes it |
|---|---|---|
| The kernel's controls: names, travels, choices | omx-contract, read only through its JSON render (`share/omx-contract/omx-contract.json`) at the pin | omx-contract |
| The kernel's instance face: `init`, `resolve`, `run`, latency | omx-dsp, `<omxdsp/fx/omx_<kernel>_instance.h>` | omx-dsp |
| The plugin: identity, design choices, `panel`, `console` | `plugins/<stem>/<stem>.decl.json`, the folder's ONE hand-written file | a person, from the wizard's draft |

There is no answers file. The declaration is the wizard's input and its output.

## Four commands

```sh
node tools/omx-new-plugin.mjs --from-contract transient   # 1. drafts plugins/omx-transient/omx-transient.decl.json
$EDITOR plugins/omx-transient/omx-transient.decl.json     # 2. a person settles every REVIEW mark
node tools/omx-new-plugin.mjs --from-contract transient   # 3. validates, generates everything, runs make test
git commit ...                                            # 4. the commit plan it printed, one concern per commit
```

Step 1 checks before it writes:

- the kernel is in omx-contract at the pin (else: the omx-contract work, `tools/new-kernel.mjs`, is
  named and nothing is written);
- omx-dsp has the kernel's instance face, in the shape the generated binding calls (`init(s, sr)`,
  `resolve(s, bypass, <one scalar per control>)`, `run(s, in_l, in_r, out_l, out_r, n)`); a kernel
  without one, or with a face of another shape (ring buffers handed in, a ports struct), is refused
  with the omx-dsp work named;
- every `resolve` argument binds to exactly one contract control BY NAME: the argument is the
  control's name in snake case (`attackDb` → `attack_db`). An argument that still carries an older
  short name (`attack_ms` for `attackTimeMs`) binds when its words are, in order, a unique subset
  of the control's; the generated binding then lists it as a rename owed by omx-dsp's name
  conformance (spec §2), and `OMX_BINDING_STRICT=1` refuses it.

## What is drafted, what a person writes

| Field | Drafted as | A person |
|---|---|---|
| `stem`, `kernel`, `kernels` | `omx-<kernel>` (underscores become hyphens), the kernel | may rename the stem before the first release |
| `name`, `vendor`, `url`, `version`, `clap.id`, `lv2.uri`, `$comment` | derived (recipe `derived`), refused if changed | — |
| `binding` | `instance`: the binding, the oracle, both faces and the Makefile are generated | — |
| `params` | one per `resolve` argument, in its order, each BY REFERENCE (`ref`, `field`), `symbol` = the control's name | — |
| each parameter's `name` | the control's words, its unit suffix dropped (`attackTimeMs` → "Attack Time") | reviews |
| a choice's `values` | the set's labels when the contract has them, else `REVIEW` | writes the labels |
| `description` | `REVIEW` | one plain sentence a host shows |
| `lv2.class`, `clap.features` | `REVIEW`, and `audio-effect`, `stereo` | the LV2 class and the CLAP kind |
| `summary`, `manual` | left out: the README row reads `description`, the section is the parameter table | optional prose |
| `panel`, `console` | left out | written when the console draws the plugin (spec §8) |

Step 3 refuses while any `REVIEW` mark is left, naming each by its JSON pointer. When the draft is
settled it fills the derived fields and runs `tools/gen.mjs`, which writes:

- per plugin: `generated/` (parameter header, LV2 bundle, MOD GUI when there is a `panel`), the
  `Makefile`, `omx_<kernel>_clap.c`, `omx_<kernel>_lv2.c`, the binding `omx_<kernel>_core.h` and
  the kernel-identity test `test/<kernel>-oracle.c`, each with a GENERATED banner;
- across plugins, from the folders: the README's catalogue and sections, the RPM `%files` lists and
  CI's installed-file lists, each between `BEGIN GENERATED` and `END GENERATED` lines.

`node tools/gen.mjs --check` holds every one of them; CI runs it. Then the wizard runs the plugin's
`make test` and `make completeness` for it, and prints the commit plan in the recipe's layer order.

## The identity test

One template, filled from the declaration: a deterministic stimulus in uneven blocks, every
parameter moving between blocks across its declared travel, the bypass toggled. The CLAP and LV2
builds must equal omx-dsp's instance face called directly, bit for bit, at every rate in
`OMX_DECLARED_RATES`, and publish its latency. Two guards keep it honest: the reference must move
the signal, and a reference rendered with one parameter one step off must differ from the faces
(the sabotage arm), so a test that could not see a wrong coefficient fails.

## What it does not do

Placement, the console's card and chip, and the panel's widgets are declared here and checked
against the vocabulary in `schema/plugin.decl.schema.json`; the console's placement rules are
openmixer's and are applied there. The package descriptions are prose a person writes; the
completeness test checks that they name every shipped plugin.
