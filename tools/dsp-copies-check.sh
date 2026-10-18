#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#
# tools/dsp-copies-check.sh — no source of this repository carries a copy of a function omx-dsp
# exports: every DSP routine is called from libomxdsp, never written again. The scan is omx-dsp's own
# omx-dsp-copy-check (omx-dsp-devel / libomxdsp-dev ship it with the exports file it reads), run with
# --enforce and no allowlist, so a copy, in a hand-written file or in a generated face, fails.
#
#   tools/dsp-copies-check.sh              the scan over this tree
#   tools/dsp-copies-check.sh --self-test  the scan over a scratch copy of the tree: the clean copy
#                                          passes; the copy with the level meter's peak-and-dB written
#                                          into a face again refuses
# Exit: 0 pass, 1 a copy was found, 2 the check could not run.
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
command -v omx-dsp-copy-check >/dev/null || { echo "dsp-copies-check: omx-dsp-copy-check not found (install omx-dsp-devel or libomxdsp-dev)" >&2; exit 2; }

if [ "${1:-}" = --self-test ]; then
  scratch="$(mktemp -d)"; trap 'rm -rf "$scratch"' EXIT
  (cd "$HERE" && git ls-files -z | xargs -0 cp --parents -t "$scratch")
  git -C "$scratch" init -q && git -C "$scratch" add -A
  omx-dsp-copy-check --enforce --root "$scratch" >/dev/null || { echo "dsp-copies-check self-test: the clean copy of the tree is red" >&2; exit 1; }
  cat >> "$scratch/plugins/omx-comp/omx_comp_clap.c" <<'C'

static double g_level_db(const float *x, uint32_t n, double min) {
  float peak = 0.0f;
  for (uint32_t i = 0; i < n; ++i) {
    const float a = fabsf(x[i]);
    if (a > peak) peak = a;
  }
  const double db = peak > 0.0f ? 20.0 * log10((double)peak) : min;
  return db > min ? db : min;
}
C
  git -C "$scratch" add -A
  if omx-dsp-copy-check --enforce --root "$scratch" >/dev/null; then
    echo "dsp-copies-check self-test: a planted copy of omx_meter_level_db passed" >&2; exit 1
  fi
  echo "dsp-copies-check self-test: the clean tree passes and a planted copy is red"
  exit 0
fi

omx-dsp-copy-check --enforce --root "$HERE"
