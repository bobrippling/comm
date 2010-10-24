#ifndef FTP_H
#define FTP_H

struct filetransfer
{
	int lasterr, lasterr_isnet;
	int sock;
	const char *fname;
};

typedef int (*ft_callback)(struct filetransfer *, size_t bytessent, size_t bytestotal);

int ft_connect(struct filetransfer *, const char *host, const char *port);
int ft_listen( struct filetransfer *, int port);
int ft_accept( struct filetransfer *);
int ft_close(  struct filetransfer *);

int ft_recv(       struct filetransfer *, ft_callback callback);
int ft_send(    struct filetransfer *, const char *fname, ft_callback callback);

#define ft_fname(ft) (ft->fname)

/*
 * all functions return 0 on success
 *
 * if, during ft_send, callback returns non-zero, the transfer
 * is cancelled
 */

#endif
