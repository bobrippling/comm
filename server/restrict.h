#ifndef RESTRICT_H
#define RESTRICT_H

/* zomg */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

int  restrict_addaddr(const char *);
int  restrict_hostallowed(struct addrinfo *);
void restrict_end(void);

#endif
