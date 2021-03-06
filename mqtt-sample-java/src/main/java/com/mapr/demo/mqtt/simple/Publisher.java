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

import java.net.*;
import java.util.Scanner;
import java.io.*;

public class Publisher extends Thread {

	private ServerSocket serverSocket;
	private MqttClient client;

	public Publisher(int port) throws IOException {
		serverSocket = new ServerSocket(port);
		serverSocket.setSoTimeout(100000);
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
		try {
			client = new MqttClient("tcp://localhost:1883", MqttClient.generateClientId());
			client.connect();
			System.out.println("Waiting for client on port " + serverSocket.getLocalPort() + "...");
			Socket server = serverSocket.accept();
			System.out.println("Just connected to " + server.getRemoteSocketAddress());
			PrintWriter out = new PrintWriter(server.getOutputStream());
			BufferedReader in = new BufferedReader(new InputStreamReader(server.getInputStream()));
			while (true) {
				try {
					
					String clientmsg = in.readLine();
					System.out.println(clientmsg);
					mqttPublish(clientmsg);
					out.println("Thank you");
				}catch (IOException | MqttException s) {
					System.out.println("Socket timed out!");
					s.printStackTrace();
					break;
				}


			}
			System.out.println("Thank you for connecting to " + server.getLocalSocketAddress() + "\nGoodbye!");
			server.close();
			client.disconnect();
		} catch (MqttException | IOException e1) {
			e1.printStackTrace();
		}
		System.out.println("== END PUBLISHER ==");
	}

	public void mqttPublish(String message) throws MqttException {
		System.out.println("== START PUBLISHER ==");
		MqttMessage mqttMessage = new MqttMessage();
		mqttMessage.setPayload(message.getBytes());
		client.publish("iot_data", mqttMessage);
		System.out.println("\tMessage '" + message + "' to 'iot_data'");

	}

}
