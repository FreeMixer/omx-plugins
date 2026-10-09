#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#
# tools/modgui-test.sh plugins/<stem> — the plugin's MOD GUI carries its declaration, tested on
# VALUES:
#   (a) `node tools/modgui-gen.mjs --check` is clean;
#   (b) every declared parameter is drawn in the template with its symbol, min, max and default;
#   (c) sord_validate passes the bundle against the LV2 specs and tools/modgui-terms.ttl (skipped
#       when sord_validate is not installed);
#   (d) a perturbed declaration (a parameter's default moved) makes `--check` fail.
# One `PASS <what>` / `FAIL <what>` line per check; exits nonzero on any FAIL.
set -u
[ $# -eq 1 ] || { echo "usage: $0 plugins/<stem>" >&2; exit 2; }
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/.." && pwd)
PDIR=$(cd "$1" && pwd) || exit 2
STEM=$(basename "$PDIR")
DECL="$PDIR/$STEM.decl.json"
BUNDLE="$PDIR/generated/$STEM.lv2"
TEMPLATE="$BUNDLE/modgui/icon-$STEM.html"
LV2DIR=${OMX_LV2_SPEC_DIR:-/usr/lib64/lv2}
fails=0
pass() { echo "PASS $1"; }
fail() { echo "FAIL $1"; fails=$((fails + 1)); }

TMP=$(mktemp -d) || exit 2
trap 'rm -rf "$TMP"' EXIT INT TERM

# (a) the committed files are the generation, byte for byte.
if node "$ROOT/tools/modgui-gen.mjs" --check "$PDIR" >"$TMP/check.out" 2>&1; then
  pass "modgui-gen --check $STEM"
else
  fail "modgui-gen --check $STEM: $(head -1 "$TMP/check.out")"
fi

# (b) every declared parameter, with its numbers, in the template. The declared rows come from the
# declaration itself (JSON, read by node); the drawn rows are grepped from the template as text.
# The declared rows are the RESOLVED declaration's (a parameter by reference reads its travel from
# omx-contract), so they come from gen.mjs's loadDecl, the one reader every generator uses.
node --input-type=module -e '
const { loadDecl } = await import(process.argv[1]);
const d = loadDecl(process.argv[2]);
for (const p of d.params) console.log(`data-symbol="${p.symbol}" data-min="${p.min}" data-max="${p.max}" data-default="${p.def}"`);
' "$ROOT/tools/gen.mjs" "$PDIR" >"$TMP/declared" || fail "read declaration $DECL"
n=0
while IFS= read -r row; do
  n=$((n + 1))
  sym=$(printf '%s\n' "$row" | sed 's/^data-symbol="\([^"]*\)".*/\1/')
  if [ -f "$TEMPLATE" ] && grep -qF "$row" "$TEMPLATE" && grep -qF "mod-port-symbol=\"$sym\"" "$TEMPLATE"; then
    pass "template draws $sym with its declared travel and default"
  else
    fail "template draws $sym with its declared travel and default ($row)"
  fi
done <"$TMP/declared"
drawn=$(grep -c 'data-symbol="' "$TEMPLATE" 2>/dev/null); drawn=${drawn:-0}
# Positive control: the scan read the template it claims to, and it draws nothing undeclared.
if [ "$n" -gt 0 ] && [ "$drawn" -eq "$n" ]; then
  pass "template draws exactly the $n declared parameters"
else
  fail "template draws exactly the declared parameters (declared $n, drawn $drawn)"
fi

# (b2) every parameter with labelled values is a selector: MOD's custom-select on its port, with one
# enumeration option per value, each at the port value the TTL's scale point carries.
node --input-type=module -e '
const { loadDecl, choicesOf } = await import(process.argv[1]);
const d = loadDecl(process.argv[2]);
for (const p of d.params) { const c = choicesOf(p); if (c) console.log(`${p.symbol} ${c.map((x) => x.value).join(" ")}`); }
' "$ROOT/tools/gen.mjs" "$PDIR" >"$TMP/choices" || fail "read the choices of $DECL"
while read -r sym values; do
  [ -n "$sym" ] || continue
  block=$(awk -v s="mod-port-symbol=\"$sym\" mod-widget=\"custom-select\"" 'index($0, s) { on = 1 } on { print } on && /<\/div>$/ && !/enumeration-option|input-control-value/ { n++ } on && n == 2 { exit }' "$TEMPLATE" 2>/dev/null)
  missing=""
  for v in $values; do printf '%s\n' "$block" | grep -qF "mod-role=\"enumeration-option\" mod-port-value=\"$v\"" || missing="$missing $v"; done
  if [ -n "$block" ] && [ -z "$missing" ]; then
    pass "template draws $sym as a selector with every value"
  else
    if [ -n "$block" ]; then why="missing value(s):$missing"; else why="no custom-select"; fi
    fail "template draws $sym as a selector with every value ($why)"
  fi
done <"$TMP/choices"

# (c) sord_validate over the LV2 spec bundles and schemas
# and the closed modgui term list, over the whole generated bundle (manifest, plugin TTL, modgui).
if command -v sord_validate >/dev/null 2>&1; then
  set --
  for s in core port-props units urid atom worker ui options midi patch time log state buf-size \
           parameters presets resize-port port-groups morph dynmanifest instance-access \
           data-access event uri-map schemas; do
    for f in "$LV2DIR/$s.lv2"/*.ttl; do [ -f "$f" ] && set -- "$@" "$f"; done
  done
  if [ $# -le 20 ]; then
    fail "sord_validate: LV2 spec bundles not found under $LV2DIR"
  else
    out=$(sord_validate "$BUNDLE"/*.ttl "$@" "$HERE/modgui-terms.ttl" 2>&1)
    summary=$(printf '%s\n' "$out" | tail -1)
    case "$summary" in
      "Found 0 errors among "*) pass "sord_validate $STEM.lv2/*.ttl against the LV2 specs and modgui terms ($summary)" ;;
      *) printf '%s\n' "$out" | head -20 >&2; fail "sord_validate $STEM.lv2/*.ttl ($summary)" ;;
    esac
    # Red on a property outside the closed modgui vocabulary.
    mkdir -p "$TMP/sord" && cp -R "$BUNDLE" "$TMP/sord/"
    awk '{ print } /^        modgui:brand / { print "        modgui:invented \"x\" ;" }' "$BUNDLE/modgui.ttl" >"$TMP/sord/$STEM.lv2/modgui.ttl"
    out=$(sord_validate "$TMP/sord/$STEM.lv2/modgui.ttl" "$@" "$HERE/modgui-terms.ttl" 2>&1)
    case "$out" in
      *modgui#invented*) pass "sord_validate refuses a modgui property outside modgui-terms.ttl" ;;
      *) fail "sord_validate refuses a modgui property outside modgui-terms.ttl" ;;
    esac
  fi
else
  echo "SKIP sord_validate (not installed)"
fi

# (d) perturbation: the same plugin dir (same basename: loadDecl requires the stem) with the first
# parameter's default moved WHERE IT IS DECLARED (the declaration, or omx-contract's data when the
# parameter is by reference); its stale generated copy must be refused.
mkdir -p "$TMP/p" "$TMP/c" && cp -R "$PDIR" "$TMP/p/$STEM"
# a variant's declaration is its base's (tools/variants.mjs): the base folder goes beside it
if [ ! -f "$DECL" ]; then
  BASE=$(node --input-type=module -e 'const { baseOf } = await import(process.argv[1]); const b = baseOf(process.argv[2]); if (b) console.log(b.file);' "$ROOT/tools/variants.mjs" "$PDIR")
  [ -n "$BASE" ] && cp -R "$(dirname "$BASE")" "$TMP/p/"
fi
if env=$(node "$ROOT/tools/perturb.mjs" "$TMP/p/$STEM" "$TMP/c"); then
  if env $env node "$ROOT/tools/modgui-gen.mjs" --check "$TMP/p/$STEM" >"$TMP/perturb.out" 2>&1; then
    fail "a moved default makes modgui-gen --check fail"
  elif grep -q "icon-$STEM.html" "$TMP/perturb.out"; then
    pass "a moved default makes modgui-gen --check fail"
  else
    fail "a moved default makes modgui-gen --check fail (not on the template: $(head -1 "$TMP/perturb.out"))"
  fi
else
  fail "perturb the first parameter's default where it is declared"
fi

[ "$fails" -eq 0 ] || { echo "modgui-test: $fails check(s) failed" >&2; exit 1; }
