include $(TOPDIR)/rules.mk

PKG_NAME:=pylon
PKG_VERSION:=1.0
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_BUILD_DEPENDS:=libcurl libsml

TARGET_LDFLAGS+= -Wl,-rpath-link=$(STAGING_DIR)/usr/lib

include $(INCLUDE_DIR)/package.mk

define Package/pylon
  SECTION:=utils
  CATEGORY:=Utilities
  DEPENDS:=+libcurl +libsml
  TITLE:=Pylon Smart Metering framework
endef

define Package/pylon/description
	Provides tools to access Smart Metering devices
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) -r ../../../src/* $(PKG_BUILD_DIR)/
endef

define Package/pylon/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/smlogger $(1)/usr/bin/
endef

$(eval $(call BuildPackage,pylon))

