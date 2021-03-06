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
#include <alloca.h>

#include <time.h>

#include <termios.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#include "../config.h"
#include "../common/socketwrapper.h"
#include "../common/util.h"

#include "cfg.h"
#include "restrict.h"

#define LONELY_MSG       "CommServer: In cyberspace, no one can hear you Comm"

#define LOG_FILE         "svrcomm.log"
#define CFG_FILE         "/.svrcommrc"
#define LISTEN_BACKLOG   5
#define POLL_SLEEP_MS    500
#define RECV_SLEEP_MS    30

#define DEBUG_SEND_TEXT " debug%d: send(): "

#define TO_CLIENT(i, msg) \
	do{ \
		fwrite(msg"\n", sizeof *msg, strlen(msg) + 1, clients[i].file); \
		/*    don't include the '\0', but the '\n' ^ */ \
		if(verbose >= DEBUG_SEND) \
			fprintf(stderr, CLIENT_FMT DEBUG_SEND_TEXT "\"%s\"\n", CLIENT_ARGS(idx), DEBUG_SEND, msg); \
	}while(0)

#define DEBUG_ERROR      0
#define DEBUG_CONN_DISCO 1
#define DEBUG_STATUS     2
#define DEBUG_RECV       3
#define DEBUG_SEND       4
#define DEBUG_MAX        DEBUG_SEND

#define CLIENT_FMT       "client %d (socket %d)"
#define CLIENT_ARGS(idx) idx, pollfds[idx].fd

#define DEBUG(idx, level, msg, ...) \
	do{ \
		if(verbose >= level){ \
			printf(CLIENT_FMT " debug%d: " msg, CLIENT_ARGS(idx), level, __VA_ARGS__); \
			putchar('\n'); \
		} \
	}while(0)


static struct client
{
	struct sockaddr_in addr, uaddr;
	FILE *file;
	char *name;
	char *colour;
	int isroot;
	int typing;

	enum
	{
		ACCEPTING,
		ACCEPTED
	} state;
} *clients = NULL;

static struct pollfd *pollfds = NULL;

static struct
{
	int blocked, allowed, accepted, sus;
} stats = { 0, 0, 0, 0 };

static int server = -1, udpserver = -1;
static int nclients = 0, verbose = 0, background = 0;
static time_t starttime;

/* extern'd - cfg */
char glob_desc[MAX_DESC_LEN] = { 0 },
     glob_port[MAX_PORT_LEN] = { 0 },
     glob_pass[MAX_PASS_LEN] = { 0 };

jmp_buf allocerr;


/*int passfromstdin(void);*/
const char *getuptime(void);

int  cfg_init(void);
void sigh(int);
int  setup(void);
void cleanup(void);

int  nonblock(int);
int  validname(const char *);
int  toclientf(int, const char *, ...);
void freeclient(int);
int  findclient(const char *name);

char svr_conn(int);
char svr_recv(int);
void svr_hup( int);
void svr_err( int);
void svr_rm(  int);
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
		fprintf(stderr, CLIENT_FMT DEBUG_SEND_TEXT, CLIENT_ARGS(idx), DEBUG_SEND);
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
		if(!((isgraph(*p) || *p == ' ') || *p < 0))
			return 0;
	return 1;
}

int findclient(const char *name)
{
	int i;
	for(i = 0; i < nclients; i++)
		if(!strcmp(name, clients[i].name))
			return i;
	return -1;
}

char *trim(char *s)
{
	char *last;

	while(isspace(*s))
		s++;

	last = s;
	while(*last)
		last++;
	last--;
	while(isspace(*last) && last > s)
		*last-- = '\0';

	return s;
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
	close(udpserver);
}

char svr_conn(int idx)
{
	memset(clients + idx, 0, sizeof clients[idx]);
	clients[idx].state = ACCEPTING;
	clients[idx].name = clients[idx].colour = NULL;

	if(!(clients[idx].file  = fdopen(pollfds[idx].fd, "r+"))){
		DEBUG(idx, DEBUG_ERROR, "fdopen(): %s", strerror(errno));
		return 0;
	}

	if(setvbuf(clients[idx].file, NULL, _IONBF, 0)){
		DEBUG(idx, DEBUG_ERROR, "setvbuf(): %s", strerror(errno));
		return 0;
	}

	DEBUG(idx, DEBUG_CONN_DISCO, "connected from %s",
			addrtostr((struct sockaddr *)&clients[idx].addr));

	if(toclientf(idx, "Comm v" VERSION_STR " %s, uptime %s",
				glob_desc, getuptime()) == 0){
		DEBUG(idx, DEBUG_ERROR, "fwrite(): %s", strerror(errno));
		return 0;
	}

	return 1;
}

