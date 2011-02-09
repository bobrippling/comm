#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <poll.h>
#include <signal.h>


#include "libft/ft.h"

#define PATH_LEN     128
#define MAX_LINE_LEN 4096

#ifdef __GNUC__
# define GCC_PRINTF_ATTRIB(x, y) __attribute__((format(printf, x, y)))
#else
# define GCC_PRINTF_ATTRIB(x)
#endif

enum { RESUME, RENAME } clobber_mode = RENAME;


char *dir = NULL;
char  file_cmd[PATH_LEN] = { 0 };
char  file_log[PATH_LEN] = { 0 };

int fd_cmd   = -1;
int got_usr1 =  0;

struct filetransfer ft;

#define eprintf(...) logprintf("err", __VA_ARGS__)

void logprintf(const char *type, const char *fmt, ...) GCC_PRINTF_ATTRIB(2, 3);
void logprintf(const char *type, const char *fmt, ...)
{
	FILE *f;
	va_list l;

	f = fopen(file_log, "a");
	if(!f)
		return;

	fprintf(f, "%s: ", type);
	va_start(l, fmt);
	vfprintf(f, fmt, l);
	va_end(l);
	fclose(f);
}

void cleanup()
{
	/* only close + remove the fifos */
	close(fd_cmd);
	remove(file_cmd);

	ft_close(&ft);
}

char *pwd()
{
	static char wd[PATH_LEN];
	return getcwd(wd, sizeof wd);
}

int block(int fd)
{
	int flags = fcntl(fd, F_GETFL) & ~O_NONBLOCK;

	if(fcntl(fd, F_SETFL, flags) == -1){
		perror("tftd: fcntl(O_NONBLOCK)");
		return 1;
	}
	return 0;
}

int reopen_cmd()
{
	if(fd_cmd != -1)
		close(fd_cmd);

	if(mkfifo(file_cmd, 0600) == -1 && errno != EEXIST){
		eprintf("tftd: mkfifo(\"%s\"): %s\n",
				file_cmd, strerror(errno));
		return 1;
	}

	if((fd_cmd = open(file_cmd, O_RDONLY | O_NONBLOCK, 0600)) == -1){
		eprintf("tftd: open(\"%s\"): %s\n", file_cmd, strerror(errno));
		remove(file_cmd);
		return 1;
	}

	if(block(fd_cmd)){
		close(fd_cmd);
		remove(file_cmd);
		return 1;
	}

	return 0;
}

int init_files()
{
	if(mkdir(dir, 0700) && errno != EEXIST){
		fprintf(stderr, "tftd: mkdir(\"%s\"): %s\n", dir, strerror(errno));
		return 1;
	}

	if(chdir(dir)){
		fprintf(stderr, "tftd: chdir(\"%s\"): %s\n", dir, strerror(errno));
		return 1;
	}

	snprintf(file_cmd, PATH_LEN, "%s/%s", dir, "tftd_cmd");
	snprintf(file_log, PATH_LEN, "%s/%s", dir, "tftd_log");

	if(reopen_cmd())
		return 1;

	close(creat(file_log, 0600)); /* touch */

	return 0;
}


int callback(struct filetransfer *ft, enum ftstate state,
		size_t bytessent, size_t bytestotal)
{
	(void)ft;
	(void)state;
	(void)bytessent;
	(void)bytestotal;

	switch(state){
		case FT_SENT:
		case FT_RECIEVED:
			logprintf(state == FT_SENT ? "end_send" : "end_recv", "%s\n", ft_fname(ft));
			break;

		case FT_BEGIN_SEND:
		case FT_BEGIN_RECV:
			logprintf(state == FT_BEGIN_SEND ? "begin_send" : "begin_recv", "%s\n", ft_fname(ft));
			break;

		case FT_RECIEVING:
		case FT_SENDING:
			if(got_usr1){
				got_usr1 = 0;
				logprintf("progress", "\"%s\": %zd K / %zd K (%2.2f%%)\n", ft_basename(ft),
						bytessent / 1024, bytestotal / 1024,
						(float)(100.0f * bytessent / bytestotal));
			}
			break;

		case FT_WAIT:
			break;
	}

	return 0;
}

