#include <stdio.h>
#include <stdlib.h>
#include <alloca.h>
#include <string.h>
#include <errno.h>

#include "../config.h"

#include "cfg.h"

#define CFG_NAME "/.commrc"

/* TODO: name, etc... in one place */
static char name[MAX_NAME_LEN];
static char port[MAX_PORT_LEN];
static char lasthost[MAX_LINE_LEN];

static int write = 0;

/* TODO: config_write() */

int config_read()
{
#ifndef _WIN32
	char *home = getenv("HOME");

	*name = *port = '\0';

	if(home){
		char *fname = alloca(strlen(home) + strlen(CFG_NAME) + 1);
		FILE *f;

		strcpy(fname, home);
		strcat(fname, CFG_NAME);

		f = fopen(fname, "r");

		if(f){
			char line[MAX_NAME_LEN + 8];
			int n = 1;

			write = 1;

			while(fgets(line, sizeof line, f)){
				char *p = strchr(line, '#');
				if(p)
					*p = '\0';
				p = strchr(line, '\n');
				if(p)
					*p = '\0';
				if(!*line)
					continue;

				if(!strncmp(line, "NAME ", 5))
					strncpy(name, line + 5, sizeof name);
				else if(!strncmp(line, "PORT ", 5))
					strncpy(port, line + 5, sizeof port);
				else if(!strncmp(line, "LASTHOST ", 9))
					strncpy(lasthost, line + 9, sizeof lasthost);
				else{
					fprintf(stderr, "Invalid config line: %d\n", n);
					goto bail;
				}
				n++;
			}

			if(ferror(f))
				perror("fgets()");
			else{
				fclose(f);
				return 0;
			}

bail:
			fclose(f);
		}else if(errno == ENOENT)
			return 0;
		else
			perror(CFG_NAME);

	}else{
		fputs("couldn't get $HOME\n", stderr);
	}
#else
	TODO
#endif
	return 1;
}

int config_write()
{
	char *home = getenv("HOME");

	if(home){
		char *fname = alloca(strlen(home) + strlen(CFG_NAME) + 1);
		FILE *f;

		strcpy(fname, home);
		strcat(fname, CFG_NAME);

		f = fopen(fname, "w");
		if(f){
#define OUT(cfg, s) \
			if(*s) fprintf(f, cfg " %s\n", s)

			OUT("NAME",     name);
			OUT("PORT",     port);
			OUT("LASTHOST", lasthost);

			fclose(f);
			return 0;
		}
	}
	return 1;
}

/* --- */

void config_setname(const char *n)
{
	strncpy(name, n, sizeof name);
}

void config_setport(const char *p)
{
	strncpy(port, p, sizeof port);
}

void config_setlasthost(const char *h)
{
	strncpy(lasthost, h, sizeof lasthost);
}

const char *config_port()
{
	return port;
}

const char *config_name()
{
	return name;
}

const char *config_lasthost()
{
	return lasthost;
}
