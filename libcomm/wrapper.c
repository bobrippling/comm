#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "resolv.h"

#include "wrapper.h"

int connectedsock(const char *host, int port)
{
	int fd = socket(PF_INET, SOCK_STREAM, 0);
	struct sockaddr_in addr;

	if(fd == -1)
		return -1;

	memset(&addr, '\0', sizeof addr);

	addr.sin_family = AF_INET;
	if(!lookup(host, port, &addr))
		goto bail;

	if(connect(fd, (struct sockaddr *)&addr, sizeof addr) == -1)
		goto bail;

	return fd;
bail:
	{
		int save = errno;
		close(fd);
		errno = save;
		return -1;
	}
}

int toserverf(FILE *f, const char *fmt, ...)
{
	va_list l;
	int ret;

	va_start(l, fmt);
	ret = toserver(f, fmt, l);
	va_end(l);
	return ret;
}

int toserver(FILE *f, const char *fmt, va_list l)
{
	static const char nl = '\n';
	int ret;

	ret = vfprintf(f, fmt, l);

	if(ret == -1)
		return -1;

	return fwrite(&nl, sizeof nl, 1, f);
}

void perrorf(const char *fmt, ...)
{
	if(fmt){
		va_list l;
		va_start(l, fmt);
		vfprintf(stderr, fmt, l);
		va_end(l);
	}
	perror(NULL);
}
