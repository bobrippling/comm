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

	/*
	 * need to append the '\n', so that
	 * sockprintf() follows printf()
	 * and it can be called for MESSAGE
	 */
# define TO_SERVER_F(fmt, ...) sockprintf(ct->sock, fmt "\n", __VA_ARGS__)
# define CLOSE(ct) closesocket(ct->sock)

# define socklen_t int

	static int wsastartup_called = 0;
#else
# include <sys/select.h>
# include <sys/socket.h>
# include <arpa/inet.h>

# define TO_SERVER_F(fmt, ...) toserverf(ct->sockf, fmt, __VA_ARGS__)
# define CLOSE(ct) \
	if(ct->sockf) \
		fclose(ct->sockf); \
	else \
		close(ct->sock);

#endif

#define WARN(x, ...) fprintf(stderr, "libcomm: " x "\n", __VA_ARGS__)

#include "comm.h"

#include "../common/socketwrapper.h"
#include "../config.h"

static struct list *comm_findname(comm_t *ct, const char *name);

static int  comm_addname(   comm_t *ct, const char *name);
static int  comm_changename(comm_t *ct, const char *from, const char *to);
static int  comm_setcolour( comm_t *ct, const char *name, const char *col);
static void comm_removename(comm_t *ct, const char *name);
static void comm_freenames( comm_t *ct);
static int comm_udpsetup(   comm_t *ct, struct sockaddr *svraddr);

static int comm_process(comm_t *ct, char *buffer, comm_callback callback);
static void comm_setlasterr(comm_t *);
#ifdef _WIN32
static void comm_setlasterr_WSA(comm_t *);
static const char *win32_lasterr(void);
#endif

static int comm_addname(comm_t *ct, const char *name)
{
	struct list *l = malloc(sizeof(*l));

	if(!l || !(memset(l, '\0', sizeof *l), l->name = malloc(strlen(name)+1))){
		if(l)
			free(l->name);
		free(l);
		return 1;
	}

	strcpy(l->name, name);

	l->next = ct->clientlist;
	ct->clientlist = l;
	return 0;
}

static struct list *comm_findname(comm_t *ct, const char *name)
{
	struct list *l;
	for(l = ct->clientlist; l; l = l->next)
		if(!strcmp(l->name, name))
			return l;
	return NULL;
}

static int comm_changename(comm_t *ct, const char *from, const char *to)
{
	struct list *l;

	if(!strcmp(from, ct->name)){
		/* we're renaming ourselves */
		char *new = realloc(ct->name, strlen(to) + 1);
		if(!new)
			return 1;
		strcpy(ct->name = new, to);
		return 0;
	}

	l = comm_findname(ct, from);
	if(l){
		char *new = realloc(l->name, strlen(to) + 1);
		if(!new)
			return 1;
		strcpy(l->name = new, to);
		return 0;
	}

	WARN("Rename from %s to %s - not found in local list", from, to);
	return 0;
}

static int comm_setcolour(comm_t *ct, const char *name, const char *col)
{
	struct list *l = comm_findname(ct, name);
	if(l){
		char *new = realloc(l->col, strlen(col)+1);
		if(!new)
			return 1;
		strcpy(l->col = new, col);
	}else
		WARN("Set colour for %s (to %s) - not found in list", name, col);
	return 0;
}

static void comm_removename(comm_t *ct, const char *name)
{
	struct list *l, *prev = NULL;

	for(l = ct->clientlist; l; prev = l, l = l->next)
		if(!strcmp(l->name, name)){
			free(l->name);
			if(prev)
				prev->next = l->next;
			else
				ct->clientlist = l->next;
			free(l);
			return;
		}

	WARN("Couldn't remove %s from clientlist: not found", name);
}

static void comm_freenames(comm_t *ct)
{
	struct list *l, *m;

	l = ct->clientlist;
	while(l){
		m = l;
		l = l->next;
		free(m->name);
		free(m->col);
		free(m);
	}
	ct->clientlist = NULL;
}

