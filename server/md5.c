#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <crypt.h>

#include "md5.h"
#include "cfg.h"
#include "../config.h"

static char randsaltchar(void);

extern char glob_pass[MAX_PASS_LEN];

static char randsaltchar(void)
{
	/*[a–zA–Z0–9./]*/
	const char *saltchars =
		"t8m/tE.YLl4Oph2szDMi0nXkTP"
		"Nrc6UqSxaweyfvVRH59KZFCdA";

	return saltchars[rand() % sizeof saltchars];
}

int md5check(const char *pass)
{
	char *passmd5;
	int ret;

	if(!pass)
		return 1;

	if(!(passmd5 = crypt(glob_pass, pass)))
		return 1;

	ret = strcmp(passmd5, glob_pass);
	return ret;
}

int md5(const char *pass)
{
	char salt[] = "$1$.........", *sp;
	char *ret;

	for(sp = salt + 3; *sp; sp++)
		*sp = randsaltchar();

	/*
	 * do _not_ free ret:
	 * it's allocated by crypt()
	 * but kept a hold of by libcrypt
	 * for other crypt() calls
	 */
	if(!(ret = crypt(pass, salt))){
		perror("crypt()");
		return 1;
	}

	strncpy(glob_pass, ret, sizeof glob_pass);
	return 0;
}
