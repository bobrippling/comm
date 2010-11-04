#define _POSIX_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <alloca.h>
#include <setjmp.h>

#include <arpa/inet.h>

#include "../config.h"
#include "../libcomm/comm.h"

#include "../common/util.h"

#define OUTPUT(f, fmt) do{ \
		va_list l; \
		va_start(l, fmt); \
		output(f, fmt, l); \
		va_end(l); \
	}while(0)

#define PATH_LEN 128
#define FIFO_POLL_WAIT 100

/* prototypes */
int init_files(void);
void term_files(void);

void output(const char *fname, const char *msg, va_list l);
void outputf(const char *fname, const char *msg, ...);

void commcallback(enum comm_callbacktype type, const char *fmt, ...);
void proc_cmd(const char *buffer);

int lewp(void);
int daemonise(void);
int main(int argc, char **argv);


/* vars */
comm_t commt;
jmp_buf allocerr;

char dir[PATH_LEN] = { 0 };
char file_output [PATH_LEN] = { 0 },
     file_info   [PATH_LEN] = { 0 },
     file_clients[PATH_LEN] = { 0 },
     file_err    [PATH_LEN] = { 0 },
     /* fifos below */
     file_cmd    [PATH_LEN] = { 0 },
     file_input  [PATH_LEN] = { 0 };

char *host = NULL;
const char *port = NULL;

int fd_input = -1, fd_cmd = -1, finito = 0;


/* funcs */

void outputf(const char *fname, const char *msg, ...)
{
	va_list l;
	va_start(l, msg);
	output(fname, msg, l);
	va_end(l);
}

void output(const char *fname, const char *msg, va_list l)
{
	char clos = 0;
	FILE *f;

	if(fname){
		f = fopen(fname, "a");
		if(!f){
			/* attempt to create dir again */
			if(init_files() == 0)
				f = fopen(fname, "a");
			/*else p the error */
		}
		clos = 1;
	}else
		f = stderr;

	if(f){
		vfprintf(f, msg, l);
		if(clos)
			fclose(f);
	}else
		perrorf("fopen(): %s", fname);
}

/* end output funcs */

void term_files()
{
	/* only close + remove the fifos */
	close(fd_input);
	close(fd_cmd);

	remove(file_input);
	remove(file_cmd);
}

int init_files()
{
	snprintf(dir, PATH_LEN, "%s:%s", host, port);
	if(mkdir(dir, 0700)){
		if(errno == EEXIST){
			fprintf(stderr, "not overwriting %s\n", dir);
			return 2;
		}else{
			perror("mkdir()");
			return 1;
		}
		/* unreachable */
	}

	snprintf(file_input,   PATH_LEN, "%s/%s", dir, "in");
	snprintf(file_cmd,     PATH_LEN, "%s/%s", dir, "cmd");

	snprintf(file_output,  PATH_LEN, "%s/%s", dir, "out");
	snprintf(file_info,    PATH_LEN, "%s/%s", dir, "server");
	snprintf(file_clients, PATH_LEN, "%s/%s", dir, "clients");
	snprintf(file_err,     PATH_LEN, "%s/%s", dir, "err");


	if(mkfifo(file_input, 0600) == -1 && errno != EEXIST){
		perrorf("mkfifo(): %s: ", file_input);
		return 1; /* ya */
	}
	if(mkfifo(file_cmd, 0600) == -1 && errno != EEXIST){
		perrorf("mkfifo(): %s: ", file_cmd);
		goto bail;
	}

	if((fd_input = open(file_input, O_RDONLY | O_NONBLOCK, 0600)) == -1){
		perrorf("open(): %s: ", file_input);
		goto bail;
	}
	if((fd_cmd = open(file_cmd, O_RDONLY | O_NONBLOCK, 0600)) == -1){
		perrorf("open(): %s: ", file_cmd);
		close(fd_input);
		goto bail;
	}

	return 0;
bail:
	remove(file_input);
	remove(file_cmd);
	return 1;
}

