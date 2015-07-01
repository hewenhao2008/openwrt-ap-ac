#ifndef CLOUD_WLAN_NL_H_
#define CLOUD_WLAN_NL_H_



#define NETLINK_CWLAN		31	/*netlink　编号，最大为32*/
/** 最大数据负荷(固定) **/  
#define MAX_DATA_PAYLOAD 512
/** 最大协议负荷(固定) **/  
#define MAX_PROTOCOL_LPAYLOAD (MAX_DATA_PAYLOAD + 8)
/*加密数据太大长度*/
#define CW_DES_LEN (MAX_DATA_PAYLOAD/2)
/*白名单最大值*/
#define CW_WHITE_LIST_MAX 100
/*URL最大长度*/
#define CW_LOCATION_URL_DATA_LEN 1024
/*URL, 注意后边是不需要加"/"的*/
#define CW_LOCATION_URL_DATA "http://www.108608.com/portal/index.jsp"
/*IP*/
#define CW_LOCATION_PORT 8080
/* 域名ip*/
#define CW_LOCATION_URL_IP_MAX 10

#define CLOUD_WLAN_WHITE_LIST_MAX_U 50


enum nl_tpye
{
/*本地debug　查看内核的基本信息*/
	CW_NLMSG_DEBUG_SHOW_ONLINE_USER,
	CW_NLMSG_DEBUG_SHOW_WHITE_LIST,
	CW_NLMSG_DEBUG_SHOW_PORTAL,
	CW_NLMSG_PUT_ONLINE_INFO_TO_AC,	//上报本地在线用户信息
	CW_NLMSG_GET_TEST,					//这个是测试用的
	
	CW_NLMSG_AP_DEBUG_MAX,

/****************************************/
	
/*apac公共用到的一些状态码*/
	CW_NLMSG_RES_OK	,					//回复ok
	CW_NLMSG_RES_FAIL ,					//回复FAIL
	
	CW_NLMSG_POBLIC_MAX,

/****************************************/

/*ap与ac之间通信命令*/

	CW_NLMSG_SET_OFF,					//全局开关
	CW_NLMSG_SET_ON	,				
	CW_NLMSG_SET_DEBUG_OFF,				//全局调试开关
	CW_NLMSG_SET_DEBUG_ON,
	
	CW_NLMSG_UPDATE_PORTAL,			//设置portal报文的localtion内容
	CW_NLMSG_UPDATE_WHITE_LIST,			//全局的目的地址白名单
	CW_NLMSG_UPDATE_SESSION_CFG,			//全局的会话基础配置
	CW_NLMSG_SET_KLOG_OFF,		//全局日志信息开关
	CW_NLMSG_SET_KLOG_ON,
	CW_NLMSG_PUT_KLOG_INFO,
	
	CW_NLMSG_SET_REBOOT,		// 重新ap
	CW_NLMSG_SET_WAN_PPPOE,		//设置ap wan接口为pppoe拨号模式
	CW_NLMSG_SET_WAN_DHCP,		//设置ap wan接口为dhcp 模式
	CW_NLMSG_SET_WIFI_INFO,		//设置ap wifi信息15
	CW_NLMSG_SET_USER_PID,		//设置一个全局的用户太pid
	
	CW_NLMSG_HEART_BEAT, //心中报文
	CW_NLMSG_SET_HEART_BEAT_INTERVAL, //心中报文发送间隔
	
	CW_NLMSG_AP_MAX,

/****************************************/

/*ac与web之间的通信命令从开始*/

	CW_NLMSG_WEB_SET_AP_CONFIG,
	CW_NLMSG_WEB_GET_AP_INFO,
	CW_NLMSG_PUT_ONLINE_INFO_TO_WEB, //上报本地在线用户信息到web
	CW_NLMSG_WEB_ADD_AP_NODE,
	CW_NLMSG_WEB_DEL_AP_NODE,
	CW_NLMSG_WEB_AP_OPEN_AUTH,
	CW_NLMSG_WEB_AP_ADMIN_AUTH,

	CW_NLMSG_AC_MAX
};

enum flow_session_status
{
	CW_FS_UP,		//在线状态
	CW_FS_DOWN,		//下线状态
	CW_FS_MAX
};


enum klog_mode
{
	REAL_TIME,
	UNREAL_TIME
};

enum klog_type
{
	KLOG_URL,
	KLOG_MAX
};

typedef struct dns_white_list
{
	u32 number;
	u32 list[CW_WHITE_LIST_MAX];
}dns_white_list_t;

typedef struct dns_protal_url
{
	u32 data_len;
	s8 data[];
}dns_protal_url_t;

/*url 过滤地址结构*/
typedef struct ac_udp_white_list
{
	u32 id;
	u32 len;	//只是本结构中数据的data长度
	u8 *data;	//字符需要加\0结构符号
}ac_udp_white_list_t;

typedef struct cwlan_flow_session_cfg
{
	u32 over_time;				//结点超时时间，以秒为单位
	u32 interval_timer;		//定时器执行间隔时间，以秒为单位
	u32 flow_max;		//最大可用流量，字节计
	u32 del_time;		//用户删除结点时间
}cwlan_flow_session_cfg_t;


typedef struct reHttp
{
	u32 destIp[CW_LOCATION_URL_IP_MAX];	//重定向指定目的地址
	u16 destPort;	//重定向指定的端口号
	s8 Location[CW_LOCATION_URL_DATA_LEN];	//重定向指定的URL
}reHttp_t;

typedef struct pppoe_cfg
{
	s8 username[64];
	s8 password[64];
}pppoe_cfg_t;


enum encryption_type
{
	EN_NONE,
	EN_WEP_OPEN,
	EN_WEP_SHARE,
	EN_WAP_PSK,
	EN_WAP2_PSK,
	EN_WAP_MIX,
	EN_MAX
};
enum arithmetic_type
{
	ALG_CCMP,
	ALG_TKIP,
	ALG_MIX,
	ALG_MAX
};
typedef struct encryption_cfg_info
{
	u8 arithmetic;
	u8 key_len;
	u8 key[128];//设置无线密码
}encryption_cfg_info_t;

typedef struct wifi
{
	u8 wlan_id;
	u8 disabled;//打开无线
	u8 txpower;    //设置功率为17dbm 太高会烧无线模块
	u8 channel;	  //设置无线信道为6
	//s8 mode;    //设置无线模式为ap
	u8 ssid_len;
	u8 ssid[128];    //设置无线SSID
	u8 en_type;    //设置加密为WPA2-PSK
	encryption_cfg_info_t en_info;
}wifi_cfg_t;


typedef struct ap_local_info 
{
	u8 apmac[6];
	u32 mem_total; // 单位是K
	u32 mem_free;
	u32 cpu_idle_rate;
	u64 run_time;	//单位是秒，开机到目前运行了多久
}ap_local_info_t;

typedef struct online_user_info
{
	u32 userip;
	u8 usermac[6];
	u8 apmac[6];
	u32 status;
	u64 time;	//上下线当前时间
}online_user_info_t;

/*所有数据通信结构*/
typedef struct kmod_log_info
{
	u32 size;
	u32 type;
	u32 userip;
	u8 usermac[6];
	u8 apmac[6];
	u64 time;
	u8 data[];
}kmod_log_info_t;

/*所有数据通信结构*/
typedef struct dcma_udp_info
{
	u32 type;	//命令类型
	u32 number;	//data 个数
	u8 data[];	
}dcma_udp_skb_info_t;

#endif

