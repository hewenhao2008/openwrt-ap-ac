/*
	文件功能说明:
	所有的函数接收的数据都认为是正确的，只处理更新数据
	例，在云端删除一些数据，那么发到ap的是剩下的需要更新的所有数据
	在云端添加一些数据，那么发到ap 的是添加后的所有数据

	函数名，以cw_ap 开头的函数则表示数据源或是发送端为本地
				以cw_ac开头的函数则表示数据源或是发送端为云端
	
*/
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <linux/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <stdlib.h>  
#include <linux/netlink.h>  
#include <strings.h>  
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <sys/stat.h>
#include <stdint.h>
#include <netdb.h>
#include <linux/sockios.h>  

#define ETHTOOL_GLINK        0x0000000a /* Get link status (ethtool_value) */


#include "cloud_wlan_types.h"
#include "cloud_wlan_nl.h"
#include "cloud_wlan_sqlite.h"
#include "cloud_wlan_cfg_update.h"
#include "cloud_wlan_ap_com.h"
#include "cloud_wlan_des.h"


static ap_udp_socket_t g_ap_udp_sock={0};
com_cfg_t g_ap_com_cfg;

/*
struct hostent {
    char *h_name;
    char **h_aliases;
    short h_addrtype;
    short h_length;
    char **h_addr_list;
    #define h_addr h_addr_list[0]
};
*/
static s8 *g_klog_tpye_info[]=
{
	"klog url",
	"klog unknown"
};

#define SET_P_NULL(p) if((p) != NULL){*(p)='\0';}

struct pthread_id
{
	pthread_t heartheat_id;
	pthread_t local_id;
	pthread_t telnet_id;
	pthread_t local_nl_id;
};
struct pthread_id g_pthread;

struct ethtool_value
{
    u32    cmd;
    u32    data;
};

u32 get_interface_info(s8 *ifname, u32 *ip, s8 *mac)     
{   
    struct ifreq req;     
    struct sockaddr_in *host;    
    struct ethtool_value edata;
	s32 sockfd;
    if(-1 == (sockfd = socket(PF_INET, SOCK_STREAM, 0)))   
    {   
        perror( "socket" );     
        return -1;   
	}
  
    memset(&req, 0, sizeof(struct ifreq));     
    strcpy(req.ifr_ifrn.ifrn_name, ifname); 
    edata.cmd = ETHTOOL_GLINK;
again_get:
    edata.data = -1;
    req.ifr_data = (s8 *)&edata;
	ioctl(sockfd, SIOCETHTOOL, &req);
	if( edata.data != 1)
	{
		printf("interface %s is down again_get\n", ifname);  
		sleep(2);
		goto again_get;
	}
    ioctl(sockfd, SIOCGIFADDR, &req);
    host = (struct sockaddr_in*)&req.ifr_ifru.ifru_addr;    
	(*ip) = host->sin_addr.s_addr;
	if(mac != NULL)
	{
		ioctl(sockfd, SIOCGIFHWADDR, &req);
		memcpy(mac, req.ifr_ifru.ifru_hwaddr.sa_data, 6);
	}
    close(sockfd);     

    return 0;
} 

s8* get_Second_Level_Domain(s8 *dest)
{
	s8 * p = NULL;
	s8 * e = NULL;
	p = strchr(dest, ':');
	if(p == NULL)
	{
	/*url?? 1
	www.chinanews.com/gj/2014/04-16/6069354.shtml
	*/
		e = strchr(dest, '/');
		SET_P_NULL(e);
		return dest;
	}
	
	if(strncmp(p, "://", 3) != 0)
	{
	/*url?? 2
	www.chinanews.com:8080/gj/2014/04-16/6069354.shtml
	*/
		SET_P_NULL(p);
		return dest;
	}

	p = p+3;
	e = strchr(p, ':');
	if(e == NULL)
	{
	/*url?? 3
	http://www.chinanews.com/gj/2014/04-16/6069354.shtml
	*/
		e = strchr(p, 47);
		SET_P_NULL(e);
		return p;
	}
	else
	{
	/*url?? 4
	http://www.chinanews.com:8080/gj/2014/04-16/6069354.shtml
	*/
		SET_P_NULL(e);
		return p;
	}

}


s32 cw_get_url_dns(s8 *url, dns_white_list_t *dns_tmp)
{
	s32               i = 0;
	s8 t[128];
	s8 *temp;
	struct hostent *host = NULL;
	
	if(url == NULL || dns_tmp == NULL)
	{
	    return CWLAN_FAIL;
	}

	strcpy(t, url);
	temp = get_Second_Level_Domain(t);

	host = gethostbyname2(temp, AF_INET);
	if (host == NULL)
	{
	/*这里nslookup失败的时候可以直接删除d b中的表项*/
	    printf("gethostbyname err: %s\n",url);
	    return CWLAN_FAIL;
	}

	for (i=0; host->h_addr_list[i]!= NULL; i++)
	{
		if(dns_tmp->number >= CW_WHITE_LIST_MAX)
		{
			continue;
		}
		dns_tmp->list[dns_tmp->number]= *(u32 *)host->h_addr_list[i];

	    printf("ip: %x \n",*(u32 *)host->h_addr_list[i]);
		dns_tmp->number++;
	}

	return CWLAN_OK;
}

