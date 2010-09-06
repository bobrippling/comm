#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "socketwrapper.h"
#include "resolv.h"

#include "../config.h"

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

int recv_newline(char *in, int /*recv*/ret, int fd)
{
	char *newl;

	if((newl = strchr(in, '\n')) && newl < (in + ret)){
		/* recv just up to the '\n' */
		int len = newl - in + 1;

		if(len > LINE_SIZE)
			len = LINE_SIZE;

		ret = recv(fd, in, len, 0);
		*newl = '\0';
	}else{
		if(ret == LINE_SIZE){
			fputs("comm error: running with data: LINE_SIZE exceeded\n", stderr);
			recv(fd, in, LINE_SIZE, 0);
		}else
			/* need more data */
			return 1;
	}
	return 0;
}
