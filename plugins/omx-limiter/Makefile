#!/usr/bin/make -f
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#
# omx-limiter as CLAP and LV2, in plain C, from ONE declaration (omx-limiter.decl.json).
#
#   make            build/omx-limiter.clap and build/omx-limiter.lv2/ (binary, TTL)
#   make test       freshness of every generated file, the parameters as declared, every parameter
#                   live, the limiter kernel-identity oracle, and the
#                   host checks (omx-clap-scan, lv2info) where installed
#   make gen        regenerate generated/ from the declaration (commit the result)
#   make install    DESTDIR/PREFIX/LIBDIR/CLAPDIR: $(CLAPDIR)/omx-limiter.clap, $(LIBDIR)/lv2/omx-limiter.lv2/
#
# Build deps: omx-dsp (pkg-config omxdsp, >= OMXDSP_MIN), omx-clap-core (the CLAP ABI's openmixer
# extensions), clap, lv2; for the tests lilv and node.

NAME       := omx-limiter
URI        := urn:openmixer:limiter
CLAP_ID    := org.openmixer.limiter
OMXDSP_MIN ?= 0.2.0
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
PLUGIN  := -fPIC -shared -Wl,-z,defs -fvisibility=hidden -I$(GEN) -I$(ROOT)/include $(DSP_CFLAGS)
LDLIBS  := $(DSP_LIBS)

PREFIX  ?= /usr
LIBDIR  ?= $(PREFIX)/lib
# the path the CLAP specification gives Linux distributions, on every distribution and architecture
CLAPDIR ?= $(PREFIX)/lib/clap

TTL      := $(GEN)/$(NAME).lv2
TTL_SRCS := $(shell find $(TTL) -type f 2>/dev/null)
CORE     := omx_limiter_core.h $(GEN)/omx_limiter_params.h

all: $(B)/$(NAME).clap $(BUNDLE)/$(NAME).so $(BUNDLE)/manifest.ttl

$(B)/$(NAME).clap: omx_limiter_clap.c $(CORE) | $(B)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARN) $(PLUGIN) $(ABI_CFLAGS) -o $@ $< $(LDFLAGS) $(LDLIBS)

$(BUNDLE)/$(NAME).so: omx_limiter_lv2.c $(CORE) | $(B)
	@mkdir -p $(BUNDLE)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARN) $(PLUGIN) $(LV2_CFLAGS) -o $@ $< $(LDFLAGS) $(LDLIBS)

# The bundle's text are generated and committed; the build copies them beside the binary.
$(BUNDLE)/manifest.ttl: $(TTL_SRCS) | $(B)
	@mkdir -p $(BUNDLE)
	cp -r $(TTL)/. $(BUNDLE)/

$(B):
	@mkdir -p $@

$(B)/clap-probe: $(ROOT)/tools/clap-probe.c | $(B)
	$(CC) $(CFLAGS) $(WARN) $(ABI_CFLAGS) -o $@ $< -ldl

$(B)/limiter-oracle: test/limiter-oracle.c $(CORE) | $(B)
	$(CC) $(CFLAGS) $(WARN) -I$(GEN) -I$(ROOT)/include $(DSP_CFLAGS) $(ABI_CFLAGS) -o $@ $< $(LILV) -ldl $(LDLIBS)

gen:
	node $(ROOT)/tools/gen.mjs $(CURDIR)

check:
	node $(ROOT)/tools/gen.mjs --check $(CURDIR)

# The parameter test: the built CLAP face's parameters are the declaration's, and every one of them
# reaches the kernel. The kernel-identity test: both faces, sample for sample, against omx-dsp's
# limiter kernel called directly. Every test runs, and the target fails if any failed, so a new
# plugin shows all its red tests at once.
test: all check $(B)/clap-probe $(B)/limiter-oracle
	@fail=0; \
	$(B)/clap-probe params $(B)/$(NAME).clap > $(B)/clap-params.txt && \
	  node $(ROOT)/tools/clap-params-check.mjs $(CURDIR) $(B)/clap-params.txt || fail=1; \
	$(B)/clap-probe live $(B)/$(NAME).clap || fail=1; \
	$(B)/limiter-oracle clap $(B)/$(NAME).clap || fail=1; \
	$(B)/limiter-oracle lv2 $(BUNDLE) $(URI) || fail=1; \
	sh $(ROOT)/tools/host-check.sh $(B)/$(NAME).clap $(BUNDLE) $(URI) $(CLAP_ID) || fail=1; \
	exit $$fail

install: all
	install -Dm755 $(B)/$(NAME).clap $(DESTDIR)$(CLAPDIR)/$(NAME).clap
	mkdir -p $(DESTDIR)$(LIBDIR)/lv2
	cp -r $(BUNDLE) $(DESTDIR)$(LIBDIR)/lv2/

clean:
	rm -rf $(B)

.PHONY: all gen check test install clean
