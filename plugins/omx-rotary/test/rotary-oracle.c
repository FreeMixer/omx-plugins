// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * rotary-oracle.c — omx-rotary's kernel-identity test: both built faces, sample for sample, against
 * omx-dsp's rotary kernel called DIRECTLY on the same blocks, at every declared rate, engaged and
 * bypassed — no tolerance.
 *
 *   rotary-oracle clap <omx-rotary.clap>
 *   rotary-oracle lv2 <omx-rotary.lv2> <uri>
 *
 * OMX_WIZARD_STUB: written by tools/omx-new-plugin.mjs, red until it is written. The oracles of the
 * plugins beside this one (plugins/omx-drive/test/drive-oracle.c) show the shape: a deterministic
 * signal in uneven blocks with parameter changes mid-stream, the reference computed through
 * <omxdsp/fx/omx_rotary.h> here, the output compared with memcmp. Delete this paragraph and the
 * marker line below when it is.
 */
#define OMX_WIZARD_STUB 1

#include <stdio.h>

int main(int argc, char **argv) {
  printf("FAIL kernel-identity %s: plugins/omx-rotary/test/rotary-oracle.c is the wizard's stub; write the "
         "comparison against omx-dsp's rotary kernel\n",
         argc > 1 ? argv[1] : "");
  return 1;
}
