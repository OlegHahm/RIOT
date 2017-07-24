PKG_NAME=openwsn-fw
PKG_URL=$(HOME)/git/openwsn-fw
PKG_VERSION=RIOT_port
PKG_BUILDDIR ?= $(BINDIRBASE)/pkg/$(BOARD)/$(PKG_NAME)
PKG_LICENSE=BSD-3-Clause

.PHONY: all

all: git-download
	$(MAKE) -C $(PKG_BUILDDIR)

include $(RIOTBASE)/pkg/pkg.mk
