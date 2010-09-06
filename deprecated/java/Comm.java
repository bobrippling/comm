import java.io.BufferedReader;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.Socket;
import java.util.Stack;
import java.net.UnknownHostException;
import java.util.EmptyStackException;

class Comm
{
	static final String MSG_UNKNOWN = "Unknown message from server ";

	static final int DEFAULT_PORT = 2848,
                   VERSION_MAJOR = 0,
                   VERSION_MINOR = 1;

	private Socket sock;
	private DataOutputStream out;
	private BufferedReader in;
	private Stack<String> messages;
	private String name;
	private boolean isconnected;

	class CommError extends Exception
	{
		private String e;
		public CommError(String e){this.e = e;}
		public String toString(){return e;}
	}

	class SockReader implements Runnable
	{
		private boolean running = true;
		public void stop()
		{
			running = false;
			try{
				in.close();
			}catch(IOException e){
			}
		}

		public void run()
		{
			try{
				do{
					String recv = in.readLine();
					if(recv == null)
						break;

					if(recv.substring(0, 8).equals("MESSAGE "))
						addmessage(recv.substring(8));
				}while(true);
			}catch(IOException e){
				// socket closed
			}
		}
	}

	public Comm(String host, int port, String name) throws CommError, IOException
	{
		this.name = name;
		messages = new Stack<String>();
		sock = connectedsock(host, port);
		new Thread(new SockReader()).start();
	}

	public void sendmessage(String msg) throws CommError
	{
		try{
			out.writeBytes("MESSAGE " + name + ": " + msg + "\n");
		}catch(IOException e){
			disconnect();
			throw new CommError(e.toString());
		}
	}

	private void addmessage(String msg)
	{
		messages.push(msg);
	}

	public String nextmessage()
	{
		try{
			return messages.pop();
		}catch(EmptyStackException e){
			return null;
		}
	}

	public void disconnect()
	{
		try{
			isconnected = false;
			sock.close();
		}catch(IOException e){
		}
	}

	public boolean isconnected()
	{
		return isconnected;
	}

	private Socket connectedsock(String host, int port) throws CommError
	{
		String recv, parts[], verparts[];
		int maj, min;
		Socket sock;

		try{
			sock = new Socket(host, port);
			out  = new DataOutputStream(sock.getOutputStream());
			in   = new BufferedReader(new InputStreamReader(sock.getInputStream()));
			recv = in.readLine(); // Comm v%d.%d
		}catch(UnknownHostException e){
			throw new CommError("Unknown Host: " + e);
		}catch(IOException e){
			throw new CommError(e.toString());
		}

		parts = recv.split(" ");

		if( parts.length != 2 ||
				!parts[0].equals("Comm") ||
				parts[1].charAt(0) != 'v')
			throw new CommError(MSG_UNKNOWN + recv);

		verparts = parts[1].substring(1).split("\\.");
		if(verparts.length != 2)
			throw new CommError(MSG_UNKNOWN + recv);

		try{
			maj = Integer.parseInt(verparts[0]);
			min = Integer.parseInt(verparts[1]);
		}catch(NumberFormatException e){
			throw new CommError(MSG_UNKNOWN + recv);
		}

		if(maj != VERSION_MAJOR || min != VERSION_MINOR)
			throw new CommError("Server version mismatch: " + maj + "." + min);

		// good to go
		try{
			out.writeBytes("NAME " + name + "\n");
			recv = in.readLine();
		}catch(IOException e){
			throw new CommError(e.toString());
		}

		if(recv.equals("ERR"))
			throw new CommError("Error from server: " + recv.substring(4));

		// very good to go
		return sock;
	}
}
