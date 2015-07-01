#ifndef CLOUD_WLAN_AP_INFO_LIST_H_
#define CLOUD_WLAN_AP_INFO_LIST_H_


enum ap_online_status
{
	AP_ON_LINE,
	AP_OFF_LINE
};
enum ap_exist_status
{
	AP_EXIST,
	AP_INEXIST,
};


/*这个结构要挂到cwlan_ap_node->cmd_list*/
typedef struct cwlan_cmd_node
{
	struct list_head list;
	u32 size;
	s8 data[0];
}cwlan_cmd_node_t;
/*这个结构要挂到cwlan_ap_node->user_list*/
typedef struct cwlan_ap_user_node
{
	struct list_head list;
	time_t create_timer;
	online_user_info_t node;
}cwlan_ap_user_node_t;

typedef struct cwlan_ap_node
{
	struct list_head list;
	struct list_head user_hlist;
	struct list_head cmd_hlist;
	time_t update_timer;//心跳更新时间
	time_t create_timer;//服务ap 上线时间
	u32 online_status;//本结点在线状态，０为上线，非０为下线；
	ap_local_info_t ap_info;
}cwlan_ap_node_t;

typedef struct cwlan_ap_hash
{
	struct list_head h_head;
	pthread_mutex_t mutex;
}cwlan_ap_hash_t;

#define CWLAN_AP_INFO_HASH_LEN 1024
#define CWLAN_AP_INFO_HASH_LEN_SECTION 32//老化是要用到

extern u32 cw_ap_info_list_init();
extern u32 cw_ap_info_list_exit();

/*以来为web开放认证ap接口*/

extern u32 cw_update_ap_info_list_age_del(u32 key);
extern u32 cw_update_ap_info_list(struct sockaddr *client_addr, ap_local_info_t ap_info);
extern u32 cw_update_user_info_list(struct sockaddr *client_addr, online_user_info_t user_info);
extern u32 cw_update_web_set_cmd_list(struct sockaddr *client_addr, cwlan_cmd_sockt_t *cmd);
extern u32 cw_web_get_ap_info_list(struct sockaddr *client_addr, u8 *apmac);
extern u32 cw_web_del_all_ap_info_list_node();

/******************************************/


/*以来为web管理认证ap接口*/
u32 cw_admin_update_ap_info_list_age_del(u32 key);
u32 cw_admin_update_user_info_list(struct sockaddr *client_addr, online_user_info_t user_info);
u32 cw_admin_update_web_set_cmd_list(struct sockaddr *client_addr, cwlan_cmd_sockt_t *cmd);
u32 cw_admin_web_get_ap_info_list(struct sockaddr *client_addr, u8 *apmac);
u32 cw_admin_web_add_ap_node(struct sockaddr *client_addr, u8 *apmac);
u32 cw_admin_web_del_ap_node(struct sockaddr *client_addr, u8 *apmac);
u32 cw_admin_update_ap_info_list(struct sockaddr *client_addr, ap_local_info_t ap_info);
/******************************************/


#endif
