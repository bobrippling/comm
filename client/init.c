#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <errno.h>
#include <setjmp.h>

#include "../settings.h"
#include "comm.h"

#define DEFAULT_NAME  "Timmy"
#define LOCALE        "en_GB.UTF-8"

jmp_buf allocerr;

static void usage(char *);
#define USAGE() do { usage(*argv); return 1; } while(0)

static void usage(char *progname)
{
	fprintf(stderr, "Usage: %s [options] host\n", progname);
	fputs(" -p port: Port\n"
			  " -n name: Username\n", stderr);
}


int main(int argc, char **argv)
{
	int i, port = DEFAULT_PORT;
	char argv_options = 1, *host = NULL;
	const char *name = DEFAULT_NAME;

	setlocale(LC_ALL, LOCALE);

	if(setjmp(allocerr)){
		perror("malloc()");
		return 1;
	}

	for(i = 1; i < argc; i++)
		if(argv_options && argv[i][0] == '-'){
			if(strlen(argv[i]) == 2)
				switch(argv[i][1]){
					case '-':
						argv_options = 0;
						break;

					case 'n':
						if(++i < argc)
							name = argv[i];
						else{
							fputs("need name\n", stderr);
							return 1;
						}
						break;

					case 'p':
						if(++i < argc)
							port = atoi(argv[i]);
						else{
							fputs("need port\n", stderr);
							return 1;
						}
						break;

					default:
						fprintf(stderr, "Unrecognised arg: \"%s\"\n", argv[1]);
						USAGE();
				}
			else
				/* strlen '-...' != 2 */
				USAGE();
		}else if(!host)
			host = argv[i];
		else
			USAGE();

#if 0
	TODO: windows
	if(daemon){
		puts("daemonising...");
		FreeConsole();
	}else
		puts("not daemonising");
#endif

	return comm_main(name, host, port);
}
