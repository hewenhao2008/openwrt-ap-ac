#include <unistd.h> 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>

#include <mysql/mysql.h>
#include "cloud_wlan_types.h"
#include "cloud_wlan_nl.h"
#include "cloud_wlan_ac_com.h"
#include "cloud_wlan_ap_info_list_db.h"
#include "cloud_wlan_db_cmd_parser.h"

MYSQL *conn;

s8 cw_table_dir[100]={0};
s8 cw_online_table_name[100]={0};
s8 cw_user_table_name[100]={0};
s8 cw_cmd_table_name[100]={0};
s8 mysql_db_service[100]={0};
s8 mysql_db[50]={0};
s8 user_name[50]={0};
s8 password[50]={0};

u8 *ip_ntoa(u8* ipbuf,u32 ina)
{
		   u8 *ucp = (u8 *)&ina;

		   sprintf((s8 *)ipbuf, "%d.%d.%d.%d",
				   ucp[0] & 0xff,
				   ucp[1] & 0xff,
				   ucp[2] & 0xff,
				   ucp[3] & 0xff);
		   return ipbuf;
}

/*这个更新ap状态是以web管理面新建，本身没有新加ap功能*/
/*在更新的同时还需要处理配置下发的处理*/
u32 cw_admin_db_update_ap_info_list(struct sockaddr *client_addr, ap_local_info_t ap_info)
{
	MYSQL_RES *res;
	MYSQL_ROW row; 
	s32 ret;
	dcma_udp_skb_info_t *buff;
	u8 sendbuf[MAX_PROTOCOL_LPAYLOAD] = {0};
	s8 g_sql[CW_MYSQL_LEN];
	u32 sendlen = 0;
	buff = (dcma_udp_skb_info_t *)sendbuf;

	snprintf(g_sql, CW_MYSQL_LEN, "update %s set apmac='%x%x%x%x%x%x', "
		"mem_total=%d, mem_free=%d, cpu_idle_rate=%d, run_time=%lld;",
		cw_user_table_name,
		ap_info.apmac[0],ap_info.apmac[1],ap_info.apmac[2],
		ap_info.apmac[3],ap_info.apmac[4],ap_info.apmac[5],
		ap_info.mem_total, ap_info.mem_free, ap_info.cpu_idle_rate, ap_info.run_time
	);

	ret = mysql_query(conn, g_sql);
	if(ret != 0)
	{
		//更新出错
		printf("%s\n\nERROR: %s\n",g_sql ,mysql_error(conn)); 
		return CWLAN_OK;
	}
	
	snprintf(g_sql, CW_MYSQL_LEN, "select type, cmd from %s where apmac='%x%x%x%x%x%x';",
		cw_cmd_table_name,
		ap_info.apmac[0],ap_info.apmac[1],ap_info.apmac[2],
		ap_info.apmac[3],ap_info.apmac[4],ap_info.apmac[5]
	);

	ret = mysql_query(conn, g_sql);
	if(ret != 0)
	{
		//查询出错
		printf("%s\n\nERROR: %s\n",g_sql ,mysql_error(conn)); 
		return CWLAN_OK;
	}

	res = mysql_store_result(conn);
	while((row=mysql_fetch_row(res)))
	{

		buff->type = atoi(row[0]);
		buff->number = 1;
		
		cw_db_cmd_branch(buff->type, row[1], (s8 *)buff->data, &sendlen);
		sendlen = sendlen + sizeof(dcma_udp_skb_info_t);
		cw_sendto_info(client_addr, (s8 *)&sendbuf, sendlen);
		sendlen =0;

	}
	
	mysql_free_result(res); //清理数据

	buff->type = CW_NLMSG_HEART_BEAT;
	buff->number = 1;
	sendlen = sizeof(dcma_udp_skb_info_t);
	cw_sendto_info(client_addr, (s8 *)sendbuf, sendlen);

	return CWLAN_OK;
}

u32 cw_admin_db_update_user_info_list(struct sockaddr *client_addr, online_user_info_t user_info)
{
	u8 buf[16]={0};
	
	s8 g_sql[CW_MYSQL_LEN];
	ip_ntoa(buf, user_info.userip);

	snprintf(g_sql, CW_MYSQL_LEN, "INSERT INTO %s set usermac='%x%x%x%x%x%x', apmac='%x%x%x%x%x%x', userip='%s', "
		"status=%d, time=%lld on duplicate key update userip='%s', status=%d, time=%lld;",
		cw_online_table_name,
		user_info.usermac[0],user_info.usermac[1],user_info.usermac[2],
		user_info.usermac[3],user_info.usermac[4],user_info.usermac[5],
		user_info.apmac[0],user_info.apmac[1],user_info.apmac[2],
		user_info.apmac[3],user_info.apmac[4],user_info.apmac[5],
		
		buf, user_info.status, user_info.time,
		buf, user_info.status, user_info.time
	);
	if(mysql_query(conn, g_sql))
	{ 
		printf("%s\n\nERROR: %s\n",g_sql ,mysql_error(conn)); 
	}

	return CWLAN_OK;
}

/*目前ac服务端与web相关的数据库中就三张表
	需要老化的只有在线用户表
*/
u32 cw_admin_db_online_user_age_del()
{
	s8 sql[128];

	while(1)
	{

		snprintf(sql, 128, "DELETE FROM %s WHERE status=%d;",
			cw_user_table_name, CW_FS_DOWN);
		/* send SQL query */ 
		if(mysql_query(conn, sql))
		{ 
			printf("%s\n\nERROR: %s\n",sql ,mysql_error(conn)); 
		}

		sleep(60);
	}

}


static int cw_create_table(s8 *table_name)
{
	s8 g_sql[CW_MYSQL_LEN];

	s8 buf[CW_MYSQL_LEN] = {0};
	s8 table_path[100] = {0};
	FILE *fd;

	snprintf(table_path, 100, "%s/%s.txt",cw_table_dir, table_name);
	
	fd = fopen (table_path, "r"); 
	if(fd == NULL)
	{
		printf("open fail %s\n",table_path);
		return CWLAN_FAIL;
	}

	fread(buf, 1, CW_MYSQL_LEN, fd);


	snprintf(g_sql, CW_MYSQL_LEN, buf, table_name);

	/* send SQL query */ 
	if (mysql_query(conn, g_sql))
	{ 
		printf("%s\n\nERROR: %s\n",g_sql ,mysql_error(conn)); 
		return CWLAN_FAIL;
	} 
	printf("init tables %s ok!\n", table_name);

	fclose(fd);
	return CWLAN_OK;

}

u32 cw_ap_info_list_db_init()
{
	s32 ret = 0;
    conn = mysql_init(NULL); 
	/* Connect to database */ 
	if (!mysql_real_connect(conn, mysql_db_service, 
		   user_name, password, mysql_db, 0, NULL, 0)) { 
	   fprintf(stderr, "%s\n", mysql_error(conn)); 
	   return CWLAN_FAIL;
	}

	
	ret += cw_create_table(cw_online_table_name);
	ret += cw_create_table(cw_user_table_name);
	ret += cw_create_table(cw_cmd_table_name);
	if(ret != CWLAN_OK)
	{
		printf("cw ac tables init fail\n");
		return CWLAN_FAIL;
	}
	return CWLAN_OK;
}
u32 cw_ap_info_list_db_exit()
{
	mysql_close(conn);
	return CWLAN_OK;
}

