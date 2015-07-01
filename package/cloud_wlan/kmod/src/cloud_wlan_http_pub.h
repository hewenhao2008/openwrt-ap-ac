#ifndef HTTP_PUBLIC_H_
#define HTTP_PUBLIC_H_


#define ABS(x) ((x)?(x):(x))

#define SET_TMP_END(tmpc,tmpp,src) {(tmpc)=*(src);(tmpp)=(src);*(src)='\0';}
#define RESTOR_TMP_END(tmpc,tmpp) {*(tmpp)=(tmpc);}


#define HTTP_FRAGMENT_HASH_LEN 1024
#define HTTP_RESPONSE_PKT_LEN 1024
#define HTTP_HOST_LEN 256

typedef struct http
{
	s8 xnat;
	//s8 url[512];
	s8 host[HTTP_HOST_LEN];
}http_t;


enum  module_switch
{
    MODULE_SWITCH_OFF,          
    MODULE_SWITCH_ON,
    MODULE_SWITCH_MAX,
};



enum 
{
    RE_PORTAL_RESPONSE,          
    RE_PORTAL_DATA,
    RE_PORTAL_DROP,
};

typedef struct portal_config
{
	unsigned short module_switch; //重定向模块开关
	reHttp_t rehttp_conf;
}portal_conf_t;

#define HTTP_RESPONSE_HEAD	"HTTP/1.1 302 Moved Temporarily"
#define HTTP_SERVER			"Server: Apache-Coyote/1.1"
#define HTTP_LOCATION		"Location: "
#define HTTP_CONTENT_TYPE	"Content-Type: text/html;charset=UTF-8"
#define HTTP_CONTENT_LENTGH	"Content-Length: 0"
#define HTTP_DATE			"Date: "
#define HTTP_END			"\r\n"

extern portal_conf_t g_portal_config;

extern u32 is_http_get_pkt(void *data ,u32 skb_data_len);
extern u32 reply_http_redirector_init(void);
extern u32 reply_http_redirector(struct sk_buff *skb);
extern u32 cloud_wlan_http_skb_parse_request(struct sk_buff *skb, http_t *http_info);
extern u32 cloud_wlan_http_skb_parse_reply(u8* buf, http_t *http_info);

#endif

