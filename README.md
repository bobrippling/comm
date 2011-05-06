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

Client -> Server
-----------------

* client name - login request with name %s
<pre>
	NAME %s
</pre>

* client sends a message to all
<pre>
	MESSAGE %s
</pre>

* client sets its message colour (this message is not sent back to a client)
<pre>
	COLOUR %s
</pre>

* Client List
<pre>
	CLIENT_LIST
</pre>

Server Reply:
<pre>
	CLIENT_LIST_START
	CLIENT_LIST %s^]%s % name, sep, col
</pre>
or:
<pre>
	CLIENT_LIST %s % name
	CLIENT_LIST_END
</pre>

for each client except the queryer
<br/>
<br/>

* _request_ to change name server replies with ERR if unsuccessful (otherwise RENAME %s %s is sent)
<pre>
	RENAME %s
</pre>

* _request_ root-control of server with password %s
if unsuccessful, an ERR message is returned
otherwise, the server may optionally send an
informational message (INFO)
<pre>
	SU %s
</pre>

* *root* request to kick user
<pre>
	KICK %s
</pre>

* Send %s[1] to %s[0] ONLY
<pre>
	PRIVMSG %s^]%s
</pre>

* Send draw info
<pre>
  D%d_%d_%d_%d
</pre>

Server -> Client
-----------------

* a new client has connected, [b]with name[/b]
<pre>
	CLIENT_CONN %s
</pre>

* authenticated (named) client disconnected
<pre>
	CLIENT_DISCO %s
</pre>
* client message error
<pre>
	ERR %s
</pre>
* see (client->server)::CLIENT\_LIST
<pre>
	CLIENT_LIST*
</pre>
* message from server to all
<pre>
	MESSAGE %s
</pre>
* client %s[0] sets its message colour to %s[1]
<pre>
	COLOUR %s^]%s
</pre>
* server informational message
<pre>
	INFO %s
</pre>
* client %s[0] renamed to %s[1]
<pre>
	RENAME %s^]%s
</pre>
* Server is restarting, reconnect in %d seconds (default to 10)
<pre>
	RESTART %d
</pre>
* Private message
<pre>
	PRIVMSG %s
</pre>


Initial Contact/Handshake
-------------------------

Server sends

	Comm v$VERSION Optional-Server-Desc
where version is something like 1.2 (always in the form %d.%d)

client either disconnects, or continues with the following

	NAME Timmy
the server then responds with

	ERR %s
or

	OK

if ERR, server then closes the connection
otherwise the handshake is successful
