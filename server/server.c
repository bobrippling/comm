#define _POSIX_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <setjmp.h>
#include <stdarg.h>
#include <ctype.h>

#include <time.h>

#include <termios.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#include "../config.h"
#include "../common/socketwrapper.h"
#include "../common/util.h"

#include "md5.h"

#define LOG_FILE         "svrcomm.log"
#define LISTEN_BACKLOG   5
#define SLEEP_MS         100

#define DEBUG_SEND_TEXT " debug%d: send(): "

#define TO_CLIENT(i, msg) \
	do{ \
		fwrite(msg"\n", sizeof *msg, strlen(msg) + 1, clients[i].file); \
		/*    don't include the '\0', but the '\n' ^ */ \
		if(verbose >= DEBUG_SEND) \
			fprintf(stderr, CLIENT_FMT DEBUG_SEND_TEXT "\"%s\"\n", CLIENT_ARGS, DEBUG_SEND, msg); \
	}while(0)

#define DEBUG_ERROR      0
#define DEBUG_CONN_DISCO 1
#define DEBUG_STATUS     2
#define DEBUG_RECV       3
#define DEBUG_SEND       4
#define DEBUG_MAX        DEBUG_SEND

#define CLIENT_FMT  "client %d (socket %d)"
#define CLIENT_ARGS idx, pollfds[idx].fd

#define DEBUG(level, msg, ...) \
	do{ \
		if(verbose >= level){ \
			printf(CLIENT_FMT " debug%d: " msg, CLIENT_ARGS, level, __VA_ARGS__); \
			putchar('\n'); \
		} \
	}while(0)


#define MD5_LEN 32

static struct client
{
	struct sockaddr_in addr;

	FILE *file;

	char *name;

	int isroot;

	enum
	{
		ACCEPTING,
		ACCEPTED
	} state;
} *clients = NULL;

static struct pollfd *pollfds = NULL;

static int server = -1, nclients = 0, background = 0, verbose = 0;
static char *md5pass = NULL, *progname, *svrdesc = "";

jmp_buf allocerr;


int passfromstdin(void);

int nonblock(int);
void cleanup(void);
void sigh(int);
void freeclient(int);
int toclientf(int, const char *, ...);
int validname(const char *);

char svr_conn(int);
char svr_recv(int);
void svr_hup( int);
void svr_err( int);
/*
 * svr_recv return 1 if the client disconnects/on error
 * i.e. the clients array has been altered
 * --> --i in the loop
 */

int toclientf(int idx, const char *fmt, ...)
{
	static const char nl = '\n';
	va_list l;
	int ret, fw;


	va_start(l, fmt);
	ret = vfprintf(clients[idx].file, fmt, l);
	va_end(l);

	if(verbose >= DEBUG_SEND){
		fprintf(stderr, CLIENT_FMT DEBUG_SEND_TEXT, CLIENT_ARGS, DEBUG_SEND);
		fputc('"', stderr);
		va_start(l, fmt);
		vfprintf(stderr, fmt, l);
		va_end(l);
		fputs("\"\n", stderr);
	}

	if(ret == -1)
		return -1;

	fw = fwrite(&nl, sizeof nl, 1, clients[idx].file);

	if(fw == -1)
		return -1;
	return fw+ ret;
}

int nonblock(int fd)
{
	int flags;

	if((flags = fcntl(fd, F_GETFL, 0)) == -1)
		flags = 0;
	flags |= O_NONBLOCK;

	if(fcntl(fd, F_SETFL, flags) == -1)
		return 1;
	return 0;
}

int validname(const char *n)
{
	const char *p;
	for(p = n; *p; p++)
		/* allow negatives, since they're probably unicode/é, etc */
		if(*p != ' ' && !isgraph(*p) && *p > 0)
			return 0;
	return 1;
}

void cleanup()
{
	int i;

	for(i = 0; i < nclients; i++)
		freeclient(i);

	free(clients);
	free(pollfds);

	shutdown(server, SHUT_RDWR);
	close(server);
}

