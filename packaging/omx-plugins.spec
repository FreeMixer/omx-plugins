# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
Name: omx-plugins
Version: 0.8.0
Release: 1%{?dist}
License: GPL-3.0-or-later
Summary: The OpenMixer console's delay, drive, EQ, channel strip, keyed gate and effects, as plugins for your DAW
URL: https://github.com/FreeMixer/omx-plugins

Source0: %{url}/archive/v%{version}/%{name}-%{version}.tar.gz

BuildRequires: gcc
BuildRequires: make
BuildRequires: pkgconf-pkg-config
# The releases this tree builds against: a floor each (no ceiling), once here and
# the same in debian/control (tools/deps-check.sh). The Makefile and tools/gen.mjs read them from here.
BuildRequires: omx-dsp-devel >= 0.18.0
BuildRequires: omx-contract-devel >= 2.12.0
BuildRequires: omx-clap-core-devel >= 0.6.0
BuildRequires: clap-devel
BuildRequires: lv2-devel
# %%check: the generators and checks are Node scripts; the hosts that load the built plugins
BuildRequires: nodejs
# make selftest copies the tree with tar
BuildRequires: tar
BuildRequires: lilv
BuildRequires: lilv-devel
BuildRequires: sord
BuildRequires: omx-clap-host

# The name a user installs: both formats.
Requires: %{name}-clap%{?_isa} = %{version}-%{release}
Requires: %{name}-lv2%{?_isa} = %{version}-%{release}

%description
Take the sound of the OpenMixer console into your DAW or onto your own rig:
delay, drive, the channel EQ as an 8-band EQ, a 16-band EQ or a 32-band EQ, a
31-band graphic EQ, the full channel strip and the keyed gate with its sidechain
input, and the console's effects: chorus, flanger, phaser, tremolo, rotary
speaker, reverb, pitch shifter, transient shaper, de-esser, limiter and compressor. They come
as CLAP plugins for Bitwig, REAPER and Carla, and as LV2 plugins for Ardour,
Carla, MOD and Zynthian. Each one runs the same DSP as the console, so a track sounds the same
in a session as it does on the desk, sample for sample. Every plugin comes in
both formats with the same controls, MOD GUI included.

This package installs both formats; omx-plugins-clap and omx-plugins-lv2
install one each.

%package clap
Summary: OpenMixer delay, drive, EQ, strip, gate and effects as CLAP plugins

%description clap
The OpenMixer console's delay, drive, 8-, 16- and 32-band EQ, 31-band graphic
EQ, channel strip, keyed gate and effects (chorus, flanger, phaser, tremolo,
rotary speaker, reverb, pitch shifter, transient shaper, de-esser, limiter, compressor) as
CLAP plugins, installed in /usr/lib/clap where Bitwig, REAPER,
Carla and other CLAP hosts find them. They run the console's own DSP, so they
sound like the desk.

%package lv2
Summary: OpenMixer delay, drive, EQ, strip, gate and effects as LV2 plugins
Requires: lv2

%description lv2
The OpenMixer console's delay, drive, 8-, 16- and 32-band EQ, 31-band graphic
EQ, channel strip, keyed gate and effects (chorus, flanger, phaser, tremolo,
rotary speaker, reverb, pitch shifter, transient shaper, de-esser, limiter, compressor) as
LV2 plugins, installed in %{_libdir}/lv2 where Ardour, Carla, MOD
and Zynthian find them, with a MOD GUI where the plugin has one. They run the
console's own DSP, so they sound like the desk.

%package devel
Summary: Headers and static library of the OpenMixer plugins, for projects that build on them
Requires: omx-dsp-devel >= 0.18.0
Requires: clap-devel

%description devel
The headers other projects include to use the OpenMixer plugins' parameters and
instance code, such as omx_delay_core.h and each plugin's generated
parameter header, installed in %{_includedir}/omx-plugins, and
libomxplugins.a (pkg-config omxplugins): every plugin's CLAP face in one
static library, with the table of their entries in omx_clap_static.h, for a
host that runs the plugins in its own process without loading .clap files.

%prep
%autosetup

%build
%set_build_flags
%make_build

%install
%make_install PREFIX=%{_prefix} LIBDIR=%{_libdir} CLAPDIR=/usr/lib/clap
%{__make} install-devel DESTDIR=%{buildroot} PREFIX=%{_prefix} LIBDIR=%{_libdir}

%check
%make_build test

%files
%license LICENSE
%doc README.md

