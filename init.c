#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <errno.h>
#include <setjmp.h>

#include "headers.h"
#if WINDOWS
#include <windows.h>
#endif

#include "settings.h"
#include "ui/ui.h"

#define DEFAULT_PORT	7331
#define DEFAULT_NAME	"Noob"
#define LOCALE				"en_GB.UTF-8"


static void usage(char *);
#define USAGE() do { usage(*argv); return 1; } while(0)

struct settings global_settings;
jmp_buf allocerr;

int main(int argc, char **argv)
{
	int ret = 0, i;
	char argv_options = 1;
#if WINDOWS
	char daemon = 1;
#endif

	setlocale(LC_ALL, LOCALE);

	if(setjmp(allocerr)){
		ui_message(UI_ERROR, "Out of memory!");
		return 1;
	}

	global_settings.name   = DEFAULT_NAME;
	global_settings.port   = DEFAULT_PORT;

	for(i = 1; i < argc; i++)
		if(argv_options && argv[i][0] == '-'){
			if(strlen(argv[i]) == 2)
				switch(argv[i][1]){
					case '-':
						argv_options = 0;
						break;

#if WINDOWS
					case 'f':
						daemon = 0;
						break;
#endif

					default:
						fprintf(stderr, "Unrecognised arg: \"%s\"\n", argv[1]);
						USAGE();
				}
			else
				/* strlen '-...' != 2 */
				USAGE();
		}else
			/* ! "-..." */
			USAGE();

#if WINDOWS
	if(daemon){
		puts("daemonising...");
		FreeConsole();
	}else
		puts("not daemonising");
#endif

	ui_main();

	return ret;
}

static void usage(char *progname)
{
	fprintf(stderr, "Usage: %s [options] host\n", progname);
	fputs(" -l:	Listen/Host mode\n"
			  " -p: Port\n"
			  " -u:	Username\n", stderr);
}

