#define _POSIX_SOURCE
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "ftp.h"

#define BUFFER_SIZE 2048

/*
 * sender:    "FILE fname"
 *            "SIZE [0-9]+"
 *            "EXTRA ..." # omg extensible
 *            "" # newline means finito
 *
 * recipient: "OK"
 *        or: <disconnect>
 *
 * sender:    <file dump>
 *            <close conn>
 */

static char *strdup2(const char *s);

#define BAIL(s, ...) \
	do{ \
		printf(__FILE__ ":%d: (serrno: %s) bailing: " s \
				"\n", __LINE__, strerror(errno), __VA_ARGS__); \
		return 1; \
	}while(0)

#define SOCK_INIT() \
	struct sockaddr_in addr; \
\
	memset(ft,    '\0', sizeof *ft); \
	memset(&addr, '\0', sizeof addr); \
\
	if((ft->sock = socket(PF_INET, SOCK_STREAM, 0)) == -1) \
		return 1; \


int ft_listen(struct filetransfer *ft, int port)
{
	int save;

	SOCK_INIT();

	addr.sin_port = htons(port);
	addr.sin_family = AF_INET;

	save = 1;
	setsockopt(ft->sock, SOL_SOCKET, SO_REUSEADDR, &save, sizeof save);

	if(bind(ft->sock, (struct sockaddr *)&addr, sizeof addr) == -1){
		save = errno;
		close(ft->sock);
		errno = save;
		BAIL("bind%s", "()");
	}

	if(listen(ft->sock, 1) == -1){
		save = errno;
		close(ft->sock);
		errno = save;
		BAIL("listen%s", "()");
	}

	return 0;
}

int ft_accept(struct filetransfer *ft)
{
	int new;

	/* TODO: select() for non blocking */
	new = accept(ft->sock, NULL, 0);

	if(new == -1)
		BAIL("accept%s", "()");

	close(ft->sock);
	ft->sock = new;
	return 0;
}

int ft_recv(struct filetransfer *ft, ft_callback callback)
{
	/*
	 * expect:    "FILE fname"
	 *            "SIZE [0-9]+"
	 *            ""
	 */
	FILE *local;
	char *basename;
	char buffer[BUFFER_SIZE];
	ssize_t size, nbytes = 0;
	enum
	{
		GOT_NOWT, GOT_FILE, GOT_SIZE, GOT_ALL, RUNNING
	} state = GOT_NOWT;

	while(1){
		ssize_t nread = recv(ft->sock, buffer, sizeof buffer, 0);

		if(nread == -1)
			BAIL("recv%s", "()");
		else if(nread == 0){
			if(state == RUNNING){
				fclose(local);
				/* check finished */
				if(nbytes == size)
					return 0;
				BAIL("nbytes != size%s", "");
			}else
				BAIL("premature transfer end: %zd != %zd",
						nbytes, size);
		}

check_buffer:
		if(state == GOT_ALL){
			/* TODO: clobber check */
			local = fopen(basename, "wb");
			if(!local)
				/* TODO: diagnostic */
				BAIL("fopen local%s", "");
			state = RUNNING;
		}

		if(state == RUNNING){
			if(write(fileno(local), buffer, nread) != nread)
				/* TODO: diagnostic */
				BAIL("fwrite()%s", "");
			nbytes += nread;
		}else{
			while(1){
				char *nl = strchr(buffer, '\n');
				if(nl){
					*nl++ = '\0';
#define SHIFT_BUFFER() \
				do{ \
					int adj = nl - buffer; \
					memmove(buffer, nl, sizeof(buffer) - adj); \
					nread -= adj; \
				}while(0)


					switch(state){
						case GOT_NOWT:
							if(!strncmp(buffer, "FILE ", 5)){
								basename = strdup2(buffer + 5);
								if(!basename)
									BAIL("strndup2()%s", "");
								SHIFT_BUFFER();
								state = GOT_FILE;
							}else
								/* TODO: diagnostic */
								BAIL("FILE... expected%s", "");
							break;
						case GOT_FILE:
							if(!strncmp(buffer, "SIZE ", 5)){
								if(sscanf(buffer + 5, "%zd", &size) != 1)
									/* TODO: diagnostic */
									BAIL("SIZE %%d... expected%s", "");
								SHIFT_BUFFER();
								state = GOT_SIZE;
							}else
								/* TODO: diagnostic */
								BAIL("SIZE... expected%s", "");
							break;
						case GOT_SIZE:
							if(buffer[0] == '\0'){
								state = GOT_ALL;
								SHIFT_BUFFER();
							}else
								/* TODO: diagnostic */
								BAIL("\\n... expected%s", "");
							goto check_buffer;
						case GOT_ALL:
						case RUNNING:
							BAIL("BUH?!?!?!%s", "");
					}
				}else
					break;
			}
		}
	}
}

int ft_send(struct filetransfer *ft, const char *fname, ft_callback callback)
{
	struct stat st;
	FILE *local;
	char buffer[BUFFER_SIZE];
	size_t nwrite;
	const char *basename = strrchr(fname, '/');

	local = fopen(fname, "rb");
	if(!local)
		/* TODO: diagnostic */
		BAIL("fopen local%s", "");

	if(fstat(fileno(local), &st) == -1)
		/* TODO: diagnostic */
		goto bail;

	if(!basename)
		basename = fname;
	else
		basename++;

	if((nwrite = snprintf(buffer, sizeof buffer, "FILE %s\nSIZE %zd\n\n",
			basename, st.st_size)) >= (signed)sizeof buffer)
		/* FIXME TODO: diagnostic */
		goto bail;

	while(1){
		if(write(ft->sock, buffer, nwrite) == -1)
			/* TODO: diagnostic */
			goto bail;

		switch((nwrite = read(fileno(local), buffer, sizeof buffer))){
			case 0:
				if(ferror(local))
					/* TODO: diagnostic */
					goto bail;
				goto complete;
			case -1:
				/* TODO: diagnostic */
				goto bail;
		}
	}
complete:

	fclose(local);
	return 0;
bail:
	BAIL("generic ft_send()%s", "");
	fclose(local);
	return 1;
}

int ft_connect(struct filetransfer *ft, const char *host, const char *port)
{
	struct addrinfo hints, *ret = NULL, *dest = NULL;
	int lastconnerr = 0;

	SOCK_INIT();

	memset(&hints, '\0', sizeof hints);
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	/* FIXME: lookup_errno */
	if(getaddrinfo(host, port, &hints, &ret))
		BAIL("getaddrinfo()%s", "");

	for(dest = ret; dest; dest = dest->ai_next){
		ft->sock = socket(dest->ai_family, dest->ai_socktype,
				dest->ai_protocol);

		if(ft->sock == -1)
			continue;

		if(connect(ft->sock, dest->ai_addr, dest->ai_addrlen) == 0)
			break;

		lastconnerr = errno;
		close(ft->sock);
		ft->sock = -1;
	}

	freeaddrinfo(ret);

	if(ft->sock == -1){
		errno = lastconnerr;
		BAIL("connect()%s", "");
	}

	return 0;
}

void ft_close(struct filetransfer *ft)
{
	close(ft->sock);
	ft->sock = -1;
}


static char *strdup2(const char *s)
{
	size_t len = strlen(s);
	char *d = malloc(len + 1);
	if(!d)
		return NULL;
	strcpy(d, s);
	return d;
}
