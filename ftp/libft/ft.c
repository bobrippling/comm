#ifdef _WIN32
# define _WIN32_WINNT 0x501
# define close(x) closesocket(x)
# define PRINTF_SIZET "%ld"
/* ws2tcpip must be first... enterprise */
# include <ws2tcpip.h>
# include <winsock2.h>
/* ^ getaddrinfo */

#define os_lasterr WSAGetLastError()

#else
# define _POSIX_C_SOURCE 200809L
# include <sys/socket.h>
# include <arpa/inet.h>
# include <netdb.h>
# include <unistd.h>
# include <fcntl.h>
# include <sys/select.h>
# include <errno.h>

#define os_lasterr errno

# define PRINTF_SIZET "%zd"

#endif

/* can't use sendfile and have callbacks */
#undef USE_SENDFILE

#ifdef USE_SENDFILE
# include <sys/sendfile.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#ifdef _WIN32
int fileno(FILE *);
ssize_t write(int fd, const char *, ssize_t);
ssize_t read( int fd, const char *, ssize_t);
char *strdup(const char *);

static int WSA_Startuped = 0;
#endif


#include "ft.h"

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

#ifdef _WIN32
# define DATA_INIT() \
	if(!WSA_Startuped){ \
		struct WSAData d; \
		WSA_Startuped = 1; \
		if(WSAStartup(MAKEWORD(2, 2), &d)){ \
			ft->lasterr = os_lasterr; \
			return 1; \
		} \
	} \
	memset(ft,    '\0', sizeof *ft); \
	memset(&addr, '\0', sizeof addr);
#else
# define DATA_INIT() \
	memset(ft,    '\0', sizeof *ft); \
	memset(&addr, '\0', sizeof addr);
#endif

int ft_listen(struct filetransfer *ft, int port)
{
#ifndef _WIN32
	int save;
#endif
	struct sockaddr_in addr;

	DATA_INIT();

	ft->lasterr_isnet = 0;
	if((ft->sock = socket(PF_INET, SOCK_STREAM, 0)) == -1){
		ft->lasterr = os_lasterr;
		return 1;
	}

	addr.sin_port = htons(port);
	addr.sin_family = AF_INET;

#ifndef _WIN32
	save = 1;
	setsockopt(ft->sock, SOL_SOCKET, SO_REUSEADDR, &save, sizeof save);
#endif

	if(bind(ft->sock, (struct sockaddr *)&addr, sizeof addr) == -1){
		ft->lasterr = os_lasterr;
		close(ft->sock);
		return 1;
	}

	if(listen(ft->sock, 1) == -1){
		ft->lasterr = os_lasterr;
		close(ft->sock);
		return 1;
	}

	return 0;
}

int ft_accept(struct filetransfer *ft, int block)
{
	int new;

	if(!block){
		fd_set fds;

		FD_ZERO(&fds);
		FD_SET(ft->sock, &fds);

		if(select(ft->sock+1, &fds, NULL, NULL, NULL) == -1){
			ft->lasterr = errno;
			return 1;
		}

		if(!FD_ISSET(ft->sock, &fds))
			return 0;
	}

	new = accept(ft->sock, NULL, 0);

	if(new == -1)
		return 1;

	ft_connected(ft) = 1;

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
	int ilocal;
	char *basename; /* FIXME: free() */
	char buffer[BUFFER_SIZE];
	ssize_t size, nbytes = 0;
	enum
	{
		GOT_NOWT, GOT_FILE, GOT_SIZE, GOT_ALL, RUNNING
	} state = GOT_NOWT;

	while(1){
		ssize_t nread = recv(ft->sock, buffer, sizeof buffer, 0);

		if(nread == -1){
			ft->lasterr = errno;
			return 1;
		}else if(nread == 0){
			if(state == RUNNING){
				fclose(local);
				/* check finished */
				if(nbytes == size){
					if(callback(ft, FT_END, nbytes, size)){
						fclose(local);
						return 1;
					}
					return 0;
				}
				ft->lasterr = "Connection prematurely closed"; // XXX
				return 1;
				/* XXX: function exit point here */
			}else
				return 1;
		}

check_buffer:
		if(state == GOT_ALL){
			/* TODO: clobber check */
			local = fopen(basename, "wb");
			if(!local)
				/* TODO: diagnostic */
				return 1;
			ilocal = fileno(local);
			if(ilocal == -1){
				/* wat */
				ft->lasterr = errno;
				fclose(local);
				return 1;
			}
			if(callback(ft, FT_BEGIN, 0, size)){
				fclose(local);
				return 1;
			}
			state = RUNNING;
		}

		if(state == RUNNING){
			if(write(ilocal, buffer, nread) != nread)
				/* TODO: diagnostic */
				return 1;
			if(callback(ft, FT_TRANSFER, nbytes += nread, size)){
				/* cancelled */
				fclose(local);
				return 1;
			}
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
								basename = strdup(buffer + 5);
								if(!basename)
									return 1;
								ft_fname(ft) = basename;
								SHIFT_BUFFER();
								state = GOT_FILE;
							}else
								/* TODO: diagnostic */
								return 1;
							break;
						case GOT_FILE:
							if(!strncmp(buffer, "SIZE ", 5)){
								if(sscanf(buffer + 5, PRINTF_SIZET, &size) != 1)
									/* TODO: diagnostic */
									return 1;
								SHIFT_BUFFER();
								state = GOT_SIZE;
							}else
								/* TODO: diagnostic */
								return 1;
							break;
						case GOT_SIZE:
							if(buffer[0] == '\0'){
								state = GOT_ALL;
								SHIFT_BUFFER();
							}else
								/* TODO: diagnostic */
								return 1;
							goto check_buffer;
						case GOT_ALL:
						case RUNNING:
							return 1;
					}
				}else
					break;
			}
		}
	}
	/* unreachable */
}

