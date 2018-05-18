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


#define MAX_RETRANSMISSIONS 8
#define NUM_HISTORY_ENTRIES 4
#define USE_RSSI 0


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

// Unicast connection for routing
static struct runicast_conn runicast;
// Unicast connection for sensor data
static struct runicast_conn sensor_runicast;


/*---------------------------------------------------------------------------*/
/*                             RELAY SENSOR DATA TO ROOT                     */

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
	printf("I HAVE SENT SENSORS DATA AGAIN\n");

}

/*---------------------------------------------------------------------------*/
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

	if(USE_RSSI == 0 && parent.nb_hops >  nb_hop){
		parent.addr.u8[0] = from->u8[0];
		parent.addr.u8[1] = from->u8[1];		
		parent.nb_hops = nb_hop;
		uint8_t tmp = nb_hop + 1;
		me.nb_hops = nb_hop + 1;
		printf("Change parent to %d.%d with %d hop\n", parent.addr.u8[0],parent.addr.u8[1],parent.nb_hops);
		// 
	}else if (USE_RSSI == 1 && last_rssi > parent.rssi){
		parent.addr.u8[0] = from->u8[0];
		parent.addr.u8[1] = from->u8[1];
		parent.rssi = last_rssi;
		printf("Change parent to %d.%d with %d rssi\n", parent.addr.u8[0],parent.addr.u8[1],parent.rssi);
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
	
	parent.addr.u8[0] = 0;
	parent.nb_hops = 254;
}

/*---------------------------------------------------------------------------*/

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

static const struct runicast_callbacks runicast_callbacks = {recv_runicast,
							     sent_runicast,
							     timedout_runicast};
static const struct runicast_callbacks sensor_runicast_callbacks = {sensor_recv_runicast,
							     	    sensor_sent_runicast,
							     	    sensor_timedout_runicast};
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

static struct broadcast_conn options_broadcast;

static void
options_broadcast_recv(struct broadcast_conn *c, const rimeaddr_t *from)
{
	if(parent.addr.u8[0] == from->u8[0] && parent.addr.u8[1] == from->u8[1]){
		char *payload = (char *)packetbuf_dataptr();
		printf("Received options broadcast from my parent '%s' \n", payload);
		packetbuf_copyfrom(payload, strlen(payload));
		broadcast_send(&options_broadcast);
		packetbuf_clear();
	}
}

static const struct broadcast_callbacks options_broadcast_call = {options_broadcast_recv};

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
	PROCESS_BEGIN();

	/* Initialize all structures used to read sensors */
	static struct etimer et_data;
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
	static struct etimer et;
	broadcast_open(&broadcast, 129, &broadcast_call); //Broadcast routing channel
	broadcast_open(&options_broadcast, 139, &options_broadcast_call);

	BROADCAST:while(parent.addr.u8[0] == 0 && parent.addr.u8[1] == 0) {
		//printf("LOOP start\n");
		/* Delay 2-4 seconds */
		etimer_set(&et, CLOCK_SECOND * 8 + random_rand() % (CLOCK_SECOND * 8));

		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
		
		// Send packet looking for a parent with your id
		printf("Broadcast sent, my parent %d.%d\n", parent.addr.u8[0], parent.addr.u8[1]);
		broadcast_send(&broadcast);
	}

	while(parent.addr.u8[0] != 0)
	{
		//PROCESS_YIELD();
		etimer_set(&et_data, CLOCK_SECOND * 8 + random_rand() % (CLOCK_SECOND * 16));
		//for (etimer_set(&et_data, CLOCK_SECOND);; etimer_reset(&et_data)) {
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et_data));

		    /* TEMPERATURE */
		    s_data.type = "./temperature";
		    s_data.value = (unsigned) (-39.60 + 0.01 * sht11_temp());
		    //printf("TYPE : %s\n", s_data.type);
		    //printf("VALUE : %u\n", s_data.value);
		    send_sensor_data();

		    /* HUMIDITY */		  
		    //rh = sht11_humidity();
		    //s_data.type = "./humidity";
		    //s_data.value = (unsigned) (-4 + 0.0405*rh - 2.8e-6*(rh*rh));
		    //printf("TYPE : %s\n", s_data.type);
		    //printf("VALUE : %u\n", s_data.value);

		    /* LIGHT */
		    //light = (unsigned) light_ziglet_read();
		    //s_data.type = "./light";
		    //s_data.value = (unsigned) light;
		    //printf("TYPE : %s\n", s_data.type);
		    //printf("VALUE : %u\n", s_data.value);
			
		  //}
	
	}
	
	
	goto BROADCAST;

	PROCESS_END();	
}
