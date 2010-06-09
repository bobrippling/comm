#ifndef PROTOCOL_H
#define PROTOCOL_H

typedef struct { char *data; } SOCKET_DATA;

void processdata(SOCKET_DATA *);
int  senddata(int, SOCKET_DATA *);

SOCKET_DATA *createmessage(char *, char *);
void processmessage(char *);
void freemessage(SOCKET_DATA *);

#endif
