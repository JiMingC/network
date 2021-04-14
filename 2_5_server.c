#include "myhead.h"
#include "kernel_list.h"

//用户节点
typedef struct client
{
	int ID;
	int connfd;
	struct sockaddr_in addr;

	//用内核链表链接起来
	struct list_head list;

}listnode, *linklist;

//全局链表：
linklist head = NULL;

void usage(int argc, char **argv)
{
	if(argc != 2)
	{
		printf("Usage: %s <PORT>\n",argv[0]);
		exit(0);
	}
}

linklist init_list(void)
{
	//搞一个头结点
	linklist head = calloc(1, sizeof(listnode));
	if(head != NULL)
	{
		INIT_LIST_HEAD(&head->list);
	}

	return head;
}

//创建一个新的客户节点，保存他的信息
linklist new_client(int ID,int fd, struct sockaddr_in addr)
{
	linklist new_cli = calloc(1, sizeof(listnode));

	if(new_cli == NULL)
	{
		perror("calloc failed");
	}
	else
	{
		new_cli->ID = ID;
		new_cli->connfd = fd;
		new_cli->addr = addr;

		INIT_LIST_HEAD(&new_cli->list);
	}

	return new_cli;
}

bool private_talk(int receiverID, char *msg)
{
	struct list_head *pos;
	linklist p;

	//遍历用户链表，找到receiverID
	list_for_each(pos, &head->list)
	{
		p = list_entry(pos, listnode, list);

		if(p->ID == receiverID)
		{
			write(p->connfd, msg, strlen(msg));
			return true;
		}
	}

	return false;
}

void broadcast_talk(int senderID, char *msg)
{
	struct list_head *pos;
	linklist p;

	//遍历用户链表，跳出senderID
	list_for_each(pos, &head->list)
	{
		p = list_entry(pos, listnode, list);

		if(p->ID == senderID)
		{
			continue;
		}
		write(p->connfd, msg, strlen(msg));
	}

}


int main(int argc, char **argv)
{
	usage(argc,argv);

	//准备好TCP的监听套接字
	int fd = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	bzero(&addr,len);

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(atoi(argv[1]));

	bind(fd, (struct sockaddr *)&addr,len);
	listen(fd,3);

	//创建一个空的用户链表
	head = init_list();

	//不断接受客户端的连接
	//每个连接产生一条线程去管理
	struct sockaddr_in peeraddr;
	len = sizeof(peeraddr);

	//创建一个套接字集合，用来统一监听
	fd_set rset;
	int maxfd = fd;
	while(1)
	{
		//清零
		FD_ZERO(&rset);

		//将监听套接字，放进去
		FD_SET(fd,&rset);

		//遍历链表，将所有的连接套接字，放进去
		struct list_head *pos;
		linklist p;
		list_for_each(pos,&head->list)
		{
			p = list_entry(pos,listnode,list);
			FD_SET(p->connfd,&rset);

			maxfd = maxfd >p->connfd ? maxfd : p->connfd;
		}
		
		//利用select静静地等待任意一个或多个描述符就绪.
		select(maxfd+1, &rset,NULL,NULL,NULL);

		//有新的连接请求来了
		//判断是否有fd这个监听套接字在集合里面
		if(FD_ISSET(fd,&rset))
		{
			bzero(&peeraddr, len);
			int connfd= accept(fd,(struct sockaddr *)&peeraddr, &len);

			printf("welcome [%s:%hu]!\n",
					inet_ntoa(peeraddr.sin_addr),
					ntohs(peeraddr.sin_port));

			//给新用户产生一个新的ID，并告知客户端其ID
			int newID = rand() % 10000;
			char ID[6];
			snprintf(ID,6, "%d", newID);
			write(connfd,ID,strlen(ID));

			//将新客户端信息添加到客户链表中
			listnode *new_cli = new_client(newID,connfd,peeraddr);
			if(new_cli == NULL)
			{
				printf("新用户创建失败\n");
			}
			else
			{
				list_add_tail(&new_cli->list,&head->list);
			}
		}

		struct list_head *q;
		list_for_each_safe(pos,q,&head->list)
		{
			p=list_entry(pos,listnode,list);

			if(FD_ISSET(p->connfd,&rset))
			{
				char msg[100];
				int n;
				bzero(msg, 100);
				n = read(p->connfd, msg, 100);

				//客户端已经断开，或者发来要离开的消息
				if(n == 0 || n > 0 && !strncmp(msg, "quit",4))
				{
					list_del(pos);
					free(p);

					break;
				}

				//收到需要私聊的消息
				char *real_msg;
				if(real_msg = strstr(msg,":") )
				{
					if(private_talk(atoi(msg),real_msg+1))
					{
						send(p->connfd,"1",1,MSG_OOB);
					}
					else
					{
						send(p->connfd,"0",1,MSG_OOB);
					}
				}

				//收到需要群发的消息
				else
				{
						broadcast_talk(p->ID, msg);
				}

			}

		}

	}

	return 0;
}

