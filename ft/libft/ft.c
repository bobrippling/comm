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

# define WIN_DEBUG(x) fprintf(stderr, \
		"WIN_DEBUG(): " x ": %d (%d): %s\n", errno, \
		WSAGetLastError(), Win32_LastErr(0))

# define PRINTF_SIZET "%ld"
# define PRINTF_SIZET_CAST long

# define  os_getlasterr() Win32_LastErr(1)

# define net_getlasterr() Win32_LastErr(0)

# define G_DIR_SEPARATOR '\\'

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

# define G_DIR_SEPARATOR '/'

#endif

#define FT_LAST_ERR(s, n)    do{ ft->lasterr = s; ft->lasterrno = n; }while(0)
#define FT_LAST_ERR_NET()    FT_LAST_ERR(net_getlasterr(), errno)
#define FT_LAST_ERR_OS()     FT_LAST_ERR( os_getlasterr(), errno)
#define FT_LAST_ERR_CLEAR()  FT_LAST_ERR(NULL, 0)

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#ifdef _WIN32
int fileno(FILE *);
char *strdup(const char *);
int access(const char *, int);

const char *Win32_LastErr(int skip_to_errno);
void        Win32_check_name(char *);
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
			FT_LAST_ERR_NET(); \
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
			FT_LAST_ERR(FT_ERR_CANCELLED, 0); \
			exit_code; \
		} \
		ft_sleep(); \
	}while(0)

#define FREE_AND_NULL(x) do{ free(x); x = NULL; }while(0)

static size_t ft_getcallback_step(size_t);
static int ft_recv(struct filetransfer *, ft_callback callback, ft_queryback, ft_fnameback);
static void ft_sleep(void);

static int ft_get_meta(struct filetransfer *ft,
		char **basename, FILE **local, size_t *size,
		ft_callback, ft_queryback,
		ft_fnameback fnameback);


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
		FT_LAST_ERR_NET();
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
		FT_LAST_ERR_NET();
		close(ft->sock);
		return 1;
	}

	if(bind(ft->sock, (struct sockaddr *)&addr, sizeof addr) == -1){
		FT_LAST_ERR_NET();
		close(ft->sock);
		return 1;
	}

	if(listen(ft->sock, 1) == -1){
		FT_LAST_ERR_NET();
		close(ft->sock);
		return 1;
	}

	return 0;
}

enum ftret ft_accept(struct filetransfer *ft, const int block)
{
	struct timeval tv;
	fd_set fds;

	int new, save;
	socklen_t socklen;


	if(!block){
		FD_ZERO(&fds);
		FD_SET(ft->sock, &fds);

		tv.tv_sec  = 0;
		tv.tv_usec = 1000;

		switch(select(ft->sock + 1, &fds, NULL, NULL, &tv)){
			case -1:
				FT_LAST_ERR_OS();
				return FT_ERR;
			case 0:
				return FT_NO;
		}
		if(!FD_ISSET(ft->sock, &fds))
			return FT_NO;
	}

	FT_LAST_ERR_CLEAR();

	socklen = sizeof(struct sockaddr_in);
	new = accept(ft->sock,
			(struct sockaddr *)ft->addr,
			&socklen);
	save = errno;

	ft_connected(ft) = 1;

	close(ft->sock);
	ft->sock = new;
	return FT_YES;
}

static char *ft_rename(struct filetransfer *ft, char *basename)
{
	int id = 1, len = strlen(basename) + 5;
	char *dot, *new;

	dot = strchr(basename, '.');
	if(dot)
		*dot++ = '\0';

	new = malloc(strlen(basename) + strlen(dot ? dot : "") + 16);
	if(!new){
		FT_LAST_ERR("libft: couldn't allocate memory", 0);
		return NULL;
	}

	do{
		/* new = basename . "1" . (dot ? ".jpg" : "") */

		if(snprintf(new, len - 1, "%s%d%s%s", basename, id++, dot ? "." : "", dot ? dot : "") >= len - 1){
			FT_LAST_ERR("libft: basename overflow in snprintf()", 0);
			free(new);
			return NULL;
		}

		if(access(new, W_OK | R_OK) == -1 && errno == ENOENT)
			break;
		else if(id > 999){
			FT_LAST_ERR("libft: can't find gap in filesystem", 0);
			free(new);
			return NULL;
		}
	}while(1);

	/* get here on successful rename */
	return new;
}

