#!/usr/bin/make -f
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#
# omx-delay as CLAP and LV2, in plain C, from ONE declaration (omx-delay.decl.json).
#
#   make            build/omx-delay.clap and build/omx-delay.lv2/ (binary, TTL, MOD GUI)
#   make test       freshness of every generated file, the plugin probe, the four-rate delay oracle,
#                   the MOD GUI checks, and the host checks (omx-clap-scan, lv2info) where installed
#   make gen        regenerate generated/ from the declaration (commit the result)
#   make install    DESTDIR/PREFIX/LIBDIR/CLAPDIR: $(CLAPDIR)/omx-delay.clap, $(LIBDIR)/lv2/omx-delay.lv2/
#
# Build deps: omx-dsp (pkg-config omxdsp, >= OMXDSP_MIN), omx-clap-core (the CLAP ABI's openmixer
# extensions), clap, lv2; for the tests lilv and node.

NAME       := omx-delay
URI        := urn:openmixer:delay
CLAP_ID    := org.openmixer.delay
OMXDSP_MIN ?= $(shell awk '$$1 == "omx-dsp" { print $$3 }' $(abspath ../..)/.github/pins.txt)
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
# the path the CLAP specification gives Linux distributions, on every distribution and architecture
CLAPDIR ?= $(PREFIX)/lib/clap

TTL      := $(GEN)/$(NAME).lv2
TTL_SRCS := $(shell find $(TTL) -type f 2>/dev/null)

all: $(B)/$(NAME).clap $(BUNDLE)/$(NAME).so $(BUNDLE)/manifest.ttl

$(B)/$(NAME).clap: omx_delay_clap.c omx_delay_instance.h $(GEN)/omx_delay_params.h | $(B)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARN) $(PLUGIN) $(ABI_CFLAGS) -o $@ $< $(LDFLAGS) $(LDLIBS)

$(BUNDLE)/$(NAME).so: omx_delay_lv2.c omx_delay_instance.h $(GEN)/omx_delay_params.h | $(B)
	@mkdir -p $(BUNDLE)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARN) $(PLUGIN) $(LV2_CFLAGS) -o $@ $< $(LDFLAGS) $(LDLIBS)

# The bundle's text and images are generated and committed; the build copies them beside the binary.
$(BUNDLE)/manifest.ttl: $(TTL_SRCS) | $(B)
	@mkdir -p $(BUNDLE)
	cp -r $(TTL)/. $(BUNDLE)/

$(B):
	@mkdir -p $@

$(B)/plugin-probe: test/plugin-probe.c | $(B)
	$(CC) $(CFLAGS) -std=gnu11 -Wall -Wextra $(DSP_CFLAGS) $(ABI_CFLAGS) $(LV2_CFLAGS) -o $@ $< -ldl $(LDLIBS)

$(B)/delay-oracle: test/delay-oracle.c | $(B)
	$(CC) $(CFLAGS) $(WARN) $(DSP_CFLAGS) $(ABI_CFLAGS) -o $@ $< $(LILV) -ldl $(LDLIBS)

gen:
	node $(ROOT)/tools/gen.mjs $(CURDIR)
	node $(ROOT)/tools/modgui-gen.mjs $(CURDIR)

check:
	node $(ROOT)/tools/gen.mjs --check $(CURDIR)
	node $(ROOT)/tools/modgui-gen.mjs --check $(CURDIR)

# LV2 port indices for the probe, in its order: in_l in_r out_l out_r latency timeMs feedback mix
# tone pingpong enabled — read back from the generated header, the one place they are numbered.
PORT = $(shell sed -n 's/^\#define OMX_DELAY_LV2_PORT_$(1) \([0-9]*\)u$$/\1/p' $(GEN)/omx_delay_params.h)
LV2_PORTS = $(call PORT,IN_L) $(call PORT,IN_R) $(call PORT,OUT_L) $(call PORT,OUT_R) $(call PORT,LATENCY) \
  $(shell p=$(call PORT,FIRST_PARAM); echo $$p $$((p+1)) $$((p+2)) $$((p+3)) $$((p+4))) $(call PORT,ENABLED)

test: all check $(B)/plugin-probe $(B)/delay-oracle
	$(B)/plugin-probe clap-params $(B)/$(NAME).clap > $(B)/clap-params.txt
	node $(ROOT)/tools/clap-params-check.mjs $(CURDIR) $(B)/clap-params.txt
	$(B)/plugin-probe clap-identity $(B)/$(NAME).clap
	$(B)/plugin-probe clap-reactivate $(B)/$(NAME).clap
	$(B)/plugin-probe lv2-identity $(BUNDLE)/$(NAME).so $(URI) $(LV2_PORTS)
	$(B)/delay-oracle clap $(B)/$(NAME).clap
	$(B)/delay-oracle lv2 $(BUNDLE) $(URI)
	sh $(ROOT)/tools/modgui-test.sh $(CURDIR)
	sh $(ROOT)/tools/host-check.sh $(B)/$(NAME).clap $(BUNDLE) $(URI) $(CLAP_ID)

install: all
	install -Dm755 $(B)/$(NAME).clap $(DESTDIR)$(CLAPDIR)/$(NAME).clap
	mkdir -p $(DESTDIR)$(LIBDIR)/lv2
	cp -r $(BUNDLE) $(DESTDIR)$(LIBDIR)/lv2/

clean:
	rm -rf $(B)

.PHONY: all gen check test install clean
