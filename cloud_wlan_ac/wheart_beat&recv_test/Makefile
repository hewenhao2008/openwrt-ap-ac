CC= gcc

#CC为交叉编译GCC路径，以后可以跟据情况修改
#CC=~/openwrt/main/staging_dir/toolchain-mips64_gcc-4.4.1_eglibc-2.10.1/usr/bin/mips64-openwrt-linux-gnu-gcc
#CC=/workPorjectCode/openwrt/trunk/staging_dir/toolchain-mips_34kc_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mips-openwrt-linux-uclibc-gcc
#LFLAGS+=
APCOM_OBJ=cloud_wlan_ap_com.o cloud_wlan_cfg_update.o cloud_wlan_des.o cloud_wlan_sqlite.o 

DEBUG_OBJ=cloud_wlan_cmd.o cloud_wlan_des.o cloud_wlan_sqlite.o

PTHREAD_LIBS = -lpthread -lsqlite3
PARAM= -W -g -Wall -Wno-unused-parameter -DUSE_RUNTIME_MODEL_CHECKS=1
INCLUDE:=-I ../include
CFLAGS:= $(PARAM) $(PTHREAD_LIBS) $(INCLUDE)

.PHONY: all
all: cloud_wlan_ap_com
cloud_wlan_ap_com:$(APCOM_OBJ)
	$(CC) $(CFLAGS) -o $@ $(APCOM_OBJ)
	
clear:
	rm *.o