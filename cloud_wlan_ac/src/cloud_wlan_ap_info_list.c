#include <unistd.h> 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>

#include "cloud_wlan_list.h"
#include "cloud_wlan_types.h"
#include "cloud_wlan_nl.h"
#include "cloud_wlan_ac_com.h"
#include "cloud_wlan_recv_info_branch.h"
#include "cloud_wlan_ap_info_list.h"

static cwlan_ap_hash_t *g_ap_info_hash;
static s32 g_ap_heart_beat_interval = 3;

static s32 g_ap_info_list_debug = 1;
#define CW_AC_DEBUG(str, args...)  \
{\
	if(g_ap_info_list_debug)\
	{\
		printf(str, ##args);\
	}\
}


u32 BKDRHash(s8 *str, u32 initval)
{
	u32 seed = 131; // 31 131 1313 13131 131313 etc..	
	u32 hash = 0;
	while (*str)
	{
		hash = hash * seed + (*str++);
	}	
	return (hash % initval);
}

u32 get_hash_key(u8 *str, u32 initval)
{
	u32 i=0;
	u32 seed = 131; // 31 131 1313 13131 131313 etc..	
	u32 hash = 0;
	for(i=0; i<6; i++)
	{
		hash = hash * seed + *(str+i);
	}
	return (hash % initval);
}
u32 cw_check_cmd_list(struct sockaddr *client_addr, struct list_head *cmd_hlist)
{
	cwlan_cmd_node_t *cmd_node;
	cwlan_cmd_node_t *cmd_temp_node;
	list_for_each_entry_safe(cmd_node, cmd_temp_node, cmd_hlist, list)
	{
		cw_sendto_info(client_addr, cmd_node->data, cmd_node->size);
		list_del(&cmd_node->list);
		free((void *)cmd_node);
	}
	return CWLAN_OK;
}

u32 cw_check_user_list(struct list_head *user_hlist, online_user_info_t *user_info)
{
	s32 user_find_key = 0;
	cwlan_ap_user_node_t *user_node;
	cwlan_ap_user_node_t *user_temp_node;
	cwlan_ap_user_node_t *user_new_node;
	time_t current_timer;

	time(&current_timer);

	list_for_each_entry_safe(user_node, user_temp_node, user_hlist, list)
	{
		if( memcmp(user_node->node.usermac, user_info->usermac, 6))
		{	/*如果当前时间减上次更新时间大于60则为下线，因为正常更新时间为3秒*/
			if(current_timer - user_node->node.time > 60)
			{
				CW_AC_DEBUG("[AP USER AGE]:USER MAC = %02X-%02X-%02X-%02X-%02X-%02X\n",
					user_info->usermac[0],user_info->usermac[1],user_info->usermac[2],
					user_info->usermac[3],user_info->usermac[4],user_info->usermac[5]
					);
				list_del(&user_node->list);
				free((void *)user_node);
			}
			continue;
		}
		/*如果是下线报文，删除结点*/
		if(user_info->status == CW_FS_DOWN)
		{
			CW_AC_DEBUG("[AP USER DOWN]:USER MAC = %02X-%02X-%02X-%02X-%02X-%02X\n",
				user_node->node.usermac[0],user_node->node.usermac[1],user_node->node.usermac[2],
				user_node->node.usermac[3],user_node->node.usermac[4],user_node->node.usermac[5]
				);
			list_del(&user_node->list);
			free((void *)user_node);
		}
		else
		{
			CW_AC_DEBUG("[AP USER UPDATE]:USER MAC = %02X-%02X-%02X-%02X-%02X-%02X\n",
				user_node->node.usermac[0],user_node->node.usermac[1],user_node->node.usermac[2],
				user_node->node.usermac[3],user_node->node.usermac[4],user_node->node.usermac[5]
				);
			memcpy((void *)&user_node->node, (void *)user_info, sizeof(online_user_info_t));
		}
		user_find_key = -1;
	}
	if( 0 == user_find_key)
	{
		/*没查到相应的AP user记录，则新建*/
		
		user_new_node = (cwlan_ap_user_node_t *)malloc(sizeof(cwlan_ap_user_node_t));
		if(user_new_node != NULL)
		{
			memcpy((void *)&user_new_node->node, (void *)user_info, sizeof(online_user_info_t));	
			user_new_node->create_timer = current_timer;
			list_add(&user_new_node->list,user_hlist);
			CW_AC_DEBUG("[AP USER UP]:USER MAC = %02X-%02X-%02X-%02X-%02X-%02X\n",
				user_new_node->node.usermac[0],user_new_node->node.usermac[1],user_new_node->node.usermac[2],
				user_new_node->node.usermac[3],user_new_node->node.usermac[4],user_new_node->node.usermac[5]
				);
		}
	}
	return CWLAN_OK;

}
u32 cw_check_and_send_ap_info(struct sockaddr *client_addr, ap_local_info_t *ap_info, struct list_head *user_hlist)
{
	cwlan_ap_user_node_t *user_node;
	cwlan_ap_user_node_t *user_temp_node;

	s8 heartbeat_msg[MAX_PROTOCOL_LPAYLOAD]={0};
	dcma_udp_skb_info_t buff;
	u32 msg_len;

	/* 向web端发送数据信息 */
	msg_len = sizeof(ap_local_info_t) + sizeof(dcma_udp_skb_info_t);
	buff.type = CW_NLMSG_WEB_GET_AP_INFO;
	buff.number = 1;
	//memcpy(heartbeat_msg, (s8 *)&ap_info, sizeof(ap_local_info_t));
	 /* 向服务器发送数据信息 */
	memcpy(heartbeat_msg, (s8 *)&buff, sizeof(u32)*2);
	memcpy(heartbeat_msg+sizeof(u32)*2, (s8 *)&ap_info, sizeof(ap_local_info_t));

	cw_sendto_info(client_addr, heartbeat_msg, msg_len);

	list_for_each_entry_safe(user_node, user_temp_node, user_hlist, list)
	{
		msg_len = sizeof(online_user_info_t) + sizeof(dcma_udp_skb_info_t);
		buff.type = CW_NLMSG_PUT_ONLINE_INFO_TO_WEB;
		buff.number = 1;
		 /* 向服务器发送数据信息 */
		memcpy(heartbeat_msg, (s8 *)&buff, sizeof(u32)*2);
		memcpy(heartbeat_msg+sizeof(u32)*2, (s8 *)&user_node->node, sizeof(online_user_info_t));
		cw_sendto_info(client_addr, heartbeat_msg, msg_len);
	}
	buff.type = CW_NLMSG_RES_OK;
	buff.number = 1;
	 /* 向服务器发送数据信息 */
	cw_sendto_info(client_addr, (void *)&buff, sizeof(dcma_udp_skb_info_t));
	return CWLAN_OK;

}
u32 cw_check_cmd_list_del(struct list_head *cmd_hlist)
{
	cwlan_cmd_node_t *cmd_node;
	cwlan_cmd_node_t *cmd_temp_node;
	list_for_each_entry_safe(cmd_node, cmd_temp_node, cmd_hlist, list)
	{
		list_del(&cmd_node->list);
		free((void *)cmd_node);
	}
	return CWLAN_OK;
}

u32 cw_check_user_list_del(struct list_head *user_hlist)
{
	cwlan_ap_user_node_t *user_node;
	cwlan_ap_user_node_t *user_temp_node;

	list_for_each_entry_safe(user_node, user_temp_node, user_hlist, list)
	{
		list_del(&user_node->list);
		free((void *)user_node);
	}
	return CWLAN_OK;
}

u32 cw_update_ap_info_list_age_del(u32 key)
{
	u32 hash_key;
	u32 step_length;
	cwlan_ap_hash_t *hash_head;
	cwlan_ap_node_t *node;
	cwlan_ap_node_t *temp_node;
	time_t current_timer;

	time(&current_timer);

	step_length = CWLAN_AP_INFO_HASH_LEN/CWLAN_AP_INFO_HASH_LEN_SECTION;
	hash_key = key * step_length;
	for(; hash_key<hash_key+step_length && hash_key<CWLAN_AP_INFO_HASH_LEN; hash_key++)
	{
		hash_head = g_ap_info_hash + hash_key;
		pthread_mutex_lock(&hash_head->mutex);

	    list_for_each_entry_safe(node, temp_node, &hash_head->h_head, list)
		{
		/*3秒是心跳发送的时间间隔，
		这里因为会遍历时间会更长所以同样设置为３秒*/
			if( current_timer - node->update_timer > g_ap_heart_beat_interval * 3)
			{
				CW_AC_DEBUG("[AGE OLD]:AP MAC = %02X-%02X-%02X-%02X-%02X-%02X , time = %d\n",
					node->ap_info.apmac[0],node->ap_info.apmac[1],node->ap_info.apmac[2],
					node->ap_info.apmac[3],node->ap_info.apmac[4],node->ap_info.apmac[5],
					(u32)(current_timer - node->update_timer)
					);
				/*free ap node cmd_hlist user_hlist*/
				cw_check_cmd_list_del(&node->cmd_hlist);
				cw_check_user_list_del(&node->user_hlist);
				list_del(&node->list);
				free((void *)node);
			}
		}
		
		pthread_mutex_unlock(&hash_head->mutex);
	}
	return CWLAN_OK;
}
/*在更新的同时还需要处理配置下发的处理*/
u32 cw_update_ap_info_list(struct sockaddr *client_addr, ap_local_info_t ap_info)
{
	u32 hash_key;
	cwlan_ap_hash_t *hash_head;
	cwlan_ap_node_t *node;
	cwlan_ap_node_t *temp_node;
	cwlan_ap_node_t *new_node;
	dcma_udp_skb_info_t buff;
	time_t current_timer;

	time(&current_timer);

	hash_key = get_hash_key(ap_info.apmac, CWLAN_AP_INFO_HASH_LEN-1);

	hash_head = g_ap_info_hash + hash_key;
	pthread_mutex_lock(&hash_head->mutex);

    list_for_each_entry_safe(node, temp_node, &hash_head->h_head, list)
	{
	
		if( memcmp(node->ap_info.apmac, ap_info.apmac, 6))
		{
			continue;
		}
		CW_AC_DEBUG("[UPDATE AP INFO]:MAC = %02X-%02X-%02X-%02X-%02X-%02X\n",
			ap_info.apmac[0],ap_info.apmac[1],ap_info.apmac[2],
			ap_info.apmac[3],ap_info.apmac[4],ap_info.apmac[5]
			);
		//要更新内容
		node->update_timer = current_timer;
		memcpy((void *)&node->ap_info, (void *)&ap_info, sizeof(ap_local_info_t));
		//查看这个ap 需要回cmd
		buff.type = CW_NLMSG_HEART_BEAT;
		buff.number = 1;
		cw_sendto_info(client_addr, (s8 *)&buff, sizeof(dcma_udp_skb_info_t));

		cw_check_cmd_list(client_addr, &node->cmd_hlist);
		
		pthread_mutex_unlock(&hash_head->mutex);
		goto out;
	}
/*没查到相应的AP记录，需要新建一个结点*/
	CW_AC_DEBUG("[CREATE NEW AP]:MAC = %02X-%02X-%02X-%02X-%02X-%02X\n",
		ap_info.apmac[0],ap_info.apmac[1],ap_info.apmac[2],
		ap_info.apmac[3],ap_info.apmac[4],ap_info.apmac[5]
		);
	CW_AC_DEBUG("hash_key = %d\n",hash_key);


	new_node = malloc(sizeof(cwlan_ap_node_t));
	if(new_node != NULL)
	{
		new_node->update_timer = current_timer;
		new_node->create_timer= current_timer;
		memcpy((void *)&new_node->ap_info, (void *)&ap_info, sizeof(ap_local_info_t));		
		list_add(&new_node->list,&hash_head->h_head);
		INIT_LIST_HEAD(&new_node->user_hlist);
		INIT_LIST_HEAD(&new_node->cmd_hlist);
	}
	pthread_mutex_lock(&hash_head->mutex);

	buff.type = CW_NLMSG_HEART_BEAT;
	buff.number = 1;
	cw_sendto_info(client_addr, (s8 *)&buff, sizeof(dcma_udp_skb_info_t));
	buff.type = CW_NLMSG_SET_ON;
	buff.number = 1;
	cw_sendto_info(client_addr, (s8 *)&buff, sizeof(dcma_udp_skb_info_t));
out:

	return CWLAN_OK;
}

u32 cw_update_user_info_list(struct sockaddr *client_addr, online_user_info_t user_info)
{
	u32 hash_key;
	cwlan_ap_hash_t *hash_head;
	cwlan_ap_node_t *node;
	cwlan_ap_node_t *temp_node;

	hash_key = get_hash_key(user_info.apmac, CWLAN_AP_INFO_HASH_LEN-1);

	hash_head = g_ap_info_hash + hash_key;
	pthread_mutex_lock(&hash_head->mutex);

    list_for_each_entry_safe(node, temp_node, &hash_head->h_head, list)
	{
	
		if( memcmp(node->ap_info.apmac, user_info.apmac, 6))
		{
			continue;
		}
		cw_check_user_list(&node->user_hlist, &user_info);
		break;
	}
	
	pthread_mutex_lock(&hash_head->mutex);
	/*没查到相应的AP记录，什么都不做直接返回*/	
	return CWLAN_OK;
}
u32 cw_update_web_set_cmd_list(struct sockaddr *client_addr, cwlan_cmd_sockt_t *cmd)
{
	u32 hash_key;
	cwlan_ap_hash_t *hash_head;
	cwlan_ap_node_t *node;
	cwlan_ap_node_t *temp_node;

	cwlan_cmd_node_t *cmd_new_node;
	time_t current_timer;

	time(&current_timer);
	hash_key = get_hash_key(cmd->apmac, CWLAN_AP_INFO_HASH_LEN-1);

	hash_head = g_ap_info_hash + hash_key;
	pthread_mutex_lock(&hash_head->mutex);

    list_for_each_entry_safe(node, temp_node, &hash_head->h_head, list)
	{
		if( memcmp(node->ap_info.apmac, cmd->apmac, 6))
		{
			continue;
		}
		CW_AC_DEBUG("[WEB SET AP CMD]:SET MAC = %02X-%02X-%02X-%02X-%02X-%02X\n",
			cmd->apmac[0],cmd->apmac[1],cmd->apmac[2],
			cmd->apmac[3],cmd->apmac[4],cmd->apmac[5]
			);
		
		CW_AC_DEBUG("hash_key = %d\n",hash_key);
		cmd_new_node = malloc(sizeof(cwlan_cmd_node_t)+ cmd->size);
		if(cmd_new_node != NULL)
		{
			memcpy((void *)cmd_new_node->data, (void *)cmd->data, cmd->size); 
			cmd_new_node->size = cmd->size;
			
			list_add(&cmd_new_node->list,&node->cmd_hlist);
		}
		
		break;
	}
	
	pthread_mutex_unlock(&hash_head->mutex);
	/*没查到相应的AP记录，什么都不做直接返回*/
	
	return CWLAN_OK;
}

u32 cw_web_get_ap_info_list(struct sockaddr *client_addr, u8 *apmac)
{
	u32 hash_key;
	cwlan_ap_hash_t *hash_head;
	cwlan_ap_node_t *node;
	cwlan_ap_node_t *temp_node;

	
	hash_key = get_hash_key(apmac, CWLAN_AP_INFO_HASH_LEN-1);

	hash_head = g_ap_info_hash + hash_key;
	pthread_mutex_lock(&hash_head->mutex);

    list_for_each_entry_safe(node, temp_node, &hash_head->h_head, list)
	{
	
		if( memcmp(node->ap_info.apmac, apmac, 6))
		{
			continue;
		}
		cw_check_and_send_ap_info(client_addr, &node->ap_info, &node->user_hlist);
		CW_AC_DEBUG("[WEB GET AP CMD]:GET MAC = %02X-%02X-%02X-%02X-%02X-%02X\n",
			apmac[0],apmac[1],apmac[2],
			apmac[3],apmac[4],apmac[5]
			);
		
		CW_AC_DEBUG("hash_key = %d\n",hash_key);
		
		break;
	}
	
	pthread_mutex_unlock(&hash_head->mutex);
/*没查到相应的AP记录，需要新建一个结点*/
	return CWLAN_OK;
}

u32 cw_web_del_all_ap_info_list_node()
{
	u32 hash_key;
	cwlan_ap_hash_t *hash_head;
	cwlan_ap_node_t *node;
	cwlan_ap_node_t *temp_node;

	for(hash_key = 0; hash_key<CWLAN_AP_INFO_HASH_LEN; hash_key++)
	{
		hash_head = g_ap_info_hash + hash_key;

	    list_for_each_entry_safe(node, temp_node, &hash_head->h_head, list)
		{
			/*free ap node cmd_hlist user_hlist*/
			cw_check_cmd_list_del(&node->cmd_hlist);
			cw_check_user_list_del(&node->user_hlist);
			list_del(&node->list);
			free((void *)node);
		}
	}
	return CWLAN_OK;
}










u32 cw_admin_update_ap_info_list_age_del(u32 key)
{
	u32 hash_key;
	u32 step_length;
	cwlan_ap_hash_t *hash_head;
	cwlan_ap_node_t *node;
	cwlan_ap_node_t *temp_node;
	time_t current_timer;

	time(&current_timer);

	step_length = CWLAN_AP_INFO_HASH_LEN/CWLAN_AP_INFO_HASH_LEN_SECTION;
	hash_key = key * step_length;
	for(; hash_key<hash_key+step_length && hash_key<CWLAN_AP_INFO_HASH_LEN; hash_key++)
	{
		hash_head = g_ap_info_hash + hash_key;
		pthread_mutex_lock(&hash_head->mutex);

	    list_for_each_entry_safe(node, temp_node, &hash_head->h_head, list)
		{
		/*3秒是心跳发送的时间间隔，
		这里因为会遍历时间会更长所以同样设置为３秒*/
		//printf("overtime = %d\n", (s32)(current_timer - node->update_timer));
			if( current_timer - node->update_timer > g_ap_heart_beat_interval * 3)
			{
				CW_AC_DEBUG("[ADMIN AP AGE OLD]:AP MAC = %02X-%02X-%02X-%02X-%02X-%02X\n",
					node->ap_info.apmac[0],node->ap_info.apmac[1],node->ap_info.apmac[2],
					node->ap_info.apmac[3],node->ap_info.apmac[4],node->ap_info.apmac[5]
					);
				node->online_status = AP_OFF_LINE;
				node->create_timer = 0;
				node->update_timer = 0;
				/*free ap node cmd_hlist user_hlist*/
				cw_check_cmd_list_del(&node->cmd_hlist);
				cw_check_user_list_del(&node->user_hlist);
				//list_del(&node->list);
				//free((void *)node);
			}
		}
		
		pthread_mutex_unlock(&hash_head->mutex);
	}
	return CWLAN_OK;
}

u32 cw_admin_update_user_info_list(struct sockaddr *client_addr, online_user_info_t user_info)
{
	u32 hash_key;
	cwlan_ap_hash_t *hash_head;
	cwlan_ap_node_t *node;
	cwlan_ap_node_t *temp_node;

	hash_key = get_hash_key(user_info.apmac, CWLAN_AP_INFO_HASH_LEN-1);

	hash_head = g_ap_info_hash + hash_key;
	pthread_mutex_lock(&hash_head->mutex);

    list_for_each_entry_safe(node, temp_node, &hash_head->h_head, list)
	{
	
		if( memcmp(node->ap_info.apmac, user_info.apmac, 6))
		{
			continue;
		}
		if(node->online_status == AP_OFF_LINE)
		{
			break;
		}
		cw_check_user_list(&node->user_hlist, &user_info);
		
		CW_AC_DEBUG("[ADMIN UPDATE AP USER]: AP MAC = %02X-%02X-%02X-%02X-%02X-%02X\n",
			user_info.apmac[0],user_info.apmac[1],user_info.apmac[2],
			user_info.apmac[3],user_info.apmac[4],user_info.apmac[5]
			);
		CW_AC_DEBUG("[ADMIN UPDATE AP USER]:STA MAC = %02X-%02X-%02X-%02X-%02X-%02X\n",
			user_info.usermac[0],user_info.usermac[1],user_info.usermac[2],
			user_info.usermac[3],user_info.usermac[4],user_info.usermac[5]
			);
		
		CW_AC_DEBUG("hash_key = %d\n",hash_key);

		break;
	}
	
	pthread_mutex_unlock(&hash_head->mutex);
	/*没查到相应的AP记录，什么都不做直接返回*/	
	return CWLAN_OK;
}
u32 cw_admin_update_web_set_cmd_list(struct sockaddr *client_addr, cwlan_cmd_sockt_t *cmd)
{
	u32 hash_key;
	cwlan_ap_hash_t *hash_head;
	cwlan_ap_node_t *node;
	cwlan_ap_node_t *temp_node;
	dcma_udp_skb_info_t buff;
	cwlan_cmd_node_t *cmd_new_node;
	time_t current_timer;
	
	buff.type = CW_NLMSG_RES_FAIL;

	time(&current_timer);
	hash_key = get_hash_key(cmd->apmac, CWLAN_AP_INFO_HASH_LEN-1);

	hash_head = g_ap_info_hash + hash_key;
	pthread_mutex_lock(&hash_head->mutex);

    list_for_each_entry_safe(node, temp_node, &hash_head->h_head, list)
	{
	
		if( memcmp(node->ap_info.apmac, cmd->apmac, 6))
		{
			continue;
		}
		if(node->online_status == AP_OFF_LINE)
		{
			break;
		}
		
		cmd_new_node = malloc(sizeof(cwlan_cmd_node_t) + cmd->size);
		if(cmd_new_node != NULL)
		{
			memcpy((void *)cmd_new_node->data, (void *)cmd->data, cmd->size); 
			cmd_new_node->size = cmd->size;
			list_add(&cmd_new_node->list,&node->cmd_hlist);
			buff.type = CW_NLMSG_RES_OK;
		}
		
		CW_AC_DEBUG("[ADMIN WEB SET AP CMD]:SET MAC = %02X-%02X-%02X-%02X-%02X-%02X\n",
			cmd->apmac[0],cmd->apmac[1],cmd->apmac[2],
			cmd->apmac[3],cmd->apmac[4],cmd->apmac[5]
			);
		
		CW_AC_DEBUG("hash_key = %d\n",hash_key);
		break;
	}
	
	pthread_mutex_unlock(&hash_head->mutex);
	/*没查到相应的AP记录，什么都不做直接返回*/
	buff.type = CW_NLMSG_RES_OK;
	buff.number = 1;
	cw_sendto_info(client_addr, (s8 *)&buff, sizeof(dcma_udp_skb_info_t));
	return CWLAN_OK;
}

u32 cw_admin_web_get_ap_info_list(struct sockaddr *client_addr, u8 *apmac)
{
	u32 hash_key;
	cwlan_ap_hash_t *hash_head;
	cwlan_ap_node_t *node;
	cwlan_ap_node_t *temp_node;
	dcma_udp_skb_info_t buff;

	buff.type = CW_NLMSG_RES_FAIL;
	hash_key = get_hash_key(apmac, CWLAN_AP_INFO_HASH_LEN-1);

	hash_head = g_ap_info_hash + hash_key;
	pthread_mutex_lock(&hash_head->mutex);

    list_for_each_entry_safe(node, temp_node, &hash_head->h_head, list)
	{
	
		if( memcmp(node->ap_info.apmac, apmac, 6))
		{
			continue;
		}
		cw_check_and_send_ap_info(client_addr, &node->ap_info, &node->user_hlist);
		buff.type = CW_NLMSG_RES_OK;
		CW_AC_DEBUG("[ADMIN WEB GET AP CMD]:GET MAC = %02X-%02X-%02X-%02X-%02X-%02X\n",
			apmac[0],apmac[1],apmac[2],
			apmac[3],apmac[4],apmac[5]
			);
		CW_AC_DEBUG("hash_key = %d\n",hash_key);
		break;
	}
	
	pthread_mutex_unlock(&hash_head->mutex);
	buff.type = CW_NLMSG_RES_OK;
	buff.number = 1;
	cw_sendto_info(client_addr, (s8 *)&buff, sizeof(dcma_udp_skb_info_t));
	/*没查到相应的AP记录，需要新建一个结点*/
	return CWLAN_OK;
}


u32 cw_admin_web_add_ap_node(struct sockaddr *client_addr, u8 *apmac)
{
	u32 hash_key;
	cwlan_ap_hash_t *hash_head;
	cwlan_ap_node_t *node;
	cwlan_ap_node_t *temp_node;
	cwlan_ap_node_t *new_node;
	dcma_udp_skb_info_t buff;

	buff.type = CW_NLMSG_RES_FAIL;
	hash_key = get_hash_key(apmac, CWLAN_AP_INFO_HASH_LEN-1);

	hash_head = g_ap_info_hash + hash_key;
	pthread_mutex_lock(&hash_head->mutex);

    list_for_each_entry_safe(node, temp_node, &hash_head->h_head, list)
	{
	
		if( memcmp(node->ap_info.apmac, apmac, 6))
		{
			continue;
		}
		CW_AC_DEBUG("[ADMIN ADD AP is exist]:AP MAC = %02X-%02X-%02X-%02X-%02X-%02X\n",
			apmac[0],apmac[1],apmac[2],
			apmac[3],apmac[4],apmac[5]
			);
		CW_AC_DEBUG("hash_key = %d\n",hash_key);
		buff.type = CW_NLMSG_RES_OK;
		
		pthread_mutex_unlock(&hash_head->mutex);
		goto out;
	}
	CW_AC_DEBUG("[ADMIN ADD AP]:AP MAC = %02X-%02X-%02X-%02X-%02X-%02X\n",
		apmac[0],apmac[1],apmac[2],
		apmac[3],apmac[4],apmac[5]
		);
	CW_AC_DEBUG("hash_key = %d\n",hash_key);

	new_node = malloc(sizeof(cwlan_ap_node_t));
	if(new_node != NULL)
	{
		new_node->online_status = AP_OFF_LINE;
		new_node->update_timer = 0;
		time(&new_node->create_timer);
		memset((void *)&new_node->ap_info, 0, sizeof(ap_local_info_t));
		memcpy(new_node->ap_info.apmac, apmac, 6);
		list_add(&new_node->list,&hash_head->h_head);
		INIT_LIST_HEAD(&new_node->user_hlist);
		INIT_LIST_HEAD(&new_node->cmd_hlist);
		buff.type = CW_NLMSG_RES_OK;
	}
	pthread_mutex_unlock(&hash_head->mutex);

out:
	buff.number = 1;
	cw_sendto_info(client_addr, (s8 *)&buff, sizeof(dcma_udp_skb_info_t));
	return CWLAN_OK;

}
u32 cw_admin_web_del_ap_node(struct sockaddr *client_addr, u8 *apmac)
{
	u32 hash_key;
	cwlan_ap_hash_t *hash_head;
	cwlan_ap_node_t *node;
	cwlan_ap_node_t *temp_node;
	dcma_udp_skb_info_t buff;
	
	hash_key = get_hash_key(apmac, CWLAN_AP_INFO_HASH_LEN-1);

	hash_head = g_ap_info_hash + hash_key;

	pthread_mutex_lock(&hash_head->mutex);

	list_for_each_entry_safe(node, temp_node, &hash_head->h_head, list)
	{
	
		if( memcmp(node->ap_info.apmac, apmac, 6))
		{
			continue;
		}
		/*ap在web删除，这里不需要发关cw报文
			因为AP在收不到心跳回复报文时就会自动下线*/
		CW_AC_DEBUG("[ADMIN DEL AP]:AP MAC = %02X-%02X-%02X-%02X-%02X-%02X\n",
			node->ap_info.apmac[0],node->ap_info.apmac[1],node->ap_info.apmac[2],
			node->ap_info.apmac[3],node->ap_info.apmac[4],node->ap_info.apmac[5]
			);
		CW_AC_DEBUG("hash_key = %d\n",hash_key);
		
		/*free ap node cmd_hlist user_hlist*/
		cw_check_cmd_list_del(&node->cmd_hlist);
		cw_check_user_list_del(&node->user_hlist);
		list_del(&node->list);
		free((void *)node);
		break;
	}
	
	pthread_mutex_unlock(&hash_head->mutex);
	/*回复web管理设置ok*/
	buff.type = CW_NLMSG_RES_OK;
	buff.number = 1;
	cw_sendto_info(client_addr, (s8 *)&buff, sizeof(dcma_udp_skb_info_t));

	return CWLAN_OK;
}
/*这个更新ap状态是以web管理面新建，本身没有新加ap功能*/
/*在更新的同时还需要处理配置下发的处理*/
u32 cw_admin_update_ap_info_list(struct sockaddr *client_addr, ap_local_info_t ap_info)
{
	u32 hash_key;
	cwlan_ap_hash_t *hash_head;
	cwlan_ap_node_t *node;
	cwlan_ap_node_t *temp_node;
	dcma_udp_skb_info_t buff;
	time_t current_timer;

	time(&current_timer);

	hash_key = get_hash_key(ap_info.apmac, CWLAN_AP_INFO_HASH_LEN-1);

	hash_head = g_ap_info_hash + hash_key;

	pthread_mutex_lock(&hash_head->mutex);

    list_for_each_entry_safe(node, temp_node, &hash_head->h_head, list)
	{
	
		if( memcmp(node->ap_info.apmac, ap_info.apmac, 6))
		{
			continue;
		}
		//要更新内容
		memcpy((void *)&node->ap_info, (void *)&ap_info, sizeof(ap_local_info_t));
		CW_AC_DEBUG("[ADMIN UPDATE AP ONLINE]:AP MAC = %02X-%02X-%02X-%02X-%02X-%02X\n",
			node->ap_info.apmac[0],node->ap_info.apmac[1],node->ap_info.apmac[2],
			node->ap_info.apmac[3],node->ap_info.apmac[4],node->ap_info.apmac[5]
			);
		CW_AC_DEBUG("hash_key = %d\n",hash_key);
		node->update_timer = current_timer;
		/* 向AP发送启动云ap功能*/
		if(AP_OFF_LINE == node->online_status)
		{
			buff.type = CW_NLMSG_SET_ON;
			buff.number = 1;
			cw_sendto_info(client_addr, (s8 *)&buff, sizeof(dcma_udp_skb_info_t));
		}
		node->online_status = AP_ON_LINE;

		buff.type = CW_NLMSG_HEART_BEAT;
		buff.number = 1;
		cw_sendto_info(client_addr, (s8 *)&buff, sizeof(dcma_udp_skb_info_t));
		//查看这个ap 需要回cmd
		cw_check_cmd_list(client_addr, &node->cmd_hlist);
		break;
	}
	pthread_mutex_unlock(&hash_head->mutex);

	return CWLAN_OK;
}











u32 cw_ap_info_list_init()
{
	u32 size;
	u32 i;
	
	size = sizeof(cwlan_ap_hash_t)*CWLAN_AP_INFO_HASH_LEN;
	g_ap_info_hash = (cwlan_ap_hash_t*)malloc(size);
	if( NULL == g_ap_info_hash )
	{
		printf("cw init g_ap_info_hash fail\n");
		return CWLAN_FAIL;
	}
	memset(g_ap_info_hash, 0, size);
	for(i = 0; i < CWLAN_AP_INFO_HASH_LEN; i++)
	{
		INIT_LIST_HEAD(&g_ap_info_hash[i].h_head);
		pthread_mutex_init(&g_ap_info_hash[i].mutex,NULL);
	}

	return CWLAN_OK;
}
u32 cw_ap_info_list_exit()
{
	u32 i;
	/*删除所有hash结点数据*/
	cw_web_del_all_ap_info_list_node();
	for(i = 0; i < CWLAN_AP_INFO_HASH_LEN; i++)
	{
		/*注销锁*/
		pthread_mutex_destroy(&g_ap_info_hash[i].mutex);
	}
	/*释放hash链*/
	free(g_ap_info_hash);	
	printf("cw_ap_info_list_exit ok\n");

	return CWLAN_OK;
}

