#define _POSIX_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <setjmp.h>
#include <stdarg.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#include "../config.h"
#include "../util.h"
#include "../socket_util.h"

#include "server.h"

#define LISTEN_BACKLOG 5
#define SLEEP_MS       100
#define TO_CLIENT(i, msg) fwrite(msg"\n", sizeof *msg, strlen(msg) + 1, clients[i].file)
/*                                                     ^don't include the '\0', but the '\n' */

#define CLIENT_FMT  "client %d (socket %d)"
#define CLIENT_ARGS idx, pollfds[idx].fd

static struct client
{
	struct sockaddr_in addr;

	FILE *file;

	char *name;

	enum
	{
		ACCEPTING,
		ACCEPTED
	} state;
} *clients = NULL;

static struct pollfd *pollfds = NULL;

static int server = -1, nclients = 0;
static char verbose = 0;

jmp_buf allocerr;


int nonblock(int);
void cleanup(void);
void sigh(int);
void freeclient(int);
int toclientf(int, const char *, ...);

char svr_conn(int);
char svr_recv(int);
void svr_hup( int);
void svr_err( int);
/*
 * svr_recv return 1 if the client disconnects/on error
 * i.e. the clients array has been altered
 * --> --i in the loop
 */

int toclientf(int idx, const char *fmt, ...)
{
	static const char nl = '\n';
	va_list l;
	int ret;

	va_start(l, fmt);
	ret = vfprintf(clients[idx].file, fmt, l);
	va_end(l);

	if(ret == -1)
		return -1;

	return fwrite(&nl, sizeof nl, 1, clients[idx].file);
}

int nonblock(int fd)
{
	int flags;

	if((flags = fcntl(fd, F_GETFL, 0)) == -1)
		flags = 0;
	flags |= O_NONBLOCK;

	if(fcntl(fd, F_SETFL, flags) == -1)
		return 1;
	return 0;
}

void cleanup()
{
	int i;

	for(i = 0; i < nclients; i++)
		freeclient(i);

	free(clients);
	free(pollfds);

	shutdown(server, SHUT_RDWR);
	close(server);
}

char svr_conn(int idx)
{
	clients[idx].state = ACCEPTING;
	clients[idx].name  = NULL;
	if(!(clients[idx].file  = fdopen(pollfds[idx].fd, "r+"))){
		fprintf(stderr, CLIENT_FMT" fdopen(): ", CLIENT_ARGS);
		perror(NULL);
		return 0;
	}

	if(setvbuf(clients[idx].file, NULL, _IONBF, 0)){
		fprintf(stderr, CLIENT_FMT" setvbuf(): ", CLIENT_ARGS);
		perror(NULL);
		return 0;
	}

	if(TO_CLIENT(idx, "Comm v"VERSION) == 0){
		fprintf(stderr, CLIENT_FMT" fwrite(): ", CLIENT_ARGS);
		perror(NULL);
		return 0;
	}

	if(verbose)
		printf(CLIENT_FMT" connected from %s\n", CLIENT_ARGS, addrtostr(&clients[idx].addr));

	return 1;
}

void svr_hup(int idx)
{
	int i;

	if(verbose)
		printf(CLIENT_FMT" disconnected\n", CLIENT_ARGS);

	if(clients[idx].state == ACCEPTED)
		for(i = 0; i < nclients; i++)
			if(i != idx && clients[i].state == ACCEPTED)
				toclientf(i, "CLIENT_DISCO %s", clients[idx].name);

	freeclient(idx);

	for(i = idx; i < nclients-1; i++){
		clients[i] = clients[i+1];
		pollfds[i] = pollfds[i+1];
	}

	clients = realloc(clients, --nclients * sizeof(*clients));
	pollfds = realloc(pollfds,   nclients * sizeof(*pollfds));
}

void freeclient(int idx)
{
	int fd = pollfds[idx].fd;

	free(clients[idx].name);

	if(fd != -1){
		shutdown(fd, SHUT_RDWR);
		/*close(fd);*/
		fclose(clients[idx].file); /* closes fd */
		pollfds[idx].fd = -1;
		clients[idx].file = NULL;
	}
}

