#include "contiki.h"
#include "net/rime.h"
#include "random.h"

#include "lib/list.h"
#include "lib/memb.h"

#include <stdio.h>
#include <stdlib.h>

/* Data sensors */
//TEMPERATURE AND HUMIDITY
#include "dev/sht11.h"
//LIGHT
#include "dev/i2cmaster.h"
#include "dev/light-ziglet.h"


#define MAX_RETRANSMISSIONS 16
#define NUM_HISTORY_ENTRIES 4
#define USE_RSSI 0
#define ACK_CHILD -42
#define MAX_CHILDREN 10

#define TYPE_SENSOR 0

/*---------------------------------------------------------------------------*/
PROCESS(sensor_network_process, "Sensor network implementation");
AUTOSTART_PROCESSES(&sensor_network_process);
/*---------------------------------------------------------------------------*/
				/*Forward declaration */
struct response_packet ; 
typedef struct response_packet{
	uint8_t id;
	uint8_t nb_hops;
} response;

struct node_unicast;
typedef struct node_unicast{
	rimeaddr_t addr;
	uint8_t nb_hops;
	int16_t rssi; //Signed because can be negative
} node;

node parent;
node me;

struct sensor_data;
typedef struct sensor_data{
	uint16_t value;
	char* type;
} sensor_data;

sensor_data s_data;

struct tuple_child;
typedef struct tuple_child{
	rimeaddr_t addr;
} tuple_child;

tuple_child children[MAX_CHILDREN];
static number_children = 0;

/* Options for the behaviour of the sensor node :
   - 0 : the node sends data periodically (every x seconds)
   - 1 : the node sends data when there is a change
   - 2 : the node sends data only when there is at least one subscriber to the topic
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

static void
relay_sensor_data(char * s_payload)
{
	printf("MESSAGE TO RELAY : %s\n", s_payload);
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

static void
relay_options_data(char * s_payload)
{
	printf("MESSAGE TO RELAY : %s\n", s_payload);
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

static int
verify_children(tuple_child t){
	int isIn = 0;
	int j;
	for(j = 0; j < number_children; j++) {
		if(children[j].addr.u8[0] == t.addr.u8[0] && children[j].addr.u8[1] == t.addr.u8[1]){
			//isIn = 1;
			return j;
		}
	}
	return isIn;
}

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
	//printf("RSSI : %d\n", last_rssi);
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
	//char * payload = (char *)  packetbuf_dataptr();
	//printf("Payload received : %s\n", payload);
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

static void
send_sensor_data()
{
	/* Set the original receiver of the packet to the root node */
	//rimeaddr_t *root;
	//root->u8[0] = 0;
	//root->u8[1] = 0;
	//packetbuf_set_addr(PACKETBUF_ADDR_ERECEIVER, root);
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

// void 	broadcast_open (struct broadcast_conn *c, uint16_t channel, const struct broadcast_callbacks *u)
// void 	broadcast_close (struct broadcast_conn *c)
// int 		broadcast_send (struct broadcast_conn *c)

// void 	runicast_open(struct runicast_conn *c, uint16_t channel, const struct runicast_callbacks *u)
// void		runicast_close(struct runicast_conn *c)
// int 		runicast_send(struct runicast_conn *c, const rimeaddr_t *receiver, uint8_t max_retransmissions)


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
		//printf("LOOP start\n");
		/* Delay 2-4 seconds */
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
