#include <stdio.h>
#include <time.h>
#include <string.h>
#include <errno.h>

/* mkdir */
#ifdef _WIN32
# include <windows.h>
#else
# include <sys/stat.h>
# include <sys/types.h>
#endif

#include "log.h"
#include "../config.h"

static char fname[32 + sizeof LOG_DIR];

int log_init(void)
{
	/* 2010-09-23.txt */
	struct tm *tm;
	time_t t;

	t = time(NULL);
	if(!(tm = localtime(&t)))
		return 1;

	strftime(fname, sizeof fname,
			LOG_DIR "/%Y-%m-%d.txt", tm);


#ifdef _WIN32
	if(!CreateDirectory(LOG_DIR, NULL) &&
			/* success = ~0 ... sigh */
			GetLastError() != ERROR_ALREADY_EXISTS)
#else
	if(mkdir(LOG_DIR, 0755) && errno != EEXIST)
#endif
		return 1;

	{
		char buf[16];
		strftime(buf, sizeof buf,
				"New Log %H:%M", tm);

		log_add(buf);
	}

	return 0;
}

void log_add(const char *msg)
{
	FILE *f = fopen(fname, "a");
	if(f){
		fprintf(f, "%s\n", msg);
		fclose(f);
	}
}
