#ifndef WRAPPER_H
#define WRAPPER_H

int connectedsock(const char *host, int port);
int toserver(FILE *f, const char *fmt, va_list l);
int toserverf(FILE *f, const char *, ...);

/* returns 1 if buffer unfilled, 0 if full */
int recv_newline(char *in, int recvret, int fd);

#endif