int ft_send(struct filetransfer *ft, ft_callback callback, const char *fname)
{
	struct stat st;
	FILE *local;
	int ilocal, cancelled = 0;
	char buffer[BUFFER_SIZE];
	size_t nwrite, nsent;
	const char *basename = strrchr(fname, '/');

	local = fopen(fname, "rb");
	if(!local)
		/* TODO: diagnostic */
		return 1;

	ilocal = fileno(local);

	if(ilocal == -1)
		/* pretty much impossible */
		goto bail;

	if(fstat(ilocal, &st) == -1)
		/* TODO: diagnostic */
		goto bail;

	if(!basename)
		basename = fname;
	else
		basename++;

	ft_fname(ft) = basename;

	if((nwrite = snprintf(buffer, sizeof buffer,
					"FILE %s\nSIZE " PRINTF_SIZET "\n\n",
			basename, st.st_size)) >= (signed)sizeof buffer)
		/* FIXME TODO: diagnostic */
		goto bail;

	if(callback(ft, FT_BEGIN, 0, st.st_size)){
		/* cancel */
		cancelled = 1;
		goto complete;
	}

	/* so the numbers add up properly later on */
	nsent = -nwrite;

#ifndef USE_SENDFILE
	while(1){
#endif
		if(write(ft->sock, buffer, nwrite) == -1)
			/* TODO: diagnostic */
			goto bail;

		nsent += nwrite;
		if(callback(ft, FT_TRANSFER, nsent, st.st_size)){
			/* cancel */
			cancelled = 1;
			break;
		}

#ifdef USE_SENDFILE
		if(sendfile(ft->sock, ilocal, NULL, st.st_size) == -1)
			goto bail;
#else
		switch((nwrite = read(ilocal, buffer, sizeof buffer))){
			case 0:
				if(ferror(local))
					/* TODO: diagnostic */
					goto bail;
				/* FIXME: assert(nbytes == st.st_size); */
				if(callback(ft, FT_END, nsent, st.st_size)){
					/* cancel */
					cancelled = 1;
					goto complete;
				}
				goto complete;
			case -1:
				/* TODO: diagnostic */
				goto bail;
		}
	}
complete:
#endif

	if(fclose(local)){
		ft->lasterr = errno;
		return 1;
	}else
		return cancelled;
bail:
	return 1;
	fclose(local);
	return 1;
}

int ft_connect(struct filetransfer *ft, const char *host, const char *port)
{
	struct addrinfo hints, *ret = NULL, *dest = NULL;
	struct sockaddr_in addr;
	int lastconnerr = 0;

	DATA_INIT();

	ft_connected(ft) = 0;

	memset(&hints, '\0', sizeof hints);
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if((ft->lasterr = getaddrinfo(host, port, &hints, &ret))){
		ft->lasterr_isnet = 1;
		return 1;
	}

	for(dest = ret; dest; dest = dest->ai_next){
		ft->sock = socket(dest->ai_family, dest->ai_socktype,
				dest->ai_protocol);

		if(ft->sock == -1)
			continue;

		if(connect(ft->sock, dest->ai_addr, dest->ai_addrlen) == 0)
			break;

		lastconnerr = os_lasterr;
		close(ft->sock);
		ft->sock = -1;
	}

	freeaddrinfo(ret);

	if(ft->sock == -1){
		ft->lasterr = lastconnerr;
		return 1;
	}

	ft_connected(ft) = 1;
	return 0;
}

int ft_close(struct filetransfer *ft)
{
	int ret = 0;

	if(close(ft->sock)){
		ft->lasterr = os_lasterr;
		ret = 1;
	}

	ft->sock = -1;
	ft_connected(ft) = 0;

	return ret;
}

const char *ft_lasterr(struct filetransfer *ft)
{
#ifdef _WIN32
	static char errbuf[256];

	if(ft->lasterr_isnet)
		return gai_strerror(ft->lasterr);
	else{
		if(!ft->lasterr)
			return "libft: no error";

		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL,
				ft->lasterr, 0, errbuf, sizeof errbuf, NULL);
		return errbuf;
	}
#else
	return ft->lasterr_isnet ?
		gai_strerror(ft->lasterr) : strerror(ft->lasterr);
#endif
}

/*
static char *strdup2(const char *s)
{
	size_t len = strlen(s);
	char *d = malloc(len + 1);
	if(!d)
		return NULL;
	strcpy(d, s);
	return d;
}
*/