void svr_hup(int idx)
{
	int i;

	DEBUG(idx, DEBUG_CONN_DISCO, "%s", "disconnected"); /* need at least one arg */

	if(clients[idx].state == ACCEPTED)
		for(i = 0; i < nclients; i++)
			if(i != idx && clients[i].state == ACCEPTED)
				toclientf(i, "CLIENT_DISCO %s", clients[idx].name);

	svr_rm(idx);
}

void svr_rm(int idx)
{
	int i;

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
	DEBUG(idx, DEBUG_ERROR, "client error %s", errno ? strerror(errno) : "unknown");
	svr_hup(idx);
}

#include "msgs.c"

char svr_recv(int idx)
{
	char in[MAX_LINE_LEN] = { 0 };
	int ret;

	switch(ret = recv(pollfds[idx].fd, in, MAX_LINE_LEN, MSG_PEEK)){
		case -1:
			fputs("recv() ", stderr);
			svr_err(idx);
			return 1;

		case 0:
			svr_hup(idx);
			return 1;
	}

	if(recv_newline(in, ret, pollfds[idx].fd, RECV_SLEEP_MS))
		/* not enough data */
		return 0;

	DEBUG(idx, DEBUG_RECV, "recv(): \"%s\"", in);

	if(clients[idx].state == ACCEPTING){
		if(!strncmp(in, "ME ", 3)){
			char *extra = in + 3, *esc;
			int len = strlen(extra), i;
			int udpport;

			if(len == 0){
				TO_CLIENT(idx, "ERR need non-zero length name");
				DEBUG(idx, DEBUG_STATUS, "%s", "zero length name");
				svr_hup(idx);
				return 1;
			}

			esc = strchr(extra, GROUP_SEPARATOR);
			if(!esc){
				TO_CLIENT(idx, "ERR no udp port given");
				DEBUG(idx, DEBUG_STATUS, "%s", "no udp port\n");
				svr_hup(idx);
				return 1;
			}

			*esc++ = '\0';
			if(sscanf(esc, "%d", &udpport) != 1){
				TO_CLIENT(idx, "ERR invalid udp port given");
				DEBUG(idx, DEBUG_STATUS, "%s", "invalid udp port\n");
				svr_hup(idx);
				return 1;
			}

			if(!validname(extra)){
				TO_CLIENT(idx, "ERR name contains invalid character(s)");
				DEBUG(idx, DEBUG_STATUS, "%s", "invalid character(s) in name\n");
				svr_hup(idx);
				return 1;
			}

			for(i = 0; i < nclients; i++)
				if(i != idx && clients[i].state == ACCEPTED && !strcmp(clients[i].name, extra)){
					TO_CLIENT(idx, "ERR name in use");
					DEBUG(idx, DEBUG_STATUS, "name %s in use", extra);
					svr_hup(idx);
					return 1;
				}

			clients[idx].name   = cstrdup(extra);
			clients[idx].isroot = 0;

			clients[idx].state = ACCEPTED;
			clients[idx].uaddr.sin_port = htons(udpport);

			DEBUG(idx, DEBUG_STATUS, "name accepted: %s", extra);

			TO_CLIENT(idx, "OK");
			stats.accepted++;

			for(i = 0; i < nclients; i++)
				if(i != idx && clients[i].state == ACCEPTED)
					toclientf(i, "CLIENT_CONN %s", extra);

		}else{
			TO_CLIENT(idx, "ERR need name");
			DEBUG(idx, DEBUG_STATUS, "%s", "name not given");
			svr_hup(idx);
			return 1;
		}
	}else{
		if(*in == 'D'){
			int i;

			/* dumb forward */
			for(i = 0; i < nclients; i++)
				if(clients[i].state == ACCEPTED && i != idx)
					toclientf(i, "%s", in);
		}else{
			struct
			{
				char *start;
				int len;
				void (*f)(int, char *);
			} msgfuncs[] = {
#define MSG_FUNC(s, f) { s, strlen(s), f }
				MSG_FUNC("MESSAGE "   , handle_message     ),
				MSG_FUNC("PRIVMSG "   , handle_privmsg     ),
				MSG_FUNC("CLIENT_LIST", handle_client_list ),
				MSG_FUNC("RENAME"     , handle_rename      ),
				MSG_FUNC("COLOUR"     , handle_colour      ),
				MSG_FUNC("KICK "      , handle_kick        ),
				MSG_FUNC("SU "        , handle_su          ),
				MSG_FUNC("TYPING "    , handle_typing      )
#undef MSG_FUNC
			};
			unsigned int i;
			int found = 0;

			for(i = 0; i < (sizeof(msgfuncs)/sizeof(msgfuncs[0])); i++)
				if(!strncmp(msgfuncs[i].start, in, msgfuncs[i].len)){
					msgfuncs[i].f(idx, in + msgfuncs[i].len);
					found = 1;
					break;
				}

			if(!found)
				toclientf(idx, "ERR command \"%s\" unknown", in);
		}
	}

	return 0;
}