s32 cw_ap_recv_ac_update_url_wl(dcma_udp_skb_info_t *buff)
{
	ac_udp_white_list_t *node = (ac_udp_white_list_t *)buff->data;
	dns_white_list_t dns_white_list = {0,{0}};

	s8 *url;
	u32 i;
	u32 ip;
	u32 maxnumber = buff->number<CLOUD_WLAN_WHITE_LIST_MAX_U?buff->number:CLOUD_WLAN_WHITE_LIST_MAX_U;

	/*需要删除数据库中所有重定向表中的数据*/
	snprintf(g_cw_sql, G_CW_SQL_BUF_LEN, "delete from %s;",CWLAN_AP_WHITE_TABLE);
	sqlite3_exec_unres(g_cw_db, g_cw_sql);

	memset(&dns_white_list, 0, sizeof(dns_white_list));
	
	get_interface_info("br-lan", &ip, NULL);
	dns_white_list.list[dns_white_list.number++] = ip;
	
	for(i=0; i<maxnumber; i++)
	{
		url = (s8 *)node->data;
		if(url != NULL)
		{
			/*更新数据库中所有重定向表中的数据*/
			snprintf(g_cw_sql, G_CW_SQL_BUF_LEN, 
				"INSERT INTO %s (url) VALUES('%s');",
				CWLAN_AP_WHITE_TABLE, url);
			sqlite3_exec_unres(g_cw_db, g_cw_sql);
			
			cw_get_url_dns(url, &dns_white_list);
		}
		node = (ac_udp_white_list_t *)(url + node->len);
	}
	
	//cloud_wlan_sendto_kmod(CW_NLMSG_UPDATE_WHITE_LIST, (s8 *)&dns_white_list, sizeof(dns_white_list));
    return CWLAN_OK;
}

s32 cw_ap_local_update_url_wl(dns_white_list_t *dns_white_list)
{
	u32 ret;
	u32 i;
	s8 *url = NULL;
	sqlite3_res res;

	snprintf(g_cw_sql, G_CW_SQL_BUF_LEN, "select * from %s;", CWLAN_AP_WHITE_TABLE);
	ret = sqlite3_exec_get_res(g_cw_db, g_cw_sql, &res);
	if(ret != CWLAN_OK)
	{
		printf("sqlite3_exec_get_res[%s]: %s\n",g_cw_sql, sqlite3_errmsg(g_cw_db));  
		return CWLAN_FAIL;
	}
	for(i=1; i<=res.nrow; i++)
	{
		sqlite3_get_s8(&res, i, "url", &url);
		if(url != NULL)
		{
			printf("INIT:cwlan white_list url: %s\n",url);
			cw_get_url_dns(url, dns_white_list);
		}
	}
	sqlite3_exec_free_res(&res);
	
	//cloud_wlan_sendto_kmod(CW_NLMSG_UPDATE_WHITE_LIST, (s8 *)dns_white_list, sizeof(dns_white_list_t));
	return CWLAN_OK;
}

u32 cw_ap_recv_ac_set_default_switch(u32 ap_cwlan_sw)
{
	/*
		更新数据库中默认开关的数据
	*/

	snprintf(g_cw_sql, G_CW_SQL_BUF_LEN, "update com_cfg set ap_cwlan_sw=%d;",ap_cwlan_sw);
	sqlite3_exec_unres(g_cw_db, g_cw_sql);
	//cloud_wlan_sendto_kmod(ap_cwlan_sw, NULL, 0);
	return CWLAN_OK;
}
u32 cw_ap_recv_ac_set_klog_switch(u32 ap_klog_sw)
{
	/*
		更新数据库中默认开关的数据
	*/

	snprintf(g_cw_sql, G_CW_SQL_BUF_LEN, "update com_cfg set ap_klog_sw=%d;",ap_klog_sw);
	sqlite3_exec_unres(g_cw_db, g_cw_sql);
	//cloud_wlan_sendto_kmod(ap_klog_sw, NULL, 0);
	return CWLAN_OK;
}


s32 cw_ap_recv_ac_update_portal_wl(dcma_udp_skb_info_t *buff)
{
	s32 ret;
	dns_white_list_t *portal_white_list;
	reHttp_t *portal_cfg;
	s8 out[CW_DES_LEN]={0};
	dns_protal_url_t *portal_url = (dns_protal_url_t *)buff->data;

	snprintf(g_cw_sql, G_CW_SQL_BUF_LEN, "UPDATE %s SET url=?;",CWLAN_AP_PORTAL_TABLE);
	sqlite3_binary_write1(g_cw_db,g_cw_sql, (s8 *)buff->data, CW_DES_LEN);

	portal_cfg = malloc(sizeof(reHttp_t));
	portal_white_list = malloc(sizeof(dns_white_list_t));

	if(portal_white_list == NULL || portal_cfg == NULL)
	{
		CW_DEBUG_U("cw_ap_local_update_portal_wl fail\n");
		return CWLAN_FAIL;
	}
	
	memset((u8 *)portal_cfg, 0, sizeof(reHttp_t));
	memset((u8 *)portal_white_list, 0, sizeof(dns_white_list_t));


	DES_Act(out, (s8 *)portal_url->data, portal_url->data_len,g_des_key, DES_KEY_LEN, DECRYPT);

	ret = cw_get_url_dns(out, portal_white_list);
	if(ret == CWLAN_FAIL)
	{
		CW_DEBUG_U("cw_get_url_dns fail\n");
		goto local_exit;
	}
	memcpy((u8 *)portal_cfg->destIp, (u8 *)portal_white_list->list, sizeof(u32)*( CW_LOCATION_URL_IP_MAX-1));
	portal_cfg->destPort = CW_LOCATION_PORT;
	memcpy(portal_cfg->Location, out, CW_LOCATION_URL_DATA_LEN);
//	cloud_wlan_sendto_kmod(CW_NLMSG_UPDATE_PORTAL, (s8 *)portal_cfg, sizeof(reHttp_t));

local_exit:
	free(portal_white_list);
	free(portal_cfg);
	return CWLAN_OK;
}

