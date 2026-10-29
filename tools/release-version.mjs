// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
//
// The tree's release version: the Makefile's VERSION, the one line a release bumps. The generated headers, the
// descriptors and the LV2 versions carry it; a version written in a declaration or descriptor is read and ignored.
import { readFileSync } from 'node:fs';
import { join } from 'node:path';

export function releaseVersion(root) {
  const file = join(root, 'Makefile');
  const m = readFileSync(file, 'utf8').match(/^VERSION\s*:?=\s*(\d+\.\d+\.\d+)\s*$/m);
  if (!m) throw new Error(`${file}: no VERSION := x.y.z`);
  return m[1];
}
