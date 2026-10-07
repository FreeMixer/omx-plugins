# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
Name: omx-plugins
Version: 0.1.0
Release: 1%{?dist}
License: GPL-3.0-or-later
Summary: The OpenMixer console's delay, drive, EQ and channel strip, as plugins for your DAW
URL: https://github.com/FreeMixer/omx-plugins

Source0: %{url}/archive/v%{version}/%{name}-%{version}.tar.gz

BuildRequires: gcc
BuildRequires: make
BuildRequires: pkgconf-pkg-config
BuildRequires: omx-dsp-devel >= 0.1.4
BuildRequires: omx-clap-core-devel
BuildRequires: clap-devel
BuildRequires: lv2-devel
# %%check: the generators and checks are Node scripts; the hosts that load the built plugins
BuildRequires: nodejs
BuildRequires: lilv
BuildRequires: lilv-devel
BuildRequires: sord
BuildRequires: omx-clap-host

# The name a user installs: both formats.
Requires: %{name}-clap%{?_isa} = %{version}-%{release}
Requires: %{name}-lv2%{?_isa} = %{version}-%{release}

%description
Take the sound of the OpenMixer console into your DAW or onto your own rig:
delay, drive, an 8-band EQ and the full channel strip, as CLAP plugins for
Bitwig, REAPER and Carla, and as LV2 plugins for Ardour, Carla, MOD and
Zynthian. Each one runs the same DSP as the console, so a track sounds the
same in a session as it does on the desk, sample for sample. Every plugin
comes in both formats with the same controls, MOD GUI included.

This package installs both formats; omx-plugins-clap and omx-plugins-lv2
install one each.

%package clap
Summary: OpenMixer delay, drive, EQ and channel strip as CLAP plugins

%description clap
The OpenMixer console's delay, drive, 8-band EQ and channel strip as CLAP
plugins, installed in %{_libdir}/clap where Bitwig, REAPER, Carla and other CLAP
hosts find them. They run the console's own DSP, so they sound like the desk.

%package lv2
Summary: OpenMixer delay, drive, EQ and channel strip as LV2 plugins
Requires: lv2

%description lv2
The OpenMixer console's delay, drive, 8-band EQ and channel strip as LV2
plugins, installed in %{_libdir}/lv2 where Ardour, Carla, MOD and Zynthian find
them, with a MOD GUI where the plugin has one. They run the console's own DSP,
so they sound like the desk.

%prep
%autosetup

%build
%set_build_flags
%make_build

%install
%make_install PREFIX=%{_prefix} LIBDIR=%{_libdir}

%check
%make_build test

%files
%license LICENSE
%doc README.md

%files clap
%license LICENSE
%doc README.md
%dir %{_libdir}/clap
%{_libdir}/clap/omx-delay.clap
%{_libdir}/clap/omx-drive.clap
%{_libdir}/clap/omx-eq8.clap
%{_libdir}/clap/omx-strip.clap

%files lv2
%license LICENSE
%doc README.md
%{_libdir}/lv2/omx-delay.lv2/
%{_libdir}/lv2/omx-drive.lv2/
%{_libdir}/lv2/omx-eq8.lv2/
%{_libdir}/lv2/omx-strip.lv2/

%changelog
* Wed Oct 07 2026 Pau Aliagas <linuxnow@gmail.com> - 0.1.0-1
- First package: the OpenMixer console's delay, drive, 8-band EQ and channel
  strip, each as a CLAP and an LV2 plugin, running the same DSP as the console
  (omx-dsp 0.1.4).
