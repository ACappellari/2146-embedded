import java.net.*;
import java.util.Scanner;
import java.io.*;

public class ClientSocket {

	public static void main(String[] args) {
		String hostname = "localhost";
		int port = 60001;
		try {
			final Socket socket = new Socket(hostname, port);
			final MqttClient client = new MqttClient("tcp://localhost:1883", MqttClient.generateClientId());
			client.connect();


			Thread receiver = new Thread(new Runnable() {

				@Override
				public void run() {
										
					InputStream input = null;
					try {
						input = socket.getInputStream();
						InputStreamReader reader = new InputStreamReader(input);
						int character;
						while(true){						
							StringBuilder data = new StringBuilder();
							while ((character = reader.read()) != -1 && character != '\n') {
								data.append((char) character);
							}
							if(data.equals("64")){
								System.out.println("Data : "+data);
								mqttPublish(clientmsg);
							}
						}
					} catch (IOException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}

				}
			});

			Thread sender = new Thread(new Runnable() {

				@Override
				public void run() {
					try {
						Writer out = new BufferedWriter(new OutputStreamWriter(socket.getOutputStream())); 
						Scanner scanner = new Scanner(System.in);
						while (true) {
							String msg = scanner.nextLine();
							out.write(msg+"\0");
							out.flush();
						}

					} catch (IOException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
				}
			});

			receiver.start();
			sender.start();
		} catch (UnknownHostException e1) {
			// TODO Auto-generated catch block
			e1.printStackTrace();
		} catch (IOException e1) {
			// TODO Auto-generated catch block
			e1.printStackTrace();
		}
	}

	public void mqttPublish(String message) throws MqttException {
		System.out.println("== START PUBLISHER ==");
		MqttMessage mqttMessage = new MqttMessage();
		mqttMessage.setPayload(message.getBytes());
		client.publish("temperature", mqttMessage);
		System.out.println("\tMessage '" + message + "' to 'temperature'");

	}
}
