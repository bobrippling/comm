#ifdef _WIN32
# define _WIN32_WINNT 0x501
# define close(x) closesocket(x)
/* ws2tcpip must be first... enterprise */
# include <ws2tcpip.h>
# include <winsock2.h>
/* ^ getaddrinfo */

#define os_getlasterr Win32_LastErr()
# define PRINTF_SIZET "%ld"

#else
# define _POSIX_C_SOURCE 200809L
# define _XOPEN_SOURCE   501
# include <sys/socket.h>
# include <arpa/inet.h>
# include <netdb.h>
# include <unistd.h>
# include <fcntl.h>
# include <sys/select.h>
# include <errno.h>

#define os_getlasterr strerror(errno)
/* needs cast to unsigned long */
# define PRINTF_SIZET "%lu"

#endif

/* can't use sendfile and have callbacks */

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

#define FT_ERR_PREMATURE_CLOSE "libft: Connection prematurely closed"
#define FT_ERR_TOO_MUCH        "libft: Too much data for file size"

#define BUFFER_SIZE            2048
#define CALLBACK_BYTES_STEP    (BUFFER_SIZE * 32)

#define STAT_SIZE_TYPE_CAST(x) ((unsigned long)(x))

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
			ft->lasterr = os_getlasterr; \
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

	if((ft->sock = socket(PF_INET, SOCK_STREAM, 0)) == -1){
		ft->lasterr = os_getlasterr;
		return 1;
	}

	addr.sin_port = htons(port);
	addr.sin_family = AF_INET;

#ifndef _WIN32
	save = 1;
	setsockopt(ft->sock, SOL_SOCKET, SO_REUSEADDR, &save, sizeof save);
#endif

	if(bind(ft->sock, (struct sockaddr *)&addr, sizeof addr) == -1){
		ft->lasterr = os_getlasterr;
		close(ft->sock);
		return 1;
	}

	if(listen(ft->sock, 1) == -1){
		ft->lasterr = os_getlasterr;
		close(ft->sock);
		return 1;
	}

	return 0;
}

