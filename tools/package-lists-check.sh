#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#
# tools/package-lists-check.sh — every plugin the build makes is installed, and each installed plugin is
# listed by the one package it belongs to, in packaging/omx-plugins.spec and in debian/*.install:
#   a shipped plugin's .clap in /usr/lib/clap        -> omx-plugins-clap
#   a shipped plugin's .lv2 bundle in <libdir>/lv2   -> omx-plugins-lv2
#   a test plugin's .clap in the test directory      -> omx-plugins-test (outside every CLAP search path)
# and nothing the packages list is missing from the install. A plugin built but packaged nowhere is the
# failure this exists for (omx-plugins#203: the test instruments were built and shipped by no package).
#
#   tools/package-lists-check.sh               `make all`, then `make install` and `make install-test` into two
#                                              scratch stages (the RPM's and the Debian layout), then the check
#   tools/package-lists-check.sh --self-test   the check over hand-made trees: a built plugin not installed, an
#                                              installed plugin missing from the spec, a spec line with nothing
#                                              installed, a plugin in the wrong package and a Debian .install
#                                              that misses one each refuse; a right tree passes
# Exit: 0 pass, 1 refused, 2 the check could not run.
set -euo pipefail
HERE=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
TESTDIR=/usr/libexec/omx-plugins/test

