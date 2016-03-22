PKG_NAME=mbedtls
PKG_URL=https://github.com/ARMmbed/mbedtls
PKG_VERSION=master
PKG_DIR=$(CURDIR)/$(PKG_NAME)

.PHONY: all clean distclean

export RIOT_CFLAGS = ${CFLAGS} ${INCLUDES}
export CFLAGS = -Wno-old-style-definition

export CFLAGS += $(INCLUDES) -I$(RIOTBASE)/sys/include -I$(RIOTBASE)/sys/net/include \
			-I$(RIOTBASE)/sys/posix/include -I$(RIOTBASE)/sys/libc/include

all: patch
	"$(MAKE)" -C $(PKG_DIR)/library

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