void svr_err(int idx)
{
	fprintf(stderr, CLIENT_FMT" error", CLIENT_ARGS);
	if(errno)
		perror(NULL);
	else
		fputc('\n', stderr);
	svr_hup(idx);
}

char svr_recv(int idx)
{
	char in[LINE_SIZE] = { 0 }, *newl;
	int ret;

	switch(ret = recv(pollfds[idx].fd, in, LINE_SIZE, MSG_PEEK)){
		case -1:
			fputs("recv() ", stderr);
			svr_err(idx);
			return 1;

		case 0:
			svr_hup(idx);
			return 1;
	}

	if((newl = strchr(in, '\n')) && newl < (in + ret)){
		/* recv just up to the '\n' */
		int len = newl - in + 1;

		if(len > LINE_SIZE)
			len = LINE_SIZE;

		ret = recv(pollfds[idx].fd, in, len, 0);
		*newl = '\0';
	}else{
		if(ret == LINE_SIZE){
			fprintf(stderr, CLIENT_FMT" running with data: LINE_SIZE exceeded\n", CLIENT_ARGS);
			recv(pollfds[idx].fd, in, LINE_SIZE, 0);
		}else
			/* need more data */
			return 0;
	}

	if(verbose > 1)
		fprintf(stderr, CLIENT_FMT" recv(): \"%s\"\n", CLIENT_ARGS, in);

	if(clients[idx].state == ACCEPTING){
		if(!strncmp(in, "NAME ", 5)){
			char *name = in + 5;
			int len = strlen(name), i;

			if(len == 0){
				TO_CLIENT(idx, "ERR need non-zero length name");
				fprintf(stderr, CLIENT_FMT": zero length name\n", CLIENT_ARGS);
				svr_hup(idx);
				return 1;
			}

			for(i = 0; i < nclients; i++)
				if(i != idx && clients[i].state == ACCEPTED && !strcmp(clients[i].name, name)){
					TO_CLIENT(idx, "ERR name in use");
					fprintf(stderr, CLIENT_FMT": name %s in use\n", CLIENT_ARGS, name);
					svr_hup(idx);
					return 1;
				}

			strcpy(clients[idx].name = cmalloc(len+1), name);

			clients[idx].state = ACCEPTED;

			if(verbose)
				printf(CLIENT_FMT" name accepted: %s\n", CLIENT_ARGS, name);

			TO_CLIENT(idx, "OK");

			for(i = 0; i < nclients; i++)
				if(i != idx && clients[i].state == ACCEPTED)
					toclientf(i, "CLIENT_CONN %s", name);

		}else{
			TO_CLIENT(idx, "ERR need name");
			fprintf(stderr, CLIENT_FMT" name not given\n", CLIENT_ARGS);
			svr_hup(idx);
			return 1;
		}
	}else{
		if(!strcmp(in, "CLIENT_LIST")){
			int i;
			TO_CLIENT(idx, "CLIENT_LIST_START");
			for(i = 0; i < nclients; i++)
				if(i != idx && clients[i].state == ACCEPTED)
					toclientf(idx, "CLIENT_LIST %s", clients[i].name);
			TO_CLIENT(idx, "CLIENT_LIST_END");

		}else if(!strncmp(in, "MESSAGE ", 8)){
			int i;

			if(!strlen(in + 8)){
				TO_CLIENT(idx, "ERR need message");
				return 0;
			}

			for(i = 0; i < nclients; i++)
				if(i != idx && clients[i].state == ACCEPTED)
					toclientf(i, "%s", in);
		}else{
			TO_CLIENT(idx, "ERR command unknown");
		}
	}

	return 0;
}

void sigh(int sig)
{
	if(sig == SIGINT){
		int i;
		printf("\nComm v"VERSION" Server Status - %d clients\n", nclients);
		if(nclients){
			puts("Index FD Name");
			for(i = 0; i < nclients; i++)
				printf("%3d %3d %s\n", i, pollfds[i].fd, clients[i].name);
		}
		fflush(stdout);
		signal(SIGINT, &sigh);
	}else{
		const char buffer[] = "We get signal\n";
		write(STDERR_FILENO, buffer, sizeof buffer);

		cleanup();

		exit(sig);
	}
}

