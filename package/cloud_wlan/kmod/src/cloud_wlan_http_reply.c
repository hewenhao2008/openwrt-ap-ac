#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <net/ip.h>
#include <linux/udp.h>
#include <linux/in.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/if_ether.h>
#include <linux/if.h>
#include <linux/socket.h>
#include <net/route.h>
#include <net/dst.h>
#include <linux/inetdevice.h>

#include <linux/timer.h>
#include <linux/timex.h>
#include <linux/rtc.h>
#include <linux/version.h>
#include <net/xfrm.h>

//#include "cloud_wlan_types.h"
#include "cloud_wlan_nl.h"
#include "cloud_wlan_main.h"
#include "cloud_wlan_http_pub.h"
#include "cloud_wlan_session.h"


u8 *week[7]={"Mon","Tues","Wed","Thur","Fri","Sat","Sun"};
u8 *month[12]={"Jan","Feb","Mar","Apr","May","June","July","Aug","Sept","Oct","Nov","Dec"};

/*
#define WRITE_DATA_BUFFER(data_buff, Location, Data_len, date_gmt) {\
			snprintf((data_buff),HTTP_RESPONSE_PKT_LEN,\
				"%s\r\n%s\r\n%s\r\n%s%s\r\n%s%s\r\n\r\n",\
				HTTP_RESPONSE_HEAD,\
				HTTP_SERVER,\
				HTTP_LOCATION,(Location),\
				HTTP_CONTENT_TYPE,\
				HTTP_DATE, (date_gmt));\
			}
*/

#define WRITE_DATA_BUFFER(data_buff, Location, apmac, stamac, staip, Data_len, date_gmt) {\
			snprintf((data_buff),HTTP_RESPONSE_PKT_LEN,\
				"%s\r\n%s\r\n%s\r\n%s%s?apmac=%s&stamac=%s&staip=%s;\r\n\r\n",\
				HTTP_RESPONSE_HEAD,\
				HTTP_CONTENT_TYPE,\
				HTTP_SERVER,\
				HTTP_LOCATION,(Location),(apmac),(stamac),(staip)\
								);\
			}

static u32 ORIGIN_TCP_FLAG_ACK(struct tcphdr *tcp_hdr)
{
	return (tcp_hdr->fin == 0 && tcp_hdr->syn == 0 && \
			tcp_hdr->rst == 0 && tcp_hdr->ack == 1 && \
			tcp_hdr->psh == 0);
}

static u32 ORIGIN_TCP_FLAG_SYN(struct tcphdr *tcp_hdr)
{
	return (tcp_hdr->fin == 0 && tcp_hdr->syn == 1 && \
			tcp_hdr->rst == 0 && tcp_hdr->ack == 0 && \
			tcp_hdr->psh == 0);
}

static u32 ORIGIN_TCP_FLAG_FIN(struct tcphdr *tcp_hdr)
{
	return (tcp_hdr->fin == 1 && tcp_hdr->syn == 0 && \
			tcp_hdr->rst == 0 && tcp_hdr->ack == 0 && \
			tcp_hdr->psh == 0);
}
											
static u32 ORIGIN_TCP_FLAG_FIN_ACK(struct tcphdr *tcp_hdr)
{
	return (tcp_hdr->fin == 1 && tcp_hdr->syn == 0 && \
			tcp_hdr->rst == 0 && tcp_hdr->ack == 1 && \
			tcp_hdr->psh == 0);
}
										
static u32 ORIGIN_TCP_FLAG_PSH_ACK(struct tcphdr *tcp_hdr)
{
	return (tcp_hdr->fin == 0 && tcp_hdr->syn == 0 && \
			tcp_hdr->rst == 0 && tcp_hdr->ack == 1 && \
			tcp_hdr->psh == 1);
}
	
//portal 重定向信息数据结构
portal_conf_t g_portal_config;

//ip头校验和函数
u16 csum (u16 *packet, u32 packlen)
{
	register unsigned long sum = 0;
	while (packlen > 1)
	{
		sum+= *(packet++);
		packlen-=2;
	}
	if (packlen > 0)
	{
		sum += *(unsigned char *)packet;
	}
	while (sum >> 16)
	{
		sum = (sum & 0xffff) + (sum >> 16);
	}
	return (u16) ~sum;
}
//tcp头校验和函数
u16 tcpcsum (unsigned char *iphdr, u16 *packet,u32 packlen)
{
	u16 *buf;
	u16 res;
	buf = kmalloc(packlen+12, GFP_KERNEL);
	if(buf == NULL) 
	{
		return 0;
	}
	memcpy(buf,iphdr+12,8); //源目地地址
	*(buf+4)=htons((u16)(*(iphdr+9)));
	*(buf+5)=htons((u16)packlen);
	memcpy(buf+6,packet,packlen);
	res = csum(buf,packlen+12);
	kfree(buf);
	return res;
}

