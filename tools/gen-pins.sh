#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#
# tools/gen-pins.sh <destdir> <clapdir> <pins-dir> <pins-file>
#
# The pin for each of our own plugins is known at build time, so it ships with the package instead of
# being recorded by plugin-hostd on first use: this runs plugin-qualify --pins over every .clap this
# build just installed and writes the result at <destdir><pins-dir>/<pins-file>, with the buildroot
# prefix (<destdir><clapdir>) rewritten to the path the plugin has once the package lands (<clapdir>),
# so the uri plugin-hostd looks a pin up by is the one it hosts the plugin from.
#
# Needs plugin-qualify >= 0.9.0 (the first release with --pins). Exit: 0 written, 2 the gate could not run
# (no plugin-qualify, no .clap installed, or plugin-qualify itself could not measure, its own exit 2).
# A plugin plugin-qualify rates unsuitable still gets a line, verdict=refused: the point of shipping the
# pin is that plugin-hostd refuses it too, not that the build refuses to ship it (tools/qualify-release.sh,
# the release gate, is what stops an unsuitable plugin from being released at all).
#
#   tools/gen-pins.sh --self-test   a stand-in plugin-qualify over a two-plugin stage: the written pins
#                                   file names the final CLAPDIR, never the buildroot that staged it
set -euo pipefail

if [ "${1:-}" = --self-test ]; then
  scratch=$(mktemp -d)
  trap 'rm -rf "$scratch"' EXIT
  mkdir -p "$scratch/bin" "$scratch/stage/usr/lib/clap"
  printf 'plugin a\n' > "$scratch/stage/usr/lib/clap/a.clap"
  # the stand-in prober: echoes its --pins target and the claps it was given, as plugin-qualify's own
  # pin_set lines would name them (the literal paths it was invoked with, buildroot included)
  cat > "$scratch/bin/plugin-qualify" <<'STUB'
#!/usr/bin/env bash
out=
while [ $# -gt 0 ]; do case "$1" in --pins) out=$2; shift 2 ;; *) break ;; esac; done
{
  echo "# written by plugin-qualify --pins; plugin-hostd reads it with pin_file"
  for clap in "$@"; do
    sha=$(sha256sum "$clap" | cut -c1-64)
    echo "pin_set clap:$clap#org.example.a a.clap=$sha omx-layout/1:deadbeef verdict=accepted"
  done
} > "$out"
exit 0
STUB
  chmod +x "$scratch/bin/plugin-qualify"
  fail=0
  log=$(PATH="$scratch/bin:$PATH" "${BASH_SOURCE[0]}" "$scratch/stage" /usr/lib/clap /usr/share/plugin-hostd/pins.d omx-plugins.pins 2>&1) || { echo "FAIL gen-pins ran: exit $?"; printf '%s\n' "$log" >&2; fail=1; }
  pins="$scratch/stage/usr/share/plugin-hostd/pins.d/omx-plugins.pins"
  if [ -f "$pins" ] && grep -qF "clap:/usr/lib/clap/a.clap#org.example.a" "$pins" && ! grep -qF "$scratch/stage" "$pins"; then
    echo "ok   the written pin names the final CLAPDIR, not the buildroot that staged it"
  else
    echo "FAIL the written pin does not name the final CLAPDIR cleanly"; cat "$pins" 2>&1 >&2 || true; fail=1
  fi
  log=$(PATH="$scratch/bin:$PATH" "${BASH_SOURCE[0]}" "$scratch/stage" /usr/lib/clap /usr/share/plugin-hostd/pins.d omx-plugins.pins 2>&1) && { echo "ok   a second run overwrites the pins file rather than appending"; } || { echo "FAIL a second run refused"; fail=1; }
  if [ "$(grep -c '^pin_set ' "$pins")" = 1 ]; then echo "ok   still one pin, not appended"; else echo "FAIL the pins file grew on a second run"; fail=1; fi
  rm -rf "$scratch/stage/usr/lib/clap"
  log=$(PATH="$scratch/bin:$PATH" "${BASH_SOURCE[0]}" "$scratch/stage" /usr/lib/clap /usr/share/plugin-hostd/pins.d omx-plugins.pins 2>&1) && rc=0 || rc=$?
  if [ "$rc" = 2 ] && grep -qF 'holds no .clap' <<<"$log"; then echo "ok   no .clap installed refuses (exit 2)"; else echo "FAIL no .clap installed: exit $rc"; printf '%s\n' "$log" >&2; fail=1; fi
  [ "$fail" = 0 ] && echo "gen-pins self-test ok"
  exit "$fail"
fi

DESTDIR=${1-} CLAPDIR=${2:?usage: gen-pins.sh <destdir> <clapdir> <pins-dir> <pins-file> | --self-test}
PINS_DIR=${3:?usage: gen-pins.sh <destdir> <clapdir> <pins-dir> <pins-file> | --self-test}
PINS_FILE=${4:?usage: gen-pins.sh <destdir> <clapdir> <pins-dir> <pins-file> | --self-test}

command -v plugin-qualify >/dev/null || { echo "gen-pins: plugin-qualify is not installed; install it (>= 0.9.0) from the channel" >&2; exit 2; }

[ -d "$DESTDIR$CLAPDIR" ] || { echo "gen-pins: $DESTDIR$CLAPDIR holds no .clap; the tree built no plugin" >&2; exit 2; }
# plugin-qualify resolves every target to an absolute path before it writes the pin's uri, so the
# prefix sed rewrites below has to be the same absolute path, not whatever relative form was passed in
STAGE_CLAPDIR=$(cd "$DESTDIR$CLAPDIR" && pwd)
mapfile -t CLAPS < <(find "$STAGE_CLAPDIR" -maxdepth 1 -name '*.clap' 2>/dev/null | sort)
[ "${#CLAPS[@]}" -gt 0 ] || { echo "gen-pins: $STAGE_CLAPDIR holds no .clap; the tree built no plugin" >&2; exit 2; }

OUT_DIR="$DESTDIR$PINS_DIR"
install -d "$OUT_DIR"
TMP=$(mktemp)
trap 'rm -f "$TMP"' EXIT
rc=0
plugin-qualify --pins "$TMP" "${CLAPS[@]}" || rc=$?
[ "$rc" -ne 2 ] || { echo "gen-pins: plugin-qualify could not measure (its own exit 2)" >&2; exit 2; }

# the buildroot never ships; once the package lands, each .clap sits at $CLAPDIR
sed "s#clap:$STAGE_CLAPDIR/#clap:$CLAPDIR/#g" "$TMP" > "$OUT_DIR/$PINS_FILE"
echo "gen-pins: $PINS_FILE written, $(grep -c '^pin_set ' "$OUT_DIR/$PINS_FILE") pin(s) for ${#CLAPS[@]} binaries"