char svr_conn(int idx)
{
	clients[idx].state = ACCEPTING;
	clients[idx].name  = NULL;
	if(!(clients[idx].file  = fdopen(pollfds[idx].fd, "r+"))){
		DEBUG(DEBUG_ERROR, "fdopen(): %s", strerror(errno));
		return 0;
	}

	if(setvbuf(clients[idx].file, NULL, _IONBF, 0)){
		DEBUG(DEBUG_ERROR, "setvbuf(): %s", strerror(errno));
		return 0;
	}

	DEBUG(DEBUG_CONN_DISCO, "connected from %s",
			addrtostr((struct sockaddr *)&clients[idx].addr));

	if(toclientf(idx, "Comm v%s%s%s", VERSION_STR,
				*svrdesc ? " " : "", svrdesc) == 0){
		DEBUG(DEBUG_ERROR, "fwrite(): %s", strerror(errno));
		return 0;
	}

	return 1;
}

void svr_hup(int idx)
{
	int i;

	DEBUG(DEBUG_CONN_DISCO, "%s", "disconnected"); /* need at least one arg */

	if(clients[idx].state == ACCEPTED)
		for(i = 0; i < nclients; i++)
			if(i != idx && clients[i].state == ACCEPTED)
				toclientf(i, "CLIENT_DISCO %s", clients[idx].name);

	freeclient(idx);

	for(i = idx; i < nclients-1; i++){
		clients[i] = clients[i+1];
		pollfds[i] = pollfds[i+1];
	}

	clients = realloc(clients, --nclients * sizeof(*clients));
	pollfds = realloc(pollfds,   nclients * sizeof(*pollfds));
}

void freeclient(int idx)
{
	int fd = pollfds[idx].fd;

	free(clients[idx].name);

	if(fd != -1){
		shutdown(fd, SHUT_RDWR);
		/*close(fd);*/
		fclose(clients[idx].file); /* closes fd */
		pollfds[idx].fd = -1;
		clients[idx].file = NULL;
	}
}

void svr_err(int idx)
{
	DEBUG(DEBUG_ERROR, "client error %s", errno ? strerror(errno) : "unknown");
	svr_hup(idx);
}

