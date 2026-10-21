#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#
# round-trip.sh [--self-test] — author, generate both faces, scan them back through plugin-hostd, compare
# (docs/declaration-as-descriptor.md, "The round trip"; FreeMixer/omx-plugins#210, ruling B). For every plugin with a
# descriptor: `node tools/gen.mjs`, build both faces, `plugin-hostd-probe describe` each built face, and
# tools/round-trip.mjs compares the scanned descriptors with the authored ones word by word (equal, lost <format>
# <word>, differs). --self-test breaks the first described plugin twice and wants each red: one range changed in a
# copy of its authored descriptor, and one lv2:maximum changed in a copy of its built .ttl. Scratch lives under
# build/tmp. Exit 1 when it does not pass, or an arm stays green.
set -u
ROOT=$(cd "$(dirname "$0")/.." && pwd)
PROBE=${PLUGIN_HOSTD_PROBE:-plugin-hostd-probe}
command -v "$PROBE" >/dev/null 2>&1 || { echo "FAIL round-trip: no $PROBE on PATH (plugin-hostd >= 0.7.0, its describe verb)"; exit 1; }
mkdir -p "$ROOT/build/tmp"
TMP=$(mktemp -d "$ROOT/build/tmp/round-trip.XXXXXX") || exit 1
trap 'rm -rf "$TMP"' EXIT INT TERM
field() { node -e 'console.log(JSON.parse(require("fs").readFileSync(process.argv[1], "utf8")).plugin[process.argv[2]])' "$1" "$2"; }
stems=$(cd "$ROOT" && node --input-type=module -e 'import { described } from "./tools/round-trip.mjs"; console.log(described().inside.join(" "))') || exit 1
[ -n "$stems" ] || { echo "FAIL round-trip: no plugin is described"; exit 1; }

(cd "$ROOT" && node tools/gen.mjs >/dev/null) || { echo "FAIL round-trip: node tools/gen.mjs"; exit 1; }
mkdir -p "$TMP/scans"
for stem in $stems; do
  P=$ROOT/plugins/$stem
  D=$P/$stem.descriptor.json
  make -s -C "$P" all >"$TMP/build.txt" 2>&1 || { cat "$TMP/build.txt"; echo "FAIL round-trip $stem: the faces do not build"; exit 1; }
  "$PROBE" describe "clap:$P/build/$stem.clap#$(field "$D" id)" >"$TMP/scans/$stem.clap.json" || { echo "FAIL round-trip $stem: describe clap"; exit 1; }
  LV2_PATH=$P/build "$PROBE" describe "lv2:$(field "$D" uri)" >"$TMP/scans/$stem.lv2.json" || { echo "FAIL round-trip $stem: describe lv2"; exit 1; }
done

if [ "${1:-}" != "--self-test" ]; then
  node "$ROOT/tools/round-trip.mjs" "$TMP/scans"
  exit $?
fi

# the arms, on the first described plugin
fails=0
stem=${stems%% *}
P=$ROOT/plugins/$stem
red() { # red <name> <word> <output file>: the run failed, on a differs of that word
  if grep -q "^differs .* $2:" "$3" && grep -q '^FAIL round-trip' "$3"; then echo "SABOTAGE $1: red ($(grep -m1 "^differs .* $2:" "$3" | cut -c1-140))"; else echo "FAIL round-trip self-test: $1 stays green"; fails=$((fails + 1)); fi
}
node -e '
const fs = require("fs"), d = JSON.parse(fs.readFileSync(process.argv[1], "utf8"));
d.controls.find((x) => x.kind === "travel").travel.max += 1;
fs.writeFileSync(process.argv[2], JSON.stringify(d));
' "$P/$stem.descriptor.json" "$TMP/moved.json"
node "$ROOT/tools/round-trip.mjs" "$TMP/scans" --authored "$stem=$TMP/moved.json" >"$TMP/arm1.txt" 2>&1
red "a range changed in the authored descriptor" 'controls\[\]\.travel\.max' "$TMP/arm1.txt"
mkdir -p "$TMP/lv2"
cp -r "$P/build/$stem.lv2" "$TMP/lv2/"
sed -i '0,/lv2:maximum \([0-9.e+-]*\) ;/s//lv2:maximum 12345 ;/' "$TMP/lv2/$stem.lv2/$stem.ttl"
cp -r "$TMP/scans" "$TMP/scans2"
LV2_PATH=$TMP/lv2 "$PROBE" describe "lv2:$(field "$P/$stem.descriptor.json" uri)" >"$TMP/scans2/$stem.lv2.json" || exit 1
node "$ROOT/tools/round-trip.mjs" "$TMP/scans2" >"$TMP/arm2.txt" 2>&1
red "an lv2:maximum changed in the built .ttl" 'controls\[\]\.travel\.max' "$TMP/arm2.txt"
[ "$fails" -eq 0 ] && echo "round-trip self-test: every sabotage red"
exit $((fails > 0))
