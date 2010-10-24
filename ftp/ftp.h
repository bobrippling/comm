#ifndef FTP_H
#define FTP_H

struct filetransfer
{
	int connected;
	int sock;

	const char *fname;
	size_t fsize;
};

typedef int (*ft_callback)(struct filetransfer *, size_t bytessent, size_t bytestotal);

int  ft_connect(struct filetransfer *, const char *host, const char *port);
int  ft_listen( struct filetransfer *, int port);
int  ft_accept( struct filetransfer *);
void ft_close(  struct filetransfer *);

int ft_recv(       struct filetransfer *, ft_callback callback);
int ft_send(    struct filetransfer *, const char *fname, ft_callback callback);


/*
 * all functions return 0 on success
 *
 * if, during ft_send, callback returns non-zero, the transfer
 * is cancelled
 */

#endif
