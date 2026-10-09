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

<!-- BEGIN GENERATED catalogue: tools/gen.mjs, from each shipped plugin's declaration -->

| Plugin | CLAP id | LV2 URI | What it does |
|---|---|---|---|
| **omx chorus** | `org.openmixer.chorus` | `urn:openmixer:chorus` | The console's chorus: up to four voices read one modulated delay line, the right leg's sweep offset by the spread, mixed with the dry signal. |
| **omx deesser** | `org.openmixer.deesser` | `urn:openmixer:deesser` | The console's de-esser: a detector on a band around the sibilance drives the compressor's gain computer, and the reduction lands on that band alone or on the whole signal. |
| **omx delay** | `org.openmixer.delay` | `urn:openmixer:delay` | Stereo delay up to 2 s: feedback through a tone filter that darkens each repeat, ping-pong, wet/dry mix. Zero latency, real-time safe, 44.1 to 192 kHz. |
| **omx drive** | `org.openmixer.drive` | `urn:openmixer:drive` | The console's drive: one waveshaper (soft, tape, tube or exciter) run inside the console's 4x oversampler, on the full band, the lows, the highs or a tilt, with drive, character, wet/dry mix and trim. Stereo, real-time safe, 44.1 to 192 kHz. |
| **omx eq8** | `org.openmixer.eq8` | `urn:openmixer:eq8` | The console's channel EQ in its 8-band form: 8 parametric bands (bell, shelves, notch, all-pass) plus high- and low-pass filters at 12 or 24 dB/oct. Stereo, zero latency, 44.1 to 192 kHz. |
| **omx eq16** | `org.openmixer.eq16` | `urn:openmixer:eq16` | The console's channel EQ in its 16-band form: 16 parametric bands (bell, shelves, notch, all-pass) plus high- and low-pass filters at 12 or 24 dB/oct. Stereo, zero latency, 44.1 to 192 kHz. |
| **omx eq32** | `org.openmixer.eq32` | `urn:openmixer:eq32` | The console's channel EQ in its 32-band form: 32 parametric bands (bell, shelves, notch, all-pass) plus high- and low-pass filters at 12 or 24 dB/oct. Stereo, zero latency, 44.1 to 192 kHz. |
| **omx flanger** | `org.openmixer.flanger` | `urn:openmixer:flanger` | The console's flanger: one modulated short delay with signed feedback, swept by one oscillator and mixed with the dry signal. |
| **omx geq** | `org.openmixer.geq` | `urn:openmixer:geq` | The console's 31-band graphic EQ: one fader per ISO third-octave band, 20 Hz to 20 kHz, ±15 dB each, designed in double precision so the lowest bands hold their shape at 192 kHz. Stereo, zero latency, 44.1 to 192 kHz. |
| **omx keyed-gate** | `org.openmixer.keyed-gate` | `urn:openmixer:keyed-gate` | The console's channel gate with a sidechain key: the detector listens to the key your host routes to it, or to the signal itself. One gain for both legs, so the stereo image never shifts. Real-time safe, 44.1 to 192 kHz. |
| **omx limiter** | `org.openmixer.limiter` | `urn:openmixer:limiter` | The precision limiter: a look-ahead, true-peak, stereo-linked brickwall. |
| **omx phaser** | `org.openmixer.phaser` | `urn:openmixer:phaser` | The console's phaser: identical all-pass sections swept in octaves by one oscillator, with feedback around the chain and a wet/dry mix. |
| **omx pitch** | `org.openmixer.pitch` | `urn:openmixer:pitch` | The console's pitch shifter: shifts both channels up or down by semitones and cents, without changing their length, mixed with the dry signal. |
| **omx reverb** | `org.openmixer.reverb` | `urn:openmixer:reverb` | The console's reverb: room, plate, hall, reverse and gated algorithms with pre-delay, size, damping, width and low and high cuts, mixed with the dry signal. |
| **omx rotary** | `org.openmixer.rotary` | `urn:openmixer:rotary` | The console's rotary speaker: a drum rotor on the low band and a horn rotor on the high band, with stop, slow and fast speeds, mixed with the dry signal. |
| **omx strip** | `org.openmixer.strip` | `urn:openmixer:strip` | The console's channel strip in one plugin: input trim, HPF/LPF, gate, eight-band EQ and compressor, in the desk's order or any other. Stereo, real-time safe, 44.1 to 192 kHz. |
| **omx transient** | `org.openmixer.transient` | `urn:openmixer:transient` | The console's transient designer: more or less attack and sustain, from two envelope contrasts, with no threshold to set. |
| **omx tremolo** | `org.openmixer.tremolo` | `urn:openmixer:tremolo` | The console's tremolo and auto-pan: one oscillator turned into a level change on both legs, or into a left-right pan. |

