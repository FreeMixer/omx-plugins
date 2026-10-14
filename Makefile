# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#
# Top-level build: every plugin under plugins/<name>/ has its own Makefile with `all`, `test`,
# `install` and `clean`.
#
#   make            build every plugin
#   make test       every plugin's tests (what the package builds run), then `make hints` and `make look`
#   make hints      no LV2 face lost a port hint: tools/port-hints.mjs against each declaration's `portHints` pin
#   make look       tools/look.json, the MOD GUI's colours and font, is what tools/look/tokens.css resolves to
#   make recipe-test   the repository's own checks, run by CI beside `make test`:
#     make completeness  every plugin against recipes/plugin.recipe.json, each gap naming its wizard step;
#                        a gap not in recipes/completeness-debt.json fails, and so does a paid debt entry
#     make selftest      the recipe's checkers, wizard and commit protocol, each sabotaged (needs git)
#   make meters-abi   include/omx_clap_meters.h against omx-clap-core's clap_meters.h: same id, enums, sizeof and
#                    offsetof (tools/meters-abi-check.sh; fetches omx-clap-core's header by the installed tag, so it
#                    needs the network and is not part of `make test`, which the package builds run offline)
#   make strip-crosscheck   omx-strip, a chain, against the hand-ported strip it replaced, bit for bit at
#                    the old plugin's settings (tools/test/strip-crosscheck.sh; needs the git history)
#   make install    DESTDIR, PREFIX, LIBDIR, CLAPDIR: $(CLAPDIR)/<name>.clap and $(LIBDIR)/lv2/<name>.lv2/
#   make lib        build/lib/libomxplugins.a: every shipped plugin's CLAP face and the table of their
#                   entries (include/omx_clap_static.h), for a host that runs them in-process
#   make lib-test   a C host linked against the archive opens every face through the table, then
#                   again through the installed omxplugins.pc (part of `make test`)
#   make install-devel   the public headers other projects include: $(INCLUDEDIR)/omx-plugins/*.h,
#                   and $(LIBDIR)/libomxplugins.a with $(LIBDIR)/pkgconfig/omxplugins.pc
#   make version    the release version, the one packaging/omx-plugins.spec and debian/changelog carry
VERSION := 0.6.0
PLUGINS := $(sort $(dir $(wildcard plugins/*/Makefile)))
PREFIX     ?= /usr
INCLUDEDIR ?= $(PREFIX)/include
LIBDIR     ?= $(PREFIX)/lib
# Every header a consumer includes: the shared parameter type, the panel description and parameter contract rows, then each shipped plugin's generated
# parameter header and its instance header (the DSP shell around the omx-dsp kernel). A plugin's
# headers are the .h files beside its Makefile and in its generated/ directory.
HEADERS := include/omx_plugin_param.h include/omx_panel_description.h include/omx_param_contract.h include/omx_clap_static.h $(foreach p,$(PLUGINS),$(wildcard $(p)*.h) $(wildcard $(p)generated/*.h))

# libomxplugins.a: each shipped plugin's CLAP face built with -DOMX_CLAP_STATIC (no `clap_entry`;
# its `omx_clap_entry_<kernel>` stays) and lib/omx_clap_static.c, the table of those entries by CLAP
# id that tools/gen.mjs fills. Position-independent, so a shared object (the console's engine) can
# link it. omxplugins.pc requires the omx-dsp .github/pins.txt names, whose archive the faces call.
B          := build
LIB_B      := $(B)/lib
LIB        := $(LIB_B)/libomxplugins.a
LIB_PC     := $(LIB_B)/omxplugins.pc
CLAP_FACES := $(foreach p,$(PLUGINS),$(wildcard $(p)omx_*_clap.c))
LIB_OBJS   := $(LIB_B)/omx_clap_static.o $(patsubst %.c,$(LIB_B)/%.o,$(notdir $(CLAP_FACES)))
OMXDSP_PIN := $(shell awk '$$1 == "omx-dsp" { print $$3 }' .github/pins.txt)
PKG_CONFIG ?= pkg-config
CC         ?= cc
AR         ?= ar
CFLAGS     ?= -O2 -g
LIB_WARN   := -std=gnu11 -Wall -Wextra -Werror
LIB_FLAGS  := -fPIC -fvisibility=hidden -DOMX_CLAP_STATIC -Iinclude
# read only by the targets that build the archive, so `make version` and the rest need no omx-dsp
lib_cflags = $(shell $(PKG_CONFIG) --cflags omxdsp omx-clap-core clap)

.PHONY: all lib lib-test test hints look install install-devel clean version recipe-test completeness selftest strip-crosscheck meters-abi
all: lib
	@for p in $(PLUGINS); do $(MAKE) -C $$p all || exit 1; done
lib: $(LIB) $(LIB_PC)
test: lib
	@if [ -z "$(PLUGINS)" ]; then echo "no plugins yet"; fi
	@for p in $(PLUGINS); do $(MAKE) -C $$p test || exit 1; done
	@$(MAKE) -s lib-test
	@$(MAKE) -s hints
	@$(MAKE) -s look
hints:
	node tools/gen.mjs --check
	node tools/gen.mjs
	node tools/port-hints.mjs
look:
	node tools/look.mjs --check
# A plugin is complete when it has every artifact the recipe lists and keeps every law; a new plugin
# starts with `node tools/omx-new-plugin.mjs --from-contract <kernel>` (docs/new-plugin-from-contract.md).
recipe-test: selftest completeness
completeness:
	node tools/plugin-recipe.mjs
selftest:
	node tools/test/recipe-selftest.mjs
meters-abi:
	tools/meters-abi-check.sh
	tools/meters-abi-check.sh --self-test
strip-crosscheck:
	sh tools/test/strip-crosscheck.sh
install:
	@for p in $(PLUGINS); do $(MAKE) -C $$p install || exit 1; done
install-devel: lib
	install -d $(DESTDIR)$(INCLUDEDIR)/omx-plugins $(DESTDIR)$(LIBDIR)/pkgconfig
	install -m644 $(HEADERS) $(DESTDIR)$(INCLUDEDIR)/omx-plugins/
	install -m644 $(LIB) $(DESTDIR)$(LIBDIR)/
	sed -e 's,@PREFIX@,$(PREFIX),' -e 's,@LIBDIR@,$(LIBDIR),' -e 's,@INCLUDEDIR@,$(INCLUDEDIR),' $(LIB_PC) \
	  > $(DESTDIR)$(LIBDIR)/pkgconfig/omxplugins.pc
	chmod 644 $(DESTDIR)$(LIBDIR)/pkgconfig/omxplugins.pc
clean:
	@for p in $(PLUGINS); do $(MAKE) -C $$p clean; done
	rm -rf $(LIB_B)

$(LIB): $(LIB_OBJS)
	rm -f $@
	$(AR) rcs $@ $^

$(LIB_B)/omx_clap_static.o: lib/omx_clap_static.c include/omx_clap_static.h | $(LIB_B)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LIB_WARN) $(LIB_FLAGS) $(lib_cflags) -c -o $@ $<

# One rule per face: its generated headers sit in its own folder.
define face_rule
$(LIB_B)/$(notdir $(1:.c=.o)): $(1) $(wildcard $(dir $(1))*.h $(dir $(1))generated/*.h) include/omx_panel_description.h include/omx_param_contract.h | $(LIB_B)
	$$(CC) $$(CPPFLAGS) $$(CFLAGS) $$(LIB_WARN) $$(LIB_FLAGS) -I$(dir $(1))generated $$(lib_cflags) -c -o $$@ $$<
endef
$(foreach f,$(CLAP_FACES),$(eval $(call face_rule,$(f))))

# The prefix, libdir and includedir are filled at install; the version and the omx-dsp pin here.
$(LIB_PC): lib/omxplugins.pc.in Makefile .github/pins.txt | $(LIB_B)
	sed -e 's,@VERSION@,$(VERSION),' -e 's,@OMXDSP_PIN@,$(OMXDSP_PIN),' $< > $@

$(LIB_B):
	@mkdir -p $@

# The consumer's proof: install-devel into a stage, then a C host built only from the staged
# omxplugins.pc (no -ldl, and dlopen must not be among its symbols) opens every face through the
# table. The ids it expects are each plugin folder's CLAP_ID, in folder order: the plugins' own.
LIB_STAGE := $(abspath $(LIB_B)/stage)
CLAP_IDS   = $(foreach p,$(PLUGINS),$(shell sed -n 's/^CLAP_ID *:= *//p' $(p)Makefile))
lib-test: lib
	rm -rf $(LIB_STAGE)
	$(MAKE) -s install-devel DESTDIR= PREFIX=$(LIB_STAGE) LIBDIR=$(LIB_STAGE)/lib
	PKG_CONFIG_PATH=$(LIB_STAGE)/lib/pkgconfig$${PKG_CONFIG_PATH:+:$$PKG_CONFIG_PATH} && export PKG_CONFIG_PATH && \
	  $(CC) $(CPPFLAGS) $(CFLAGS) $(LIB_WARN) $$($(PKG_CONFIG) --cflags omxplugins) -o $(LIB_B)/static-table-host \
	    tools/test/static-table-host.c $(LDFLAGS) $$($(PKG_CONFIG) --libs omxplugins)
	@if nm -u $(LIB_B)/static-table-host | grep -qw dlopen; then echo "FAIL static-table-host calls dlopen"; exit 1; fi
	@echo "PASS static-table-host links libomxplugins through omxplugins.pc, without dlopen"
	$(LIB_B)/static-table-host $(CLAP_IDS)
version:
	@echo $(VERSION)
