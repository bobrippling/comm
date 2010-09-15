#define _POSIX_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>

#include <sys/types.h>
#if _WIN32
# define _WIN32_WINNT 0x501
# include <winsock2.h>
# include <ws2tcpip.h>
/* ^ getaddrinfo */
#else
# include <sys/socket.h>
# include <arpa/inet.h>
# include <netdb.h>
#endif

#include "socketwrapper.h"

#include "../config.h"

static int lookup_errno = 0;

const char *lastsockerr()
{
	if(lookup_errno)
		return gai_strerror(lookup_errno);
	return errno ? strerror(errno) : NULL;
	/* libcomm handles NULL and converts to a WSA_... error instead */
}

int connectedsock(const char *host, const char *port)
{
	struct addrinfo hints, *ret = NULL, *dest = NULL;
	int sock = -1, lastconnerr = 0;

	memset(&hints, '\0', sizeof hints);
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if((lookup_errno = getaddrinfo(host, port, &hints, &ret)))
		return -1;
	lookup_errno = 0;


	for(dest = ret; dest; dest = dest->ai_next){
		sock = socket(dest->ai_family, dest->ai_socktype,
				dest->ai_protocol);

		if(sock == -1)
			continue;

		if(connect(sock, dest->ai_addr, dest->ai_addrlen) == 0)
			break;

		if(errno)
			lastconnerr = errno;
		close(sock);
		sock = -1;
	}

	freeaddrinfo(ret);

	if(sock == -1){
		errno = lastconnerr;
		return -1;
	}

	return sock;
}

const char *addrtostr(struct sockaddr *ad)
{
	static char buf[128];

	if(getnameinfo(ad,
			sizeof *ad,
			buf, sizeof buf,
			NULL, 0, /* no service */
			0 /* flags */))
		return NULL;
	return buf;
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
