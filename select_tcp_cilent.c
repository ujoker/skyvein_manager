#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>
#include <time.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

#define MAXLINE 1024
#define IPADDRESS "127.0.0.1"
#define SERV_PORT 8787

#define max(a,b) (a > b) ? a : b

static void handle_recv_msg(int sockfd, char *buf) 
{
	printf("client recv msg is:%s\n", buf);
	//sleep(5);  //延时5秒
	write(sockfd, buf, strlen(buf) +1);
}

static void handle_connection(int sockfd)
{
	char buf[MAXLINE];
	int maxfdp,stdineof;
	fd_set readfds;
	int n;
	struct timeval tv;
	int retval = 0;

	while (1) {
        printf("Enter string to send:");  
        scanf("%s",buf);  
        if(!strcmp(buf,"quit"))  
            break;
		write(sockfd, buf, strlen(buf)+1);
		
		FD_ZERO(&readfds);
		FD_SET(sockfd,&readfds);
		maxfdp = sockfd;

		tv.tv_sec = 5;
		tv.tv_usec = 0;

		retval = select(maxfdp+1,&readfds,NULL,NULL,&tv);

		if (retval == -1) {
			return ;
		}

		if (retval == 0) {
			printf("client timeout.\n");
			continue;
		}

		if (FD_ISSET(sockfd, &readfds)) {
			n = read(sockfd,buf,MAXLINE);
			if (n <= 0) {
				fprintf(stderr,"client: server is closed.\n");
				close(sockfd);
				FD_CLR(sockfd,&readfds);
				return;
			}
			printf("client recv msg is:%s\n", buf);
			//handle_recv_msg(sockfd, buf);
		}
	}
}

int main(int argc,char *argv[])
{
	int sockfd;
	struct sockaddr_in servaddr;  //服务器端网络地址结构体

	sockfd = socket(AF_INET,SOCK_STREAM,0);

	bzero(&servaddr,sizeof(servaddr)); //数据初始化
	servaddr.sin_family = AF_INET;  //设置为IP通信
	servaddr.sin_port = htons(SERV_PORT);  //服务器端口号
	inet_pton(AF_INET,IPADDRESS,&servaddr.sin_addr);

	int retval = 0;
	retval = connect(sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr));
	if (retval < 0) {
		fprintf(stderr, "connect fail,error:%s\n", strerror(errno));
	return -1;
	}

	printf("client send to server .\n");
	//write(sockfd, "hello server", 32);

	handle_connection(sockfd);

	return 0;
}