%files clap
%license LICENSE
%doc README.md
%dir /usr/lib/clap
# BEGIN GENERATED clap-files: tools/gen.mjs, one line per shipped plugin
/usr/lib/clap/omx-chorus.clap
/usr/lib/clap/omx-comp.clap
/usr/lib/clap/omx-deesser.clap
/usr/lib/clap/omx-delay.clap
/usr/lib/clap/omx-drive.clap
/usr/lib/clap/omx-eq8.clap
/usr/lib/clap/omx-eq16.clap
/usr/lib/clap/omx-eq32.clap
/usr/lib/clap/omx-flanger.clap
/usr/lib/clap/omx-geq.clap
/usr/lib/clap/omx-keyed-gate.clap
/usr/lib/clap/omx-limiter.clap
/usr/lib/clap/omx-phaser.clap
/usr/lib/clap/omx-pitch.clap
/usr/lib/clap/omx-reverb.clap
/usr/lib/clap/omx-rotary.clap
/usr/lib/clap/omx-strip.clap
/usr/lib/clap/omx-transient.clap
/usr/lib/clap/omx-tremolo.clap
# END GENERATED clap-files

%files devel
%license LICENSE
%{_includedir}/omx-plugins/
%{_libdir}/libomxplugins.a
%{_libdir}/pkgconfig/omxplugins.pc

%files lv2
%license LICENSE
%doc README.md
# BEGIN GENERATED lv2-files: tools/gen.mjs, one line per shipped plugin
%{_libdir}/lv2/omx-chorus.lv2/
%{_libdir}/lv2/omx-comp.lv2/
%{_libdir}/lv2/omx-deesser.lv2/
%{_libdir}/lv2/omx-delay.lv2/
%{_libdir}/lv2/omx-drive.lv2/
%{_libdir}/lv2/omx-eq8.lv2/
%{_libdir}/lv2/omx-eq16.lv2/
%{_libdir}/lv2/omx-eq32.lv2/
%{_libdir}/lv2/omx-flanger.lv2/
%{_libdir}/lv2/omx-geq.lv2/
%{_libdir}/lv2/omx-keyed-gate.lv2/
%{_libdir}/lv2/omx-limiter.lv2/
%{_libdir}/lv2/omx-phaser.lv2/
%{_libdir}/lv2/omx-pitch.lv2/
%{_libdir}/lv2/omx-reverb.lv2/
%{_libdir}/lv2/omx-rotary.lv2/
%{_libdir}/lv2/omx-strip.lv2/
%{_libdir}/lv2/omx-transient.lv2/
%{_libdir}/lv2/omx-tremolo.lv2/
# END GENERATED lv2-files

%changelog
* Thu Oct 15 2026 Pau Aliagas <linuxnow@gmail.com> - 0.8.0-1
- Library dependencies are a floor only, no ceiling: omx-dsp >= 0.18.0
  (the first release the plugins build and pass make test against; 0.17.0
  does not link) and omx-contract >= 2.11.0. tools/deps-check.sh fails on
  any upper bound.
- The parameter-contract header names the LUFS unit (OMX_CLAP_UNIT_LUFS,
  0x103), which openmixer's MAIN loudness target travels in.

* Wed Oct 14 2026 Pau Aliagas <linuxnow@gmail.com> - 0.7.0-1
- Built against omx-dsp 0.18.0 (at least 0.18.0, below 0.19) and omx-
  contract 2.11.0 (at least 2.11.0, below 2.12). No pins file: the
  packaging declares both ranges and the build reads them from there.
- The manuals read every range, default and limit from the plugins'
  parameters, which come from omx-contract, so a travel moved in the
  contract moves the manual with it.

* Tue Oct 13 2026 Pau Aliagas <linuxnow@gmail.com> - 0.6.0-1
- Built against omx-dsp 0.13.0 and omx-contract 2.6.0, the pair the channel
  installs.
- omx comp takes the compressor's detector topology: Feedforward hears the
  input, Feedback the compressor's own output. omx strip keeps its
  parameters and runs its compressor feed-forward.
* Tue Oct 13 2026 Pau Aliagas <linuxnow@gmail.com> - 0.5.1-1
- omx deesser reports its gain reduction: the CLAP face serves the kernel's
  gain reduction per block through clap.meters/1, and its oracle holds that
  meter to the kernel at every rate.
- A plugin declares its meters in its declaration; the generator serves them
  through clap.meters/1 and checks them in the oracle.
* Mon Oct 12 2026 Pau Aliagas <linuxnow@gmail.com> - 0.5.0-1
- The faces follow omx-dsp 0.6.0, the oldest release they build against now.
- omx pitch reports its latency at full mix: its shortest delay plus half its
  window, 883 frames at 44.1 kHz, and 0 below 100 %, where the dry signal is
  on time. The CLAP face takes a new latency at its next activation and asks
  the host for that restart when the mix crosses 100 %; the LV2 latency port
  follows the mix.
