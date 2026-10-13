// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * meters-abi-probe.c — prints the layout of the clap.meters/1 declarations the header named by
 * -DMETERS_HEADER="<path>" gives: the extension id, the enum values, and sizeof/offsetof of each
 * structure. tools/meters-abi-check.sh builds it once against include/omx_clap_meters.h and once
 * against omx-clap-core's src/clap_meters.h and demands the two outputs be identical.
 */
#include METERS_HEADER

#include <stddef.h>
#include <stdio.h>

#define SIZE(t) printf("sizeof " #t " %zu\n", sizeof(t))
#define OFF(t, f) printf("offsetof " #t "." #f " %zu\n", offsetof(t, f))
#define VAL(v) printf(#v " %d\n", (int)(v))

int main(void)
{
    printf("CLAP_EXT_METERS %s\n", CLAP_EXT_METERS);
    VAL(CLAP_METER_KIND_OTHER);
    VAL(CLAP_METER_KIND_GAIN_REDUCTION);
    VAL(CLAP_METER_KIND_LEVEL);
    VAL(CLAP_METER_IS_COHERENT);
    SIZE(clap_meter_info_t);
    OFF(clap_meter_info_t, id);
    OFF(clap_meter_info_t, name);
    OFF(clap_meter_info_t, kind);
    OFF(clap_meter_info_t, flags);
    OFF(clap_meter_info_t, channel_count);
    OFF(clap_meter_info_t, min_value);
    OFF(clap_meter_info_t, max_value);
    SIZE(clap_plugin_meters_t);
    OFF(clap_plugin_meters_t, count);
    OFF(clap_plugin_meters_t, get_info);
    OFF(clap_plugin_meters_t, get_values);
    SIZE(clap_host_meters_t);
    OFF(clap_host_meters_t, changed);
    return 0;
}