<!-- END GENERATED catalogue -->

<!-- BEGIN GENERATED sections: tools/gen.mjs, from each shipped plugin's declaration (its manual, else its parameters) -->

### omx chorus

| Parameter | Range | Default |
|---|---|---|
| Spread | 0 to 0.5 | 0 |
| Rate | 0.05 to 8 Hz | 0.6 Hz |
| Depth | 0 to 12 ms | 4 ms |
| Voices | 1 to 4, whole steps | 3 |
| Mix | 0 to 100 % | 35 % |

Plus the host's bypass.

### omx deesser

| Parameter | Range | Default |
|---|---|---|
| Freq | 2000 to 16000 Hz, whole steps | 7000 Hz |
| Width | 0.25 to 4 oct | 1 oct |
| Threshold | -60 to 0 dB | -30 dB |
| Ratio | 1 to 20 | 4 |
| Range | -24 to 0 dB | -12 dB |
| Attack | 0.1 to 50 ms | 1 ms |
| Release | 5 to 500 ms, whole steps | 60 ms |
| Mode | Split / Wideband | Split |

Plus the host's bypass.

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
| Band Frequency | 20 to 20000 Hz, whole steps | 2000 Hz |
| Mix | 0 to 100 % | 100 % |
| Trim | -24 to 12 dB | 0 dB |
| Curve | Soft / Tape / Tube / Exciter | Soft |
| Band | Full / Low / High / Tilt | Full |
| Auto Gain | off / on | on |
| Stereo Link | off / on | on |
| HF Roll-off | 0 / 12000 / 16000 | 0 |

Plus the host's bypass.

### omx eq8

| Section | Parameters |
|---|---|
| High-pass | on/off, 20 to 1000 Hz, 12 or 24 dB/oct |
| Low-pass | on/off, 1 to 20 kHz, 12 or 24 dB/oct |
| Bands 1 to 8 | each on/off, type, frequency (20 Hz to 20 kHz), gain (±15 dB), Q (0.3 to 8, a notch to 116) |

Plus the host's bypass, which is the whole EQ's switch. Every band and filter starts off, so a freshly loaded instance passes the
signal untouched.

### omx eq16

| Section | Parameters |
|---|---|
| High-pass | on/off, 20 to 1000 Hz, 12 or 24 dB/oct |
| Low-pass | on/off, 1 to 20 kHz, 12 or 24 dB/oct |
| Bands 1 to 16 | each on/off, type, frequency (20 Hz to 20 kHz), gain (±15 dB), Q (0.3 to 8, a notch to 116) |

Plus the host's bypass, which is the whole EQ's switch. Every band and filter starts off, so a freshly loaded instance passes the
signal untouched.

### omx eq32

| Section | Parameters |
|---|---|
| High-pass | on/off, 20 to 1000 Hz, 12 or 24 dB/oct |
| Low-pass | on/off, 1 to 20 kHz, 12 or 24 dB/oct |
| Bands 1 to 32 | each on/off, type, frequency (20 Hz to 20 kHz), gain (±15 dB), Q (0.3 to 8, a notch to 116) |

Plus the host's bypass, which is the whole EQ's switch. Every band and filter starts off, so a freshly loaded instance passes the
signal untouched.

### omx flanger

