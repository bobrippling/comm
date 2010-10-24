#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libft/ft.h"

#define PORT "7643"

void cleanup(void);

struct filetransfer ft;
char *recvfile = NULL;

int callback(struct filetransfer *ft, enum ftstate state,
		size_t bytessent, size_t bytestotal)
{
	if(state == FT_END && !(recvfile = strdup(ft_fname(ft))))
		perror("strdup()");

	printf("\"%s\": %zd / %zd (%2.2f)%%\r", ft_fname(ft),
			bytessent, bytestotal, (float)(100.0f * bytessent / bytestotal));
	return 0;
}

void cleanup()
{
	if(ft_connected(&ft))
		ft_close(&ft);
}

int main(int argc, char **argv)
{
	int i, listen = 0, verbose = 0;
	const char *fname = NULL, *host  = NULL, *port = PORT;

	for(i = 1; i < argc; i++)
		if(!strcmp(argv[i], "-l"))
			listen = 1;
		else if(!strcmp(argv[i], "-p"))
			if(++i < argc)
				port = argv[i];
			else
				goto usage;
		else if(!strcmp(argv[i], "-v"))
			verbose = 1;
		else if(!host)
			host = argv[i];
		else if(!fname)
			fname = argv[i];
		else{
		usage:
			fprintf(stderr, "Usage: %s [-p port] -l   [file]\n"
					            "       %s [-p port] host [file]\n",
					            *argv, *argv);
			return 1;
		}

	atexit(cleanup);

	if(listen){
		int iport;

		if(host && fname) /* no need to check file */
			goto usage;
		else if(host){
			fname = host;
			host = NULL;
			printf("%s: serving %s...\n", *argv, fname);
		}else if(verbose)
			printf("%s: listening for incomming files...", *argv);

		if(sscanf(port, "%d", &iport) != 1){
			fprintf(stderr, "need numeric port (\"%s\")\n", port);
			return 1;
		}

		if(ft_listen(&ft, iport)){
			fprintf(stderr, "ft_listen(%d): %s\n", iport, ft_lasterr(&ft));
			return 1;
		}

		if(ft_accept(&ft, 0)){
			fprintf(stderr, "ft_accept(): %s\n", ft_lasterr(&ft));
			return 1;
		}else if(!ft_connected(&ft)){
			/* shouldn't get here - since it blocks until a connection is made */
			fprintf(stderr, "no incomming connections... :S\n");
			return 1;
		}
	}else{
		if(!host)
			goto usage;

		if(ft_connect(&ft, host, port)){
			fprintf(stderr, "ft_connect(\"%s\", \"%s\"): %s\n", host, port,
					ft_lasterr(&ft));
			return 1;
		}
	}


	if(fname){
		if(verbose)
			printf("%s: sending %s\n", *argv, fname);

		if(ft_send(&ft, callback, fname)){
			fprintf(stderr, "ft_send(): %s\n", ft_lasterr(&ft));
			return 1;
		}
	}else{
		if(verbose)
			printf("%s: receiving incomming file\n", *argv);

		if(ft_recv(&ft, callback)){
			fprintf(stderr, "ft_recv(): %s\n", ft_lasterr(&ft));
			return 1;
		}
	}

	ft_close(&ft);

	return 0;
}
