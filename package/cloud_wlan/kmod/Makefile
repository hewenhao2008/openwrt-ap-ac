#
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=cloud_wlan/kmod
PKG_RELEASE:=1
PKG_BUILD_DIR:= $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define KernelPackage/cloud_wlan
#这个模块要加在Base system下
	SECTION:=cloud_wlan_kmod
	CATEGORY:=cloud_wlan modules
#DEPENDS:=@TARGET_octeon
	TITLE:=cloud_wlan kernel support
#DEPENDS:=@GPIO_SUPPORT +kmod-i2c-core +kmod-i2c-gpio
	FILES:=$(PKG_BUILD_DIR)/cloud_wlan.ko
#生成启动脚本配置文件
	AUTOLOAD:=$(call AutoLoad,30,cloud_wlan,1)
	KCONFIG:=
endef

define KernelPackage/cloud_wlan/description
 Kernel module for register a custom cloud_wlan platform device.
endef

EXTRA_KCONFIG:= \
	#CONFIG_I2C_GPIO_CUSTOM=m

EXTRA_CFLAGS:= \
	$(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=m,%,$(filter %=m,$(EXTRA_KCONFIG)))) \
	$(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=y,%,$(filter %=y,$(EXTRA_KCONFIG)))) \

MAKE_OPTS:= \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	SUBDIRS="$(PKG_BUILD_DIR)" \
	EXTRA_CFLAGS="$(EXTRA_CFLAGS)" \
	$(EXTRA_KCONFIG)

define Build/Prepare
	rm -rf $(PKG_BUILD_DIR)
	mkdir -p $(PKG_BUILD_DIR)
	\$(CP) ./src/* $(PKG_BUILD_DIR)/
	\$(CP) ../include/* $(PKG_BUILD_DIR)/
endef
define Package/ubox/install
	$(INSTALL_DIR) $(1)/etc/init.d/
	$(INSTALL_BIN) ./etc/init.d/* $(1)/etc/init.d/
endef
define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef
#[安装前执行的脚本 记得加上#!/bin/sh 没有就空着]
#kmod-cloud_wlan参数是编译目录是的文件名不是路径
    #!/bin/sh
define Package/kmod-cloud_wlan/preinst
endef
#[安装后执行的脚本 记得加上#!/bin/sh 没有就空着]
    #!/bin/sh
define Package/kmod-cloud_wlan/postinst
endef

#[删除前执行的脚本 记得加上#!/bin/sh 没有就空着]
define Package/kmod-cloud_wlan/prerm
    #!/bin/sh
	/etc/init.d/cwlan stop
endef

#[删除后执行的脚本 记得加上#!/bin/sh 没有就空着]
define Package/kmod-cloud_wlan/postrm
    #!/bin/sh
	rm -rf /lib/modules/3.10.32/cloud_wlan.ko
	rmmod cloud_wlan
endef

$(eval $(call KernelPackage,cloud_wlan))
