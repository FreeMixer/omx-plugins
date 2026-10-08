#!/usr/bin/make -f
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#
# omx-strip, the channel strip, as CLAP and LV2, in plain C, from ONE declaration (omx-strip.decl.json).
# No DSP here: omx_strip.h calls omx-dsp's strip modules (trim + HPF/LPF, gate, EQ, compressor) in order.
#
#   make            build/omx-strip.clap and build/omx-strip.lv2/ (binary, TTL)
#   make test       freshness of every generated file, the CLAP parameters against the declaration,
#                   the strip oracle (both faces = the modules called in sequence, every order, at
#                   44.1/48/96/192 kHz), and the host checks (omx-clap-scan, lv2info) where installed
#   make gen        regenerate generated/ from the declaration (commit the result)
#   make install    DESTDIR/PREFIX/LIBDIR: $(LIBDIR)/clap/omx-strip.clap, $(LIBDIR)/lv2/omx-strip.lv2/
#
# Build deps: omx-dsp (pkg-config omxdsp, >= OMXDSP_MIN), omx-clap-core (the CLAP ABI's openmixer
# extensions), clap, lv2; for the tests lilv and node.

NAME       := omx-strip
URI        := urn:openmixer:strip
CLAP_ID    := org.openmixer.strip
OMXDSP_MIN ?= 0.1.8
ROOT       := $(abspath ../..)
GEN        := generated
B          := build
BUNDLE     := $(B)/$(NAME).lv2

PKG_CONFIG ?= pkg-config
ifeq ($(shell $(PKG_CONFIG) --atleast-version=$(OMXDSP_MIN) omxdsp && echo y),)
$(error omx-dsp >= $(OMXDSP_MIN) not found by `$(PKG_CONFIG) omxdsp`: install omx-dsp-devel (see BUILDING.md))
endif
DSP_CFLAGS := $(shell $(PKG_CONFIG) --cflags omxdsp)
DSP_LIBS   := $(shell $(PKG_CONFIG) --libs omxdsp)
ABI_CFLAGS := $(shell $(PKG_CONFIG) --cflags omx-clap-core clap 2>/dev/null)
LV2_CFLAGS := $(shell $(PKG_CONFIG) --cflags lv2 2>/dev/null)
LILV       := $(shell $(PKG_CONFIG) --cflags --libs lilv-0 2>/dev/null)

CC      ?= cc
CFLAGS  ?= -O2 -g
WARN    := -std=gnu11 -Wall -Wextra -Werror
PLUGIN  := -fPIC -shared -fvisibility=hidden -I$(GEN) -I$(ROOT)/include $(DSP_CFLAGS)
# omx-dsp is a static archive: the oversampler the gate and the compressor run is linked in, and
# -z defs refuses a plugin that would leave a symbol for the host's dlopen to miss.
LDLIBS  := -Wl,-z,defs $(DSP_LIBS) -lm

PREFIX  ?= /usr
LIBDIR  ?= $(PREFIX)/lib

TTL      := $(GEN)/$(NAME).lv2
TTL_SRCS := $(shell find $(TTL) -type f 2>/dev/null)
CORE     := omx_strip.h $(GEN)/omx_strip_params.h Makefile

all: $(B)/$(NAME).clap $(BUNDLE)/$(NAME).so $(BUNDLE)/manifest.ttl

$(B)/$(NAME).clap: omx_strip_clap.c $(CORE) | $(B)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARN) $(PLUGIN) $(ABI_CFLAGS) -o $@ $< $(LDFLAGS) $(LDLIBS)

$(BUNDLE)/$(NAME).so: omx_strip_lv2.c $(CORE) | $(B)
	@mkdir -p $(BUNDLE)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARN) $(PLUGIN) $(LV2_CFLAGS) -o $@ $< $(LDFLAGS) $(LDLIBS)

# The bundle's text is generated and committed; the build copies it beside the binary.
$(BUNDLE)/manifest.ttl: $(TTL_SRCS) | $(B)
	@mkdir -p $(BUNDLE)
	cp -r $(TTL)/. $(BUNDLE)/

$(B):
	@mkdir -p $@

# The oracle reads the declaration's table (names, symbols, defaults) and calls the modules itself;
# it never includes omx_strip.h.
$(B)/strip-oracle: test/strip-oracle.c $(GEN)/omx_strip_params.h | $(B)
	$(CC) $(CFLAGS) $(WARN) -I$(GEN) -I$(ROOT)/include $(DSP_CFLAGS) $(ABI_CFLAGS) -o $@ $< $(LILV) -ldl $(DSP_LIBS) -lm

gen:
	node $(ROOT)/tools/gen.mjs $(CURDIR)

check:
	node $(ROOT)/tools/gen.mjs --check $(CURDIR)

test: all check $(B)/strip-oracle
	$(B)/strip-oracle clap-params $(B)/$(NAME).clap > $(B)/clap-params.txt
	node $(ROOT)/tools/clap-params-check.mjs $(CURDIR) $(B)/clap-params.txt
	$(B)/strip-oracle clap $(B)/$(NAME).clap
	$(B)/strip-oracle lv2 $(BUNDLE) $(URI)
	sh $(ROOT)/tools/host-check.sh $(B)/$(NAME).clap $(BUNDLE) $(URI) $(CLAP_ID)

install: all
	install -Dm755 $(B)/$(NAME).clap $(DESTDIR)$(LIBDIR)/clap/$(NAME).clap
	mkdir -p $(DESTDIR)$(LIBDIR)/lv2
	cp -r $(BUNDLE) $(DESTDIR)$(LIBDIR)/lv2/

clean:
	rm -rf $(B)

.PHONY: all gen check test install clean
