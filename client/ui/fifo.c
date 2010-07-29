#define _POSIX_SOURCE
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <alloca.h>

#include "../../config.h"
#include "../comm.h"
#include "ui.h"
#include "../common.h"

#define OUTPUT(f, fmt) do{ \
		va_list l; \
		va_start(l, fmt); \
		output(f, fmt, l); \
		va_end(l); \
	}while(0)

#define PATH_LEN 128

static char dir[PATH_LEN] = { 0 };
static char file_output [PATH_LEN] = { 0 },
            file_info   [PATH_LEN] = { 0 },
            file_clients[PATH_LEN] = { 0 },
            /* fifos below */
            file_cmd    [PATH_LEN] = { 0 },
            file_input  [PATH_LEN] = { 0 };

static const char *host;
static int port, fd_input = -1, fd_cmd = -1, finito = 0;

static void output( const char *fname, const char *msg, va_list);
static void outputf(const char *fname, const char *msg, ...);
static void proc_cmd(const char *buffer);
static int setup_files(void);

void ui_info(   const char *fmt, ...) { OUTPUT(file_info,  fmt); }
void ui_message(const char *fmt, ...) { OUTPUT(file_output, fmt); }
void ui_warning(const char *fmt, ...) { OUTPUT(NULL, fmt); }
void ui_error(  const char *fmt, ...) { OUTPUT(NULL, fmt); }
void ui_perror( const char *msg     ) { ui_error("%s: %s", msg, strerror(errno)); }

void ui_gotclient(const char *cname)
{
	outputf(file_clients, "Client: %s", cname);
}

static void outputf(const char *fname, const char *msg, ...)
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
			setup_files();
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
		ui_error("fopen()");
}

int initfifo()
{
	if(mkfifo(file_input, 0600) == -1 && errno != EEXIST){
		perrorf("mkfifo(): %s: ", file_input);
		return 1;
	}
	if(mkfifo(file_cmd, 0600) == -1 && errno != EEXIST){
		perrorf("mkfifo(): %s: ", file_cmd);
		return 1;
	}
	return 0;
}

static int setup_files()
{
	if(mkdir(dir, 0700))
		if(errno != EEXIST){
			perror("mkdir()");
			return 1;
		}

	if(initfifo())
		return 1;

	if((fd_input = open(file_input, O_RDONLY | O_NONBLOCK, 0600)) == -1){
		perrorf("open(): %s: ", file_input);
		remove(file_input);
		return 1;
	}

	if((fd_cmd = open(file_cmd, O_RDONLY | O_NONBLOCK, 0600)) == -1){
		perrorf("open(): %s: ", file_cmd);
		close(fd_input);
		remove(file_input);
		remove(file_cmd);
		return 1;
	}

	return 0;
}

int ui_init(const char *host2, int port2)
{
	if(!host2){
		fputs("need host\n", stderr);
		return 1;
	}

	host = host2;
	port = port2;

	snprintf(dir,          PATH_LEN, "%s:%d", host, port);
	snprintf(file_output,  PATH_LEN, "%s/%s", dir, "out");
	snprintf(file_info,    PATH_LEN, "%s/%s", dir, "server");
	snprintf(file_clients, PATH_LEN, "%s/%s", dir, "clients");

	snprintf(file_input,   PATH_LEN, "%s/%s", dir, "in");
	snprintf(file_cmd,     PATH_LEN, "%s/%s", dir, "cmd");

	if(access(dir, F_OK) == 0){
		fprintf(stderr, "not overwriting %s\n", dir);
		return 1;
	}

	if(setup_files())
		return 1;
	return 0;
}

void ui_term()
{
	/* only close + remove the fifos */
	close(fd_input);
	close(fd_cmd);

	remove(file_input);
	remove(file_cmd);
}

static void proc_cmd(const char *buffer)
{
	if(*buffer == '/')
		if(!strcmp(buffer+1, "ls"))
			comm_listclients();
		else if(!strcmp(buffer+1, "exit") || !strcmp(buffer+1, "quit"))
			finito = 1;
		else
			ui_warning("unknown command: %s", buffer + 1);
	else
		ui_warning("invalid command: %s", buffer);
}

int ui_doevents()
{
	struct pollfd pfd[2];
	static char buffer[LINE_SIZE], *nl;
	int nread, saverrno;

	pfd[0].fd     = fd_input;
	pfd[1].fd     = fd_cmd;
	pfd[0].events = pfd[1].events = POLLIN;

	switch(poll(pfd, 2, CLIENT_UI_WAIT)){
		case 0:
			return 0;
		case -1:
			ui_perror("poll()");
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

	if(BIT(pfd[0].revents, POLLIN)){
		READ(pfd[0].fd);
		if(nread > 0)
			/* message */
			comm_sendmessage("%s", buffer);
	}

	if(BIT(pfd[1].revents, POLLIN)){
		READ(pfd[1].fd);
		if(nread > 0)
			proc_cmd(buffer);
	}

	return finito;
}
