/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
 *
 * omx_preset_file.h — what a face reads of a factory preset file (org.openmixer.preset/1,
 * presets/<plugin id>/<preset id>.json, docs/presets.md): the top-level strings `format`, `plugin`
 * and `name`, and each `params` entry's `symbol` and `value`. Everything else of the file is
 * skipped. The CLAP faces' clap.preset-discovery provider and clap.preset-load read files through
 * it; the files are tools/gen-presets.mjs's.
 *
 * A small strict reader for JSON as that tool writes it: no allocation beyond the file's text, no
 * \u escapes kept (a name never needs one), a malformed file read as no preset.
 */
#ifndef OMX_PRESET_FILE_H
#define OMX_PRESET_FILE_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OMX_PRESET_FORMAT "org.openmixer.preset/1"
/* The installed root: presets/<plugin id>/ lies under it. */
#ifndef OMX_PRESET_ROOT
#define OMX_PRESET_ROOT "/usr/share/omx/presets"
#endif
/* The largest file a face reads: a 32-band EQ's preset is under 16 KiB. */
#define OMX_PRESET_MAX_BYTES (256u * 1024u)

/* The file's text, NUL-terminated, malloc'ed; NULL for a missing, empty or oversized file. */
static inline char *omx_preset_slurp(const char *path) {
  FILE *f = path ? fopen(path, "rb") : NULL;
  if (!f) return NULL;
  char *buf = (char *)malloc(OMX_PRESET_MAX_BYTES + 1);
  size_t len = buf ? fread(buf, 1, OMX_PRESET_MAX_BYTES + 1, f) : 0;
  fclose(f);
  if (!buf || len == 0 || len > OMX_PRESET_MAX_BYTES) {
    free(buf);
    return NULL;
  }
  buf[len] = 0;
  return buf;
}

static inline const char *omx_preset_ws(const char *s) {
  while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') ++s;
  return s;
}

/* A string at s (its opening quote): its text into out (cut to n - 1, escapes resolved), the
 * position after its closing quote; NULL if s is no string. */
static inline const char *omx_preset_string(const char *s, char *out, size_t n) {
  if (*s != '"') return NULL;
  size_t k = 0;
  for (++s; *s && *s != '"'; ++s) {
    char c = *s;
    if (c == '\\') {
      c = *++s;
      if (!c) return NULL;
      if (c == 'n') c = '\n'; else if (c == 't') c = '\t'; else if (c == 'u') { c = '?'; for (int i = 0; i < 4 && s[1]; ++i) ++s; }
    }
    if (out && k + 1 < n) out[k++] = c;
  }
  if (out && n) out[k] = 0;
  return *s == '"' ? s + 1 : NULL;
}

/* Past one value at s (string, number, literal, object or array); NULL if malformed. */
static inline const char *omx_preset_skip(const char *s) {
  s = omx_preset_ws(s);
  if (*s == '"') return omx_preset_string(s, NULL, 0);
  if (*s == '{' || *s == '[') {
    int depth = 0;
    while (*s) {
      if (*s == '"') { s = omx_preset_string(s, NULL, 0); if (!s) return NULL; continue; }
      if (*s == '{' || *s == '[') ++depth;
      else if ((*s == '}' || *s == ']') && --depth == 0) return s + 1;
      ++s;
    }
    return NULL;
  }
  const char *b = s;
  while (*s && *s != ',' && *s != '}' && *s != ']' && *s != ' ' && *s != '\n' && *s != '\r' && *s != '\t') ++s;
  return s == b ? NULL : s;
}

/* Each member of the object at s: fn(key, value position, ctx) returns false to stop. False if s is
 * no well-formed object. */
static inline bool omx_preset_members(const char *s, bool (*fn)(const char *key, const char *value, void *ctx), void *ctx) {
  s = omx_preset_ws(s);
  if (*s != '{') return false;
  s = omx_preset_ws(s + 1);
  if (*s == '}') return true;
  for (;;) {
    char key[64];
    s = omx_preset_string(s, key, sizeof key);
    if (!s) return false;
    s = omx_preset_ws(s);
    if (*s != ':') return false;
    const char *v = omx_preset_ws(s + 1);
    if (!fn(key, v, ctx)) return true;
    s = omx_preset_skip(v);
    if (!s) return false;
    s = omx_preset_ws(s);
    if (*s == '}') return true;
    if (*s != ',') return false;
    s = omx_preset_ws(s + 1);
  }
}

/* What a face needs of the file's head. */
struct omx_preset_head {
  char format[64];
  char plugin[128];
  char name[128];
  const char *params; /* the `params` array, NULL if none */
};

static inline bool omx_preset_head_member(const char *key, const char *v, void *ctx) {
  struct omx_preset_head *h = (struct omx_preset_head *)ctx;
  if (strcmp(key, "format") == 0) omx_preset_string(v, h->format, sizeof h->format);
  else if (strcmp(key, "plugin") == 0) omx_preset_string(v, h->plugin, sizeof h->plugin);
  else if (strcmp(key, "name") == 0) omx_preset_string(v, h->name, sizeof h->name);
  else if (strcmp(key, "params") == 0 && *v == '[') h->params = v;
  return true;
}

/* The file's head; false unless it is an org.openmixer.preset/1 file for plugin `id` with a name
 * and a params list. */
static inline bool omx_preset_head(const char *text, const char *id, struct omx_preset_head *h) {
  memset(h, 0, sizeof *h);
  if (!text || !omx_preset_members(text, omx_preset_head_member, h)) return false;
  return strcmp(h->format, OMX_PRESET_FORMAT) == 0 && strcmp(h->plugin, id) == 0 && h->name[0] && h->params;
}

struct omx_preset_param {
  char symbol[64];
  double value;
  bool has_value;
};

static inline bool omx_preset_param_member(const char *key, const char *v, void *ctx) {
  struct omx_preset_param *q = (struct omx_preset_param *)ctx;
  if (strcmp(key, "symbol") == 0) omx_preset_string(v, q->symbol, sizeof q->symbol);
  else if (strcmp(key, "value") == 0) {
    char *end;
    q->value = strtod(v, &end);
    q->has_value = end != v;
  }
  return true;
}

/* Each entry of the params array (h->params): fn(symbol, value, ctx). An entry without a symbol or
 * a numeric value is skipped; false if the array is malformed. */
static inline bool omx_preset_params(const struct omx_preset_head *h, void (*fn)(const char *symbol, double value, void *ctx), void *ctx) {
  const char *s = omx_preset_ws(h->params + 1);
  if (*s == ']') return true;
  for (;;) {
    struct omx_preset_param q = {{0}, 0.0, false};
    if (!omx_preset_members(s, omx_preset_param_member, &q)) return false;
    if (q.symbol[0] && q.has_value) fn(q.symbol, q.value, ctx);
    s = omx_preset_skip(s);
    if (!s) return false;
    s = omx_preset_ws(s);
    if (*s == ']') return true;
    if (*s != ',') return false;
    s = omx_preset_ws(s + 1);
  }
}

#endif
