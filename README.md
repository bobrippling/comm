Comm v2.0

Protocol
--------

	<TYPE><DATA>\n

See poorman\'s\ tcomm.sh for a netcat example

Seperators in messages are the character 0x1d
"group seperator" aka ^]

Standard hailing frequency: Port 2848


Message Types
-------------

Client -\> Server:
	NAME %s
		client name - login request with name %s
	MESSAGE %s
		client sends a message to all
	COLOUR %s
		client sets its message colour
		this message is not sent back to a client
	CLIENT\_LIST
		server replies like so:
			CLIENT\_LIST\_START
			CLIENT\_LIST %s^]%s % name, sep, col
		OR
			CLIENT\_LIST %s % name
			CLIENT\_LIST\_END
			for each client except the queryer
	RENAME %s
		\_request\_ to change name
			server replies with ERR if unsuccessful
			(otherwise RENAME %s %s is sent)
	SU %s
		_request_ root-control of server with password %s
			if unsuccessful, an ERR message is returned
			otherwise, the server may optionally send an
			informational message (INFO)
	KICK %s
		ROOT request to kick user
	PRIVMSG %s^]%s
		Send %s[1] to %s[0] ONLY

Server -\> Client:
	CLIENT\_CONN %s
		a new client has connected, [b]with name[/b]
	CLIENT\_DISCO %s
		authenticated (named) client disconnected
	ERR %s
		client message error
	CLIENT\_LIST\*
		see (client-\>server)::CLIENT\_LIST
	MESSAGE %s
		message from server to all
	COLOUR %s^]%s
		client %s[0] sets its message colour to %s[1]
	INFO %s
		server informational message
	RENAME %s^]%s
		client %s[0] renamed to %s[1]
	RESTART %d
		Server is restarting, reconnect in %d seconds (default to 10)
	PRIVMSG %s
		Private message



Initial Contact/Handshake
-------------------------

Server sends
	Comm v$VERSION Optional-Server-Desc
where version is something like 1.2 (always in the form %d.%d)

client either disconnects, or continues with the following
	NAME Timmy
the server then responds with ERR %s or OK
if ERR, server then closes the connection

otherwise the handshake is successful
