#ifndef _OP_EPOLL_H_
#define _OP_EPOLL_H_

#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CONNECT	    		128 
#define MAX_LISTEN_QUEUE		5

int start_server(char *ip, int port);
int op_read(int *fd, void * buf, ssize_t count);
int op_write(int *fd, const void *buf, size_t count);

void handle(void *clisockfd);

#endif