const char *getuptime(void)
{
	static char t[64];
	time_t diff = time(NULL) - starttime;
	int sec, min, hr;

#define PLURAL_PAIR(i) i, (i == 1 ? "" : "s")

	sec = diff % 60;
	min = diff / 60;

	if(min){
		hr   = min / 60;
		min %= 60;
		if(hr)
			snprintf(t, sizeof t, "%d hour%s, %d minute%s, %d second%s",
					PLURAL_PAIR(hr), PLURAL_PAIR(min), PLURAL_PAIR(sec));
		else
			snprintf(t, sizeof t, "%d minute%s, %d second%s",
					PLURAL_PAIR(min), PLURAL_PAIR(sec));
	}else
		snprintf(t, sizeof t, "%d second%s",
				PLURAL_PAIR(sec));

#undef PLURAL_PAIR

	return t;
}

void sigh(int sig)
{
	if(sig == SIGINT){
		printf(
	      "%sComm v"VERSION_STR" %d Server Status (SIGQUIT ^\\ to quit)\n"
	      "Stats: %d allowed, %d refused, %d named login (%d invalid), %d su:s, %d still open\n"
	      "Uptime: %s\n"
	      , background ? "" : "\n", getpid(), stats.allowed, stats.blocked,
				stats.accepted, stats.allowed - stats.accepted, stats.sus, nclients, getuptime());

		if(nclients){
			int i;

			puts("Index FD Name");
			for(i = 0; i < nclients; i++)
				printf("%3d %3d  %s @ %s:{%d,%d}\n",
						i, pollfds[i].fd, clients[i].name ? clients[i].name : "(not logged in)",
						inet_ntoa(clients[i].addr.sin_addr),
						ntohs(clients[i].addr.sin_port),
						ntohs(clients[i].uaddr.sin_port));
		}

	}else if(sig == SIGUSR1){
		/* restart server */
		int idx;
		fputs("we get SIGUSR1 - server reset\n", stderr);
		for(idx = 0; idx < nclients; idx++){
			TO_CLIENT(idx, "INFO server reset");
			svr_rm(idx);
		}

	}else if(sig == SIGUSR2){
		if(++verbose > DEBUG_MAX)
			verbose = 0;
		printf("we get SIGUSR2 - verbose set to %d\n", verbose);

	}else if(sig == SIGHUP){
		char oldport[MAX_PORT_LEN];
		strcpy(oldport, glob_port);

		puts("got SIGHUP, reloading serverrc...");

		/* reload settans */
		cfg_end();
		if(cfg_init()){
			fputs("Couldn't reload serverrc\n", stderr);
			cleanup();
			exit(1);
		}

		if(strcmp(oldport, glob_port)){
			/*
			 * need to rebind
			 * gently does it - keep old connections,
			 * just change the listen port
			 */
			puts("port changed, rebinding...");
			close(server);
			close(udpserver);
			if(setup()){
				cleanup();
				exit(1);
			}
		}

	}else{
		fprintf(stderr, "we get signal %d\n", sig);

		cleanup();
		cfg_end();

		/* program normal exit point here */
		exit(128 + sig);
	}

	/* restore */
	signal(sig, &sigh);
}

int cfg_init()
{
	char *home, *fname;
	FILE *cfg;

	home = getenv("HOME");
	if(!home){
		fputs("Couldn't get $HOME\n", stderr);
		return 1;
	}

	fname = alloca(1 + strlen(home) + strlen(CFG_FILE));
	strcpy(fname, home);
	strcat(fname, /* has a / */CFG_FILE);

	cfg = fopen(fname, "r");
	if(!cfg){
		if(errno == ENOENT)
			return 0;

		perror(fname);
		return 1;
	}

	if(cfg_read(cfg)){
		fclose(cfg);
		return 1;
	}
	fclose(cfg);

	return 0;
}

