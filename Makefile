# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#
# Top-level build: every plugin under plugins/<name>/ has its own Makefile with `all`, `test`,
# `install` and `clean`.
#
#   make            build every plugin
#   make test       every plugin's tests (what the package builds run), then `make hints`
#   make hints      no LV2 face lost a port hint: tools/port-hints.mjs against each declaration's `portHints` pin
#   make recipe-test   the repository's own checks, run by CI beside `make test`:
#     make completeness  every plugin against recipes/plugin.recipe.json, each gap naming its wizard step;
#                        a gap not in recipes/completeness-debt.json fails, and so does a paid debt entry
#     make selftest      the recipe's checkers, wizard and commit protocol, each sabotaged (needs git)
#   make install    DESTDIR, PREFIX, LIBDIR, CLAPDIR: $(CLAPDIR)/<name>.clap and $(LIBDIR)/lv2/<name>.lv2/
#   make install-devel   the public headers other projects include: $(INCLUDEDIR)/omx-plugins/*.h
#   make version    the release version, the one packaging/omx-plugins.spec and debian/changelog carry
VERSION := 0.2.0
PLUGINS := $(sort $(dir $(wildcard plugins/*/Makefile)))
PREFIX     ?= /usr
INCLUDEDIR ?= $(PREFIX)/include
# Every header a consumer includes: the shared parameter type, then each shipped plugin's generated
# parameter header and its instance header (the DSP shell around the omx-dsp kernel). A plugin's
# headers are the .h files beside its Makefile and in its generated/ directory.
HEADERS := include/omx_plugin_param.h $(foreach p,$(PLUGINS),$(wildcard $(p)*.h) $(wildcard $(p)generated/*.h))

.PHONY: all test hints install install-devel clean version recipe-test completeness selftest
all:
	@for p in $(PLUGINS); do $(MAKE) -C $$p all || exit 1; done
test:
	@if [ -z "$(PLUGINS)" ]; then echo "no plugins yet"; fi
	@for p in $(PLUGINS); do $(MAKE) -C $$p test || exit 1; done
	@$(MAKE) -s hints
hints:
	node tools/gen.mjs --check
	node tools/gen.mjs
	node tools/port-hints.mjs
# A plugin is complete when it has every artifact the recipe lists and keeps every law; a new plugin
# starts with `node tools/omx-new-plugin.mjs --from-contract <kernel>` (docs/new-plugin-from-contract.md).
recipe-test: selftest completeness
completeness:
	node tools/plugin-recipe.mjs
selftest:
	node tools/test/recipe-selftest.mjs
install:
	@for p in $(PLUGINS); do $(MAKE) -C $$p install || exit 1; done
install-devel:
	install -d $(DESTDIR)$(INCLUDEDIR)/omx-plugins
	install -m644 $(HEADERS) $(DESTDIR)$(INCLUDEDIR)/omx-plugins/
clean:
	@for p in $(PLUGINS); do $(MAKE) -C $$p clean; done
version:
	@echo $(VERSION)