/* expects a nul-terminated single line, with _no_ \n at the end */
static int comm_process(comm_t *ct, char *buffer, comm_callback callback)
{
#define UNKNOWN_MESSAGE(b) \
	do{ \
		WARN("Unknown message from server: \"%s\"", b); \
		ct->lasterr = "Invalid message for Comm Protocol"; \
	}while(0)

	if(!strncmp("ERR ", buffer, 4)){
		callback(COMM_ERR, "%s", buffer + 4);
		return 0;
	}

	switch(ct->state){
		case COMM_ACCEPTED:
			/* normal message */
			if(*buffer == 'D'){
				switch(buffer[1]){
					case 'D':
						callback(COMM_DRAW, buffer);
						break;

					case 'C':
						callback(COMM_DRAW_CLEAR, NULL);
						break;

					default:
						/* ignored */
						break;
				}

			}else if(!strncmp(buffer, "MESSAGE ", 8))
				callback(COMM_MSG, "%s", buffer + 8);

			else if(!strncmp(buffer, "TYPING ", 7)){
				char *data = buffer + 7;

				if(strlen(data) > 2){
					int typing = !!*data;
					char *who = data + 1;

					callback(COMM_TYPING, NULL);
				}

			}else if(!strncmp(buffer, "PRIVMSG ", 8))
				callback(COMM_PRIVMSG, "%s", buffer + 8);

			else if(!strncmp(buffer, "RENAME ", 7)){
				char *from, *to, *sep;

				sep = strchr(buffer + 7, GROUP_SEPARATOR);
				if(sep){
					*sep = '\0';
					from = buffer + 7;
					to   = sep + 1;

					comm_changename(ct, from, to);

					if(!strcmp(to, ct->name))
						callback(COMM_SELF_RENAME, NULL);
					else{
						/* updated list from comm_changename */
						callback(COMM_CLIENT_LIST, NULL);
						callback(COMM_RENAME, "%s to %s", from, to);
					}
				}else{
					UNKNOWN_MESSAGE(buffer);
				}

			}else if(!strncmp(buffer, "CLIENT_LIST", 11)){
				if(!strcmp(buffer + 11, "_START")){
					comm_freenames(ct);
					ct->listing = 1;
				}else if(!strcmp(buffer + 11, "_END")){
					ct->listing = 0;
					callback(COMM_CLIENT_LIST, NULL);
				}else if(ct->listing){
					char *sep = strchr(buffer + 12, GROUP_SEPARATOR);

					if(sep)
						*sep++ = '\0';

					if(comm_addname(ct, buffer + 12)){
						int save = errno;
						comm_close(ct);
						errno = save;
						comm_setlasterr(ct);
						callback(COMM_STATE_CHANGE, NULL);
						return 1;
					}

					if(sep)
						comm_setcolour(ct, buffer+12, sep);

				}else
					/* shouldn't get them out-of-_START-_END */
					UNKNOWN_MESSAGE(buffer);

			}else if(!strncmp(buffer, "INFO ", 5))
				callback(COMM_SERVER_INFO, "%s", buffer + 5);

			else if(!strncmp(buffer, "COLOUR ", 7)){
				char *sep = strchr(buffer, GROUP_SEPARATOR);

				if(sep){
					char *const name = buffer+7, *col = sep+1;
					*sep = '\0';

					comm_setcolour(ct, name, col);
				}else
					UNKNOWN_MESSAGE(buffer);

			}else if(!strncmp(buffer, "CLIENT_DISCO ", 13)){
				callback(COMM_CLIENT_DISCO, "%s", buffer + 13);
				comm_removename(ct, buffer+13);
				callback(COMM_CLIENT_LIST, NULL);

			}else if(!strncmp(buffer, "CLIENT_CONN ", 12)){
				callback(COMM_CLIENT_CONN, "%s", buffer + 12);
				if(comm_addname(ct, buffer+12)){
					int save = errno;
					comm_close(ct);
					errno = save;
					callback(COMM_STATE_CHANGE, NULL);
					return 1;
				}
				callback(COMM_CLIENT_LIST, NULL);

			}else
				UNKNOWN_MESSAGE(buffer);
			break;

		case COMM_NAME_WAIT:
			if(!strcmp(buffer, "OK")){
				ct->state = COMM_ACCEPTED;
				callback(COMM_INFO, "%s", "Name accepted");
				callback(COMM_STATE_CHANGE, NULL);
				return !TO_SERVER_F("%s", "CLIENT_LIST");
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
					int port;

					callback(COMM_INFO, "%s Version OK: %d.%d, checking name...",
							desc_space ? desc_space + 1 : "Server",
							maj, min);

					{
						struct sockaddr_in addr;
						socklen_t len = sizeof addr;
						if(getsockname(ct->udpsock, (struct sockaddr *)&addr, &len) == -1)
							port = 0;
						else
							port = ntohs(addr.sin_port);
					}

					TO_SERVER_F("ME %s%c%d", ct->name, GROUP_SEPARATOR, port);

					ct->state = COMM_NAME_WAIT;
					callback(COMM_STATE_CHANGE, NULL);
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

		case COMM_DISCONNECTED:
			WARN("%s", "major logic error, you should never see this");
			break;

		case COMM_CONNECTING:
			/* TODO */
			break;
	}

	return 0;
}

#ifdef _WIN32
static const char *win32_lasterr()
{
	static char buffer[256];
	char *nl;

	if(!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
			NULL, WSAGetLastError(), 0 /* lang */, buffer, sizeof buffer, NULL))
		return NULL;

	if((nl = strchr(buffer, '\n')))
		*nl = '\0';

	return buffer;
}

