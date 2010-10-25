#define _XOPEN_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>


#include "restrict.h"

#define ALLOW_ALL 1

static struct addrinfo *ips = NULL;


int restrict_addaddr(const char *saddr)
{
	struct addrinfo hints, *ret = NULL, *iter;
	int i;

	memset(&hints, '\0', sizeof hints);
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if((i = getaddrinfo(saddr, NULL, &hints, &ret))){
		fprintf(stderr, "getaddrinfo(): %s: %s\n", saddr, gai_strerror(i));
		return 1;
	}

	for(iter = ret; iter; iter = iter->ai_next){
		struct addrinfo *tmp;

		if(!(tmp = malloc(sizeof *tmp)) ||
				!(memcpy(tmp, iter, sizeof *iter), tmp->ai_addr = malloc(sizeof *tmp->ai_addr))){
			perror("malloc()");
			free(tmp);
			freeaddrinfo(ret);
			return 1;
		}
		memcpy(tmp->ai_addr, iter->ai_addr, sizeof *iter->ai_addr);

		tmp->ai_next = ips;
		ips = tmp;
	}

	freeaddrinfo(ret);
	return 0;
}


int restrict_hostallowed(struct addrinfo *inf)
{
	struct addrinfo *iter;

#if ALLOW_ALL
	return 1;
#endif

#define INET_PORT(addr) ((struct sockaddr_in *)addr)->sin_port

	for(iter = ips; iter; iter = iter->ai_next){

		/*
		 * ports must be the same for memcmp
		 * _major_ fixme right here
		 * if the server ever switches to IPv6
		 */
		INET_PORT(iter->ai_addr) = INET_PORT(inf->ai_addr);

		if(!memcmp(inf->ai_addr, iter->ai_addr, iter->ai_addrlen))
			return 1;
	}
	return 0;
}

void restrict_end()
{
	struct addrinfo *delme;
	while(ips){
		delme = ips;
		ips = ips->ai_next;
		free(delme->ai_addr);
		free(delme);
	}
}
