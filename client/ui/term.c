#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#include <poll.h>

#include "../../settings.h"
#include "../../socket_util.h"

#include "ui.h"

#define TIMEOUT 100

static enum state { VERSION_WAIT, NAME_WAIT, ACCEPTED } state = VERSION_WAIT;
static int sock;
static FILE *sockf;
static const char *name;

int connectedsock(char *, int);

int sock_read(void);
int stdin_read(void);

int sock_read()
{
	char buffer[LINE_SIZE], *nl;
	int ret;

	ret = recv(sock, buffer, LINE_SIZE, 0);

	if(ret == 0){
		/* disco */
		puts("server disconnect");
		return 1;
	}else if(ret < 0){
		perror("recv()");
		return 1;
	}

	if((nl = strchr(buffer, '\n')))
		*nl = '\0';

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
				fprintf(stderr, "unknown message from server: %s\n", buffer);
			break;

		case VERSION_WAIT:
		{
			int maj, min;
			if(sscanf(buffer, "Comm v%d.%d", &maj, &min) == 2){
				printf("Server Version %d.%d\n", maj, min);
				if(maj == VERSION_MAJOR && min == VERSION_MINOR){
					state = NAME_WAIT;
					fprintf(sockf, "NAME %s\n", name);
				}else{
					puts("server version mismatch");
					return 1;
				}
			}else{
				printf("Unknown message from server (%s)\n", buffer);
				return 1;
			}
			break;
		}

		case NAME_WAIT:
			if(!strcmp(buffer, "OK")){
				state = ACCEPTED;
				puts("Name accepted, fire away");
			}else{
				printf("Unknown message from server (%s)\n", buffer);
				return 1;
			}
			break;
	}

	return 0;
}

int stdin_read()
{
	char buffer[LINE_SIZE];

	if(fgets(buffer, LINE_SIZE, stdin)){
		if(*buffer == '/'){
			/* command */
			if(!strcmp(buffer+1, "exit"))
				return 1;
			/*else if(!strcmp(buffer+1, "ls"))
				showclients();*/
		}else{
			/* message */
			fprintf(sockf, "MESSAGE %s\n", buffer);
		}
	}else
		/* eof */
		return 1;

	return 0;
}


int connectedsock(char *host, int port)
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in addr;

	if(fd == -1){
		perror("socket()");
		return -1;
	}

	memset(&addr, '\0', sizeof addr);

	addr.sin_family = AF_INET;
	if(!lookup(host, port, &addr)){
		perror("lookup()");
		close(fd);
		return -1;
	}

	if(connect(fd, (struct sockaddr *)&addr, sizeof addr) == -1){
		perror("connect()");
		return -1;
	}

	return fd;
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

	printf("Connected to %s:%d, checking name...\n", host, port);

	memset(pollfds, '\0', 2 * sizeof pollfds);
	pollfds[0].fd = sock;
	pollfds[1].fd = STDIN_FILENO;
	pollfds[0].events = pollfds[1].events = POLLIN;

	for(;;){
		int pret = poll(pollfds, 1 + (state == ACCEPTED), TIMEOUT);

		if(pret == 0)
			continue;
		else if(pret < 0){
			perror("poll()");
			return 1;
		}

#define BIT(a, b) (((a) & (b)) == (b))
		if(BIT(pollfds[0].revents, POLLIN) && sock_read())
			goto bail;

		if(BIT(pollfds[1].revents, POLLIN) && stdin_read())
			goto bail;
	}

bail:
	shutdown(sock, SHUT_RDWR);
	if(sockf)
		fclose(sockf);
	else
		close(sock);
	return ret;
}
