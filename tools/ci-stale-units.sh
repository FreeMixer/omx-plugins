#!/bin/sh
# The channel's omx-plugins-contract before 0.17.0 installs contract/param-units.json, the sheet that moved to omx-contract
# 3.6.0 (OMX_PARAM_CONTRACT_UNITS): beside omx-contract 3.6.0 the contract directory then declares that item twice and
# every omx-contract read refuses it (contract.item.duplicate). The build dependencies pull that package in (omx-panel
# requires it), so a CI job drops its stale sheet after installing them. The package this tree builds no longer has the
# file, so once 0.17.0 is in the channel there is nothing to drop and this script and its calls go.
set -eu
f=/usr/share/omx-contract/contract.d/3/omx-plugins/param-units.json
[ -e "$f" ] || { echo "ci-stale-units: no stale sheet"; exit 0; }
echo "ci-stale-units: dropping $f, installed by the channel's omx-plugins-contract before 0.17.0"
rm -f "$f"
