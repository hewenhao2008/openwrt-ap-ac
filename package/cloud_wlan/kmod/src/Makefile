MODULE_NAME := cloud_wlan
RESMAN_CORE_OBJS:=cloud_wlan_main.o

RESMAN_GLUE_OBJS:=  cloud_wlan_http_reply.o cloud_wlan_session.o \
					cloud_wlan_nl_k_if.o cloud_wlan_hhparse.o \
					cloud_wlan_log.o
		#cloud_wlan_http_reply.o \
		#cloud_wlan_hhparse.o \
		#cloud_wlan_forward_test.o
$(MODULE_NAME)-objs:=$(RESMAN_GLUE_OBJS) $(RESMAN_CORE_OBJS)
obj-m :=cloud_wlan.o