int queryback(struct filetransfer *ft, enum ftquery qtype, const char *msg, ...)
{
	(void)ft;
	(void)msg;

	if(qtype == FT_FILE_EXISTS)
		/* overwrite, resume, rename */
		switch(clobber_mode){
			case RESUME:    return 1;
			case RENAME:    return 2;
		}
	else if(qtype == FT_CANT_OPEN)
		/* retry, rename, fail */
		return 1; /* rename */

	eprintf("queryback(): unknown qtype %d\n", qtype);
	return 0;
}

char *fnameback(struct filetransfer *ft, char *fname)
{
	(void)ft;
	return fname;
}

char *inputback(struct filetransfer *ft, enum ftinput itype, const char *prompt, char *def)
{
	(void)ft;
	(void)itype;
	(void)prompt;
	return def;
}

int connected_lewp()
{
	struct pollfd pfd[2];

	pfd[0].fd     = ft_get_fd(&ft);
	pfd[0].events = POLLIN;

	pfd[1].fd     = fd_cmd;
	pfd[1].events = POLLIN | POLLHUP;

	for(;;){
		switch(poll(pfd, 2, -1)){
			case 0:
				/* notan happan */
				continue;
			case -1:
				if(errno == EINTR){
					if(got_usr1){
						got_usr1 = 0;
						logprintf("connected", "waiting messages\n");
					}
					continue;
				}

				if(ft_lasterrno(&ft)){
					eprintf("poll: %s\n", strerror(errno));
					return 1;
				}
				return 0;
		}

#define BIT(x, b) (((x) & (b)) == (b))
		if(BIT(pfd[0].revents, POLLIN)){
			if(ft_handle(&ft, callback, queryback, fnameback, inputback)){
				/* "recieved %s" done in callback */
				if(ft_lasterrno(&ft)){
					eprintf("ft_recv(): %s\n", ft_lasterr(&ft));
					return 1;
				}else{
					logprintf("state", "connection closed from %s\n", ft_remoteaddr(&ft));
					return 0; /* conn closed */
				}
			}
		}

		if(BIT(pfd[1].revents, POLLIN)){
			int nread, saverrno;
			char buffer[MAX_LINE_LEN], *nl;

			nread = read(fd_cmd, buffer, MAX_LINE_LEN);
			saverrno = errno;

			switch(nread){
				case -1:
					eprintf("read(): %s\n", strerror(errno));
					return 1;
				case 0:
					break;
				default:
					if((nl = memchr(buffer, '\n', nread)))
						*nl = '\0';
					else
						buffer[nread-1] = '\0';

					if(!strncmp(buffer, "send ", 5)){
						register char *fname = buffer + 5;
						if(ft_connected(&ft)){
							if(ft_send(&ft, callback, fname, 1 /* recurse */))
								eprintf("ft_send(\"%s\"): %s\n", fname, ft_lasterr(&ft));
						}else
							eprintf("not connected\n");
					}else if(!strcmp(buffer, "pwd")){
		pwd:
						logprintf("pwd", "%s\n", pwd());
					}else if(!strncmp(buffer, "cd ", 3)){
						register char *dir = buffer + 3;

						if(chdir(dir))
							eprintf("chdir(\"%s\"): %s (in %s)\n",
									dir, strerror(errno), pwd());
						else
							goto pwd;
					}else
						eprintf("unknown directive \"%s\"\n", buffer);
			}
		}else if(BIT(pfd[1].revents, POLLHUP)){
			if(reopen_cmd()){
				eprintf("reopen cmd: %s\n", strerror(errno));
				return 1;
			}
		}
	}
}

void sigh(int sig)
{
	switch(sig){
		case SIGSEGV:
		case SIGINT:
		case SIGTERM:
		case SIGQUIT:
			cleanup();
			logprintf("exit", "signal %d\n", sig);
			exit(sig + 128);
			break;

		case SIGUSR1:
		case SIGHUP:
			signal(sig, sigh);

			if(sig == SIGHUP)
				ft_close(&ft);
			else
				got_usr1 = 1;

	}
}

