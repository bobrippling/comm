#include <stdio.h>
#include <stdarg.h>
#include "log.h"

#define LOG_FILENAME "LOG.txt"

static FILE *logfile = NULL;

void log_append(const char *s, ...)
{
	va_list list;

	if(!logfile)
		logfile = fopen(LOG_FILENAME, "w");

	if(!logfile)
		return;

	va_start(list, s);
	vfprintf(logfile, s, list);
	va_end(list);
}


void log_close(void)
{
	if(logfile){
		fclose(logfile);
		logfile = NULL;
	}
}