s32 cw_ap_local_update_portal_wl(void)
{
	s32 ret;
	dns_white_list_t *portal_white_list;
	reHttp_t *portal_cfg;
	s8 *decryt;
	s8 out[CW_DES_LEN]={0};
	u32 data_len;

	portal_cfg = malloc(sizeof(reHttp_t));
	portal_white_list = malloc(sizeof(dns_white_list_t));

	if(portal_white_list == NULL || portal_cfg == NULL)
	{
		printf("cw_ap_local_update_portal_wl fail\n");
		return CWLAN_FAIL;
	}
	
	memset((u8 *)portal_cfg, 0, sizeof(reHttp_t));
	memset((u8 *)portal_white_list, 0, sizeof(dns_white_list_t));

	
	snprintf(g_cw_sql, G_CW_SQL_BUF_LEN, "select url from %s;", CWLAN_AP_PORTAL_TABLE);
	
	data_len = sqlite3_binary_read(g_cw_db, g_cw_sql, &decryt, 0);
	DES_Act(out, decryt, data_len,g_des_key, DES_KEY_LEN, DECRYPT);

	ret = cw_get_url_dns(out, portal_white_list);
	if(ret == CWLAN_FAIL)
	{
		printf("cw_get_url_dns fail\n");
		goto local_exit;
	}

	memcpy((u8 *)portal_cfg->destIp, (u8 *)portal_white_list->list, sizeof(u32)*( CW_LOCATION_URL_IP_MAX-1));
	portal_cfg->destPort = CW_LOCATION_PORT;
	memcpy(portal_cfg->Location, out, CW_LOCATION_URL_DATA_LEN);
	//cloud_wlan_sendto_kmod(CW_NLMSG_UPDATE_PORTAL, (s8 *)portal_cfg, sizeof(reHttp_t));
	
local_exit:
	free(portal_white_list);
	free(portal_cfg);
	return CWLAN_OK;
}
u32 cw_ap_recv_ac_set_reboot()
{
	system("reboot");
	return CWLAN_OK;
}
/*以下调用的uci需要luci的支持
以后最好是不要使用uci或是太依赖，考虑在移植问题
*/
u32 cw_ap_recv_ac_set_wan_pppoe(pppoe_cfg_t *pppoe)
{
	s8 cmd[256];
	
	snprintf(cmd, 256, "uci set network.wan.proto=pppoe; uci set network.wan.username=%s;uci set network.wan.password=%s",
		pppoe->username, pppoe->password);
	system(cmd);
	system("uci commit; /etc/init.d/network restart");

	return CWLAN_OK;
}
u32 cw_ap_recv_ac_set_wan_dhcp()
{
	system("uci set network.wan.proto=dhcp; uci commit; /etc/init.d/network restart");
	return CWLAN_OK;
}
u32 cw_ap_recv_ac_set_wifi_info(wifi_cfg_t *wifi_cfg)
{
	s8 cmd[256];
	s8 ssid[128]={0};
	
	snprintf(cmd, 256, "uci set wireless.@wifi-device[%d].disabled=%d",
		wifi_cfg->wlan_id, wifi_cfg->disabled);
	system(cmd);
	snprintf(cmd, 256, "uci set wireless.@wifi-device[%d].txpower=%d",
		wifi_cfg->wlan_id, wifi_cfg->txpower);
	system(cmd);
	snprintf(cmd, 256, "uci set wireless.@wifi-device[%d].channel=%d",
		wifi_cfg->wlan_id, wifi_cfg->channel);
	system(cmd);

	if(wifi_cfg->ssid_len !=0 )
	{
		memcpy(ssid, wifi_cfg->ssid, wifi_cfg->ssid_len>128?128:wifi_cfg->ssid_len);
		snprintf(cmd, 256, "uci set wireless.@wifi-iface[%d].ssid=%s",
			wifi_cfg->wlan_id, ssid);
		system(cmd);
	}
	if(wifi_cfg->en_type == EN_NONE)
	{
		snprintf(cmd, 256, "uci set wireless.@wifi-iface[%d].encryption=none",
			wifi_cfg->wlan_id);
		system(cmd);

	}
	snprintf(cmd, 256, "uci set wireless.@wifi-iface[%d].wds=1",
		wifi_cfg->wlan_id);
	system(cmd);//无线漫游
	
	system("uci commit; /etc/init.d/network restart");

	return CWLAN_OK;

/*
	uci set wireless.@wifi-device[0].disabled=0    //打开无线
	uci set wireless.@wifi-device[0].txpower=17    //设置功率为17dbm 太高会烧无线模块
	uci set wireless.@wifi-device[0].channel=6	  //设置无线信道为6
	uci set wireless.@wifi-iface[0].mode=ap    //设置无线模式为ap
	uci set wireless.@wifi-iface[0].ssid=[自己设置SSID]    //设置无线SSID
	uci set wireless.@wifi-iface[0].network=lan    //无线链接到lan上
	uci set wireless.@wifi-iface[0].encryption=psk2    //设置加密为WPA2-PSK
	uci set wireless.@wifi-iface[0].key=[密码]	  //设置无线密码
*/
}


