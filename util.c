#include <poll.h>

#include "util.h"

void mssleep(int ms)
{
	poll(NULL, 0, ms);
}
