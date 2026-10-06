#!/usr/bin/make -f
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#
# omx-eq8 as CLAP and LV2, in plain C, from ONE declaration (omx-eq8.decl.json).
#
#   make            build/omx-eq8.clap and build/omx-eq8.lv2/ (binary, TTL)
#   make test       freshness of every generated file, the plugin probe (each face against the
#                   instance core called directly, bit for bit), the four-rate EQ oracle (closed-form
#                   responses), and the host checks (omx-clap-scan, lv2info) where installed
#                   (no MOD GUI: tools/modgui-gen.mjs does not draw selectors, and the band type and
#                   the pass-filter slopes are selectors)
#   make gen        regenerate generated/ from the declaration (commit the result)
#   make install    DESTDIR/PREFIX/LIBDIR: $(LIBDIR)/clap/omx-eq8.clap, $(LIBDIR)/lv2/omx-eq8.lv2/
#
# Build deps: omx-dsp (pkg-config omxdsp, >= OMXDSP_MIN), omx-clap-core (the CLAP ABI's openmixer
# extensions), clap, lv2; for the tests lilv and node.

NAME       := omx-eq8
URI        := urn:openmixer:eq8
CLAP_ID    := org.openmixer.eq8
OMXDSP_MIN ?= 0.1.4
ROOT       := $(abspath ../..)
GEN        := generated
B          := build
BUNDLE     := $(B)/$(NAME).lv2

PKG_CONFIG ?= pkg-config
ifeq ($(shell $(PKG_CONFIG) --atleast-version=$(OMXDSP_MIN) omxdsp && echo y),)
$(error omx-dsp >= $(OMXDSP_MIN) not found by `$(PKG_CONFIG) omxdsp`: install omx-dsp-devel (see BUILDING.md))
endif
DSP_CFLAGS := $(shell $(PKG_CONFIG) --cflags omxdsp)
ABI_CFLAGS := $(shell $(PKG_CONFIG) --cflags omx-clap-core clap 2>/dev/null)
LV2_CFLAGS := $(shell $(PKG_CONFIG) --cflags lv2 2>/dev/null)
LILV       := $(shell $(PKG_CONFIG) --cflags --libs lilv-0 2>/dev/null)

CC      ?= cc
CFLAGS  ?= -O2 -g
WARN    := -std=gnu11 -Wall -Wextra -Werror
PLUGIN  := -fPIC -shared -fvisibility=hidden -I$(GEN) -I$(ROOT)/include $(DSP_CFLAGS)
LDLIBS  := -lm

PREFIX  ?= /usr
LIBDIR  ?= $(PREFIX)/lib

TTL      := $(GEN)/$(NAME).lv2
TTL_SRCS := $(shell find $(TTL) -type f 2>/dev/null)

all: $(B)/$(NAME).clap $(BUNDLE)/$(NAME).so $(BUNDLE)/manifest.ttl

$(B)/$(NAME).clap: omx_eq8_clap.c omx_eq8_stereo.h $(GEN)/omx_eq8_params.h | $(B)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARN) $(PLUGIN) $(ABI_CFLAGS) -o $@ $< $(LDFLAGS) $(LDLIBS)

$(BUNDLE)/$(NAME).so: omx_eq8_lv2.c omx_eq8_stereo.h $(GEN)/omx_eq8_params.h | $(B)
	@mkdir -p $(BUNDLE)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARN) $(PLUGIN) $(LV2_CFLAGS) -o $@ $< $(LDFLAGS) $(LDLIBS)

# The bundle's text is generated and committed; the build copies them beside the binary.
$(BUNDLE)/manifest.ttl: $(TTL_SRCS) | $(B)
	@mkdir -p $(BUNDLE)
	cp -r $(TTL)/. $(BUNDLE)/

$(B):
	@mkdir -p $@

$(B)/plugin-probe: test/plugin-probe.c | $(B)
	$(CC) $(CFLAGS) -std=gnu11 -Wall -Wextra -I$(GEN) -I$(ROOT)/include $(DSP_CFLAGS) $(ABI_CFLAGS) $(LV2_CFLAGS) -o $@ $< -ldl $(LDLIBS)

$(B)/eq-oracle: test/eq-oracle.c | $(B)
	$(CC) $(CFLAGS) $(WARN) $(ABI_CFLAGS) -o $@ $< $(LILV) -ldl $(LDLIBS)

gen:
	node $(ROOT)/tools/gen.mjs $(CURDIR)

check:
	node $(ROOT)/tools/gen.mjs --check $(CURDIR)

# LV2 port indices for the probe, read back from the generated header, the one place they are
# numbered: in_l in_r out_l out_r latency, the first declared parameter (the rest follow it), enabled.
PORT = $(shell sed -n 's/^\#define OMX_EQ8_LV2_PORT_$(1) \([0-9]*\)u$$/\1/p' $(GEN)/omx_eq8_params.h)
LV2_PORTS = $(call PORT,IN_L) $(call PORT,IN_R) $(call PORT,OUT_L) $(call PORT,OUT_R) $(call PORT,LATENCY) \
  $(call PORT,FIRST_PARAM) $(call PORT,ENABLED)

test: all check $(B)/plugin-probe $(B)/eq-oracle
	$(B)/plugin-probe clap-params $(B)/$(NAME).clap > $(B)/clap-params.txt
	node $(ROOT)/tools/clap-params-check.mjs $(CURDIR) $(B)/clap-params.txt
	$(B)/plugin-probe clap-identity $(B)/$(NAME).clap
	$(B)/plugin-probe clap-reactivate $(B)/$(NAME).clap
	$(B)/plugin-probe lv2-identity $(BUNDLE)/$(NAME).so $(URI) $(LV2_PORTS)
	$(B)/plugin-probe clap-vs-lv2 $(B)/$(NAME).clap $(BUNDLE)/$(NAME).so $(URI) $(LV2_PORTS)
	$(B)/eq-oracle clap $(B)/$(NAME).clap
	$(B)/eq-oracle lv2 $(BUNDLE) $(URI)
	sh $(ROOT)/tools/host-check.sh $(B)/$(NAME).clap $(BUNDLE) $(URI) $(CLAP_ID)

install: all
	install -Dm755 $(B)/$(NAME).clap $(DESTDIR)$(LIBDIR)/clap/$(NAME).clap
	mkdir -p $(DESTDIR)$(LIBDIR)/lv2
	cp -r $(BUNDLE) $(DESTDIR)$(LIBDIR)/lv2/

clean:
	rm -rf $(B)

.PHONY: all gen check test install clean
