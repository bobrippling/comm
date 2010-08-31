#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <unistd.h>
#include <poll.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>

#include "comm.h"

void callback(enum callbacktype, const char *fmt, ...);


void callback(enum callbacktype type, const char *fmt, ...)
{
	const char *type_str = "unknown";
	va_list l;

#define TYPE(e, s) case e: type_str = s; break
	switch(type){
		TYPE(COMM_MSG,  "message");
		TYPE(COMM_INFO, "info");
		TYPE(COMM_ERR,  "err");
		TYPE(COMM_CLIENT_CONN,  "client_conn");
		TYPE(COMM_CLIENT_DISCO, "client_disco");
		TYPE(COMM_CLIENT_LIST,  "client_list");
	}
#undef TYPE

	printf("%s: ", type_str);

	va_start(l, fmt);
	vprintf(fmt, l);
	va_end(l);
	putchar('\n');
}

int main(void)
{
	comm_t ct;
	char buffer[128];
	struct pollfd fds;

	comm_init(&ct);

	if(comm_open(&ct, "127.0.0.1", 5678, "Tim")){
		printf("couldn't open: %s\n", comm_lasterr(&ct));
		return 1;
	}

	puts("connect()ed");

	fds.fd = STDIN_FILENO;
	fds.events = POLLIN;

	while(!comm_recv(&ct, &callback))
		switch(poll(&fds, 1, 250)){
			case 0:
				break;
			case -1:
				perror("poll()");
				goto bail;
			default:
				if(fgets(buffer, sizeof buffer, stdin)){
					char *nl;
					if((nl = strchr(buffer, '\n')))
						*nl = '\0';
					comm_sendmessage(&ct, buffer);
				}else
					goto bail;
		}

bail:
	comm_close(&ct);

	return 0;
}
