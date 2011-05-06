#define _XOPEN_SOURCE 500
#define _POSIX_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>

#include <sys/types.h>
#if _WIN32
# define _WIN32_WINNT 0x600
/* ws2tcpip must be first... enterprise */
# include <ws2tcpip.h>
# include <winsock2.h>
/* ^ getaddrinfo */
#else
# include <sys/socket.h>
# include <arpa/inet.h>
# include <netdb.h>
#endif

#include "socketwrapper.h"

#include "../config.h"

#ifdef _WIN32
# define DEBUG 1
#else
# define DEBUG 0
#endif

static int lookup_errno = 0;

const char *lastsockerr()
{
	if(lookup_errno)
		return gai_strerror(lookup_errno);
	return errno ? strerror(errno) : NULL;
	/* libcomm handles NULL and converts to a WSA_... error instead */
}

int connectedsock(const char *host, const char *port, struct sockaddr *peeraddr)
{
	struct addrinfo hints, *ret = NULL, *dest = NULL;
	int sock = -1, lastconnerr = 0;

	memset(&hints, '\0', sizeof hints);
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if((lookup_errno = getaddrinfo(host, port, &hints, &ret))){
#if DEBUG
		fprintf(stderr, "connectedsock(): getaddrinfo failed: (%d) \"%s\"\n",
				lookup_errno, lastsockerr());
#endif
		return -1;
	}
	lookup_errno = 0;


	for(dest = ret; dest; dest = dest->ai_next){
		sock = socket(dest->ai_family, dest->ai_socktype,
				dest->ai_protocol);

		if(sock == -1){
#if DEBUG
			fputs("connectedsock(): socket() failed\n", stderr);
#endif
			continue;
		}

		if(connect(sock, dest->ai_addr, dest->ai_addrlen) == 0){
			if(peeraddr)
				memcpy(peeraddr, dest->ai_addr, sizeof *peeraddr);
			break;
		}

		if(errno)
#if DEBUG
			fprintf(stderr, "connectedsock(): connect() failed: (%d) \"%s\"\n",
					errno, strerror(errno));
#endif
			lastconnerr = errno;
		close(sock);
		sock = -1;
	}

	freeaddrinfo(ret);

	if(sock == -1){
		errno = lastconnerr;
#if DEBUG
		fputs("connectedsock(): no successful connections\n", stderr);
#endif
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

int recv_newline(char *in, int /*recv*/ret, int fd, int sleepms)
{
	char *newl;

	if((newl = memchr(in, '\n', ret)) && newl < (in + ret)){
		/* recv just up to the '\n' */
		int len = newl - in + 1;

		if(len > MAX_LINE_LEN)
			len = MAX_LINE_LEN;

		ret = recv(fd, in, len, 0);
		*newl = '\0';
	}else{
		if(ret == MAX_LINE_LEN){
			fputs("comm error: running with data: MAX_LINE_LEN exceeded\n", stderr);
			recv(fd, in, MAX_LINE_LEN, 0);
		}else{
			/* need more data */
#if DEBUG
			fprintf(stderr, "socketwrapper::recv_newline(fd = %d): "
					"no newline, sleeping...\n", fd);
#endif
			usleep(sleepms * 1000);
			return 1;
		}
	}
	return 0;
}
