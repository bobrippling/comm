#ifdef _WIN32
# define _WIN32_WINNT 0x501
# define close(x) closesocket(x)

/* ws2tcpip must be first... enterprise */
# include <ws2tcpip.h>
# include <winsock2.h>
/* ^ getaddrinfo */

# include <sys/types.h>
# include <sys/stat.h>
/*
 * can't do this:
 * # include <io.h>
 * since closesocket conflicts... enterprise
 */
# include <errno.h>
# define W_OK 02
# define R_OK 04

# define WIN_DEBUG(x) perror("WIN_DEBUG(): " x )

# define PRINTF_SIZET "%ld"
# define PRINTF_SIZET_CAST long
# define PATH_SEPERATOR '\\'

# define  os_getlasterr() Win32_LastErr(1)
# define net_getlasterr() Win32_LastErr(0)

#else
# define _POSIX_C_SOURCE 200809L
# define _XOPEN_SOURCE   501
# include <sys/socket.h>
# include <arpa/inet.h>
# include <netdb.h>
# include <unistd.h>
# include <fcntl.h>
# include <errno.h>
# include <sys/select.h>

# define WIN_DEBUG(x)

# define  os_getlasterr() strerror(errno)
# define net_getlasterr() strerror(errno)

/* needs cast to unsigned long */
# define PRINTF_SIZET "%lu"
# define PRINTF_SIZET_CAST unsigned long

# define PATH_SEPERATOR '/'

#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#ifdef _WIN32
int fileno(FILE *);
char *strdup(const char *);
int access(const char *, int);

const char *Win32_LastErr(int skip_to_errno);
static int WSA_Startuped = 0;
#endif

#include "ft.h"

#define FT_ERR_PREMATURE_CLOSE "libft: Connection prematurely closed"
#define FT_ERR_TOO_MUCH        "libft: Too much data for file size"
#define FT_ERR_INVALID_MSG     "libft: Invalid message recieved"
#define FT_ERR_CANCELLED       "Cancelled"

#define BUFFER_SIZE            512
#define LINGER_TIME            20


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
			ft->lasterr = net_getlasterr(); \
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

#define FT_SLEEP(exit_code) \
	do{ \
		if(callback(ft, FT_WAIT, 0, 1)){ \
			ft->lasterr = FT_ERR_CANCELLED; \
			exit_code; \
		} \
		ft_sleep(); \
	}while(0)

static size_t ft_getcallback_step(size_t);
static void ft_sleep(void);

static size_t ft_getcallback_step(size_t siz)
{
	size_t ret;
	/* callback every 2% */
	ret = ((float)siz * 2.0f / 100.0f);

	if(ret < 1)
		ret = 1;
	else if(ret > BUFFER_SIZE * 4)
		ret = BUFFER_SIZE * 4;
	return ret;
}

void ft_zero(struct filetransfer *ft)
{
	memset(ft, 0, sizeof *ft);
}

int ft_listen(struct filetransfer *ft, int port)
{
	int save;
	struct sockaddr_in addr;
	struct linger l;

	DATA_INIT();

	if((ft->sock = socket(PF_INET, SOCK_STREAM, 0)) == -1){
		ft->lasterr = net_getlasterr();
		return 1;
	}

	addr.sin_port = htons(port);
	addr.sin_family = AF_INET;

#ifdef _WIN32
# define CAST (const char *)
#else
# define CAST
#endif

	save = 1;
	setsockopt(ft->sock, SOL_SOCKET, SO_REUSEADDR, CAST &save, sizeof save);
	/* discard ret */

	l.l_onoff  = 1; /* do linger, G */
	l.l_linger = LINGER_TIME; /* time (seconds) */
	if(setsockopt(ft->sock, SOL_SOCKET, SO_LINGER, CAST &l, sizeof l)){
		ft->lasterr = net_getlasterr();
		close(ft->sock);
		return 1;
	}

	if(bind(ft->sock, (struct sockaddr *)&addr, sizeof addr) == -1){
		ft->lasterr = net_getlasterr();
		close(ft->sock);
		return 1;
	}

	if(listen(ft->sock, 1) == -1){
		ft->lasterr = net_getlasterr();
		close(ft->sock);
		return 1;
	}

	return 0;
}

enum ftret ft_accept(struct filetransfer *ft, const int block)
{
	int new, save;
	socklen_t socklen;
#ifndef _WIN32
	int flags;
#endif

	ft->lasterr = NULL; /* must be checkable after this funcall */