- Every face serves its panel description, read from its declaration.
- The CLAP faces carry the extensions their declaration names: the parameter
  contract, remote-control pages and each placement's defaults.
- Each plugin declares its panel once; the console's sections and graphs are
  part of it.
- The panels use one widget vocabulary, the console's names.
- The LV2 faces group their ports, one group per panel section.
- The MOD GUI look is generated from its design tokens.
- The declarations' comments are in plain words.
* Sun Oct 11 2026 Pau Aliagas <linuxnow@gmail.com> - 0.4.0-1
- **libomxplugins**: one static library with a table of every plugin's CLAP
  entry, so a host links the plugins in-process without dlopen. The devel
  package ships `libomxplugins.a`, `omx-plugins/omx_clap_static.h` and
  `omxplugins.pc`; `omx_clap_static_find(id)` returns a plugin's entry and
  `omx_clap_static_entry` is one entry whose factory lists them all.
* Sat Oct 10 2026 Pau Aliagas <linuxnow@gmail.com> - 0.3.0-1
- New plugin, **omx comp** (CLAP and LV2): the console's compressor.
  Threshold, ratio and knee set how hard it holds the level down, attack and
  release how fast, with make-up gain and a dry/wet mix; Kind picks the RMS
  detector (compressor) or the peak detector (limiter), and detector
  oversampling can run the control path at 4x.
- **omx strip** is now built from its declaration, as a chain of the console's
  own stages: input trim, high- and low-pass filters, gate, four-band EQ and
  compressor, each with its own on/off switch, in the console's order or any
  other.
- **omx strip: sessions saved with 0.2.0 do not load as they were.** Its
  parameter list changed: the new Input and Filters switches come first, so
  every later parameter moved; Order now picks among the 120 orders of five
  stages (0 is still the console's order); the gate gains knee start and end,
  hold and hysteresis; the RMS switch is replaced by Kind (compressor, RMS
  detector, or limiter, peak), and a fresh strip now starts as a compressor
  where 0.2.0 started with the peak detector; the compressor gains mix and
  detector oversampling. At the settings 0.2.0 had, the strip sounds the same,
  sample for sample.
- Builds against omx-dsp 0.5.2 and omx-contract 2.3.0, the releases with the
  input trim.

* Thu Oct 08 2026 Pau Aliagas <linuxnow@gmail.com> - 0.2.0-1
- New plugin, **omx keyed-gate** (CLAP and LV2): the console's keyed gate, its
  detector fed by a sidechain key the host routes to it, or by the signal
  itself. It takes over the keyed gate bundle the OpenMixer console shipped,
  with the same LV2 URI, ports and defaults.
- New plugins, **omx eq16** and **omx eq32** (CLAP and LV2): the console's
  channel EQ as a 16-band EQ and a 32-band EQ, the same bands, pass filters
  and DSP as omx eq8. Stereo, zero latency; every band starts off, so a
  freshly loaded instance passes the signal untouched.
- Every LV2 plugin shows its controls as the console's bundles did:
  frequencies on a logarithmic travel in hertz, gains in dB, times,
  percentages and octaves in their units, the latency in frames, and the EQ
  band types and filter slopes as named choices instead of bare numbers.
- New package **omx-plugins-devel** (RPM) and **omx-plugins-dev** (DEB): the
  headers other projects include, such as omx_delay_instance.h and each
  plugin's generated parameter header, so they stop copying them.
- Every parameter's travel, default and choices now come from the OpenMixer
  contract (omx-contract 1.3.0), not from numbers typed in each plugin, so a
  plugin and the console cannot disagree about a range. A fresh EQ (eq8, eq16,
  eq32) now starts with its bands where the console's one rule puts them: four
  bands at 100, 400, 2000 and 8000 Hz, any other count spread evenly and
  snapped to preferred frequencies, a low shelf first, a high shelf last and
  bells between. Every band still starts off, so a freshly loaded instance
  passes the signal untouched.
- The CLAP plugins now install to /usr/lib/clap on every distribution, the
  path the CLAP standard gives for Linux. The Fedora packages used
  /usr/lib64/clap before.
- Builds against omx-dsp 0.2.0 and omx-contract 1.3.0; CI runs on GitHub's
  ubuntu-latest runners.

* Wed Oct 07 2026 Pau Aliagas <linuxnow@gmail.com> - 0.1.0-1
- First package: the OpenMixer console's delay, drive, 8-band EQ and channel
  strip, each as a CLAP and an LV2 plugin, running the same DSP as the console
  (omx-dsp 0.1.4).
