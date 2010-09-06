#ifndef RESOLV_H
#define RESOLV_H

int lookup(const char *, int, struct sockaddr_in *);
const char *lookup_strerror(void);
const char *addrtostr(struct sockaddr_in *);

#endif
