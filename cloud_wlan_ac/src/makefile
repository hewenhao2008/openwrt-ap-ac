CC= gcc
APCOM_OBJ= cloud_wlan_ac_com.o cloud_wlan_ap_info_list.o cloud_wlan_ap_info_list_db.o cloud_wlan_recv_info_branch.o cloud_wlan_des.o cloud_wlan_db_cmd_parser.o 

INCLUDE:=-I ../include
PTHREAD_LIBS= -lpthread
CFLAGS= -W -g -Wall -Wno-unused-parameter -DUSE_RUNTIME_MODEL_CHECKS=1 -D_GNU_SOURCE `mysql_config --cflags --libs` $(INCLUDE) $(PTHREAD_LIBS) 

.PHONY: all
all: cloud_wlan_ac_com
cloud_wlan_ac_com:$(APCOM_OBJ)
	$(CC) $(CFLAGS) -o $@ $(APCOM_OBJ)

clear:
	rm -rf *.o cloud_wlan_ac_com