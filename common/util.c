#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <stdarg.h>

#include "util.h"

extern jmp_buf allocerr;


void *cmalloc(size_t len)
{
	void *p = malloc(len);
	if(!p)
		longjmp(allocerr, 1);
	return p;
}

char *cstrdup(const char *s)
{
	void *p = malloc(strlen(s) + 1);
	if(!p)
		longjmp(allocerr, 1);
	strcpy(p, s);
	return p;
}

void perrorf(const char *fmt, ...)
{
	if(fmt){
		va_list l;
		va_start(l, fmt);
		vfprintf(stderr, fmt, l);
		va_end(l);
	}
	perror(NULL);
}
