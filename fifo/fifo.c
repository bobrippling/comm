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
#include <comm.h>

#include "../common/util.h"

#define OUTPUT(f, fmt) do{ \
		va_list l; \
		va_start(l, fmt); \
		output(f, fmt, l); \
		va_end(l); \
	}while(0)

#define PATH_LEN 128

/* prototypes */
int init_files(void);
void term_files(void);

void output(const char *fname, const char *msg, va_list l);
void outputf(const char *fname, const char *msg, ...);

void commcallback(enum comm_callbacktype type, const char *fmt, ...);
void proc_cmd(const char *buffer);

int lewp(void);
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
int port   = -1;

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
			init_files();
			f = fopen(fname, "a");
		}
		clos = 1;
	}else
		f = stderr;

	if(f){
		vfprintf(f, msg, l);
		fputc('\n', f);
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
	snprintf(dir, PATH_LEN, "%s:%d", host, port);
	if(mkdir(dir, 0700)){
		if(errno == EEXIST)
			fprintf(stderr, "not overwriting %s\n", dir);
		else
			perror("mkdir()");
		return 1;
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

	va_start(l, fmt);

#define TYPE(e, f) case e: output(f, fmt, l); break
	switch(type){
		TYPE(COMM_INFO,         file_info);
		TYPE(COMM_CLIENT_CONN,  file_info);
		TYPE(COMM_CLIENT_DISCO, file_info);
		TYPE(COMM_RENAME,       file_info);

		TYPE(COMM_ERR,          file_err);
		TYPE(COMM_CLIENT_LIST,  file_clients);

		TYPE(COMM_MSG,          file_output);
	}
#undef TYPE

	va_end(l);
}

void proc_cmd(const char *buffer)
{
	if(!strcmp(buffer, "ls"))
		outputf(file_err, "ls: TODO");
		/* TODO comm_listclients(); */
	else if(!strcmp(buffer, "exit"))
		finito = 1;
	else
		outputf("unknown command: %s (use \"ls\" or \"exit\")", buffer);
}

int lewp()
{
	struct pollfd pfd[2];
	static char buffer[LINE_SIZE], *nl;
	int nread, saverrno;

	pfd[0].fd     = fd_input;
	pfd[1].fd     = fd_cmd;
	pfd[0].events = pfd[1].events = POLLIN;

	while(!finito){
		if(comm_recv(&commt, &commcallback)){
			fprintf(stderr, "comm_recv() failed: %s\n", comm_lasterr(&commt));
			return 1;
		}

		switch(poll(pfd, 2, CLIENT_UI_WAIT)){
			case 0:
				/* notan happan */
				continue;
			case -1:
				perror("poll()");
				return 1;
		}

#define READ(fd) \
			nread = read(fd, buffer, LINE_SIZE); \
			saverrno = errno; \
			errno = saverrno; \
	 \
			switch(nread){ \
				case -1: \
					perror("read()"); \
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


int main(int argc, char **argv)
{
	char *name = NULL;
	int i, ret = 0;

	for(i = 1; i < argc; i++)
		if(!name)
			name = argv[i];
		else if(!host)
			host = argv[i];
		else if(port == -1)
			port = atoi(argv[i]);
		else
			goto usage;

	if(!host || !name || !port)
		goto usage;

	if(port == -1)
		port = DEFAULT_PORT;

	if(setjmp(allocerr)){
		perror("malloc()");
		return 1;
	}

	if(init_files())
		return 1;

	comm_init(&commt);

	if(comm_connect(&commt, host, port, name)){
		fprintf(stderr, "%s: couldn't connect: %s\n", *argv, comm_lasterr(&commt));
		return 1;
	}

	ret = lewp();

	comm_close(&commt);
	term_files();

	return ret;
usage:
	printf("Usage: %s name host [port]\n", *argv);
	return 1;
}
