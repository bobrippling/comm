#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <errno.h>
#include <setjmp.h>

#include "settings.h"
#include "ui/ui.h"

#define DEFAULT_PORT  2848
#define DEFAULT_NAME  "Timmy"
#define LOCALE        "en_GB.UTF-8"

struct settings global_settings;


static void usage(char *);
#define USAGE() do { usage(*argv); return 1; } while(0)

static void usage(char *progname)
{
	fprintf(stderr, "Usage: %s [options] host\n", progname);
	fputs(" -l: Listen/Host mode\n"
			  " -p: Port\n"
			  " -u: Username\n", stderr);
}


int main(int argc, char **argv)
{
	int ret = 0, i;
	char argv_options = 1;

	setlocale(LC_ALL, LOCALE);

	if(setjmp(allocerr)){
		ui_message(UI_ERROR, "Out of memory!");
		return 1;
	}

	global_settings.name = DEFAULT_NAME;
	global_settings.port = DEFAULT_PORT;

	for(i = 1; i < argc; i++)
		if(argv_options && argv[i][0] == '-'){
			if(strlen(argv[i]) == 2)
				switch(argv[i][1]){
					case '-':
						argv_options = 0;
						break;

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

#if 0
	TODO: windows
	if(daemon){
		puts("daemonising...");
		FreeConsole();
	}else
		puts("not daemonising");
#endif

	ui_main();

	return ret;
}
