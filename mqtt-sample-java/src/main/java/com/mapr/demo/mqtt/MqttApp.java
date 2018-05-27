package com.mapr.demo.mqtt;

import org.eclipse.paho.client.mqttv3.MqttException;

import com.mapr.demo.mqtt.simple.ClientSocket;
import com.mapr.demo.mqtt.simple.Subscriber;

/**
 * Basic launcher for Gateway and Subscriber
 */
public class MqttApp {

  public static void main(String[] args) throws MqttException {

    if (args.length < 1) {
      throw new IllegalArgumentException("Must have either 'publisher', 'subscriber' or 'gateway' as argument");
    }
    switch (args[0]) {
      case "subscriber":
        Subscriber.main(args);
        break;
      case "gateway":
    	ClientSocket.main(null);
	break;
      default:
        throw new IllegalArgumentException("Don't know how to do " + args[0]);
    }
  }
}


