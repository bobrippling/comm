#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>

#define DEBUG 0

int lookup(const char *host, int port, struct sockaddr_in *addr)
{
	struct addrinfo *res = NULL;

	if(getaddrinfo(host,
				NULL /* service - uninitialised in ret */,
				NULL, &res))
		return 0;

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

	memcpy(addr, res->ai_addr, sizeof addr);
	addr->sin_port = htons(port);

	freeaddrinfo(res);
	return 1;
}

const char *addrtostr(struct sockaddr_in *ad)
{
#define BUFSIZ 32
	static char buf[BUFSIZ];
	return inet_ntop(AF_INET, &ad->sin_addr, buf, BUFSIZ);
#undef BUFSIZ
}
