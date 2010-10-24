#ifndef FTP_H
#define FTP_H

struct filetransfer
{
	int lasterr, lasterr_isnet;
	int sock, connected;
	const char *fname;
};

enum ftstate
{
	FT_BEGIN, FT_TRANSFER, FT_END
};

typedef int (*ft_callback)(struct filetransfer *, enum ftstate state,
		size_t bytessent, size_t bytestotal);

int ft_connect(struct filetransfer *, const char *host, const char *port);
int ft_listen( struct filetransfer *, int port);
int ft_accept( struct filetransfer *, int block);
int ft_close(  struct filetransfer *);

int ft_recv(   struct filetransfer *, ft_callback callback);
int ft_send(   struct filetransfer *, ft_callback callback, const char *fname);

const char *ft_lasterr(struct filetransfer *);

#define ft_fname(ft)     ((ft)->fname)
#define ft_connected(ft) ((ft)->connected)

/*
 * all functions return 0 on success
 *
 * if, during ft_send, callback returns non-zero, the transfer
 * is cancelled
 */

#endif
