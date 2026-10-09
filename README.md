<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
<!-- Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com> -->
# omx-plugins

The effects of the OpenMixer console, as plugins you can load
in any host: **CLAP** for Bitwig, REAPER, Carla, openmixer and every other CLAP host, and **LV2** for
Ardour, Carla, MOD and Zynthian, the MOD GUI included.

The DSP inside each one is the console's own (the [omx-dsp](https://github.com/FreeMixer/omx-dsp)
library), so a plugin sounds the same in your DAW as on the desk, sample for sample. Each plugin is
drawn from one declaration: its CLAP parameters, its LV2 ports and its MOD GUI cannot disagree.

## The plugins

| Plugin | CLAP id | LV2 URI | What it does |
|---|---|---|---|
| **omx delay** | `org.openmixer.delay` | `urn:openmixer:delay` | Stereo delay up to 2 s: feedback through a tone filter that darkens each repeat, ping-pong, wet/dry mix. Zero latency, real-time safe, 44.1 to 192 kHz. |
| **omx drive** | `org.openmixer.drive` | `urn:openmixer:drive` | The console's drive: one waveshaper run inside the console's oversampler, with drive, character, band, wet/dry mix and trim. Stereo, real-time safe, 44.1 to 192 kHz. |
| **omx eq8** | `org.openmixer.eq8` | `urn:openmixer:eq8` | The console's channel EQ in its eight-band form: eight parametric bands (bell, shelves, notch, all-pass) plus high- and low-pass filters at 12 or 24 dB/oct. Stereo, zero latency, 44.1 to 192 kHz. |
| **omx eq16** | `org.openmixer.eq16` | `urn:openmixer:eq16` | The console's channel EQ in its 16-band form: sixteen parametric bands (bell, shelves, notch, all-pass) plus high- and low-pass filters at 12 or 24 dB/oct. Stereo, zero latency, 44.1 to 192 kHz. |
| **omx eq32** | `org.openmixer.eq32` | `urn:openmixer:eq32` | The console's channel EQ in its 32-band form: thirty-two parametric bands (bell, shelves, notch, all-pass) plus high- and low-pass filters at 12 or 24 dB/oct. Stereo, zero latency, 44.1 to 192 kHz. |
| **omx strip** | `org.openmixer.strip` | `urn:openmixer:strip` | The console's channel strip in one plugin: input trim with HPF/LPF, gate, four-band EQ and compressor, in the desk's order or any other. Stereo, real-time safe, 44.1 to 192 kHz. |
| **omx chorus** | `org.openmixer.chorus` | `urn:openmixer:chorus` | the native CHORUS stage: N voices reading ONE modulated fractional delay line. |
| **omx keyed-gate** | `org.openmixer.keyed-gate` | `urn:openmixer:keyed-gate` | The console's channel gate with a sidechain key: the detector listens to the key your host routes to it, or to the signal itself. One gain for both legs, so the stereo image never shifts. Real-time safe, 44.1 to 192 kHz. |

### omx delay

| Parameter | Range | Default |
|---|---|---|
| Time | 0 to 2000 ms, whole ms | 300 ms |
| Feedback | 0 to 0.99 | 0.3 |
| Mix (wet) | 0 to 1 | 0.3 |
| Tone (0 bright, 1 dark repeats) | 0 to 1 | 0.3 |
| Ping-pong | off / on | off |

Plus the host's bypass. The tone filter holds the same corner at every sample rate, so a session
moved from 48 to 96 kHz sounds the same.

### omx drive

| Parameter | Range | Default |
|---|---|---|
| Drive | 0 to 36 dB | 0 dB |
| Character | -1 to 1 | 0 |
| Band | 20 to 20000 Hz | 2000 Hz |
| Mix | 0 to 100 % | 100 % |
| Trim | -24 to +12 dB | 0 dB |

Plus the host's bypass. The latency the plugin reports is the oversampler's.

### omx eq8

| Section | Parameters |
|---|---|
| EQ | on/off (on) |
| High-pass | on/off, 20 to 1000 Hz, 12 or 24 dB/oct |
| Low-pass | on/off, 1 to 20 kHz, 12 or 24 dB/oct |
| Bands 1 to 8 | each on/off, type, frequency (20 Hz to 20 kHz), gain (±15 dB), Q (0.3 to 116) |

Plus the host's bypass. Every band and filter starts off, so a freshly loaded instance passes the
signal untouched.

### omx eq16

| Section | Parameters |
|---|---|
| EQ | on/off (on) |
| High-pass | on/off, 20 to 1000 Hz, 12 or 24 dB/oct |
| Low-pass | on/off, 1 to 20 kHz, 12 or 24 dB/oct |
| Bands 1 to 16 | each on/off, type, frequency (20 Hz to 20 kHz), gain (±15 dB), Q (0.3 to 116) |

