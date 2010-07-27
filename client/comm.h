#ifndef COMM_H
#define COMM_H

int comm_sendmessage(const char *msg, ...);
enum commstate { VERSION_WAIT, NAME_WAIT, ACCEPTED };
enum commstate comm_getstate(void);
int comm_main(const char *name, const char *host, int port);

#endif