u32 ip_tcp_csum( struct iphdr *ipv4_hdr, struct tcphdr *tcp_hdr, u32 data_len)
{

	ipv4_hdr->tot_len = htons( ipv4_hdr->ihl * 4 + tcp_hdr->doff * 4 + data_len);
	ipv4_hdr->check = csum((u16 *)ipv4_hdr,ipv4_hdr->ihl * 4);
	tcp_hdr->check = tcpcsum((unsigned char *)ipv4_hdr, (u16 *)tcp_hdr, tcp_hdr->doff * 4 +data_len);
	return CWLAN_OK;
}

u32 is_http_get_pkt(void *data ,u32 skb_data_len)
{

    if ( skb_data_len <= 0
		|| skb_data_len <= 5)
	{
		return CWLAN_FAIL;
    }
	if(memcmp(data, "GET ", 4) != 0)
	{
		return CWLAN_FAIL;
	}

	return CWLAN_OK;

}
/******************************************************************************* 
功能: http报文tcp 分片hash查找更新数据
 -------------------------------------------------------------------------------
参数:	
-------------------------------------------------------------------------------
返回值:	
*******************************************************************************/
u32 reply_tcp_fragment_updata(cloud_wlan_quintuple_s *quintuple_info)
{
	//return flow_session_tcp_fragment_inc(quintuple_info);
	return CWLAN_OK;
}

u32 reply_tcp_fragment_clr(cloud_wlan_quintuple_s *quintuple_info)
{
	//return flow_session_tcp_fragment_clr(quintuple_info);
	return CWLAN_OK;
}

