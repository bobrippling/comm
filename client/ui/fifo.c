#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <arpa/inet.h> /* struct sockaddr_in */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>

#include "../../socket_util.h"
#include "../common.h"
#include "ui.h"

#include "../../settings.h"

#define TIMEOUT 100

#ifndef PIPE_BUF
#define PIPE_BUF 4096
#endif

int fifo_init(void);
int fifo_term(void);
int readfifo(int idx);
int run(void);
int handshake(void);

struct fifo
{
	const char *path;
	int fd;
} fifos[] = {
	{ "ctl_in",   -1 },
	{ "ctl_out",  -1 },
	{ "comm_in",  -1 },
	{ "comm_out", -1 }
};

#define NFIFOS ((signed)(sizeof(fifos)/sizeof(fifos[0])))

static const char *name;
static int serverfd;


int fifo_init(void)
{
	int i;
	for(i = 0; i < NFIFOS; i++){
		if(mkfifo(fifos[i].path, 0700) == -1){
			perror("mkfifo()");
			goto cleanup;
		}
		fifos[i].fd = open(fifos[i].path, O_RDONLY | O_NONBLOCK);
		if(fifos[i].fd == -1){
			perror("open()");
			goto cleanup;
		}
	}

	return 0;
cleanup:
	for(--i; i >= 0; i--)
		remove(fifos[i].path);
	return 1;
}

int fifo_term(void)
{
	int i;
	for(i = 0; i < NFIFOS; i++){
		close(fifos[i].fd);
		if(remove(fifos[i].path))
			perror("remove()");
	}
	return 0;
}

int readfifo(int idx)
{
	char buffer[PIPE_BUF];

	switch(read(fifos[idx].fd, buffer, sizeof buffer)){
		case -1:
			perror("read()");
			return 1;

		case 0:
			return 0;
	}

	printf("got data on fifo %d (%s): \"%s\"\n", idx, fifos[idx].path, buffer);

	return 0;
}

int run(void)
{
	struct pollfd pollfds[NFIFOS];
	int i;

	for(i = 0; i < NFIFOS; i++){
		pollfds[i].fd = fifos[i].fd;
		pollfds[i].events = POLLIN;
	}

	for(;;){
		int pret = poll(pollfds, NFIFOS, TIMEOUT);

		if(pret == 0)
			continue;
		else if(pret < 0){
			if(errno == EINTR)
				continue;
			perror("poll()");
			return 1;
		}

#define BIT(a, b) (((a) & (b)) == (b))
		for(i = 0; i < NFIFOS; i++)
			if(BIT(pollfds[i].revents, POLLIN) && readfifo(i))
				return 1;
	}

	return 0;
}

int handshake(void)
{
#define BSIZ 512
	char buffer[512] = { 0 };
	int nrec, vmaj, vmin;

	switch((nrec = recv(serverfd, buffer, BSIZ, 0))){
		case 0:
			/* disco() */
			fputs("got disconnect\n", stderr);
			return 1;

		case -1:
			perror("recv()");
			return 1;
	}

	if(sscanf(buffer, "Comm v%d.%d", &vmaj, &vmin) != 2){
		fprintf(stderr, "Invalid protocol: \"%s\"\n", buffer);
		return 1;
	}

	if(vmaj != VERSION_MAJOR || vmin != VERSION_MINOR){
		fprintf(stderr, "server version mismatch (%d.%d)\n", vmaj, vmin);
		return 1;
	}

	toserverf(serverfd, "NAME %s", name);

	memset(buffer, '\0', BSIZ);
	switch((nrec = recv(serverfd, buffer, BSIZ, 0))){
		case 0:
			/* disco() */
			fputs("got disconnect after NAME\n", stderr);
			return 1;

		case -1:
			perror("(after NAME): recv()");
			return 1;
	}

	/* FIXME: socket read() function */
	if(!strncmp(buffer, "OK", 2))
		return 0;

	fputs("invalid name\n", stderr);
	return 1;
#undef BSIZ
}

int ui_main(const char *nme, char *host, int port)
{
	int ret = 0;

	name = nme;
	if(!host){
		fputs("need host\n", stderr);
		return 1;
	}

	if((serverfd = connectedsock(host, port)) == -1)
		return 1;

	if(handshake()){
		ret = 1;
		goto sock_down;
	}

	if(fifo_init()){
		ret = 1;
		goto sock_down;
	}

	ret = run();

	fifo_term();
sock_down:
	shutdown(serverfd, SHUT_RDWR);
	close(serverfd);

	return ret;
}
