#include <linux/in.h>
#include <linux/skbuff.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/if_vlan.h>

//#include "cloud_wlan_types.h"
#include "cloud_wlan_nl.h"
#include "cloud_wlan_main.h"
#include "cloud_wlan_http_pub.h"
#include "cloud_wlan_session.h"

u32 cloud_wlan_get_http_header_info(u8* buf, http_t *http_info)
{
	u8 *p = buf;
	u8 *tbuf0 ;
	u8 *tbuf1;
	u32 tk;

	if(p == NULL || strlen(p)<15)
	{
		return CWLAN_FAIL;
	}
	memset(http_info->host, 0 , 256);
	
	/*Judge each \r\n on the front is not the HTTP protocol version number*/
	p = memchr(p,'\r',strlen(p));
	tk = memcmp(p-8, "HTTP/1." ,7);
	if( tk != 0)
	{
		printk("skb is not replace http!\n");
		return CWLAN_FAIL;
	}

	p =buf;
	while(1)
	{
		tbuf0 = p;
		p = memchr(tbuf0,'\r',strlen(tbuf0));
		if(*(p+1) == '\n')
		{
			/*Determine whether or not the message head-end\r\n\r\n*/
			if(*(p+2) == '\r')
			{
				return CWLAN_FAIL;
			}
			p = p+2;

			
			tk = memcmp(p, "Host:" ,5);
			if( tk == 0)
			{	/*Gets the host field contents*/
				tbuf0 = p+5;
				tbuf1= memchr(tbuf0,'\r',strlen(tbuf0));
				strncpy(http_info->host,p+5,ABS(tbuf1-tbuf0));
				
				return CWLAN_OK;
			}
			/*For additional fields
			tk = memcmp(p, "Referer:" ,8);
			if( tk == 0)
			{	
				tbuf0 = p+8;
				tbuf1= memchr(tbuf0,'\r',strlen(tbuf0));
				strncpy(url,p+8,ABS(tbuf1-tbuf0));				
				break;
			}
		
			*/
		}
			
			
	}

	return CWLAN_FAIL;
}
/*解析回复报文reply*/
u32 cloud_wlan_http_skb_parse_reply(u8* buf, http_t *http_info)
{
	u8 *p = buf;
	u8 *tbuf0 ;
	//u8 *tbuf1;
	u32 tk;
	u8 temp[20]={0};
	
	if(p == NULL || strlen(p)<15)
	{
		return CWLAN_FAIL;
	}
	if(http_info != NULL)
	{
		memset(http_info, 0 , 1024);
	}
	/*Judge each \r\n on the front is not the HTTP protocol version number*/
	//p = memchr(p,'\r',strlen(p));
	tk = memcmp(p, "HTTP/1." ,7);
	if( tk != 0)
	{
		memcpy(temp, p, 15);
		CLOUD_WLAN_DEBUG("http heard: %s\n", temp);		
		CLOUD_WLAN_DEBUG("skb is not response http! \n");
		return CWLAN_FAIL;
	}

	while(1)
	{
		tbuf0 = p;
		p = memchr(tbuf0,'\r',strlen(tbuf0));
		if(*(p+1) == '\n')
		{
			/*Determine whether or not the message head-end\r\n\r\n*/
			if(*(p+2) == '\r')
			{
				return CWLAN_FAIL;
			}
			p = p+2;

			
			tk = memcmp(p, "xnat:" ,5);
			if( tk == 0)
			{
				CLOUD_WLAN_DEBUG("skb is xnat!\n");
				return CWLAN_OK;
			}
			/*For additional fields
			tk = memcmp(p, "Referer:" ,8);
			if( tk == 0)
			{	
				tbuf0 = p+8;
				tbuf1= memchr(tbuf0,'\r',strlen(tbuf0));
				strncpy(url,p+8,ABS(tbuf1-tbuf0));				
				break;
			}
		
			*/
		}
			
			
	}

	return CWLAN_FAIL;
}

/*请求头部解析request*/
u32 cloud_wlan_http_skb_parse_request(struct sk_buff *skb, http_t *http_info)
{
	u32 ret;
	struct iphdr *iphdr;
	struct tcphdr *tcphdr;
	
	iphdr = ip_hdr(skb);
	tcphdr = tcp_hdr(skb);
	
	if((IPPROTO_TCP != iphdr->protocol) 
		|| (PROTO_HTTP!= ntohs(tcphdr->dest) && PROTO_HTTP2 != ntohs(tcphdr->dest)) )
	{
		return CWLAN_FAIL;
	}
	/*分析报文头get 未使用*/
	ret = is_http_get_pkt((void *)tcphdr + tcphdr->doff*4 , iphdr->tot_len - tcphdr->doff*4);

	if(ret != CWLAN_OK)
	{
		return CWLAN_FAIL;
	}

	ret = cloud_wlan_get_http_header_info( (u8 *)tcphdr + tcphdr->doff * 4, http_info);
	if(ret != CWLAN_OK)
	{
		return CWLAN_FAIL;
	}

	return CWLAN_OK;
}
