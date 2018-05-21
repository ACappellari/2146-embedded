import java.net.*;
import java.util.Scanner;
import java.io.*;

public class ClientSocket {

	public static void main(String[] args) {
		String hostname = "localhost";
		int port = 60001;

		try {
			final Socket socket = new Socket(hostname, port);
			Thread receiver = new Thread(new Runnable() {

				@Override
				public void run() {
					InputStream input = null;
					try {
						input = socket.getInputStream();
						InputStreamReader reader = new InputStreamReader(input);
						int character;
						StringBuilder data = new StringBuilder();
						while ((character = reader.read()) != -1) {
							data.append((char) character);
						}
						System.out.println(data);
					} catch (IOException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}

				}
			});

			//Thread sender = new Thread(new Runnable() {

				//@Override
				//public void run() {
					try {
						
						//DataOutputStream out = new DataOutputStream(socket.getOutputStream());
						Writer out = new BufferedWriter(new OutputStreamWriter(socket.getOutputStream())); 
						//BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
						Scanner scanner = new Scanner(System.in);
						while (true) {

							System.out.println("Enter your username: ");
							String msg = scanner.nextLine();
							//out.writeChars(msg);
							//out.flush();
							//out.writeInt((int)msg);
							//out.write((int)msg);
							out.write(msg+"\n");
							out.flush();
						}

					} catch (IOException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
				//}
			//});

			receiver.start();
			//sender.start();
		} catch (UnknownHostException e1) {
			// TODO Auto-generated catch block
			e1.printStackTrace();
		} catch (IOException e1) {
			// TODO Auto-generated catch block
			e1.printStackTrace();
		}
	}
}
