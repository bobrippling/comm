#ifndef LIBCOMM_H
#define LIBCOMM_H

#ifdef __cplusplus
extern "C" {
#endif

enum commstate
{
	COMM_DISCONNECTED, COMM_CONNECTING,
	COMM_VERSION_WAIT, COMM_NAME_WAIT, COMM_ACCEPTED
};

typedef struct
{
 enum commstate	state;

	struct sockaddr_in hostaddr;
	int port;
	char *name, *col;

	struct list
	{
		char *name, *col;
		struct list *next;
	} *clientlist;
	int listing;

	int sock, udpsock;
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
	COMM_TYPING,
	COMM_PRIVMSG,
	COMM_INFO,
	COMM_SERVER_INFO,
	COMM_ERR,
	COMM_RENAME,
	COMM_SELF_RENAME,
	COMM_CLIENT_CONN,
	COMM_CLIENT_DISCO,
	COMM_CLIENT_LIST,
	COMM_STATE_CHANGE, /* use comm_state() to do stuff */

	COMM_DRAW,
	COMM_DRAW_CLEAR
};

typedef void (*comm_callback)(enum comm_callbacktype, const char *, ...);

void comm_init(comm_t *);

int comm_connect(comm_t *, const char *host,
		const char *port, const char *name);

int comm_sendmessagef(comm_t *, const char *msg, ...);
int comm_sendmessage( comm_t *, const char *msg);
int comm_rename(comm_t *, const char *);
int comm_recv(comm_t *, comm_callback);
int comm_kick(comm_t *, const char *name);
int comm_su(comm_t *, const char *pass);
int comm_privmsg(comm_t *, const char *name, const char *msg);
int comm_rels(comm_t *);
int comm_colour(comm_t *, const char *col);
int comm_typing(comm_t *, int);

int  comm_draw(comm_t *, int x, int y, int last_x, int last_y, int colour);
void comm_getdrawdata(const char *, int *, int *, int *, int *);
int  comm_draw_clear(comm_t *);

void comm_close(comm_t *);

enum commstate comm_state(comm_t *);
const char    *comm_lasterr(comm_t *);
int            comm_nclients(comm_t *);

struct list   *comm_clientlist(comm_t *);
const char    *comm_getname(   comm_t *);
const char    *comm_getcolour( comm_t *, const char *name);

#ifdef __cplusplus
}
#endif

#endif
