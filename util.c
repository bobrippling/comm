#include <stdlib.h>
#include <setjmp.h>
#include <string.h>

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
