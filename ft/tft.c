#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#ifdef _WIN32
# include <winsock2.h>
/* wtf */
#else
# include <sys/select.h>
# include <sys/stat.h>
# include <fcntl.h>
# include <signal.h>
#endif

#include "libft/ft.h"

#define MAX(x, y) (x > y ? x : y)
#undef  TFT_DIE_ON_SEND

#ifdef FT_USE_PING
# define PING_DELAY 20
#endif

void  clrtoeol(void);
void  cleanup(void);
int   eprintf(const char *, ...);
void  check_files(int fname_idx, int argc, char **argv);
char *readline(int nulsep);
int   tft_send(const char *fname);
int   callback(struct filetransfer *ft, enum ftstate state,
        size_t bytessent, size_t bytestotal);
void sigh(int sig);

int recursive = 0;
struct filetransfer ft;
enum { OVERWRITE, RESUME, RENAME, ASK } clobber_mode = ASK;


int tft_send(const char *fname)
{
	printf("tft: sending %s\n", fname);
	return ft_send(&ft, callback, fname, recursive);
}

int callback(struct filetransfer *ft, enum ftstate state,
		size_t bytessent, size_t bytestotal)
{
	if(state == FT_SENT || state == FT_RECIEVED){
		clrtoeol();
		printf("tft: done: %s\n", ft_fname(ft));
		return 0;
	}else if(state == FT_WAIT)
		return 0;

	printf("\"%s\": %zd K / %zd K (%2.2f)%%\r", ft_basename(ft),
			bytessent / 1024, bytestotal / 1024,
			(float)(100.0f * bytessent / bytestotal));

	return 0;
}

int queryback(struct filetransfer *ft, enum ftquery querytype, const char *msg, ...)
{
	va_list l;
	int opt, c, formatargs = 0;
	const char *percent;

	(void)ft;

	if(querytype == FT_FILE_EXISTS)
		switch(clobber_mode){
			case ASK: break;
			case OVERWRITE: return 0;
			case RESUME:    return 1;
			case RENAME:    return 2;
		}

	va_start(l, msg);
	vfprintf(stderr, msg, l);
	va_end(l);

	fputc('\n', stderr);

	percent = msg-1;
	while(1)
		if((percent = strchr(percent+1, '%'))){
			if(percent[1] != '%')
				formatargs++;
		}else
			break;

	/* walk forwards formatargs times, then we're at the option names */
	va_start(l, msg);
	while(formatargs --> 0)
		va_arg(l, const char *);

	formatargs = 0;
	while((percent = va_arg(l, const char *)))
		fprintf(stderr, "%d: %s\n", formatargs++, percent);
	va_end(l);

	fputs("Choice: ", stderr);
	opt = getchar();
	if(opt == '\n')
		opt = '0';
	else if(opt != EOF)
		while((c = getchar()) != EOF && c != '\n');

	return opt - '0';
}

char *fnameback(struct filetransfer *ft, char *name)
{
	(void)ft;
	return name;
}

void clrtoeol()
{
	static const char esc[] = {
		'\r',                      /* mv(0, y) */
		0x1b, '[', '2', 'K', '\0', /* clear */
	};
	fputs(esc, stdout);
}

void cleanup()
{
	if(ft_connected(&ft))
		ft_close(&ft);
}

int eprintf(const char *fmt, ...)
{
	va_list l;
	int ret;

	fprintf(stderr, "tft: ");
	va_start(l, fmt);
	ret = vfprintf(stderr, fmt, l);
	va_end(l);
	return ret;
}

void check_files(int fname_idx, int argc, char **argv)
{
	int i;

	if(fname_idx > 0)
		for(i = fname_idx; i < argc; i++)
			if(access(argv[i], R_OK))
				fprintf(stderr, "tft: warning: no read access for \"%s\" (%s)\n",
						argv[i], strerror(errno));
}

int send_from_stdin(int nul)
{
#ifdef _WIN32
	static char buffer[4096]; /* static for ~stackoverflow */
	char *start;
	int nread;

	nread = fread(buffer, sizeof *buffer, sizeof buffer, stdin);

	if(!nread){
		if(ferror(stdin))
			perror("fread()");
		return 1;
	}

	start = buffer;
	do{
		char *end = memchr(start, nul ? 0 : '\n', nread);

		if(!end){
			fprintf(stderr, "tft: couldn't find terminator for:\n%s\n", buffer);
			return 0;
		}

		*end = '\0';

		if(tft_send(start)){
			clrtoeol();
			eprintf("tft_send(): %s\n", ft_lasterr(&ft));
			return 1;
		}

		/* check for any mores */
		nread -= (end - start);
		start = end + 1;
	}while(nread > 0);
	return 0;
#else
	char  *line = NULL, *delim;
	size_t size = 0;

	if(getdelim(&line, &size, nul ? 0 : '\n', stdin) == -1){
		if(ferror(stdin)){
			perror("tft: read()");
			return 2;
		}
		return 1;
	}

	delim = strchr(line, nul ? 0 : '\n');
	if(delim)
		*delim = '\0';

	if(tft_send(line)){
		clrtoeol();
		eprintf("tft_send(): %s: %s\n", line, ft_lasterr(&ft));
		return 2;
	}

	free(line);
	return 0;
#endif
}

void sigh(int sig)
{
	cleanup();
	exit(sig);
}

