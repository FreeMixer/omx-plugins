#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#
# pins-check.sh — the releases this repository builds against are written in .github/pins.txt and
# nowhere else but the packaging that has to repeat them as a requirement; this check holds the
# repetitions equal to the pin, and the omx-contract the pinned omx-dsp requires equal to ours.
#
#   tools/pins-check.sh            the check
#   tools/pins-check.sh --self-test  each sabotage must go red
set -euo pipefail
HERE="$(cd "$(dirname "$0")/.." && pwd)"

check() { # <root>
  local root="$1" fail=0 dsp contract
  dsp="$(awk '$1 == "omx-dsp" { print $3 }' "$root/.github/pins.txt")"
  contract="$(awk '$1 == "omx-contract" { print $3 }' "$root/.github/pins.txt")"
  [ -n "$dsp" ] && [ -n "$contract" ] || { echo "pins-check: .github/pins.txt names no omx-dsp or no omx-contract" >&2; return 1; }
  grep -qx "BuildRequires: omx-dsp-devel >= $dsp" "$root/packaging/omx-plugins.spec" || { echo "pins-check: the spec's BuildRequires omx-dsp-devel is not >= $dsp" >&2; fail=1; }
  grep -qx "Requires: omx-dsp-devel >= $dsp" "$root/packaging/omx-plugins.spec" || { echo "pins-check: the spec's Requires omx-dsp-devel is not >= $dsp" >&2; fail=1; }
  [ "$(grep -c "libomxdsp-dev (>= $dsp)" "$root/debian/control")" = 2 ] || { echo "pins-check: debian/control does not require libomxdsp-dev (>= $dsp) twice" >&2; fail=1; }
  if [ -n "${PINS_CHECK_PKG-1}" ] && command -v pkg-config >/dev/null 2>&1 && pkg-config --exists omxdsp 2>/dev/null; then
    have="$(pkg-config --modversion omxdsp)"
    req="$(pkg-config --print-requires omxdsp | tr -d ' ')"
    printf '%s\n' "$req" | grep -qx "omx-contract=$contract" || { echo "pins-check: the installed omx-dsp $have requires $(printf '%s' "$req" | tr '\n' ' '), the pin is omx-contract $contract" >&2; fail=1; }
  fi
  [ "$fail" = 0 ] || return 1
  echo "pins-check: omx-dsp >= $dsp and omx-contract $contract agree in the pins, the spec and debian/control"
}

if [ "${1:-}" = --self-test ]; then
  scratch="$(mktemp -d)"; trap 'rm -rf "$scratch"' EXIT
  fail=0
  for s in spec-buildrequires spec-requires control pin; do
    rm -rf "$scratch/t"; mkdir -p "$scratch/t/.github" "$scratch/t/packaging" "$scratch/t/debian"
    cp "$HERE/.github/pins.txt" "$scratch/t/.github/"; cp "$HERE/packaging/omx-plugins.spec" "$scratch/t/packaging/"; cp "$HERE/debian/control" "$scratch/t/debian/"
    case "$s" in
      spec-buildrequires) sed -i 's/^BuildRequires: omx-dsp-devel >= .*/BuildRequires: omx-dsp-devel >= 0.0.1/' "$scratch/t/packaging/omx-plugins.spec" ;;
      spec-requires) sed -i 's/^Requires: omx-dsp-devel >= .*/Requires: omx-dsp-devel >= 0.0.1/' "$scratch/t/packaging/omx-plugins.spec" ;;
      control) sed -i '0,/libomxdsp-dev (>= [0-9.]*)/s//libomxdsp-dev (>= 0.0.1)/' "$scratch/t/debian/control" ;;
      pin) sed -i 's/^omx-dsp \(.*\) [0-9.]*$/omx-dsp \1 0.0.1/' "$scratch/t/.github/pins.txt" ;;
    esac
    if PINS_CHECK_PKG= check "$scratch/t" >/dev/null 2>&1; then echo "SABOTAGE $s: STAYED GREEN"; fail=1; else echo "SABOTAGE $s: red"; fi
  done
  PINS_CHECK_PKG= check "$HERE" >/dev/null || { echo "self-test: the unsabotaged check is not green" >&2; exit 1; }
  [ "$fail" = 0 ] || exit 1
  echo "pins-check self-test: every sabotage red"
  exit 0
fi
check "$HERE"
