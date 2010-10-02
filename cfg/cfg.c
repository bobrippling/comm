#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
/* alloca bs */
# include <malloc.h>
#else
# include <alloca.h>
#endif

#include "../config.h"

#include "cfg.h"

#ifdef _WIN32
# define CFG_NAME "comm.cfg"
#else
# define CFG_NAME "/.commrc"
#endif

static const char *config_path(void);

/* TODO: name, etc... in one place */
static char name[MAX_NAME_LEN];
static char port[MAX_PORT_LEN];
static char lasthost[MAX_LINE_LEN];
static char colour[MAX_COLOUR_LEN];

#ifndef _WIN32
static int write = 0;
#endif

static const char *config_path()
{
#ifdef _WIN32
	return CFG_NAME;
#else
	static char path[1024];
	char *home;

	home = getenv("HOME");
	if(!home){
		fputs("couldn't get $HOME\n", stderr);
		return NULL;
	}
	strncpy(path, home,     sizeof path);
	strncat(path, CFG_NAME, sizeof path);

	return path;
#endif
}

int config_read()
{
	const char *fname = config_path();
	FILE *f;

	*name = *port = *lasthost = *colour = '\0';

	f = fopen(fname, "r");

	if(f){
		char line[MAX_CFG_LEN];
		int n = 1;

#ifndef _WIN32
		write = 1;
#endif

		while(fgets(line, sizeof line, f)){
			char *p = strchr(line, '\n');
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
			else if(!strncmp(line, "COLOUR ", 7))
				strncpy(colour, line + 7, sizeof colour);
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
	return 1;
}

int config_write()
{
	const char *fname = config_path();
	FILE *f;

#ifndef _WIN32
	if(!write){
		puts("not writing new config");
		return 0;
	}
#endif

	f = fopen(fname, "w");
	if(f){
#define OUT(cfg, s) \
		if(*s) fprintf(f, cfg " %s\n", s)

		OUT("NAME",     name);
		OUT("PORT",     port);
		OUT("LASTHOST", lasthost);
		OUT("COLOUR",   colour);

		fclose(f);
		return 0;
	}
	return 1;
}

/* --- */

#define CFG(n) \
	void config_set##n(const char *s) { strncpy(n, s, sizeof n); } \
	const char *config_##n() { return n; }

CFG(port)
CFG(name)
CFG(lasthost)
CFG(colour)
