#include <unistd.h> 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>

#include "cloud_wlan_list.h"
#include "cloud_wlan_types.h"
#include "cloud_wlan_nl.h"
#include "cloud_wlan_ac_com.h"
#include "cloud_wlan_recv_info_branch.h"
#include "cloud_wlan_ap_info_list.h"
#include "cloud_wlan_ac_log.h"

s8 log_path[50] = "./ac.log";


u32 cw_ac_log_proc();
{

	return CWLAN_OK;
}

u32 cw_ac_log_init()
{

	return CWLAN_OK;
}


