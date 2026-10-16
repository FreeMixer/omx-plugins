#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#
# sabotage.sh <scratch-dir> <oracle> — the note echo's oracle, proven able to see each defect it
# guards. Each sabotage is a COPY of omx_test_note_echo_clap.c with one line broken, built into a
# .clap the oracle must refuse on the named arm. The tree is never edited; a sed that matches nothing
# is a failure, not a skip, and a red with no FAIL line of that arm (a crash) is not a red arm.
# CC, CFLAGS and PLUGIN come from the Makefile.
set -eu
HERE=$(cd "$(dirname "$0")/.." && pwd)
OUT=${1:?scratch dir}
ORACLE=${2:?oracle}
SRC="$HERE/omx_test_note_echo_clap.c"
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

sabotage frame echo 's|    if (g_is_note(h)) proc->out_events->try_push(proc->out_events, h);|    if (g_is_note(h)) { union { clap_event_note_t n; clap_event_midi_t m; } u; memcpy(\&u, h, h->size); u.n.header.time = 0; proc->out_events->try_push(proc->out_events, \&u.n.header); }|'
sabotage midi echo 's|  return kind == 0x80 \|\| kind == 0x90;|  return false \&\& (kind == 0x80 \|\| kind == 0x90);|'
sabotage off echo 's|h->type == CLAP_EVENT_NOTE_OFF \|\| ||'
sabotage twice echo 's|    if (g_is_note(h)) proc->out_events->try_push(proc->out_events, h);|    if (g_is_note(h)) proc->out_events->try_push(proc->out_events, h), proc->out_events->try_push(proc->out_events, h);|'
sabotage other other 's|  if (h->type != CLAP_EVENT_MIDI) return false;|  if (h->type != CLAP_EVENT_MIDI) return true;|'
sabotage audio audio 's|memset(proc->audio_outputs\[o\].data32\[c\], 0, proc->frames_count \* sizeof(float));|memset(proc->audio_outputs[o].data32[c], 0, proc->frames_count * sizeof(float) / 2);|'
sabotage dialect ports 's|  info->supported_dialects = CLAP_NOTE_DIALECT_CLAP \| CLAP_NOTE_DIALECT_MIDI;|  info->supported_dialects = CLAP_NOTE_DIALECT_CLAP;|'
[ "$fail" -eq 0 ] && echo "sabotage.sh: the note echo's oracle sees all seven sabotages"
exit "$fail"
