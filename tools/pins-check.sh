#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#
# tools/pins-check.sh <pins-file> <clap-dir>
#
# The shipped omx-plugins.pins names, for every plugin, the SHA-256 of the .clap file plugin-hostd
# pins it to; this reads the pins file against the .clap files the same build just installed into
# <clap-dir> and refuses when an installed binary's hash does not match its pin, or when an installed
# binary has no pin_set line at all. The sabotage this guards: a binary swapped after the pins were
# written, or a build that shipped a pins file stale against the binaries it now carries.
#
#   tools/pins-check.sh --self-test   a two-plugin fixture: a match passes; a binary changed after the
#                                     pin was written, a pin altered to a hash the file does not have
#                                     and an installed .clap with no pin_set line each refuse
set -euo pipefail

if [ "${1:-}" = --self-test ]; then
  scratch=$(mktemp -d)
  trap 'rm -rf "$scratch"' EXIT
  mkdir -p "$scratch/clap"
  printf 'plugin a bytes\n' > "$scratch/clap/a.clap"
  printf 'plugin b bytes\n' > "$scratch/clap/b.clap"
  sha_a=$(sha256sum "$scratch/clap/a.clap" | cut -d' ' -f1)
  sha_b=$(sha256sum "$scratch/clap/b.clap" | cut -d' ' -f1)
  cat > "$scratch/ok.pins" <<EOF
# written by plugin-qualify --pins; plugin-hostd reads it with pin_file
pin_set clap:/usr/lib/clap/a.clap#org.example.a a.clap=$sha_a omx-layout/1:deadbeef verdict=accepted
pin_set clap:/usr/lib/clap/b.clap#org.example.b b.clap=$sha_b omx-layout/1:deadbeef verdict=refused
EOF
  fail=0
  run() { # <pins-file> <wanted-exit> <what> [<text the output must carry>]
    local rc=0 log
    log=$("${BASH_SOURCE[0]}" "$1" "$scratch/clap" 2>&1) || rc=$?
    if [ "$rc" = "$2" ]; then echo "ok   $3 (exit $rc)"; else echo "FAIL $3: exit $rc, wanted $2"; printf '%s\n' "$log" >&2; fail=1; fi
    if [ -n "${4:-}" ] && ! grep -qF -- "$4" <<<"$log"; then echo "FAIL $3: the output does not say '$4'"; printf '%s\n' "$log" >&2; fail=1; fi
  }
  run "$scratch/ok.pins" 0 "every pin matches the shipped binary"

  printf 'sabotaged bytes\n' > "$scratch/clap/a.clap"
  run "$scratch/ok.pins" 1 "a binary changed after the pin was written refuses" 'a.clap: pinned sha256'
  printf 'plugin a bytes\n' > "$scratch/clap/a.clap"

  sed "s/a\.clap=$sha_a/a.clap=00000000000000000000000000000000000000000000000000000000000000/" "$scratch/ok.pins" > "$scratch/altered.pins"
  run "$scratch/altered.pins" 1 "a pin naming a hash the file does not have refuses" 'a.clap: pinned sha256'

  grep -v '^pin_set clap:/usr/lib/clap/b.clap' "$scratch/ok.pins" > "$scratch/missing.pins"
  run "$scratch/missing.pins" 1 "an installed .clap with no pin_set line refuses" 'b.clap: no pin_set line'

  run "$scratch/no-such-file.pins" 2 "no pins file at all refuses" 'no pins file'

  [ "$fail" = 0 ] && echo "pins-check self-test ok"
  exit "$fail"
fi

PINS=${1:?usage: pins-check.sh <pins-file> <clap-dir> | --self-test}
CLAPDIR=${2:?usage: pins-check.sh <pins-file> <clap-dir> | --self-test}
[ -f "$PINS" ] || { echo "pins-check: no pins file at $PINS" >&2; exit 2; }

mapfile -t CLAPS < <(find "$CLAPDIR" -maxdepth 1 -name '*.clap' 2>/dev/null | sort)
[ "${#CLAPS[@]}" -gt 0 ] || { echo "pins-check: $CLAPDIR holds no .clap" >&2; exit 2; }

refused=0
for clap in "${CLAPS[@]}"; do
  base=$(basename "$clap")
  base_re=$(printf '%s' "$base" | sed 's/[.[\*^$]/\\&/g')
  line=$(grep '^pin_set ' "$PINS" | grep -E "[ ,]$base_re=" || true)
  if [ -z "$line" ]; then
    echo "REFUSED $base: no pin_set line in $PINS names it" >&2
    refused=1
    continue
  fi
  pinned=$(printf '%s\n' "$line" | sed -n "s/.*[ ,]$base_re=\([0-9a-f]\{64\}\).*/\1/p")
  actual=$(sha256sum "$clap" | cut -d' ' -f1)
  if [ "$pinned" != "$actual" ]; then
    echo "REFUSED $base: pinned sha256 $pinned, built sha256 $actual" >&2
    refused=1
  else
    echo "ok   $base: pinned sha256 matches the built binary"
  fi
done
[ "$refused" = 0 ] || exit 1
echo "pins-check: every shipped pin matches its binary (${#CLAPS[@]} plugins)"
