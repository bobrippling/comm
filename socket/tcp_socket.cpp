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

#include "tcp_socket.h"

#define DEBUG 0

#define DOMAIN AF_INET /* AF_INET6 */
#define TYPE SOCK_STREAM
#define PROTOCOL 0 /* aka use default */
#define LISTEN_BACKLOG 10

#define ERR_PREFIX          errno ? strerror(errno) :
#define ERR_INIT            ERR_PREFIX "Couldn't create socket/set non blocking"
#define ERR_CLOSE           ERR_PREFIX "Couldn't close socket"
#define ERR_SHUTDOWN        ERR_PREFIX "Couldn't shutdown socket"

#define ERR_NTOP            ERR_PREFIX "Couldn't convert IP socket address"
#define ERR_POLL            ERR_PREFIX "TCPSocket poll error"

#define ERR_SEND            ERR_PREFIX "Couldn't send data"
#define ERR_RECV            ERR_PREFIX "TCPSocket receive error"

#define ERR_NOT_CONNECTED   ERR_PREFIX "Not connected"
#define ERR_NOT_IDLE        ERR_PREFIX "Not in idle state"

#define ERR_TIMED_OUT       ERR_PREFIX "Timed out"
#define ERR_NOT_LISTENING   ERR_PREFIX "Not listening"

#define ERR_COULDNT_BIND    ERR_PREFIX "Couldn't bind port"
#define ERR_COULDNT_ACCEPT  ERR_PREFIX "Couldn't accept connection"
#define ERR_COULDNT_CONNECT ERR_PREFIX "Couldn't connect"
#define ERR_COULDNT_LOOKUP  ERR_PREFIX "Couldn't resolve host"


#define TEST_IDLE() do{ \
		if(state != IDLE){ \
			lerr = ERR_NOT_IDLE; \
			return false; \
		} \
	}while(0)

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

TCPSocket::TCPSocket(
				TCPSocket& (connreq)(),
				void (*disconnected)(),
				void (*received)(void *, size_t),
				void (*error)(),
				int bs
		):
	fd(socket(DOMAIN, TYPE, PROTOCOL)), addr(),
	lerr(NULL), buffer(new char[bs]), buffersize(bs),
	connrequestf(connreq), disconnectedf(disconnected),
	receivedf(received), errorf(error), state(IDLE)
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

	memset(&addr, '\0', sizeof addr);
}


TCPSocket::TCPSocket(const TCPSocket& s):
	fd(), addr(), lerr(), buffer(NULL), buffersize(-1),
	connrequestf(), disconnectedf(), receivedf(),
	errorf(), state()
{
	*this = s;
}

TCPSocket& TCPSocket::operator=(const TCPSocket& s)
{
	fd = s.fd;

	memcpy(&addr, &s.addr, sizeof addr);

	lerr = s.lerr;

	buffer = new char[buffersize = s.buffersize];

	connrequestf = s.connrequestf;
	disconnectedf = s.disconnectedf;
	receivedf = s.receivedf;
	errorf = s.errorf;

	state = s.state;

	return *this;
}

bool TCPSocket::setblocking(bool block) const
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


TCPSocket::~TCPSocket()
{
	cleanup();
	shutdown(fd, SHUT_RDWR);
	close(fd);
	fd = -1;
	delete[] buffer;
}

const char *TCPSocket::remoteaddr()
{
	if(state != CONNECTED)
		throw ERR_NOT_CONNECTED;

	return addrtostr(&addr);
}

// data i/o -----------------------------------------

void TCPSocket::cleanup()
{
	newsocket(socket(DOMAIN, TYPE, PROTOCOL));
}

void TCPSocket::newsocket(int newfd)
{
	if(newfd == -1 || !setblocking(false))
		throw ERR_INIT;

	if(state == CONNECTED)
		shutdown(fd, SHUT_RDWR);

	close(fd);
	fd = newfd;
}

void TCPSocket::disconnect()
{
	if(state != IDLE)
		cleanup();
	lerr = NULL;
}

bool TCPSocket::senddata(const void *p, size_t siz)
{
	int ret;

	if(state != CONNECTED)
		throw ERR_NOT_CONNECTED;

	//win = sendto(fd, p, siz, 0, NULL, 0) != -1;
	//                                addr, addrlen

#if defined(MSG_NOSIGNAL) && !defined(SO_NOSIGPIPE)
	ret = send(fd, p, siz, MSG_NOSIGNAL /* prevent SIGPIPE etc */);
#elif defined(SO_NOSIGPIPE)
	ret = send(fd, p, siz, 0);
#else
	// TODO
	// not bsd nor linux
	// neither, nor:
	// http://krokisplace.blogspot.com/2010/02/suppressing-sigpipe-in-library.html
#error unknown OS
#endif

	if(ret == -1 || errno == EPIPE){
		if(errno == ECONNRESET)
			// disconnected
			lerr = ERR_NOT_CONNECTED;
		else
			lerr = ERR_SEND;

		cleanup();
		return false;
	}

	return true;
}

bool TCPSocket::senddata(const std::string& data)
{
	return senddata(data.c_str());
}

bool TCPSocket::senddata(const char c)
{
	return senddata(&c, (size_t)sizeof(char));
}

bool TCPSocket::senddata(const char *data)
{
	return senddata((const void *)data, strlen(data));
}