u32 get_mem_info (ap_local_info_t *ap_info)
{
	FILE *fd;          
	char buff[256];   
	char tmp[30];

	//system("cat /proc/meminfo | sed -n '1,2p' > /tmp/apinfo.txt"); 

	fd = fopen ("/proc/meminfo", "r"); 

	fgets (buff, sizeof(buff), fd); 
	sscanf (buff, "%s %u", tmp, &ap_info->mem_total); 
	fgets (buff, sizeof(buff), fd); 
	sscanf (buff, "%s %u", tmp, &ap_info->mem_free); 
	//printf ("%u %u\n", tmp, ap_info->mem_total, ap_info->mem_free); 

	fclose(fd);
	return CWLAN_OK;
}


u32 get_run_time(ap_local_info_t *ap_info)
{
	FILE *fd;          
	char buff[256];   

	fd = fopen ("/proc/uptime", "r"); 

	fgets(buff, sizeof(buff), fd); 
	sscanf(buff, "%llu", &ap_info->run_time); 
	//printf ("%u %u\n", tmp, ap_info->mem_total, ap_info->mem_free); 

	fclose(fd);
	return CWLAN_OK;
}

u32 cal_cpu_rate (cpu_info_t *o, cpu_info_t *n) 
{   
	u64 one, two;    
	u64 idle;
	u64 totalcputime;
	u32 cpu_use = 0;   

	one = (u64) (o->user + o->nice + o->system +o->idle+o->iowait+o->irp+ o->softirp+ o->stealstolen+ o->guest);//???(??+???+??+??)??????od
	two = (u64) (n->user + n->nice + n->system +n->idle+n->iowait+n->irp+ n->softirp+ n->stealstolen+ n->guest);//???(??+???+??+??)??????od
	  
	idle = (u64) (n->idle - o->idle); 

	totalcputime = two-one;
	if(totalcputime != 0)
		cpu_use = idle*100/totalcputime;
	else
		cpu_use = 0;
	return cpu_use;
}

u32 get_cpu_info (cpu_info_t *cpust)
{   
	FILE *fd;         
	char buff[256]; 
	char tmp[30];
	cpu_info_t *cpu_occupt;
	cpu_occupt=cpust;
	         
	//system("cat /proc/stat | sed -n '1,1p' > /tmp/apinfo.txt;cat /proc/stat | sed -n '1,1p' >> /tmp/apinfo.txt"); 

	fd = fopen ("/proc/stat", "r"); 
	fgets (buff, sizeof(buff), fd);

	sscanf (buff, "%s %u %u %u %u %u %u %u %u %u",
		tmp, &cpu_occupt->user, &cpu_occupt->nice,&cpu_occupt->system, &cpu_occupt->idle, &cpu_occupt->iowait,
		&cpu_occupt->irp, &cpu_occupt->softirp, &cpu_occupt->stealstolen, &cpu_occupt->guest );
	/*
	printf ("%s %u %u %u %u %u %u %u %u %u\n",
		tmp, cpu_occupt->user, cpu_occupt->nice,cpu_occupt->system, cpu_occupt->idle, cpu_occupt->iowait,
		cpu_occupt->irp, cpu_occupt->softirp, cpu_occupt->stealstolen, cpu_occupt->guest );
	*/

	fclose(fd);
	return CWLAN_OK;
}

u32 cw_get_ap_local_info(ap_local_info_t *ap_info)
{
    cpu_info_t cpu_stat1;
    cpu_info_t cpu_stat2;
    
	get_mem_info(ap_info);
	get_run_time(ap_info);

	get_cpu_info((cpu_info_t *)&cpu_stat1);
	sleep(1);
	get_cpu_info((cpu_info_t *)&cpu_stat2);
	ap_info->cpu_idle_rate = cal_cpu_rate ((cpu_info_t *)&cpu_stat1, (cpu_info_t *)&cpu_stat2);
	return CWLAN_OK;
}

void *cw_ap_sendto_ac_heartbeat(void *param)
{
	u32 sendsize=0;
	u32 msg_len = 0;
	s8 heartbeat_msg[MAX_PAYLOAD]={0};
	ap_local_info_t ap_info;	
	dcma_udp_skb_info_t buff;


	/* 向服务器端发送数据信息 */
	memcpy(ap_info.apmac, g_ap_udp_sock.client_mac, 6);
	msg_len = sizeof(ap_local_info_t) + sizeof(dcma_udp_skb_info_t);
	buff.type = CW_NLMSG_HEART_BEAT;
	buff.number = 1;
	//memcpy(heartbeat_msg, (s8 *)&ap_info, sizeof(ap_local_info_t));
	 /* 向服务器发送数据信息 */
	memcpy(heartbeat_msg, (s8 *)&buff, sizeof(u32)*2);
	while(1)
	{
		cw_get_ap_local_info(&ap_info);
		sendsize = 0;
		 /* 向服务器发送数据信息 */
		memcpy(heartbeat_msg+sizeof(u32)*2, (s8 *)&ap_info, sizeof(ap_local_info_t));
		//heartbeat_msg = (s8 *)&ap_info;
		while( sendsize < msg_len )
		{
			/* 每次发送后数据指针向后移动 */
			sendsize = sendsize + sendto(g_ap_udp_sock.sockfd, heartbeat_msg+sendsize, msg_len, 0, 
											(struct sockaddr *)&g_ap_udp_sock.server_addr, 
											sizeof(struct sockaddr));
		};

		sleep(3);
	}
	return NULL;
}

