# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#
# Top-level build: every plugin under plugins/<name>/ has its own Makefile with `all`, `test`,
# `install` and `clean`.
#
#   make            build every plugin
#   make test       every plugin's tests (what the package builds run)
#   make recipe-test   the repository's own checks, run by CI beside `make test`:
#     make completeness  every plugin against recipes/plugin.recipe.json, each gap naming its wizard step
#     make selftest      the recipe's checkers, wizard and commit protocol, each sabotaged (needs git)
#   make install    DESTDIR, PREFIX, LIBDIR: $(LIBDIR)/clap/<name>.clap and $(LIBDIR)/lv2/<name>.lv2/
#   make version    the release version, the one packaging/omx-plugins.spec and debian/changelog carry
VERSION := 0.1.0
PLUGINS := $(sort $(dir $(wildcard plugins/*/Makefile)))

.PHONY: all test install clean version recipe-test completeness selftest
all:
	@for p in $(PLUGINS); do $(MAKE) -C $$p all || exit 1; done
test:
	@if [ -z "$(PLUGINS)" ]; then echo "no plugins yet"; fi
	@for p in $(PLUGINS); do $(MAKE) -C $$p test || exit 1; done
# A plugin is complete when it has every artifact the recipe lists and keeps every law; a new plugin
# starts with `node tools/omx-new-plugin.mjs --answers <file>`, which writes them.
recipe-test: selftest completeness
completeness:
	node tools/plugin-recipe.mjs
selftest:
	node tools/test/recipe-selftest.mjs
install:
	@for p in $(PLUGINS); do $(MAKE) -C $$p install || exit 1; done
clean:
	@for p in $(PLUGINS); do $(MAKE) -C $$p clean; done
version:
	@echo $(VERSION)
