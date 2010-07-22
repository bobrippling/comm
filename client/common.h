#ifndef COMMON_H
#define COMMON_H

int connectedsock(char *host, int port);
int toserverf(int fd, const char *, ...);

#endif
