##############################################
# OpenWrtMakefile for cloud_wlan_ap_com program
#
#
# Most ofthe variables used here are defined in
# theinclude directives below. We just need to
# specifya basic description of the package,
# whereto build our program, where to find
# thesource files, and where to install the
#compiled program on the router.
#
# Be verycareful of spacing in this file.
# Indentsshould be tabs, not spaces, and
# thereshould be no trailing whitespace in
# linesthat are not commented.
#
##############################################
include $(TOPDIR)/rules.mk
# Nameand release number of this package
PKG_NAME:=cloud_wlan/umod
PKG_RELEASE:=1
 
 
# Thisspecifies the directory where we're going to build the program.
# Theroot build directory, $(BUILD_DIR), is by default the build_mipsel
#directory in your OpenWrt SDK directory
PKG_BUILD_DIR:= $(BUILD_DIR)/$(PKG_NAME)
 
 
include $(INCLUDE_DIR)/package.mk
 
 
 
# Specifypackage information for this program.
# Thevariables defined here should be self explanatory.
# If youare running Kamikaze, delete the DESCRIPTION
#variable below and uncomment the Kamikaze define
# directivefor the description below
define Package/cloud_wlan_ap_com
	#在Base system显示模块名
	SECTION:=cloud_wlan_umod
	#这个模块要加在Base system下
	CATEGORY:=cloud_wlan modules
	#这个模块提示信息
	TITLE:=cloud_wlan user support
	#安装的时候不自动识别一些动态库,少什么加什么
	DEPENDS:=+libpthread
	DEPENDS:=+libsqlite3
endef
 
 
 
# Specifywhat needs to be done to prepare for building the package.
# In ourcase, we need to copy the source files to the build directory.
# This isNOT the default.  The default uses thePKG_SOURCE_URL and the
#PKG_SOURCE which is not defined here to download the source from the web.
# Inorder to just build a simple program that we have just written, it is
# mucheasier to do it this way.
define Build/Prepare
	rm -rf $(PKG_BUILD_DIR)
	mkdir -p $(PKG_BUILD_DIR)
	\$(CP) ./src/* $(PKG_BUILD_DIR)/
	\$(CP) ../include/* $(PKG_BUILD_DIR)/
endef
 
 
# We donot need to define Build/Configure or Build/Compile directives
# Thedefaults are appropriate for compiling a simple program such as this one
 
 
# Specifywhere and how to install the program. Since we only have one file,
# thecloud_wlan_ap_com executable, install it by copying it to the /bin directory on
# therouter. The $(1) variable represents the root directory on the router running
#OpenWrt. The $(INSTALL_DIR) variable contains a command to prepare the install
#directory if it does not already exist. Likewise $(INSTALL_BIN) contains the
# commandto copy the binary file from its current location (in our case the build
#directory) to the install directory.
define Package/cloud_wlan_ap_com/install
	#生成可执行文件目录
	$(INSTALL_DIR) $(1)/usr/cloud_wlan/bin
	$(INSTALL_DIR) $(1)/usr/cloud_wlan/cfg
	$(INSTALL_DIR) $(1)/etc/init.d/
	#安装的时候不自动识别一些动态库需要cp到编译目录
	#$(CP) /lib/libpthread.so.0 $(1)/sbin
	#$(CP) /lib/libsqlite3.so.0 $(1)/sbin
	#echo $(INSTALL_BIN)
	#echo $(PKG_BUILD_DIR)
	#echo $1
	#编译目标文件到安装目录
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/cloud_wlan_ap_com $(1)/usr/cloud_wlan/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/cloud_wlan_cmd $(1)/usr/cloud_wlan/bin/
	$(INSTALL_BIN) ./etc/init.d/* $(1)/etc/init.d/
	$(INSTALL_BIN) ./cfg/* $(1)/usr/cloud_wlan/cfg/
endef
 
#[安装前执行的脚本 记得加上#!/bin/sh 没有就空着]
define Package/cloud_wlan_ap_com/preinst
    #!/bin/sh
endef

#[安装后执行的脚本 记得加上#!/bin/sh 没有就空着]
define Package/cloud_wlan_ap_com/postinst
    #!/bin/sh
	/etc/init.d/cwlan start
	
	rm -rf /etc/rc.d/S99cwlan
	ln -s /etc/init.d/cwlan /etc/rc.d/S99cwlan
endef

#[删除前执行的脚本 记得加上#!/bin/sh 没有就空着]
define Package/cloud_wlan_ap_com/prerm
    #!/bin/sh
	/etc/init.d/cwlan stop
endef

#[删除后执行的脚本 记得加上#!/bin/sh 没有就空着]
define Package/cloud_wlan_ap_com/postrm
    #!/bin/sh
	rm -rf /etc/rc.d/S99cwlan
	rm -rf /usr/cloud_wlan
	rm -rf /etc/config/cw_ap_cfg.db
	rm -rf /etc/config/cw_ap.log
endef
# Thisline executes the necessary commands to compile our program.
# Theabove define directives specify all the information needed, but this
# linecalls BuildPackage which in turn actually uses this information to
# build apackage.
$(eval $(call BuildPackage,cloud_wlan_ap_com))