u32 cw_ap_sendto_ac_heartbeat_init()
{
	u32 ret;
	
	/* 远端配置动态心跳线程*/
	ret = pthread_create(&g_pthread.heartheat_id,NULL,cw_ap_sendto_ac_heartbeat,NULL);
	if(ret!=0){
		printf("init cw_ap_sendto_ac_heartbeat_init pthread_create fail %d!\n", ret);
	}

	return ret;
}
/*
	接收 云ac 的配置更新功能
*/
static void cw_recv_ac_info_branch(dcma_udp_skb_info_t *buff)  
{  
printf("%d\n",buff->type);
	switch(buff->type)  
	{  
		case CW_NLMSG_HEART_BEAT:
			//printf("ok %d\n" , CW_NLMSG_HEART_BEAT);
			break;
		case CW_NLMSG_SET_DEBUG_OFF:
			printf("cw_recv_ac debuf off\n");
			break;
		case CW_NLMSG_SET_DEBUG_ON:
			printf("cw_recv_ac debuf on %d\n",buff->type);
			break;
		case CW_NLMSG_SET_OFF:
			cw_ap_recv_ac_set_default_switch(buff->type);
			printf("cw_recv_ac set off\n");
			break;
		case CW_NLMSG_SET_ON:
			cw_ap_recv_ac_set_default_switch(buff->type);
			printf("cw_recv_ac set on\n");
			break;
		case CW_NLMSG_SET_KLOG_OFF:
			cw_ap_recv_ac_set_klog_switch(buff->type);
			printf("cw_recv_ac set klog off\n");
			break;
		case CW_NLMSG_SET_KLOG_ON:
			cw_ap_recv_ac_set_klog_switch(buff->type);
			printf("cw_recv_ac set klog on\n");
			break;
		case CW_NLMSG_UPDATE_WHITE_LIST:
			cw_ap_recv_ac_update_url_wl(buff);
			printf("cw_ap_recv_ac_update_url_wl\n");
			break;
		case CW_NLMSG_UPDATE_PORTAL:
			cw_ap_recv_ac_update_portal_wl(buff);
			printf("cw_ap_recv_ac_update_portal_wl\n");
			break;
		case CW_NLMSG_SET_REBOOT:
			cw_ap_recv_ac_set_reboot();
			printf("cw_ap_recv_ac_set_reboot\n");
			break;
		case CW_NLMSG_SET_WAN_PPPOE:
			cw_ap_recv_ac_set_wan_pppoe((pppoe_cfg_t *)buff->data);
			printf("cw_ap_recv_ac_set_wan_pppoe\n");
			break;
		case CW_NLMSG_SET_WAN_DHCP:
			cw_ap_recv_ac_set_wan_dhcp();
			printf("cw_ap_recv_ac_set_wan_dhcp\n");
			break;
		case CW_NLMSG_SET_WIFI_INFO:
			cw_ap_recv_ac_set_wifi_info((wifi_cfg_t *)buff->data);
			printf("cw_ap_recv_ac_set_wifi_info\n");
			break;

        default:  
			break;
    }  
    return;  
}  

void *cw_ap_recv_ac_cfg_dynamic_update(void *param)
{
    socklen_t sin_size = sizeof(struct sockaddr_in);
	int recv_st=0;
	dcma_udp_skb_info_t *buf = NULL;

	buf = malloc(MAX_PAYLOAD);
	if(buf == NULL)
	{
		printf("init cw_ac_cfg_dynamic_update fail!\n");
		return NULL;
	}
	
	while(1)
	{
		
		/* 接收服务器端应答数据信息 */
		recv_st = recvfrom(g_ap_udp_sock.sockfd, buf, MAX_PAYLOAD, 0, (struct sockaddr *)&g_ap_udp_sock.server_addr, &sin_size);
		if(-1 == recv_st)
		{
			printf("recvfrom ac :%d %s\n", errno, strerror(errno));
			//goto err_out;
			continue;
		}


		cw_recv_ac_info_branch(buf);		
	}
	


    /* 关闭套接字描述符 */
	close(g_ap_udp_sock.sockfd);
    printf("Client:sockfd closed!\n");
	return NULL;
}

u32 cw_ap_recv_ac_cfg_dynamic_update_init()
{
	u32 ret;
	
	/* 远端配置动态更新线程*/
	ret = pthread_create(&g_pthread.telnet_id,NULL,cw_ap_recv_ac_cfg_dynamic_update,NULL);
	if(ret!=0){
		printf("init telnet config update pthread_create fail %d!\n",ret);
	}

	return ret;
}


