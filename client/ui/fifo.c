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

#include "../../settings.h"
#include "../comm.h"
#include "ui.h"

#define OUTPUT(fmt) do{ \
		va_list l; \
		va_start(l, fmt); \
		output(fmt, l); \
		va_end(l); \
	}while(0)


static const char *file_output = "fifocomm_out";
static const char *file_input  = "fifocomm_in";

static void output(const char *msg, va_list l);
void ui_info(   const char *fmt, ...) { OUTPUT(fmt); }
void ui_message(const char *fmt, ...) { OUTPUT(fmt); }
void ui_warning(const char *fmt, ...) { OUTPUT(fmt); }
void ui_perror( const char *msg     ) { ui_error("%s: %s", msg, strerror(errno)); }

void ui_error(const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	vfprintf(stderr, fmt, l);
	va_end(l);
	fputc('\n', stderr);
}


void output(const char *msg, va_list l)
{
	FILE *f = fopen(file_output, "a");
	if(f){
		vfprintf(f, msg, l);
		fputc('\n', f);
		fclose(f);
	}
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
	if(initfifo())
		return 1;
	close(open(file_output, O_WRONLY | O_TRUNC));
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

	fd = open(file_input, O_RDONLY | O_NONBLOCK);

	if(fd == -1){
		initfifo();
		fd = open(file_input, O_RDONLY | O_NONBLOCK);
		if(fd == -1){
			ui_perror("open()");
			return 1;
		}
	}

	pfd.fd     = fd;
	pfd.events = POLLIN;

	switch(poll(&pfd, 1, 100)){
		case 0:
			return 0;
		case -1:
			ui_perror("poll()");
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
