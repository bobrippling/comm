#ifndef LIBCOMM_H
#define LIBCOMM_H

typedef struct
{
	enum commstate
	{
		VERSION_WAIT, NAME_WAIT, ACCEPTED
	} state;

	struct sockaddr_in hostaddr;
	int port;
	const char *name;

	int sock;
	FILE *sockf;
	struct pollfd pollfds;

	const char *lasterr;
} comm_t;

enum callbacktype
{
	COMM_MSG,
	COMM_INFO,
	COMM_ERR,
	COMM_RENAME,
	COMM_CLIENT_CONN,
	COMM_CLIENT_DISCO,
	COMM_CLIENT_LIST
};

void comm_init(comm_t *);

int comm_connect(comm_t *, const char *host,
		int port, const char *name);

int comm_sendmessage(comm_t *, const char *msg, ...);
int comm_rename(comm_t *, const char *);
int comm_recv(comm_t *,
	void (callback(enum callbacktype, const char *, ...)));

void comm_close(comm_t *);

enum commstate comm_state(comm_t *);
const char    *comm_lasterr(comm_t *);

#endif
