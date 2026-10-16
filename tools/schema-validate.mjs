// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * schema-validate.mjs — a JSON Schema validator for the subset schema/plugin.decl.schema.json uses.
 * A module of its own, importing nothing, so every tool can validate a declaration without loading
 * the completeness checker (tools/plugin-recipe.mjs re-exports it).
 */

/** Errors of `value` against `schema` (draft 2020-12: $ref into $defs, type, const, enum, pattern,
 * minLength, required, properties, additionalProperties, items, minItems, uniqueItems, anyOf). */
export function validate(root, schema, value, path = '') {
  const errs = [];
  const err = (msg) => errs.push(`${path || '/'}: ${msg}`);
  if (schema.$ref) {
    const target = schema.$ref.replace(/^#\//, '').split('/').reduce((n, k) => n?.[k], root);
    if (!target) return [`${path || '/'}: unresolved ${schema.$ref}`];
    errs.push(...validate(root, target, value, path));
  }
  const type = Array.isArray(value) ? 'array' : value === null ? 'null' : typeof value;
  if (schema.type) {
    const ok = schema.type === 'integer' ? Number.isInteger(value) : schema.type === type;
    if (!ok) return [...errs, `${path || '/'}: is ${type}, not ${schema.type}`];
  }
  if ('const' in schema && JSON.stringify(value) !== JSON.stringify(schema.const)) err(`must be ${JSON.stringify(schema.const)}`);
  if (schema.enum && !schema.enum.includes(value)) err(`must be one of ${schema.enum.join(', ')}`);
  if (typeof value === 'string') {
    if (schema.pattern && !new RegExp(schema.pattern).test(value)) err(`'${value}' does not match ${schema.pattern}`);
    if (schema.minLength && value.length < schema.minLength) err(`shorter than ${schema.minLength}`);
  }
  if (type === 'object') {
    for (const k of schema.required ?? []) if (!(k in value)) err(`'${k}' is required`);
    for (const [k, v] of Object.entries(value)) {
      if (schema.properties?.[k]) errs.push(...validate(root, schema.properties[k], v, `${path}/${k}`));
      else if (schema.additionalProperties === false) err(`'${k}' is not a property`);
      else if (typeof schema.additionalProperties === 'object') errs.push(...validate(root, schema.additionalProperties, v, `${path}/${k}`));
    }
  }
  if (type === 'array') {
    if (schema.minItems && value.length < schema.minItems) err(`fewer than ${schema.minItems} items`);
    if (schema.uniqueItems && new Set(value.map((v) => JSON.stringify(v))).size !== value.length) err('items repeat');
    if (schema.items) value.forEach((v, i) => errs.push(...validate(root, schema.items, v, `${path}/${i}`)));
  }
  if (schema.anyOf && !schema.anyOf.some((s) => validate(root, s, value, path).length === 0)) {
    err(`matches none of: ${schema.anyOf.map((s) => (s.required ? `{${s.required.join(', ')}}` : '…')).join(' or ')}`);
  }
  return errs;
}
