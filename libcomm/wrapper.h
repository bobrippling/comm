#ifndef WRAPPER_H
#define WRAPPER_H

int connectedsock(const char *host, int port);
int toserver(FILE *f, const char *fmt, va_list l);
int toserverf(FILE *f, const char *, ...);

void perrorf(const char *fmt, ...);

#endif
