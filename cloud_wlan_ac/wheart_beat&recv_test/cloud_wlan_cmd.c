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
#include <sys/stat.h>
#include <stdint.h>
#include <netdb.h>

#include "cloud_wlan_types.h"
#include "cloud_wlan_nl.h"
#include "cloud_wlan_sqlite.h"
//#include "cloud_wlan_cfg_update.h"
#include "cloud_wlan_ap_com.h"
//#include "cloud_wlan_cfg_init.h"
#include "cloud_wlan_des.h"

u32 cw_debug_show_help()
{
	printf("ap help\n");
	
	printf("	test	<data>				ap test netlink\n");

	printf("	switch	<on|off>			ap set cloud switch status\n");
	printf("	debug   <on|off>			ap set cloud debug status\n");

	//printf("	show	<ap_cfg>			ap show local config\n");
	printf("	show	<p_url>				ap show protal url\n");
	printf("	show	<w_list>			ap show white list\n");
	printf("	show	<online>			ap show online user info\n");
	printf("	des     <e|d> <data>		ap des url \n");
	printf("	des     <def>				ap des url \n\n");
printf("ap set (need restart cloud_wlan_ap_com)\n");
	printf("	set		<ac_com_addr>    <data>				set ap to ac addr\n");
	printf("	set		<ac_com_port>    <data>				set ap to ac port\n");
	printf("	set		<ap_com_eth>     <data>				set　ap to ac eth\n");
	printf("	set		<over_time>	     <data>				setap user old time\n");
	printf("	set     <interval_timer> <data>				set　ap user old check timer \n");
	printf("	set     <del_time>		 <data>				set　ap user old del timer \n\n");

	return CWLAN_OK;
}


