#include "op_epoll.h"
#include "threadpool.h"

threadpool_t *epoll_pool;
int epollfd;

static void set_nonblocking(int sock)
{
	int opts;
	opts = fcntl(sock, F_GETFL);

	if(opts < 0) {
		perror("fcntl(sock, GETFL)");
		exit(1);
	}

	opts = opts | O_NONBLOCK;

	if(fcntl(sock, F_SETFL, opts) < 0) {
		perror("fcntl(sock, SETFL, opts)");
		exit(1);
	}
}

static void op_add_event(int *fd)
{
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = *fd;
	epoll_ctl(epollfd,EPOLL_CTL_ADD,*fd,&ev);
}

static void op_delete_event(int *fd)
{
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = *fd;
	epoll_ctl(epollfd,EPOLL_CTL_DEL,*fd,&ev);
}

static void op_modify_event(int *fd)
{
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = *fd;
	epoll_ctl(epollfd,EPOLL_CTL_MOD,*fd,&ev);
}

static int init_servsock(char * ip, int port)
{
	int listenfd;
	int on = 1;
	struct sockaddr_in serveraddr;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	set_nonblocking(listenfd);

	setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(int));
	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	inet_aton(ip, &(serveraddr.sin_addr));
	serveraddr.sin_port = htons(port);

	int ret = bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
	if(ret < 0)
	{
		printf("%s\n", "bind error!");
		exit(-1);
	}

	ret = listen(listenfd, MAX_LISTEN_QUEUE);	
	if(ret<0)
	{
		printf("errno code is %d\n", errno);
	}

	return listenfd;
}

int op_read(int *fd, void * buf, ssize_t count)
{
	int n = read((*fd), buf, count);
	if(n < 0 && errno == EAGAIN)
	{
		printf("%s\n", "Data has been read, no more data can be read.");
		return -1;
	}
	else if(n == 0)
	{
		printf("%s, fd is %d\n", "Client disconnect.", (*fd));
		op_delete_event(fd);
		return -2;
	}
	else if(n < 0)
	{
		printf("Read error. Error code is %d\n", errno);
		op_delete_event(fd);
		return -3;
	}
	else
	{
		return n;
	}
}

int op_write(int *fd, const void *buf, size_t count)
{
	int n = write((*fd), buf, count);
	if(n < 0 && errno == EAGAIN)
	{
		printf("%s\n", "System buffer data has been written.");
		return -1;
	}
	else if(n == 0)
	{
		printf("%s,fd is %d\n", "Client disconnect.", (*fd));
		op_delete_event(fd);
		return -2;
	}					
	else if(n < 0)
	{					
		printf("Read error. Error code is %d\n", errno);
		op_delete_event(fd);
		return -3;	
	}
	else
	{
		return n;
	}
}

int start_server(char *ip, int port)
{	
	int i, listenfd, connfd, sockfd, nfds;
	ssize_t n;	
	struct sockaddr_in clientaddr;
	
	socklen_t clilen;

	struct epoll_event ev, events[MAX_CONNECT];

	printf("bind : \"ip\":%s,\"port\":%d\n", ip, port);
	
	epoll_pool = threadpool_create(32, MAX_CONNECT-32, 0);
	if(!epoll_pool)
	{
		printf("%s\n", "threadpool init error!");
		exit(1);
	}
	listenfd = init_servsock(ip, port);	
	epollfd = epoll_create(MAX_CONNECT-1);
	ev.data.fd = listenfd;
	ev.events = EPOLLIN | EPOLLET;

	epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev);

	for(; ;) 
	{
		nfds = epoll_wait(epollfd, events, MAX_CONNECT, -1);

		for(i = 0; i < nfds; ++i) 
		{
			if(events[i].data.fd == listenfd) {
				
				connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clilen);				
				if(connfd < 0) {
					perror("connfd < 0");
					exit(1);
				}				
				set_nonblocking(connfd);						
				printf("accept connect from %s, fd is %d\n", inet_ntoa(clientaddr.sin_addr), connfd);
				ev.data.fd = connfd;
				ev.events = EPOLLIN | EPOLLET;

				epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev);
			}
			else if(events[i].events & EPOLLIN) 
			{						
				threadpool_add(epoll_pool, &handle, &events[i].data.fd, 0);
			}
			else if(events[i].events & EPOLLOUT) 
			{

			}
		}
	}
}
