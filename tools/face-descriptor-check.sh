#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#
# face-descriptor-check.sh [--self-test] — every described plugin's BUILT faces, scanned as a host scans
# anyone's plugin (tools/face-scan.c), against its authored descriptor (tools/face-descriptor-check.mjs):
# any difference is contract.descriptor.face-differs. Run after the plugins are built (`make test` runs
# it after their own tests). --self-test breaks the first described plugin twice and wants each red:
# one range changed in a copy of its authored descriptor, and one in a copy of its built LV2 bundle's
# .ttl. Scratch lives under build/tmp. Exit 1 on a difference or an arm that stays green.
set -u
ROOT=$(cd "$(dirname "$0")/.." && pwd)
PKG_CONFIG=${PKG_CONFIG:-pkg-config}
SCAN=$ROOT/build/face-scan
mkdir -p "$ROOT/build/tmp"
${CC:-cc} -std=gnu11 -O2 -Wall -Wextra -Werror $($PKG_CONFIG --cflags clap lilv-0) -o "$SCAN" "$ROOT/tools/face-scan.c" $($PKG_CONFIG --libs lilv-0) -ldl || exit 1
TMP=$(mktemp -d "$ROOT/build/tmp/face-check.XXXXXX") || exit 1
trap 'rm -rf "$TMP"' EXIT INT TERM
uri_of() { node -e 'console.log(JSON.parse(require("fs").readFileSync(process.argv[1], "utf8")).plugin.uri)' "$1"; }
fails=0
described=""
for d in "$ROOT"/plugins/*/; do
  stem=$(basename "$d")
  [ -f "$d$stem.descriptor.json" ] && described="$described $stem"
done
[ -n "$described" ] || { echo "FAIL face-descriptor: no plugin is described"; exit 1; }

if [ "${1:-}" != "--self-test" ]; then
  for stem in $described; do
    P=$ROOT/plugins/$stem
    if [ ! -f "$P/build/$stem.clap" ] || [ ! -d "$P/build/$stem.lv2" ]; then
      echo "FAIL face-descriptor $stem: no built faces in plugins/$stem/build (build the plugin first)"
      fails=$((fails + 1))
      continue
    fi
    "$SCAN" clap "$P/build/$stem.clap" >"$TMP/$stem.clap.txt" && node "$ROOT/tools/face-descriptor-check.mjs" "$P" clap "$TMP/$stem.clap.txt" || fails=$((fails + 1))
    "$SCAN" lv2 "$P/build/$stem.lv2" "$(uri_of "$P/$stem.descriptor.json")" >"$TMP/$stem.lv2.txt" && node "$ROOT/tools/face-descriptor-check.mjs" "$P" lv2 "$TMP/$stem.lv2.txt" || fails=$((fails + 1))
  done
  [ "$fails" -eq 0 ] || echo "face-descriptor: $fails face(s) differ from their descriptor"
  exit $((fails > 0))
fi

# the arms, on the first described plugin
stem=$(echo $described | cut -d' ' -f1)
P=$ROOT/plugins/$stem
URI=$(uri_of "$P/$stem.descriptor.json")
red() { # red <name> <output file>: the check failed, with the code
  if grep -q 'contract.descriptor.face-differs' "$2"; then echo "SABOTAGE $1: red ($(grep -m1 FAIL "$2" | cut -c1-120))"; else echo "FAIL face-descriptor self-test: $1 stays green"; fails=$((fails + 1)); fi
}
"$SCAN" clap "$P/build/$stem.clap" >"$TMP/clap.txt" || exit 1
node -e '
const fs = require("fs"), d = JSON.parse(fs.readFileSync(process.argv[1], "utf8"));
const c = d.controls.find((x) => x.kind === "travel");
c.travel.max += 1;
fs.writeFileSync(process.argv[2], JSON.stringify(d));
' "$P/$stem.descriptor.json" "$TMP/moved.json"
node "$ROOT/tools/face-descriptor-check.mjs" "$P" clap "$TMP/clap.txt" "$TMP/moved.json" >"$TMP/arm1.txt" 2>&1 && : 
red "a range changed in the authored descriptor (clap)" "$TMP/arm1.txt"
cp -r "$P/build/$stem.lv2" "$TMP/bundle.lv2"
sed -i '0,/lv2:maximum \([0-9.e+-]*\) ;/s//lv2:maximum 12345 ;/' "$TMP/bundle.lv2/$stem.ttl"
"$SCAN" lv2 "$TMP/bundle.lv2" "$URI" >"$TMP/lv2.txt" || exit 1
node "$ROOT/tools/face-descriptor-check.mjs" "$P" lv2 "$TMP/lv2.txt" >"$TMP/arm2.txt" 2>&1 && :
red "a range changed in the built .ttl (lv2)" "$TMP/arm2.txt"
[ "$fails" -eq 0 ] && echo "face-descriptor self-test: every sabotage red"
exit $((fails > 0))
