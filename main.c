#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <errno.h>

#include "log.h"
#include "comm.h"

#define DEFAULT_PORT "1337"
#define DEFAULT_NAME "Noob"
#define LOCALE       "en_GB.UTF-8"

void cleanup(void);
void usage(char *);

static char *host = NULL, *service = NULL, *name = NULL;
static int server = 0;


int main(int argc, char **argv)
{
	int ret;

	log_append("Comm v2.0 started\n");

	if(!setlocale(LC_ALL, LOCALE)){
		if(errno == ENOENT)
			fputs("setlocale(): Locale "LOCALE" not found\n", stderr);
		else
			perror("setlocale()");
		ret = 1;
		goto cleanup;
	}

	ret = processargs(argc, argv);

	if(ret)
		goto cleanup;
	if(service == NULL && !(service = strdup(DEFAULT_PORT))){
		perror("malloc()");
		goto cleanup;
	}

	if(name == NULL && !(name = strdup(DEFAULT_NAME))){
		perror("strdup()");
		goto cleanup;
	}

	if(server)
		ret = serverloop(service, name);
	else
		ret = clientloop(host, service, name);

cleanup:
	cleanup();
	return ret;
}

int processargs(int argc, char **argv)
{
	int c, argexit = 0;
	static struct option long_options[] = {
		{"help",		0,	 0, 'h'},
		{"service", 0,	 0, 's'},
		{"name",		0,	 0, 'n'},
		{"listen",	0,	 0, 'l'},
		{0,				 0,	 0, '0'}
	};

	while( !argexit && (c = getopt_long(argc, argv, "hs:n:l", long_options, NULL)) != -1 ){
		switch(c){
			case 'h':
				usage(*argv);
			case 's':
				service = strdup(optarg);
				if(!service){
					perror("strdup()");
					return 1;
				}
				break;
			case 'n':
				name = strdup(optarg);
				if(!name){
					perror("strdup()");
					return 1;
				}
				break;
			case 'l':
				if(host)
					usage(*argv);
				server = 1;
				break;
			case '?':
				usage(*argv);
				break;
		}
	}

	if(optind == argc-1){
		host = strdup(argv[optind]);
		if(!host){
			perror("strdup()");
			return 1;
		}
	}else if(!server){
		puts("Need host");
		usage(*argv);
	}

	if(argexit)
		exit(0);
	return 0;
}

void usage(char *progname)
{
	printf("Usage: %s [options] host\n", progname);
	fputs(" -h, --help:		Display this usage\n"
			 " -l, --listen:	Host mode\n"
			 " -s, --service: Service/Port\n"
			 " -n, --name:		Username\n", stdout);
	cleanup();
	exit(0);
}

void cleanup(void)
{
	free(host);
	free(service);
	free(name);
	log_close();
}
