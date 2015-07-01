#include <unistd.h> 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>


#include "cloud_wlan_list.h"
#include "cloud_wlan_types.h"
#include "cloud_wlan_nl.h"
#include "cloud_wlan_ac_com.h"
#include "cloud_wlan_recv_info_branch.h"
#include "cloud_wlan_ap_info_list.h"
#include "cloud_wlan_des.h"




/*字符串分割*/
typedef struct Split_struct
{
    char** strings;
    u32 len;
    char* internal_buf;
}str_split_s;

static void split_free_res(str_split_s *string_array)
{   
    if(NULL == string_array)
    {
        return;
    }
    if(string_array->internal_buf)
    {
        free(string_array->internal_buf);
    }
    if(string_array->strings)
    {
        free(string_array->strings);
    }
}

static void split_string(s8 delimiter, s8 *string, s32 limit, str_split_s * string_array)
{   
    u32 count = 1;
    s8 *pchar, **ptr;

	if ( NULL != string_array ) {
		memset(string_array, 0, sizeof(str_split_s));
	}
    
    if(NULL == string || NULL == string_array || string[0] == '\0')
    {
        return;
    }
    
    if (0 == limit)
    {
        // 全部分割
        limit = 99999;
    }
    
    string_array->internal_buf = strdup(string);
    if(NULL == string_array->internal_buf)
    {
        return;
    }
    
    pchar = string;
    while('\0' != *pchar && (s32)count < limit)
    {
        if (delimiter == *pchar)
        {
            count++;
        }
        pchar++;
    }
    string_array->strings = (s8**)malloc(count*sizeof(s8*));
    if(NULL == string_array->strings)
    {
        return;
    }
    string_array->len = count;
    
    ptr = string_array->strings;
    *ptr = string_array->internal_buf;
    pchar = string_array->internal_buf;
    while('\0' != *pchar && count > 1)
    {
        if (delimiter == *pchar)
        {
            ptr++;
            *ptr = pchar+1;
            *pchar = '\0';
            count--;
        }
        pchar++;
    }
}

u32 cw_db_cmd_white_parse(s8 *src, s8 *dest, u32 *res_len)
{
	u32 i, j;
	u32 buf_len = 0;
	ac_udp_white_list_t *white_list;
	str_split_s split_res;

	split_string('|', src, 0, &split_res);

	j = CLOUD_WLAN_WHITE_LIST_MAX_U > split_res.len?split_res.len:CLOUD_WLAN_WHITE_LIST_MAX_U;
	
	for(i=0; i< j; i++)
	{
		white_list = (ac_udp_white_list_t *)(dest + buf_len);
		white_list->id = i;
		white_list->len = strlen(split_res.strings[i]);
		strcpy((s8 *)white_list->data, split_res.strings[i]);
		buf_len += white_list->len + sizeof(u32) * 2;
	}
	*res_len = buf_len;

	split_free_res(&split_res);

	return CWLAN_OK;
}


u32 cw_db_cmd_portal_url_parse(s8 *src, s8 *dest, u32 *res_len)
{
	dns_protal_url_t *portal_url;

	portal_url = (dns_protal_url_t *)(dest);
	//加密
	portal_url->data_len = CW_DES_LEN;
	DES_Act(portal_url->data, src, strlen(src),g_des_key, DES_KEY_LEN, ENCRYPT);
	*res_len = CW_DES_LEN + sizeof(u32);

	return CWLAN_OK;
}

u32 cw_db_cmd_wan_pppoe_parse(s8 *src, s8 *dest, u32 *res_len)
{
	pppoe_cfg_t *pppoe_cfg;
	str_split_s split_res;

	pppoe_cfg = (pppoe_cfg_t *)dest;
	split_string('|', src, 0, &split_res);
		
	strcpy(pppoe_cfg->username, split_res.strings[0]);
	strcpy(pppoe_cfg->password, split_res.strings[1]);
	*res_len = sizeof(pppoe_cfg_t);
	split_free_res(&split_res);
	
	return CWLAN_OK;
}

u32 cw_db_cmd_wifi_info_parse(s8 *src, s8 *dest, u32 *res_len)
{
	wifi_cfg_t *wifi_cfg;
	str_split_s split_res;

	wifi_cfg = (wifi_cfg_t *)dest;
	split_string('|', src, 0, &split_res);

	wifi_cfg->wlan_id = atoi(split_res.strings[0]);
	wifi_cfg->disabled = atoi(split_res.strings[1]);
	wifi_cfg->txpower= atoi(split_res.strings[2]);
	wifi_cfg->channel= atoi(split_res.strings[3]);
	wifi_cfg->ssid_len= atoi(split_res.strings[4]);
	strcpy((s8 *)wifi_cfg->ssid,split_res.strings[5]);
	wifi_cfg->en_type= atoi(split_res.strings[6]);
	strcpy((s8 *)wifi_cfg->en_info.key,split_res.strings[7]);
	*res_len = sizeof(wifi_cfg_t);
	split_free_res(&split_res);
	
	return CWLAN_OK;
}


u32 cw_db_cmd_branch(u32 type, s8 *src, s8 *dest, u32 *res_len)
{

	switch(type)
	{
		case CW_NLMSG_SET_HEART_BEAT_INTERVAL:
			break;
		case CW_NLMSG_SET_OFF:
			break;
		case CW_NLMSG_SET_ON:
			break;
		case CW_NLMSG_SET_KLOG_OFF:
			break;
		case CW_NLMSG_SET_KLOG_ON:
			break;
		case CW_NLMSG_UPDATE_WHITE_LIST:
			cw_db_cmd_white_parse(src, dest, res_len);
			break;
		case CW_NLMSG_UPDATE_PORTAL:
			cw_db_cmd_portal_url_parse(src, dest, res_len);
			break;
		case CW_NLMSG_SET_REBOOT:
			break;
		case CW_NLMSG_SET_WAN_PPPOE:
			cw_db_cmd_wan_pppoe_parse(src, dest, res_len);
			break;
		case CW_NLMSG_SET_WAN_DHCP:
			break;
		case CW_NLMSG_SET_WIFI_INFO:
			cw_db_cmd_wifi_info_parse(src, dest, res_len);
			break;
		default:
			break;
	}

	return CWLAN_OK;

}