char svr_recv(int idx)
{
	char in[LINE_SIZE] = { 0 };
	int ret;

	switch(ret = recv(pollfds[idx].fd, in, LINE_SIZE, MSG_PEEK)){
		case -1:
			fputs("recv() ", stderr);
			svr_err(idx);
			return 1;

		case 0:
			svr_hup(idx);
			return 1;
	}

	if(recv_newline(in, ret, pollfds[idx].fd))
		/* not enough data */
		return 0;

	DEBUG(DEBUG_RECV, "recv(): \"%s\"", in);

	if(clients[idx].state == ACCEPTING){
		if(!strncmp(in, "NAME ", 5)){
			char *name = in + 5;
			int len = strlen(name), i;

			if(len == 0){
				TO_CLIENT(idx, "ERR need non-zero length name");
				DEBUG(DEBUG_STATUS, "%s", "zero length name");
				svr_hup(idx);
				return 1;
			}

			if(!validname(name)){
				TO_CLIENT(idx, "ERR name contains invalid character(s)");
				DEBUG(DEBUG_STATUS, "%s", "invalid character(s) in name\n");
				svr_hup(idx);
				return 1;
			}

			for(i = 0; i < nclients; i++)
				if(i != idx && clients[i].state == ACCEPTED && !strcmp(clients[i].name, name)){
					TO_CLIENT(idx, "ERR name in use");
					DEBUG(DEBUG_STATUS, "name %s in use", name);
					svr_hup(idx);
					return 1;
				}

			clients[idx].name   = cstrdup(name);
			clients[idx].isroot = 0;

			clients[idx].state = ACCEPTED;

			DEBUG(DEBUG_STATUS, "name accepted: %s", name);

			TO_CLIENT(idx, "OK");

			for(i = 0; i < nclients; i++)
				if(i != idx && clients[i].state == ACCEPTED)
					toclientf(i, "CLIENT_CONN %s", name);

		}else{
			TO_CLIENT(idx, "ERR need name");
			DEBUG(DEBUG_STATUS, "%s", "name not given");
			svr_hup(idx);
			return 1;
		}
	}else{
		if(!strncmp(in, "MESSAGE ", 8)){
			int i;

			if(!strlen(in + 8)){
				TO_CLIENT(idx, "ERR need message");
				return 0;
			}

			for(i = 0; i < nclients; i++)
				if(clients[i].state == ACCEPTED)
					/* send back to idx */
					toclientf(i, "%s", in);

		}else if(!strcmp(in, "CLIENT_LIST")){
			int i;
			TO_CLIENT(idx, "CLIENT_LIST_START");
			for(i = 0; i < nclients; i++)
				if(i != idx && clients[i].state == ACCEPTED)
					toclientf(idx, "CLIENT_LIST %s", clients[i].name);
			TO_CLIENT(idx, "CLIENT_LIST_END");

		}else if(!strncmp(in, "RENAME ", 7)){
			char *name = in + 7, *last;

			while(isspace(*name))
				name++;

			last = name;
			while(*last)
				last++;
			last--;
			while(isspace(*last) && last > name)
				last--;
			if(isspace(*last))
				*last = '\0';

			if(!strlen(name) || strlen(name) > MAX_NAME_LEN)
				TO_CLIENT(idx, "ERR need name/name too long");
			else if(!validname(name))
				TO_CLIENT(idx, "ERR invalid name");
			else{
				if(!strcmp(clients[idx].name, name)){
					TO_CLIENT(idx, "ERR can't rename to the same name");
				}else{
					int i, good = 1;

					for(i = 0; i < nclients; i++)
						if(clients[idx].state == ACCEPTED && !strcmp(name, clients[idx].name)){
							good = 0;
							break;
						}

					if(good){
						char oldname[MAX_NAME_LEN];
						char *new;
						strncpy(oldname, clients[idx].name, MAX_NAME_LEN);

						new = realloc(clients[idx].name, strlen(name) + 1);
						if(!new)
							longjmp(allocerr, 1);
						strcpy(clients[idx].name = new, name);
						for(i = 0; i < nclients; i++)
							/* send back to idx too, to confirm */
							toclientf(i, "RENAME %s%c%s", oldname, RENAME_SEPARATOR ,new);
					}else
						TO_CLIENT(idx, "ERR name already taken");
				}
			}
		}else if(!strncmp(in, "KICK ", 5)){
			if(clients[idx].isroot){
				char *name = in + 5;
				int i, found = 0;

				if(!strlen(name))
					TO_CLIENT(idx, "ERR need name to kick");
				else{
					for(i = 0; i < nclients; i++)
						if(!strcmp(clients[i].name, name)){
							found = 1;
							break;
						}

					if(!found)
						toclientf(idx, "ERR name \"%s\" not found", name);
					else{
						int j;
						for(j = 0; j < nclients; j++)
							if(j != i)
								toclientf(j, "INFO %s was kicked", name);
						TO_CLIENT(i, "INFO you have been kicked");
						DEBUG(DEBUG_STATUS, "<- (%s) kicked %s",
									clients[idx].name, name);
						svr_hup(i);
					}
				}
			}else
				TO_CLIENT(idx, "ERR not root");

		}else if(!strncmp(in, "SU ", 3)){
			if(clients[idx].isroot){
				TO_CLIENT(idx, "INFO dropping root");
				clients[idx].isroot = 0;
			}else{
				char *pass = in + 3;

				if(!*pass)
					TO_CLIENT(idx, "Need md5'd password");
				else if(md5check(pass, md5pass)){
					TO_CLIENT(idx, "ERR incorrect root passphrase");
					DEBUG(DEBUG_STATUS, "%s root logout", clients[idx].name);
				}else{
					clients[idx].isroot = 1;
					TO_CLIENT(idx, "INFO root login successful");
					DEBUG(DEBUG_STATUS, "root login: %s", clients[idx].name);
				}
			}

		}else{
			toclientf(idx, "ERR command \"%s\" unknown", in);
		}
	}

	return 0;
}

void sigh(int sig)
{
	if(sig == SIGINT && !background){
		int i;
		printf("\nComm v"VERSION_STR" Server Status - %d clients (SIGQUIT ^\\ to quit)\n", nclients);
		if(nclients){
			puts("Index FD Name");
			for(i = 0; i < nclients; i++)
				printf("%3d %3d  %s\n", i, pollfds[i].fd, clients[i].name ? clients[i].name : "(not logged in)");
		}
		fflush(stdout);
		signal(SIGINT, &sigh);
	}else{
		fprintf(stderr, "we get signal %d\n", sig);

		cleanup();

		exit(sig);
	}
}

