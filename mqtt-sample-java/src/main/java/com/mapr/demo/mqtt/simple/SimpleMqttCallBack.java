package com.mapr.demo.mqtt.simple;

import java.io.BufferedWriter;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.io.Writer;
import java.net.Socket;
import java.util.Scanner;

import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttMessage;

public class SimpleMqttCallBack implements MqttCallback {
	Socket socket;
	boolean gateway;

	public SimpleMqttCallBack(Socket socket) {
		this.socket = socket;
		this.gateway = true;
	}

	public SimpleMqttCallBack() {
	}

	public void connectionLost(Throwable throwable) {
		System.out.println("Connection to MQTT broker lost!");
	}

	public void messageArrived(String s, MqttMessage mqttMessage) throws Exception {
		
		if (!gateway){
			System.out.println("Message received:\t"+ new String(mqttMessage.getPayload()) );
			return;
		}
		try {
			Writer out = new BufferedWriter(new OutputStreamWriter(socket.getOutputStream()));
			Scanner scanner = new Scanner(System.in);
			String msg = new String(mqttMessage.getPayload());
			if(Integer.parseInt(msg) <= 1){
				out.write(2 + "\0");
				out.flush();
			}else{
				out.write(0 + "\0");
				out.flush();
			}

		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		System.out.println("Message received:\t" + new String(mqttMessage.getPayload()));
	}

	public void deliveryComplete(IMqttDeliveryToken iMqttDeliveryToken) {
	}
}

