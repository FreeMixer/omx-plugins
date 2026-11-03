#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#
# tools/lv2bench-ran.sh URI < lv2bench-output — exit 0 when lv2bench ran the plugin URI, 1 when its output does not
# say so. lv2bench prints one line per plugin it ran, and the line has two shapes: lilv up to Debian trixie's
# prints "<seconds> <uri>", lilv 0.26 (Fedora 44) a tab-separated table under a "Block Frames Rate ... Plugin"
# header whose rows end "<total seconds><TAB><uri>". Both end in the URI, preceded by a number, so that is what a
# line must show: the whole last field is the URI (urn:openmixer:eq is not urn:openmixer:eq8) and the field before
# it a number (the header row and a plugin's name in a message do not count).
set -euo pipefail
[ $# -eq 1 ] || { echo "usage: $0 URI < lv2bench-output" >&2; exit 2; }
awk -v u="$1" '$NF == u && NF >= 2 && $(NF - 1) ~ /^[0-9][0-9.]*([eE][-+]?[0-9]+)?$/ { found = 1 } END { exit !found }'
