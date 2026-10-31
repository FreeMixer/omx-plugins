#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#
# sabotage.sh <scratch-dir> <oracle> — the pool voice's oracle, proven able to see each defect it
# guards. Each sabotage is a COPY of omx_test_pool_voice_clap.c with one line broken, built into a
# .clap the oracle must refuse on the named arm. The tree is never edited; a sed that matches nothing
# is a failure, not a skip, and a red with no FAIL line of that arm (a crash) is not a red arm.
# CC, CFLAGS and PLUGIN come from the Makefile.
set -eu
HERE=$(cd "$(dirname "$0")/.." && pwd)
OUT=${1:?scratch dir}
ORACLE=${2:?oracle}
SRC="$HERE/omx_test_pool_voice_clap.c"
rm -rf "$OUT" && mkdir -p "$OUT"
fail=0

sabotage() { # <name> <arm the oracle must fail> <sed expression>
  d="$OUT/$1"
  mkdir -p "$d"
  sed "$3" "$SRC" > "$d/face.c"
  if cmp -s "$SRC" "$d/face.c"; then
    echo "FAIL sabotage '$1' changed nothing: the line it targets is gone"
    fail=1
    return
  fi
  # shellcheck disable=SC2086 # word lists
  $CC $CFLAGS $PLUGIN -I"$HERE/generated" -o "$d/plugin.clap" "$d/face.c" -lm
  rc=0
  "$ORACLE" "$d/plugin.clap" > "$d/out" 2>&1 || rc=$?
  n=$(grep -c "^FAIL $2 " "$d/out" || true)
  if [ "$rc" -eq 0 ] || [ "$n" -eq 0 ]; then
    echo "FAIL sabotage '$1': the oracle's $2 arm did not go red (exit $rc, $n FAIL lines of it)"
    fail=1
  else
    echo "ok   sabotage '$1': the $2 arm went red on $n checks"
  fi
}

# the task dispatch removed: the voices render serially and the host is never asked
sabotage dispatch dispatch 's|done = g->pool->request_exec(g->host, nact);|done = false;|'
# every task renders the first voice
sabotage task note 's|g_render_voice(g, \&g->v\[g->act\[task\]\]);|g_render_voice(g, \&g->v[g->act[0]]);|'
# a host that refuses is not rendered for
sabotage fallback note 's|if (!done)|if (!done \&\& 0)|'
# the sum taken in a different order from the voice order is not caught by bits alone on a few voices, so the
# sum is broken outright: the second sounding voice is not added
sabotage sum note 's|for (uint32_t a = 0; a < nact; a++) {|for (uint32_t a = 0; a < (nact > 1 ? 1 : nact); a++) {|'
# no load
sabotage load load 's|i < (uint32_t)OMX_TEST_POOL_VOICE_LOAD_ITERS|i < 1u|'
# a voice's release that does not report its end
sabotage end release 's|if (v->end != UINT32_MAX) {|if (v->end == UINT32_MAX - 1) {|'
[ "$fail" -eq 0 ] && echo "sabotage.sh: the pool voice's oracle sees all six sabotages"
exit "$fail"