static int ft_get_meta(struct filetransfer *ft,
		char **basename, FILE **local, size_t *size,
		ft_callback callback, ft_queryback queryback,
		ft_fnameback fnameback)
{
#define INVALID_MSG() \
				do{\
					FT_LAST_ERR(FT_ERR_INVALID_MSG, 0); \
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
			FT_LAST_ERR(FT_ERR_PREMATURE_CLOSE, 0);
			return 1;
		}else if(thisread == -1){
			FT_LAST_ERR_NET();
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
				FT_LAST_ERR("libft: meta data overflow (shorten your darn filename)", 0);
				return 1;
			}

			FT_SLEEP(return 1);
		}
	}while(1);

	thisread = bufptr - buffer + 1; /* truncate to how much is left */

	if(recv(ft->sock, buffer, thisread, 0) != thisread){
		FT_LAST_ERR_NET();
		return 1;
	}

	bufptr = strtok(buffer, "\n");
	if(!strncmp(bufptr, "FILE ", 5)){
		*basename = strdup(buffer + 5);
		if(!*basename){
			FT_LAST_ERR_OS();
			return 1;
		}
		ft_fname(ft) = *basename;
	}else
		INVALID_MSG();

	bufptr = strtok(NULL, "\n");
	if(!strncmp(bufptr, "SIZE ", 5)){
		if(sscanf(bufptr + 5, PRINTF_SIZET, (PRINTF_SIZET_CAST *)size) != 1){
			FT_LAST_ERR("libft: Invalid SIZE recieved", 0);
			FREE_AND_NULL(*basename);
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
		char *newfname;

#ifdef _WIN32
		Win32_check_name(*basename);
#endif

		newfname = fnameback(ft, *basename);

		if(!newfname){
			FREE_AND_NULL(*basename);
			FT_LAST_ERR(FT_ERR_CANCELLED, 0);
			return 1;
		}else if(newfname != *basename){
			FREE_AND_NULL(*basename);
			ft_fname(ft) = *basename = newfname;
		}

		if(access(*basename, W_OK | R_OK) == 0){
			/* file exists - clobber/resume check */
			switch(queryback(ft, FT_FILE_EXISTS, "%s exists", *basename, "Overwrite", "Resume", "Rename", NULL)){
				case 0: /* overwrite */
					/* just carry on */
					break;

				case 1: /* resume */
					resume = 1;
					break;

rename:
				case 2: /* rename */
					*basename = ft_rename(ft, *basename);
					if(!*basename)
						return 1;
					break;

				default:
					FT_LAST_ERR(FT_ERR_INVALID_MSG, 0);
					return 1;
			}/* switch querycallback */
		}else if(errno != ENOENT)
			/* access error */
			goto access_error;


		{
			char mode[4];
open_retry:
			if(resume)
				strcpy(mode, "a+b"); /* doesn't */
			else
				strcpy(mode, "wb"); /* truncates *basename */
			*local = fopen(*basename, mode);
		}
		if(!*local){
access_error:
			switch(queryback(ft, FT_CANT_OPEN, "Can't open %s: %s", *basename, os_getlasterr(),
					"Retry", "Rename", "Fail", NULL)){
				case 0:
					goto open_retry;
				case 1:
					goto rename;
				case 2:
				default:
					break;
			}
			FT_LAST_ERR_OS();
			FREE_AND_NULL(*basename);
			return 1;
		}
		if(resume)
			if(fseek(*local, 0, SEEK_END)){
				FT_LAST_ERR_OS();
				FREE_AND_NULL(*basename);
				fclose(*local);
				return 1;
			}/* else leave fpointer at the end for ftell() below */
	}else
		INVALID_MSG();

	return 0;
#undef INVALID_MSG
}

#define PING_PONG(fname, msg) \
int ft_#fname(struct filetransfer *ft) \
{ \
	const char buffer[] = msg "\n"; \
	return send(ft->sock, buffer, sizeof buffer, 0) != -1; \
}

PING_PONG(ping, "PING")
PING_PONG(pong, "PONG")

