/**
 * \file
 *         Sensor node implementation, for the "IOT network using Rime" project of the "LINGI2146 - Mobile and Embedded Computing"
 *         course. UCL 2017-2018.
 * \author
 *         Daubry Benjamin <benjamin.daubry@student.uclouvain.be>
 *         Dizier Romain <romain.dizier@student.uclouvain.be>
 *         Lovisetto Gary <gary.lovisetto@student.uclouvain.be>
 */

#include "contiki.h"
#include "net/rime.h"
#include "random.h"
#include "lib/list.h"
#include "lib/memb.h"
#include <stdio.h>
#include <stdlib.h>
#include "dev/sht11.h"
#include "dev/i2cmaster.h"
#include "dev/light-ziglet.h"

/* CONSTANTS */
#define MAX_RETRANSMISSIONS 16
#define NUM_HISTORY_ENTRIES 4
#define USE_RSSI 0 //Whether or not we use number of hops or RSSI to chose parent
#define ACK_CHILD -42 //Value used to identify an aknowledgment from a child node
#define MAX_CHILDREN 10 //Maximum number of entries in the table of children per node
#define TYPE_SENSOR 0 //Type of data collected by the sensor node : 0=temperature, 1=humidity, 2=light

/*---------------------------------------------------------------------------*/
PROCESS(sensor_network_process, "Sensor network implementation");
AUTOSTART_PROCESSES(&sensor_network_process);
/*---------------------------------------------------------------------------*/
				/*Forward declaration */

//Structure used to model a routing packet
struct response_packet ; 
typedef struct response_packet{
	uint8_t id;
	uint8_t nb_hops;
} response;

//Structure used to model a node
struct node_unicast;
typedef struct node_unicast{
	rimeaddr_t addr;
	uint8_t nb_hops;
	int16_t rssi;
} node;

//Each node store its own values as well as its parent values
node parent;
node me;

//Structure used to model sensor data produced by a sensor node
struct sensor_data;
typedef struct sensor_data{
	uint16_t value;
	char* type;
} sensor_data;

//Each node stores its sensor data
sensor_data s_data;

//Structure used to model a child node
struct tuple_child;
typedef struct tuple_child{
	rimeaddr_t addr;
} tuple_child;

//Each node stores an array containing the address of its children
tuple_child children[MAX_CHILDREN];
static number_children = 0;

/* Options for the behaviour of the sensor node :
   - 0 : the node sends data periodically (every x seconds)
   - 1 : the node sends data when there is a change
   - 2 : the node does not sends data since there are no subscribers for its subject.
*/
uint8_t option_sensor_data = 0;

// Unicast connection for routing
static struct runicast_conn runicast;
// Unicast connection for sensor data
static struct runicast_conn sensor_runicast;
// Unicast connection for options
static struct runicast_conn options_runicast;

/*---------------------------------------------------------------------------*/
			/*RELAY SENSOR DATA TO ROOT*/

/*
 * Function used to send data values coming from another node (down) towards to root node (up).
 * \parameter : the string representation of the data to be relayed.
 */
static void
relay_sensor_data(char * s_payload)
{
	printf("MESSAGE TO RELAY : %s\n", s_payload);
	/* Waits for the connection to be available */
	while(runicast_is_transmitting(&sensor_runicast)){}
	/* Create the sensor packet */
	packetbuf_clear();
	int len = strlen(s_payload);
	char buf[len+16];
	snprintf(buf, sizeof(buf), "%s", s_payload);
	printf("THIS IS PACKET CONTENT %s\n", buf);
	packetbuf_copyfrom(&buf, strlen(buf));
	/* Then sends it */
	runicast_send(&sensor_runicast, &parent.addr, MAX_RETRANSMISSIONS);
	packetbuf_clear();
	printf("I HAVE SENT SENSORS DATA AGAIN\n");

}

/*---------------------------------------------------------------------------*/
			/*RELAY OPTIONS TO CHILD NODES*/

/*
 * Function used to send the option value coming from another node (up) towards child nodes (down).
 * \parameter : the string representation of the option to be relayed.
 */
static void
relay_options_data(char * s_payload)
{
	printf("MESSAGE TO RELAY : %s\n", s_payload);
	/* Waits for the connection to be available */
	while(runicast_is_transmitting(&options_runicast)){}
	/* Create the sensor packet */
	int len = strlen(s_payload);
	char buf[len];
	snprintf(buf, sizeof(buf), "%s", s_payload);
	packetbuf_clear();
	packetbuf_copyfrom(&buf, strlen(buf));
	/* Then sends it */
	int j;
	for(j = 0; j < number_children; j++) {
		runicast_send(&options_runicast, &children[j].addr, MAX_RETRANSMISSIONS);
	}
	packetbuf_clear();
	printf("I HAVE SENT OPTIONS DATA AGAIN\n");

}