	if(!block){
#ifdef _WIN32
		u_long m = 0;
		if(ioctlsocket(ft->sock, FIONBIO, &m) != 0){
			ft->lasterr = net_getlasterr();
			return FT_ERR;
		}
#else
		flags = fcntl(ft->sock, F_GETFL);
		if(fcntl(ft->sock, F_SETFL, flags | O_NONBLOCK) == -1){
			ft->lasterr = net_getlasterr();
			return FT_ERR;
		}
#endif
	}

	socklen = sizeof(struct sockaddr_in);
	new = accept(ft->sock,
			(struct sockaddr *)ft->addr,
			&socklen);
	save = errno;

	if(!block){
#ifdef _WIN32
		u_long m = 1;
		if(ioctlsocket(ft->sock, FIONBIO, &m) != 0){
			ft->lasterr = net_getlasterr();
			return FT_ERR;
		}
#else
		if(fcntl(ft->sock, F_SETFL, flags & ~O_NONBLOCK) == -1){
			ft->lasterr = net_getlasterr();
			return FT_ERR;
		}
#endif
	}
	errno = save;

	if(new == -1){
		if(
#ifdef _WIN32
				errno == WSAEWOULDBLOCK
#else
				errno == EAGAIN || errno == EWOULDBLOCK
#endif
				)
			return FT_NO;
		else{
			ft->lasterr = net_getlasterr();
			return FT_ERR;
		}
	}

	ft_connected(ft) = 1;

	close(ft->sock);
	ft->sock = new;
	return FT_YES;
}

static int ft_get_meta(struct filetransfer *ft,
		char **basename, FILE **local, size_t *size,
		ft_callback, ft_queryback);

static int ft_get_meta(struct filetransfer *ft,
		char **basename, FILE **local, size_t *size,
		ft_callback callback, ft_queryback queryback)
{
#define INVALID_MSG() \
				do{\
					ft->lasterr = FT_ERR_INVALID_MSG; \
					return 1; \
				}while(0)
	char buffer[2048], *bufptr;
	ssize_t thisread;
	int nnewlines;

	ft->lastcallback = 0;

	do{
		/* read three newlines */
		thisread = recv(ft->sock, buffer, sizeof buffer, MSG_PEEK);

		if(thisread == 0){
			ft->lasterr = FT_ERR_PREMATURE_CLOSE;
			return 1;
		}else if(thisread == -1){
			ft->lasterr = net_getlasterr();
			return 1;
		}

		nnewlines = 0;
		for(bufptr = buffer;
				bufptr - buffer < (signed)sizeof buffer;
				++bufptr)
			if(*bufptr == '\n')
				if(++nnewlines == 3)
					break;

		if(nnewlines == 3)
			break;
		else{
			if((unsigned)thisread > sizeof(buffer) - 32){
				ft->lasterr = "libft: meta data overflow (shorten your darn filename)";
				return 1;
			}

			FT_SLEEP(return 1);
		}
	}while(1);

	thisread = bufptr - buffer + 1; /* truncate to how much is left */

	if(recv(ft->sock, buffer, thisread, 0) != thisread){
		ft->lasterr = net_getlasterr();
		return 1;
	}

	bufptr = strtok(buffer, "\n");
	if(!strncmp(bufptr, "FILE ", 5)){
		*basename = strdup(buffer + 5);
		if(!*basename){
			ft->lasterr = os_getlasterr();
			return 1;
		}
		ft_fname(ft) = *basename;
	}else
		INVALID_MSG();

	bufptr = strtok(NULL, "\n");
	if(!strncmp(bufptr, "SIZE ", 5)){
		if(sscanf(bufptr + 5, PRINTF_SIZET, (PRINTF_SIZET_CAST *)size) != 1){
			ft->lasterr = "libft: Invalid SIZE recieved";
			free(*basename);
			return 1;
		}
	}else
		INVALID_MSG();


	bufptr = strchr(bufptr, '\0');
	/*
	 * FILE name\nSIZE 1234\n\n
	 *          |
	 *          V
	 * FILE name\0SIZE 1234\0\nDATA_GOES_HERE
	 *                     ^   ^-------+
	 *                     |           |
	 *                     |           |
	 * bufptr ----------------+   bufptr -+
	 *
	 * safe to check bufptr[1], since
	 * there are at least 3 \n:s
	 *
	 * bufptr[1] hasn't been changed by strtok
	 * yet either (but check both \n and \0
	 * for libc agnositicism
	 */

	if(!bufptr[1] || bufptr[1] == '\n'){
		int resume = 0;

		if(access(*basename, W_OK | R_OK) == 0)
			/* file exists - clobber/resume check */
			switch(queryback(ft, "%s exists", *basename, "Overwrite", "Resume", "Rename", NULL)){
				case 0: /* overwrite */
					/* just carry on */
					break;

				case 1: /* resume */
					resume = 1;
					break;

				case 2: /* rename */
				{
					int clob = 1, len = strlen(*basename) + 5;
					char *tmp = realloc(*basename, len + 1), *dup;

					if(!tmp){
						ft->lasterr = os_getlasterr();
						free(*basename);
						return 1;
					}
					dup = strdup(*basename = tmp);
					if(!dup){
						ft->lasterr = os_getlasterr();
						free(tmp);
						return 1;
					}

					do{
						if(snprintf(tmp, len - 1, "%s%d", dup, clob++) >= len - 1){
							ft->lasterr = "libft: basename overflow in snprintf()";
							free(tmp);
							free(dup);
							return 1;
						}

						if(access(tmp, W_OK | R_OK) == -1 && errno == ENOENT)
							break;
						else if(clob > 99){
							ft->lasterr = "libft: can't find gap in filesystem";
							free(tmp);
							free(dup);
							return 1;
						}
					}while(1);
					/* get here on successful rename */
					free(dup);
					/* *basename free'd later */
					break;
				}

				default:
					ft->lasterr = FT_ERR_INVALID_MSG;
					return 1;
			}/* switch querycallback */
		else if(errno != ENOENT){
			/* access error */
			ft->lasterr = os_getlasterr();
			free(*basename);
			return 1;
		}


		{
			char mode[4];
			if(resume)
				strcpy(mode, "a+b");
			else
				strcpy(mode, "wb");
			*local = fopen(*basename, mode); /* truncates *basename */
		}
		if(!*local){
			ft->lasterr = os_getlasterr();
			free(*basename);
			return 1;
		}
		if(resume)
			if(fseek(*local, 0, SEEK_END)){
				ft->lasterr = os_getlasterr();
				free(*basename);
				fclose(*local);
				return 1;
			}/* else leave fpointer at the end for ftell() below */
	}else
		INVALID_MSG();

	return 0;
#undef INVALID_MSG
}

