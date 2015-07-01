#ifndef CLOUD_WLAN_PUB_H_
#define CLOUD_WLAN_PUB_H_

#define CWLAN_OK 0
#define CWLAN_FAIL 1


#define PROTO_DNS 53
#define PROTO_DHCP67 67
#define PROTO_DHCP68 68
#define PROTO_SNMP1 161
#define PROTO_SNMP2 162
#define PROTO_HTTP 80
#define PROTO_HTTP2 8080
#define PROTO_HTTPS 443
#define PROTO_SSH 22

#define PROTO_CAPWAP_C 5246
#define PROTO_CAPWAP_D 5247



extern rwlock_t g_rwlock;

extern u32 g_cloud_wlan_switch;
extern u32 g_cloud_wlan_debug;
extern u32 g_cloud_wlan_nlmsg_pid;

#define CLOUD_WLAN_DEBUG(str, args...)  \
{\
	if(g_cloud_wlan_debug)\
	{\
		printk(str, ##args);\
	}\
}
extern s32 cloud_wlan_sendto_umod(s32 type, s8 *buff, u32 datalen);


#endif

