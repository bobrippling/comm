#ifndef SOCKET_H
#define SOCKET_H

int createsocket(void);
int destroysocket(int);

#ifndef _WIN32
int trylisten(char *);
int tryconnect(char *, char *);
#endif

#endif
