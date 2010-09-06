import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.IOException;
import java.lang.InterruptedException;

class Main
{
	static void usage()
	{
		System.out.println("Usage: Comm [OPTIONS] host");
		System.out.println(" -n NAME: Set name");
		System.out.println(" -p PORT: Set port");
		System.exit(1);
	}

	public static void main(String args[])
	{
		String host = null, name = "JavaNoob";
		int port = 2848;

		for(int i = 0; i < args.length; i++)
			if("--".equals(args[i]))
				continue;
			else if(args[i].charAt(0) == '-'){
				switch(args[i].charAt(1)){
					case 'p':
						if(++i < args.length)
							try{
								port = Integer.parseInt(args[i]);
							}catch(NumberFormatException e){
								System.out.println(e);
								usage();
							}
						else{
							System.out.println("Need port");
							usage();
						}
						break;

					case 'n':
						if(++i < args.length)
							name = args[i];
						else{
							System.out.println("Need name");
							usage();
						}
						break;
				}
			}else if(host == null)
				host = args[i];
			else
				usage();

		if(host == null)
			usage();


		// Run begins here
		final Comm comm;
		try{
			comm = new Comm(host, port, name);
		}catch(Comm.CommError e){
			System.out.println("Couldn't connect to server: " + e);
			System.exit(1);
			return; // fudging java
		}catch(IOException e){
			System.err.println(e);
			System.exit(1);
			return; // fudging java
		}

		class Shutdown extends Thread
		{
			public void run()
			{
				comm.disconnect();
			}
		}
		Runtime.getRuntime().addShutdownHook(new Shutdown());

		class CommCheck extends Thread
		{
			public void run()
			{
				while(comm.isconnected()){
					String msg = comm.nextmessage();
					if(msg != null)
						System.out.println(msg);
					else
						try{
							Thread.sleep(150);
						}catch(InterruptedException e){
						}
				}
			}
		}
		new Thread(new CommCheck()).start();

		BufferedReader stdin = new BufferedReader(new InputStreamReader(System.in));
		try{
			do{
				String recv = stdin.readLine();

				if(recv == null){
					comm.disconnect();
					break;
				}else{
					try{
						comm.sendmessage(recv);
					}catch(Comm.CommError e){
						System.err.println(e);
						break;
					}
				}
			}while(true);
		}catch(IOException e){
			System.exit(1);
		}
	}
}
