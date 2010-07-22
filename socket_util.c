#define _POSIX_SOURCE 1
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>

#include "socket_util.h"

#define DEBUG 0

#if DEBUG
#include <stdio.h>
#endif

static int lookup_errno = 0;

const char *lookup_strerror()
{
	return gai_strerror(lookup_errno);
}

int lookup(const char *host, int port, struct sockaddr_in *addr)
{
	struct addrinfo *res = NULL;
	int ret;

	if((ret = getaddrinfo(host,
				NULL /* service - uninitialised in ret */,
				NULL, &res))){
		lookup_errno = ret;
		return 0;
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
	addr->sin_port = htons(port);

	freeaddrinfo(res);
	return 1;
}

const char *addrtostr(struct sockaddr_in *ad)
{
#define BUFSIZE 32
	static char buf[BUFSIZE];
	return inet_ntop(AF_INET, &ad->sin_addr, buf, BUFSIZE);
#undef BUFSIZE
}