int main(int argc, char **argv)
{
	struct sockaddr_in svr_addr;
	int i, port = DEFAULT_PORT;

#define USAGE() do { \
		fprintf(stderr, "Usage: %s [-p port] [-v*]\n", *argv); \
		return 1; \
	} while(0)

	if(setjmp(allocerr)){
		perror("longjmp'd to allocerr");
		return 1;
	}

	/* ignore sigpipe - sent when recv() called on a disconnected socket */
	if( signal(SIGPIPE, SIG_IGN) == SIG_ERR ||
			signal(SIGINT,  &sigh)   == SIG_ERR ||
			signal(SIGQUIT, &sigh)   == SIG_ERR ||
			signal(SIGTERM, &sigh)   == SIG_ERR ||
			signal(SIGSEGV, &sigh)   == SIG_ERR){
		perror("signal()");
		return 1;
	}

	for(i = 1; i < argc; i++)
		if(!strcmp(argv[i], "-p")){
			if(++i < argc)
				port = atoi(argv[i]);
			else{
				fputs("need port\n", stderr);
				return 1;
			}
		}else if(!strncmp(argv[i], "-v", 2)){
			char *p = argv[i] + 1;

			while(*p == 'v'){
				verbose++;
				p++;
			}

			if(*p != '\0')
				USAGE();
		}else{
			USAGE();
		}

	if(verbose > 1)
		printf("verbose %d\n", verbose);

	server = socket(AF_INET, SOCK_STREAM, 0);
	if(server == -1){
		perror("socket()");
		return 1;
	}

	memset(&svr_addr, '\0', sizeof svr_addr);
	svr_addr.sin_family = AF_INET;
	svr_addr.sin_port   = htons(port);

	if(INADDR_ANY) /* usually optimised out of existence */
		svr_addr.sin_addr.s_addr = htonl(INADDR_ANY);


	if(bind(server, (struct sockaddr *)&svr_addr, sizeof svr_addr) == -1){
		perror("bind()");
		close(server);
		return 1;
	}

	if(listen(server, LISTEN_BACKLOG) == -1){
		perror("listen()");
		close(server);
		return 1;
	}

	if(nonblock(server)){
		perror("nonblock()");
		close(server);
		return 1;
	}

	for(;;){
		struct sockaddr_in addr;
		socklen_t len = sizeof addr;
		int cfd = accept(server, (struct sockaddr *)&addr, &len), ret;

		if(cfd != -1){
			struct client *new     = realloc(clients, ++nclients * sizeof(*clients));
			struct pollfd *newpfds = realloc(pollfds,   nclients * sizeof(*newpfds));

			if(!new || !newpfds){
				perror("realloc()");
				cleanup();
				return 1;
			}else{
				int idx = nclients-1;

				clients = new;
				pollfds = newpfds;

				memcpy(&clients[idx].addr, &addr, sizeof addr);

				pollfds[idx].fd = cfd;

				if(!svr_conn(idx)){
					if(clients[idx].file)
						fclose(clients[idx].file);
					clients = realloc(clients, --nclients * sizeof(*clients));
					pollfds = realloc(pollfds,   nclients * sizeof(*pollfds));
					close(cfd);
				}
			}
		}else if(errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK){
			perror("accept()");
			cleanup();
			return 1;
		}

		for(i = 0; i < nclients; i++)
			pollfds[i].events = POLLIN | POLLERR | POLLHUP;

		ret = poll(pollfds, nclients, SLEEP_MS);
		/* need to timeout, since we're also accept()ing above */

		/*
		 * ret = 0: timeout
		 * ret > 0: number of fds that have status
		 * ret < 0: err
		 */
		if(ret == 0)
			continue;
		else if(ret < 0){
			if(errno == EINTR)
				continue;

			perror("poll()");
			cleanup();
			return 1;
		}

#define BIT(a, b) (((a) & (b)) == (b))
		for(i = 0; ret > 0 && i < nclients; i++){
			if(BIT(pollfds[i].revents, POLLHUP)){
				ret--;
				svr_hup(i--);
				continue;
			}

			if(BIT(pollfds[i].revents, POLLERR)){
				ret--;
				svr_err(i--);
				continue;
			}

			if(BIT(pollfds[i].revents, POLLIN)){
				ret--;
				i -= svr_recv(i);
			}
		}
	}
	/* UNREACHABLE */
}