void *cw_ap_local_cfg_dynamic_update(void *param)
{
	time_t cw_network = 0;
	u32 ip = 0;
	s8 cmd[128]={0};
	s8 logname[64]={0};
	struct in_addr tmp_ip;
	//sqlite3_res res;
	dns_white_list_t dns_white_list = {0,{0}};
	struct stat filestat;
	while(1)
	{
/*这个需要系统功能支持*/
		stat(DNS_CONFIG_FILE, &filestat);
		if(cw_network < filestat.st_ctime)
		{
			cw_network = filestat.st_ctime;
			memset(&dns_white_list, 0, sizeof(dns_white_list));
/*优先更新本地，要不管理web页面都上不去*/
			get_interface_info("br-lan", &ip, NULL);
			dns_white_list.list[dns_white_list.number++] = ip;
			//cloud_wlan_sendto_kmod(CW_NLMSG_UPDATE_WHITE_LIST, (s8 *)&dns_white_list, sizeof(dns_white_list_t));
			
			while(1)
			{
				get_interface_info(g_ap_com_cfg.ap_com_eth, &ip, NULL);
				if(ip != 0)
				{
					break;
				}
			}

			/*更新portal的url*/
			cw_ap_local_update_portal_wl();

			cw_ap_local_update_url_wl(&dns_white_list);

		}
		stat(CW_AP_LOG_FILE, &filestat);
		if( filestat.st_size> CW_AP_LOG_MAX)
		{
			system(">cw_ap.log");
		}
		 
		stat(CW_AP_KLOG_FILE, &filestat);
		if( filestat.st_size> CW_AP_KLOG_MAX)
		{

			snprintf(logname, 64, "%02x%02x%02x%02x%02x%02x.%s.%d", 
				(u8)g_ap_udp_sock.client_mac[0],(u8)g_ap_udp_sock.client_mac[1],
				(u8)g_ap_udp_sock.client_mac[2],(u8)g_ap_udp_sock.client_mac[3],
				(u8)g_ap_udp_sock.client_mac[4],(u8)g_ap_udp_sock.client_mac[5],
				CW_AP_KLOG_FILE,(u32)filestat.st_mtime
				);
			
			printf("Turn on the log: %s\n",logname);
			
			tmp_ip.s_addr = g_ap_com_cfg.ac_com_addr;
			snprintf(cmd, 128, "mv %s %s; tftp -p -l %s %s; rm %s",
				CW_AP_KLOG_FILE, logname, logname, inet_ntoa(tmp_ip), logname);
			system(cmd);
		}
		/*
		stat(CLIENTS_FILE_PATH, &filestat);
		if(clients < filestat.st_ctime)
		{
			clients = filestat.st_ctime;
			printf("call clients config update\n");
		}
		*/
		
		sleep(3);
	};
}
u32 cw_ap_local_cfg_dynamic_update_init()
{
	u32 ret;
	ret = pthread_create(&g_pthread.local_id,NULL,cw_ap_local_cfg_dynamic_update,NULL);
	if(ret!=0)
	{
		printf("init cw_ap_recv_ac_cfg_dynamic_update pthread_create fail %d!\n", ret);
	}

	return ret;
}
u32 cw_ap_sendto_ac_online_info(dcma_udp_skb_info_t *buff)
{
	u32 sendsize=0;
	u32 msg_len = 0;
	s8 heartbeat_msg[MAX_PAYLOAD]={0};
	online_user_info_t *ap_user_info = (online_user_info_t *)buff->data;


	/* 向服务器端发送数据信息 */
	msg_len = sizeof(online_user_info_t) + sizeof(dcma_udp_skb_info_t);
	
	time((time_t *)&ap_user_info->time);
	sendsize = 0;
	buff->number = 1;
	//memcpy(heartbeat_msg, (s8 *)&ap_info, sizeof(ap_local_info_t));
	 /* 向服务器发送数据信息 */
	memcpy(ap_user_info->apmac, g_ap_udp_sock.client_mac, 6);
	memcpy(heartbeat_msg, (s8 *)buff, sizeof(u32)*2);
	memcpy(heartbeat_msg+sizeof(u32)*2, (s8 *)ap_user_info, sizeof(online_user_info_t));
	while( sendsize < msg_len )
	{
		/* 每次发送后数据指针向后移动 */
		sendsize = sendsize + sendto(g_ap_udp_sock.sockfd, &heartbeat_msg[sendsize], msg_len, 0, 
										(struct sockaddr *)&g_ap_udp_sock.server_addr, 
										sizeof(struct sockaddr));
	};

	return CWLAN_OK;
}
u32 cw_kmod_log_writeto_server(dcma_udp_skb_info_t *buff)
{
	u32 sendsize=0;
	u32 msg_len = 0;
	s8 heartbeat_msg[MAX_PAYLOAD]={0};
	kmod_log_info_t * ap_kmod_log_info = (kmod_log_info_t *)buff->data;


	/* 向服务器端发送数据信息 */
	msg_len = ap_kmod_log_info->size + sizeof(u32)*2;
	
	time((time_t *)&ap_kmod_log_info->time);
	sendsize = 0;
	
	buff->number = 1;
	//memcpy(heartbeat_msg, (s8 *)&ap_info, sizeof(ap_local_info_t));
	 /* 向服务器发送数据信息 */
	
	memcpy(ap_kmod_log_info->apmac, g_ap_udp_sock.client_mac, 6);
	memcpy(heartbeat_msg, (s8 *)buff, sizeof(u32)*2);
	memcpy(heartbeat_msg+sizeof(u32)*2, (s8 *)ap_kmod_log_info, ap_kmod_log_info->size);
	while( sendsize < msg_len )
	{
		/* 每次发送后数据指针向后移动 */
		sendsize = sendsize + sendto(g_ap_udp_sock.sockfd, &heartbeat_msg[sendsize], msg_len, 0, 
										(struct sockaddr *)&g_ap_udp_sock.server_addr, 
										sizeof(struct sockaddr));
	};
	return CWLAN_OK;

}
u32 cw_kmod_log_writeto_file(dcma_udp_skb_info_t *buff)
{
	kmod_log_info_t *kmod_log = (kmod_log_info_t *)buff->data;
	FILE *tfp;
	tfp = fopen(CW_AP_KLOG_FILE, "a");
	if ( tfp != NULL)
	{
		fprintf(tfp, "%d, %s, %2x:%2x:%2x:%2x:%2x:%2x, %2x:%2x:%2x:%2x:%2x:%2x, %x, %s, %s\n", 
				kmod_log->type, g_klog_tpye_info[kmod_log->type],
				g_ap_com_cfg.ap_com_eth[0],g_ap_com_cfg.ap_com_eth[1],g_ap_com_cfg.ap_com_eth[2],
				g_ap_com_cfg.ap_com_eth[3],g_ap_com_cfg.ap_com_eth[4],g_ap_com_cfg.ap_com_eth[5],
				kmod_log->usermac[0],kmod_log->usermac[1],kmod_log->usermac[2],
				kmod_log->usermac[3],kmod_log->usermac[4],kmod_log->usermac[5],
				kmod_log->userip, 
				ctime((time_t *)&kmod_log->time),
				kmod_log->data
			);
		fclose(tfp);
	}
	return CWLAN_OK;
}

