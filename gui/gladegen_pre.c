#include <stdio.h>

#include "gladegen.h"

#define FILENAME "glade.xml"

void gladegen_term(void)
{
	remove(FILENAME);
}

int gladegen_init(void)
{
	FILE *f = fopen(FILENAME, "wb");
	int ret = 0;

	if(!f){
		perror("open: "FILENAME);
		return 1;
	}

