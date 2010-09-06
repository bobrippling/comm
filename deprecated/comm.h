#ifndef COMM_H
#define COMM_H

enum commstate
{
	VERSION_WAIT, NAME_WAIT, ACCEPTED
};

enum commstate comm_getstate(void);

int  comm_sendmessage(const char *msg, ...);
int  comm_listclients(void);

int comm_main(const char *name, const char *host, int port);

#define BIT(a, b) (((a) & (b)) == (b))

#endif
