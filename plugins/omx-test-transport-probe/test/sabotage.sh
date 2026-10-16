#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#
# sabotage.sh <scratch-dir> <oracle> — the transport probe's oracle, proven able to see each defect it
# guards. Each sabotage is a COPY of omx_test_transport_probe_clap.c with one line broken, built into a
# .clap the oracle must refuse on the named arm. The tree is never edited; a sed that matches nothing
# is a failure, not a skip, and a red with no FAIL line of that arm (a crash) is not a red arm.
# CC, CFLAGS and PLUGIN come from the Makefile.
set -eu
HERE=$(cd "$(dirname "$0")/.." && pwd)
OUT=${1:?scratch dir}
ORACLE=${2:?oracle}
SRC="$HERE/omx_test_transport_probe_clap.c"
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
  $CC $CFLAGS $PLUGIN -I"$HERE/generated" -I"$HERE" -o "$d/plugin.clap" "$d/face.c" -lm
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

sabotage whole transport 's|if (proc->transport) memcpy(\&r.transport, proc->transport, sizeof r.transport);|if (proc->transport) memcpy(\&r.transport, proc->transport, sizeof r.transport - 8);|'
sabotage stale transport 's|if (proc->transport) memcpy|if (proc->transport \&\& g->blocks == 1) memcpy|'
sabotage count transport 's|r.blocks = ++g->blocks;|r.blocks = g->blocks++;|'
sabotage absent absent 's|r.has_transport = proc->transport != NULL;|r.has_transport = 1;|'
sabotage legs audio 's|const float \*src = c < in->channel_count ? in->data32\[c\] : NULL;|const float *src = c < in->channel_count ? in->data32[in->channel_count - 1 - c] : NULL;|'
sabotage none none 's|return a != 0u;|return true;|'
[ "$fail" -eq 0 ] && echo "sabotage.sh: the transport probe's oracle sees all six sabotages"
exit "$fail"
