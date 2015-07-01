#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

typedef struct dcma_udp_info
{
	int type;	//?üá?ààDí
	int number;	//data ??êy
	char data[];	
}dcma_udp_skb_info_t;

typedef struct cwlan_cmd_sockt
{
	unsigned char apmac[6];
	int size;
	char data[0];
}cwlan_cmd_sockt_t;

void hexlog(unsigned char* str, int len) {
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
					printf( "   ");
			printf( "\t\t %s\n", x);
			memset(x, 0, 17);
		}
	}
	printf( "\n----------------hex dump end-------------------\n");
}


int main(int argc, char *argv[])
{

int sock;
//sendto中使用的对方地址
struct sockaddr_in toAddr;
//在recvfrom中使用的对方主机地址
struct sockaddr_in fromAddr;
unsigned int fromLen;
char recvBuffer[128];
char cmdbuff[512]={0};

	dcma_udp_skb_info_t *acbuf;
	dcma_udp_skb_info_t *apbuf;
	cwlan_cmd_sockt_t *cmd;
	int cmd_len;

	acbuf = (dcma_udp_skb_info_t *)cmdbuff;
	acbuf->type = 0x0FF00;
	acbuf->number = 1;
	cmd = (cwlan_cmd_sockt_t *)(cmdbuff+sizeof(dcma_udp_skb_info_t));
	cmd_len = sizeof(dcma_udp_skb_info_t);

	cmd->apmac[0]=0x00;
	cmd->apmac[1]=0x0c;
	cmd->apmac[2]=0x29;
	cmd->apmac[3]=0x11;
	cmd->apmac[4]=0xfd;
	cmd->apmac[5]=0x56;
	cmd_len = cmd_len + sizeof(cwlan_cmd_sockt_t);
	apbuf = (cwlan_cmd_sockt_t *)(cmdbuff+cmd_len);
	apbuf->type = 3;
	apbuf->number = 1;
	
	cmd->size = sizeof(dcma_udp_skb_info_t);
	cmd_len = cmd_len + cmd->size ;



	hexlog((unsigned char *)cmdbuff, cmd_len);


sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
if(sock < 0)
{
 printf("创建套接字失败了.\r\n");
 exit(1);
}
memset(&toAddr,0,sizeof(toAddr));
toAddr.sin_family=AF_INET;
toAddr.sin_addr.s_addr=inet_addr("127.0.0.1");

toAddr.sin_port = htons(22222);


printf("%d\n", cmd_len);
if(sendto(sock, cmdbuff, cmd_len,0,(struct sockaddr*)&toAddr,sizeof(toAddr)) != cmd_len)
{
 printf("sendto() 函数使用失败了.\r\n");
 close(sock);
 exit(1);
}

return 0;
fromLen = sizeof(fromAddr);
if(recvfrom(sock,recvBuffer,128,0,(struct sockaddr*)&fromAddr,&fromLen)<0)
{
 printf("()recvfrom()函数使用失败了.\r\n");
 close(sock);
 exit(1);
}
printf("recvfrom() result:%s\r\n",recvBuffer);
close(sock);
}