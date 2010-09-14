#define _POSIX_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#ifdef _WIN32

	/* winsock.h has select()... wtf */
# include <winsock.h>
# include "../common/sockprintf.h"

# define TO_SERVER_F(...) sockprintf_newline(ct->sock, __VA_ARGS__)
# define CLOSE(ct) closesocket(ct->sock)

	static int wsastartup_called = 0;

#else
# include <sys/select.h>
# include <sys/socket.h>
# include <arpa/inet.h>

# define closesocket(s) close(s)
# define TO_SERVER_F(...) toserverf(ct->sockf, __VA_ARGS__)
# define CLOSE(ct) \
	if(ct->sockf) \
		fclose(ct->sockf); \
	else \
		closesocket(ct->sock);

#endif


#include "comm.h"

#include "../common/socketwrapper.h"
#include "../config.h"

static int comm_process(comm_t *ct, char *buffer, comm_callback callback);
static void comm_setlasterr(comm_t *);
#ifdef _WIN32
static void comm_setlasterr_WSA(comm_t *);
#endif

/* expects a nul-terminated single line, with _no_ \n at the end */
static int comm_process(comm_t *ct, char *buffer, comm_callback callback)
{
#define UNKNOWN_MESSAGE(b) \
	do{ \
		fprintf(stderr, "libcomm: Unknown message from server: \"%s\"\n", b); \
		ct->lasterr = "Invalid message for Comm Protocol"; \
	}while(0)

	if(!strncmp("ERR ", buffer, 4)){
		callback(COMM_ERR, "%s", buffer + 4);
		return 0;
	}

	switch(ct->state){
		case COMM_DISCONNECTED:
			fputs("libcomm: major logic error, you should never see this\n", stderr);
			break;

		case CONN_CONNECTING:
			/* TODO */
			break;

		case COMM_ACCEPTED:
			/* normal message */
			if(!strncmp(buffer, "MESSAGE ", 8))
				callback(COMM_MSG, "%s", buffer + 8);

			else if(!strncmp(buffer, "INFO ", 5))
				callback(COMM_SERVER_INFO, "%s", buffer + 5);

			else if(!strncmp(buffer, "CLIENT_DISCO ", 13))
				callback(COMM_CLIENT_DISCO, "%s", buffer + 13);

			else if(!strncmp(buffer, "CLIENT_CONN ", 12))
				callback(COMM_CLIENT_CONN, "%s", buffer + 12);

			else if(!strncmp(buffer, "RENAME ", 7))
				callback(COMM_RENAME, "%s", buffer + 7);

			else if(!strncmp(buffer, "CLIENT_LIST", 11)){
				if(strcmp(buffer + 11, "_START") && strcmp(buffer + 11, "_END"))
					callback(COMM_CLIENT_LIST, "%s", buffer + 12);
				/* _START/_END are ignored FIXME: maintain list of clients */

			}else
				UNKNOWN_MESSAGE(buffer);
			break;

		case COMM_NAME_WAIT:
			if(!strcmp(buffer, "OK")){
				ct->state = COMM_ACCEPTED;
				callback(COMM_INFO, "%s", "Name accepted");
			}else{
				UNKNOWN_MESSAGE(buffer);
				return 1;
			}
			break;

		case COMM_VERSION_WAIT:
		{
			int maj, min;
			char *desc_space = NULL;

			if(strlen(buffer) > 6){
				desc_space = strchr(buffer + 6, ' ');
				if(desc_space)
					*desc_space = '\0';
			}

			if(sscanf(buffer, "Comm v%d.%d", &maj, &min) == 2){
				if(maj == VERSION_MAJOR && min == VERSION_MINOR){
					callback(COMM_INFO, "%s Version OK: %d.%d, checking name...",
							desc_space ? desc_space + 1 : "Server",
							maj, min);
					ct->state = COMM_NAME_WAIT;
					TO_SERVER_F("NAME %s", ct->name);
				}else{
					callback(COMM_ERR, "Server version mismatch: %d.%d", maj, min);
					ct->lasterr = "Server version mismatch";
					return 1;
				}
			}else{
				UNKNOWN_MESSAGE(buffer);
				return 1;
			}
			break;
		}
	}

	return 0;
}

#ifdef _WIN32
static void comm_setlasterr_WSA(comm_t *ct)
{
#define BSIZ 256
	static char buffer[BSIZ];
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
			NULL, WSAGetLastError(), 0 /* lang */, buffer, BSIZ, NULL);

  ct->lasterr = buffer;
#undef BSIZ
}
#endif

static void comm_setlasterr(comm_t *ct)
{
#ifdef _WIN32
	if(WSAGetLastError())
			comm_setlasterr_WSA(ct);
	else
#endif
		ct->lasterr = strerror(errno);
}

/* end of static */

enum commstate comm_state(comm_t *ct)
{
	return ct->state;
}