| Parameter | Range | Default |
|---|---|---|
| Rate | 0.05 to 5 Hz | 0.25 Hz |
| Depth | 0 to 5 ms | 2 ms |
| Feedback | -0.95 to 0.95 | 0.6 |
| Mix | 0 to 100 % | 50 % |

Plus the host's bypass.

### omx geq

| Parameter | Range | Default |
|---|---|---|
| 20 Hz | -15 to 15 dB | 0 dB |
| 25 Hz | -15 to 15 dB | 0 dB |
| 31.5 Hz | -15 to 15 dB | 0 dB |
| 40 Hz | -15 to 15 dB | 0 dB |
| 50 Hz | -15 to 15 dB | 0 dB |
| 63 Hz | -15 to 15 dB | 0 dB |
| 80 Hz | -15 to 15 dB | 0 dB |
| 100 Hz | -15 to 15 dB | 0 dB |
| 125 Hz | -15 to 15 dB | 0 dB |
| 160 Hz | -15 to 15 dB | 0 dB |
| 200 Hz | -15 to 15 dB | 0 dB |
| 250 Hz | -15 to 15 dB | 0 dB |
| 315 Hz | -15 to 15 dB | 0 dB |
| 400 Hz | -15 to 15 dB | 0 dB |
| 500 Hz | -15 to 15 dB | 0 dB |
| 630 Hz | -15 to 15 dB | 0 dB |
| 800 Hz | -15 to 15 dB | 0 dB |
| 1 kHz | -15 to 15 dB | 0 dB |
| 1.25 kHz | -15 to 15 dB | 0 dB |
| 1.6 kHz | -15 to 15 dB | 0 dB |
| 2 kHz | -15 to 15 dB | 0 dB |
| 2.5 kHz | -15 to 15 dB | 0 dB |
| 3.15 kHz | -15 to 15 dB | 0 dB |
| 4 kHz | -15 to 15 dB | 0 dB |
| 5 kHz | -15 to 15 dB | 0 dB |
| 6.3 kHz | -15 to 15 dB | 0 dB |
| 8 kHz | -15 to 15 dB | 0 dB |
| 10 kHz | -15 to 15 dB | 0 dB |
| 12.5 kHz | -15 to 15 dB | 0 dB |
| 16 kHz | -15 to 15 dB | 0 dB |
| 20 kHz | -15 to 15 dB | 0 dB |

Plus the host's bypass.

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
| Knee start | -80 to 0 dB | -43 dB |
| Knee end | -80 to 0 dB | -37 dB |
| Hold | 0 to 2000 ms | 10 ms |
| Hysteresis | 0 to 24 dB | 3 dB |

Plus the host's bypass. An attack under 0.5 ms engages the 4x detector path, which adds 72 frames
of latency, reported to the host; otherwise the latency is zero. Its LV2 URI, ports and defaults
are those of the keyed gate bundle the OpenMixer console used to ship, so a session saved with that
bundle loads this one.

### omx limiter

| Parameter | Range | Default |
|---|---|---|
| Ceiling | -12 to 0 dBFS | -1 dBFS |
| Lookahead | 0.5 to 5 ms | 1.5 ms |
| Release | 1 to 1000 ms, whole steps | 50 ms |

Plus the host's bypass.

### omx phaser

| Parameter | Range | Default |
|---|---|---|
| Rate | 0.05 to 5 Hz | 0.5 Hz |
| Base | 50 to 2000 Hz, whole steps | 200 Hz |
| Depth | 0 to 6 oct | 4 oct |
| Stages | 2 to 12, whole steps | 6 |
| Feedback | -0.9 to 0.9 | 0.4 |
| Mix | 0 to 100 % | 50 % |

Plus the host's bypass.

### omx pitch

| Parameter | Range | Default |
|---|---|---|
| Semitones | -12 to 12 | 0 |
| Cents | -50 to 50 | 0 |
| Mix | 0 to 100 | 100 |

Plus the host's bypass.

### omx reverb

