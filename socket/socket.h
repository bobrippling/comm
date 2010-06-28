#ifndef SOCKET_H
#define SOCKET_H

int lookup(const char *, int, struct sockaddr_in *);
const char *addrtostr(struct sockaddr_in *);

#endif
