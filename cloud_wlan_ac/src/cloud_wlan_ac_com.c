#include <unistd.h> 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <signal.h>


#include "cloud_wlan_list.h"
#include "cloud_wlan_types.h"
#include "cloud_wlan_nl.h"
#include "cloud_wlan_ac_com.h"
#include "cloud_wlan_recv_info_branch.h"
#include "cloud_wlan_ap_info_list.h"
#include "cloud_wlan_ap_info_list_db.h"


static u32 server_port;
static struct sockaddr_in server_addr;
static s32 sockfd = -1;


void hexlog(unsigned char* str, int len)
{
	unsigned char* p = str;
	int out_len = len;
	char x[17];
	int nn;
	int j;
	if (len <= 0)
		return;
	printf( "----------------hex dump start-----------------\n");
	for (nn = 0; nn < out_len; nn++) {
		printf( "%02X ", p[nn]);
		if ((p[nn] & 0xff) < 0x20 || (p[nn] & 0xff) >= 0x7f)
			x[nn % 16] = '.';
		else
			x[nn % 16] = p[nn];
		if (nn % 16 == 15 || nn == out_len - 1) {
			if (nn == out_len - 1 && nn % 16 != 15)
				for (j = 0; j < 16 - (nn % 16); j++)
					printf( "	");
			printf( "\t\t %s\n", x);
			memset(x, 0, 17);
		}
	}
	printf( "\n----------------hex dump end-------------------\n");
}

int cw_base_config_init(s8 *path)
{
	FILE *fd;

	char buff[64];
	char tmp1[64];
	
	fd = fopen (path, "r"); 
	if(fd == NULL)
	{
		printf("open fail %s\n",path);
		return CWLAN_FAIL;
	}

	while(!feof(fd))
	{
		fgets (buff, sizeof(buff), fd); 

		if( !memcmp(buff, "mysql_db_service", strlen("mysql_db_service")))
		{
			sscanf(buff, "%s = %s",tmp1,mysql_db_service); 
			continue;
		}
		if( !memcmp(buff, "user_name", strlen("user_name")))
		{
			sscanf(buff, "%s = %s",tmp1,user_name); 
			continue;
		}
		if( !memcmp(buff, "password", strlen("password")))
		{
			sscanf(buff, "%s = %s",tmp1,password); 
			continue;
		}
		if( !memcmp(buff, "mysql_db", strlen("mysql_db")))
		{
			sscanf(buff, "%s = %s",tmp1,mysql_db); 
			continue;
		}
		if( !memcmp(buff, "server_port", strlen("server_port")))
		{
			sscanf(buff, "%s = %d",tmp1,&server_port); 
			continue;
		}
		if( !memcmp(buff, "cw_table_dir", strlen("cw_table_dir")))
		{
			sscanf(buff, "%s = %s",tmp1,cw_table_dir); 
			continue;
		}
		
		if( !memcmp(buff, "cw_online_table_name", strlen("cw_online_table_name")))
		{
			sscanf(buff, "%s = %s",tmp1,cw_online_table_name); 
			continue;
		}
		if( !memcmp(buff, "cw_user_table_name", strlen("cw_user_table_name")))
		{
			sscanf(buff, "%s = %s",tmp1,cw_user_table_name); 
			continue;
		}
		if( !memcmp(buff, "cw_cmd_table_name", strlen("cw_cmd_table_name")))
		{
			sscanf(buff, "%s = %s",tmp1,cw_cmd_table_name); 
			continue;
		}
		if( !memcmp(buff, "ac_service_mode", strlen("ac_service_mode")))
		{
			sscanf(buff, "%s = %d",tmp1,&g_ac_service_mode); 
			continue;
		}
		
	}
	fclose(fd);

	if(cw_online_table_name[0] == '\0' || cw_user_table_name[0] == '\0'
		|| cw_cmd_table_name[0] == '\0' || mysql_db_service[0] == '\0' 
    	|| user_name[0] == '\0' || mysql_db[0] == '\0'
    	|| cw_table_dir[0] == '\0')
	{
		printf("config file is error\n");
		return CWLAN_FAIL;
	}
	
	printf("cw_table_dir         : %s\n", cw_table_dir);
	printf("cw_online_table_name : %s\n", cw_online_table_name);
	printf("cw_user_table_name   : %s\n", cw_user_table_name);
	printf("cw_cmd_table_name    : %s\n", cw_cmd_table_name);
	printf("user_name            : %s\n", user_name);
	printf("password             : %s\n", password);
	printf("mysql_db             : %s\n", mysql_db);
	printf("g_ac_service_mode    : %d\n", g_ac_service_mode);

	return CWLAN_OK;
}



