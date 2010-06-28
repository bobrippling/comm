#ifndef TCP_SOCKET_H
#define TCP_SOCKET_H

#define DEF_BUF_SIZE 256

class TCPSocket
{
	private:
		void cleanup();
		bool setblocking(bool) const;
		bool senddata(const void *, size_t);

		// events
		void newsocket(int);
		bool accept();
		bool checkconn();
		bool connectedyet();

		int fd;
		struct sockaddr_in addr;

		const char *lerr;
		char *buffer;
		int buffersize;

		// func ptrs
		TCPSocket& (*connrequestf)();
		void    (*disconnectedf)();
		void    (*receivedf)(void *, size_t);
		void    (*errorf)();

	public:
		enum State
		{
			CONNECTED,
			LISTENING,
			CONNECTING,
			IDLE
		};

		virtual ~TCPSocket();
		TCPSocket(
				TCPSocket& (connreq)(),
				void (*disconnected)(),
				void (*received)(void *, size_t),
				void (*error)(),
				int buffersize = DEF_BUF_SIZE
				);
		TCPSocket(const TCPSocket&);
		virtual TCPSocket& operator=(const TCPSocket&);

		bool runevents();

		bool connect(const char *host, int);
		bool connect(const std::string& host, int);

		bool listen(int port);
		bool listen(int port, const char *);

		void disconnect();

		bool senddata(const char *);
		bool senddata(const char);
		bool senddata(const std::string&);
		TCPSocket& operator<<(const char *);
		TCPSocket& operator<<(const char);
		TCPSocket& operator<<(const std::string&);

		enum State getstate() const;
		const char *getstatestr() const;
		const char *remoteaddr();
		const char *lasterr() const;

		// function pointer getters
		void (*get_connrequestfunc());
		void (*get_disconnectedfunc());
		void (*get_receivedfunc(void *, size_t));
		void (*get_errorfunc());

		// function pointer setters
		void set_connrequestfunc( void (*f));
		void set_disconnectedfunc(void (*f));
		void set_receivedfunc(    void (*f(void *, size_t)));
		void set_errorfunc(       void (*f));

	private:
		enum State state;
};

#undef DEF_BUF_SIZE

#endif