const char *comm_lasterr(comm_t *ct)
{
	if(ct->lasterr)
		return ct->lasterr;
	return "(unknown error)";
}

void comm_init(comm_t *ct)
{
	memset(ct, '\0', sizeof *ct);
	ct->state = COMM_DISCONNECTED;
	ct->sock = -1;
}

int comm_connect(comm_t *ct, const char *host,
		const char *port, const char *name)
{
#ifdef _WIN32
	if(!wsastartup_called){
		WSADATA wsaBullshitWhyTheHellWouldIWantThisEverQuestionMark;
		int ret = WSAStartup(MAKEWORD(2, 2), &wsaBullshitWhyTheHellWouldIWantThisEverQuestionMark);

		wsastartup_called = 1;

		if(ret){
			fprintf(stderr, "libcomm: WSAStartup returned %d, WSALastError: %d\n",
					ret, WSAGetLastError());
			comm_setlasterr_WSA(ct);
			return 1;
		}
	}
#endif
	ct->name = malloc(strlen(name) + 1);
	if(!ct->name){
		ct->lasterr = strerror(errno);
		return 1;
	}
	strcpy(ct->name, name);

	if(!port)
		port = DEFAULT_PORT;

	/* TODO: async */
	if((ct->sock = connectedsock(host, port)) == -1)
		goto bail_noclose;

#ifndef _WIN32
	ct->sockf = fdopen(ct->sock, "r+");
	if(!ct->sockf)
		goto bail;

	if(setvbuf(ct->sockf, NULL, _IONBF, 0))
		goto bail;
#endif

	ct->state = COMM_VERSION_WAIT;

	return 0;
#ifndef _WIN32
bail:
	CLOSE(ct);
#endif
bail_noclose:
	if(!(ct->lasterr = lastsockerr()))
		comm_setlasterr(ct);
	return 1;
}

void comm_close(comm_t *ct)
{
#ifndef _WIN32
	shutdown(ct->sock, SHUT_RDWR);
#endif
	CLOSE(ct);
	free(ct->name);

	comm_init(ct);
}

#define COMM_SIMPLE(fname, proto) \
	int fname(comm_t *ct, const char *str) \
	{ \
		return TO_SERVER_F(proto " %s", str); \
	}

COMM_SIMPLE(comm_rename, "RENAME")
COMM_SIMPLE(comm_su,     "SU"    )
COMM_SIMPLE(comm_kick,   "KICK"  )

#undef COMM_SIMPLE

int comm_sendmessage(comm_t *ct, const char *msg, ...)
{
	va_list l;
	int ret;

	/*
	 * can't use TO_SERVER_F here because
	 * we need to send "MESSAGE %s" and the va_list
	 */
#ifdef _WIN32
	if(sockprintf(ct->sock, "MESSAGE %s: ", ct->name) < 0)
		return 1;

	va_start(l, msg);
	ret = sockprint_newline(ct->sock, msg, l);
	va_end(l);
#else
	if(fprintf(ct->sockf, "MESSAGE %s: ", ct->name) <= 0)
		return 1;

	va_start(l, msg);
	ret = toserver(ct->sockf, msg, l);
	va_end(l);
#endif
	return ret <= 0 ? 1 : 0;
}

int comm_recv(comm_t *ct, comm_callback callback)
{
	/*
	 * need to loop to process all the messages we've missed
	 * i.e. not just one message per comm_recv() call
	 */
	for(;;){
		fd_set set;
		struct timeval timeout;

		char buffer[LINE_SIZE] = { 0 };
		int ret;

		FD_ZERO(&set);
		FD_SET(ct->sock, &set);

		timeout.tv_sec = 0;
		timeout.tv_usec = 1000 /* 1 ms */;
		/*
		 * if we wait too long, on shitty OSs, like Windows,
		 * we get major GUI lag
		 */

		switch(select(FD_SETSIZE, &set, NULL, NULL, &timeout)){
			case 0:
				/* timeout */
				return 0; /* normal function exit point here */
			case -1:
				comm_setlasterr(ct);
				return 1;
		}

		/* got data */
		ret = recv(ct->sock, buffer, LINE_SIZE, MSG_PEEK);

		if(ret == 0){
			/* disco */
#ifdef _WIN32
closeconn:
#endif
			comm_close(ct);
			ct->lasterr = "server disconnect";
			return 1;
		}else if(ret < 0){
#ifdef _WIN32
			switch(errno){
				case WSAECONNRESET:
				case WSAECONNABORTED:
				case WSAESHUTDOWN:
					goto closeconn;
				/*case WSAEWOULDBLOCK:*/
			}
#endif
			comm_setlasterr(ct);
			return 1;
		}

		if(recv_newline(buffer, ret, ct->sock))
			/* not full */
			return 0;
		if(comm_process(ct, buffer, callback))
			return 1;

		/* continue, look for next message */
	}
}
