# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
Name: omx-plugins
Version: 0.2.0
Release: 1%{?dist}
License: GPL-3.0-or-later
Summary: The OpenMixer console's delay, drive, EQ, channel strip and keyed gate, as plugins for your DAW
URL: https://github.com/FreeMixer/omx-plugins

Source0: %{url}/archive/v%{version}/%{name}-%{version}.tar.gz

BuildRequires: gcc
BuildRequires: make
BuildRequires: pkgconf-pkg-config
BuildRequires: omx-dsp-devel >= 0.2.0
BuildRequires: omx-clap-core-devel
BuildRequires: clap-devel
BuildRequires: lv2-devel
# %%check: the generators and checks are Node scripts; the hosts that load the built plugins
BuildRequires: nodejs
# the tests check the generated files against omx-contract's release, fetched once (tools/omx-contract.mjs)
BuildRequires: curl
BuildRequires: tar
BuildRequires: gzip
BuildRequires: lilv
BuildRequires: lilv-devel
BuildRequires: sord
BuildRequires: omx-clap-host

# The name a user installs: both formats.
Requires: %{name}-clap%{?_isa} = %{version}-%{release}
Requires: %{name}-lv2%{?_isa} = %{version}-%{release}

%description
Take the sound of the OpenMixer console into your DAW or onto your own rig:
delay, drive, the channel EQ as an 8-band EQ, a 16-band EQ or a 32-band EQ, the
full channel strip and the keyed gate with its sidechain input, as CLAP plugins
for Bitwig, REAPER and Carla, and as LV2 plugins for Ardour, Carla, MOD and
Zynthian. Each one runs the same DSP as the console, so a track sounds the same
in a session as it does on the desk, sample for sample. Every plugin comes in
both formats with the same controls, MOD GUI included.

This package installs both formats; omx-plugins-clap and omx-plugins-lv2
install one each.

%package clap
Summary: OpenMixer delay, drive, EQ, channel strip and keyed gate as CLAP plugins

%description clap
The OpenMixer console's delay, drive, 8-, 16- and 32-band EQ, channel strip and
keyed gate as CLAP plugins, installed in /usr/lib/clap where Bitwig, REAPER,
Carla and other CLAP hosts find them. They run the console's own DSP, so they
sound like the desk.

%package lv2
Summary: OpenMixer delay, drive, EQ, channel strip and keyed gate as LV2 plugins
Requires: lv2

%description lv2
The OpenMixer console's delay, drive, 8-, 16- and 32-band EQ, channel strip and
keyed gate as LV2 plugins, installed in %{_libdir}/lv2 where Ardour, Carla, MOD
and Zynthian find them, with a MOD GUI where the plugin has one. They run the
console's own DSP, so they sound like the desk.

%package devel
Summary: Headers of the OpenMixer plugins, for projects that build on them
Requires: omx-dsp-devel >= 0.2.0

%description devel
The headers other projects include to use the OpenMixer plugins' parameters and
instance code, such as omx_delay_instance.h and each plugin's generated
parameter header, installed in %{_includedir}/omx-plugins.

%prep
%autosetup

%build
%set_build_flags
%make_build

%install
%make_install PREFIX=%{_prefix} LIBDIR=%{_libdir} CLAPDIR=/usr/lib/clap
%{__make} install-devel DESTDIR=%{buildroot} PREFIX=%{_prefix}

%check
%make_build test

%files
%license LICENSE
%doc README.md

%files clap
%license LICENSE
%doc README.md
%dir /usr/lib/clap
/usr/lib/clap/omx-delay.clap
/usr/lib/clap/omx-drive.clap
/usr/lib/clap/omx-eq8.clap
/usr/lib/clap/omx-eq16.clap
/usr/lib/clap/omx-eq32.clap
/usr/lib/clap/omx-rotary.clap
/usr/lib/clap/omx-keyed-gate.clap
/usr/lib/clap/omx-strip.clap

%files devel
%license LICENSE
%{_includedir}/omx-plugins/

%files lv2
%license LICENSE
%doc README.md
%{_libdir}/lv2/omx-delay.lv2/
%{_libdir}/lv2/omx-drive.lv2/
%{_libdir}/lv2/omx-eq8.lv2/
%{_libdir}/lv2/omx-eq16.lv2/
%{_libdir}/lv2/omx-eq32.lv2/
%{_libdir}/lv2/omx-rotary.lv2/
%{_libdir}/lv2/omx-keyed-gate.lv2/
%{_libdir}/lv2/omx-strip.lv2/

%changelog
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