int setup()
{
	struct sockaddr_in svr_addr;
	struct linger linger;
	int i;

	server    = socket(PF_INET, SOCK_STREAM, 0);
	udpserver = socket(PF_INET, SOCK_DGRAM,  0);

	if(server == -1 || udpserver == -1){
		perror("socket()");
		goto bail;
	}

	memset(&svr_addr, '\0', sizeof svr_addr);
	svr_addr.sin_family = AF_INET;
	svr_addr.sin_port   = htons(atoi(glob_port)); /* TODO: use getaddrinfo? */

	if(INADDR_ANY) /* usually optimised out of existence */
		svr_addr.sin_addr.s_addr = htonl(INADDR_ANY);


	/* wait if there's more data in the tubes */
	linger.l_onoff = 1;
	linger.l_linger = 1;
	if(setsockopt(server, SOL_SOCKET, SO_LINGER, &linger, sizeof linger) < 0)
		perror("warning: setsockopt(SO_LINGER)");

	/* listen immediately afterwards */
	i = 1;
	if(setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &i, sizeof i) < 0)
		perror("warning: setsockopt(SO_REUSEADDR)");

#define BIND(s) \
		bind(s, (struct sockaddr *)&svr_addr, sizeof svr_addr)


	if(BIND(server) == -1 || BIND(udpserver) == -1){
		fprintf(stderr, "bind on port %s: %s\n", glob_port, strerror(errno));
		goto bail;
	}

	if(listen(server, LISTEN_BACKLOG) == -1){
		perror("listen()");
		goto bail;
	}

	if(nonblock(server) || nonblock(udpserver)){
		perror("nonblock()");
		goto bail;
	}

	return 0;
bail:
	close(server);
	close(udpserver);
	return 1;
}

