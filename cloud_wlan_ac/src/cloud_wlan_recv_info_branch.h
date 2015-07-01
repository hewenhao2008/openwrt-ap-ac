#ifndef CLOUD_WLAN_RECV_INFO_BRANCH_H_
#define CLOUD_WLAN_RECV_INFO_BRANCH_H_


enum ac_servic_mode
{
	AC_ADMIN_DB_MODE,
	AC_ADMIN_MEM_MODE,
	AC_OPEN_MEM_MODE,
};


typedef struct dcma_pthread_info
{
	struct sockaddr *client_info;
	void *data;	
}dcma_pthread_info_t;

typedef struct cwlan_cmd_sockt
{
	u8 apmac[6];
	u32 size;
	s8 data[0];
}cwlan_cmd_sockt_t;

extern u32 g_ac_service_mode;

extern u32 cw_recv_info_dispose_pthread_cfg_init();
extern u32 cw_recv_info_dispose_pthread_cfg_exit();

extern void cw_recv_info_branch(dcma_udp_skb_info_t *buff, u32 len, struct sockaddr *client_info);


#endif
