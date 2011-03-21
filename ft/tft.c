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
# include <fcntl.h>
# include <signal.h>
#endif

# include <sys/stat.h>

#include "libft/ft.h"

#define MAX(x, y) (x > y ? x : y)
#undef  TFT_DIE_ON_SEND

void  clrtoeol(void);
void  cleanup(void);
void  fout(FILE *f, const char *fmt, va_list l);
void  eprintf(const char *fmt, ...);
void  oprintf(const char *fmt, ...);
void  check_files(int fname_idx, int argc, char **argv);
char *readline(int nulsep);
int   tft_send(const char *fname);
int   callback(struct filetransfer *ft, enum ftstate state,
        size_t bytessent, size_t bytestotal);
void sigh(int sig);
void beep(void);
const char *now(void);

int recursive = 0, canbeep = 1;
struct filetransfer ft;
enum { OVERWRITE, RESUME, RENAME_ASK, RENAME, ASK } clobber_mode = ASK;

void fout(FILE *f, const char *fmt, va_list l)
{
	fprintf(f, "%s tft: ", now());
	vfprintf(f, fmt, l);
	fputc('\n', f);
}

void oprintf(const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	fout(stdout, fmt, l);
	va_end(l);
}

void eprintf(const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	fout(stderr, fmt, l);
	va_end(l);
}

const char *now()
{
	static char buffer[256];
	time_t tm = time(NULL);
	struct tm *t = localtime(&tm);

	strftime(buffer, sizeof buffer, "%H:%M:%S", t);

	return buffer;
}

int tft_send(const char *fname)
{
	oprintf("sending %s", fname);
	return ft_send(&ft, callback, fname, recursive);
}

void beep()
{
	if(canbeep)
		putchar('\007');
}

int callback(struct filetransfer *ft, enum ftstate state,
		size_t bytessent, size_t bytestotal)
{
	if(state == FT_SENT || state == FT_RECIEVED){
		clrtoeol();
		oprintf("done: %s", ft_fname(ft));

		if(state == FT_RECIEVED)
			beep();
		return 0;
	}else if(state == FT_WAIT)
		return 0;

	printf("\"%s\": %zd K / %zd K (%2.2f%%)\r", ft_basename(ft),
			bytessent / 1024, bytestotal / 1024,
			(float)(100.0f * bytessent / bytestotal));

	return 0;
}