u32 cw_ap_sendto_ac_kmod_log_info(dcma_udp_skb_info_t *buff)
{
	if(g_ap_com_cfg.ap_klog_mode ==REAL_TIME)
	{
		cw_kmod_log_writeto_server(buff);
	}
	else
	{
		cw_kmod_log_writeto_file(buff);
	}
	return CWLAN_OK;
}

/*
	接收 kmod上报文的数据
*/
static void cw_recv_kmod_info_branch(dcma_udp_skb_info_t *buff)  
{  
	switch(buff->type)  
	{
		case CW_NLMSG_RES_OK:
			break;
		case CW_NLMSG_PUT_ONLINE_INFO_TO_AC:  
			cw_ap_sendto_ac_online_info(buff);
			break; 
		case CW_NLMSG_PUT_KLOG_INFO: 
			cw_ap_sendto_ac_kmod_log_info(buff);
			break;
        default:  
			break;
    }  
    return;  
}  

extern s32 cloud_wlan_nl_recv_kmod(dcma_udp_skb_info_t *buff);
void *cw_ap_recv_kmod_info(void *param)
{
	//int ret=0;
	dcma_udp_skb_info_t *buf;
	online_user_info_t *tt;

	buf = malloc(MAX_PAYLOAD);
	if(buf == NULL)
	{
		printf("init cw_ac_cfg_dynamic_update fail!\n");
		return NULL;
	}
	
	tt = (online_user_info_t *)buf->data;
	
	tt->userip=0x02020202;
	tt->usermac[0]=0x11;
	tt->usermac[1]=0x22;
	tt->usermac[2]=0x33;
	tt->usermac[3]=0x44;
	tt->usermac[4]=0x55;
	tt->usermac[5]=0x66;
	tt->apmac[0]=0x00;
	tt->apmac[1]=0x0c;
	tt->apmac[2]=0x29;
	tt->apmac[3]=0x11;
	tt->apmac[4]=0xfd;
	tt->apmac[5]=0x56;
	tt->status=0;
	tt->time= 11111;

	buf->type=CW_NLMSG_PUT_ONLINE_INFO_TO_AC;
	buf->number=1;
	
	while(1)
	{
		
		/* 接收kernel nl数据信息 */
		//ret = cloud_wlan_nl_recv_kmod(&buf);

		tt->status=tt->status%CW_FS_MAX;
		printf("ap user status %d\n", tt->status);
		cw_recv_kmod_info_branch(buf);
		tt->status++;
		sleep(10);
	}
	
	return NULL;
}

u32 cw_ap_recv_kmod_info_init()
{
	u32 ret;
	
	/* 本地接收内核数据线程*/
	ret = pthread_create(&g_pthread.local_nl_id,NULL,cw_ap_recv_kmod_info,NULL);
	if(ret!=0){
		printf("init local cw_ap_recv_kmod_info pthread_create fail %d!\n",ret);
	}

	return ret;
}

u32 cw_ap_local_db_com_cfg_init(sqlite3 * db)
{
	sqlite3_res res;
	s8 *buf=NULL;
	u32 ret;
	cwlan_flow_session_cfg_t temp_session_cfg;
	
	
	
	
	return CWLAN_OK;

		
		
  	snprintf(g_cw_sql, G_CW_SQL_BUF_LEN, "select * from %s;", CWLAN_AP_CFG_TABLE);
	ret = sqlite3_exec_get_res(db, g_cw_sql, &res);
	if(ret != CWLAN_OK)
	{
		printf("INIT:sqlite3_exec_get_res[%s]: %s\n",g_cw_sql, sqlite3_errmsg(g_cw_db));  
		exit(1);
	}
	sqlite3_get_s8(&res, 1, "ac_com_addr", &buf);
	g_ap_com_cfg.ac_com_addr=inet_addr(buf);
	printf("INIT:cwlan ac ip          %x\n", g_ap_com_cfg.ac_com_addr);

	sqlite3_get_u32(&res, 1, "ac_com_port", &g_ap_com_cfg.ac_com_port);
	printf("INIT:cwlan ac port         %d\n", g_ap_com_cfg.ac_com_port);
	sqlite3_get_s8(&res, 1, "ap_com_eth", &buf);
	strcpy(g_ap_com_cfg.ap_com_eth,buf);
	printf("INIT:cwlan ac eth          %s\n", g_ap_com_cfg.ap_com_eth);
	sqlite3_get_u32(&res, 1, "ap_cwlan_sw", &g_ap_com_cfg.ap_cwlan_sw);
	printf("INIT:cwlan ap_cwlan_sw     %d\n", g_ap_com_cfg.ap_cwlan_sw);
	sqlite3_get_u32(&res, 1, "ap_klog_sw", &g_ap_com_cfg.ap_klog_sw);
	printf("INIT:cwlan ap_klog_sw      %d\n", g_ap_com_cfg.ap_klog_sw);
	sqlite3_get_u32(&res, 1, "ap_klog_mode", &g_ap_com_cfg.ap_klog_mode);
	printf("INIT:cwlan ap_klog_mode    %d :0 is REAL_TIME; no 0 is unreal_time\n", g_ap_com_cfg.ap_klog_mode);

	sqlite3_get_u32(&res, 1, "over_time", &temp_session_cfg.over_time);
	printf("INIT:cwlan over_time       %d\n", temp_session_cfg.over_time);
	sqlite3_get_u32(&res, 1, "interval_timer", &temp_session_cfg.interval_timer);
	printf("INIT:cwlan interval_timer  %d\n", temp_session_cfg.interval_timer);
	sqlite3_get_u32(&res, 1, "del_time", &temp_session_cfg.del_time);
	printf("INIT:cwlan del_time        %d\n", temp_session_cfg.del_time);


	sqlite3_exec_free_res(&res);



	
	//cloud_wlan_sendto_kmod(CW_NLMSG_UPDATE_SESSION_CFG, (s8 *)&temp_session_cfg, sizeof(temp_session_cfg));

	return CWLAN_OK;
}

