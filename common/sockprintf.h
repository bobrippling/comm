#ifndef SOCKPRINTF_H
#define SOCKPRINTF_H

int sockprintf(int fd, const char *fmt, ...);
int sockprint( int fd, const char *fmt, va_list);

int sockprintf_newline(int fd, const char *fmt, ...);
int sockprint_newline( int fd, const char *fmt, va_list);

#endif