int main(int argc, char **argv)
{
	char *port = NULL, *host = NULL;
	int i, daemon = 1, sig = 0;

#define ARG(s) !strcmp(argv[i], "-" s)

	/* usual suspects... */
	signal(SIGINT,  sigh);
	signal(SIGQUIT, sigh);
	signal(SIGTERM, sigh);
	signal(SIGSEGV, sigh); /* oh hai */

	/* interaction */
	signal(SIGUSR1, sigh);
	signal(SIGHUP,  sigh);

	for(i = 1; i < argc; i++)
		if(ARG("p"))
			if(++i < argc)
				port = argv[i];
			else
				goto usage;
		else if(ARG("n"))
			clobber_mode = RENAME;
		else if(ARG("r"))
			clobber_mode = RESUME;
		else if(ARG("D"))
			daemon = 0;
		else if(ARG("S")){
			if(++i == argc){
				fputs("-S needs an option\n", stderr);
				goto usage;
			}
			if(!strcmp(argv[i], "kill"))
				sig = SIGTERM;
			else if(!strcmp(argv[i], "info"))
				sig = SIGUSR1;
			else if(!strcmp(argv[i], "hup"))
				sig = SIGHUP;
			else{
				fprintf(stderr, "-S: invalid option \"%s\"\n", argv[i]);
				goto usage;
			}
		}else if(!dir)
			dir = argv[i];
		else if(!host)
			host = argv[i];
		else{
		usage:
			fprintf(stderr,
					"Usage: %s [OPTS] path/to/base/dir [host]\n"
					"  -D: Don't daemonise\n"
					"  -p: Port to listen on\n"
					"  -n: Rename file if existing (default)\n"
					"  -r: Resume transfer if existing\n"
					"  -S: send message to current instance\n"
					"      \"kill\" - stop\n"
					"      \"info\" - print transfer info to log\n"
					"      \"hup\"  - hang up current connection\n"
					, *argv);
			return 1;
		}

	if(sig){
		char *args[] = { "pkill", NULL, "tftd", NULL };

		if(!(args[1] = malloc(5))){
			perror("tftd: malloc()");
			return 1;
		}
		snprintf(args[1], 5, "-%d", sig);

		execvp(args[0], args);
		perror("tftd: exec: pkill");
		return 1;
	}

	if(!dir){
		fputs("need path\n", stderr);
		goto usage;
	}

	if(!port)
		port = FT_DEFAULT_PORT;

	if(init_files())
		return 1;

	if(daemon)
		switch(fork()){
			case -1:
				perror("tftd: fork()");
				return 1;

			case 0:
				close(0);
				close(1);
				close(2);
				break;

			default:
				printf("tftd: daemonised\n");
				return 0;
		}

	for(;;){
#define NAP(str) \
		do{ \
			if(ft_lasterrno(&ft) == EINTR && got_usr1){ \
				got_usr1 = 0; \
				logprintf("state", str "ing\n"); \
			}else{ \
				eprintf("ft_" str ": %s, napping for 5\n", ft_lasterr(&ft)); \
				sleep(5); \
			} \
			goto conn_restart; \
		}while(0)

conn_restart:
		ft_close(&ft);
		if(host){
			logprintf("connecting", "connecting to %s:%s\n", host, port);
			if(ft_connect(&ft, host, port))
				NAP("connect");

			logprintf("state", "connected to %s\n", ft_remoteaddr(&ft));
		}else{
			if(ft_listen(&ft, atoi(port)))
				NAP("listen");
			logprintf("listening", "port %s\n", port);
			if(ft_accept(&ft, 1) != FT_YES)
				NAP("accept");

			logprintf("state", "connection from %s\n", ft_remoteaddr(&ft));
		}

		connected_lewp();
	}

	eprintf("unreachable code\n");
}
