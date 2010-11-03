#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "libft/ft.h"

#define MAX(x, y) (x > y ? x : y)

#ifdef _WIN32
char __progname[512];
#endif

void clrtoeol(void);
void cleanup(void);
int  eprintf(const char *, ...);

struct filetransfer ft;

int callback(struct filetransfer *ft, enum ftstate state,
		size_t bytessent, size_t bytestotal)
{
	if(state == FT_SENT || state == FT_RECIEVED){
		clrtoeol();
		printf("Done: %s\n", ft_fname(ft));
		return 0;
	}

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

	va_start(l, msg);
	vprintf(msg, l);
	va_end(l);

	putchar('\n');

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
		printf("%d: %s\n", formatargs++, percent);
	va_end(l);

	fputs("Choice: ", stdout);
	opt = getchar();
	if(opt == '\n')
		opt = '0';
	else if(opt != EOF)
		while((c = getchar()) != EOF && c != '\n');

	return opt - '0';
}

void clrtoeol()
{
	static const char esc[] = { 0x1b, '[', '2', 'K', 0 };
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
	int i, listen = 0, verbose = 0;
	const char *fname = NULL, *host  = NULL, *port = DEFAULT_PORT;

#ifdef _WIN32
	strncpy(__progname, *argv, sizeof __progname);
#endif

	for(i = 1; i < argc; i++)
		if(!strcmp(argv[i], "-l"))
			listen = 1;
		else if(!strcmp(argv[i], "-p"))
			if(++i < argc)
				port = argv[i];
			else
				goto usage;
		else if(!strcmp(argv[i], "-v"))
			verbose = 1;
		else if(!host)
			host = argv[i];
		else if(!fname)
			fname = argv[i];
		else{
		usage:
			eprintf("Usage: %s [-p port] -l   [file]\n"
					            "       %s [-p port] host [file]\n",
					            *argv, *argv);
			return 1;
		}

	atexit(cleanup);

	if(listen){
		int iport;

		if(host && fname) /* no need to check file */
			goto usage;
		else if(host){
			fname = host;
			host = NULL;
			if(verbose)
				printf("%s: serving %s...\n", *argv, fname);
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
		if(!host)
			goto usage;

		if(verbose)
			printf("connecting to %s...\n", host);

		if(ft_connect(&ft, host, port)){
			eprintf("ft_connect(\"%s\", \"%s\"): %s\n", host, port,
					ft_lasterr(&ft));
			return 1;
		}
	}


	if(fname){
		if(verbose)
			printf("sending %s\n", fname);

		if(ft_send(&ft, callback, fname)){
			clrtoeol();
			eprintf("ft_send(): %s\n", ft_lasterr(&ft));
			return 1;
		}
	}else{
		if(verbose)
			printf("%s: receiving incomming file\n", *argv);

		if(ft_recv(&ft, callback, queryback)){
			clrtoeol();
			eprintf("ft_recv(): %s\n", ft_lasterr(&ft));
			return 1;
		}
	}

	ft_close(&ft);

	return 0;
}
