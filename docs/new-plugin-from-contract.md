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
  with the omx-dsp work named. A control the kernel takes once per band is one array argument,
  `const float band[OMX_GEQ_BANDS]`, sized by a count the contract renders: the draft gives it one
  parameter per band (`band01` … `band31`), the binding passes them in declaration order, and the
  generated code asserts their count is the extent. A keyed face takes the sidechain block first,
  `run(s, key, in_l, in_r, out_l, out_r, n)` (`NULL`: nothing routed, the kernel's own detector); its
  declaration names the key port in `sidechain`, and the faces and the identity test carry the key
  (spec §15.1);
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
| `panel` | one section of every parameter (the MOD GUI) | groups the controls into sections |
| `console` | its `placement`'s strip kinds and group as `REVIEW` | the placement, and the card, chip and panel the console draws (spec §8) |
| `portHints` | written after the first generation: the pin of the hints the generated TTL carries (`make hints` holds every later TTL to it) | — |

Step 3 refuses while any `REVIEW` mark is left, naming each by its JSON pointer. When the draft is
settled it fills the derived fields and runs `tools/gen.mjs`, which writes:

- per plugin: `generated/` (parameter header, LV2 bundle, MOD GUI from the `panel`), the
  `Makefile`, `omx_<kernel>_clap.c`, `omx_<kernel>_lv2.c`, the binding `omx_<kernel>_core.h` and
  the kernel-identity test `test/<kernel>-oracle.c`, each with a GENERATED banner;
- across plugins, from the folders: the README's catalogue and sections, the RPM `%files` lists and
  CI's installed-file lists, each between `BEGIN GENERATED` and `END GENERATED` lines.

`node tools/gen.mjs --check` holds every one of them; CI runs it. Then the wizard runs the plugin's
`make test` and `make completeness` for it, and prints the commit plan in the recipe's layer order.

## The identity test

One template, filled from the declaration. The plan has three parts. First, a deterministic stimulus
in uneven blocks, with every parameter moving between blocks across its declared travel and the
bypass toggled. Second, at the defaults, a full-scale burst followed by a tail that falls onto a quiet
bed. The tail lasts the sum of every time travel's default (at least 100 ms), so a dynamics stage
engages and releases, and a gate holds and closes. Third, for every choice or toggle, each of its
values in turn over its own burst and two tails: the other parameters stay at their defaults for the
first tail and are stepped once for the second. A control that only one mode reads is therefore
reached. The second and third parts are timed in milliseconds, so every rate gets the same time. The CLAP and LV2
builds must equal omx-dsp's instance face called directly, bit for bit, at every rate in
`OMX_DECLARED_RATES`, and publish its latency. A keyed face runs the plan twice, with a key that
differs from the main signal and with none, each block 64 times in a row so a detector has time to
release through every threshold; the key must change the output. Two guards keep it honest: the reference must move
the signal, and a reference rendered with one parameter one step off must differ from the faces
(the sabotage arm), so a test that could not see a wrong coefficient fails.

A parameter whose contract control declares `rearms` is held at its default in every block. Such a
control re-arms the kernel's state when it changes, so it is not a smooth parameter: the limiter's
look-ahead, for example, rebuilds the rings and restarts the gain at unity. Moving it between blocks
would test the re-arm, not the identity. The flag is read from omx-contract's JSON render, where
`kernels.<kernel>.controls` lists each kernel's controls in declared order. The oracle's header comment names each held parameter, and the sabotage arm still moves it
by one step. The other parameters move as before.

## A plugin made of several kernels: a chain

`node tools/omx-new-plugin.mjs --from-contract <k1>,<k2>,... --stem omx-<x>` drafts a composite
(`"binding": "chain"`, spec 2026-10-09-plugin-from-contract §15.2): one element per kernel, in the
order given, each element's parameters drafted as a single plugin's are and refused, naming the
omx-dsp work, for a kernel with no instance face of the generated shape. The declaration has no
`params`: the list is generated from `chain`, element by element, each element's switch `<id>On`
(its default the element's `on`) and then its parameters, then `order` when the chain says
`"order": "permutable"` (the permutations of the elements, lexicographic, 0 the declared order).

An element may declare `bands`, the key of its kernel's count sheet whose `max` is how many bands
it exposes (`strip`), `faceBands`, the key its face is compiled at where omx-dsp's face takes no
smaller count (`eq8`), and `fixed`, the controls of its kernel it does not expose, each at an id, a
number, `"default"` (refused for a travel that declares none) or `{"defaultRef": "<SHEET>.<field>"}`;
a per-band control fixed so covers every band the element does not expose, `"default"` giving each
its own default by the contract's one rule. omx-strip's EQ exposes the strip's four bands on the
eq8 face, the other four fixed off; its filter element is the eq face with every band fixed off;
its gate, which takes no key in a strip, has its key source fixed at `self`.

The generated binding copies in to out once and runs each element's face in place in the chosen
order; an element's bypass is its switch off or the host's bypass; the latency is the sum. The
oracle's reference is the same chain of faces called directly, its order computed in the test.
A chain converted from a released plugin names that release in `shipped`; `tools/gen.mjs` then
refuses a parameter list that differs from the release's, unless `shippedDiff` names every
difference; where the tag cannot be read (a tarball build) gen says the list went unchecked.
`make strip-crosscheck` (`tools/test/strip-crosscheck.sh`) holds omx-strip, so converted, to the hand-ported
plugin it replaced, bit for bit at that plugin's settings.

## What it does not do

Placement, the console's card, chip and panel, and the panel's widgets are declared here (every
plugin declares `panel` and `console`) and checked against the vocabulary in
`schema/plugin.decl.schema.json`; the console's placement rules are
openmixer's and are applied there. The package descriptions are prose a person writes; the
completeness test checks that they name every shipped plugin.