int queryback(struct filetransfer *ft, enum ftquery querytype,
		const char *msg, ...)
{
	va_list l;
	int opt, c, formatargs = 0;
	const char *percent;

	(void)ft;

	if(querytype == FT_FILE_EXISTS)
		switch(clobber_mode){
			case ASK:        break;
			case OVERWRITE:  return 0;
			case RESUME:     return 1;
			case RENAME_ASK:
			case RENAME:     return 2;
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
	beep();
	opt = getchar();
	if(opt == '\n')
		opt = '0';
	else if(opt != EOF)
		while((c = getchar()) != EOF && c != '\n');

	return opt - '0';
}

char *inputback(struct filetransfer *ft, enum ftinput type, const char *prompt,
		char *def)
{
	if(type == FT_RENAME && clobber_mode == RENAME_ASK){
		char *new = malloc(2048);

		(void)ft;

		printf("%s\n(default [nothing]: %s) > ", prompt, def);
		beep();

		if(!fgets(new, 2048, stdin)){
			free(new);
			new = NULL;
		}else{
			if(*new == '\n'){
				free(new);
				return def;
			}else{
				char *n = strchr(new, '\n');
				if(n)
					*n = '\0';
			}
		}
		return new;
	}else
		return def;
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

void check_files(int fname_idx, int argc, char **argv)
{
	int i;

	if(fname_idx > 0)
		for(i = fname_idx; i < argc; i++){
			struct stat st;
			if(stat(argv[i], &st))
				fprintf(stderr, "tft: warning: couldn't stat \"%s\" (%s)\n",
						argv[i], strerror(errno));
			else if(!recursive && S_ISDIR(st.st_mode))
				fprintf(stderr, "tft: warning: \"%s\" is a directory && !recursive\n",
						argv[i]);
		}
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
			eprintf("tft_send(): %s", ft_lasterr(&ft));
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

	if(line[0] == ':'){ /* get round this with ./:myfile.txt etc etc */
		if(!strncmp(line+1, "cd", 2)){
			const char *dir;

			if(line[3])
				dir = line + 4;
			else{
				dir = getenv("HOME");
				if(!dir){
					eprintf("getenv(\"HOME\") failed, using \"/\"");
					dir = "/";
				}
			}

			if(chdir(dir))
				perror("chdir()");

			return 0;
		}else if(!strcmp(line+1, "pwd")){
			char path[512];

			if(getcwd(path, sizeof path))
				printf("%s\n", path);
			else
				perror("tft: getcwd()");
			return 0;
		}else{
			eprintf("unknown command, assuming file (\"%s\")", line);
		}
	}

	if(tft_send(line)){
		clrtoeol();
		eprintf("tft_send(): %s: %s", line, ft_lasterr(&ft));
		return 2;
	}

	free(line);
	return 0;
#endif
}

void sigh(int sig)
{
	cleanup();
	exit(sig + 128);
}

int main(int argc, char **argv)
{
	int i, listen;
	int read_stdin, read_stdin_nul;
	int stay_up;
	const char *port = NULL;
	int fname_idx, host_idx;

	listen = stay_up = 0;
	read_stdin = read_stdin_nul = 0;
	fname_idx = host_idx = -1;

	ft_zero(&ft);

#ifndef _WIN32
	/* the usual suspects... */
	signal(SIGINT,  sigh);
	signal(SIGHUP,  sigh);
	signal(SIGTERM, sigh);
	signal(SIGQUIT, sigh);
#endif

#define ARG(c) !strcmp(argv[i], "-" c)

	for(i = 1; i < argc; i++)
		if(ARG("l"))
			listen = 1;
		else if(ARG("p"))
			if(++i < argc && !port)
				port = argv[i];
			else
				goto usage;
		else if(ARG("o"))
			clobber_mode = OVERWRITE;
		else if(ARG("a"))
			clobber_mode = RENAME;
		else if(ARG("n"))
			clobber_mode = RENAME_ASK;
		else if(ARG("r"))
			clobber_mode = RESUME;
		else if(ARG("B"))
			canbeep = 0;
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
			eprintf("Usage: %s [-p port] [OPTS] [-l | host] [files...]"
							"  -l: listen\n"
							"  -u: remain running at the end of a transfer (implies -i)\n"
							"  -i: read supplementary file list from stdin\n"
							"  -0: file list is nul-delimited\n"
							"  -R: send directories recursively\n"
							"  -B: don't beep on file-receive\n"
							" If file exists:\n"
							"  -o: overwrite\n"
							"  -a: rename incoming automatically\n"
							"  -n: rename incoming\n"
							"  -r: resume transfer\n"
							" Default Port: " FT_DEFAULT_PORT "\n"
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

	if(!port){
		char *colon;
		if(host_idx > -1 && (colon = strchr(argv[host_idx], ':'))){
			*colon++ = '\0';
			port = colon;
		}else
			port = FT_DEFAULT_PORT;
	}

	check_files(fname_idx, argc, argv);


	if(listen){
		int iport;

		if(sscanf(port, "%d", &iport) != 1){
			eprintf("need numeric port (\"%s\")", port);
			return 1;
		}

		if(ft_listen(&ft, iport)){
			eprintf("ft_listen(%d): %s", iport, ft_lasterr(&ft));
			return 1;
		}

		switch(ft_accept(&ft)){
			case FT_ERR:
				eprintf("ft_accept(): %s", ft_lasterr(&ft));
				return 1;
			case FT_NO:
				/* shouldn't get here - since it blocks until a connection is made */
				eprintf("no incoming connections... :S (%s)\n", ft_lasterr(&ft));
				return 1;
			case FT_YES:
				printf("tft: %s: got connection from %s\n", now(), ft_remoteaddr(&ft));
				break; /* accepted */
		}
	}else if(ft_connect(&ft, argv[host_idx], port, callback)){
		eprintf("ft_connect(\"%s\", \"%s\"): %s\n",
				argv[host_idx], port, ft_lasterr(&ft));
		return 1;
	}else
		oprintf("connected to %s", ft_remoteaddr(&ft));

#if 0
	{
		int len = strlen(argv[0]) + strlen(" [connected]") + 1;
		char *arg0 = malloc(len);
		if(arg0){
			snprintf(arg0, len, "%s%s", argv[0], " [connected]");
			argv[0] = arg0;
		}
	}
	- ~/code/Single/rename.c
#endif

	/* connection established */
	beep();

	do{
		if(fname_idx > 0){
			char *fname = argv[fname_idx];

			if(++fname_idx == argc)
				fname_idx = -1;

			if(tft_send(fname)){
				clrtoeol();
				eprintf("tft_send(): %s: %s", fname, ft_lasterr(&ft));
#ifdef TFT_DIE_ON_SEND
				goto bail;
#endif
			}
		}else{
			for(;;){
				fd_set fds;
				const int ft_fd = ft_get_fd(&ft);
				int maxfd = ft_fd;

				FD_ZERO(&fds);
				FD_SET(ft_fd, &fds);

				if(read_stdin){
					FD_SET(STDIN_FILENO, &fds);

					if(read_stdin > ft_fd)
						maxfd = read_stdin;
				}

				switch(select(maxfd + 1, &fds, NULL, NULL, NULL)){
					case -1:
						eprintf("select(): %s", strerror(errno));
						goto bail;
					case 0:
						continue;
				}

				if(FD_ISSET(STDIN_FILENO, &fds))
					/* got a file to send */
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


				if(FD_ISSET(ft_fd, &fds) &&
						/*ft_poll_connected(&ft) &&*/
						ft_recv(&ft, callback, queryback, fnameback, inputback)){

					clrtoeol();
					if(ft_haderror(&ft)){
						eprintf("ft_handle(): %s", ft_lasterr(&ft));
						goto bail;
					}else{
						/* disco */
						oprintf("disconnected from %s", ft_remoteaddr(&ft));
						goto fin;
					}
				}
			}

		}
	}while(stay_up || read_stdin || fname_idx > 0);

fin:
	ft_close(&ft);

	return 0;
bail:
	ft_close(&ft);
	return 1;
}
