#include <sys/types.h> /* posix */
#if _WIN32
# include <winsock.h>
#else
# include <sys/socket.h>
# include <netdb.h> /* addrinfo */
# include <unistd.h>
#endif

#include <stdlib.h>
#include <string.h>

#include "socket.h"

#define DOMAIN AF_INET /* AF_INET6 */
#define TYPE SOCK_STREAM
#define PROTOCOL 0
#define LISTEN_BACKLOG 10
/* SOCK_DGRAM */

int createsocket(void)
{
	return socket(DOMAIN, TYPE, PROTOCOL);
}

int destroysocket(int i)
{
#ifdef WINDOW_H
	crabsa;
#else
	close(i); /* blocks is SO_LINGER is set */
#endif
	return 0;
}

/* returns socket on success, 0 on failure */
int tryconnect(char *address, char *service /* aka port, or "ssh" */)
{
	struct addrinfo hints, *rp, *result;
	int sfd;

	memset(&hints, '\0', sizeof(hints));
	hints.ai_family = AF_UNSPEC; /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0; /* Any protocol */

	if(getaddrinfo(address, service, &hints, &result))
		return 0;

	for(rp = result; rp; rp = rp->ai_next){
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(sfd == -1)
			continue;

		if(connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
			break;

		close(sfd);
	}

	freeaddrinfo(result);

	if(rp == NULL)
		return 0;

	return sfd;
}

int trylisten(char *service)
{
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int sfd;

	memset(&hints, '\0', sizeof(hints));
	hints.ai_family = AF_UNSPEC;	 /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
	hints.ai_flags = AI_PASSIVE;	 /* For wildcard IP address */
	hints.ai_protocol = 0;			 /* Any protocol */
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL; /* FIXME: allow listening on a certain IP (i.e. interface) */
	hints.ai_next = NULL;

	if(getaddrinfo(NULL, service, &hints, &result) != 0)
		return 0;

	for(rp = result; rp != NULL; rp = rp->ai_next){
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(sfd == -1)
			continue;

		if(!bind(sfd, rp->ai_addr, rp->ai_addrlen) &&
			 !listen(sfd, LISTEN_BACKLOG))
				break;


		close(sfd);
	}

	freeaddrinfo(result);

	if(rp == NULL)
		return 0;

	return sfd;
}
