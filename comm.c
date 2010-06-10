#include "headers.h"

#include <stdio.h>

#include <errno.h>
#include <sys/types.h>

#if !WINDOWS
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# define WIN_ERROR_BODGE()
#endif

#if WINDOWS
# include <winsock.h>
# define close(s)       closesocket(s)
# define ioctl(a, b, c) ioctlsocket(a, b, c)
# define WIN_ERROR_BODGE() do{ \
		switch(WSAGetLastError()){ \
			/*
			case WSANOTINITIALISED: errno = EOPNOTSUPP;  break; \
			case WSAECONNRESET:     errno = ECONNRESET;  break; \
			case WSAEINPROGRESS:    errno = EINPROGRESS; break; \
			case WSAENETDOWN:       errno = ENETDOWN;    break; \
			case WSAENOBUFS:        errno = ENOBUFS;     break; \
			case WSAENOTSOCK:       errno = ENOTSOCK;    break; \
			case WSAEWOULDBLOCK:    errno = EWOULDBLOCK; break;
			case WSAEOPNOTSUPP:     errno = EOPNOTSUPP;  break; */ \
			case WSAEFAULT:         errno = EFAULT;      break; \
			case WSAEINTR:          errno = EINTR;       break; \
			case WSAEINVAL:         errno = EINVAL;      break; \
			case WSAEMFILE:         errno = EMFILE;      break; \
			default:								errno = WSAGetLastError(); \
		} \
	}while(0)
#endif

#include <string.h>
#include <stdlib.h>

#include "comm.h"
#include "settings.h"


extern struct settings global_settings;

static int socket_nb(void);

static int socket_nb(void)
{
	int s = socket(PF_INET, SOCK_STREAM, 0);
	unsigned long mode = 1;

	if(s == -1){
		WIN_ERROR_BODGE();
		return -1;
	}

	ioctl(s, FIONBIO, &mode); /* non-blocking */

	return s;
}


int beginconnect(const char *host)
{
	int sock;
	/* the 0th STREAM protocol */
	struct sockaddr_in serv_addr;

	if((sock = socket_nb()) == -1)
		return 1;

	serv_addr.sin_family      = AF_INET;
	serv_addr.sin_port        = htons(global_settings.port);
	/*serv_addr.sin_addr.s_addr = INADDR_ANY;*/
	serv_addr.sin_addr.s_addr = inet_addr(host);
	memset(serv_addr.sin_zero, '\0', sizeof(serv_addr.sin_zero));

	if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) == -1){
		WIN_ERROR_BODGE();
		if(errno != EAGAIN)
			return 1;
	}

	return sock; /* currently connecting... */
}

int beginlisten(void)
{
	struct sockaddr_in serv;
	int sock = socket_nb();

	if(sock == -1)
		return -1;

	serv.sin_family = AF_INET;
	serv.sin_port = htons(global_settings.port);
	serv.sin_addr.s_addr = INADDR_ANY;
	memset(&serv.sin_zero, '\0', sizeof(serv.sin_zero));

	if(bind(sock, (struct sockaddr *)&serv, sizeof(struct sockaddr)) == -1){
		WIN_ERROR_BODGE();
		goto bail;
	}

	if(listen(sock, 3)){ /* n = backlog */
		WIN_ERROR_BODGE();
		goto bail;
	}

	return sock;
bail:
	if(sock != -1)
		close(sock);
	return -1;
}
