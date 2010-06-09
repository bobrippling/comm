#include <stdio.h>
#ifdef _WIN32
#include <winsock.h>
#else
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <errno.h>
#include <string.h>
#include <stdlib.h>


#include "protocol.h"
#include "comm.h"
#include "socket.h"
#include "line.h"

#define BUFFER_SIZE 1024
#define SECOND_WAIT 0
#define USEC_WAIT   1000 * 100
/* aka 100ms */

static int mainloop(int, char *);

static int server; /* bool */


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
