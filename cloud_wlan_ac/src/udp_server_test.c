#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/socket.h>
#include <linux/in.h>
#include <errno.h>


#define 	BACKLOG			5
#define 	SERVER_IP 		"0.0.0.0"
#define 	SERVER_PORT 	22222
#define		BUFFLENTH		4096
#define     ERROUT          -1
#define     SUCCESS         0


int main(int argc, char** argv)
{
	int sendsize=0;
	int number=0;
	int recv_st=0;
	int sockfd = -1;
	struct sockaddr_in server_addr, client_addr;
    int sin_size = sizeof(struct sockaddr_in);
	char msg[BUFFLENTH] = {0};
	char buf[BUFFLENTH] = {0};
    
	/* 获取套接字描述符 */	
	if (-1 == (sockfd = socket(AF_INET, SOCK_DGRAM, 0)))        
	{
		printf("%d\n", errno);
		return ERROUT;
	}
    printf("Server:sockfd = %d\n",sockfd);

    /* 设置服务器端信息 */
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
	bzero(&(server_addr.sin_zero), 8);

	/* 指定一个套接字使用的端口 */
	if (-1 == bind (sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)))
	{
		printf("%d\n", errno);
		goto err_out;
	}
	printf("Server:Bind succeed!\n\n\n");

	while(1)
	{
		/* 接收客户机端请数据信息 */
		recv_st = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&client_addr, &sin_size); 
		if(-1 == recv_st)
		{
			printf("%d\n", errno);
			printf("-----------------------------\n\n\n");
			//goto err_out;
			continue;
		} 
		printf("recvBuf = %d\nlen = %d\n", *(int *)buf, strlen(buf));
		/* 显示通信连接基本信息*/
		printf("\n========== INFOMATION ===========\n");
		printf("Server:sockfd = %d\n",sockfd);
		printf("Server:server IP   : %s\n",inet_ntoa(server_addr.sin_addr));
		printf("Server:server PORT : %d\n",ntohs(server_addr.sin_port));
		printf("Server:client IP   : %s\n",inet_ntoa(client_addr.sin_addr));
		printf("Server:client PORT : %d\n\n",ntohs(client_addr.sin_port));

		/* 向客户机端发送数据信息 */
		snprintf(msg, BUFFLENTH, "Hello! UDP client [%d]!\n", number++);
		sendsize = 0;
		while( sendsize < strlen(msg) )                 /* 向服务器发送数据信息 */
		{
			sendsize = sendsize + sendto(sockfd, msg+sendsize, strlen(msg), 0, (struct sockaddr *)&client_addr, sizeof(struct sockaddr));
			/* 每次发送后数据指针向后移动 */
		};

		printf("-----------------------------\n\n\n");

	}

	err_out:
    /* 关闭套接字sockfd描述符 */
	close(sockfd);
    printf("Server:sockfd closed!\n");
	return SUCCESS;
}




