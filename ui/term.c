#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
/* window size */
#include <sys/types.h>
#include <sys/ioctl.h>


#define escape_clrtoeol() escape_print("K")
void term_setoriginal(void);
void term_setcustom(void);

int ncols(void);
int nrows(void);

void escape_print(const char *);
void escape_movex(signed char);


/*
 * http://en.wikibooks.org/wiki/Serial_Programming/termios
 *
 * ECHO	  - echo chars
 * ICANON	- enable erase, kill, werase and rprint special chars (^W?)
 * ECHONL	- echo newline
 * ISIG	  - enable interrupt, quit and suspend
 * HUP	   - send SIGHUP when last process closes tty
 * TOSTOP	- send SIGSTP when bg process tries to write
 * EOF char  - char sends EOF
 *
 * COOKED	- not raw
 */

/*
#define FLAGS  ISIG	|\
			  ~ECHO	|\
			  ~ECHONL  |\
			   TOSTOP  |\
			  ~ECHOE   |\
			  ~ECHOK   |\
			  ~IEXTEN  |\
			  ~ECHOCTL |\
			  ~ECHOPRT
*/

#define TERM_ATTR(x) tcsetattr(STDIN_FILENO, TCSANOW, &x)

#include "term.h"

static void singlechar(int, struct termios *);
static void echo(int, struct termios *);

static struct termios origattr, customattr;
static const char terminal_escape[] = { 0x1B, 0x5B, 0x0 }; /* aka something[ */


void initterm()
{
	if(tcgetattr(STDIN_FILENO, &customattr)){
		perror("tcgetattr()");
		return;
	}

	memcpy(&origattr, &customattr, sizeof(struct termios));

	/*
	t.c_lflag &= FLAGS;

	t.c_cc[VMIN]  = 1;
	t.c_cc[VTIME] = 0;
	*/

	singlechar(1, &customattr);
	echo(0, &customattr);

	if(TERM_ATTR(customattr))
		perror("tcsetattr()");
}

void term_setoriginal()
{
	TERM_ATTR(origattr);
}

void term_setcustom()
{
	TERM_ATTR(customattr);
}

/*
static void printflags(struct termios *t)
{
#define PRINT_FLAG(x) if( (x & t->c_lflag) == x )\
						  puts(#x);

	PRINT_FLAG(ISIG);
	PRINT_FLAG(~ECHO);
	PRINT_FLAG(~ECHONL);
	PRINT_FLAG( TOSTOP);
	PRINT_FLAG(~ECHOE);
	PRINT_FLAG(~ECHOK);
	PRINT_FLAG(~IEXTEN);
}
*/

static void singlechar(int i, struct termios *t)
{
	if(i){
		t->c_cc[VMIN]  = sizeof(char); /* minumum number of bytes to be read before returning from read() */
		t->c_lflag &= ~ICANON;
	}else{
		t->c_cc[VMIN] = 0;
		t->c_lflag |= ICANON;
	}
}


static void echo(int i, struct termios *t)
{
	if(i)
		t->c_lflag |= ECHO;
	else
		t->c_lflag &= ~ECHO;
}

void escape_print(const char *esc)
{
	fputs(terminal_escape, stdout);
	fputs(esc, stdout);
}

void escape_movex(signed char i)
{
	static char cmd[5]; /* largest a signed char can be is 255 - 3. +1 for the letter, +1 for null */

	/* A - up, B - down, C - right, D - left */
	if(i > 0)
		sprintf(cmd, "%dC", i);
	else
		sprintf(cmd, "%dD", -i);

	escape_print(cmd);
}

int ncols()
{
	struct winsize ws;
	ioctl(1, TIOCGWINSZ, &ws);
	return ws.ws_col;
}

int nrows()
{
	struct winsize ws;
	ioctl(1, TIOCGWINSZ, &ws);
	return ws.ws_row;
}


int clientloop(char *host, char *service, char *name)
{
	int fd = tryconnect(host, service), ret;

	if(!fd){
		fprintf(stderr, "Couldn't connect to %s:%s: ", host, service);
		perror(NULL);
		return 1;
	}

	printf("Connected to %s:%s\n", host, service);

	server = 0;
	ret = mainloop(fd, name);
	destroysocket(fd);
	return ret;
}


int serverloop(char *service, char *name)
{
	int fd = trylisten(service), ret, cfd;
	struct sockaddr_in clientaddr; /* inet */
#ifdef _WIN32
	int
#else
	socklen_t
#endif
		addrlen = sizeof(clientaddr);

	if(!fd){
		fprintf(stderr, "Couldn't listen on %s: ", service);
		perror(NULL);
		return 1;
	}

	printf("Listening on %s...\n", service);

	cfd = accept(fd, (struct sockaddr *)&clientaddr, &addrlen);
	if(cfd == -1){
		perror("accept()");
		destroysocket(fd);
		return 1;
	}

	printf("Got connection from %s\n", inet_ntoa(clientaddr.sin_addr));

	server = 1;
	ret = mainloop(cfd, name);
	destroysocket(fd);
	destroysocket(cfd);
	return ret;
}

int mainloop(int fd, char *name)
{
	char *line = NULL;
	fd_set fdset;
	int linesize = 0, printprompt = 1;

	setbuf(stdout, NULL); /* unbuffered */

	do{
		struct timeval tv = { SECOND_WAIT, USEC_WAIT };

		FD_ZERO(&fdset);
		FD_SET(fileno(stdin), &fdset);
		FD_SET(fd, &fdset);

		if(printprompt){
			printprompt = 0;
			fputs("> ", stdout);
		}

		switch(select(fd + 1, &fdset, NULL, NULL, &tv)){
			case -1:
				/* error */
				if(errno == EINTR)
					/* interrupt */
					continue;
				perror("select()");
				goto cleanup;

			case 0:
				/* zero bytes read - timer stuff goes here */
				break;

			default:
				if(FD_ISSET(fileno(stdin), &fdset)){
					SOCKET_DATA *msg;

					if(rline(&line, &linesize) == 2){
						/* mem err */
						perror("malloc()");
						goto cleanup;
					}

					if(*line == EOF){
						putchar('\n');
						goto cleanup;
					}else if(!strcmp("exit", line))
						goto cleanup;

					if(!(msg = createmessage(name, line))){
						perror("malloc()");
						goto cleanup;
					}

					if(senddata(fd, msg)){
						perror("senddata()");
						goto cleanup;
					}
					freemessage(msg);
					printprompt = 1;
				}else if(FD_ISSET(fd, &fdset)){
					char buf[BUFFER_SIZE];
					SOCKET_DATA msg;
					struct sockaddr addrfrom;
#ifdef _WIN32
					int
#else
					/* can't be const */socklen_t
#endif
						 slen = sizeof(addrfrom);

					int readlen = recvfrom(fd, buf, BUFFER_SIZE-1, 0, &addrfrom, &slen); /* FIXME: change to read() */

					switch(readlen){
						case -1:
							if(errno == EINTR)
								continue;
							perror("recvfrom()");
							goto cleanup;

						case 0:
							/* done, clean up */
							puts(server ? "Client disconnect\n" : "Server disconnect\n");
							goto cleanup;

						default:
							buf[readlen] = '\0';
							msg.data = buf;
							processdata(&msg);
					}
				}
		}
	}while(1);

cleanup:
	free(line);
	return 0;
}