/******************************************************************************* 
功能: http报文3L头
 -------------------------------------------------------------------------------
参数:	ipv4_hdr；3L数据头指针
-------------------------------------------------------------------------------
返回值:	
*******************************************************************************/
u32 reply_packet_iphdr_data(struct iphdr *ipv4_hdr, struct iphdr *ipv4_hdr_t)
{
	ipv4_hdr_t->id=0;
	ipv4_hdr_t->version=4;
	ipv4_hdr_t->ihl=ipv4_hdr->ihl;
	ipv4_hdr_t->ttl=64;
	ipv4_hdr_t->protocol=6;	  //tcp 6
	ipv4_hdr_t->frag_off=ntohs(0x4000);
	ipv4_hdr_t->tos=ipv4_hdr->tos;

	ipv4_hdr_t->daddr=ipv4_hdr->saddr;
	ipv4_hdr_t->saddr=ipv4_hdr->daddr;
	ipv4_hdr_t->check = 0;

	return CWLAN_OK;
}
/******************************************************************************* 
功能: http报文4L头
 -------------------------------------------------------------------------------
参数:	tcp_hdr；4L数据头指针
-------------------------------------------------------------------------------
返回值:	CWLAN_FAIL		丢包处理
			CWLAN_OK		发302数据报文
			FIN_SYN_ACK	回ack 报文
*******************************************************************************/
u32 reply_packet_tcphdr_data(struct sk_buff *skb, struct tcphdr *tcp_hdr_s, struct tcphdr *tcp_hdr_t)
{
	struct iphdr *iphdr = ip_hdr(skb);
	u32 data_len = iphdr->tot_len - iphdr->ihl * 4 - tcp_hdr_s->doff * 4;
	u32 ret;
	u32 is_get;
	
	tcp_hdr_t->dest = tcp_hdr_s->source;
	tcp_hdr_t->source = tcp_hdr_s->dest;
	tcp_hdr_t->doff = tcp_hdr_s->doff;
	tcp_hdr_t->window = tcp_hdr_s->window;
	tcp_hdr_t->check = 0;
	tcp_hdr_t->seq = tcp_hdr_s->ack_seq;
	tcp_hdr_t->ack = 1;

	memcpy((void *)tcp_hdr_t + 20, (void *)tcp_hdr_s + 20, tcp_hdr_s->doff * 4 - 20);
	
	//这是一个SYN　包
	if(ORIGIN_TCP_FLAG_SYN(tcp_hdr_s) )
	{
		CLOUD_WLAN_DEBUG("tcphdr data S syn \n");

		tcp_hdr_t->ack_seq = htonl(ntohl(tcp_hdr_s->seq)+ 1);
		tcp_hdr_t->syn = 1;
		return RE_PORTAL_RESPONSE;
	}

	/*分析报文头get 未使用*/
	is_get = is_http_get_pkt((void *)tcp_hdr(skb) + tcp_hdr_t->doff*4 , data_len);
	//这是一个ACK　包
	if(ORIGIN_TCP_FLAG_ACK(tcp_hdr_s))
	{

		CLOUD_WLAN_DEBUG("tcphdr data S ack and is_get [%d]\n",is_get);
		//是ack get包直接丢包不做处理
		if(is_get == CWLAN_OK)
		{//更新看是不是tcp分片，需要不需要回ack包。
			//ret = reply_tcp_fragment_updata(quintuple_info);
			//当前认为所有报文不分片
			ret = CWLAN_OK;
			if( ret == CWLAN_OK)
			{
				tcp_hdr_t->ack_seq = htonl(ntohl(tcp_hdr_s->seq)+ data_len);
				tcp_hdr_t->psh = 1;
				tcp_hdr_t->fin= 1;
				return RE_PORTAL_DATA;
			}
		}
		return RE_PORTAL_DROP;
	}
	//这是第一个数据包
	if(ORIGIN_TCP_FLAG_PSH_ACK(tcp_hdr_s) )
	{
		CLOUD_WLAN_DEBUG("tcphdr data S psh ack and is_get [%d]\n",is_get);
		//是ack get包直接丢包不做处理
		if(is_get == CWLAN_OK)
		{
			//tcp fragment 置0
			//reply_tcp_fragment_clr(quintuple_info);
			tcp_hdr_t->ack_seq = htonl(ntohl(tcp_hdr_s->seq)+ data_len);
			tcp_hdr_t->psh = 1;
			tcp_hdr_t->fin= 1;
			return RE_PORTAL_DATA;
		}
		return RE_PORTAL_DROP;
	}

	//这是一个FIN_ACK
	if(ORIGIN_TCP_FLAG_FIN(tcp_hdr_s) 
		||ORIGIN_TCP_FLAG_FIN_ACK(tcp_hdr_s) )
	{
		CLOUD_WLAN_DEBUG("tcphdr data S fin ack \n");

		tcp_hdr_t->ack_seq = htonl(ntohl(tcp_hdr_s->seq)+ 1);
		return RE_PORTAL_RESPONSE;
	}
	
	//这是一个RST_ACK　包,直接丢包不处理

	return RE_PORTAL_DROP;
	
}
/******************************************************************************* 
功能: http报文2L头
 -------------------------------------------------------------------------------
参数:	eth_hdr；2L数据头指针
-------------------------------------------------------------------------------
返回值:	
*******************************************************************************/
u32 reply_packet_ethhdr_data(struct ethhdr *eth_hdr, struct ethhdr *eth_hdr_t)
{
	eth_hdr_t->h_proto = eth_hdr->h_proto;
	memcpy(eth_hdr_t->h_dest, eth_hdr->h_source, ETH_ALEN);
	memcpy(eth_hdr_t->h_source, eth_hdr->h_dest, ETH_ALEN);

	return CWLAN_OK;
}
/******************************************************************************* 
功能: http报文302数据信息
 -------------------------------------------------------------------------------
参数:	data_hdr 数据头
-------------------------------------------------------------------------------
返回值:	数据长度
*******************************************************************************/
u32 reply_packet_buffer_data(struct sk_buff *skb, char * data_hdr, s8 *location)
{
	struct iphdr *iphdr = ip_hdr(skb);
	struct ethhdr *ethhdr = eth_hdr(skb);
	struct rtc_time tm;
	char date_gmt[35] = {0};
	struct timeval tv;
	u32 data_len;
	u8 apmac[13]={0};
	u8 stamac[13]={0};
	u8 staip[20]={0};
	//u8 apip[20]={0};
	u8 *staip_t = (u8 *)&iphdr->saddr;
	//u8 *apip_t;
	//struct net_device *dev;
	//dev = dev_get_by_name(&init_net, "eth0.2");
	//apip_t = (u8 *)&dev->ip_ptr->ifa_list->ifa_address;
	
	snprintf(apmac, 13, "%2x%2x%2x%2x%2x%2x", ethhdr->h_dest[0],ethhdr->h_dest[1],ethhdr->h_dest[2],
			ethhdr->h_dest[3],ethhdr->h_dest[4],ethhdr->h_dest[5]);
	snprintf(stamac, 13, "%2x%2x%2x%2x%2x%2x", ethhdr->h_source[0],ethhdr->h_source[1],ethhdr->h_source[2],
			ethhdr->h_source[3],ethhdr->h_source[4],ethhdr->h_source[5]);
	//snprintf(apip, 20, "%d.%d.%d.%d", apip_t[0], apip_t[1], apip_t[2], apip_t[3]);
	snprintf(staip, 20, "%d.%d.%d.%d", staip_t[0], staip_t[1], staip_t[2], staip_t[3]);
	
	do_gettimeofday(&tv);
	rtc_time_to_tm(tv.tv_sec,&tm);
	
	snprintf(date_gmt,35,"%s, %d %s %d %02d:%02d:%02d GMT",week[tm.tm_wday],tm.tm_mday,month[tm.tm_mon], tm.tm_year+1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
	WRITE_DATA_BUFFER(data_hdr, location, apmac, stamac, staip, data_len, date_gmt);
//	snprintf((data_hdr),HTTP_RESPONSE_PKT_LEN, "%s\r\nLocation: %s\r\n\r\n",HTTP_RESPONSE_HEAD, location);
	data_len = strlen(data_hdr);

	return data_len;
}

static u32 cloud_wlan_send_to_route(struct sk_buff *skb, struct iphdr *iph, struct tcphdr *tcphdr)
{
    struct rtable *rt = NULL;
    struct flowi4 fl4;
	int err;

	fl4.daddr = iph->daddr;
	fl4.flowi4_tos = RT_TOS(iph->tos);
	fl4.flowi4_oif = skb->dev->ifindex;

	err = ip_route_input_noref(skb, iph->daddr, iph->saddr,
					   iph->tos, skb->dev);
	if (unlikely(err))
	{
		CLOUD_WLAN_DEBUG("ip route input fail\n\n");
		kfree_skb(skb);
	    return CWLAN_FAIL;
	}
	
	rt = skb_rtable(skb);
	if (IS_ERR(rt))
	{
		CLOUD_WLAN_DEBUG("rt is error\n\n");
		kfree_skb(skb);
	    return CWLAN_FAIL;
    }
	err = dst_output(skb);
	CLOUD_WLAN_DEBUG("dst_output ret = %d!\n\n",err);

    return err;
}
void hexlog(unsigned char* str, int len) {
	unsigned char* p = str;
	int out_len = len;
	char x[17];
	int nn;
	int j;
	if (len <= 0)
		return;
	printk( "----------------hex dump start-----------------\n");
	for (nn = 0; nn < out_len; nn++) {
		printk( "%02X ", p[nn]);
		if ((p[nn] & 0xff) < 0x20 || (p[nn] & 0xff) >= 0x7f)
			x[nn % 16] = '.';
		else
			x[nn % 16] = p[nn];
		if (nn % 16 == 15 || nn == out_len - 1) {
			if (nn == out_len - 1 && nn % 16 != 15)
				for (j = 0; j < 16 - (nn % 16); j++)
					printk( "   ");
			printk( "\t\t %s\n", x);
			memset(x, 0, 17);
		}
	}
	printk( "\n----------------hex dump end-------------------\n");
}

/******************************************************************************* 
功能: http报文重定向接口函数
 -------------------------------------------------------------------------------
参数:	pdesc 原报文信息
		reHttp_pdesc新报文信息
		task_info接口信息
		iface
-------------------------------------------------------------------------------
返回值:	FLOW_ACTION_FORBID		丢包处理
			CWLAN_OK	重定向匹配成功
			FLOW_ACTION_PERMIT		透传处理
*******************************************************************************/
u32 reply_http_redirector(struct sk_buff *skb)
{
	struct iphdr *ipv4_hdr_t;
	struct tcphdr *tcp_hdr_t;
    struct ethhdr *eth_hdr_t;

	char *data_hdr_t;

	u32 eth_hdr_len = 0;
	u32 ipv4_hdr_len = 0;
	u32 tcp_hdr_len = 0;
	u32 data_len = 0;
	u32 pkt_len = 0;
	u32 ret;
	u8 packet[512]={0}; 
	struct sk_buff *skb_t;
	struct net_device *dev;
	struct iphdr *iphdr;
	struct tcphdr *tcphdr;
	struct ethhdr *ethhdr;

	ethhdr =  eth_hdr(skb);
	iphdr = ip_hdr(skb);
	tcphdr = tcp_hdr(skb);

	//memcpy(&quintuple_info->eth_hd, eth_hdr(skb), sizeof(struct ethhdr));
	eth_hdr_len = ethhdr->h_proto == htons(ETH_P_8021Q) ? ETH_HLEN + 4: ETH_HLEN;
	ipv4_hdr_len = iphdr->ihl *4;
	tcp_hdr_len = tcphdr->doff * 4;
	
	eth_hdr_t = (struct ethhdr *)(packet);
	ipv4_hdr_t = (struct iphdr *)(packet + eth_hdr_len);
	tcp_hdr_t = (struct tcphdr *)(packet + eth_hdr_len + ipv4_hdr_len);
	data_hdr_t = packet + eth_hdr_len + ipv4_hdr_len + tcp_hdr_len;


	reply_packet_ethhdr_data(ethhdr, eth_hdr_t);

	reply_packet_iphdr_data(iphdr, ipv4_hdr_t);

	ret = reply_packet_tcphdr_data(skb, tcphdr, tcp_hdr_t);
	if( ret == RE_PORTAL_DROP)
	{
		return CWLAN_FAIL;
	}
	else if( ret == RE_PORTAL_DATA)
	{//需要回的 数据包
		data_len = reply_packet_buffer_data(skb, data_hdr_t, g_portal_config.rehttp_conf.Location);
	}

	ipv4_hdr_t->tot_len = htons( ipv4_hdr_t->ihl * 4 + tcp_hdr_t->doff * 4 + data_len);

	ip_tcp_csum(ipv4_hdr_t, tcp_hdr_t, data_len);

	pkt_len = eth_hdr_len + ipv4_hdr_len + tcp_hdr_len + data_len;
	
	skb_t = alloc_skb(pkt_len, GFP_ATOMIC);
	if( skb_t == NULL)
	{
		CLOUD_WLAN_DEBUG("alloc_skb error!\n");
		return CWLAN_FAIL;
	}

	dev = dev_get_by_name(&init_net, "br-lan");
//	dev = dev_get_by_name(&init_net, "eth0.2");
	if (NULL == dev)
	{
		CLOUD_WLAN_DEBUG("get dev error!\n");
		return CWLAN_FAIL;
	}

	skb_t->dev = dev;
	dev_hold(skb_t->dev);
	skb_t->pkt_type = PACKET_HOST;
	skb_t->protocol = __constant_htons(ETH_P_IP);
	skb_t->ip_summed = CHECKSUM_NONE;
	skb_t->priority = 0;
	skb_t->mac_len = eth_hdr_len;

	//预留协议头部空间大小
	skb_reserve(skb_t, eth_hdr_len + ipv4_hdr_len + tcp_hdr_len);
	//向尾部扩展data room
	skb_put(skb_t, data_len);
	memcpy(skb_t->data, (void *)data_hdr_t, (s32)data_len);

	skb_push(skb_t, tcp_hdr_len);
	memcpy(skb_t->data, (void *)tcp_hdr_t, (s32)tcp_hdr_len);
	skb_reset_transport_header(skb_t);
	
	skb_push(skb_t, ipv4_hdr_len);
	memcpy(skb_t->data, (void *)ipv4_hdr_t, (s32)ipv4_hdr_len);
	skb_reset_network_header(skb_t);
	
	skb_push(skb_t, eth_hdr_len);
	memcpy(skb_t->data, (void *)eth_hdr_t, (s32)eth_hdr_len);
	skb_reset_mac_header(skb_t);
	skb_t->data = skb_t->data + eth_hdr_len;

	//hexlog(skb_t->head, pkt_len);

	ret = cloud_wlan_send_to_route(skb_t, ipv4_hdr_t, tcp_hdr_t);
	if(ret != CWLAN_OK)
	{
		CLOUD_WLAN_DEBUG("cloud_wlan_send_to_route error!\n");
		return CWLAN_FAIL;
	}

	return ret;
	
}

/******************************************************************************* 
功能: http报文重定向配置初始化
 -------------------------------------------------------------------------------
参数:	void
-------------------------------------------------------------------------------
返回值:	
*******************************************************************************/
u32 reply_http_redirector_init(void)
{
//目前由用户态下发初始化，为最后的配置
	g_portal_config.rehttp_conf.destIp[0] = 0;
/*端口没有使用*/
	g_portal_config.rehttp_conf.destPort = CW_LOCATION_PORT;
	
	memcpy((void *)g_portal_config.rehttp_conf.Location, CW_LOCATION_URL_DATA,1024);


	printk("cw init reply_http_redirector ok!\n");
	return CWLAN_OK;
}


