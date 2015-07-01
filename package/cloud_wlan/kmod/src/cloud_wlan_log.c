#include <linux/module.h>  
#include <linux/netlink.h>  
#include <net/netlink.h>  
#include <net/net_namespace.h>  
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <linux/if_ether.h>

#include "cloud_wlan_nl.h"

#include "cloud_wlan_main.h"
#include "cloud_wlan_session.h"
#include "cloud_wlan_http_pub.h"
#include "cloud_wlan_log.h"

u32 g_cloud_wlan_klog_switch = 0;

u32 cloud_wlan_klog_url(struct sk_buff *skb, cloud_wlan_quintuple_s *quintuple_info)
{
	u32 ret;
	u32 bufflen;
	kmod_log_info_t *buff;
	http_t url_log;
	
	bufflen = sizeof(kmod_log_info_t)+HTTP_HOST_LEN;

	ret = cloud_wlan_http_skb_parse_request(skb, &url_log);
	if(ret == CWLAN_FAIL)
	{
		return CWLAN_FAIL;
	}
	
	buff = kmalloc(bufflen, GFP_KERNEL);
	if(buff == NULL)
	{
		return CWLAN_FAIL;
	}
	buff->size = bufflen;
	buff->userip = quintuple_info->ip_hd.saddr;
	memcpy(buff->usermac, quintuple_info->eth_hd.h_source, 6);
	memcpy(buff->data, url_log.host, HTTP_HOST_LEN);
	
	cloud_wlan_sendto_umod(CW_NLMSG_PUT_KLOG_INFO, (s8 *)buff, bufflen);

	kfree(buff);
	
	return CWLAN_OK;
}
u32 cloud_wlan_generate_klog_main(struct sk_buff *skb, cloud_wlan_quintuple_s *quintuple_info, u32 type)
{
	struct tcphdr *tcphdr;
	tcphdr = tcp_hdr(skb);

	if(!g_cloud_wlan_klog_switch || ntohs(tcphdr->dest) != PROTO_HTTP)
	{
		return CWLAN_FAIL;
	}


	switch(type)
	{
		case KLOG_URL:
			cloud_wlan_klog_url(skb, quintuple_info);
			break;

		default:
			break;
	}
	
	return CWLAN_OK;
}
