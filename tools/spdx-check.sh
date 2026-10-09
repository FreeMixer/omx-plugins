#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#
# Every tracked source file names its licence in an SPDX line within its first five lines.
# Files that cannot carry a comment (the licence text, the changelogs the packaging is generated
# from or into, Debian's control files, JSON, images) are
# listed here and nowhere else.
set -euo pipefail
cd "$(dirname "$0")/.."
exempt='^(LICENSE|debian/(changelog|control|copyright|source/format|[^/]+\.(docs|install))|.*\.json|.*\.png)$'
# Listed first, so a git that cannot read the tree (a checkout another user owns) fails here
# instead of handing the loop an empty list that passes.
files=$(git ls-files)
[ -n "$files" ] || { echo "spdx-check: git lists no file" >&2; exit 1; }
missing=0
while IFS= read -r f; do
  [[ "$f" =~ $exempt ]] && continue
  if ! head -5 "$f" | grep -q 'SPDX-License-Identifier: GPL-3.0-or-later'; then
    echo "spdx-check: $f carries no SPDX line" >&2
    missing=1
  fi
done <<<"$files"
[[ $missing == 0 ]] && echo "spdx-check: every source file carries its SPDX line"
exit "$missing"
