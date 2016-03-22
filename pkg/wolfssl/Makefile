PKG_NAME=wolfssl
PKG_URL=https://github.com/wolfSSL/wolfssl
PKG_VERSION=master
PKG_DIR=$(CURDIR)/$(PKG_NAME)

.PHONY: all clean distclean

export RIOT_CFLAGS = ${CFLAGS} ${INCLUDES}

all: patch
	"$(MAKE)" -C $(PKG_DIR)/src

patch: $(PKG_DIR)/Makefile

$(PKG_DIR)/Makefile: $(PKG_DIR)/.git/config
	cd "$(PKG_DIR)" && git am --ignore-whitespace "$(CURDIR)"/*.patch

$(PKG_DIR)/.git/config:
	test -d "$(PKG_DIR)" || $(GITCACHE) clone "$(PKG_URL)" "$(PKG_VERSION)" "$(PKG_DIR)"

clean::
	@echo "Cleaning up $(PKG_NAME) package..."
	@cd "$(PKG_DIR)" 2> /dev/null > /dev/null && \
		git clean -x -f && \
		git am --abort && \
		git reset --hard "$(PKG_VERSION)" && \
		$(MAKE) patch || true

distclean::
	rm -rf "$(PKG_DIR)"

Makefile.include:
	@true
