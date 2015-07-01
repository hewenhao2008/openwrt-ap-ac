#include <stdio.h> 
#include <stdlib.h> 
#include <pthread.h> 
#include <unistd.h> 
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#include "cloud_wlan_list.h"
#include "cloud_wlan_types.h"
#include "cloud_wlan_nl.h"
#include "cloud_wlan_ac_com.h"
#include "cloud_wlan_recv_info_branch.h"
#include "cloud_wlan_ap_info_list.h"
#include "cloud_wlan_ap_info_list_db.h"

static pthread_attr_t attr;
static pthread_t g_ap_age_pt;

u32 g_ac_service_mode = AC_ADMIN_DB_MODE;

void *cw_pthread_hear_beat_dispose(void *data)
{
	struct sockaddr client_info;
	ap_local_info_t ap_info;
	dcma_pthread_info_t *pthread_para = (dcma_pthread_info_t *)data;

	memcpy((void *)&client_info, (void *)pthread_para->client_info, sizeof(struct sockaddr));
	memcpy((void *)&ap_info, pthread_para->data, sizeof(ap_local_info_t));

	//更新ap信息，没有则新加一个
	//查看是否有发送信息
	
	if(g_ac_service_mode == AC_OPEN_MEM_MODE)
	{
		cw_update_ap_info_list(&client_info, ap_info);
	}
	else if(g_ac_service_mode == AC_ADMIN_MEM_MODE)
	{
		cw_admin_update_ap_info_list(&client_info, ap_info);
	}
	else
	{

	}
	return NULL;
}
void *cw_pthread_online_user_dispose(void *data)
{
	struct sockaddr client_info;
	online_user_info_t user_info;
	dcma_pthread_info_t *pthread_para = (dcma_pthread_info_t *)data;

	memcpy((void *)&client_info, (void *)pthread_para->client_info, sizeof(struct sockaddr));
	memcpy((void *)&user_info, pthread_para->data, sizeof(online_user_info_t));
	//更新在线用户信息，ap不存在，什么都不做。ap存在，没有添加一个
	
	if(g_ac_service_mode == AC_OPEN_MEM_MODE)
	{
		cw_update_user_info_list(&client_info, user_info);
	}
	else if(g_ac_service_mode == AC_ADMIN_MEM_MODE)
	{
		cw_admin_update_user_info_list(&client_info, user_info);
	}
	else
	{
		cw_admin_db_update_user_info_list(&client_info, user_info);
	}
	
	return NULL;
}

void *cw_pthread_web_set_cfg_dispose(void *data)
{
	struct sockaddr client_info;
	cwlan_cmd_sockt_t *cmd;
	dcma_pthread_info_t *pthread_para = (dcma_pthread_info_t *)data;
	memcpy((void *)&client_info, (void *)pthread_para->client_info, sizeof(struct sockaddr));

	cmd = (cwlan_cmd_sockt_t *)pthread_para->data;

	//查ap并挂配置到buff

	if(g_ac_service_mode == AC_OPEN_MEM_MODE)
	{
		cw_update_web_set_cmd_list(&client_info, cmd);
	}
	else if(g_ac_service_mode == AC_ADMIN_MEM_MODE)
	{
		cw_admin_update_web_set_cmd_list(&client_info, cmd);
	}
	else
	{
	}

	return NULL;
}
void *cw_pthread_web_get_cfg_dispose(void *data)
{
	//查ap信息发到web
	
	struct sockaddr client_info;
	u8 apmac[6];
	
	dcma_pthread_info_t *pthread_para = (dcma_pthread_info_t *)data;
	memcpy((void *)&client_info, (void *)pthread_para->client_info, sizeof(struct sockaddr));
	memcpy((void *)&apmac, pthread_para->data, 6);

	//查ap并发送到web
	
	if(g_ac_service_mode == AC_OPEN_MEM_MODE)
	{
		cw_web_get_ap_info_list(&client_info, apmac);
	}
	else if(g_ac_service_mode == AC_ADMIN_MEM_MODE)
	{
		cw_admin_web_get_ap_info_list(&client_info, apmac);
	}
	else
	{

	}
	
	return NULL;
}
void *cw_pthread_web_add_ap_node_dispose(void *data)
{
	//添加ap ，重复性由数据库去把握
	
	struct sockaddr client_info;
	u8 apmac[6];
	
	dcma_pthread_info_t *pthread_para = (dcma_pthread_info_t *)data;
	memcpy((void *)&client_info, (void *)pthread_para->client_info, sizeof(struct sockaddr));
	memcpy((void *)&apmac, pthread_para->data, 6);

	cw_admin_web_add_ap_node(&client_info, apmac);
	return NULL;
}
void *cw_pthread_web_del_ap_node_dispose(void *data)
{
	//删除ap ，重复性由数据库去把握
	
	struct sockaddr client_info;
	u8 apmac[6];
	
	dcma_pthread_info_t *pthread_para = (dcma_pthread_info_t *)data;
	memcpy((void *)&client_info, (void *)pthread_para->client_info, sizeof(struct sockaddr));
	memcpy((void *)&apmac, pthread_para->data, 6);

	cw_admin_web_del_ap_node(&client_info, apmac);
	return NULL;
}
void *cw_pthread_online_ap_age_del_dispose(void *data)
{
	u32 key = 0;
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	/*以下为系统默认方式*/
	//pthread_setcanceltype(PTHREAD_CANCEL_DEFFERED, NULL);

	while(1)
	{
		key = key % CWLAN_AP_INFO_HASH_LEN_SECTION;
		pthread_testcancel();
		if(g_ac_service_mode == AC_OPEN_MEM_MODE)
		{
			cw_update_ap_info_list_age_del(key);
		}
		else if(g_ac_service_mode == AC_ADMIN_MEM_MODE)
		{
			cw_admin_update_ap_info_list_age_del(key);
		}
		else if(g_ac_service_mode == AC_ADMIN_DB_MODE)
		{
			cw_admin_db_online_user_age_del();
		}
		
		key++;
		
		pthread_testcancel();
		sleep(2);
	}
	return NULL;
}
u32 cw_ac_auth_switch_to_admin(dcma_pthread_info_t *pthread_para)
{
	dcma_udp_skb_info_t buff;

	if(g_ac_service_mode == AC_ADMIN_MEM_MODE)
	{
		return CWLAN_OK;
	}
	
	/*因为是弱条件转强条件需要删除所有结点
	重新添加ap*/
	g_ac_service_mode = AC_ADMIN_MEM_MODE;
	cw_web_del_all_ap_info_list_node();

	buff.type = CW_NLMSG_RES_OK;
	buff.number = 1;
	cw_sendto_info(pthread_para->client_info, (s8 *)&buff, sizeof(dcma_udp_skb_info_t));
	return CWLAN_OK;
}
u32 cw_ac_auth_switch_to_open(dcma_pthread_info_t *pthread_para)
{
	dcma_udp_skb_info_t buff;
	if(g_ac_service_mode == AC_OPEN_MEM_MODE)
	{
		return CWLAN_OK;
	}
	/*因为是强条件转弱条件不需要删除所有结点*/
	
	g_ac_service_mode = AC_OPEN_MEM_MODE;
	
	buff.type = CW_NLMSG_RES_OK;
	buff.number = 1;
	cw_sendto_info(pthread_para->client_info, (s8 *)&buff, sizeof(dcma_udp_skb_info_t));
	return CWLAN_OK;
}