/*---------------------------------------------------------------------------*/
			/* SEND CHILD UNICAST */

/*
 * Function used to send an acknowledgment towards the newly accepted parent of the node.
 */
static void
send_child_confirmation()
{
	/* Wait for the runicast channel to be available*/
	while(runicast_is_transmitting(&runicast)){}
	/* Create the sensor packet */
	packetbuf_clear();
	uint16_t max = ACK_CHILD;
	char buf[16];
	snprintf(buf, sizeof(buf), "%d", max);
	packetbuf_copyfrom(&buf, strlen(buf));
	/* Then sends it */
	runicast_send(&runicast, &parent.addr, MAX_RETRANSMISSIONS);
	packetbuf_clear();
	printf("I HAVE SENT CHILD ACK\n");

}

/*
 * Function used to verify that a given node is not already in the list of child of the node.
 * \parameter : a tuple_child structure representing a node.
 */
static int
verify_children(tuple_child t){
	int isIn = 0;
	int j;
	for(j = 0; j < number_children; j++) {
		if(children[j].addr.u8[0] == t.addr.u8[0] && children[j].addr.u8[1] == t.addr.u8[1]){
			return j;
		}
	}
	return isIn;
}

/*
 * Function used to remove a child node from the array of known child by shifting the whole array from its index.
 * \parameter : the index of the child to remove.
 */
void removeChild(int index)
{
	int i;
	for(i = index; i < number_children - 1; i++){
		children[i] = children[i + 1];
	}
	number_children--;
  
}
/*---------------------------------------------------------------------------*/
			/* UNICAST ROUTING PACKETS */

static void
recv_runicast(struct runicast_conn *c, const rimeaddr_t *from, uint8_t seqno)
{
  printf("runicast message received from %d.%d, seqno %d\n",
	 from->u8[0], from->u8[1], seqno);

	//Received a response from my broadcast, should compare number of hops and override
	uint16_t last_rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);
	char * payload = (char *) packetbuf_dataptr();
	printf("Payload received : %s\n", payload);
	uint8_t nb_hop = (uint8_t) atoi(payload);
	uint8_t test = -42;
	//This is a ACK_CHILD message
	if(nb_hop == test){
		if(number_children < MAX_CHILDREN){
			tuple_child temp;
			temp.addr.u8[0] = from->u8[0];
			temp.addr.u8[1] = from->u8[1];
			if(verify_children(temp) == 0){
				children[number_children] = temp;
				number_children++;
			}
		}
		else{
			printf("Max number of children reached\n");
		}
	}
	//This is a standard routing message
	else{
		tuple_child temp;
		temp.addr.u8[0] = from->u8[0];
		temp.addr.u8[1] = from->u8[1];
		if(USE_RSSI == 0 && parent.nb_hops >  nb_hop && verify_children(temp) == 0){
			parent.addr.u8[0] = from->u8[0];
			parent.addr.u8[1] = from->u8[1];		
			parent.nb_hops = nb_hop;
			uint8_t tmp = nb_hop + 1;
			me.nb_hops = nb_hop + 1;
			printf("Change parent to %d.%d with %d hop\n", parent.addr.u8[0],parent.addr.u8[1],parent.nb_hops);
			//Send a unicast message to the parent to add me to its children
			send_child_confirmation();
		}else if (USE_RSSI == 1 && last_rssi > parent.rssi && verify_children(temp) == 0){
			parent.addr.u8[0] = from->u8[0];
			parent.addr.u8[1] = from->u8[1];
			parent.rssi = last_rssi;
			printf("Change parent to %d.%d with %d rssi\n", parent.addr.u8[0],parent.addr.u8[1],parent.rssi);
			//Send a unicast message to the parent to add me to its children
			send_child_confirmation();
		}
	}
	
}
static void
sent_runicast(struct runicast_conn *c, const rimeaddr_t *to, uint8_t retransmissions)
{
  printf("runicast message sent to %d.%d, retransmissions %d\n",
	 to->u8[0], to->u8[1], retransmissions);

}
static void
timedout_runicast(struct runicast_conn *c, const rimeaddr_t *to, uint8_t retransmissions)
{
  	printf("runicast message timed out when sending to %d.%d, retransmissions %d\n",
	 to->u8[0], to->u8[1], retransmissions);
	
	//Create temporary node for testing
	tuple_child temp;
	temp.addr.u8[0] = to->u8[0];
	temp.addr.u8[1] = to->u8[1];
	int isChild = verify_children(temp);
	
	//If the node lost is the parent
	if(temp.addr.u8[0] == parent.addr.u8[0] && temp.addr.u8[1] == parent.addr.u8[1]){
		parent.addr.u8[0] = 0;
		parent.nb_hops = 254;
	}
	//If the node lost is a child node
	else if(isChild > 0){
		removeChild(isChild);
	}
	else{
		printf("Error, unicast message sent to unknown node timed out\n");
	}
}

