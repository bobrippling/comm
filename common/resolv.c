#define _POSIX_SOURCE 1
#include <sys/types.h>

#ifdef _WIN32
# include <winsock2.h>
  /* getaddrinfo */
# include <ws2tcpip.h>
#else
# include <sys/socket.h>
# include <netdb.h>
# include <arpa/inet.h>
#endif

#include <string.h>

#include "resolv.h"

#define DEBUG 0

#if DEBUG
#include <stdio.h>
#endif

static int lookup_errno = 0;

const char *lookup_strerror()
{
	if(!lookup_errno)
		return NULL;
	return gai_strerror(lookup_errno);
}

int lookup(const char *host, const char *port, struct sockaddr_in *addr)
{
	struct addrinfo *res = NULL;
	struct addrinfo hints;
	int ret;

	memset(&hints, '\0', sizeof hints);
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if((ret = getaddrinfo(host, port,
				&hints, &res))){
		lookup_errno = ret;
		return 1;
	}

	/*if(!inet_pton(AF_INET, host, &addr.sin_addr))
		return false;*/

#if DEBUG
	{
		struct addrinfo *p = res;
		fprintf(stderr, "lookup for %s done\n", host);
		while(p){
			fprintf(stderr, "  %s\n", addrtostr((struct sockaddr_in *)p->ai_addr));
			p = p->ai_next;
		}
		fprintf(stderr, "done\n");
	}
#endif

	memcpy(addr, res->ai_addr, sizeof *addr);

	freeaddrinfo(res);

	lookup_errno = 0;
	return 0;
}

const char *addrtostr(struct sockaddr_in *ad)
{
	static char buf[32];
#ifdef _WIN32
	return getnameinfo((struct sockaddr *)ad,
			sizeof *ad,
			buf, sizeof buf,
			NULL, 0, /* no service */
			0 /* flags */);
#else
	return inet_ntop(AF_INET, &ad->sin_addr, buf, sizeof buf);
#endif
}
