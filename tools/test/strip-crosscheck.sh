#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#
# strip-crosscheck.sh [<ref>] — omx-strip generated as a chain (spec 2026-10-09-plugin-from-contract
# §15.2) against the hand-ported omx-strip it replaced, bit for bit at the old plugin's settings, at
# every rate the contract declares (tools/test/clap-crosscheck.c, tools/test/strip-0.2.0.map).
#
# The old plugin is built from <ref>, by default the last commit that holds its hand-written
# omx_strip.h; the new one from this tree. Exit 0 when the two are the same plugin at every setting the
# old one had, 1 on any difference, 2 when either cannot be built.
set -eu
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
REF=${1:-$(git -C "$ROOT" rev-list -n1 HEAD -- plugins/omx-strip/omx_strip.h)^}
W=$(mktemp -d)
trap 'rm -rf "$W"' EXIT INT TERM
git -C "$ROOT" archive "$REF" include plugins/omx-strip | tar -x -C "$W"
[ -f "$W/plugins/omx-strip/omx_strip.h" ] || { echo "strip-crosscheck: $REF holds no hand-written omx_strip.h" >&2; exit 2; }
make -s -C "$W/plugins/omx-strip" build/omx-strip.clap >/dev/null || exit 2
make -s -C "$ROOT/plugins/omx-strip" build/omx-strip.clap >/dev/null || exit 2
PKG_CONFIG=${PKG_CONFIG:-pkg-config}
${CC:-cc} -O2 -std=gnu11 -Wall -Wextra -Werror $($PKG_CONFIG --cflags clap omx-contract) -o "$W/clap-crosscheck" \
  "$ROOT/tools/test/clap-crosscheck.c" -ldl -lm || exit 2
echo "strip-crosscheck: the hand-ported omx-strip at $(git -C "$ROOT" rev-parse --short "$REF") against this tree's"
"$W/clap-crosscheck" "$W/plugins/omx-strip/build/omx-strip.clap" "$ROOT/plugins/omx-strip/build/omx-strip.clap" \
  "$ROOT/tools/test/strip-0.2.0.map"
