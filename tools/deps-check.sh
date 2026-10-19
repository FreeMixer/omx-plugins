#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#
# deps-check.sh — the releases this repository builds against are declared only in the packaging, as a
# FLOOR and nothing above it (operator ruling 2026-10-15: library dependencies carry no ceiling):
# packaging/omx-plugins.spec's BuildRequires (one `>=` line per dependency, no `<`), the -devel package's
# Requires on omx-dsp and omx-contract, and debian/control's Build-Depends and the -dev package's Depends, written the same.
# A ceiling anywhere is a failure.
#
#   tools/deps-check.sh              the check
#   tools/deps-check.sh --self-test  each sabotage must go red
set -euo pipefail
HERE="$(cd "$(dirname "$0")/.." && pwd)"

# how many times the fixed string $1 occurs in file $2 (several on one line count each)
count() { { grep -oF -- "$1" "$2" || true; } | wc -l; }
check() {
  local root="$1" fail=0 spec="$1/packaging/omx-plugins.spec" control="$1/debian/control"
  # rpm name, deb name, how many times debian/control must carry the deb floor, does -devel require it
  for row in "omx-dsp-devel libomxdsp-dev 2 yes" "omx-contract-devel libomx-contract-dev 2 yes" "omx-clap-core-devel libomx-clap-core-dev 1 no"; do
    read -r rpm deb times devel <<<"$row"
    local floor
    floor="$(sed -n "s/^BuildRequires: $rpm >= \([0-9.]*\)\$/\1/p" "$spec")"
    if [ "$(wc -l <<<"$floor")" != 1 ] || [ -z "$floor" ]; then
      echo "deps-check: the spec needs exactly one 'BuildRequires: $rpm >= <floor>' line" >&2; fail=1; continue
    fi
    if grep -qE "^(BuildRequires|Requires): $rpm *(<|<=|=|==) " "$spec"; then
      echo "deps-check: the spec bounds $rpm from above (or pins it): a floor only, no ceiling" >&2; fail=1
    fi
    if [ "$devel" = yes ]; then
      grep -qx "Requires: $rpm >= $floor" "$spec" ||
        { echo "deps-check: the -devel package does not require $rpm >= $floor" >&2; fail=1; }
    fi
    [ "$(count "$deb (>= $floor)" "$control")" = "$times" ] ||
      { echo "deps-check: debian/control does not carry $deb (>= $floor) $times time(s)" >&2; fail=1; }
    [ "$(count "$deb (" "$control")" = "$times" ] || { echo "deps-check: debian/control names $deb with another range too (a ceiling?)" >&2; fail=1; }
    [ "$fail" = 1 ] || echo "deps-check: $rpm / $deb >= $floor, no ceiling, in the spec and debian/control"
  done
  if grep -qE '^Requires:.*omxdsp *<' "$root/lib/omxplugins.pc.in"; then echo "deps-check: omxplugins.pc.in bounds omxdsp from above" >&2; fail=1; fi
  return "$fail"
}

if [ "${1:-}" = --self-test ]; then
  scratch="$(mktemp -d)"; trap 'rm -rf "$scratch"' EXIT
  fail=0
  for s in dsp-floor dsp-ceiling-spec devel-ceiling control-dsp control-ceiling pc-ceiling contract-missing; do
    rm -rf "$scratch/t"; mkdir -p "$scratch/t/packaging" "$scratch/t/debian"
    mkdir -p "$scratch/t/lib"; cp "$HERE/lib/omxplugins.pc.in" "$scratch/t/lib/"
    cp "$HERE/packaging/omx-plugins.spec" "$scratch/t/packaging/"; cp "$HERE/debian/control" "$scratch/t/debian/"
    case "$s" in
      dsp-floor) sed -i 's/^BuildRequires: omx-dsp-devel >= .*/BuildRequires: omx-dsp-devel >= 0.0.1/' "$scratch/t/packaging/omx-plugins.spec" ;;
      dsp-ceiling-spec) sed -i 's/^BuildRequires: omx-dsp-devel >= .*/&\nBuildRequires: omx-dsp-devel < 1/' "$scratch/t/packaging/omx-plugins.spec" ;;
      devel-ceiling) sed -i 's/^Requires: omx-dsp-devel >= .*/&\nRequires: omx-dsp-devel < 1/' "$scratch/t/packaging/omx-plugins.spec" ;;
      control-dsp) sed -i '0,/libomxdsp-dev (>= [0-9.]*)/s//libomxdsp-dev (>= 0.0.1)/' "$scratch/t/debian/control" ;;
      control-ceiling) sed -i 's/^\( *\)libomx-contract-dev (>= [0-9.]*),/&\n\1libomx-contract-dev (<< 3),/' "$scratch/t/debian/control" ;;
      pc-ceiling) sed -i 's/^Requires: omxdsp >= @OMXDSP_MIN@/&, omxdsp < 1/' "$scratch/t/lib/omxplugins.pc.in" ;;
      contract-missing) sed -i '/^BuildRequires: omx-contract-devel /d' "$scratch/t/packaging/omx-plugins.spec" ;;
    esac
    if check "$scratch/t" >/dev/null 2>&1; then echo "SABOTAGE $s: STAYED GREEN"; fail=1; else echo "SABOTAGE $s: red"; fi
  done
  check "$HERE" >/dev/null || { echo "self-test: the unsabotaged check is not green" >&2; exit 1; }
  [ "$fail" = 0 ] || exit 1
  echo "deps-check self-test: every sabotage red"
  exit 0
fi

check "$HERE"
