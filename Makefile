# SPDX-License-Identifier: GPL-3.0-or-later
# Top-level build: every plugin under plugins/<name>/ has its own Makefile with `all` and `test`.
PLUGINS := $(sort $(dir $(wildcard plugins/*/Makefile)))

.PHONY: all test clean
all:
	@for p in $(PLUGINS); do $(MAKE) -C $$p all || exit 1; done
test:
	@if [ -z "$(PLUGINS)" ]; then echo "no plugins yet"; fi
	@for p in $(PLUGINS); do $(MAKE) -C $$p test || exit 1; done
clean:
	@for p in $(PLUGINS); do $(MAKE) -C $$p clean; done
