# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Pau Aliagas <linuxnow@gmail.com>
#
# Top-level build: every plugin under plugins/<name>/ has its own Makefile with `all`, `test`,
# `install` and `clean`.
#
#   make            build every plugin
#   make test       every plugin's tests
#   make install    DESTDIR, PREFIX, LIBDIR: $(LIBDIR)/clap/<name>.clap and $(LIBDIR)/lv2/<name>.lv2/
#   make version    the release version, the one packaging/omx-plugins.spec and debian/changelog carry
VERSION := 0.1.0
PLUGINS := $(sort $(dir $(wildcard plugins/*/Makefile)))

.PHONY: all test install clean version
all:
	@for p in $(PLUGINS); do $(MAKE) -C $$p all || exit 1; done
test:
	@if [ -z "$(PLUGINS)" ]; then echo "no plugins yet"; fi
	@for p in $(PLUGINS); do $(MAKE) -C $$p test || exit 1; done
install:
	@for p in $(PLUGINS); do $(MAKE) -C $$p install || exit 1; done
clean:
	@for p in $(PLUGINS); do $(MAKE) -C $$p clean; done
version:
	@echo $(VERSION)
