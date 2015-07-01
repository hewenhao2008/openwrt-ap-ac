#ifndef CLOUD_WLAN_CFG_UPDATE_H_
#define CLOUD_WLAN_CFG_UPDATE_H_


#define DNS_CONFIG_FILE "/etc/config/network"
#define CW_AP_LOG_FILE "/etc/config/cw_ap.log"
#define CW_AP_LOG_MAX 1024*1024

#define CW_AP_KLOG_FILE "/var/cw_ap_url.log"
#define CW_AP_KLOG_MAX 1024*1024

/*本地与ac通信公共socket结构信息*/
typedef struct ap_udp_com
{
	u32 server_ip;
	u32 client_ip;
	u16 server_port;
	u16 client_port;
	s32 sockfd;
	struct sockaddr_in client_addr;
	struct sockaddr_in server_addr;
	s8 client_mac[6];
}ap_udp_socket_t;

typedef struct cpu_info
{
	u32 user; 
	u32 nice;
	u32 system;
	u32 idle;
	u32 iowait;
	u32 irp;
	u32 softirp;
	u32 stealstolen;
	u32 guest;
}cpu_info_t;

typedef struct com_cfg
{
	u32 ap_cwlan_sw;		//云ap内核转发模块开关
	u32 ap_klog_sw;
	u32 ap_klog_mode;
	u32 ac_com_addr;	//云ac的公网地址
	u32 ac_com_port;	//云ac的端口
	s8 ap_com_eth[10];	//本地出接口名
}com_cfg_t;


extern com_cfg_t g_ap_com_cfg;
extern u32 cw_cfg_dynamic_update_init();
extern u32 cw_cfg_dynamic_update_exit();

extern u32 cw_ap_local_db_com_cfg_init(sqlite3 * db);


#endif

