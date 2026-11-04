// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
/*
 * param-contract-abi-test.c — org.openmixer.param-contract/1 as include/omx_param_contract.h declares
 * it is the table omx-clap-core's <omx-clap-host/omx_clap_ext.h> reads: version, fingerprint and get,
 * nothing after them, and the entry get fills, size and offset alike. A member added to either side,
 * or one side keeping a member the other dropped, stops this compiling.
 * Compile-only: `make param-contract-abi`, part of `make test`.
 */
#include "omx_param_contract.h"

#include <omx-clap-host/omx_clap_ext.h>

#include <stddef.h>
#include <string.h>

#define PTR sizeof(void (*)(void))
#define SAME_SIZE(a, b) _Static_assert(sizeof(a) == sizeof(b), "sizeof " #a " == sizeof " #b)
#define SAME_OFF(a, fa, b, fb) _Static_assert(offsetof(a, fa) == offsetof(b, fb), "offsetof " #a "." #fa " == " #b "." #fb)

/* the plugin's table: three functions, version, fingerprint and get, in that order */
_Static_assert(sizeof(omx_clap_plugin_param_contract_t) == 3 * PTR, "the param-contract table is version, fingerprint and get");
_Static_assert(offsetof(omx_clap_plugin_param_contract_t, version) == 0, "offsetof version");
_Static_assert(offsetof(omx_clap_plugin_param_contract_t, fingerprint) == PTR, "offsetof fingerprint");
_Static_assert(offsetof(omx_clap_plugin_param_contract_t, get) == 2 * PTR, "offsetof get");

/* the host's reading of it */
SAME_SIZE(omx_clap_plugin_param_contract_t, omx_clap_plugin_param_contract_ext_t);
SAME_OFF(omx_clap_plugin_param_contract_t, version, omx_clap_plugin_param_contract_ext_t, version);
SAME_OFF(omx_clap_plugin_param_contract_t, fingerprint, omx_clap_plugin_param_contract_ext_t, fingerprint);
SAME_OFF(omx_clap_plugin_param_contract_t, get, omx_clap_plugin_param_contract_ext_t, get);

/* the entry get fills */
SAME_SIZE(omx_clap_param_contract_info_t, omx_clap_param_contract_entry_t);
SAME_OFF(omx_clap_param_contract_info_t, param_id, omx_clap_param_contract_entry_t, param_id);
SAME_OFF(omx_clap_param_contract_info_t, symbol, omx_clap_param_contract_entry_t, symbol);
SAME_OFF(omx_clap_param_contract_info_t, unit, omx_clap_param_contract_entry_t, unit);
SAME_OFF(omx_clap_param_contract_info_t, scale, omx_clap_param_contract_entry_t, scale);
SAME_OFF(omx_clap_param_contract_info_t, step, omx_clap_param_contract_entry_t, step);
_Static_assert(OMX_CLAP_PARAM_SYMBOL_SIZE == OMX_CLAP_PARAM_CONTRACT_SYMBOL_SIZE, "the symbol size");

int main(void)
{
    return strcmp(OMX_PARAM_CONTRACT_ID, OMX_CLAP_EXT_PARAM_CONTRACT) != 0;
}
