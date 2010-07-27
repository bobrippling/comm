#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <poll.h>
#include <unistd.h>

#include "../comm.h"
#include "../../settings.h"

#define SLEEP_MS 50

#define print(f, fmt) do{ \
		va_list l; \
		va_start(l, fmt); \
		vfprintf(f, fmt, l); \
		va_end(l); \
		fputc('\n', f); \
	}while(0);

void ui_info(   const char *fmt, ...) { print(stdout, fmt); }
void ui_message(const char *fmt, ...) { print(stdout, fmt); }

void ui_warning(const char *fmt, ...) { print(stderr, fmt); }
void ui_error(  const char *fmt, ...) { print(stderr, fmt); }

void ui_perror(const char *err) { perror(err); }

void showclients()
{
	/* FIXME */
	/*TO_SERVER_F("CLIENT_LIST");*/
	/*
	CLIENT_LIST_START
	CLIENT_LIST %s
	CLIENT_LIST_END
	*/
}

/* return ~0 to exit */
int ui_doevents()
{
	static char buffer[LINE_SIZE], *nl;

	if(comm_getstate() == ACCEPTED){
		struct pollfd pfd;

		pfd.fd = STDIN_FILENO;
		pfd.events = POLLIN;

		switch(poll(&pfd, 1, SLEEP_MS)){
			case 0:
				return 0;
			case -1:
				ui_perror("poll()");
				return 1;
		}

		if(fgets(buffer, LINE_SIZE, stdin)){
			if((nl = strchr(buffer, '\n')))
				*nl = '\0';

			if(*buffer == '/'){
				/* command */
				if(!strcmp(buffer+1, "exit"))
					return 1;
				else if(!strcmp(buffer+1, "ls"))
					showclients();
				else
					printf("unknown command: %s\n", buffer+1);
			}else{
				/* message */
				comm_sendmessage("%s", buffer);
			}
			return 0;
		}else{
			if(ferror(stdin))
				perror("fgets()");
			/* eof */
			return 1;
		}
	}

	return 0;
}
