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

static char fname[2048 + sizeof LOG_DIR];

int mkdir2(const char *path)
{
#ifdef _WIN32
	if(!CreateDirectory(path, NULL) &&
			/* success = ~0 ... sigh */
			GetLastError() != ERROR_ALREADY_EXISTS)
#else
	if(mkdir(path, 0755) && errno != EEXIST)
#endif
		return 1;

	return 0;
}

int mkdir_p(char *path)
{
	char *slash;

	for(slash = strchr(path, '/');
			slash;
			slash = strchr(slash + 1, '/')){

		*slash = '\0';
		if(mkdir2(path))
			return 1;
		*slash = '/';
	}

	return 0;
}

int log_init(void)
{
	/* 2010-09-23.txt */
	struct tm *tm;
	time_t t;
	FILE *f;

	t = time(NULL);
	if(!(tm = localtime(&t)))
		return 1;

	strftime(fname, sizeof fname, LOG_DIR "/%Y-%m-%d/%H.%M.%S.txt", tm);

	if(mkdir_p(fname))
		return 1;

	f = fopen(fname, "a");
	if(f)
		fclose(f);

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
