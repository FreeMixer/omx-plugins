#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#
# private-refs-check.sh — this repository is public, so its tracked files state facts and never point
# into a private repository: no private repository name, no private issue or pull request number, no
# private branch name or file path, no name of a private specification file. Say what the value is and
# why instead, and cite only public repositories (omx-dsp, omx-contract, omx-plugins, plugin-qualify).
# Product-name mentions of OpenMixer, and values such as the org.openmixer.* CLAP ids, the
# urn:openmixer:* LV2 URIs and the @openmixer npm scope, are not pointers and are not matched.
#
#   tools/private-refs-check.sh              the check, over every tracked file (generated ones included)
#   tools/private-refs-check.sh --self-test  each sabotage, in every kind of file, must go red
#
# ALLOW lists the tracked files exempt from the check, each with the reason; nothing else is.
set -euo pipefail
HERE="$(cd "$(dirname "$0")/.." && pwd)"

# <file> — <why it may name them>
ALLOW=(
  "tools/private-refs-check.sh"   # it spells the patterns it looks for
)

# Extended regexps, matched case-insensitively. A new kind of pointer is one line here.
PATTERNS=(
  'FreeMixer/openmixer([^A-Za-z0-9_-]|$)'             # the private repository
  'openmixer-ops'                                     # the private operations repository
  'openmixer ?#[0-9]'                                 # one of its issues or pull requests
  '[A-Za-z0-9_.]-dev#[0-9]'                           # an issue of a private *-dev repository
  'integration/[A-Za-z0-9]'                           # a private integration branch
  'packages/[a-z][a-z0-9-]+'                          # a path into the private repository's packages
  '\.vue([^A-Za-z0-9_]|$)'                            # its web UI components
  "openmixer's spec"                                  # its specification files
  '20[0-9]{2}-[0-9]{2}-[0-9]{2}-[a-z][a-z0-9-]+\.md'  # a dated specification file name
  'spec 20[0-9]{2}-[0-9]{2}-[0-9]{2}'                 # a citation of one
)

check() { # <root>
  local root="$1" fail=0 args=() p a hits
  for p in "${PATTERNS[@]}"; do args+=(-e "$p"); done
  local excl=()
  for a in "${ALLOW[@]}"; do excl+=(":(exclude)${a%% *}"); done
  [ -n "$(git -C "$root" ls-files)" ] || { echo "private-refs-check: git lists no file" >&2; return 1; }
  hits=$(git -C "$root" grep -nIiE "${args[@]}" -- . "${excl[@]}" | cut -c1-200 || true)
  [ -z "$hits" ] || { printf '%s\n' "$hits" >&2; fail=1; }
  [ "$fail" = 0 ] || { echo "private-refs-check: the lines above point into private repositories; state the fact instead" >&2; return 1; }
  echo "private-refs-check: no tracked file points into a private repository"
}

if [ "${1:-}" = --self-test ]; then
  scratch="$(mktemp -d)"; trap 'rm -rf "$scratch"' EXIT
  fail=0
  # a clean tree with one file of each kind the repository tracks; every sabotage writes one private pointer into one
  mk() { rm -rf "$scratch/t"; mkdir -p "$scratch/t/plugins/x/generated" "$scratch/t/tools" "$scratch/t/docs"
    git -C "$scratch" init -q t
    printf '{\n  "$comment": "The OpenMixer console draws its x stage."\n}\n' > "$scratch/t/plugins/x/x.decl.json"
    printf 'rdfs:comment "The OpenMixer console x." ;\nlv2:binary <x.so> ; # urn:openmixer:x\n' > "$scratch/t/plugins/x/generated/x.ttl"
    printf '// org.openmixer.x, @openmixer/x\n' > "$scratch/t/tools/gen.mjs"
    printf '# Docs\n\nOpenMixer plugins.\n' > "$scratch/t/docs/a.md"
    printf '/* x */\n' > "$scratch/t/plugins/x/generated/x.h"; }
  sab() { # <name> <file> <line>
    mk; printf '%s\n' "$3" >> "$scratch/t/$2"; git -C "$scratch/t" add -A
    if check "$scratch/t" >/dev/null 2>&1; then echo "SABOTAGE $1: STAYED GREEN"; fail=1; else echo "SABOTAGE $1: red"; fi; }
  mk; git -C "$scratch/t" add -A
  check "$scratch/t" >/dev/null || { echo "self-test: the clean tree is not green" >&2; exit 1; }
  sab repo-in-json   plugins/x/x.decl.json           '{"$comment": "as FreeMixer/openmixer draws it"}'
  sab branch-in-json plugins/x/x.decl.json           '{"$comment": "drawn on integration/lean-engine"}'
  sab issue-in-md    docs/a.md                       'see openmixer#1580'
  sab path-in-md     docs/a.md                       'packages/web-ui/Foo.vue'
  sab ops-in-mjs     tools/gen.mjs                   '// see openmixer-ops'
  sab dev-in-mjs     tools/gen.mjs                   '// omx-dsp-dev#32'
  sab spec-in-mjs    tools/gen.mjs                   "// openmixer's spec 2026-10-09-plugin-from-contract.md"
  sab ttl            plugins/x/generated/x.ttl       'rdfs:comment "see FreeMixer/openmixer#12" ;'
  sab header         plugins/x/generated/x.h         '/* packages/core/src/x.ts */'
  [ "$fail" = 0 ] || exit 1
  check "$HERE" >/dev/null || { echo "self-test: the repository is not green" >&2; exit 1; }
  echo "private-refs-check self-test: every sabotage red"
  exit 0
fi
check "$HERE"
