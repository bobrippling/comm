#ifndef FTP_H
#define FTP_H

struct filetransfer
{
	const char *lasterr, *fname;
	int sock, connected;
	size_t lastcallback;
	/*struct sockaddr_in addr;*/
	char addr[32]; /* sizeof(sockaddr_in) == 16 */
};

enum ftstate
{
	FT_WAIT,
	FT_BEGIN_SEND, FT_BEGIN_RECV,
	FT_RECIEVING, FT_SENDING,
	FT_SENT, FT_RECIEVED
};

enum ftret
{
	FT_YES, FT_NO, FT_ERR
};

typedef int (*ft_callback)(struct filetransfer *, enum ftstate state,
		size_t bytessent, size_t bytestotal);
typedef int (*ft_queryback)(struct filetransfer *, const char *msg, ...);
typedef char *(*ft_fnameback)(struct filetransfer *, char *);

void ft_zero(struct filetransfer *);

int ft_connect(struct filetransfer *, const char *host, const char *port);
int ft_listen( struct filetransfer *, int port);
int ft_close(  struct filetransfer *);

enum ftret ft_accept(   struct filetransfer *, int block);
enum ftret ft_poll_recv(struct filetransfer *);

int ft_recv(     struct filetransfer *, ft_callback callback, ft_queryback, ft_fnameback);
int ft_send(     struct filetransfer *, ft_callback callback, const char *fname);

const char *ft_lasterr(   struct filetransfer *);
const char *ft_basename(  struct filetransfer *);
const char *ft_remoteaddr(struct filetransfer *);

#define ft_fname(ft)     ((ft)->fname)
#define ft_connected(ft) ((ft)->connected)
#define ft_haderror(ft)  (!!(ft)->lasterr)


#define FT_DEFAULT_PORT "7643"

/*
 * all functions return 0 on success
 *
 * if, during ft_send, callback returns non-zero, the transfer
 * is cancelled
 */

#endif
