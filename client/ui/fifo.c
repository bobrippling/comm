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

#include "../../config.h"
#include "../comm.h"
#include "ui.h"

#define OUTPUT(f, fmt) do{ \
		va_list l; \
		va_start(l, fmt); \
		output(f, fmt, l); \
		va_end(l); \
	}while(0)


static const char *file_output = "out";
static const char *file_input  = "in";

static void output(FILE *, const char *, va_list);

void ui_info(   const char *fmt, ...) { OUTPUT(NULL, fmt); }
void ui_message(const char *fmt, ...) { OUTPUT(NULL, fmt); }
void ui_warning(const char *fmt, ...) { OUTPUT(stderr, fmt); }
void ui_error(  const char *fmt, ...) { OUTPUT(stderr, fmt); }
void ui_perror( const char *msg     ) { ui_error("%s: %s", msg, strerror(errno)); }


void output(FILE *f, const char *msg, va_list l)
{
	char clos = 0;
	if(!f){
		f = fopen(file_output, "a");
		clos = 1;
	}

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
	if(mkfifo(file_input, 0666) == -1 && errno != EEXIST){
		perror("mkfifo()");
		return 1;
	}
	return 0;
}

int ui_init()
{
	if(access(file_output, F_OK) == 0){
		fprintf(stderr, "not overwriting %s\n", file_output);
		return 1;
	}
	if(initfifo())
		return 1;
	creat(file_output, 0600); /* so the user can tail -f */
	return 0;
}

void ui_term()
{
	remove(file_input);
}

int ui_doevents()
{
	struct pollfd pfd;
	static char buffer[LINE_SIZE], *nl;
	int fd, nread, saverrno;

	fd = open(file_input, O_RDONLY | O_NONBLOCK, 0600);

	if(fd == -1){
		if(initfifo())
			return 1;
		fd = open(file_input, O_RDONLY | O_NONBLOCK);
		if(fd == -1){
			ui_perror("open()");
			return 1;
		}
	}

	pfd.fd     = fd;
	pfd.events = POLLIN;

	switch(poll(&pfd, 1, CLIENT_UI_WAIT)){
		case 0:
			close(fd);
			return 0;
		case -1:
			ui_perror("poll()");
			close(fd);
			return 1;
	}

	nread = read(fd, buffer, LINE_SIZE);
	saverrno = errno;
	close(fd);
	errno = saverrno;

	switch(nread){
		case -1:
			perror("read()");
			return 1;
		case 0:
			return 0;
	}


	buffer[nread-1] = '\0';
	if((nl = strchr(buffer, '\n')))
		*nl = '\0';

	/* message */
	comm_sendmessage("%s", buffer);
	return 0;
}