static void comm_setlasterr_WSA(comm_t *ct)
{
  ct->lasterr = win32_lasterr();
}
#endif

static void comm_setlasterr(comm_t *ct)
{
	fputs("comm_setlasterr()\n", stderr);
#ifdef _WIN32
	if(WSAGetLastError())
			comm_setlasterr_WSA(ct);
	else
#endif
		ct->lasterr = strerror(errno);
	fprintf(stderr, "lasterr: (%d) \"%s\"\n", errno, ct->lasterr);
}

/* end of static */

int comm_nclients(comm_t *ct)
{
	struct list *l;
	int i;
	for(i = 0, l = ct->clientlist; l; l = l->next, i++);
	return i; /* self excluded */
}

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

static int comm_udpsetup(comm_t *ct, struct sockaddr *svraddr)
{
	struct sockaddr_in addr;

	if((ct->udpsock = socket(PF_INET, SOCK_DGRAM, 0)) == -1)
		goto bail;

	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;

	if(bind(ct->udpsock, (struct sockaddr *)&addr, sizeof addr) == -1)
		goto bail;

	/* peer addr */
	if(connect(ct->udpsock, svraddr, sizeof(struct sockaddr_in /*bodge*/)) == -1)
		goto bail;

	return 0;
bail:
	comm_setlasterr(ct);
	return 1;
}

int comm_connect(comm_t *ct, const char *host,
		const char *port, const char *name)
{
	struct sockaddr svraddr;

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
	if(ct->name)
		free(ct->name);

	ct->name = malloc(strlen(name) + 1);
	if(!ct->name){
		ct->lasterr = strerror(errno);
		return 1;
	}
	strcpy(ct->name, name);

	if(!port)
		port = DEFAULT_PORT;

	/* TODO: async */
	if((ct->sock = connectedsock(host, port, &svraddr)) == -1)
		goto bail_noclose;

	if(comm_udpsetup(ct, &svraddr))
		goto bail;

#ifndef _WIN32
	ct->sockf = fdopen(ct->sock, "r+");
	if(!ct->sockf)
		goto bail;

	if(setvbuf(ct->sockf, NULL, _IONBF, 0))
		goto bail;
#endif

	ct->state = COMM_VERSION_WAIT;

	return 0;
bail:
#ifdef _WIN32
	fputs("conn_connect(): bail\n", stderr);
#endif
	CLOSE(ct);
bail_noclose:
#ifdef _WIN32
	{
		const char *tmp = win32_lasterr();

		fputs("conn_connect(): bail_noclose\n", stderr);
		if(tmp)
			fprintf(stderr, "WSAGetLastError(): \"%s\"\n", tmp);
	}
	comm_setlasterr(ct);
#else
	if(!(ct->lasterr = lastsockerr()))
		comm_setlasterr(ct);
#endif
	return 1;
}

