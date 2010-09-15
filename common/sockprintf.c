/*
 * Windows only, since non-crippled OSs
 * already have this feature
 */
#include <stdarg.h>
#include <winsock2.h>
#include <stdio.h>
#include <string.h>

#include "sockprintf.h"

#define DEBUG 1

static int _sockprint(int fd, const char *fmt, va_list l);


int sockprintf(int fd, const char *fmt, ...)
{
	va_list l;
	int ret;

	va_start(l, fmt);
	ret = sockprint(fd, fmt, l);
	va_end(l);
	return ret;
}

int sockprint(int fd, const char *fmt, va_list l)
{
	return _sockprint(fd, fmt, l);
}

static int _sockprint(int fd, const char *fmt, va_list l)
{
	int size = 64, ret, esave;
	char *p;
	va_list ap;

	if(!(p = malloc(size)))
		return -1;

	for(;;){
		int n;
		char *np;

		va_copy(ap, l);
		n = vsnprintf(p, size, fmt, l);
		va_end(ap);

		if(n > -1 && n < size)
			break;

		/* more space... */
		if(n > -1)
			size = n + 1; /* exact */
		else
			size *= 2;

		if(!(np = realloc(p, size))){
			free(p);
			return -1;
		}
		p = np; /* lol */
	}

	size = strlen(p);

	ret = send(fd, p, size, 0);
	esave = errno;
	free(p);
	errno = esave;
	return ret;
}