int ft_recv(struct filetransfer *ft, ft_callback callback, ft_queryback queryback)
{
#define INVALID_MSG() \
				do{\
					ft->lasterr = FT_ERR_INVALID_MSG; \
					RET(1); \
				}while(0)
#define RET(x) do{ ret = x; goto ret; }while(0)

	/*
	 * expect:    "FILE fname"
	 *            "SIZE [0-9]+"
	 *            ""
	 */
	FILE *local = NULL;
	int ret = 0;
	char buffer[BUFFER_SIZE], *basename = NULL;
	size_t size, size_so_far = 0, callback_step;
	ssize_t nread;

	ft_fname(ft) = NULL;
	if(ft_get_meta(ft, &basename, &local, &size, callback, queryback))
		RET(1);
	ft_fname(ft) = basename;

	callback_step = ft_getcallback_step(size);

	{
		long tmp = ftell(local);
		size_so_far = tmp;
		if(tmp == -1){
			ft->lasterr = os_getlasterr();
			RET(1);
		}else if(size_so_far == size){
			ft->lasterr = "Can't resume - have full file";
			RET(1);
		}
	}

	/* resuming (works with 0) */
	snprintf(buffer, sizeof(buffer), "RESUME " PRINTF_SIZET "\n",
			(PRINTF_SIZET_CAST)size_so_far);
	if(send(ft->sock, buffer, strlen(buffer), 0) == -1){
		ft->lasterr = net_getlasterr();
		RET(1);
	}

	if(callback(ft, FT_BEGIN_RECV, 0, size)){
		ft->lasterr = FT_ERR_CANCELLED;
		RET(1);
	}

	for(;;){
		nread = recv(ft->sock, buffer, sizeof buffer, 0);

		if(nread == -1){
			ft->lasterr = net_getlasterr();
			RET(1);
		}else if(nread == 0){
done:
			/* check */
			if(size_so_far < size)
				ft->lasterr = FT_ERR_PREMATURE_CLOSE;
			else if(size_so_far > size)
				ft->lasterr = FT_ERR_TOO_MUCH;
			else{
				/* good so far, don't be a douche and cancel it.. */
				if(callback(ft, FT_RECIEVED, size_so_far, size)){
					/* ... douche */
					ft->lasterr = FT_ERR_CANCELLED;
					RET(1);
				}
				/* send OK\n */
				if(send(ft->sock, "OK\n", 3, 0) == -1){
					ft->lasterr = net_getlasterr();
					RET(1);
				}
				RET(0); /* XXX: function exit point here */
			}
			/* exit for too much/little */
			RET(1);
		}

		if((signed)fwrite(buffer, sizeof(char),
					nread, local) != nread /* !*sizeof char */){
			ft->lasterr = os_getlasterr();
			RET(1);
		}

		/* no need to have this above, since it's only 0/-1 */
		size_so_far += nread;
		if(size_so_far >= size)
			/*
			 * on normal operation, the == above will be true
			 * when we've recieved the entire file,
			 * so we won't end up blocking at the main recv()
			 * yay
			 */
			goto done;

		if(ft->lastcallback < size_so_far){
			ft->lastcallback += callback_step;
			if(callback(ft, FT_RECIEVING, size_so_far, size)){
				/* cancelled */
				ft->lasterr = FT_ERR_CANCELLED;
				RET(1);
			}
		}
	}
	/* unreachable */

ret:
	if(local){
		fclose(local);
		local = NULL;
	}
	if(basename){
		free(basename);
		ft_fname(ft) = NULL;
	}
	return ret;
#undef RET
#undef INVALID_MSG
}