int check_new()
{
	struct sockaddr_in addr;
	socklen_t len;
	int cfd;

start:
	len = sizeof addr;
	cfd = accept(server, (struct sockaddr *)&addr, &len);

	if(cfd != -1){
		struct client *new;
		struct pollfd *newpfds;
		struct addrinfo ai;
		int idx;

		ai.ai_addr = (struct sockaddr *)&addr;
		ai.ai_addrlen = len; /* lol ipv6 */
		if(!restrict_hostallowed(&ai)){
			fprintf(stderr, "connection from %s dropped, not in allow list\n",
					addrtostr((struct sockaddr *)&addr));
#define WRITE(sock, str) write(sock, str, strlen(str))
			WRITE(cfd, "ERR denied\n");
#undef WRITE
			close(cfd);
			stats.blocked++;
			goto start;
		}
		stats.allowed++;

		idx = ++nclients - 1;

#define INTERRUPT_WRAP(funcall, failcode) \
	while(!(funcall)) \
		if(errno != EINTR) \
			failcode


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

		memcpy(&clients[idx].addr,  &addr, sizeof clients[idx].addr);
		memcpy(&clients[idx].uaddr, &addr, sizeof clients[idx].uaddr); /* port updated later */

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
	return 0;
}

int poll_clients()
{
	int i, ret;

start:
	for(i = 0; i < nclients; i++)
		pollfds[i].events = POLLIN | POLLERR | POLLHUP;

	ret = poll(pollfds, nclients, POLL_SLEEP_MS);
	/* need to timeout, since we're also accept()ing above */

	/*
		* ret = 0: timeout
		* ret > 0: number of fds that have status
		* ret < 0: err
		*/
	if(ret == 0)
		return 0;
	else if(ret < 0){
		if(errno == EINTR)
			goto start;

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
	return 0;
}

int poll_udp()
{
	ssize_t ret;
	char buf[MAX_UDP_PACKET];
	char *iter;
	socklen_t fromlen;
	struct sockaddr_in from;
	int id;

start:
	fromlen = 0;
	memset(&from, 0, sizeof from);

	ret = recvfrom(udpserver, buf, sizeof buf - 1, 0,
		(struct sockaddr *)&from, &fromlen);

	buf[sizeof buf - 1] = '\0';

	if(ret == -1){
		if(errno == EINTR || errno == EAGAIN)
			return 0;
		perror("udp recvfrom()");
		return 1;
	}

	/* format is name^]packet data */
	iter = memchr(buf, GROUP_SEPARATOR, ret);
	if(iter){
		*iter++ = '\0';
		id = findclient(buf);

		if(id != -1){
			int i;

			ret = strlen(iter);

			for(i = 0; i < nclients; i++)
				if(i != id){
						if(sendto(udpserver, iter, ret,
								0, (struct sockaddr *)&clients[i].uaddr,
								sizeof clients[i].uaddr) == -1)
							perror("udp sendto()");
					}
		}else if(verbose >= DEBUG_ERROR)
			fprintf(stderr, "no owner for udp packet from %s\n", inet_ntoa(from.sin_addr));

	}else if(verbose >= DEBUG_ERROR)
		fprintf(stderr, "got invalid udp packet from %s\n", inet_ntoa(from.sin_addr));

	goto start; /* keep processing */
}

int main(int argc, char **argv)
{
	int i, log = 0;

	starttime = time(NULL);
	strcpy(glob_port, DEFAULT_PORT);

	if(setjmp(allocerr)){
		perror("longjmp'd to allocerr");
		return 1;
	}

	/* ignore sigpipe - sent when recv() called on a disconnected socket */
	if( signal(SIGPIPE, SIG_IGN) == SIG_ERR ||
			signal(SIGUSR1, &sigh)   == SIG_ERR ||
			signal(SIGUSR2, &sigh)   == SIG_ERR ||
			signal(SIGHUP,  &sigh)   == SIG_ERR ||
			/* usual suspects... */
			signal(SIGINT,  &sigh)   == SIG_ERR ||
			signal(SIGQUIT, &sigh)   == SIG_ERR ||
			signal(SIGTERM, &sigh)   == SIG_ERR ||
			signal(SIGSEGV, &sigh)   == SIG_ERR){
		perror("signal()");
		return 1;
	}

	if(cfg_init())
		return 1;

	for(i = 1; i < argc; i++)
		if(!strcmp(argv[i], "-p")){
			if(++i < argc)
				strncpy(glob_port, argv[i], sizeof glob_port);
			else{
				fputs("need port\n", stderr);
				return 1;
			}

		}else if(!strcmp(argv[i], "-P")){
			if(++i < argc){
				strncpy(glob_pass, argv[i], sizeof glob_pass);
				memset(argv[i], '*', strlen(argv[i])); /* too late, but eh... */
			}else{
				fputs("need pass\n", stderr);
				return 1;
			}

		}else if(!strcmp(argv[i], "-d")){
			if(++i < argc)
				strncpy(glob_desc, argv[i], sizeof glob_desc);
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

#define USAGE(args) do { fprintf args; goto usage; } while(0)

			if(*p != '\0')
				USAGE((stderr, "nothing should be after -v\n"));

		}else if(!strcmp(argv[i], "-b")){
			background = 1;
		}else if(!strcmp(argv[i], "-l")){
			log = 1;
		}else
			USAGE((stderr, "unknown arg %s\n", argv[i]));

	if(!*glob_pass)
		fputs("'root' password disabled\n", stderr);
	if(!*glob_desc)
		strcpy(glob_desc, "Server");

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
		fprintf(stderr, "%s: %sforking to background\n", *argv, log ? "" : "closing output and ");

		/*
		 * TODO: use daemon(3)?
		 *
		 * daemon(1, 0);
		 * 1 - don't chdir("/")
		 * 0 - close STD{IN,OUT,ERR}
		 */

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
	}

	setvbuf(stdout, NULL, _IONBF, 0);

	if(setup())
		return 1;

	printf("Comm v"VERSION_STR" %d Server init @ localhost:%s\n", getpid(), glob_port);

	/* main */
	for(;;)
		if(check_new() || poll_clients() || poll_udp())
			return 1;


	/* UNREACHABLE */

usage:
	fprintf(stderr, "Usage: %s [-p port] [-v*] [-b] [-l]\n"
	                " -p: Port to listen on\n"
	                " -P: 'root' password\n"
	                " -d: Server description\n"
	                " -v: Verbose\n"
	                " -b: Fork to background\n"
	                " -l: Redirect output to logs\n"
	                "[pPdv] take arguments\n"
	                "\n"
	                "Verbosity levels:\n"
	                "1: Show connections\n"
	                "2: Show status changes\n"
	                "3: Show recv() data\n"
	                "4: Show send() data\n"
	                "\n"
	                "Signals:\n"
	                "SIGINT: status\n"
	                "SIGUSR1: server reset\n"
	                "SIGUSR2: verbosity level change\n"
	                "SIGHUP: reload config\n"
	                "SIG*: quit\n"
	                , *argv);
	return 1;
}