/*
四类报文:
1, ap心跳更新
2,ap在线更新
3,web获取ap info
4,web下发配置
*/

void cw_recv_info_branch(dcma_udp_skb_info_t *buff, u32 len, struct sockaddr *client_info)
{
	s32 ret = 0;
	pthread_t pt = 0;
	dcma_pthread_info_t pthread_para;
	pthread_para.client_info = client_info;
	pthread_para.data = buff->data;
	
	//printf("buff->type = %x, len = %d\n", buff->type, len);
	switch(buff->type)	
	{
		case CW_NLMSG_HEART_BEAT:
			ret = pthread_create(&pt, &attr, cw_pthread_hear_beat_dispose, (void *)&pthread_para);	 
			break;
		case CW_NLMSG_PUT_ONLINE_INFO_TO_AC:	
			ret = pthread_create(&pt, &attr, cw_pthread_online_user_dispose, (void *)&pthread_para);	 
			break; 
		case CW_NLMSG_WEB_SET_AP_CONFIG:
			ret = pthread_create(&pt, &attr, cw_pthread_web_set_cfg_dispose, (void *)&pthread_para);	 
			break;
		case CW_NLMSG_WEB_GET_AP_INFO: 
			ret = pthread_create(&pt, &attr, cw_pthread_web_get_cfg_dispose, (void *)&pthread_para);	 
			break;
		case CW_NLMSG_WEB_ADD_AP_NODE:
			ret = pthread_create(&pt, &attr, cw_pthread_web_add_ap_node_dispose, (void *)&pthread_para);	 
			break;
		case CW_NLMSG_WEB_DEL_AP_NODE:
			ret = pthread_create(&pt, &attr, cw_pthread_web_del_ap_node_dispose, (void *)&pthread_para);	 
			break;


		case CW_NLMSG_WEB_AP_OPEN_AUTH:
			cw_ac_auth_switch_to_open(&pthread_para);
			break;
		case CW_NLMSG_WEB_AP_ADMIN_AUTH:
			cw_ac_auth_switch_to_admin(&pthread_para);
			break;
		default:
			printf("[unkown cmd]:%d\n", buff->type);
			break;
	} 
	if(0 != ret  )
	{
		printf("pthread_create [%d] fail\n", buff->type);
	}
	return;  
}
u32 cw_recv_info_dispose_pthread_cfg_init()
{
	u32 ret = 0;
	//struct sched_param  params;
	
	ret += pthread_attr_init(&attr);
	//ret += pthread_attr_setschedpolicy(&attr, SCHED_FIFO);

	//线程退出直接释放资源
	ret += pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	
	//ret += pthread_attr_setschedparam(&attr, &params);
	/*开启在线ap 老化线程*/
	ret += pthread_create(&g_ap_age_pt, NULL, cw_pthread_online_ap_age_del_dispose, NULL);	 


	return ret;
}
u32 cw_recv_info_dispose_pthread_cfg_exit()
{
	u32 ret = 0;
	//struct sched_param  params;
	/*kill 在线ap 老化线程*/
	while( !pthread_cancel(g_ap_age_pt))
	{};
	printf("online_ap_age_del_dispose kill ok\n");


	return ret;
}