# plugins <stage>: one line per installed plugin, "<path> <want package>", path from the stage's root
plugins() {
  local stage=$1 f
  while IFS= read -r f; do
    f=${f#"$stage"}
    case $f in
      "$TESTDIR"/*.clap) echo "$f omx-plugins-test" ;;
      /usr/lib/clap/*.clap) echo "$f omx-plugins-clap" ;;
      */lv2/*.lv2) echo "$f omx-plugins-lv2" ;;
      *) echo "$f none" ;;
    esac
  done < <(find "$stage" \( -name '*.clap' -type f \) -o \( -name '*.lv2' -type d -prune \) | sort)
}

# rpm_lists <spec>: "<path> <package>" for every plugin path the %files sections list, macros expanded as on
# Fedora (%{_libdir} /usr/lib64, %{_libexecdir} /usr/libexec)
rpm_lists() {
  awk -v name=omx-plugins '
    /^%files/ { sec = ($2 == "" ? name : name "-" $2); next }
    /^%(package|description|prep|build|install|check|changelog)/ { sec = ""; next }
    sec != "" && $0 !~ /^#/ && $0 !~ /^%(dir|license|doc)/ {
      p = $0; gsub(/%\{_libdir\}/, "/usr/lib64", p); gsub(/%\{_libexecdir\}/, "/usr/libexec", p); sub(/\/$/, "", p)
      if (p ~ /\.(clap|lv2)$/) print p, sec
    }' "$1"
}

# deb_package_of <debian dir> <path>: the package whose .install glob matches path, or nothing
deb_package_of() {
  local dir=$1 path=${2#/} inst glob
  for inst in "$dir"/*.install; do
    while IFS= read -r glob; do
      [ -n "$glob" ] || continue
      glob=${glob%% *}
      # shellcheck disable=SC2053 # the .install line is a glob, matched as one
      if [[ $path == $glob ]] || [[ $path == $glob/* ]]; then basename "$inst" .install; return 0; fi
    done < "$inst"
  done
  return 0
}

# check <rpm stage> <deb stage> <spec> <debian dir> <built names>: one line per refusal, the count is the status
check() {
  local rstage=$1 dstage=$2 spec=$3 debdir=$4 built=$5 bad=0 n path want got listed staged
  staged=$(plugins "$rstage")
  listed=$(rpm_lists "$spec")
  for n in $built; do
    if ! grep -q "/$n " <<<"$staged"; then echo "FAIL package-lists: $n is built and not installed"; bad=$((bad + 1)); fi
  done
  while read -r path want; do
    [ -n "$path" ] || continue
    if [ "$want" = none ]; then echo "FAIL package-lists: $path is installed where no package's plugins go"; bad=$((bad + 1)); continue; fi
    got=$(awk -v p="$path" '$1 == p { print $2 }' <<<"$listed")
    if [ -z "$got" ]; then echo "FAIL package-lists: $path is installed and listed by no %files section"; bad=$((bad + 1))
    elif [ "$got" != "$want" ]; then echo "FAIL package-lists: $path is listed in $got, it belongs in $want"; bad=$((bad + 1)); fi
  done <<<"$staged"
  while read -r path got; do
    [ -n "$path" ] || continue
    if ! grep -q "^$path " <<<"$staged"; then echo "FAIL package-lists: the spec lists $path ($got) and nothing installs it"; bad=$((bad + 1)); fi
  done <<<"$listed"
  while read -r path want; do
    [ -n "$path" ] || continue
    got=$(deb_package_of "$debdir" "$path")
    if [ -z "$got" ]; then echo "FAIL package-lists: $path is in no debian/*.install"; bad=$((bad + 1))
    elif [ "$got" != "$want" ]; then echo "FAIL package-lists: $path goes to $got in Debian, it belongs in $want"; bad=$((bad + 1)); fi
  done < <(plugins "$dstage")
  return "$bad"
}

# built_names: the .clap and the .lv2 bundle each plugin folder's build made under its own name (a test's
# scratch builds in build/ carry other names and are not plugins)
built_names() {
  local m dir stem
  for m in "$HERE"/plugins/*/Makefile; do
    dir=$(dirname "$m"); stem=$(basename "$dir")
    [ -f "$dir/build/$stem.clap" ] && echo "$stem.clap"
    [ -d "$dir/build/$stem.lv2" ] && echo "$stem.lv2"
  done
  return 0
}

if [ "${1:-}" = --self-test ]; then
  scratch=$(mktemp -d "${TMPDIR:-/tmp}/package-lists.XXXXXX")
  trap 'rm -rf "$scratch"' EXIT
  fail=0
  tree() { # a right tree: one shipped plugin in both formats and one test plugin, staged and listed
    rm -rf "$scratch/r" "$scratch/d" "$scratch/deb" && mkdir -p "$scratch/r/usr/lib/clap" "$scratch/r/usr/lib64/lv2/omx-a.lv2" \
      "$scratch/r$TESTDIR" "$scratch/d/usr/lib/clap" "$scratch/d/usr/lib/lv2/omx-a.lv2" "$scratch/d$TESTDIR" "$scratch/deb"
    : > "$scratch/r/usr/lib/clap/omx-a.clap"; : > "$scratch/r$TESTDIR/omx-test-t.clap"
    : > "$scratch/d/usr/lib/clap/omx-a.clap"; : > "$scratch/d$TESTDIR/omx-test-t.clap"
    printf '%s\n' '%files' '%license LICENSE' '%files clap' '%dir /usr/lib/clap' '/usr/lib/clap/omx-a.clap' \
      '%files lv2' '%{_libdir}/lv2/omx-a.lv2/' '%files test' '%{_libexecdir}/omx-plugins/test/omx-test-t.clap' \
      '%changelog' '- /usr/lib/clap/omx-old.clap is not a file line' > "$scratch/spec"
    echo 'usr/lib/clap/*.clap' > "$scratch/deb/omx-plugins-clap.install"
    echo 'usr/lib/lv2/*.lv2' > "$scratch/deb/omx-plugins-lv2.install"
    echo 'usr/libexec/omx-plugins/test/*.clap' > "$scratch/deb/omx-plugins-test.install"
    built="omx-a.clap omx-a.lv2 omx-test-t.clap"
  }
  arm() { # <name> <want 0|1>
    local rc=0 out
    out=$(check "$scratch/r" "$scratch/d" "$scratch/spec" "$scratch/deb" "$built") || rc=$?
    if { [ "$2" = 0 ] && [ "$rc" -ne 0 ]; } || { [ "$2" = 1 ] && [ "$rc" -eq 0 ]; }; then
      echo "FAIL package-lists self-test: arm '$1' exit $rc, wanted $([ "$2" = 0 ] && echo pass || echo a refusal)"
      [ -z "$out" ] || echo "$out"
      fail=1
    else
      echo "ok   package-lists self-test: $1 ($([ "$rc" -eq 0 ] && echo passes || echo "refused: ${out%%$'\n'*}"))"
    fi
  }
  tree; arm "a right tree" 0
  tree; built="$built omx-b.clap"; arm "a built plugin not installed" 1
  tree; rm "$scratch/r$TESTDIR/omx-test-t.clap" "$scratch/d$TESTDIR/omx-test-t.clap"; built="omx-a.clap omx-a.lv2"; arm "a test plugin installed by no target, listed by the spec" 1
  tree; grep -v 'omx-test-t' "$scratch/spec" > "$scratch/s2" && mv "$scratch/s2" "$scratch/spec"; arm "an installed test plugin missing from the spec" 1
  tree; sed -i 's|^%{_libexecdir}/omx-plugins/test/omx-test-t.clap|/usr/lib/clap/omx-test-t.clap|' "$scratch/spec"; arm "a spec line with nothing installed" 1
  tree; sed -i '/^%files test/d' "$scratch/spec"; arm "a test plugin listed in the lv2 package" 1
  tree; rm "$scratch/deb/omx-plugins-test.install"; arm "a Debian .install that misses the test plugins" 1
  tree; : > "$scratch/r/usr/lib/clap/omx-test-t.clap"; arm "a test plugin in the CLAP path" 1
  exit "$fail"
fi

command -v make >/dev/null || { echo "package-lists: no make"; exit 2; }
scratch=$(mktemp -d "${TMPDIR:-/tmp}/package-lists.XXXXXX")
trap 'rm -rf "$scratch"' EXIT
make -s -C "$HERE" all >/dev/null || { echo "package-lists: make all failed"; exit 2; }
make -s -C "$HERE" install DESTDIR="$scratch/r" PREFIX=/usr LIBDIR=/usr/lib64 CLAPDIR=/usr/lib/clap >/dev/null || { echo "package-lists: make install failed"; exit 2; }
make -s -C "$HERE" install-test DESTDIR="$scratch/r" PREFIX=/usr TESTCLAPDIR="$TESTDIR" >/dev/null || { echo "package-lists: make install-test failed"; exit 2; }
make -s -C "$HERE" install DESTDIR="$scratch/d" PREFIX=/usr LIBDIR=/usr/lib CLAPDIR=/usr/lib/clap >/dev/null || { echo "package-lists: make install (Debian layout) failed"; exit 2; }
make -s -C "$HERE" install-test DESTDIR="$scratch/d" PREFIX=/usr TESTCLAPDIR="$TESTDIR" >/dev/null || { echo "package-lists: make install-test (Debian layout) failed"; exit 2; }
built=$(built_names)
[ -n "$built" ] || { echo "package-lists: the build made no plugin"; exit 2; }
rc=0
check "$scratch/r" "$scratch/d" "$HERE/packaging/omx-plugins.spec" "$HERE/debian" "$built" || rc=$?
if [ "$rc" -ne 0 ]; then exit 1; fi
echo "package-lists: $(wc -w <<<"$built") built plugins, each installed and listed by its package (RPM and Debian)"
