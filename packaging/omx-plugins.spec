# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
Name: omx-plugins
Version: 0.1.0
Release: 1%{?dist}
License: GPL-3.0-or-later
Summary: The effects of the OpenMixer console as CLAP and LV2 plugins
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
The effects of the OpenMixer console as plugins for any host: CLAP for
Bitwig, REAPER, Carla and every other CLAP host, LV2 for Ardour, Carla, MOD
and Zynthian. The DSP inside each one is the console's own (omx-dsp, linked
in statically), so a plugin sounds the same in a DAW as on the desk, sample
for sample. Each plugin is drawn from one declaration, so its CLAP
parameters, its LV2 ports and its MOD GUI cannot disagree.

Plugins: omx delay, omx drive, omx eq8 and omx strip. This package installs
both formats; omx-plugins-clap and omx-plugins-lv2 install one each.

%package clap
Summary: The OpenMixer console's effects as CLAP plugins

%description clap
omx delay, omx drive, omx eq8 and omx strip as CLAP plugins, in
%{_libdir}/clap where CLAP hosts find them. The DSP is the OpenMixer
console's own.

%package lv2
Summary: The OpenMixer console's effects as LV2 plugins
Requires: lv2

%description lv2
omx delay, omx drive, omx eq8 and omx strip as LV2 plugins, in
%{_libdir}/lv2 where LV2 hosts find them, with a MOD GUI where the plugin
declares one. The DSP is the OpenMixer console's own.

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
- First package: omx delay, omx drive, omx eq8 and omx strip, each as CLAP and
  LV2, over omx-dsp 0.1.4.
