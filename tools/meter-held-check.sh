#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#
# meter-held-check.sh — a BUILT face holds its meters until the host reads them, through the whole path (omx-comp's
# CLAP face, lib/omx_clap_face.c, include/omx_meter_ring.h), under tools/test/meter-held-face.c: a 10 ms burst
# followed by twenty silent blocks is still in the one read that comes after, and the read after that has recovered.
#
#   tools/meter-held-check.sh              build omx-comp and run it
#   tools/meter-held-check.sh --self-test  the same over a copy of the tree whose ring holds nothing: it must go red
set -euo pipefail
HERE="$(cd "$(dirname "$0")/.." && pwd)"
CC="${CC:-cc}"
PKG_CONFIG="${PKG_CONFIG:-pkg-config}"
scratch="$(mktemp -d)"; trap 'rm -rf "$scratch"' EXIT

run() { # <tree> <label>
  make -s -C "$1/plugins/omx-comp" all >/dev/null
  # shellcheck disable=SC2046
  $CC -std=gnu11 -O2 -Wall -Wextra -Werror $($PKG_CONFIG --cflags omx-clap-core clap 2>/dev/null || true) -I"$1/include" \
    -o "$scratch/held-face" "$HERE/tools/test/meter-held-face.c" -ldl -lm
  "$scratch/held-face" "$1/plugins/omx-comp/build/omx-comp.clap"
}

if [ "${1:-}" = --self-test ]; then
  tree="$scratch/tree"; mkdir -p "$tree"
  (cd "$HERE" && git ls-files -z | tar --null -T - -cf -) | tar -C "$tree" -xf -
  h="$tree/include/omx_meter_ring.h"
  fail=0
  # each arm: the ring built without one of the things the face relies on
  for arm in no-hold no-restart; do
    cp "$HERE/include/omx_meter_ring.h" "$h"
    case $arm in
      no-hold) sed -i 's/^    if (restart || !((r->hold_mask >> i) \& 1u)) {$/    if (1 || restart) {/' "$h" ;;
      no-restart) sed -i 's/^    const int restart = .*$/    const int restart = 0;/' "$h" ;;
    esac
    cmp -s "$h" "$HERE/include/omx_meter_ring.h" && { echo "SABOTAGE $arm: did not change the header"; fail=1; continue; }
    rm -rf "$tree/plugins/omx-comp/build"
    if run "$tree" "$arm" > "$scratch/$arm.log" 2>&1; then echo "SABOTAGE $arm: STAYED GREEN"; cat "$scratch/$arm.log"; fail=1
    else echo "SABOTAGE $arm: red ($(grep -m1 '^FAIL' "$scratch/$arm.log"))"; fi
  done
  [ "$fail" = 0 ] || exit 1
  echo "meter-held-check self-test: every sabotage red"
  exit 0
fi
run "$HERE" real
