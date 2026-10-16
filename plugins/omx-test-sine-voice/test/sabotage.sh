#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#
# sabotage.sh <scratch-dir> <oracle> — the sine voice's oracle, proven able to see each defect it
# guards. Each sabotage is a COPY of omx_test_sine_voice_clap.c with one line broken, built into a
# .clap the oracle must refuse on the named arm. The tree is never edited; a sed that matches nothing
# is a failure, not a skip, and a red with no FAIL line of that arm (a crash) is not a red arm.
# CC, CFLAGS and PLUGIN come from the Makefile.
set -eu
HERE=$(cd "$(dirname "$0")/.." && pwd)
OUT=${1:?scratch dir}
ORACLE=${2:?oracle}
SRC="$HERE/omx_test_sine_voice_clap.c"
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

sabotage frequency note 's|OMX_TEST_SINE_VOICE_A4_HZ \* pow|(OMX_TEST_SINE_VOICE_A4_HZ + 1.0) * pow|'
sabotage level note 's|OMX_TEST_SINE_VOICE_GAIN \* (vel > 1.0 ? 1.0 : vel)|OMX_TEST_SINE_VOICE_GAIN|'
sabotage attack note 's|return v->k >= g->attack ? 1.0 : (double)v->k / (double)g->attack;|return 1.0;|'
sabotage frame partials 's|const uint32_t t = h->time < n ? h->time : n;|const uint32_t t = 0;|'
sabotage sum partials 's|l\[f\] += (float)|l[f] = (float)|'
sabotage midi partials 's|if (kind == 0x90 \&\& vel > 0)|if (kind == 0x90 \&\& vel > 127)|'
sabotage release release 's|return j >= g->release ? 0.0 : v->e0 \* (1.0 - (double)j / (double)g->release);|return j >= g->release ? 0.0 : v->e0 * (1.0 - (double)j / (double)(2 * g->release));|'
sabotage end release 's|        g_end(g, v, f, out);|        v->on = out == NULL; /* ends, unreported */|'
sabotage velocity0 velocity0 's|else if (kind == 0x80 \|\| kind == 0x90)|else if (kind == 0x80)|'
sabotage capacity voices 's|info->voice_capacity = VOICES;|info->voice_capacity = VOICES + 1;|'
sabotage steal voices 's|if (g->v\[i\].age < v->age) v = \&g->v\[i\];|if (g->v[i].age > v->age) v = \&g->v[i];|'
[ "$fail" -eq 0 ] && echo "sabotage.sh: the sine voice's oracle sees all eleven sabotages"
exit "$fail"
