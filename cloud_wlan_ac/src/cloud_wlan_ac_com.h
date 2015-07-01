#ifndef CLOUD_WLAN_AC_COM_H_
#define CLOUD_WLAN_AC_COM_H_


extern u32 cw_sendto_info(struct sockaddr *client_addr, s8* data, u32 len);
extern void hexlog(unsigned char* str, int len);


#endif
