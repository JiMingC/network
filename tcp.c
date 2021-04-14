#include<stdio.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<string.h>
#include<unistd.h>
#include<netinet/in.h>
#include <arpa/inet.h>

#define LINE     10
#define DEBUG    1

int main()
{
	//创建socket套接字
	int serfd=0;
	serfd=socket(AF_INET,SOCK_STREAM,0);
	if(serfd<0)
	{
		perror("socket failed");
		return -1;
	}

#if DEBUG
	printf("socket ok!\n");
#endif

	//通过调用bind绑定IP地址和端口号
	int ret=0;
	struct sockaddr_in seraddr={0};
	seraddr.sin_family=AF_INET;
	seraddr.sin_port=htons(8888);
	//seraddr.sin_addr.s_addr=inet_addr("192.168.59.131");
	//seraddr.sin_addr.s_addr=inet_addr("150.109.115.216");
	//seraddr.sin_addr.s_addr=inet_addr("172.0.0.1");
	seraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	ret=bind(serfd,(struct sockaddr *)&seraddr,sizeof(seraddr));
	if(ret<0)
	{
		perror("bind failed");
		close(serfd);
		return -1;
	}

#if DEBUG
	printf("bind success\n");
#endif

	//通过调用listen将套接字设置为监听模式
	int lis=0;
	lis=listen(serfd,LINE);
	if(lis<0)
	{
		perror("listen failed");
		close(serfd);
		return -1;
	}

#if DEBUG
	printf("listen success\n");
#endif

	//服务器等待客户端连接中，游客户端连接时调用accept产生一个新的套接字
	int confd=0;
	socklen_t addrlen;
	struct sockaddr_in clientaddr={0};
	addrlen=sizeof(clientaddr);
	confd=accept(serfd,(struct sockaddr *)&clientaddr,&addrlen);
	if(confd<0)
	{
		perror("accept failed");
		close(serfd);
		return -1;
	}
	printf("connect success!\n");
	printf("ip=%s,port=%u\n",inet_ntoa(clientaddr.sin_addr),ntohs(clientaddr.sin_port));
	//调用recv接收客户端的消息
	while(1)
	{
		int rev=0;
		int sed=0;
		char buf[1024]={0};
		rev=recv(confd,buf,sizeof(buf),0);
		if(rev>0)
		{
			printf("本次收到了%d个字节\n",rev);
			printf("receive: %s\n",buf);
		}
	
		memset(buf,0,sizeof(buf));
		gets(buf);
		sed=send(confd,buf,strlen(buf),0);
		if(sed<0)
		{
			perror("send failed");
			close(serfd);
			return -1;
		}
		printf("send success\n");
	}
	close(confd);
	close(serfd);
	
	return 0;
}
