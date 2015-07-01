#ifndef CLOUD_WLAN_AP_COM_H_
#define CLOUD_WLAN_AP_COM_H_

extern u32 g_cw_debug;
/*这个日志不打印初始化日志信息*/
#define CW_DEBUG_U(str, args...)  \
           {\
                FILE *tfp;\
                if (g_cw_debug && (tfp = fopen("/etc/config/cw_ap.log", "a")) != NULL)\
                {\
						fprintf(tfp, str, ##args);\
                       	fclose(tfp);\
                }\
           }


#define CWLAN_AP_CFG_DB "/etc/config/cw_ap_cfg.db"
#define CWLAN_AP_CFG_TABLE "com_cfg"
#define CWLAN_AP_PORTAL_TABLE "portal_list"
#define CWLAN_AP_WHITE_TABLE "white_list"


typedef struct cw_nl_info
{
	s32 sockfd;  
	struct nlmsghdr *nlh;  
	struct sockaddr_nl src_addr;
	struct sockaddr_nl dst_addr;	
	struct msghdr msg;
}cw_nl_info_t;

extern s32 cloud_wlan_nl_cfg_init(void);
extern s32 cloud_wlan_nl_close(void);
extern s32 cloud_wlan_sendto_kmod(int type, s8 *buff, u32 datalen);
extern s32 cloud_wlan_sendto_kmod_ok(int type, s8 *buff, u32 datalen);

#endif
