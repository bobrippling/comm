#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#ifdef _WIN32
# include <winsock2.h>
/* wtf */
#else
# include <sys/select.h>
#endif

#include "libft/ft.h"

#define MAX(x, y) (x > y ? x : y)

#ifdef _WIN32
char __progname[512];
#endif

void clrtoeol(void);
void cleanup(void);
int  eprintf(const char *, ...);

struct filetransfer ft;
enum { OVERWRITE, RESUME, RENAME, ASK } clobber_mode = ASK;

int callback(struct filetransfer *ft, enum ftstate state,
		size_t bytessent, size_t bytestotal)
{
	if(state == FT_SENT || state == FT_RECIEVED){
		clrtoeol();
		printf("Done: %s\n", ft_fname(ft));
		return 0;
	}else if(state == FT_WAIT)
		return 0;

	printf("\"%s\": %zd K / %zd K (%2.2f)%%\r", ft_basename(ft),
			bytessent / 1024, bytestotal / 1024,
			(float)(100.0f * bytessent / bytestotal));

	return 0;
}

int queryback(struct filetransfer *ft, const char *msg, ...)
{
	va_list l;
	int opt, c, formatargs = 0;
	const char *percent;

	(void)ft;

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
#ifndef _WIN32
	extern char *__progname;
#endif

	fprintf(stderr, "%s: ", __progname);
	va_start(l, fmt);
	ret = vfprintf(stderr, fmt, l);
	va_end(l);
	return ret;
}

int main(int argc, char **argv)
{
	int i, listen = 0, verbose = 0, stay_up = 0;
	const char *port = FT_DEFAULT_PORT;
	int fname_idx, host_idx;

	fname_idx = host_idx = -1;

#ifdef _WIN32
	strncpy(__progname, *argv, sizeof __progname);
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
		else if(ARG("v"))
			verbose = 1;
		else if(ARG("o"))
			clobber_mode = OVERWRITE;
		else if(ARG("n"))
			clobber_mode = RENAME;
		else if(ARG("r"))
			clobber_mode = RESUME;
		else if(ARG("O"))
			stay_up = 1;
		else if(host_idx < 0)
			host_idx = i;
		else if(fname_idx < 0){
			fname_idx = i;
			break;
		}else{
		usage:
			eprintf("Usage: %s [-p port] [OPTS] [-l | host] [files...]\n"
							"  -l: listen\n"
							"  -O: remain running at the end of a transfer\n"
							" If file exists:\n"
							"  -o: overwrite\n"
							"  -n: rename incoming\n"
							"  -r: resume transfer\n"
					    , *argv, *argv);
			return 1;
		}

	atexit(cleanup);

	if(listen){
		int iport;

		if(host_idx > 0){
			if(fname_idx > 0)
				goto usage;

			/* host idx given, but we're listening, assume it's a file */
			fname_idx = host_idx;
			host_idx = -1;

			if(verbose)
				printf("%s: serving file(s)...\n", *argv);

		}else if(verbose)
			printf("%s: listening for incomming files...", *argv);

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
				printf("got connection from %s\n", ft_remoteaddr(&ft));
				break; /* accepted */
		}
	}else{
		char *host = NULL;

		if(host_idx > 0)
			host = argv[host_idx];
		else
			goto usage;

		if(verbose)
			printf("connecting to %s...\n", host);

		if(ft_connect(&ft, host, port)){
			eprintf("ft_connect(\"%s\", \"%s\"): %s\n", host, port,
					ft_lasterr(&ft));
			return 1;
		}
	}


	do{
		if(fname_idx > 0){
			char *fname = argv[fname_idx];

			if(++fname_idx == argc)
				fname_idx = -1;

			if(verbose)
				printf("sending %s\n", fname);

			if(ft_send(&ft, callback, fname)){
				clrtoeol();
				eprintf("ft_send(): %s\n", ft_lasterr(&ft));
				goto bail;
			}
		}else{
			int lewp = 1;

			if(verbose)
				printf("%s: waiting for incomming file\n", *argv);

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
						struct timeval tv;
						tv.tv_sec = 0;
						tv.tv_usec = 250000; /* 1/4 sec */
						select(0, NULL, NULL, NULL, &tv);
						continue;
					}
				}

			if(ft_poll_connected(&ft)){
				if(verbose)
					printf("%s: receiving file\n", *argv);

				if(ft_recv(&ft, callback, queryback, fnameback)){
					clrtoeol();
					eprintf("ft_recv(): %s\n", ft_lasterr(&ft));
					goto bail;
				}
			}else
				/* connection closed properly */
				break;
		}
	}while(stay_up || fname_idx > 0);

	ft_close(&ft);

	return 0;
bail:
	ft_close(&ft);
	return 1;
}
