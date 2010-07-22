#define _POSIX_SOURCE 1
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#include <poll.h>

#include "../../settings.h"
#include "../../socket_util.h"
#include "../common.h"

#include "ui.h"

#define TIMEOUT 100
#define TO_SERVER_F(...) toserverf(sock, __VA_ARGS__)

static enum state { VERSION_WAIT, NAME_WAIT, ACCEPTED } state = VERSION_WAIT;
static int sock;
static FILE *sockf;
static const char *name;

void showclients(void);

int sock_read(void);
int stdin_read(void);
int gotdata(char *);

void showclients()
{
	TO_SERVER_F("CLIENT_LIST");
	/*
	CLIENT_LIST_START
	CLIENT_LIST %s
	CLIENT_LIST_END
	*/
}

/* ----------- */


int sock_read()
{
	static char buffer[LINE_SIZE] = { 0 };
	char *nul;
	int ret;

	ret = recv(sock, buffer, LINE_SIZE, MSG_PEEK);

	if(ret == 0){
		/* disco */
		puts("server disconnect");
		return 1;
	}else if(ret < 0){
		perror("recv()");
		return 1;
	}

	if((nul = strchr(buffer, '\0')) && nul < (buffer + ret)){
		/* got a newline, recieve _up_to_ that, only */
		/* assert(newret == oldret) */
		int len = nul - buffer + 1;

		if(len > LINE_SIZE)
			len = LINE_SIZE; /* prevent 1 byte sploitz */

		ret = recv(sock, buffer, len, 0);

		return gotdata(buffer);
	}else{
		/* need to wait for more, or buffer isn't big enough */
		if(ret == LINE_SIZE){
			/* need a larger buffer */
			fputs("error - buffer not big enough, discarding data\n", stderr);
		}else{
			/* save for later, i.e. don't recv() */
		}
		return 0;
	}
}


int gotdata(char *buffer)
{
#define UNKNOWN_MESSAGE(b) fprintf(stderr, "Unknown message from server: \"%s\"\n", b)
	if(!strncmp("ERR ", buffer, 4)){
		printf("protocol error: %s\n", buffer + 4);
		return 0;
	}

	switch(state){
		case ACCEPTED:
			/* normal message */
			if(!strncmp(buffer, "CLIENT_CONN ", 12))
				printf("new client connected to server: %s\n", buffer + 12);
			else if(!strncmp(buffer, "CLIENT_DISCO ", 13))
				printf("client disconnected from server: %s\n", buffer + 13);
			else if(!strncmp(buffer, "MESSAGE ", 8))
				printf("%s\n", buffer + 8);
			else if(!strcmp(buffer, "CLIENT_LIST")){
				/* TODO */
			}else
				UNKNOWN_MESSAGE(buffer);
			break;

		case VERSION_WAIT:
		{
			int maj, min;
			if(sscanf(buffer, "Comm v%d.%d", &maj, &min) == 2){
				printf("Server Version OK: %d.%d, checking name...\n", maj, min);
				if(maj == VERSION_MAJOR && min == VERSION_MINOR){
					state = NAME_WAIT;
					TO_SERVER_F("NAME %s", name);
				}else{
					printf("Server version mismatch: %d.%d\n", maj, min);
					return 1;
				}
			}else{
				UNKNOWN_MESSAGE(buffer);
				return 1;
			}
			break;
		}

		case NAME_WAIT:
			if(!strcmp(buffer, "OK")){
				state = ACCEPTED;
				puts("Name accepted, fire away");
			}else{
				UNKNOWN_MESSAGE(buffer);
				return 1;
			}
			break;
	}

	return 0;
}

int stdin_read()
{
	char buffer[LINE_SIZE], *nl;

	if(fgets(buffer, LINE_SIZE, stdin)){
		if((nl = strchr(buffer, '\n')))
			*nl = '\0';

		if(*buffer == '/'){
			/* command */
			if(!strcmp(buffer+1, "exit"))
				return 1;
			else if(!strcmp(buffer+1, "ls"))
				showclients();
			else
				printf("unknown command: %s\n", buffer+1);
		}else{
			/* message */
			TO_SERVER_F("MESSAGE %s: %s", name, buffer);
		}
		return 0;
	}else{
		if(ferror(stdin))
			perror("fgets()");
		/* eof */
		return 1;
	}
}

int ui_main(const char *nme, char *host, int port)
{
	struct pollfd pollfds[2]; /* stdin and socket */
	int ret = 0;

	if(!host){
		fputs("need host\n", stderr);
		return 1;
	}

	name = nme;

	if((sock = connectedsock(host, port)) == -1)
		return 1;

	sockf = fdopen(sock, "r+");
	if(!sockf){
		perror("fdopen()");
		goto bail;
	}

	if(setvbuf(sockf, NULL, _IONBF, 0)){
		perror("setvbuf()");
		goto bail;
	}

	printf("Connected to %s:%d, checking version...\n", host, port);

	memset(pollfds, '\0', 2 * sizeof pollfds);
	pollfds[0].fd = sock;
	pollfds[0].events = POLLIN;

	pollfds[1].fd = STDIN_FILENO;
	pollfds[1].events = POLLIN | POLLHUP;

	for(;;){
		int pret = poll(pollfds, 1 + (state == ACCEPTED), TIMEOUT);

		if(pret == 0)
			continue;
		else if(pret < 0){
			if(errno == EINTR)
				continue;
			perror("poll()");
			return 1;
		}

#define BIT(a, b) (((a) & (b)) == (b))
		if(BIT(pollfds[0].revents, POLLIN) && sock_read())
			goto bail;

		if(BIT(pollfds[1].revents, POLLIN) && stdin_read())
			goto bail;
		if(BIT(pollfds[1].revents, POLLHUP)){
			/*
			 * get here if we're reading input from a pipe
			 * && eof
			 */
			fputs("tcomm: input hangup\n", stderr);
			goto bail;
		}
	}

bail:
	shutdown(sock, SHUT_RDWR);
	if(sockf)
		fclose(sockf);
	else
		close(sock);
	return ret;
}
