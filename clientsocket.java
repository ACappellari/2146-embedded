import java.net.*;
import java.io.*;
 
/**
 * This program is a socket client application that connects to a time server
 * to get the current date time.
 *
 * @author www.codejava.net
 */
public class clientsocket {
 
    public static void main(String[] args) {
        String hostname = "localhost";
        int port = 60001;
 
        try (Socket socket = new Socket(hostname, port)) {
 
            InputStream input = socket.getInputStream();
            InputStreamReader reader = new InputStreamReader(input);
 		DataOutputStream out = new DataOutputStream(socket.getOutputStream());
BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
	//out.writeByte(1);
////out.writeUTF("oulaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
//out.flush();
int i = 0;
while(i!=10){
	//System.out.println(in.readByte());
	System.out.println(in.readLine());
i++;
}
            int character;
            StringBuilder data = new StringBuilder();
 		
            while ((character = reader.read()) != -1) {
                data.append((char) character);
            }
 
            System.out.println(data);
 	  
 
        } catch (UnknownHostException ex) {
 
            System.out.println("Server not found: " + ex.getMessage());
 
        } catch (IOException ex) {
 
            System.out.println("I/O error: " + ex.getMessage());
        }
    }
}
