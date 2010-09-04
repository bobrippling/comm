#define _POSIX_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#include <poll.h>

#include "wrapper.h"
#include "comm.h"
#include "resolv.h"
#include "../config.h"

#define TO_SERVER_F(...) toserverf(ct->sockf, __VA_ARGS__)

#define CALLBACK(name) \
	void (name(enum callbacktype, const char *, ...))

static int comm_read(comm_t *, CALLBACK(callback));
static int comm_process(comm_t *ct, char *buffer, CALLBACK(callback));

static int comm_read(comm_t *ct, CALLBACK(callback))
{
	char buffer[LINE_SIZE] = { 0 }, *newl;
	int ret;

	ret = recv(ct->sock, buffer, LINE_SIZE, MSG_PEEK);

	if(ret == 0){
		/* disco */
		/* TODO: disco */
		return 1;
	}else if(ret < 0){
		ct->lasterr = strerror(errno);
		return 1;
	}

	if((newl = strchr(buffer, '\n')) && newl < (buffer + ret)){
		/* got a newline, recieve _up_to_ that, only */
		/* assert(newret == oldret) */
		int len = newl - buffer + 1;

		if(len > LINE_SIZE)
			len = LINE_SIZE;

		ret = recv(ct->sock, buffer, len, 0);

		*newl = '\0';

		return comm_process(ct, buffer, callback);
	}else{
		/* need to wait for more, or buffer isn't big enough */
		if(ret == LINE_SIZE){
			/* need a larger buffer */
			fputs("libcomm: error - buffer not big enough, discarding data\n", stderr);
		}else{
			;
		}
		return 0;
	}
}


/* expects a nul-terminated single line, with _no_ \n at the end */
static int comm_process(comm_t *ct, char *buffer, CALLBACK(callback))
{
#define UNKNOWN_MESSAGE(b) \
	fprintf(stderr, "libcomm: Unknown message from server: \"%s\"", b)

	if(!strncmp("ERR ", buffer, 4)){
		callback(COMM_ERR, "%s", buffer + 4);
		return 0;
	}

	switch(ct->state){
		case ACCEPTED:
			/* normal message */
			if(!strncmp(buffer, "CLIENT_CONN ", 12))
				callback(COMM_CLIENT_CONN, "%s", buffer + 12);

			else if(!strncmp(buffer, "CLIENT_DISCO ", 13))
				callback(COMM_CLIENT_DISCO, "%s", buffer + 13);

			else if(!strncmp(buffer, "MESSAGE ", 8))
				callback(COMM_MSG, "%s", buffer + 8);

			else if(!strncmp(buffer, "RENAME ", 7))
				callback(COMM_RENAME, "%s", buffer + 7);

			else if(!strncmp(buffer, "CLIENT_LIST", 11)){
				if(strcmp(buffer + 11, "_START") && strcmp(buffer + 11, "_END"))
					callback(COMM_CLIENT_LIST, "%s", buffer + 12);
				/* _START/_END are ignored FIXME: maintain list of clients */
			}else
				UNKNOWN_MESSAGE(buffer);
			break;

		case NAME_WAIT:
			if(!strcmp(buffer, "OK")){
				ct->state = ACCEPTED;
				callback(COMM_INFO, "%s", "Name accepted");
			}else{
				UNKNOWN_MESSAGE(buffer);
				return 1;
			}
			break;

		case VERSION_WAIT:
		{
			int maj, min;
			if(sscanf(buffer, "Comm v%d.%d", &maj, &min) == 2){
				if(maj == VERSION_MAJOR && min == VERSION_MINOR){
					callback(COMM_INFO, "Server Version OK: %d.%d, checking name...", maj, min);
					ct->state = NAME_WAIT;
					TO_SERVER_F("NAME %s", ct->name);
				}else{
					callback(COMM_ERR, "Server version mismatch: %d.%d", maj, min);
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

/* end of static */

enum commstate comm_state(comm_t *ct)
{
	return ct->state;
}

const char *comm_lasterr(comm_t *ct)
{
	if(ct->lasterr)
		return ct->lasterr;
	return "(no error)";
}

void comm_init(comm_t *ct)
{
	memset(ct, '\0', sizeof *ct);
	ct->state = VERSION_WAIT;
	ct->sock = -1;
}

int comm_connect(comm_t *ct, const char *host,
		int port, const char *name)
{
	ct->name = name;

	if((ct->sock = connectedsock(host, port)) == -1)
		goto bail_noclose;

	ct->sockf = fdopen(ct->sock, "r+");
	if(!ct->sockf)
		goto bail;

	if(setvbuf(ct->sockf, NULL, _IONBF, 0))
		goto bail;

	memset(&ct->pollfds, '\0', sizeof ct->pollfds);
	ct->pollfds.fd     = ct->sock;
	ct->pollfds.events = POLLIN;

	ct->state = VERSION_WAIT;

	return 0;
bail:
	if(ct->sockf)
		fclose(ct->sockf);
	else
		close(ct->sock);
bail_noclose:
	if(errno)
		ct->lasterr = strerror(errno);
	else if(!(ct->lasterr = lookup_strerror()))
		ct->lasterr = "(unknown error)";
	return 1;
}

void comm_close(comm_t *ct)
{
	shutdown(ct->sock, SHUT_RDWR);
	if(ct->sockf)
		fclose(ct->sockf);
	else
		close(ct->sock);

	comm_init(ct);
}

int comm_rename(comm_t *ct, const char *name)
{
	return fprintf(ct->sockf, "RENAME %s", name) <= 0;
}

int comm_sendmessage(comm_t *ct, const char *msg, ...)
{
	va_list l;
	int ret;

	if(fprintf(ct->sockf, "MESSAGE %s: ", ct->name) <= 0)
		return 1;

	va_start(l, msg);
	ret = toserver(ct->sockf, msg, l);
	va_end(l);

	return ret <= 0 ? 1 : 0;
}

int comm_recv(comm_t *ct, CALLBACK(callback))
{
	while(1){
		int pret = poll(&ct->pollfds, 1, CLIENT_SOCK_WAIT);

		if(pret == 0)
			/* nothing to declare */
			return 0;
		else if(pret < 0){
			if(errno == EINTR)
				continue;
			goto bail;
		}

#define BIT(i, mask) (((i) & (mask)) == (mask))

		if(BIT(ct->pollfds.revents, POLLIN) && comm_read(ct, callback))
			goto bail;
	}
	/* unreachable */

bail:
	{
		int save = errno;
		comm_close(ct);
		errno = save;
		ct->lasterr = strerror(errno);
		return 1;
	}
}
