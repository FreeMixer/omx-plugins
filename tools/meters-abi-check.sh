#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#
# meters-abi-check.sh — include/omx_clap_meters.h re-declares the clap.meters/1 draft because
# omx-clap-core does not install its src/clap_meters.h (FreeMixer/omx-plugins#119). Two copies of
# an ABI must not drift, so this compiles the layout probe against both and demands the same
# extension id, enum values, sizeof and offsetof for every structure.
#
#   tools/meters-abi-check.sh              the check
#   tools/meters-abi-check.sh --self-test  a reordered field and a changed enum must each go red
#
# omx-clap-core's header comes from, in order: $OMX_CLAP_CORE_METERS (a path), the tag of the
# installed omx-clap-core (pkg-config) fetched from GitHub. The CLAP SDK headers come from pkg-config
# `clap`, or $CLAP_INCLUDE.
set -euo pipefail
HERE="$(cd "$(dirname "$0")/.." && pwd)"
CC="${CC:-cc}"
scratch="$(mktemp -d)"; trap 'rm -rf "$scratch"' EXIT

clap_cflags() {
  if [ -n "${CLAP_INCLUDE-}" ]; then echo "-I$CLAP_INCLUDE"; else pkg-config --cflags clap; fi
}

core_header() {
  if [ -n "${OMX_CLAP_CORE_METERS-}" ]; then echo "$OMX_CLAP_CORE_METERS"; return; fi
  local v; v="$(pkg-config --modversion omx-clap-core)"
  curl -fsSL -o "$scratch/core_meters.h" "https://raw.githubusercontent.com/FreeMixer/omx-clap-core/v$v/src/clap_meters.h"
  echo "$scratch/core_meters.h"
}

layout() { # <header> <out>
  # shellcheck disable=SC2046
  $CC -std=gnu11 -Wall -Wextra -Werror $(clap_cflags) -DMETERS_HEADER="\"$1\"" \
    -o "$scratch/probe" "$HERE/tools/test/meters-abi-probe.c"
  "$scratch/probe" > "$2"
}

compare() { # <ours> <core>
  layout "$1" "$scratch/ours.txt"
  layout "$2" "$scratch/core.txt"
  if diff -u --label "omx-plugins $1" --label "omx-clap-core $2" "$scratch/ours.txt" "$scratch/core.txt"; then
    echo "meters-abi-check: $(wc -l < "$scratch/ours.txt") facts of clap.meters/1 agree between the two headers"
  else
    echo "meters-abi-check: the two clap.meters/1 declarations differ" >&2
    return 1
  fi
}

core="$(core_header)"

if [ "${1:-}" = --self-test ]; then
  fail=0
  compare "$HERE/include/omx_clap_meters.h" "$core" >/dev/null || { echo "self-test: the unsabotaged check is not green" >&2; exit 1; }
  # sabotage 1: two fields swapped in the info struct
  sed 's/^    uint32_t kind; /    uint32_t flags_;\n    uint32_t kind; /' "$core" > "$scratch/s1.h"
  # sabotage 2: an enum value moved
  sed 's/CLAP_METER_KIND_LEVEL = 2/CLAP_METER_KIND_LEVEL = 3/' "$core" > "$scratch/s2.h"
  for s in s1 s2; do
    cmp -s "$core" "$scratch/$s.h" && { echo "SABOTAGE $s: did not change the header"; fail=1; continue; }
    if compare "$HERE/include/omx_clap_meters.h" "$scratch/$s.h" >/dev/null 2>&1; then echo "SABOTAGE $s: STAYED GREEN"; fail=1; else echo "SABOTAGE $s: red"; fi
  done
  [ "$fail" = 0 ] || exit 1
  echo "meters-abi-check self-test: every sabotage red"
  exit 0
fi

compare "$HERE/include/omx_clap_meters.h" "$core"
