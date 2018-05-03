package com.mapr.demo.mqtt.simple;



import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketTimeoutException;

import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;

public class Publisher extends Thread{

	private ServerSocket serverSocket;

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
				String clientmsg = in.readLine();
				System.out.println(in.readLine());
				try {
					mqttPublish(clientmsg);
				} catch (MqttException e) {
					System.out.println("Mqtt publish failed");
					e.printStackTrace();
				}
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

	public void mqttPublish(String message) throws MqttException {
		System.out.println("== START PUBLISHER ==");

		MqttClient client = new MqttClient("tcp://localhost:1883", MqttClient.generateClientId());
		client.connect();
		MqttMessage mqttMessage = new MqttMessage();
		mqttMessage.setPayload(message.getBytes());
		client.publish("iot_data", mqttMessage);

		System.out.println("\tMessage '" + message + "' to 'iot_data'");

		client.disconnect();

		System.out.println("== END PUBLISHER ==");
	}

}