u32 cw_debug_show_process(s8 *cmd)
{
	if(!strcmp(cmd, "p_url"))
	{
		cloud_wlan_sendto_kmod(CW_NLMSG_DEBUG_SHOW_PORTAL, NULL, 0);
		return CWLAN_OK;
	}
	if(!strcmp(cmd, "w_list"))
	{
		cloud_wlan_sendto_kmod(CW_NLMSG_DEBUG_SHOW_WHITE_LIST, NULL, 0);
		return CWLAN_OK;
	}
	if(!strcmp(cmd, "online"))
	{
		cloud_wlan_sendto_kmod(CW_NLMSG_DEBUG_SHOW_ONLINE_USER, NULL, 0);
		return CWLAN_OK;
	}
	cw_debug_show_help();
	return CWLAN_OK;	

	return CWLAN_OK;
}
u32 cw_debug_set_switch_process(s8 *cmd)
{
	if(!strcmp(cmd, "on"))
	{
		cloud_wlan_sendto_kmod(CW_NLMSG_SET_ON, NULL, 0);
		return CWLAN_OK;
	}
	if(!strcmp(cmd, "off"))
	{
		cloud_wlan_sendto_kmod(CW_NLMSG_SET_OFF, NULL, 0);
		return CWLAN_OK;
	}
	cw_debug_show_help();
	return CWLAN_OK;	
}
u32 cw_debug_set_debug_process(s8 *cmd)
{
	if(!strcmp(cmd, "on"))
	{
		cloud_wlan_sendto_kmod(CW_NLMSG_SET_DEBUG_ON, NULL, 0);
		return CWLAN_OK;
	}
	if(!strcmp(cmd, "off"))
	{
		cloud_wlan_sendto_kmod(CW_NLMSG_SET_DEBUG_OFF, NULL, 0);
		return CWLAN_OK;
	}
	cw_debug_show_help();
	return CWLAN_OK;
}
u32 cw_debug_des_process(s8 *cmd, s8 *data)
{
	s8 out[CW_DES_LEN]={0};
	s8 *decryt;
	u32 ret;
	sqlite3 *db;
	u32 data_len;

	ret =sqlite3_open(CWLAN_AP_CFG_DB, &db);
	if( ret )						//如果出错，给出提示信息并退出程序	
	{
		printf("INIT:Can'topen database: %s\n", sqlite3_errmsg(g_cw_db));  
		sqlite3_close(db); 
		return CWLAN_OK;
	}

	if(!strcmp(cmd, "def"))
	{
		DES_Act(out, CW_LOCATION_URL_DATA, strlen(CW_LOCATION_URL_DATA),g_des_key, DES_KEY_LEN, ENCRYPT);
/*目前不区分portal类型*/
		snprintf(g_cw_sql, G_CW_SQL_BUF_LEN, 
			"UPDATE %s SET url=?;",CWLAN_AP_PORTAL_TABLE);
		sqlite3_binary_write1(db,g_cw_sql,out, CW_DES_LEN);
		sqlite3_close(db);
		return CWLAN_OK;
	}
	if(!strcmp(cmd, "e") && data != NULL)
	{
		DES_Act(out, data, strlen(data),g_des_key, DES_KEY_LEN, ENCRYPT);

		snprintf(g_cw_sql, G_CW_SQL_BUF_LEN, "UPDATE %s SET url=?;",CWLAN_AP_PORTAL_TABLE);
		sqlite3_binary_write1(db,g_cw_sql,out, CW_DES_LEN);
		sqlite3_close(db); 
		return CWLAN_OK;
	}
	if(!strcmp(cmd, "d"))
	{
		snprintf(g_cw_sql, G_CW_SQL_BUF_LEN, "select url from %s;", CWLAN_AP_PORTAL_TABLE);

		data_len = sqlite3_binary_read(db, g_cw_sql, &decryt, 0);

		DES_Act(out, decryt, data_len,g_des_key, DES_KEY_LEN, DECRYPT);
		printf("%d %s\n",data_len, out);

		sqlite3_close(db); 
		return CWLAN_OK;
	}
	cw_debug_show_help();
	
	sqlite3_close(db); 
	return CWLAN_OK;
}
u32 cw_debug_test_process(s8 *data)
{
	cloud_wlan_sendto_kmod_ok(CW_NLMSG_GET_TEST, data, strlen(data));
	
	return CWLAN_OK;
}
u32 cw_debug_set_process(s8 *cmd, s8 *data)
{
		u32 ret;
		sqlite3 *db;
		sqlite3_res res;
		s8 *buf=NULL;
		cwlan_flow_session_cfg_t temp_session_cfg;

		ret =sqlite3_open(CWLAN_AP_CFG_DB, &db);
		if( ret )						//如果出错，给出提示信息并退出程序	
		{
			printf("INIT:Can'topen database: %s\n", sqlite3_errmsg(g_cw_db));  
			sqlite3_close(db); 
			return CWLAN_OK;
		}
	
		if(!strcmp(cmd, "ac_com_addr") && data != NULL)
		{
			snprintf(g_cw_sql, G_CW_SQL_BUF_LEN, 
				"UPDATE %s SET ac_com_addr='%s';",CWLAN_AP_CFG_TABLE,data);
			sqlite3_exec_unres(db, g_cw_sql);
			sqlite3_close(db);
			return CWLAN_OK;
		}
		if(!strcmp(cmd, "ac_com_port") && data != NULL)
		{
			snprintf(g_cw_sql, G_CW_SQL_BUF_LEN, 
				"UPDATE %s SET ac_com_port=%s;",CWLAN_AP_CFG_TABLE,data);
			sqlite3_exec_unres(db, g_cw_sql);
			sqlite3_close(db);
			return CWLAN_OK;
		}
		if(!strcmp(cmd, "ap_com_eth") && data != NULL)
		{
			snprintf(g_cw_sql, G_CW_SQL_BUF_LEN, 
				"UPDATE %s SET ap_com_eth='%s';",CWLAN_AP_CFG_TABLE,data);
			sqlite3_exec_unres(db, g_cw_sql);
			sqlite3_close(db);
			return CWLAN_OK;
		}

		if(!strcmp(cmd, "over_time") && data != NULL)
		{
			snprintf(g_cw_sql, G_CW_SQL_BUF_LEN, 
				"UPDATE %s SET over_time='%s';",CWLAN_AP_CFG_TABLE,data);
			sqlite3_exec_unres(db, g_cw_sql);
			sqlite3_close(db);
			return CWLAN_OK;
		}
		if(!strcmp(cmd, "interval_timer") && data != NULL)
		{
			snprintf(g_cw_sql, G_CW_SQL_BUF_LEN, 
				"UPDATE %s SET over_time='%s';",CWLAN_AP_CFG_TABLE,data);
			sqlite3_exec_unres(db, g_cw_sql);
			sqlite3_close(db);
			return CWLAN_OK;
		}
		if(!strcmp(cmd, "del_time") && data != NULL)
		{
			snprintf(g_cw_sql, G_CW_SQL_BUF_LEN, 
				"UPDATE %s SET over_time='%s';",CWLAN_AP_CFG_TABLE,data);
			sqlite3_exec_unres(db, g_cw_sql);
			sqlite3_close(db);
			return CWLAN_OK;
		}

		cw_debug_show_help();
		
		sqlite3_close(db); 
		return CWLAN_OK;
}
int main(int argc, char **argv)  
{

	if( argc<=2 )						//如果出错，给出提示信息并退出程序	
	{
		cw_debug_show_help();
		return 0;
	}
	cloud_wlan_nl_cfg_init();

	if(!strcmp(argv[1], "set"))
	{
		cw_debug_set_process(argv[2], argv[3]);
		goto quit;
	}
 	if(!strcmp(argv[1], "show"))
	{
		cw_debug_show_process(argv[2]);
		goto quit;
	}
	if(!strcmp(argv[1], "switch"))
	{
		cw_debug_set_switch_process(argv[2]);
		goto quit;
	}
	if(!strcmp(argv[1], "debug"))
	{
		cw_debug_set_debug_process(argv[2]);
		goto quit;
	}
	
	if(!strcmp(argv[1], "des"))
	{
		cw_debug_des_process(argv[2],argv[3]);
		goto quit;
	}
	if(!strcmp(argv[1], "test"))
	{
		cw_debug_test_process(argv[2]);
		goto quit;
	}
quit:
	cloud_wlan_nl_close();
	return CWLAN_OK;
}

