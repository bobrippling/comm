#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>

#include "libft/ft.h"

#define PATH_LEN     128
#define MAX_LINE_LEN 4096


enum { RESUME, RENAME } clobber_mode = RENAME;


char *dir = NULL;
char  file_cmd[PATH_LEN] = { 0 };
char  file_log[PATH_LEN] = { 0 };

int fd_cmd = -1;

struct filetransfer ft;


void term_files()
{
	/* only close + remove the fifos */
	close(fd_cmd);
	remove(file_cmd);
}

int init_files()
{
	if(mkdir(dir, 0700) && errno != EEXIST){
		perror("mkdir()");
		return 1;
	}

	snprintf(file_cmd,     PATH_LEN, "%s/%s", dir, "cmd");
	snprintf(file_log,  PATH_LEN, "%s/%s", dir, "out");


	if(mkfifo(file_cmd, 0600) == -1 && errno != EEXIST){
		fprintf(stderr, "tftd: mkfifo(): %s: %s\n",
				file_cmd, strerror(errno));
		return 1;
	}

	if((fd_cmd = open(file_cmd, O_RDONLY | O_NONBLOCK, 0600)) == -1){
		fprintf(stderr, "tftd: open(): %s: %s\n", file_cmd, strerror(errno));
		remove(file_cmd);
		return 1;
	}

	return 0;
}


int callback(struct filetransfer *ft, enum ftstate state,
		size_t bytessent, size_t bytestotal)
{
}

int queryback(struct filetransfer *ft, enum ftquery qtype, const char *msg, ...)
{
	if(qtype == FT_FILE_EXISTS)
		switch(clobber_mode){
			case RESUME:    return 1;
			case RENAME:    return 2;
		}
}

char *fnameback(struct filetransfer *ft, char *fname)
{
}



int lewp()
{
	struct pollfd pfd[2];

	pfd[0].fd     = ft_get_fd(&ft);
	pfd[1].fd     = fd_cmd;
	pfd[0].events = pfd[1].events = POLLIN;

	for(;;){
		switch(poll(pfd, 2, -1)){
			case 0:
				/* notan happan */
				continue;
			case -1:
				fprintf(stderr, "tftd: poll: %s\n", strerror(errno));
				return 1;
		}

#define BIT(x, b) (((x) & (b)) == (b))
		if(BIT(pfd[0].revents, POLLIN)){
			if(ft_handle(&ft, callback, queryback, fnameback))
				fprintf(stderr, "ft_recv(): %s\n", ft_lasterr(&ft));
		}

		if(BIT(pfd[1].revents, POLLIN)){
			int nread, saverrno;
			char buffer[MAX_LINE_LEN], *nl;

			nread = read(fd_cmd, buffer, MAX_LINE_LEN);
			saverrno = errno;

			switch(nread){
				case -1:
					fprintf(stderr, "tftd: read(): %s\n", strerror(errno));
					return 1;
				case 0:
					break;
				default:
					if((nl = memchr(buffer, '\n', nread)))
						*nl = '\0';
			}
			if(nread > 0){
				if(ft_send(&ft, callback, buffer))
					fprintf(stderr, "tftd: ft_send(): %s\n", ft_lasterr(&ft));
			}
		}
	}
}

int main(int argc, char **argv)
{
	char *port = NULL, *host = NULL;
	int i;

	/*
	 * TODO: chdir $DOWNLOAD_DIR
	 * while true; tft -R -u -l < $pipe
	 */

#define ARG(s) !strcmp(argv[i], "-" s)

	for(i = 1; i < argc; i++)
		if(ARG("p"))
			if(++i < argc)
				port = argv[i];
			else
				goto usage;
		else if(ARG("n"))
			clobber_mode = RENAME;
		else if(ARG("r"))
			clobber_mode = RESUME;
		else if(!dir)
			dir = argv[i];
		else if(!host)
			host = argv[i];
		else{
		usage:
			fprintf(stderr,
					"Usage: %s [-p port] [-n] [-r] path/to/base/dir [host]\n"
					"  -p: Port to listen on (excludes host)\n"
					"  -n: Rename file if existing\n"
					"  -r: Resume transfer if existing\n"
					, argv[i]);
			return 1;
		}

	if(!dir)
		goto usage;

	if(init_files())
		return 1;

	if(host){
		if(ft_connect(&ft, host, port)){
			fprintf(stderr, "tftd: ft_connect: %s\n", ft_lasterr(&ft));
			return 1;
		}
	}else if(ft_listen(&ft, atoi(port))){
		fprintf(stderr, "tftd: ft_listen: %s\n", ft_lasterr(&ft));
		return 1;
	}

	{
		int ret = lewp();
		ft_close(&ft);
		return ret;
	}
}
