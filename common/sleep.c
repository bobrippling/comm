#ifdef _WIN32
# include <winsock2.h>
/* WTF */
#else
# define _POSIX_C_SOURCE 199309L
# include <time.h>
# include "sleep.h"
#endif

void sleep_ms(int ms)
{
#ifdef _WIN32
	struct timeval tv;
	tv.tv_usec = ms * 1000;
	tv.tv_sec  = 0;
	select(0, NULL, NULL, NULL, &tv);
#else
	struct timespec rqtp;

	rqtp.tv_sec  = 0;
	rqtp.tv_nsec = ms * 1000000;

	nanosleep(&rqtp, NULL);
#endif
}
