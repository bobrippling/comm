#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "log.h"
#include "protocol.h"

/**
 * FIXME: change to have a data separator, like \0 ?
 * Message Format
 *
 * ${NAME};${SOCKET_DATA}
 */

#define SOCKET_DATA_SENTINEL ";"

enum
{
	PROTOCOL_MESSAGE = 'M'
};


void processdata(SOCKET_DATA *msg)
{
	switch(msg->data[0]){
		case PROTOCOL_MESSAGE:
			/* message */
			processmessage(msg->data + 1);

		default:
			log_append("Unrecognised message: \"%s\"\n", msg->data);
	}
}

int senddata(int socket, SOCKET_DATA *msg)
{
	int strl = strlen(msg->data);
	if(write(socket, msg->data, strl) != strl)
		return 1;
	return 0;
}


void processmessage(char *msg)
{
	char *name = msg, *message;

	message = strchr(msg, *SOCKET_DATA_SENTINEL);
	if(!message){
		log_append("Message received with no sentinel: \"%s\"\n", msg);
		return;
	}

	*message++ = '\0';

	/* handle message */
	printf("%s: %s\n", name, message);
}

SOCKET_DATA *createmessage(char *name, char *message)
{
	SOCKET_DATA *msg = malloc(sizeof(SOCKET_DATA));
	char *s = malloc(strlen(name) + strlen(message) + 3);
	if(!s || !msg){
		free(s);
		free(msg);
		return NULL;
	}


	s[0] = PROTOCOL_MESSAGE;
	s[1] = '\0';
	strcat(s, name);
	strcat(s, SOCKET_DATA_SENTINEL);
	strcat(s, message);

	msg->data = s;

	return msg;
}

void freemessage(SOCKET_DATA *msg)
{
	free(msg->data);
	free(msg);
}

