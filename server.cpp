#include <vector>

#include "util.h"
#include "socket/tcp_socket.h"


static vector<TCPSocket *> clients;

TCPSocket& server_connreq(TCPSocket&)
void       client_conn(   TCPSocket&);
void       client_disco(  TCPSocket&);
void       client_recv(   TCPSocket&);


int server_main()
{
	TCPSocket listen(&connreq, NULL, NULL, NULL);

	if(!listen.listen(global_settings.port)){
		fprintf(stderr, "Couldn't listen: %s\n", listen.lasterr());
		return 1;
	}

	for(;;){
		listen.runevents();

		mssleep(200);
	}
}

TCPSocket& server_connreq(TCPSocket& s)
{
	TCPSocket *client = new TCPSocket(NULL, &client_conn, &client_disco, &client_recv);

	// assert(s == &listen)

	clients.push_back(client);

	return *client;
}

void client_conn(TCPSocket& s)
{
}

void client_disco(TCPSocket& s)
{
}

void client_recv(TCPSocket& s)
{
}