int main(int argc, char **argv)
{
	int i, listen;
	int read_stdin, read_stdin_nul;
	int stay_up;
	const char *port = FT_DEFAULT_PORT;
	int fname_idx, host_idx;
#ifdef FT_USE_PING
	int last_ping;
#endif

	listen = stay_up = 0;
	read_stdin = read_stdin_nul = 0;
	fname_idx = host_idx = -1;

#ifndef _WIN32
	/* the usual suspects... */
	signal(SIGINT,  sigh);
	signal(SIGHUP,  sigh);
	signal(SIGTERM, sigh);
	signal(SIGQUIT, sigh);
#endif

#ifdef FT_USE_PING
	srand(time(NULL));
	last_ping = rand() % PING_DELAY;
#endif

#define ARG(c) !strcmp(argv[i], "-" c)

	for(i = 1; i < argc; i++)
		if(ARG("l"))
			listen = 1;
		else if(ARG("p"))
			if(++i < argc)
				port = argv[i];
			else
				goto usage;
		else if(ARG("o"))
			clobber_mode = OVERWRITE;
		else if(ARG("n"))
			clobber_mode = RENAME;
		else if(ARG("r"))
			clobber_mode = RESUME;
		else if(ARG("u")){
			stay_up = 1;
			read_stdin = 1;
		}else if(ARG("i"))
			read_stdin = 1;
		else if(ARG("0"))
			read_stdin_nul = 1;
		else if(ARG("R"))
			recursive = 1;
		else if(!listen && host_idx < 0)
			host_idx = i;
		else if(fname_idx < 0){
			fname_idx = i;
			break;
		}else{
		usage:
			eprintf("Usage: %s [-p port] [OPTS] [-l | host] [files...]\n"
							"  -l: listen\n"
							"  -u: remain running at the end of a transfer (implies -i)\n"
							"  -i: read supplementary file list from stdin\n"
							"  -0: file list is nul-delimited\n"
							"  -R: send directories recursively\n"
							" If file exists:\n"
							"  -o: overwrite\n"
							"  -n: rename incoming\n"
							"  -r: resume transfer\n"
					    , *argv, *argv);
			return 1;
		}

	setbuf(stdout, NULL);
	atexit(cleanup);

	/* sanity */
	if(listen){
		if(host_idx > 0 && fname_idx > 0)
			goto usage;

		/* host idx given, but we're listening, assume it's a file */
		if(fname_idx < 0){
			fname_idx = host_idx;
			host_idx = -1;
		}
	}else if(host_idx == -1)
		goto usage;


	check_files(fname_idx, argc, argv);


	if(listen){
		int iport;

		if(sscanf(port, "%d", &iport) != 1){
			eprintf("need numeric port (\"%s\")\n", port);
			return 1;
		}

		if(ft_listen(&ft, iport)){
			eprintf("ft_listen(%d): %s\n", iport, ft_lasterr(&ft));
			return 1;
		}

		switch(ft_accept(&ft, 1)){
			case FT_ERR:
				eprintf("ft_accept(): %s\n", ft_lasterr(&ft));
				return 1;
			case FT_NO:
				/* shouldn't get here - since it blocks until a connection is made */
				eprintf("no incomming connections... :S\n");
				return 1;
			case FT_YES:
				printf("tft: got connection from %s\n", ft_remoteaddr(&ft));
				break; /* accepted */
		}
	}else if(ft_connect(&ft, argv[host_idx], port)){
		eprintf("ft_connect(\"%s\", \"%s\"): %s\n",
				argv[host_idx], port, ft_lasterr(&ft));
		return 1;
	}


	do{
		if(fname_idx > 0){
			char *fname = argv[fname_idx];

			if(++fname_idx == argc)
				fname_idx = -1;

			if(tft_send(fname)){
				clrtoeol();
				eprintf("tft_send(): %s: %s\n", fname, ft_lasterr(&ft));
#ifdef TFT_DIE_ON_SEND
				goto bail;
#endif
			}
		}else{
			int lewp = 1;

			while(lewp)
				switch(ft_poll_recv_or_close(&ft)){
					case FT_ERR:
						eprintf("ft_poll_recv(): %s\n", ft_lasterr(&ft));
						goto bail;

					case FT_YES:
						lewp = 0;
						break;

					case FT_NO:
					{
						/* check for a file to send */
						struct timeval tv;
						fd_set fds;

						tv.tv_sec = 0;
						tv.tv_usec = 250000; /* 1/4 sec */
						FD_ZERO(&fds);

						if(read_stdin)
							FD_SET(STDIN_FILENO, &fds);

						switch(select(read_stdin, &fds, NULL, NULL, &tv)){
							case -1:
								perror("select()");
								goto bail;

							case 0:
								/* TODO */
#ifdef FT_USE_PING
								if(last_ping++ > PING_DELAY){
									last_ping = 0;

									if(ft_ping(&ft))
										perror("ft_ping()");
								}
#endif
								continue;

							default:
								/* got a file to send */
								if(FD_ISSET(STDIN_FILENO, &fds))
									switch(send_from_stdin(read_stdin_nul)){
										case 0:
											/* good to carry on */
											continue;
										case 1:
											/* eof */
											read_stdin = 0;
											if(!stay_up)
												goto fin;
											continue;
										case 2:
											/* ferror */
#ifdef TFT_DIE_ON_SEND
											goto bail;
#else
											break;
#endif
									}
								continue;
						}
						/* unreachable */
					}
				}
			/* end while(lewp) */

			if(ft_poll_connected(&ft)){
				if(ft_handle(&ft, callback, queryback, fnameback)){
					clrtoeol();
					eprintf("ft_handle(): %s\n", ft_lasterr(&ft));
					goto bail;
				}
			}else
				/* connection closed properly */
				break;
		}
	}while(stay_up || read_stdin || fname_idx > 0);

fin:
	ft_close(&ft);

	return 0;
bail:
	ft_close(&ft);
	return 1;
}
