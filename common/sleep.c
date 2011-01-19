#define _POSIX_C_SOURCE 199309L
#include <time.h>
#include "sleep.h"

void sleep_ms(int ms)
{
#ifdef _WIN32
	todo();
#else
	struct timespec rqtp;

	rqtp.tv_sec  = 0;
	rqtp.tv_nsec = ms * 1000000;

	nanosleep(&rqtp, NULL);
#endif
}