u32 cw_cfg_dynamic_update_init()
{
	s32 ret = CWLAN_OK;
	/* 获取套接字描述符 */
	g_ap_udp_sock.sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (-1 == g_ap_udp_sock.sockfd)
	{
		printf("socket: %d %s\n", errno, strerror(errno));
		return CWLAN_FAIL;
	}

    /* 设置服务器端基本配置信息 */
	g_ap_udp_sock.server_addr.sin_family =AF_INET;
	g_ap_udp_sock.server_addr.sin_port = htons(g_ap_com_cfg.ac_com_port);
	g_ap_udp_sock.server_addr.sin_addr.s_addr = htonl(g_ap_com_cfg.ac_com_addr);
	bzero(&(g_ap_udp_sock.server_addr.sin_zero), 8);

	/*获得本地出口地址和端口*/
	get_interface_info(g_ap_com_cfg.ap_com_eth, &g_ap_udp_sock.client_ip, g_ap_udp_sock.client_mac);
	g_ap_udp_sock.client_port = 0;

    /* 设置客户机端基本配置信息 */
    g_ap_udp_sock.client_addr.sin_family =AF_INET;
	g_ap_udp_sock.client_addr.sin_port = htons(g_ap_udp_sock.client_port);
	g_ap_udp_sock.client_addr.sin_addr.s_addr = 0;//htonl(g_ap_udp_sock.client_ip);
	bzero(&(g_ap_udp_sock.client_addr.sin_zero), 8);

    
    /* 显示通信连接基本信息 */
    printf("\n========== INFOMATION ===========\n");
    printf("Server:sockfd = %d\n",g_ap_udp_sock.sockfd);
    printf("Server:server IP   : %s\n",inet_ntoa(g_ap_udp_sock.server_addr.sin_addr));
    printf("Server:server PORT : %d\n",ntohs(g_ap_udp_sock.server_addr.sin_port));
    printf("Server:client IP   : %s\n",inet_ntoa(g_ap_udp_sock.client_addr.sin_addr));
    printf("Server:client PORT : %d\n",ntohs(g_ap_udp_sock.client_addr.sin_port));
	printf("Server:client MAC  : %02x:%02x:%02x:%02x:%02x:%02x\n", 
			(u8)g_ap_udp_sock.client_mac[0],(u8)g_ap_udp_sock.client_mac[1],
			(u8)g_ap_udp_sock.client_mac[2],(u8)g_ap_udp_sock.client_mac[3],
			(u8)g_ap_udp_sock.client_mac[4],(u8)g_ap_udp_sock.client_mac[5]
		);

    printf("-----------------------------------\n");
    
    /* 指定一个套接字使用的端口 */
	if (-1 == bind (g_ap_udp_sock.sockfd, (struct sockaddr *)&g_ap_udp_sock.client_addr, sizeof(struct sockaddr)))
	{
		printf("bind: %d %s\n", errno, strerror(errno));
		return CWLAN_FAIL;
	}
	
	//ret += cw_ap_local_cfg_dynamic_update_init();	
	//printf("init cw_ap_local_cfg_dynamic_update_init: %d\n", ret);
	ret += cw_ap_sendto_ac_heartbeat_init();
	printf("init cw_ap_sendto_ac_heartbeat_init: %d \n", ret);
	ret += cw_ap_recv_ac_cfg_dynamic_update_init();
	printf("init cw_ac_recv_ac_cfg_dynamic_update_init: %d\n", ret);
	ret += cw_ap_recv_kmod_info_init();
	printf("init cw_ap_recv_kmod_info_init: %d\n", ret);

	/*cloud_wlan.ko初始是关闭的。
	这个需要在用户态配置初始后重新下发一个云的正确配置*/
	/*
	if(g_ap_com_cfg.ap_cwlan_sw)
	{
		cloud_wlan_sendto_kmod(CW_NLMSG_SET_ON, NULL, 0);
	}
	*/
	return ret;
	
}
