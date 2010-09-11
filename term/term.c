#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include <unistd.h>
#include <poll.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>

#include "../config.h"
#include "../libcomm/comm.h"

void callback(enum comm_callbacktype, const char *fmt, ...);

void callback(enum comm_callbacktype type, const char *fmt, ...)
{
	const char *type_str = "unknown";
	va_list l;

#define TYPE(e, s) case e: type_str = s; break
	switch(type){
		TYPE(COMM_MSG,  "message");
		TYPE(COMM_INFO, "info");
		TYPE(COMM_ERR,  "err");
		TYPE(COMM_RENAME,  "rename");
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

int main(int argc, char **argv)
{
	comm_t ct;
	char buffer[128], *host = NULL, *name = NULL;
	const char *port = NULL;
	struct pollfd fds;
	int i;

	for(i = 1; i < argc; i++)
		if(!name)
			name = argv[i];
		else if(!host)
			host = argv[i];
		else if(!port){
			char *p = argv[i];
			while(*p && isdigit(*p))
				p++;
			if(*p)
				goto usage;
			port = argv[i];
		}else
			goto usage;


	if(!host || !name)
		goto usage;
	if(!port)
		port = DEFAULT_PORT;

	comm_init(&ct);
	if(comm_connect(&ct, host, port, name)){
		printf("couldn't connect: %s\n", comm_lasterr(&ct));
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

					if(*buffer == '/'){
						if(!strncmp(buffer+1, "rename ", 7))
							if(buffer[8])
								comm_rename(&ct, buffer + 8);
							else
								fprintf(stderr, "%s: need name!\n", *argv);
						else
							fputs("Invalid command: use /rename\n", stderr);
					}else{
						if((nl = strchr(buffer, '\n')))
							*nl = '\0';
						comm_sendmessage(&ct, buffer);
					}
				}else
					goto bail;
		}

bail:
	comm_close(&ct);

	return 0;
usage:
	fprintf(stderr, "Usage: %s name host [port]\n", *argv);
	return 1;
}
