/*
 * Windows only, since non-crippled OSs
 * already have this feature
 */
#include <stdarg.h>
#include <winsock2.h>
#include <stdio.h>
#include <string.h>

#include "sockprintf.h"

#define DEBUG 0

static int _sockprint(int fd, const char *fmt, va_list l, int newline);

int sockprintf(int fd, const char *fmt, ...)
{
	va_list l;
	int ret;

	va_start(l, fmt);
	ret = sockprint(fd, fmt, l);
	va_end(l);
	return ret;
}

int sockprintf_newline(int fd, const char *fmt, ...)
{
	va_list l;
	int ret;

	va_start(l, fmt);
	ret = sockprint_newline(fd, fmt, l);
	va_end(l);
	return ret;
}

int sockprint(int fd, const char *fmt, va_list l)
{
	return _sockprint(fd, fmt, l, 0);
}

int sockprint_newline(int fd, const char *fmt, va_list l)
{
	return _sockprint(fd, fmt, l, 1);
}

static int _sockprint(int fd, const char *fmt, va_list l, int newline)
{
	int buflen = newline /* assert 1 or 0 */, ret;
	char *buf;
	const char *fmtchar, *arg;
	va_list cpy;

	va_copy(cpy, l); /* save for later */

	for(fmtchar = fmt; *fmtchar; fmtchar++)
		if(*fmtchar == '%'){
			if(*++fmtchar == '%'){
				buflen++; /* just count the single % */
				continue;
			}

			arg = va_arg(l, const char *);
			switch(*fmtchar){
#define FMT_LEN(c, n) case c: buflen += n; break
				FMT_LEN('d', 20);
				FMT_LEN('c', 3);
				case 's':
					buflen += strlen((const char *)arg);
					break;
				default:
					buflen += 10; /* eh... */
					fprintf(stderr, "libcomm (sockprintf): "
							"comm_sendmessage error: format '%c'\n",
							*fmtchar);
			}
#undef FMT_LEN
		}else
			buflen++;

	buf = malloc(buflen);
	if(!buf)
		return 1;

	if(vsnprintf(buf, buflen, fmt, cpy) < 0){
		va_end(cpy);
		return 1;
	}
	va_end(cpy);

	if(newline)
		strcat(buf, "\n");

#if DEBUG
	fprintf(stderr, "libcomm (sockprintf): sending \"%s\"\n", buf);
#endif
	ret = send(fd, buf, buflen, 0);

	free(buf);

	return ret; /* #bytes or -1+error */
}
