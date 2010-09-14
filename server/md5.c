#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <crypt.h>

#include "md5.h"

static char randsaltchar(void);

static char randsaltchar(void)
{
	/*[a–zA–Z0–9./]*/
	const char *saltchars =
		"t8m/tE.YLl4Oph2szDMi0nXkTP"
		"Nrc6UqSxaweyfvVRH59KZFCdA";

	return saltchars[rand() % sizeof saltchars];
}

int md5check(const char *pass, const char *md5)
{
	char *passmd5;
	if(!md5)
		return 1;

	if(!(passmd5 = crypt(pass, md5)))
		return 1;

	return strcmp(md5, passmd5);
}

char *md5(const char *pass)
{
	static char last[22 + 16 + 4];
	char salt[] = "$1$.........", *sp;
	char *ret;

	for(sp = salt + 3; *sp; sp++)
		*sp = randsaltchar();

	if(!(ret = crypt(pass, salt))){
		perror("crypt()");
		return NULL;
	}

	strcpy(last, ret);

	return last;
}
