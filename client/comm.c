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

#include "common.h"
#include "ui/ui.h"

#include "comm.h"
#include "../settings.h"

#define TIMEOUT 100
#define TO_SERVER_F(...) toserverf(sockf, __VA_ARGS__)

static enum commstate comm_state = VERSION_WAIT;
static int sock;
static FILE *sockf;
static const char *name;

static int sock_read(void);
static int gotdata(char *);
void sigh(int sig);
void cleanup(void);

enum commstate comm_getstate(void)
{
	return comm_state;
}

static int sock_read()
{
	char buffer[LINE_SIZE] = { 0 }, *newl;
	int ret;

	ret = recv(sock, buffer, LINE_SIZE, MSG_PEEK);

	if(ret == 0){
		/* disco */
		ui_info("server disconnect");
		return 1;
	}else if(ret < 0){
		ui_perror("recv()");
		return 1;
	}

	if((newl = strchr(buffer, '\n')) && newl < (buffer + ret)){
		/* got a newline, recieve _up_to_ that, only */
		/* assert(newret == oldret) */
		int len = newl - buffer + 1;

		if(len > LINE_SIZE)
			len = LINE_SIZE; /* prevent 1 byte sploitz */

		ret = recv(sock, buffer, len, 0);

		*newl = '\0';

		return gotdata(buffer);
	}else{
		/* need to wait for more, or buffer isn't big enough */
		if(ret == LINE_SIZE){
			/* need a larger buffer */
			ui_warning("error - buffer not big enough, discarding data");
		}else{
			/* save for later, i.e. don't recv() */
			;
		}
		return 0;
	}
}


/* expects a nul-terminated single line, with _no_ \n at the end */
static int gotdata(char *buffer)
{
#define UNKNOWN_MESSAGE(b) ui_warning("Unknown message from server: \"%s\"", b)
	if(!strncmp("ERR ", buffer, 4)){
		ui_warning("protocol error: %s", buffer + 4);
		return 0;
	}

	switch(comm_state){
		case ACCEPTED:
			/* normal message */
			if(!strncmp(buffer, "CLIENT_CONN ", 12))
				ui_info("new client connected to server: %s", buffer + 12);
			else if(!strncmp(buffer, "CLIENT_DISCO ", 13))
				ui_info("client disconnected from server: %s", buffer + 13);
			else if(!strncmp(buffer, "MESSAGE ", 8))
				ui_message("%s", buffer + 8);
			else if(!strcmp(buffer, "CLIENT_LIST"))
				/* TODO - mid-way through a client list */
				;
			else
				UNKNOWN_MESSAGE(buffer);
			break;

		case NAME_WAIT:
			if(!strcmp(buffer, "OK")){
				comm_state = ACCEPTED;
				ui_info("Name accepted, fire away");
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
					ui_info("Server Version OK: %d.%d, checking name...", maj, min);
					comm_state = NAME_WAIT;
					TO_SERVER_F("NAME %s", name);
				}else{
					ui_error("Server version mismatch: %d.%d", maj, min);
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

/* end of private funcs */

int comm_sendmessage(const char *msg, ...)
{
	va_list l;
	int ret;

	fprintf(sockf, "MESSAGE %s: ", name);

	va_start(l, msg);
	ret = toserver(sockf, msg, l);
	va_end(l);

	return ret;
}

void cleanup()
{
	ui_term();
	shutdown(sock, SHUT_RDWR);
	if(sockf)
		fclose(sockf);
	else
		close(sock);
}

void sigh(int sig)
{
	ui_warning("we get signal %d\n", sig);
	cleanup();
	exit(sig);
}

int comm_main(const char *nme, const char *host, int port)
{
	struct pollfd pollfds;
	int ret = 0;

	signal(SIGINT , &sigh);
	signal(SIGTERM, &sigh);
	signal(SIGQUIT, &sigh);

	if(ui_init())
		return 1;

	if(!host){
		ui_error("need host");
		return 1;
	}

	name = nme;

	if((sock = connectedsock(host, port)) == -1)
		return 1;

	sockf = fdopen(sock, "r+");
	if(!sockf){
		ui_perror("fdopen()");
		goto bail;
	}

	if(setvbuf(sockf, NULL, _IONBF, 0)){
		ui_perror("setvbuf()");
		goto bail;
	}

	ui_info("Connected to %s:%d, checking version...", host, port);

	memset(&pollfds, '\0', sizeof pollfds);
	pollfds.fd = sock;
	pollfds.events = POLLIN;

	for(; !ui_doevents();){
		int pret = poll(&pollfds, 1, TIMEOUT);

		if(pret == 0)
			continue;
		else if(pret < 0){
			if(errno == EINTR)
				continue;
			ui_perror("poll()");
			return 1;
		}

#define BIT(a, b) (((a) & (b)) == (b))
		if(BIT(pollfds.revents, POLLIN) && sock_read())
			goto bail;
	}

bail:
	cleanup();
	return ret;
}
