#ifndef CLOUD_WLAN_DES_H_
#define CLOUD_WLAN_DES_H_

#define DES_KEY_LEN 16
char g_des_key[DES_KEY_LEN];
enum{ENCRYPT,DECRYPT};

char DES_Act(char *Out,char *In,long datalen,const char *Key,int keylen,char Type);


#endif /* CLOUD_WLAN_DES_H_ */
