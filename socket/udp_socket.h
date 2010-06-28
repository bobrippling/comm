#ifndef UDP_SOCKET_H
#define UDP_SOCKET_H

#define DEF_BUF_SIZE 256

class UDPSocket
{
	private:
		bool setblocking(bool) const;

		const char *lerr;
		char *buffer;
		int buffersize, fd;
		struct sockaddr_in addr;

	public:
		virtual ~UDPSocket();
		UDPSocket(const char *host, int port, int bs = DEF_BUF_SIZE);
		UDPSocket(const UDPSocket&);
		virtual UDPSocket& operator=(const UDPSocket&);

		bool senddata(const void *, size_t);
		bool senddata(const char *);
		bool senddata(const char);
		bool senddata(const std::string&);
		UDPSocket& operator<<(const char *);
		UDPSocket& operator<<(const char);
		UDPSocket& operator<<(const std::string&);

		bool recvdata(void *, size_t);
		bool recvdata(char *, size_t);
		bool recvdata(std::string&);
		UDPSocket& operator>>(std::string&);

		const char *remoteaddr();
		const char *lasterr() const;
};

#undef DEF_BUF_SIZE

#endif
