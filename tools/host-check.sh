#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#
# host-check.sh <x.clap> <bundle.lv2> <lv2-uri> <clap-id> — the built faces load in real hosts' scanners:
# omx-clap-scan (omx-clap-host's scanner) for the CLAP file, lv2ls/lv2info (lilv) for the LV2 bundle.
# A scanner that is not installed is SKIPped, named; one that is installed must pass.
set -u
clap=$1 bundle=$2 uri=$3 clap_id=$4
fail=0
have() { command -v "$1" >/dev/null 2>&1; }

if have omx-clap-scan; then
  out=$(omx-clap-scan --json "$clap" 2>&1)
  if [ $? -eq 0 ] && printf '%s' "$out" | grep -q "\"id\":\"$clap_id\"" ; then
    echo "PASS host-check omx-clap-scan loads $(basename "$clap")"
  else
    echo "FAIL host-check omx-clap-scan $(basename "$clap"): $out"; fail=1
  fi
else
  echo "SKIP host-check omx-clap-scan: not installed"
fi

if have lv2ls && have lv2info; then
  lv2dir=$(mktemp -d) && cp -r "$bundle" "$lv2dir/"
  if LV2_PATH=$lv2dir lv2ls | grep -qx "$uri"; then echo "PASS host-check lv2ls lists $uri"; else echo "FAIL host-check lv2ls does not list $uri"; fail=1; fi
  if LV2_PATH=$lv2dir lv2info "$uri" >"$lv2dir/info.txt" 2>&1 && grep -q 'Binary:' "$lv2dir/info.txt"; then
    echo "PASS host-check lv2info reads $uri"
  else
    echo "FAIL host-check lv2info $uri:"; cat "$lv2dir/info.txt"; fail=1
  fi
  rm -rf "$lv2dir"
else
  echo "SKIP host-check lv2ls/lv2info: not installed"
fi
exit $fail