int ft_handle(struct filetransfer *ft,
		ft_callback  callback,
		ft_queryback queryback,
		ft_fnameback fnameback)
{
	/* check for PING or FILE */
	char buffer[16], *nl;
	ssize_t thisread;
	int nloops = 5;


	do{
		thisread = recv(ft->sock, buffer, sizeof buffer, MSG_PEEK);
		switch(thisread){
			case 0:
				FT_LAST_ERR(FT_ERR_PREMATURE_CLOSE, 0);
				return 1;
			case -1:
				FT_LAST_ERR();
				return 1;
		}
		if(nl = strchr(buffer, '\n'))
			break;
		else if(--nloops <= 0)
			return 0;
	}while(1);

	*nl = '\0';
	if(!strcmp(buffer, "PING")){
		recv(ft->sock, buffer, nl - buffer + 1, MSG_PEEK);
		return ft_pong(ft);
	}else
		/* assume transfer */
		return ft_re(ft, callback, que, fnameback);
}


static int ft_recv(struct filetransfer *ft,
		ft_callback callback,
		ft_queryback queryback,
		ft_fnameback fnameback)
{
#define INVALID_MSG() \
				do{\
					FT_LAST_ERR(FT_ERR_INVALID_MSG, 0); \
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
	if(ft_get_meta(ft, &basename, &local, &size,
				callback, queryback, fnameback))
		RET(1);

	ft_fname(ft) = basename;

	callback_step = ft_getcallback_step(size);

	{
		long tmp = ftell(local);
		size_so_far = tmp;
		if(tmp == -1){
			FT_LAST_ERR_OS();
			RET(1);
		}else if(size_so_far == size){
			FT_LAST_ERR("Can't resume - have full file", 0);
			RET(1);
		}
	}

	/* resuming (works with 0) */
	snprintf(buffer, sizeof(buffer), "RESUME " PRINTF_SIZET "\n",
			(PRINTF_SIZET_CAST)size_so_far);
	if(send(ft->sock, buffer, strlen(buffer), 0) == -1){
		FT_LAST_ERR_NET();
		RET(1);
	}

	if(callback(ft, FT_BEGIN_RECV, 0, size)){
		FT_LAST_ERR(FT_ERR_CANCELLED, 0);
		RET(1);
	}

	for(;;){
		nread = recv(ft->sock, buffer, sizeof buffer, 0);

		if(nread == -1){
			FT_LAST_ERR_NET();
			RET(1);
		}else if(nread == 0){
done:
			/* check */
			if(size_so_far < size)
				FT_LAST_ERR(FT_ERR_PREMATURE_CLOSE, 0);
			else if(size_so_far > size)
				FT_LAST_ERR(FT_ERR_TOO_MUCH, 0);
			else{
				/* good so far, don't be a douche and cancel it.. */
				if(callback(ft, FT_RECIEVED, size_so_far, size)){
					/* ... douche */
					FT_LAST_ERR(FT_ERR_CANCELLED, 0);
					RET(1);
				}
				/* send OK\n */
				if(send(ft->sock, "OK\n", 3, 0) == -1){
					FT_LAST_ERR_NET();
					RET(1);
				}
				RET(0); /* XXX: function exit point here */
			}
			/* exit for too much/little */
			RET(1);
		}

		if((signed)fwrite(buffer, sizeof(char),
					nread, local) != nread /* !*sizeof char */){
			FT_LAST_ERR_OS();
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
				FT_LAST_ERR(FT_ERR_CANCELLED, 0);
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

enum ftret ft_poll_recv_or_close(struct filetransfer *ft)
{
	struct timeval tv;
	fd_set fds;

	FD_ZERO(&fds);
	FD_SET(ft->sock, &fds);

	tv.tv_sec  = 0;
	tv.tv_usec = 1000;

	switch(select(ft->sock + 1, &fds, NULL, NULL, &tv)){
		case -1:
			FT_LAST_ERR_OS();
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
	const char *basename  = strrchr(fname, G_DIR_SEPARATOR),
				     *basename2 = strrchr(fname, G_DIR_SEPARATOR == '/' ? '\\' : '/');

	if(!basename || basename < basename2)
		basename = basename2;

	ft->lastcallback = 0;

	local = fopen(fname, "rb");
	if(!local){
		WIN_DEBUG("fopen()");
		FT_LAST_ERR_OS();
		return 1;
	}

	ilocal = fileno(local);

	if(ilocal == -1){
		/* pretty much impossible */
		WIN_DEBUG("fileno()");
		FT_LAST_ERR_OS();
		goto bail;
	}

	if(fstat(ilocal, &st) == -1){
		WIN_DEBUG("fstat()");
		FT_LAST_ERR_OS();
		goto bail;
	}else if(st.st_size == 0){
		WIN_DEBUG("zero len file");
		FT_LAST_ERR("libft: Can't send zero-length file", 0);
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
		WIN_DEBUG("long fname");
		FT_LAST_ERR("libft: Shorten your darn filename", 0);
		goto bail;
	}

	/* less complicated to do it here than ^whileloop */
	if(send(ft->sock, buffer, nwrite, 0) == -1){
		WIN_DEBUG("ft_send(): inital write");
		FT_LAST_ERR_NET();
		goto bail;
	}

#define BUFFER_RECV(len, flag) \
		switch((nwrite = recv(ft->sock, buffer, len, flag))){ \
			case -1: \
				WIN_DEBUG("ft_send(): BUFFER_RECV recv()"); \
				FT_LAST_ERR_NET(); \
				goto bail; \
			case 0: \
				WIN_DEBUG("ft_send(): BUFFER_RECV premature"); \
				FT_LAST_ERR(FT_ERR_PREMATURE_CLOSE, 0); \
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
		WIN_DEBUG("invalid RESUME");
		FT_LAST_ERR("libft: Invalid RESUME message", 0);
		goto bail;
	}

	if(nsent)
		if(fseek(local, nsent, SEEK_SET) == -1){
			WIN_DEBUG("fseek()");
			FT_LAST_ERR_OS();
			goto bail;
		}

	if(callback(ft, FT_BEGIN_SEND, 0, st.st_size)){
		/* cancel */
		FT_LAST_ERR(FT_ERR_CANCELLED, 0);
		cancelled = 1;
		goto complete;
	}

	while(1){
		switch((nwrite = fread(buffer, sizeof(char), sizeof buffer, local))){
			case 0:
				if(ferror(local)){
					WIN_DEBUG("ferror()");
					FT_LAST_ERR_NET();
					goto bail;
				}
				goto complete;
			case -1:
				WIN_DEBUG("fread()");
				FT_LAST_ERR_NET();
				goto bail;
		}

		if(send(ft->sock, buffer, nwrite, 0) == -1){
			WIN_DEBUG("send()");
			FT_LAST_ERR_NET();
			goto bail;
		}

		if((nsent += nwrite) >= (unsigned)st.st_size)
			break; /* goto complete */

		if(ft->lastcallback < nsent){
			ft->lastcallback += callback_step;
			if(callback(ft, FT_SENDING, nsent, st.st_size)){
				/* cancel */
				cancelled = 1;
				FT_LAST_ERR(FT_ERR_CANCELLED, 0);
				break;
			}
		}
	}

complete:
	if(nsent < (unsigned)st.st_size)
		FT_LAST_ERR(FT_ERR_PREMATURE_CLOSE, 0);
	else if(nsent > (unsigned)st.st_size){
		FT_LAST_ERR(FT_ERR_TOO_MUCH, 0);
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
			WIN_DEBUG("\"OK\" not recv()'d");
			FT_LAST_ERR(FT_ERR_INVALID_MSG, 0);
			goto bail;
		}

		if(callback(ft, FT_SENT, nsent, st.st_size)){
			/* cancel */
			cancelled = 1;
			FT_LAST_ERR(FT_ERR_CANCELLED, 0);
			goto bail;
		}

		if(fclose(local)){
			WIN_DEBUG("fclose()");
			FT_LAST_ERR_OS();
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
		FT_LAST_ERR(gai_strerror(eno), eno);
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
		FT_LAST_ERR(lastconnerr, 0);
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
			FT_LAST_ERR_OS();
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

void Win32_check_name(char *s)
{
	for(; *s; s++)
		switch(*s){
			case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05:
			case 0x06: case 0x07: case 0x08: case 0x09: case 0x0a: case 0x0b:
			case 0x0c: case 0x0d: case 0x0e: case 0x0f: case 0x10: case 0x11:
			case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
			case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d:
			case 0x1e: case 0x1f:
			case '"':
			case '*':
			case '/':
			case ':':
			case '<':
			case '>':
			case '?':
			case '|':
			case '\\':
				*s = '_';
		}
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
	const char *slash = strrchr(fname, G_DIR_SEPARATOR);

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

int ft_poll_connected(struct filetransfer *ft)
{
	char dummy;

	if(recv(ft->sock, &dummy, sizeof dummy, MSG_PEEK) == 0)
		return 0;

	return 1;
}
