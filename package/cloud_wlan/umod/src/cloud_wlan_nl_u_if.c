#include <stdlib.h>  
#include <stdio.h>  
#include <unistd.h>  
#include <linux/netlink.h>  
#include <sys/socket.h>  
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdint.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <errno.h>


#include "cloud_wlan_types.h"
#include "cloud_wlan_nl.h"
#include "cloud_wlan_sqlite.h"

#include "cloud_wlan_cfg_update.h"
#include "cloud_wlan_ap_com.h"

static cw_nl_info_t g_cw_nl_info;
static struct iovec iov;  

s32 cloud_wlan_nl_cfg_init(void)
{
	u32 ret;

    g_cw_nl_info.sockfd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_CWLAN); // 创建NETLINK_CWLAN协议的socket  
    /* 设置本地端点并绑定，用于侦听 */  
    bzero(&g_cw_nl_info.src_addr, sizeof(struct sockaddr_nl));  
    g_cw_nl_info.src_addr.nl_family = AF_NETLINK;  
    g_cw_nl_info.src_addr.nl_pid = getpid();  
    g_cw_nl_info.src_addr.nl_groups = 0; //未加入多播组  
    ret = bind(g_cw_nl_info.sockfd, (struct sockaddr*)&g_cw_nl_info.src_addr, sizeof(struct sockaddr_nl));  
	if( ret != CWLAN_OK)
	{
		printf("cloud_wlan_nl_cfg_init bind error %d %s\n", errno, strerror(errno));
		return CWLAN_FAIL;
	}
    /* 构造目的端点，用于发送 */  
    bzero(&g_cw_nl_info.dst_addr, sizeof(struct sockaddr_nl));  
    g_cw_nl_info.dst_addr.nl_family = AF_NETLINK;  
    g_cw_nl_info.dst_addr.nl_pid = 0; // 表示内核  
    g_cw_nl_info.dst_addr.nl_groups = 0; //未指定接收多播组   
    /* 构造发送消息 */  

    g_cw_nl_info.nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_DATA_PAYLOAD)); 
	if(g_cw_nl_info.nlh == NULL)
	{
		printf("g_cw_nl_info struct init fail\n");
		return CWLAN_FAIL;
	}
    g_cw_nl_info.nlh->nlmsg_len = NLMSG_SPACE(MAX_DATA_PAYLOAD); //保证对齐  
    g_cw_nl_info.nlh->nlmsg_pid = getpid();  /* self pid */  
    g_cw_nl_info.nlh->nlmsg_flags = 0;  
    g_cw_nl_info.nlh->nlmsg_type = CW_NLMSG_RES_OK;  
    snprintf(NLMSG_DATA(g_cw_nl_info.nlh), MAX_DATA_PAYLOAD, "OK!\n");  
    iov.iov_base = (void *)g_cw_nl_info.nlh;  
    iov.iov_len = g_cw_nl_info.nlh->nlmsg_len;  
    g_cw_nl_info.msg.msg_name = (void *)&g_cw_nl_info.dst_addr;  
    g_cw_nl_info.msg.msg_namelen = sizeof(struct sockaddr_nl);  
    g_cw_nl_info.msg.msg_iov = &iov;  
    g_cw_nl_info.msg.msg_iovlen = 1;  

	cloud_wlan_sendto_kmod(CW_NLMSG_SET_USER_PID, (s8 *)&g_cw_nl_info.nlh->nlmsg_pid, sizeof(u32));
	return CWLAN_OK;

}
s32 cloud_wlan_nl_close(void)
{
	close(g_cw_nl_info.sockfd);
	return CWLAN_OK;
}

s32 cloud_wlan_nl_set_tpye(unsigned short nlmsg_type)
{
    g_cw_nl_info.nlh->nlmsg_type = nlmsg_type;  
	return CWLAN_OK;
}

s32 cloud_wlan_nl_set_data(s8 *buff, u32 datalen)
{
	
	if(buff == NULL)
	{
		datalen = 0;
	}
	datalen = datalen>MAX_DATA_PAYLOAD?MAX_DATA_PAYLOAD:datalen;
    memcpy(NLMSG_DATA(g_cw_nl_info.nlh), buff, datalen);
	return CWLAN_OK;
}
s32 cloud_wlan_nl_send_data(void)
{
	sendmsg(g_cw_nl_info.sockfd, &g_cw_nl_info.msg, 0); // 发送  
	return CWLAN_OK;
}
s32 cloud_wlan_nl_recv_ok(void)
{
	/* 接收消息并打印 */  
	memset(g_cw_nl_info.nlh, 0, NLMSG_SPACE(MAX_DATA_PAYLOAD));  
	recvmsg(g_cw_nl_info.sockfd, &g_cw_nl_info.msg, 0);
	
	printf("nlmsg_type: %d\n", g_cw_nl_info.nlh->nlmsg_type); 
	printf("Received message payload: %s\n",
		NLMSG_DATA(g_cw_nl_info.nlh));  
	return CWLAN_OK;
}
s32 cloud_wlan_nl_recv_kmod(dcma_udp_skb_info_t *buff)
{
	int recv_st=0;
	/* 接收消息并打印 */  
	memset(g_cw_nl_info.nlh, 0, NLMSG_SPACE(MAX_DATA_PAYLOAD));  
	recvmsg(g_cw_nl_info.sockfd, &g_cw_nl_info.msg, 0);
	if(-1 == recv_st)
	{
		printf("recvfrom :%d %s\n", errno, strerror(errno));
		return CWLAN_FAIL;
	}
	buff->type = g_cw_nl_info.nlh->nlmsg_type;
	memcpy(buff->data, NLMSG_DATA(g_cw_nl_info.nlh), MAX_DATA_PAYLOAD); 

	return CWLAN_OK;
}

s32 cloud_wlan_sendto_kmod(s32 type, s8 *buff, u32 datalen)
{
	cloud_wlan_nl_set_tpye(type);
	cloud_wlan_nl_set_data(buff, datalen);
	cloud_wlan_nl_send_data();
	return CWLAN_OK;
}
s32 cloud_wlan_sendto_kmod_ok(s32 type, s8 *buff, u32 datalen)
{
	cloud_wlan_nl_set_tpye(type);
	cloud_wlan_nl_set_data(buff, datalen);
	cloud_wlan_nl_send_data();
	
	cloud_wlan_nl_recv_ok();
	
	return CWLAN_OK;
}

