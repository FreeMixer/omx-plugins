// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * meter-flags-abi-test.c — the two meter flag bits lib/omx_clap_face.c sets are the ones omx-clap-core's host reads
 * (src/clap_host.h): held is bit 8, a clip word bit 9. Built by tools/meter-held-check.sh with OMX_CLAP_CORE_SRC naming a
 * checkout of omx-clap-core whose src/ holds that header; a compile error is the red.
 */
#include <stdio.h>
#include "clap_host.h"
_Static_assert(OMX_CLAP_METER_IS_HELD == (1u << 8), "held is bit 8");
_Static_assert(OMX_CLAP_METER_HAS_CLIP == (1u << 9), "a clip word is bit 9");
int main(void) { return puts("ok   meter flags: held is bit 8, a clip word bit 9, as omx-clap-core's host reads them"), 0; }
