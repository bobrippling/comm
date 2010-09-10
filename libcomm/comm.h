#ifndef LIBCOMM_H
#define LIBCOMM_H

typedef struct
{
	enum commstate
	{
		COMM_DISCONNECTED, CONN_CONNECTING,
		COMM_VERSION_WAIT, COMM_NAME_WAIT, COMM_ACCEPTED
	} state;

	struct sockaddr_in hostaddr;
	int port;
	char *name;

	int sock;
#ifndef _WIN32
	FILE *sockf;
	/*
	 * otherwise,
	 * crippled toy OSs, such as Windows, don't have
	 * fdopen, hence I must code my own shitty
	 * emulation for printf()'ing to a socket
	 * (see sockprintf)
	 */
#endif

	const char *lasterr;
} comm_t;

enum comm_callbacktype
{
	COMM_MSG,
	COMM_INFO,
	COMM_ERR,
	COMM_RENAME,
	COMM_CLIENT_CONN,
	COMM_CLIENT_DISCO,
	COMM_CLIENT_LIST
};

typedef void (*comm_callback)(enum comm_callbacktype, const char *, ...);

void comm_init(comm_t *);

int comm_connect(comm_t *, const char *host,
		int port, const char *name);

int comm_sendmessage(comm_t *, const char *msg, ...);
int comm_rename(comm_t *, const char *);
int comm_recv(comm_t *, comm_callback);

void comm_close(comm_t *);

enum commstate comm_state(comm_t *);
const char    *comm_lasterr(comm_t *);

#endif
