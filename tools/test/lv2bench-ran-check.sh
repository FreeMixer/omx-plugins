#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#
# tools/test/lv2bench-ran-check.sh — tools/lv2bench-ran.sh reads both shapes of lv2bench's output (Debian's
# "<seconds> <uri>" and Fedora lilv 0.26.4's tab table, captured from lv2bench there), finds a plugin that ran, and
# refuses one that did not, a header row, a URI that is only the start of another plugin's, and an empty output.
# `make lv2bench-ran`, part of `make test`.
set -euo pipefail
HERE=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
ran=$HERE/lv2bench-ran.sh
bad=0
expect() { # want-exit uri output label
  local rc=0
  printf '%s' "$3" | "$ran" "$2" || rc=$?
  if [ "$rc" = "$1" ]; then echo "PASS lv2bench-ran: $4"; else echo "FAIL lv2bench-ran: $4 (exit $rc, wanted $1)"; bad=$((bad + 1)); fi
}
debian=$'0.014597676 urn:openmixer:chorus\n0.2 urn:openmixer:eq8\n'
fedora=$'Block\tFrames\tRate\tMin\tMean\tMax\tTotal\tPlugin\n512\t524288\t48000.000000\t1.3991e-05\t1.41876377e-05\t3.5002e-05\t0.014597676\turn:openmixer:chorus\n512\t524288\t48000.000000\t1e-05\t1.2e-05\t2e-05\t0.0121\turn:openmixer:eq8\n'
expect 0 urn:openmixer:chorus "$debian" "Debian shape: a plugin that ran is found"
expect 0 urn:openmixer:chorus "$fedora" "Fedora 0.26.4 shape: a plugin that ran is found"
expect 1 urn:openmixer:delay "$debian" "Debian shape: a plugin that did not run is refused"
expect 1 urn:openmixer:delay "$fedora" "Fedora 0.26.4 shape: a plugin that did not run is refused"
expect 1 urn:openmixer:eq "$debian" "Debian shape: a URI that starts another plugin's is refused"
expect 1 urn:openmixer:eq "$fedora" "Fedora 0.26.4 shape: a URI that starts another plugin's is refused"
expect 1 Plugin "${fedora%%$'\n'*}"$'\n' "the header row alone is refused"
expect 1 urn:openmixer:chorus "" "empty output is refused"
expect 1 urn:openmixer:chorus $'lv2bench: failed to instantiate urn:openmixer:chorus\n' "an error line naming the plugin is refused"
[ "$bad" = 0 ] && echo "PASS lv2bench-ran: both output shapes" || { echo "lv2bench-ran: $bad failure(s)"; exit 1; }
