<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
<!-- Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com> -->
# A plugin's meters

A dynamics plugin that replaces a console stage owes the console the stage's gain-reduction meter
(openmixer's spec `2026-10-02-lean-engine-effects-as-plugins.md` §8.2 and §8.4). This is how a
plugin here reports one.

## The interface

`clap.meters/1`, the CLAP meters extension (free-audio/clap#521), and nothing else. It is the one
omx-clap-core reads for meters since it removed `org.openmixer.meters/1` (omx-clap-core
`docs/meters.md`, openmixer#1544): every channel of every meter becomes an output symbol, read at
the host's idle rate through `get_values()`.

No released CLAP SDK ships `clap/ext/draft/meters.h` yet, and omx-clap-core does not install its
copy. `include/omx_clap_meters.h` declares the draft's id and structures exactly as the draft and
omx-clap-core's `src/clap_meters.h` do (the same include guard, so the two are one header), and
includes the SDK's header instead once the SDK has it.

## The declaration

A plugin declares its meters in its declaration, `meters`, one entry each:

- `id`: the meter's symbol, also its stable `clap_meter_info.name` (the host derives the output
  symbol from the name, so the two agree). Its CLAP id is its index in the list: append-only, as
  the parameters are.
- `kind`: `gain_reduction`, the only kind declared so far.
- `field`: where the kernel's own reading is, in its instance face's state
  (`state.band.gr_min_db` for the de-esser). The face reads it and nothing else: no meter is
  computed twice.
- `floor`: the parameter whose travel's low end is the deepest reduction the kernel can apply; it
  is the display range's minimum, and 0 dB its maximum.

## What the face reports

- **The value is the kernel's.** A gain reduction is dB with CLAP's sign: 0 none, negative a
  reduction, the deepest gain the kernel applied inside the block (omx-dsp's `gr_min_db`, what
  the console's own stage publishes). The console holds and releases it for display, as it does
  for its own stages; the plugin does not.
- **Per block.** The audio thread publishes it after each `process()` with one relaxed atomic
  store: no allocation, no lock. `get_values()` loads it from any thread.
- **Bypassed is 0.** A bypassed kernel leaves its state alone, so its last reading is stale; the
  face reports 0, as the console's disabled stage does.
- **Nothing before the first block.** After activation, until a block is processed,
  `get_values()` writes nothing and returns 0.
- `channel_count` is 1 (one detector drives both legs) and the meter is coherent.

## The test

The identity test (`test/<kernel>-oracle.c`) reads the meter after every block of its plan through
the CLAP face, at every declared rate, and holds it to the reference: the same instance face, run
directly on the same blocks, read at the declared `field` (0 when bypassed). Bit for bit. Two
guards: the plan must make the kernel reduce gain somewhere, and the face must write nothing
before its first block. A wrong sign or a value one block stale is red.
