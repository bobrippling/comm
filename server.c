#include <stdlib.h>
#include <unistd.h>
#include <poll.h>

#include "settings.h"

#define LISTEN_BACKLOG 5
#define SLEEP_MS       50
#define RECV_BUFSIZ    512

#define TO_CLIENT(i, msg) write(clients[i].pfd.fd, msg, strlen(msg))


struct client
{
	struct pollfd pfd;
	const char *name;

	enum
	{
		ACCEPTING,
		ACCEPTED
	} state;
} *clients = NULL;

static int server, nclients = 0;


void svr_conn(int);
char svr_recv(int);
void svr_hup( int);
void svr_err( int);
/*
 * svr_recv return 1 if the client disconnects/on error
 * i.e. the clients array has been altered
 * --> --i in the loop
 */


int server_main()
{
	struct sockaddr_in addr;
	int flags;

	/* ignore sigpipe - sent when recv() called on a disconnected socket */
	if(signal(SIGPIPE, SIG_IGN) == SIG_ERR){
		perror("signal()");
		return 1;
	}

	server = socket(AF_INET, SOCK_STREAM, 0);

	if(!server == -1){
		perror("socket()");
		return 1;
	}

	addr.sin_family = AF_INET;
	addr.sin_port   = global_settings.port;

	if(bind(server, (struct sockaddr *)&addr, sizeof addr) == -1){
		perror("bind()");
		close(server);
		return 1;
	}

	if(listen(server, LISTEN_BACKLOG) == -1){
		perror("listen()");
		close(server);
		return 1;
	}

	if((flags = fcntl(fd, F_GETFL, 0)) == -1)
		flags = 0;
	flags |= O_NONBLOCK;

	if(fcntl(fd, F_SETFL, flags) == -1){
		perror("fcntl()");
		close(server);
		return 1;
	}

	for(;;){
		int cfd = accept(server, NULL, NULL), i, ret;

		if(cfd != -1){
			struct pollfd *new = realloc(clients, ++nclients * sizeof(*clients));

			if(!new){
				fputs("realloc() error (continuing without accept()): ", stderr);
				perror(NULL);
				close(cfd);
			}else{
				clients = new;
				if(!svr_conn(clients[nclients-1].pfd.fd = cfd)){
					clients = realloc(clients, --nclients);
					close(cfd);
				}
			}
		}

		for(i = 0; i < nclients; i++)
			clients[i].pfd.events = POLLIN | POLLERR | POLLHUP;

		ret = poll(clients, nclients, SLEEP_MS);
		/* need to timeout, since we're also accept()ing above */

		/*
		 * ret = 0: timeout
		 * ret > 0: number of fds that have status
		 * ret < 0: err
		 */
		if(ret == 0)
			continue;
		else if(ret < 0){
			perror("poll()");
			cleanup();
			return 1;
		}

#define BIT(a, b) (((a) & (b)) == (b))
		for(i = 0; i < nclients; i++){
			if(BIT(clients[i].revents, POLLHUP))
				svr_hup(i);
			if(BIT(clients[i].revents, POLLERR))
				svr_err(i);
			if(BIT(clients[i].revents, POLLIN))
				i -= svr_recv(i);
		}
	}

	return 0;
}

void svr_conn(int index)
{
	static char buffer[] = "Comm v"VERSION"\n";

	if(write(fd, buffer, sizeof buffer) == -1){
		perror("write()");
		return 0;
	}

	clients[index].state = ACCEPTING;
	clients[index].name  = NULL;

	return 1;
}

void svr_hup(int index)
{
	int i;

	close(clients[index].pfd.fd);
	clients[index].pfd.fd = -1;

	for(i = index; i < nclients-1; i++)
		clients[i] = clients[i+1];

	clients = realloc(clients, --nclients);
}

void svr_err(int index)
{
	fprintf(stderr, "error with client %d (socket %d)", index, clients[index].pfd.fd);
	if(errno)
		fprintf(stderr, ": %s\n", strerror(errno));
	else
		fputc('\n', stderr);
	svr_hup(index);
}

char svr_recv(int index)
{
	/* TODO: any size buffer using dynamic mem */
	char in[RECV_BUFSIZ];

	/* FIXME: only recv up to a new line */
	if(recv(clients[index].pfd.fd, in, RECV_BUFSIZ, RECV_FLAGS) == -1){
		fputs("recv() ", stderr);
		svr_err(index);
		return 1;
	}

	if(clients[index].state == ACCEPTING){
		if(!strcmp(in, "NAME ", 5)){
			int len = strlen(in + 5);
			if(len == 0){
				TO_CLIENT(index, "need non-zero length name\n");
				fprintf(stderr, "client %d - zero length name\n", index);
				svr_hup(index);
				return 1;
			}

			/* TODO: check name isn't in use */
			strcpy(clients[index].name = cmalloc(len),
					in + 5);

			clients[index].state = ACCEPTED;
		}else{
			TO_CLIENT(index, "need name\n");
			fprintf(stderr, "client %d - name not given\n", index);
			svr_hup(index);
			return 1;
		}
	}else{
		/* TODO: generic protocl shiz */
	}

	return 0;
}