/*---------------------------------------------------------------------------*/
			  /* UNICAST SENSOR DATA PACKETS */

static void
sensor_recv_runicast(struct runicast_conn *c, const rimeaddr_t *from, uint8_t seqno)
{
  printf("runicast message received from %d.%d, seqno %d\n",
	 from->u8[0], from->u8[1], seqno);

	char * s_payload = (char *) packetbuf_dataptr();
	printf("SENSOR PACKET RECEIVED : %s\n", s_payload);
	// Then sends it to its parent node until it reaches the root
	relay_sensor_data(s_payload);
		

}
static void
sensor_sent_runicast(struct runicast_conn *c, const rimeaddr_t *to, uint8_t retransmissions)
{
  printf("runicast message sent to %d.%d, retransmissions %d\n",
	 to->u8[0], to->u8[1], retransmissions);

}

static void
sensor_timedout_runicast(struct runicast_conn *c, const rimeaddr_t *to, uint8_t retransmissions)
{
  	printf("runicast message timed out when sending to %d.%d, retransmissions %d\n",
	 to->u8[0], to->u8[1], retransmissions);
	
	parent.addr.u8[0] = 0;
	parent.nb_hops = 254;
}

/*---------------------------------------------------------------------------*/
			  /* UNICAST OPTIONS PACKETS */

static void
options_recv_runicast(struct runicast_conn *c, const rimeaddr_t *from, uint8_t seqno)
{
  printf("runicast message received from %d.%d, seqno %d\n",
	 from->u8[0], from->u8[1], seqno);

	char * payload = (char *) packetbuf_dataptr();
	printf("OPTION PACKET RECEIVED : %s\n", payload);
	uint8_t option = (uint8_t) atoi(payload);
	if(option == 0 || option == 1 || option == 2){
		option_sensor_data = option;
		relay_options_data(payload);
	}
}

static void
options_sent_runicast(struct runicast_conn *c, const rimeaddr_t *to, uint8_t retransmissions)
{
  printf("runicast message sent to %d.%d, retransmissions %d\n",
	 to->u8[0], to->u8[1], retransmissions);

}

static void
options_timedout_runicast(struct runicast_conn *c, const rimeaddr_t *to, uint8_t retransmissions)
{
  	printf("runicast message timed out when sending to %d.%d, retransmissions %d\n",
	 to->u8[0], to->u8[1], retransmissions);
	
	//Create temporary node for testing
	tuple_child temp;
	temp.addr.u8[0] = to->u8[0];
	temp.addr.u8[1] = to->u8[1];
	int isChild = verify_children(temp);
	//If the node lost is a child node
	if(isChild > 0){
		removeChild(isChild);
	}
	else{
		printf("Error, unicast message sent to unknown node timed out\n");
	}
}

/*---------------------------------------------------------------------------*/
			/* UNICAST CALLBACKS */

static const struct runicast_callbacks runicast_callbacks = {recv_runicast,
							     sent_runicast,
							     timedout_runicast};

static const struct runicast_callbacks sensor_runicast_callbacks = {sensor_recv_runicast,
							     	    sensor_sent_runicast,
							     	    sensor_timedout_runicast};

static const struct runicast_callbacks options_runicast_callbacks = {options_recv_runicast,
							     	     options_sent_runicast,
							     	     options_timedout_runicast};
/*---------------------------------------------------------------------------*/

static void
broadcast_recv(struct broadcast_conn *c, const rimeaddr_t *from)
{
	// Check if I have a parent and if yes I open a unicast and send response
	if(parent.addr.u8[0] != 0){
		printf("I have a parent and received broadcast\n");
		while(runicast_is_transmitting(&runicast)){}
		//printf("Entered unicast send preparation\n");	
		packetbuf_clear();
		rimeaddr_t recv;
		char *nb_hop;
		sprintf(nb_hop, "%d", me.nb_hops);
		packetbuf_copyfrom(nb_hop, sizeof(nb_hop));
      		recv.u8[0] = from->u8[0];
      		recv.u8[1] = from->u8[1];
		runicast_send(&runicast, &recv, MAX_RETRANSMISSIONS);
		//printf("Left unicast send\n");
	}
}

static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;

/*---------------------------------------------------------------------------*/
			/* SEND SENSOR DATA */

/*
 * Function used to send data values coming from this node (down) towards its parent node (up).
 */
