#include <stdio.h>
#include <time.h>
#include <string.h>
#include <errno.h>

/* mkdir */
#include <sys/stat.h>
#include <sys/types.h>

#include "log.h"
#include "../config.h"

static FILE *log = NULL;

int log_init(void)
{
	/* 2010-09-23.txt */
	struct tm *tm;
	time_t t;
	char fname[sizeof(LOG_DIR) + 32];

	t = time(NULL);
	if(!(tm = localtime(&t)))
		return 1;

	strftime(fname, sizeof fname,
			LOG_DIR "/%Y-%m-%d.txt", tm);

	if(mkdir(LOG_DIR, 0740) && errno != EEXIST)
		return 1;

	log = fopen(fname, "a");
	if(!log)
		return 1;

	memset(fname, '\0', sizeof fname);
	strftime(fname, sizeof fname,
			"New Log %H:%M\n", tm);

	fputs(fname, log);

	return 0;
}

void log_add(const char *msg)
{
	if(log)
		fprintf(log, "%s\n", msg);
}

void log_term(void)
{
	if(log){
		fclose(log);
		log = NULL;
	}
}
