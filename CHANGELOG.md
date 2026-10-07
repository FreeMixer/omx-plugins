# Changelog

What changed in each release of omx-plugins, in plain words. The RPM and Debian changelogs and the
GitHub release notes are generated from this file.

## Unreleased

- New plugin, **omx keyed-gate** (CLAP and LV2): the console's keyed gate, its detector fed by a
  sidechain key the host routes to it, or by the signal itself. It takes over the keyed gate bundle
  the OpenMixer console shipped, with the same LV2 URI, ports and defaults.
- New plugins, **omx eq16** and **omx eq32** (CLAP and LV2): the console's channel EQ as a
  16-band EQ and a 32-band EQ, the same bands, pass filters and DSP as omx eq8. Stereo, zero
  latency; every band starts off, so a freshly loaded instance passes the signal untouched.

## 0.1.0 - 2026-10-07

- First package: the OpenMixer console's delay, drive, 8-band EQ and channel strip, each as a CLAP
  and an LV2 plugin, running the same DSP as the console (omx-dsp 0.1.4).
