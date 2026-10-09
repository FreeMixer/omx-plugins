// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * flanger-oracle.c — omx-flanger's kernel-identity test: both built faces, sample for sample, against
 * omx-dsp's flanger kernel called DIRECTLY on the same blocks, at every declared rate, engaged and
 * bypassed — no tolerance.
 *
 *   flanger-oracle clap <omx-flanger.clap>
 *   flanger-oracle lv2 <omx-flanger.lv2> <uri>
 *
 * OMX_WIZARD_STUB: written by tools/omx-new-plugin.mjs, red until it is written. The oracles of the
 * plugins beside this one (plugins/omx-drive/test/drive-oracle.c) show the shape: a deterministic
 * signal in uneven blocks with parameter changes mid-stream, the reference computed through
 * <omxdsp/fx/omx_flanger.h> here, the output compared with memcmp. Delete this paragraph and the
 * marker line below when it is.
 */
#define OMX_WIZARD_STUB 1

#include <stdio.h>

int main(int argc, char **argv) {
  printf("FAIL kernel-identity %s: plugins/omx-flanger/test/flanger-oracle.c is the wizard's stub; write the "
         "comparison against omx-dsp's flanger kernel\n",
         argc > 1 ? argv[1] : "");
  return 1;
}
