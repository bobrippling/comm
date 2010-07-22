#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "../socket_util.h"

#include "common.h"

int connectedsock(char *host, int port)
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in addr;

	if(fd == -1){
		perror("socket()");
		return -1;
	}

	memset(&addr, '\0', sizeof addr);

	addr.sin_family = AF_INET;
	if(!lookup(host, port, &addr)){
		fprintf(stderr, "couldn't lookup %s: %s\n", host, lookup_strerror());
		close(fd);
		return -1;
	}

	if(connect(fd, (struct sockaddr *)&addr, sizeof addr) == -1){
		perror("connect()");
		return -1;
	}

	return fd;
}

int toserverf(int fd, const char *fmt, ...)
{
#define BSIZ 4096
	static char buffer[BSIZ];

	va_list l;
	int ret;

	va_start(l, fmt);
	ret = vsnprintf(buffer, BSIZ, fmt, l);
	va_end(l);

	if(ret == -1)
		return -1;
	else if(ret >= BSIZ)
		buffer[BSIZ-1] = '\0';

	return write(fd, buffer, strlen(buffer));
#undef BSIZ
}