Plus the host's bypass. Every band and filter starts off, so a freshly loaded instance passes the
signal untouched. The bands' frequencies start spread from 25 Hz to 16 kHz, as on
the console.

### omx eq32

| Section | Parameters |
|---|---|
| EQ | on/off (on) |
| High-pass | on/off, 20 to 1000 Hz, 12 or 24 dB/oct |
| Low-pass | on/off, 1 to 20 kHz, 12 or 24 dB/oct |
| Bands 1 to 32 | each on/off, type, frequency (20 Hz to 20 kHz), gain (±15 dB), Q (0.3 to 116) |

Plus the host's bypass. Every band and filter starts off, so a freshly loaded instance passes the
signal untouched. The bands' frequencies start spread from 22 Hz to 18 kHz, as on
the console.

### omx strip

Four stages, each the console's own module, run one after the other:

| Stage | Parameters |
|---|---|
| Input | Trim (-24 to +24 dB); HPF (20 to 1000 Hz) and LPF (1 to 20 kHz), each on/off, 12 or 24 dB/oct |
| Gate | on/off, threshold, ratio, range, attack, release |
| EQ | on/off; four bands, each on/off, type (bell, low/high shelf, notch, all-pass), frequency, gain (±15 dB), Q |
| Comp | on/off, threshold, ratio, knee, attack, release, make-up, RMS/peak |

**Order** (0 to 23) picks the order of the four stages: 0 is the desk's input > gate > EQ > comp,
and the rest follow in lexicographic order (1 is input > gate > comp > EQ, 2 is input > EQ > gate >
comp, ... 23 is comp > EQ > gate > input). Plus the host's bypass. The latency reported is the gate's
and the compressor's 4x detector paths while they are engaged, else zero.

### omx keyed-gate

The console's gate, keyed: the stereo signal is gated while the detector listens to a second, mono
input, the **key** (an LV2 sidechain port; in CLAP a second input port). With nothing routed to the
key, or **Key** set to Self, it is the ordinary self-keyed gate.

| Parameter | Range | Default |
|---|---|---|
| Key | Self / Sidechain | Sidechain |
| Threshold | -80 to 0 dB | -40 dB |
| Ratio | 1 to 100 | 16 |
| Range | -90 to 0 dB | -90 dB |
| Attack | 0 to 500 ms | 1 ms |
| Release | 0 to 5000 ms | 100 ms |

Plus the host's bypass. An attack under 0.5 ms engages the 4x detector path, which adds 72 frames
of latency, reported to the host; otherwise the latency is zero. Its LV2 URI, ports and defaults
are those of the keyed gate bundle the OpenMixer console used to ship, so a session saved with that
bundle loads this one.

### omx chorus

| Parameter | Range | Default |
|---|---|---|
| Rate | 0.05 to 8 Hz | 0.6 Hz |
| Depth | 0 to 12 ms | 4 ms |
| Voices | 1 to 4, whole steps | 3 |
| Mix | 0 to 100 % | 35 % |
| Spread | 0 to 0.5 | 0 |

Plus the host's bypass.

## Install

On **Fedora**, from the FreeMixer package channel:

```sh
sudo dnf config-manager addrepo --from-repofile=https://freemixer.github.io/rpm/freemixer.repo
sudo dnf install omx-plugins
```

On **Debian** bookworm or trixie, **Raspberry Pi OS** and **Zynthian** (replace `trixie` with your
release):

```sh
sudo install -d /etc/apt/keyrings
sudo curl -fsSL -o /etc/apt/keyrings/freemixer.asc https://freemixer.github.io/deb/freemixer.asc
echo "deb [signed-by=/etc/apt/keyrings/freemixer.asc] https://freemixer.github.io/deb/debian/trixie ./" \
  | sudo tee /etc/apt/sources.list.d/freemixer.list
sudo apt update
sudo apt install omx-plugins
```

`omx-plugins` installs both formats. To take only the one your host loads, install
`omx-plugins-clap` (CLAP hosts) or `omx-plugins-lv2` (LV2 hosts) instead. The plugins land where
hosts look without configuration (`/usr/lib/clap` on every distribution, the path the CLAP specification gives; `/usr/lib64/lv2` on Fedora,
`/usr/lib/lv2` on Debian). Rescan plugins in your host and look for **omx delay**, **omx drive**, **omx eq8**,
**omx eq16**, **omx eq32**, **omx strip** and **omx keyed-gate** under openmixer.

Every release also carries the packages on its
[GitHub release page](https://github.com/FreeMixer/omx-plugins/releases).

Projects that build on the plugins' parameters and instance code install the headers instead of
copying them: `omx-plugins-devel` on Fedora, `omx-plugins-dev` on Debian (they land in
`/usr/include/omx-plugins`).

To build from source, see [BUILDING.md](BUILDING.md).

## Licence

GPL-3.0-or-later. See [LICENSE](LICENSE).
