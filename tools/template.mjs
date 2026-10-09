// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * template.mjs — what the generators, the wizard and the completeness test share without importing
 * each other: the template renderer and where omx-dsp's headers are.
 */
import { execFileSync } from 'node:child_process';
import { existsSync } from 'node:fs';
import { join } from 'node:path';

/** omx-dsp's include directory (the one holding omxdsp/): OMXDSP_INCLUDE, or pkg-config's includedir. */
export function omxdspInclude() {
  if (process.env.OMXDSP_INCLUDE) return process.env.OMXDSP_INCLUDE;
  let dir;
  try {
    dir = execFileSync('pkg-config', ['--variable=includedir', 'omxdsp'], { encoding: 'utf8', stdio: ['ignore', 'pipe', 'ignore'] }).trim();
  } catch {
    return undefined;
  }
  return dir && existsSync(join(dir, 'omxdsp')) ? dir : undefined;
}

/**
 * A minimal mustache: `{{x}}`, `{{#x}}…{{/x}}` (a list repeats with each item's fields in scope, a
 * truthy value renders once), `{{^x}}…{{/x}}` (renders when x is falsy or empty), `{{! … }}` (dropped
 * with its line). An unknown `{{x}}` is an error, so a template cannot silently print nothing.
 */
export function render(template, view) {
  const t = template.replace(/^\{\{![\s\S]*?\}\}\n?/gm, '');
  const section = /\{\{([#^])(\w+)\}\}([\s\S]*?)\{\{\/\2\}\}/g;
  const expand = (src, scope) =>
    src
      .replace(section, (_, kind, key, body) => {
        const v = scope[key];
        const truthy = Array.isArray(v) ? v.length > 0 : Boolean(v);
        if (kind === '^') return truthy ? '' : expand(body, scope);
        if (!truthy) return '';
        return Array.isArray(v) ? v.map((item) => expand(body, { ...scope, ...item })).join('') : expand(body, scope);
      })
      .replace(/\{\{(\w+)\}\}/g, (_, key) => {
        if (!(key in scope)) throw new Error(`template: no value for {{${key}}}`);
        return String(scope[key]);
      });
  return expand(t, view);
}