void commcallback(enum comm_callbacktype type, const char *fmt, ...)
{
	va_list l;
	const char *pre = NULL, *fname = NULL;

#define TYPE(e, f, p) case e: fname = f; pre = p; break
	switch(type){
		TYPE(COMM_INFO,         file_info, "info");
		TYPE(COMM_SERVER_INFO,  file_info, "server info");
		TYPE(COMM_CLIENT_CONN,  file_info, "client connected");
		TYPE(COMM_CLIENT_DISCO, file_info, "client disconnected");
		TYPE(COMM_RENAME,       file_info, "client renamed");

		TYPE(COMM_ERR,          file_err,  "error");

		case COMM_PRIVMSG:
			pre = "privmsg";
		case COMM_MSG:
			fname = file_output;
			break;

		case COMM_CLIENT_LIST:
		case COMM_SELF_RENAME:
		{
			FILE *f = fopen(file_clients, "w"); /* truncate */
			struct list *l;

			if(!f && init_files() == 0)
				f = fopen(file_clients, "w");

			if(!f){
				perrorf("fopen(): %s", file_clients);
			}else{
				fprintf(f, "me: %s\n", comm_getname(&commt));
				for(l = comm_clientlist(&commt); l; l = l->next)
					fprintf(f, "client: %s\n", l->name);
				fclose(f);
			}
			return;
		}

		case COMM_STATE_CHANGE:
			switch(comm_state(&commt)){
				case COMM_DISCONNECTED:
					outputf(file_info, "Connection closed: %s\n", comm_lasterr(&commt));
					finito = 1;

				case COMM_CONNECTING:
				case COMM_VERSION_WAIT:
				case COMM_NAME_WAIT:
				case COMM_ACCEPTED:
					break;
			}
			return;
	}
#undef TYPE

	if(pre)
		outputf(fname, "%s: ", pre);

	va_start(l, fmt);
	output(fname, fmt, l);
	va_end(l);

	outputf(fname, "\n");
}

void proc_cmd(const char *buffer)
{
	if(!strcmp(buffer, "exit"))
		finito = 1;
	else if(!strncmp(buffer, "su ", 3))
		comm_su(&commt, buffer+3);
	else if(!strncmp(buffer, "kick ", 5))
		comm_kick(&commt, buffer+5);
	else if(!strncmp(buffer, "rename ", 7))
		comm_rename(&commt, buffer+7);
	else
		outputf(file_err, "unknown command: ``%s'' (use rename, su, kick or exit)\n", buffer);
}

int lewp()
{
	struct pollfd pfd[2];
	static char buffer[MAX_LINE_LEN], *nl;
	int nread, saverrno;

	pfd[0].fd     = fd_input;
	pfd[1].fd     = fd_cmd;
	pfd[0].events = pfd[1].events = POLLIN;

	while(!finito){
		if(comm_recv(&commt, &commcallback)){
			outputf(file_err, "comm_recv() failed: %s\n", comm_lasterr(&commt));
			return 1;
		}

		switch(poll(pfd, 2, FIFO_POLL_WAIT)){
			case 0:
				/* notan happan */
				continue;
			case -1:
				perrorf("%s", "poll()");
				return 1;
		}

#define READ(fd) \
			nread = read(fd, buffer, MAX_LINE_LEN); \
			saverrno = errno; \
	 \
			switch(nread){ \
				case -1: \
					perrorf("%s", "read()"); \
					return 1; \
				case 0: \
					break; \
				default: \
					buffer[nread-1] = '\0'; \
					if((nl = strchr(buffer, '\n'))) \
						*nl = '\0'; \
			}

#define BIT(x, b) (((x) & (b)) == (b))
		if(BIT(pfd[0].revents, POLLIN)){
			READ(pfd[0].fd);
			if(nread > 0)
				/* message */
				comm_sendmessage(&commt, buffer);
		}

		if(BIT(pfd[1].revents, POLLIN)){
			READ(pfd[1].fd);
			if(nread > 0)
				proc_cmd(buffer);
		}
	}
	return 0;
}

int daemonise()
{
	switch(fork()){
		case -1:
			perror("fork()");
			return 1;
		case 0:
			return 0;
		default:
			puts("forked to background");
			exit(0);
	}
}

int main(int argc, char **argv)
{
	char *name = NULL;
	int i = 1, ret = 0, daemon = 0;

	if(argc > 1 && !strcmp(argv[1], "-d")){
		i++;
		daemon = 1;
	}

	for(; i < argc; i++)
		if(!name)
			name = argv[i];
		else if(!host)
			host = argv[i];
		else if(!port)
			port = argv[i];
		else{
			fprintf(stderr, "Unknown option: ``%s''\n", argv[i]);
			goto usage;
		}

	if(!host || !name)
		goto usage;

	if(!port)
		port = DEFAULT_PORT;

	if(setjmp(allocerr)){
		perror("malloc()");
		return 1;
	}

	if((ret = init_files()))
		return ret;

	if(daemon && daemonise()){
		term_files();
		return 1;
	}

	comm_init(&commt);

	if(comm_connect(&commt, host, port, name)){
		outputf(file_err, "%s: couldn't connect: %s\n", *argv, comm_lasterr(&commt));
		term_files();
		return 1;
	}

	ret = lewp();

	comm_close(&commt);
	term_files();

	return ret;
usage:
	printf("Usage: %s [-d] name host [port]\n"
	       "  -d: daemonise\n"
	       , *argv);
	return 1;
}