int ft_accept(struct filetransfer *ft, int block)
{
	int new;

	ft->lasterr = NULL; /* must be checkable after this funcall */

	if(!block){
		fd_set fds;

		FD_ZERO(&fds);
		FD_SET(ft->sock, &fds);

		if(select(ft->sock+1, &fds, NULL, NULL, NULL) == -1){
			ft->lasterr = os_getlasterr;
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
#define RET(x) do{ ret = x; goto ret; }while(0)
#define INVALID_MSG() \
				do{\
					ft->lasterr = "libft: Invalid message recieved"; \
					RET(1); \
				}while(0)

	/* FIXME */
#define DEBUG(x, ...) printf(x "\n", __VA_ARGS__)

	/*
	 * expect:    "FILE fname"
	 *            "SIZE [0-9]+"
	 *            ""
	 */
	FILE *local = NULL;
	int ilocal, ret = 0;
	char *basename = NULL; /* dynamic alloc */
	char buffer[BUFFER_SIZE], *bufptr = buffer, *tok;
	size_t size, nbytes = 0;
	ssize_t nread; /* must allow -1 */

	ft->lastcallback = 0;

	/* first loop - read three newlines */
	nread = 0;
	for(;;){
		unsigned int nnewlines = 0;
		ssize_t i, thisread;

		/*
		 * -1 on buffer size, so tok can't overflow below
		 *  justified, since this recv() is called a negligible amount
		 *  compared to the data loop below
		 */
		thisread = recv(ft->sock, bufptr, sizeof buffer - (bufptr - buffer) - 1, 0);
		if(thisread == 0){
			ft->lasterr = FT_ERR_PREMATURE_CLOSE;
			RET(1);
		}else if(thisread == -1){
			ft->lasterr = os_getlasterr;
			RET(1);
		}

		nread += thisread;

		if((bufptr += thisread) > (buffer + sizeof buffer)){
			ft->lasterr = "libft: too much initialisation data";
			RET(1);
		}

		for(i = 0; i < nread; i++) /* could be more efficient, but eh */
			if(buffer[i] == '\n')
				if(++nnewlines == 3)
					break;

		if(nnewlines == 3)
			break; /* next stage */
		else{
			struct timeval tv;
			tv.tv_sec = 0;
			tv.tv_usec = 100000; /* 100ms */

			select(0, NULL, NULL, NULL, &tv);
			if(callback(ft, FT_WAIT, 0, 0)){
				/* cancelled */
				ft->lasterr = "Cancelled";
				RET(1);
			}
		}
	}/* first loop */

	tok = strtok(buffer, "\n");

	if(!strncmp(tok, "FILE ", 5)){
		basename = strdup(buffer + 5);
		if(!basename){
			ft->lasterr = os_getlasterr;
			RET(1);
		}
		DEBUG("GOT FILE: %s", basename);
		ft_fname(ft) = basename;
	}else
		INVALID_MSG();

	tok = strtok(NULL, "\n");

	if(!strncmp(tok, "SIZE ", 5)){
		if(sscanf(tok + 5, PRINTF_SIZET, (unsigned long *)&size) != 1){
			ft->lasterr = "libft: Invalid SIZE recieved";
			RET(1);
		}
		DEBUG("GOT SIZE: %ld", size);
	}else
		INVALID_MSG();


	tok = strchr(tok, '\0');
	/*
	 * FILE name\nSIZE 1234\n\n
	 *          |
	 *          V
	 * FILE name\0SIZE 1234\0\nDATA_GOES_HERE
	 *                     ^          ^-----^
	 *                     |             |
	 *                     |             |
	 * tok ----------------+   bufptr ---+ (anywhere)
	 *
	 * safe to check tok[1], since
	 * there are at least 3 \n:s
	 *
	 * tok[1] hasn't been changed by strtok
	 * yet either (but check both \n and \0
	 * for libc agnositicism
	 */

	if(!tok[1] || tok[1] == '\n'){
		/* FIXME: clobber check */
		local = fopen(basename, "wb");
		if(!local){
			ft->lasterr = os_getlasterr;
			RET(1);
		}
		ilocal = fileno(local);
		if(ilocal == -1){
			/* wat */
			ft->lasterr = os_getlasterr;
			RET(1);
		}
		if(callback(ft, FT_BEGIN, 0, size)){
			ft->lasterr = "Cancelled";
			RET(1);
		}
	}else
		INVALID_MSG();

	tok += 2;
	/*
	 * tok now points at the very first byte of data,
	 * (or the space where it will go)
	 */

	nread -= tok - buffer; /* much left over? */

	printf("nread: %ld\n", nread);

	/* shift the buffer */
	memmove(buffer, tok, tok - buffer);

	/*
	 * check if there's any tail on the buffer that needs writing
	 * with the initial write() in the loop
	 */
	for(;;){
		if(write(ilocal, buffer, nread) != nread){
			ft->lasterr = os_getlasterr;
			RET(1);
		}

		if(STAT_SIZE_TYPE_CAST(nbytes += nread) >= size)
			goto done;

		if(ft->lastcallback < nbytes){
			ft->lastcallback += CALLBACK_BYTES_STEP;
			if(callback(ft, FT_TRANSFER, nbytes, size)){
				/* cancelled */
				ft->lasterr = "Cancelled";
				fclose(local);
				RET(1);
			}
		}

		nread = recv(ft->sock, buffer, sizeof buffer, 0);

		if(nread == -1){
			ft->lasterr = os_getlasterr;
			RET(1);
		}else if(nread == 0){
done:
			/* check */
			if(nbytes < size)
				ft->lasterr = FT_ERR_PREMATURE_CLOSE;
			else if(nbytes > size)
				ft->lasterr = FT_ERR_TOO_MUCH;
			else{
				/* good so far */
				if(callback(ft, FT_END, nbytes, size)){
					ft->lasterr = "Cancelled";
					RET(1);
				}
				RET(0); /* XXX: function exit point here */
			}
			/* exit for too much/little */
			RET(1);
		}
	}
	/* unreachable */

ret:
	if(local){
		fclose(local);
		local = NULL;
	}
	if(basename){
		if(ret)
			remove(basename);
		free(basename);
		basename = NULL;
	}
	return ret;
#undef RET
#undef INVALID_MSG
}

int ft_send(struct filetransfer *ft, ft_callback callback, const char *fname)
{
	struct stat st;
	FILE *local;
	int ilocal, cancelled = 0;
	char buffer[BUFFER_SIZE];
	size_t nwrite, nsent;
	const char *basename = strrchr(fname, '/');

	ft->lastcallback = 0;

	local = fopen(fname, "rb");
	if(!local){
		ft->lasterr = os_getlasterr;
		return 1;
	}

	ilocal = fileno(local);

	if(ilocal == -1)
		/* pretty much impossible */
		goto bail;

	if(fstat(ilocal, &st) == -1){
		ft->lasterr = os_getlasterr;
		goto bail;
	}

	if(!basename)
		basename = fname;
	else
		basename++;

	ft_fname(ft) = basename;

	if((nwrite = snprintf(buffer, sizeof buffer,
					"FILE %s\nSIZE " PRINTF_SIZET "\n\n",
			basename, st.st_size)) >= (signed)sizeof buffer){
		ft->lasterr = "libft: Shorten your darn filename";
		goto bail;
	}

	/* less complicated to do it here than ^whileloop */
	if(write(ft->sock, buffer, nwrite) == -1){
		ft->lasterr = os_getlasterr;
		goto bail;
	}

	if(callback(ft, FT_BEGIN, 0, st.st_size)){
		/* cancel */
		ft->lasterr = "Cancelled";
		cancelled = 1;
		goto complete;
	}

	nsent = 0;
	while(1){
		switch((nwrite = read(ilocal, buffer, sizeof buffer))){
			case 0:
				if(ferror(local)){
					ft->lasterr = os_getlasterr;
					goto bail;
				}
				goto complete;
			case -1:
				ft->lasterr = os_getlasterr;
				goto bail;
		}

		if(write(ft->sock, buffer, nwrite) == -1){
			ft->lasterr = os_getlasterr;
			goto bail;
		}

		if((nsent += nwrite) >= STAT_SIZE_TYPE_CAST(st.st_size))
			break; /* goto complete */

		if(ft->lastcallback < nsent){
			ft->lastcallback += CALLBACK_BYTES_STEP;
			if(callback(ft, FT_TRANSFER, nsent, st.st_size)){
				/* cancel */
				cancelled = 1;
				ft->lasterr = "Cancelled";
				break;
			}
		}
	}

complete:
	if(nsent < STAT_SIZE_TYPE_CAST(st.st_size))
		ft->lasterr = FT_ERR_PREMATURE_CLOSE;
	else if(nsent > STAT_SIZE_TYPE_CAST(st.st_size)){
		ft->lasterr = FT_ERR_TOO_MUCH;
		fprintf(stderr, "libft: nsent: %ld, st.st_size: %ld\n",
				nsent, st.st_size);
	}else{
		if(callback(ft, FT_END, nsent, st.st_size)){
			/* cancel */
			cancelled = 1;
			ft->lasterr = "Cancelled";
			goto complete;
		}

		if(fclose(local)){
			ft->lasterr = os_getlasterr;
			return 1;
		}
		/* normal exit here */
		return cancelled;
	}
bail:
	fclose(local);
	return 1;
}

int ft_connect(struct filetransfer *ft, const char *host, const char *port)
{
	struct addrinfo hints, *ret = NULL, *dest = NULL;
	struct sockaddr_in addr;
	const char *lastconnerr = NULL;
	int eno;

	DATA_INIT();

	ft_connected(ft) = 0;

	memset(&hints, '\0', sizeof hints);
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if(!port)
		port = DEFAULT_PORT;

	if((eno = getaddrinfo(host, port, &hints, &ret))){
		ft->lasterr = gai_strerror(eno);
		return 1;
	}

	for(dest = ret; dest; dest = dest->ai_next){
		ft->sock = socket(dest->ai_family, dest->ai_socktype,
				dest->ai_protocol);

		if(ft->sock == -1)
			continue;

		if(connect(ft->sock, dest->ai_addr, dest->ai_addrlen) == 0)
			break;

		lastconnerr = os_getlasterr;
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

	if(ft->sock != -1){
		if(close(ft->sock)){
			ft->lasterr = os_getlasterr;
			ret = 1;
		}

		ft->sock = -1;
	}
	ft_connected(ft) = 0;

	return ret;
}

#ifdef _WIN32
const char *Win32_LastErr()
{
	static char errbuf[256];
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL,
			ft->lasterr, 0, errbuf, sizeof errbuf, NULL);
	return errbuf;
}
#endif

const char *ft_lasterr(struct filetransfer *ft)
{
	if(!ft->lasterr)
		return "libft: no error";
	return ft->lasterr;
}