enum ftret ft_poll_recv(struct filetransfer *ft)
{
	struct timeval tv;
	fd_set fds;

	FD_ZERO(&fds);
	FD_SET(ft->sock, &fds);

	tv.tv_sec  = 0;
	tv.tv_usec = 1000;

	switch(select(ft->sock + 1, &fds, NULL, NULL, &tv)){
		case -1:
			ft->lasterr = os_getlasterr();
			return FT_ERR;
		case 0:
			return FT_NO;
	}
	return FD_ISSET(ft->sock, &fds) ? FT_YES : FT_NO;
}

static void ft_sleep()
{
	struct timeval tv;

	/* sleep while we wait */
	tv.tv_sec  = 0;
	tv.tv_usec = 1000000;
	select(0, NULL, NULL, NULL, &tv);
}

int ft_send(struct filetransfer *ft, ft_callback callback, const char *fname)
{
	struct stat st;
	FILE *local;
	int ilocal, cancelled = 0;
	char buffer[BUFFER_SIZE];
	size_t nwrite, nsent, callback_step;
	const char *basename = strrchr(fname, PATH_SEPERATOR);

	ft->lastcallback = 0;

	local = fopen(fname, "rb");
	if(!local){
		WIN_DEBUG("fopen()");
		ft->lasterr = os_getlasterr();
		return 1;
	}

	ilocal = fileno(local);

	if(ilocal == -1){
		/* pretty much impossible */
		WIN_DEBUG("fileno()");
		ft->lasterr = os_getlasterr();
		goto bail;
	}

	if(fstat(ilocal, &st) == -1){
		WIN_DEBUG("fstat()");
		ft->lasterr = os_getlasterr();
		goto bail;
	}else if(st.st_size == 0){
		ft->lasterr = "libft: Can't send zero-length file";
		goto bail;
	}

	callback_step = ft_getcallback_step(st.st_size);

	if(!basename)
		basename = fname;
	else
		basename++;

	ft_fname(ft) = basename;

	if((nwrite = snprintf(buffer, sizeof buffer,
					"FILE %s\nSIZE " PRINTF_SIZET "\n\n",
			basename, (PRINTF_SIZET_CAST)st.st_size)) >= (signed)sizeof buffer){
		ft->lasterr = "libft: Shorten your darn filename";
		goto bail;
	}

	/* less complicated to do it here than ^whileloop */
	if(send(ft->sock, buffer, nwrite, 0) == -1){
		ft->lasterr = net_getlasterr();
		goto bail;
	}

#define BUFFER_RECV(len, flag) \
		switch((nwrite = recv(ft->sock, buffer, len, flag))){ \
			case -1: \
				ft->lasterr = net_getlasterr(); \
				goto bail; \
			case 0: \
				ft->lasterr = FT_ERR_PREMATURE_CLOSE; \
				goto bail; \
		}

	/* wait for RESUME reply */
	do{
		char *nl;

		BUFFER_RECV(sizeof(buffer), MSG_PEEK);

		nl = memchr(buffer, '\n', nwrite);
		if(nl){
			BUFFER_RECV(nl - buffer + 1, 0);
			break;
		}
		FT_SLEEP(goto bail);
	}while(1);

	if(sscanf(buffer, "RESUME " PRINTF_SIZET "\n",
				(PRINTF_SIZET_CAST *)&nsent) != 1){
		ft->lasterr = "libft: Invalid RESUME message";
		goto bail;
	}

	if(nsent)
		if(fseek(local, nsent, SEEK_SET) == -1){
			ft->lasterr = os_getlasterr();
			goto bail;
		}

	if(callback(ft, FT_BEGIN_SEND, 0, st.st_size)){
		/* cancel */
		ft->lasterr = FT_ERR_CANCELLED;
		cancelled = 1;
		goto complete;
	}

	while(1){
		switch((nwrite = fread(buffer, sizeof(char), sizeof buffer, local))){
			case 0:
				if(ferror(local)){
					ft->lasterr = net_getlasterr();
					goto bail;
				}
				goto complete;
			case -1:
				ft->lasterr = net_getlasterr();
				goto bail;
		}

		if(send(ft->sock, buffer, nwrite, 0) == -1){
			ft->lasterr = net_getlasterr();
			goto bail;
		}

		if((nsent += nwrite) >= (unsigned)st.st_size)
			break; /* goto complete */

		if(ft->lastcallback < nsent){
			ft->lastcallback += callback_step;
			if(callback(ft, FT_SENDING, nsent, st.st_size)){
				/* cancel */
				cancelled = 1;
				ft->lasterr = FT_ERR_CANCELLED;
				break;
			}
		}
	}

complete:
	if(nsent < (unsigned)st.st_size)
		ft->lasterr = FT_ERR_PREMATURE_CLOSE;
	else if(nsent > (unsigned)st.st_size){
		ft->lasterr = FT_ERR_TOO_MUCH;
		fprintf(stderr, "libft: nsent: " PRINTF_SIZET
				", st.st_size: " PRINTF_SIZET "\n",
				(PRINTF_SIZET_CAST)nsent, (PRINTF_SIZET_CAST)st.st_size);
	}else{
		/* wait until they send an OK */
		do{
			char *nl;

			BUFFER_RECV(sizeof(buffer), MSG_PEEK);

			nl = memchr(buffer, '\n', nwrite);
			if(nl){
				BUFFER_RECV(nl - buffer + 1, 0);
				break;
			}
			FT_SLEEP(goto bail);
		}while(1);

		if(strncmp(buffer, "OK\n", 3)){
			ft->lasterr = FT_ERR_INVALID_MSG;
			goto bail;
		}

		if(callback(ft, FT_SENT, nsent, st.st_size)){
			/* cancel */
			cancelled = 1;
			ft->lasterr = FT_ERR_CANCELLED;
			goto bail;
		}

		if(fclose(local)){
			ft->lasterr = os_getlasterr();
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
		port = FT_DEFAULT_PORT;

	if((eno = getaddrinfo(host, port, &hints, &ret))){
		ft->lasterr = gai_strerror(eno);
		return 1;
	}

	for(dest = ret; dest; dest = dest->ai_next){
		ft->sock = socket(dest->ai_family, dest->ai_socktype,
				dest->ai_protocol);

		if(ft->sock == -1)
			continue;

		if(connect(ft->sock, dest->ai_addr, dest->ai_addrlen) == 0){
			memcpy(ft->addr, dest->ai_addr, sizeof(struct sockaddr_in));
			break;
		}

		lastconnerr = net_getlasterr();
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
#ifdef _WIN32
# define SHUTDOWN_FLAG SD_SEND
#else
# define SHUTDOWN_FLAG SHUT_WR
#endif
		shutdown(ft->sock, SHUTDOWN_FLAG);

		if(close(ft->sock)){
			ft->lasterr = os_getlasterr();
			ret = 1;
		}

		ft->sock = -1;
	}
	ft_connected(ft) = 0;

	return ret;
}

#ifdef _WIN32
const char *Win32_LastErr(int skip_to_errno)
{
	static char errbuf[256];
	int ecode;

	if(skip_to_errno)
		ecode = errno;
	else if(!(ecode = WSAGetLastError()))
		ecode = errno;

	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL,
			ecode, 0,
			errbuf, sizeof errbuf, NULL);
	return errbuf;
}
#endif

const char *ft_lasterr(struct filetransfer *ft)
{
	if(!ft->lasterr)
		return "libft: no error";
	return ft->lasterr;
}

const char *ft_basename(struct filetransfer *ft)
{
	const char *fname = ft_fname(ft);
	const char *slash = strrchr(fname, PATH_SEPERATOR);

	if(slash)
		return slash+1;
	return fname;
}

const char *ft_remoteaddr(struct filetransfer *ft)
{
	static char buf[128];

	if(getnameinfo((struct sockaddr *)ft->addr,
			sizeof(struct sockaddr_in),
			buf, sizeof buf,
			NULL, 0, /* no service */
			0 /* flags */))
		return NULL;
	return buf;
}
