#ifndef CLOUD_WLAN_AP_INFO_LIST_DB_H_
#define CLOUD_WLAN_AP_INFO_LIST_DB_H_

#define CW_MYSQL_LEN 1024

extern s8 cw_table_dir[100];
extern s8 cw_online_table_name[100];
extern s8 cw_user_table_name[100];
extern s8 cw_cmd_table_name[100];
extern s8 mysql_db_service[100];
extern s8 mysql_db[50];
extern s8 user_name[50];
extern s8 password[50];

extern u32 cw_admin_db_update_ap_info_list(struct sockaddr *client_addr, ap_local_info_t ap_info);
extern u32 cw_admin_db_update_user_info_list(struct sockaddr *client_addr, online_user_info_t user_info);
extern u32 cw_admin_db_online_user_age_del();

extern u32 cw_ap_info_list_db_init();
extern u32 cw_ap_info_list_db_exit();


#endif