int passfromstdin()
{
	struct termios tio;
	char pass[16], *nl, *ret;
	int echooff = 0;

	if(isatty(STDIN_FILENO)){
		if(!tcgetattr(STDIN_FILENO, &tio)){
			tio.c_lflag = (tio.c_lflag & ~ECHO) | ECHONL;
			if(!tcsetattr(STDIN_FILENO, TCSANOW, &tio))
				echooff = 1;
			else
				perror("tcsetattr()");
		}else
			perror("tcgetattr()");

		fputs("server 'root' password: ", stderr);
	}

	ret = fgets(pass, sizeof pass, stdin);

	if(echooff){
		tio.c_lflag = tio.c_lflag | ECHO;
		if(tcsetattr(STDIN_FILENO, TCSANOW, &tio))
			perror("tcsetattr()");
	}

	if(!ret){
		fprintf(stderr, "\n%s: warning: root access disabled "
				"(no password)\n", progname);
		return 0;
	}

	if((nl = strchr(pass, '\n')))
		*nl = '\0';
	else
		fputs("warning: too many chars, truncated to 15\n", stderr);

	md5pass = md5(pass);

	return !md5pass;
}

int main(int argc, char **argv)
{
	struct sockaddr_in svr_addr;
	int i, log = 0, gotpass = 0;
	const char *port = DEFAULT_PORT;

	srand(getpid() + geteuid() + time(NULL)); /* 'root' password */

	progname = *argv;

	if(setjmp(allocerr)){
		perror("longjmp'd to allocerr");
		return 1;
	}

	/* ignore sigpipe - sent when recv() called on a disconnected socket */
	if( signal(SIGPIPE, SIG_IGN) == SIG_ERR ||
			/* usual suspects... */
			signal(SIGINT,  &sigh)   == SIG_ERR ||
			signal(SIGQUIT, &sigh)   == SIG_ERR ||
			signal(SIGTERM, &sigh)   == SIG_ERR ||
			signal(SIGSEGV, &sigh)   == SIG_ERR ||
			signal(SIGHUP,  &sigh)   == SIG_ERR){
		perror("signal()");
		return 1;
	}

	for(i = 1; i < argc; i++)
		if(!strcmp(argv[i], "-p")){
			if(++i < argc)
				port = argv[i];
			else{
				fputs("need port\n", stderr);
				return 1;
			}

		}else if(!strncmp(argv[i], "-P", 2)){
			if(++i < argc){
				gotpass = 1;
				if(!(md5pass = md5(argv[i])))
					return 1;
				memset(argv[i], '*', strlen(argv[i])); /* too late, but eh... */
			}else{
				fputs("need pass\n", stderr);
				return 1;
			}

		}else if(!strcmp(argv[i], "-d")){
			if(++i < argc)
				svrdesc = argv[i];
			else{
				fputs("need description\n", stderr);
				return 1;
			}

		}else if(!strncmp(argv[i], "-v", 2)){
			char *p = argv[i] + 1;

			while(*p == 'v'){
				verbose++;
				p++;
			}

			if(*p != '\0')
				goto usage;
		}else if(!strcmp(argv[i], "-b")){
			background = 1;
		}else if(!strcmp(argv[i], "-l")){
			log = 1;
		}else
			goto usage;

	if(!gotpass && passfromstdin())
		return 1;

	if(verbose > 1){
		if(verbose > DEBUG_MAX){
			fprintf(stderr, "max verbose level is %d\n", DEBUG_MAX);
			return 1;
		}else
			printf("verbose %d\n", verbose);
	}

	if(log){
		int logfd = creat(LOG_FILE, 0644);
		if(logfd == -1){
			perror("log: creat()");
			return 1;
		}
		if(dup2(logfd, STDOUT_FILENO) == -1 ||
			 dup2(logfd, STDERR_FILENO) == -1){
			perror("dup2()");
			close(logfd);
			return 1;
		}
		close(logfd); /* cleanup */
	}

	if(background){
		fprintf(stderr, "%s: closing output and forking to background", *argv);

		signal(SIGCHLD, SIG_IGN);
		switch(fork()){
			case 0:
				if(setsid() == (pid_t)-1){
					perror("setsid()");
					return 1;
				}
				if(!log){
					close(STDOUT_FILENO);
					close(STDERR_FILENO);
				}
				close(STDIN_FILENO);
				break;
			case -1:
				perror("fork()");
				return 1;
			default:
				/* parent */
				return 0;
		}
	}else
		puts("Comm v"VERSION_STR" Server init");


	server = socket(PF_INET, SOCK_STREAM, 0);
	if(server == -1){
		perror("socket()");
		return 1;
	}

	memset(&svr_addr, '\0', sizeof svr_addr);
	svr_addr.sin_family = AF_INET;
	svr_addr.sin_port   = htons(atoi(port)); /* TODO: use getaddrinfo? */

	if(INADDR_ANY) /* usually optimised out of existence */
		svr_addr.sin_addr.s_addr = htonl(INADDR_ANY);


	/* setsockopt ... SO_REUSE(ADDR|PORT) ? */
	if(bind(server, (struct sockaddr *)&svr_addr, sizeof svr_addr) == -1){
		perror("bind()");
		close(server);
		return 1;
	}

	if(listen(server, LISTEN_BACKLOG) == -1){
		perror("listen()");
		close(server);
		return 1;
	}

	if(nonblock(server)){
		perror("nonblock()");
		close(server);
		return 1;
	}

#define INTERRUPT_WRAP(funcall, failcode) \
	while(!(funcall)) \
		if(errno != EINTR) \
			failcode

	for(;;){
		struct sockaddr_in addr;
		socklen_t len = sizeof addr;
		int cfd = accept(server, (struct sockaddr *)&addr, &len), ret;

		if(cfd != -1){
			struct client *new;
			struct pollfd *newpfds;
			int idx = ++nclients - 1;

			INTERRUPT_WRAP(
					new = realloc(clients, nclients * sizeof(*clients)),
					{
						perror("realloc()");
						cleanup();
						return 1;
					})

			INTERRUPT_WRAP(
					newpfds = realloc(pollfds, nclients * sizeof(*newpfds)),
					{
						perror("realloc()");
						cleanup();
						return 1;
					})

			clients = new;
			pollfds = newpfds;

			memcpy(&clients[idx].addr, &addr, sizeof addr);

			pollfds[idx].fd = cfd;

			if(!svr_conn(idx)){
				if(clients[idx].file)
					fclose(clients[idx].file);
				clients = realloc(clients, --nclients * sizeof(*clients));
				pollfds = realloc(pollfds,   nclients * sizeof(*pollfds));
				close(cfd);
			}
		}else if(errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK){
			perror("accept()");
			cleanup();
			return 1;
		}

		for(i = 0; i < nclients; i++)
			pollfds[i].events = POLLIN | POLLERR | POLLHUP;

		ret = poll(pollfds, nclients, SLEEP_MS);
		/* need to timeout, since we're also accept()ing above */

		/*
		 * ret = 0: timeout
		 * ret > 0: number of fds that have status
		 * ret < 0: err
		 */
		if(ret == 0)
			continue;
		else if(ret < 0){
			if(errno == EINTR)
				continue;

			perror("poll()");
			cleanup();
			return 1;
		}

#define BIT(a, b) (((a) & (b)) == (b))
		for(i = 0; ret > 0 && i < nclients; i++){
			if(BIT(pollfds[i].revents, POLLHUP)){
				ret--;
				svr_hup(i--);
				continue;
			}

			if(BIT(pollfds[i].revents, POLLERR)){
				ret--;
				svr_err(i--);
				continue;
			}

			if(BIT(pollfds[i].revents, POLLIN)){
				ret--;
				i -= svr_recv(i);
			}
		}
	}
	/* UNREACHABLE */

usage:
	fprintf(stderr, "Usage: %s [-p port] [-v*] [-b] [-l]\n"
	                " -p: Port to listen on\n"
	                " -P: 'root' password\n"
	                " -d: Server description\n"
	                " -v: Verbose\n"
	                " -b: Fork to background\n"
	                " -l: Redirect output to logs\n"
	                "[pPdv] take arguments\n", *argv);
	return 1;
}