u32 cw_sendto_info(struct sockaddr *client_addr, s8* data, u32 len)
{
	u32 sendsize=0;

	sendsize = 0;
	while( sendsize < len ) 				/* 向服务器发送数据信息 */
	{
		sendsize = sendsize + sendto(sockfd, data+sendsize, len - sendsize, 0, client_addr, sizeof(struct sockaddr));
		/* 每次发送后数据指针向后移动 */
	};
	return CWLAN_OK;
}

u32 cw_server_socket_init()
{
	if (-1 == (sockfd = socket(AF_INET, SOCK_DGRAM, 0)))        
	{
		printf("%d\n", errno);
		return CWLAN_FAIL;
	}
    printf("Server:sockfd = %d\n",sockfd);

    /* 设置服务器端信息 */
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(server_port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bzero(&(server_addr.sin_zero), 8);

	//本地地址和端口可重用，目录不需要也不能开放
	//setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&val, sizeof(s32)))

	/* 指定一个套接字使用的端口 */
	if (-1 == bind (sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)))
	{
		printf("%d\n", errno);
		return CWLAN_FAIL;
	}
	printf("Server:Bind succeed!\n");
	printf("\n========== INFOMATION ===========\n");
	printf("Server:sockfd = %d\n",sockfd);
	printf("Server:server IP   : %s\n",inet_ntoa(server_addr.sin_addr));
	printf("Server:server PORT : %d\n\n\n",ntohs(server_addr.sin_port));
	return CWLAN_OK;
}
s32 cw_ac_com_init(s8 *path)
{
	s32 ret=0;
	/*基本配置初始化*/
	ret = cw_base_config_init(path);
	if(ret != CWLAN_OK)
	{
		printf("cw ac com init fail\n");
		return CWLAN_FAIL;
	}
	/* 套接字初始化*/	
	ret = cw_server_socket_init();
	if(ret != CWLAN_OK)
	{
		printf("cw_server_socket_init fail\n");
		return CWLAN_FAIL;
	}
	
	if(g_ac_service_mode != AC_ADMIN_DB_MODE)
	{
		ret = cw_ap_info_list_init();
		if(ret != CWLAN_OK)
		{
			printf("cw_ap_info_list_init fail\n");
			return CWLAN_FAIL;
		}
	}
	else
	{
		ret = cw_ap_info_list_db_init();
		if(ret != CWLAN_OK)
		{
			printf("cw_ap_info_list_init_db fail\n");
			return CWLAN_FAIL;
		}
	}
	
	ret = cw_recv_info_dispose_pthread_cfg_init();
	if(ret != CWLAN_OK)
	{
		printf("cw_recv_info_dispose_pthread_cfg_init fail\n");
		return CWLAN_FAIL;
	}
	return CWLAN_OK;
}
void cw_ac_com_exit(int sig)
{

    /* 关闭套接字sockfd描述符 */
	close(sockfd);
    printf("\nServer:sockfd closed!\n");

	cw_recv_info_dispose_pthread_cfg_exit();

	
	if(g_ac_service_mode != AC_ADMIN_DB_MODE)
	{
		cw_ap_info_list_exit();
	}
	else
	{
		cw_ap_info_list_db_exit();
	}

	exit(0);
}

void cw_ac_com_sig_init()
{
	signal(SIGINT, cw_ac_com_exit);
	signal(SIGQUIT, cw_ac_com_exit);
	signal(SIGILL, cw_ac_com_exit);//非常指令
	signal(SIGFPE, cw_ac_com_exit);//算述异常
	signal(SIGSEGV, cw_ac_com_exit);
	signal(SIGTERM, cw_ac_com_exit);//kill 不带参数
}

s32 main(int argc, char** argv)
{
	s32 ret=0;
    u32 sin_size = sizeof(struct sockaddr_in);
	s8 buf[MAX_PROTOCOL_LPAYLOAD] = {0};
	struct sockaddr client_addr;

	if(argc < 2)
	{
		printf("%s <config file path> \n", argv[0]);
		return 0;
	}

	ret = cw_ac_com_init(argv[1]);
	if(ret != CWLAN_OK)
	{
		printf("cw ac com init fail\n");
		return CWLAN_FAIL;
	}
	
	cw_ac_com_sig_init();
	
	while(1)
	{
		/* 接收客户机端请数据信息 */
		ret = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&client_addr, &sin_size); 
		if(-1 == ret)
		{
			printf("%d\n", errno);
			//goto err_out;
			continue;
		}

		cw_recv_info_branch((dcma_udp_skb_info_t *)buf, ret, &client_addr);		

	}

	return CWLAN_OK;
}