TCPSocket& TCPSocket::operator<<(const char c)
{
	senddata(c);
	return *this;
}

TCPSocket& TCPSocket::operator<<(const char *s)
{
	senddata(s);
	return *this;
}

TCPSocket& TCPSocket::operator<<(const std::string& s)
{
	senddata(s);
	return *this;
}

// ------------------------------------------------

const char *TCPSocket::getstatestr() const
{
	switch(state){
		case CONNECTED:  return "connected";
		case LISTENING:  return "listening";
		case CONNECTING: return "connecting";
		case IDLE:       return "idle";
	}
	return NULL;
}

enum TCPSocket::State TCPSocket::getstate() const
{
	return state;
}

const char *TCPSocket::lasterr() const
{
	return lerr;
}

// socket instructions -----------------------------

bool TCPSocket::connect(const char *host, int port)
{
	TEST_IDLE();

	if(!lookup(host, port, &addr)){
		lerr = ERR_COULDNT_LOOKUP;
		return false;
	}

	if(::connect(fd, (sockaddr *) &addr, sizeof addr) == -1)
		if(errno == EINPROGRESS){
			state = CONNECTING;
			return true;
		}
	lerr = ERR_COULDNT_CONNECT;
	return false;
}

bool TCPSocket::connect(const std::string& h, int port)
{
	return connect(h.c_str(), port);
}

bool TCPSocket::listen(int port)
{
	TEST_IDLE();

	addr.sin_family = AF_INET;
	addr.sin_port   = htons(port);

	if(bind(fd, (struct sockaddr *) &addr, sizeof addr) == -1){
		lerr = ERR_COULDNT_BIND;
		return false;
	}

	if(::listen(fd, LISTEN_BACKLOG) == -1){
		lerr = strerror(errno);
		return false;
	}else{
		state = LISTENING;
		lerr = NULL;
		return true;
	}
}

bool TCPSocket::runevents()
{
	/*
	 * if connected:
	 *   receive data/check disconnect
	 * elsif listening:
	 *   check for connections
	 * elsif connecting:
	 *   check connected
	 * elsif idle
	 *   break;
	 *
	 * returns true on state change
	 */

	switch(state){
		case CONNECTED:
			return checkconn();

		case LISTENING:
			return accept();

		case CONNECTING:
			return connectedyet();

		case IDLE:
			return false;
	}
	return true;
}

bool TCPSocket::connectedyet()
{
	struct pollfd fds;
	// check if we're connected yet

	fds.fd = fd;
	fds.events = POLLOUT;

	switch(poll(&fds, 1 /* count */, 0 /*ms*/)){
		case -1:
			cleanup();
			if(errno)
				lerr = strerror(errno);
			else
				lerr = ERR_TIMED_OUT;
#if DEBUG
			std::cerr << "TCPSocket::connectedyet(): connect() error: "
				<< lerr << " (" << errno << ")\n";
#endif
			return true;

		case 0:
			// not connected
			return false;

		default:
			if((fds.revents & POLLOUT) == POLLOUT){
				state = CONNECTED;
				return true;
			}
	}

	return false;
}

bool TCPSocket::accept()
{
	int newfd;
	struct sockaddr_in ad;
	socklen_t siz = sizeof ad;

	if(!connrequestf)
		return false;

	newfd = ::accept(fd, (struct sockaddr *) &ad, &siz);

	if(newfd == -1){
		if(errno != EAGAIN && errno != EWOULDBLOCK)
			lerr = ERR_COULDNT_ACCEPT;
		else
			lerr = NULL;

	}else /*if(connrequestf)*/ {
		TCPSocket& s(connrequestf());

		s.newsocket(newfd);

		s.state = CONNECTED;
		memcpy(&s.addr, &ad, sizeof ad);

		return true;
	}
	return false;
}

bool TCPSocket::checkconn()
{
	struct pollfd fds;
	int ret;

	if(state != CONNECTED)
		throw ERR_NOT_CONNECTED;

	fds.fd = fd;
	fds.events = POLLIN;

	switch(poll(&fds, 1, 0)){
		case -1:
#if DEBUG
			std::cerr << "TCPSocket::checkconn(): post-poll(): poll error " << errno << std::endl;
#endif
			cleanup();
			lerr = ERR_POLL;
			return true;

		case 0:
			// timeout
			return false;

		default:
			if((fds.revents & POLLIN) != POLLIN)
				return false;
	}

#if defined(MSG_NOSIGNAL) && !defined(SO_NOSIGPIPE)
	ret = recv(fd, buffer, buffersize, MSG_NOSIGNAL /* prevent SIGPIPE */);
#elif defined(SO_NOSIGPIPE)
	ret = recv(fd, buffer, buffersize, 0);
#else
	// not bsd nor linux
#error unknown OS
#endif
	/*
	 * funcs:
	 * read
	 * recv
	 * recvmsg <- udp
	 */

	switch(ret){
		case -1:
#if DEBUG
			std::cerr << "TCPSocket::checkconn(): recv() returned -1: " << errno << std::endl;
#endif
			if(errno == EWOULDBLOCK || errno == EAGAIN)
				return false;
			cleanup();
			state = IDLE;
			lerr = ERR_RECV;
			return true;

		case 0:
			// disco
			cleanup();
			state = IDLE;
			lerr = NULL;
			return true;
	}

	// got data
	receivedf(buffer, buffersize);
	return true;
}
