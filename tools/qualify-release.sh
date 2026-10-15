#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#
# tools/qualify-release.sh <out-dir>
#
# The release gate: every CLAP plugin the tree builds gets plugin-qualify's real-time-safety verdict,
# and the release goes ahead only when each one is `suitable`. It builds the tree, installs it into a
# staging directory, runs plugin-qualify's CLAP prober over every installed `.clap` and checks the
# documents it wrote, one per plugin binary and host profile (`<sha256>.<profile>.rt-safety.json`),
# so the verdicts that gate the release are the ones that ship beside it.
#
#   - a plugin whose document does not say `suitable` refuses the release, named with its verdict;
#   - an installed `.clap` with no document refuses it too (a prober that wrote nothing is not a pass);
#   - plugin-qualify's own exit code (1 some unqualified, 2 not measurable) refuses it.
#
# Needs the channel packages: plugin-qualify (the prober and its interposer), omx-clap-host
# (`omx-clap-scan`) and the build dependencies of packaging/omx-plugins.spec. `make` is skipped when
# QUALIFY_STAGE names a prefix that already holds a build.
# Exit: 0 every plugin suitable, 1 refused, 2 the gate could not run.
set -euo pipefail
HERE=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
OUT=${1:?usage: qualify-release.sh <out-dir>}

for tool in plugin-qualify omx-clap-scan node; do
  command -v "$tool" >/dev/null || { echo "qualify-release: $tool is not installed; install plugin-qualify and omx-clap-host from the channel" >&2; exit 2; }
done

STAGE=${QUALIFY_STAGE:-}
if [ -z "$STAGE" ]; then
  STAGE=$(mktemp -d)
  trap 'rm -rf "$STAGE"' EXIT
  make -C "$HERE" -j"$(nproc)"
  make -C "$HERE" install DESTDIR="$STAGE" LIBDIR=/usr/lib64
fi

CLAPDIR=$(find "$STAGE" -type d -path '*/lib/clap' | head -1)
[ -n "$CLAPDIR" ] || { echo "qualify-release: no lib/clap under $STAGE; nothing was installed" >&2; exit 2; }
mapfile -t CLAPS < <(find "$CLAPDIR" -maxdepth 1 -name '*.clap' | sort)
[ "${#CLAPS[@]}" -gt 0 ] || { echo "qualify-release: $CLAPDIR holds no .clap; the tree built no plugin" >&2; exit 2; }

mkdir -p "$OUT"
rc=0
plugin-qualify --out "$OUT" "${CLAPS[@]}" || rc=$?

# The documents are the evidence: one per binary, each read here rather than trusted from the exit code.
node - "$OUT" "${CLAPS[@]}" <<'JS' || exit_js=$?
const { readdirSync, readFileSync } = require('node:fs');
const { createHash } = require('node:crypto');
const { join, basename } = require('node:path');
const [out, ...claps] = process.argv.slice(2);
const docs = readdirSync(out).filter((f) => f.endsWith('.rt-safety.json')).map((f) => ({ f, d: JSON.parse(readFileSync(join(out, f), 'utf8')) }));
// plugin-qualify.rt-safety/1: verdict.rating is suitable | unsuitable | unknown, verdict.rtSafe its boolean
const verdictOf = (d) => (d.verdict?.rating === 'suitable' && d.verdict?.rtSafe === true ? 'suitable' : (d.verdict?.rating ?? 'absent'));
let refused = 0;
for (const clap of claps) {
  const sha = createHash('sha256').update(readFileSync(clap)).digest('hex');
  const mine = docs.filter((x) => x.f.startsWith(`${sha}.`));
  if (mine.length === 0) { console.log(`REFUSED ${basename(clap)}: no verdict document for sha256 ${sha}`); refused++; continue; }
  for (const { f, d } of mine) {
    const v = verdictOf(d);
    if (v !== 'suitable') { console.log(`REFUSED ${basename(clap)}: ${f} says ${JSON.stringify(v)}`); refused++; }
    else console.log(`suitable ${basename(clap)}: ${f}`);
  }
}
process.exit(refused === 0 ? 0 : 1);
JS
exit_js=${exit_js:-0}

if [ "$rc" -ne 0 ] || [ "$exit_js" -ne 0 ]; then
  echo "qualify-release: REFUSED (plugin-qualify exit $rc, document check exit $exit_js): no release without a suitable verdict for every plugin" >&2
  exit 1
fi
echo "qualify-release: every plugin is suitable (${#CLAPS[@]} binaries)"