static void
send_sensor_data()
{
	/* Wait for the runicast channel to be available*/
	while(runicast_is_transmitting(&sensor_runicast)){}
	/* Create the sensor packet */
	packetbuf_clear();
	int len = strlen(s_data.type);
	char buf[len + 16];
	snprintf(buf, sizeof(buf), "%s %d", s_data.type, s_data.value);
	//printf("THIS IS PACKET CONTENT %s\n", buf);

	packetbuf_copyfrom(&buf, strlen(buf));
	/* Then sends it */
	runicast_send(&sensor_runicast, &parent.addr, MAX_RETRANSMISSIONS);
	packetbuf_clear();
	printf("I HAVE SENT SENSORS DATA\n");

}


/*---------------------------------------------------------------------------*/
			/* PROCESS THREAD */

PROCESS_THREAD(sensor_network_process, ev, data)
{
	
	PROCESS_EXITHANDLER(broadcast_close(&broadcast);)
	PROCESS_EXITHANDLER(runicast_close(&runicast);)
	PROCESS_EXITHANDLER(runicast_close(&sensor_runicast);)
	PROCESS_EXITHANDLER(runicast_close(&options_runicast);)
	PROCESS_BEGIN();

	/* Initialize all structures used to read sensors */
	static struct etimer et_data;
	static struct etimer et;
	static unsigned rh;
	sht11_init();
	uint16_t light;
 	light_ziglet_init();
	
	/* Initialize internal parameters of the node */
	parent.addr.u8[0] = 0;
	parent.addr.u8[1] = 0;
	parent.rssi = (signed) -65534;
	parent.nb_hops = 254;	

	me.addr.u8[0] = rimeaddr_node_addr.u8[0];
	me.addr.u8[1] = rimeaddr_node_addr.u8[1];
	
	/* Opens the unicast and broadcast channels */
	runicast_open(&runicast, 144, &runicast_callbacks); //Unicast routing channel
	runicast_open(&sensor_runicast, 154, &sensor_runicast_callbacks); //Sensor data channel
	runicast_open(&options_runicast, 164, &options_runicast_callbacks); //Options channel
	broadcast_open(&broadcast, 129, &broadcast_call); //Broadcast routing channel

	BROADCAST:while(parent.addr.u8[0] == 0 && parent.addr.u8[1] == 0) {
		/* Delay */
		etimer_set(&et, CLOCK_SECOND * 8 + random_rand() % (CLOCK_SECOND * 16));

		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
		
		// Send packet looking for a parent with your id
		printf("Broadcast sent, my parent %d.%d\n", parent.addr.u8[0], parent.addr.u8[1]);
		broadcast_send(&broadcast);
	}

	while(parent.addr.u8[0] != 0)
	{
		
		etimer_set(&et_data, CLOCK_SECOND * 2 + random_rand() % (CLOCK_SECOND * 8));
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et_data));
		
		/* TEMPERATURE */
		if(TYPE_SENSOR == 0){
			s_data.type = "./temperature";
			if(option_sensor_data == 0){
				s_data.value = (unsigned) (-39.60 + 0.01 * sht11_temp());
				send_sensor_data();
			}
			else if(option_sensor_data == 1){
				uint16_t tempvalue = (unsigned) (-39.60 + 0.01 * sht11_temp());
				if(s_data.value != tempvalue){
					s_data.value = tempvalue;
					send_sensor_data();
				}
			}
			else if(option_sensor_data == 2){
				/*Does nothing, there is no subscribers */
			}
			else{
				printf("Unknown sensor option value");
			}
		}

		/* HUMIDITY */
		if(TYPE_SENSOR == 1){		  
			rh = sht11_humidity();
			s_data.type = "./humidity";
			if(option_sensor_data == 0){
				s_data.value = (unsigned) (-4 + 0.0405*rh - 2.8e-6*(rh*rh));
				send_sensor_data();
			}
			else if(option_sensor_data == 1){
				uint16_t tempvalue = (unsigned) (-4 + 0.0405*rh - 2.8e-6*(rh*rh));
				if(s_data.value != tempvalue){
					s_data.value = tempvalue;
					send_sensor_data();
				}
			}
			else if(option_sensor_data == 2){
				/*Does nothing, there is no subscribers */
			}
			else{
				printf("Unknown sensor option value");
			}
		}

		if(TYPE_SENSOR == 2){
			/* LIGHT */
			s_data.type = "./light";

			if(option_sensor_data == 0){
				light = (unsigned) light_ziglet_read();
				s_data.value = (unsigned) light;
				send_sensor_data();
			}
			else if(option_sensor_data == 1){
				light = (unsigned) light_ziglet_read();
				uint16_t tempvalue = (unsigned) light;
				if(s_data.value != tempvalue){
					s_data.value = tempvalue;
					send_sensor_data();
				}
			}
			else if(option_sensor_data == 2){
				/*Does nothing, there is no subscribers */
			}
			else{
				printf("Unknown sensor option value");
			}	
		}
	}
	
	
	goto BROADCAST;

	PROCESS_END();	
}
