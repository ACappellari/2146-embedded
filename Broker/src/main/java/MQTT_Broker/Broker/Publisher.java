package com.mapr.demo.mqtt.simple;

import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;

public class Publisher {

	private ServerSocket serverSocket;
	String clientmsg = "";

	public Publisher(int port) throws IOException {
		serverSocket = new ServerSocket(port);
		serverSocket.setSoTimeout(10000);
	}

	public static void main(String[] args) throws MqttException {

		String messageString = "Hello World from Java!";

		if (args.length == 2) {
			messageString = args[1];
		}

		int port = 5689;
		try {
			Thread t = new Publisher(port);
			t.start();
		}

		catch (IOException e) {
			e.printStackTrace();
		}

	}

	public void run() {
		while (true) {
			try {
				System.out.println("Waiting for client on port " + serverSocket.getLocalPort() + "...");
				Socket server = serverSocket.accept();
				System.out.println("Just connected to " + server.getRemoteSocketAddress());
				BufferedReader in = new BufferedReader(new InputStreamReader(server.getInputStream()));
				clientmsg = in.readLine();
				System.out.println(in.readLine());
				mqttPublish(clientmsg);
				PrintWriter out = new PrintWriter(server.getOutputStream());
				System.out.println("Thank you for connecting to " + server.getLocalSocketAddress() + "\nGoodbye!");
				server.close();
			}

			catch (SocketTimeoutException s) {
				System.out.println("Socket timed out!");
				break;
			}

			catch (IOException e) {
				e.printStackTrace();
				break;
			}
		}
	}

	public void mqqtPublish(String message) {
		System.out.println("== START PUBLISHER ==");

		MqttClient client = new MqttClient("tcp://localhost:1883", MqttClient.generateClientId());
		client.connect();
		MqttMessage message = new MqttMessage();
		message.setPayload(messageString.getBytes());
		client.publish("iot_data", message);

		System.out.println("\tMessage '" + messageString + "' to 'iot_data'");

		client.disconnect();

		System.out.println("== END PUBLISHER ==");
	}

}