void comm_close(comm_t *ct)
{
#ifndef _WIN32
	shutdown(ct->sock, SHUT_RDWR);
#endif
	CLOSE(ct);
	comm_freenames(ct);
	free(ct->name);
	free(ct->col);

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

int comm_typing(comm_t *ct, int b)
{
	return TO_SERVER_F("TYPING %d", b);
}

int comm_privmsg(comm_t *ct, const char *name, const char *msg)
{
	return TO_SERVER_F("PRIVMSG %s%c%s -> %s: %s",
			name, GROUP_SEPARATOR, ct->name, name, msg);
}

int comm_colour(comm_t *ct, const char *col)
{
	char *new = realloc(ct->col, strlen(col) + 1);

	if(!new)
		return 1;

	strcpy(ct->col = new, col);

	return TO_SERVER_F("COLOUR %s", col);
}

const char *comm_getname(comm_t *ct)
{
	return ct->name;
}

struct list *comm_clientlist(comm_t *ct)
{
	return ct->clientlist;
}

const char *comm_getcolour(comm_t *ct, const char *name)
{
	if(strcmp(name, ct->name)){
		struct list *l = comm_findname(ct, name);
		if(!l)
			return NULL;
		return l->col;
	}else
		return ct->col;
}

int comm_sendmessage(comm_t *ct, const char *msg)
{
	return TO_SERVER_F("MESSAGE %s: %s", ct->name, msg);
}

int comm_sendmessagef(comm_t *ct, const char *msg, ...)
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
	ret = sockprint(ct->sock, msg, l);
	va_end(l);

	if(ret < 0)
		return 1;

	ret = sockprintf(ct->sock, "\n");
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

		FD_ZERO(&set);
		FD_SET(ct->sock,    &set);
		FD_SET(ct->udpsock, &set);

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
		if(FD_ISSET(ct->sock, &set)){
			char buffer[MAX_LINE_LEN] = { 0 };
			int ret = recv(ct->sock, buffer, MAX_LINE_LEN, MSG_PEEK);

			if(ret == 0){
				/* disco */
#ifdef _WIN32
	closeconn:
#endif
				comm_close(ct);
				callback(COMM_STATE_CHANGE, NULL);
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

			if(recv_newline(buffer, ret, ct->sock, LIBCOMM_RECV_TIMEOUT))
				/* not full */
				return 0;
			if(comm_process(ct, buffer, callback))
				return 1;
		}

		if(FD_ISSET(ct->udpsock, &set)){
			char buffer[MAX_UDP_PACKET];
			int ret = recvfrom(ct->udpsock, buffer, sizeof buffer - 1, 0, NULL, NULL);

			buffer[ret] = '\0';

			if(ret <= 0)
				fprintf(stderr, "comm_recv(): udp recvfrom(): %s\n", strerror(errno));
			else if(comm_process(ct, buffer, callback))
				return 1;
		}
		/* continue, look for next message */
	}
}

int comm_draw(comm_t *ct, int x, int y, int x2, int y2, int colour)
{
	/* TODO: colour */
	char buffer[MAX_UDP_PACKET];
	int len;

	len = snprintf(buffer, sizeof buffer,
			"%s%cDD%d_%d_%d_%d",
			ct->name, GROUP_SEPARATOR, x, y, x2, y2);

	return sendto(ct->udpsock, buffer, len, 0, NULL, 0);
}

int comm_draw_clear(comm_t *ct)
{
	char buffer[MAX_UDP_PACKET];
	int len;

	len = snprintf(buffer, sizeof buffer,
			"%s%cDC", ct->name, GROUP_SEPARATOR);

	return sendto(ct->udpsock, buffer, len, 0, NULL, 0);
}

void comm_getdrawdata(const char *s, int *x1, int *y1, int *x2, int *y2)
{
	sscanf(s, "%d_%d_%d_%d", x1, y1, x2, y2);
}
