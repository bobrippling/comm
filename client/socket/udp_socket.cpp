#include <string>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <poll.h>
#include <netdb.h>

#include "udp_socket.h"

#define DEBUG 0

#define DOMAIN AF_INET /* AF_INET6 */
#define TYPE SOCK_DGRAM
#define PROTOCOL 0 /* aka use default */

#define ERR_PREFIX          errno ? strerror(errno) :
#define ERR_INIT            ERR_PREFIX "Couldn't create socket/set non blocking"
#define ERR_CLOSE           ERR_PREFIX "Couldn't close socket"
#define ERR_SHUTDOWN        ERR_PREFIX "Couldn't shutdown socket"

#define ERR_NTOP            ERR_PREFIX "Couldn't convert IP socket address"
#define ERR_POLL            ERR_PREFIX "UDPSocket poll error"

#define ERR_SEND            ERR_PREFIX "Couldn't send data"
#define ERR_RECV            ERR_PREFIX "UDPSocket receive error"

#define ERR_COULDNT_BIND    ERR_PREFIX "Couldn't bind port"
#define ERR_COULDNT_LOOKUP  ERR_PREFIX "Couldn't lookup host"

#if DEBUG
#include <iostream>
#endif

/*
 * http://www.cs.utah.edu/dept/old/texinfo/glibc-manual-0.02/library_15.html
 */

extern "C"
{
#include "socket.h"
}

UDPSocket::UDPSocket(const char *host, int port, int bs):
	lerr(NULL), buffer(new char[bs]), buffersize(bs),
	fd(socket(DOMAIN, TYPE, PROTOCOL)), addr()
{
	if(fd == -1 || !setblocking(false))
		throw ERR_INIT;

#if defined(SO_NOSIGPIPE)
	{
		int val = 1;
		// ignore sigpipe - FreeBSD
		setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &val, sizeof(val));
	}
#endif

	if(host){
		if(!lookup(host, port, &addr))
			throw ERR_COULDNT_LOOKUP;
	}else
		memset(&addr, '\0', sizeof addr);
	// TODO bind etc etc
}

bool UDPSocket::setblocking(bool block) const
{
	int flags;

	if((flags = fcntl(fd, F_GETFL, 0)) == -1)
		flags = 0;

	if(block)
		flags &= ~O_NONBLOCK;
	else
		flags |= O_NONBLOCK;

	return fcntl(fd, F_SETFL, flags) != -1;
}

UDPSocket::~UDPSocket()
{
	close(fd);
	fd = -1;
	delete[] buffer;
}

const char *UDPSocket::remoteaddr()
{
	return addrtostr(&addr);
}

// data i/o -----------------------------------------

bool UDPSocket::senddata(const void *p, size_t siz)
{
	int ret;

	//win = sendto(fd, p, siz, 0, NULL, 0) != -1;
	//                                addr, addrlen

#if defined(MSG_NOSIGNAL) && !defined(SO_NOSIGPIPE)
	ret = sendto(fd, p, siz, MSG_NOSIGNAL /* prevent SIGPIPE etc */,
			(struct sockaddr *)&addr, sizeof addr);
#elif defined(SO_NOSIGPIPE)
	ret = sendto(fd, p, siz, 0, &addr, sizeof addr);
#else
	// TODO
	// not bsd nor linux
	// neither, nor:
	// http://krokisplace.blogspot.com/2010/02/suppressing-sigpipe-in-library.html
#error unknown OS
#endif

	if(ret == -1 || errno == EPIPE)
		// FIXME
		return false;

	return true;
}

bool UDPSocket::senddata(const std::string& data)
{
	return senddata(data.c_str());
}

bool UDPSocket::senddata(const char c)
{
	return senddata(&c, (size_t)sizeof(char));
}

bool UDPSocket::senddata(const char *data)
{
	return senddata((const void *)data, strlen(data));
}

UDPSocket& UDPSocket::operator<<(const char c)
{
	senddata(c);
	return *this;
}

UDPSocket& UDPSocket::operator<<(const char *s)
{
	senddata(s);
	return *this;
}

UDPSocket& UDPSocket::operator<<(const std::string& s)
{
	senddata(s);
	return *this;
}

const char *UDPSocket::lasterr() const
{
	return lerr;
}

bool UDPSocket::recvdata(void *p, size_t siz)
{
	int ret;
	socklen_t len = sizeof addr;

	/*
	 * restrict who we recieve from to addr
	 * TODO:
	 * allow global recieve
	 */

#if defined(MSG_NOSIGNAL) && !defined(SO_NOSIGPIPE)
	ret = recvfrom(fd, p, siz, MSG_NOSIGNAL, (struct sockaddr *)&addr, &len);
#elif defined(SO_NOSIGPIPE)
	ret = recvfrom(fd, p, siz, 0, &addr, &len);
#else
	// TODO
	// not bsd nor linux
	// neither, nor:
	// http://krokisplace.blogspot.com/2010/02/suppressing-sigpipe-in-library.html
#error unknown OS
#endif

	if(ret == -1){
		lerr = ERR_RECV;
		return false;
	}
	return true;
}

bool UDPSocket::recvdata(char *p, size_t s)
{
	return recvdata((void *)p, s);
}

bool UDPSocket::recvdata(std::string& s)
{
	if(recvdata(buffer, buffersize)){
		s = buffer;
		return true;
	}
	return false;
}

UDPSocket& UDPSocket::operator>>(std::string& s)
{
	recvdata(s);
	return *this;
}
