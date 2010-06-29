#include <stdlib.h>
#include <setjmp.h>

#include "util.h"

extern jmp_buf allocerr;


void *cmalloc(size_t len)
{
	void *p = malloc(len);
	if(!p)
		longjmp(allocerr, 1);
	return p;
}