| Parameter | Range | Default |
|---|---|---|
| Plate Mod Depth | 0 to 400 %, whole steps | 100 % |
| Mix | 0 to 1 | 0.3 |
| Size | 0 to 1 | 0.7 |
| Damping | 0 to 1 | 0.5 |
| Width | 0 to 1 | 1 |
| Predelay | 0 to 100 ms, whole steps | 0 ms |
| Lowcut | 0 to 20000 Hz, whole steps | 0 Hz |
| Highcut | 0 to 20000 Hz, whole steps | 20000 Hz |
| Reverse | 50 to 500 ms, whole steps | 300 ms |
| Hold | 10 to 2000 ms, whole steps | 120 ms |
| Release | 1 to 500 ms, whole steps | 20 ms |
| Gate Threshold | -80 to 0 dBFS, whole steps | -40 dBFS |
| Algorithm | Room / Plate / Hall / Reverse / Gated | Room |

Plus the host's bypass.

### omx rotary

| Parameter | Range | Default |
|---|---|---|
| Horn Slow | 0.1 to 2 Hz | 0.8 Hz |
| Horn Fast | 3 to 10 Hz | 6.8 Hz |
| Drum Slow | 0.1 to 2 Hz | 0.7 Hz |
| Drum Fast | 3 to 10 Hz | 5.9 Hz |
| Accel | 0.25 to 4 x | 1 x |
| Balance | -100 to 100 %, whole steps | 0 % |
| Mix | 0 to 100 % | 100 % |
| Speed | Stop / Slow / Fast | Slow |

Plus the host's bypass.

### omx strip

Five stages, each omx-dsp's instance face of the console's own kernel, run one after the other; each has its own switch:

| Stage | Parameters |
|---|---|
| Input | on/off; Trim, a click-free ramp to each new gain |
| Filters | on/off; HPF and LPF, each on/off, its frequency, 12 or 24 dB/oct |
| Gate | on/off, threshold, ratio, range, attack, release, knee start and end, hold, hysteresis |
| EQ | on/off; eight bands, each on/off, type (bell, low or high shelf, notch, all-pass 1st or 2nd order), frequency, gain, Q |
| Comp | on/off, threshold, ratio, knee, attack, release, make-up, kind (compressor: RMS detector; limiter: peak), mix, detector oversampling (auto, off, 4x) |

**Order** (0 to 119) picks the order of the five stages: 0 is the desk's input > filters > gate > EQ >
comp, and the rest follow in lexicographic order (1 is input > filters > gate > comp > EQ, ... 119 is
comp > EQ > gate > filters > input). Plus the host's bypass. The latency reported is the sum of the
stages': the gate's and the compressor's 4x detector paths while they are engaged, else zero.

### omx transient

| Parameter | Range | Default |
|---|---|---|
| Attack | -24 to 24 dB | 0 dB |
| Sustain | -24 to 24 dB | 0 dB |
| Attack Time | 2 to 50 ms | 10 ms |
| Sustain Time | 50 to 2000 ms, whole steps | 250 ms |
| Output | -24 to 12 dB | 0 dB |

Plus the host's bypass.

### omx tremolo

| Parameter | Range | Default |
|---|---|---|
| Rate | 0.1 to 20 Hz | 4 Hz |
| Depth | 0 to 100 % | 50 % |
| Mix | 0 to 100 % | 100 % |
| Mode | Tremolo / Pan | Tremolo |

Plus the host's bypass.

<!-- END GENERATED sections -->

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
`/usr/lib/lv2` on Debian). Rescan plugins in your host and look for the **omx** plugins under openmixer.

Every release also carries the packages on its
[GitHub release page](https://github.com/FreeMixer/omx-plugins/releases).

Projects that build on the plugins' parameters and instance code install the headers instead of
copying them: `omx-plugins-devel` on Fedora, `omx-plugins-dev` on Debian (they land in
`/usr/include/omx-plugins`).

To build from source, see [BUILDING.md](BUILDING.md).

## Licence

GPL-3.0-or-later. See [LICENSE](LICENSE).
