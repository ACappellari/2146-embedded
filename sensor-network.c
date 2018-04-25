#include "contiki.h"
#include "net/rime.h"
#include "random.h"

#include "lib/list.h"
#include "lib/memb.h"

#include "dev/button-sensor.h"
#include "dev/leds.h"
#include <stdio.h>
#include <stdlib.h>


#define MAX_RETRANSMISSIONS 4
#define NUM_HISTORY_ENTRIES 4
#define USE_RSSI 0

/*---------------------------------------------------------------------------*/
PROCESS(sensor_network_process, "Sensor network implementation");
AUTOSTART_PROCESSES(&sensor_network_process);
/*---------------------------------------------------------------------------*/
struct response_packet ; /* Forward declaration */

typedef struct response_packet
{
	uint8_t id;
	uint8_t nb_hops;
} response ;

struct node_unicast;
typedef struct node_unicast{
	rimeaddr_t addr;
	uint8_t nb_hops;
	uint16_t rssi;
} node;

node parent ;
node me;

/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
static void
recv_runicast(struct runicast_conn *c, const rimeaddr_t *from, uint8_t seqno)
{
  printf("runicast message received from %d.%d, seqno %d\n",
	 from->u8[0], from->u8[1], seqno);

	//Received a response from my broadcast, should compare number of hops and override
	uint16_t last_rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);
	printf("RSSI : %d\n", last_rssi);
	char * payload = (char *)  packetbuf_dataptr();
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
static const struct runicast_callbacks runicast_callbacks = {recv_runicast,
							     sent_runicast,
							     timedout_runicast};
static struct runicast_conn runicast;
/*---------------------------------------------------------------------------*/

static void
broadcast_recv(struct broadcast_conn *c, const rimeaddr_t *from)
{
	// Check if I have a parent and if yes I open a unicast and send response
	//rimeaddr_t empty;
	if(parent.addr.u8[0] != 0){
		printf("I have a parent and received broadcast");
		rimeaddr_t recv;
		char *nb_hop;
		sprintf(nb_hop, "%d", me.nb_hops);
		packetbuf_copyfrom(nb_hop, sizeof(nb_hop));
      		recv.u8[0] = from->u8[0];
      		recv.u8[1] = from->u8[1];
		runicast_send(&runicast, &recv, MAX_RETRANSMISSIONS);
	}
}

static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;

/*---------------------------------------------------------------------------*/

// void 	broadcast_open (struct broadcast_conn *c, uint16_t channel, const struct broadcast_callbacks *u)
// void 	broadcast_close (struct broadcast_conn *c)
// int 		broadcast_send (struct broadcast_conn *c)

// void 	runicast_open(struct runicast_conn *c, uint16_t channel, const struct runicast_callbacks *u)
// void		runicast_close(struct runicast_conn *c)
// int 		runicast_send(struct runicast_conn *c, const rimeaddr_t *receiver, uint8_t max_retransmissions)


PROCESS_THREAD(sensor_network_process, ev, data)
{
	// 
	
	PROCESS_EXITHANDLER(broadcast_close(&broadcast);)
	PROCESS_EXITHANDLER(runicast_close(&runicast);)
	PROCESS_BEGIN();

	parent.addr.u8[0] = 0;
	parent.addr.u8[1] = 0;
	parent.rssi = -65534;
	parent.nb_hops = 254;	

	me.addr.u8[0] = rimeaddr_node_addr.u8[0];
	me.addr.u8[1] = rimeaddr_node_addr.u8[1];
	

	runicast_open(&runicast, 144, &runicast_callbacks);

	static struct etimer et;

	broadcast_open(&broadcast, 129, &broadcast_call);

	BROADCAST:while(parent.addr.u8[0] == 0  && parent.addr.u8[1] == 0) {
		printf("Broadcasting\n");
		/* Delay 2-4 seconds */
		etimer_set(&et, CLOCK_SECOND * 4 + random_rand() % (CLOCK_SECOND * 4));

		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
		
		// Send packet looking for a parent with your id
		broadcast_send(&broadcast);
		//printf("broadcast message sent\n");
	}
	printf("Stopped Broadcasting\n");

	while(parent.addr.u8[0] != 0)
	{
		/* Delay 2-4 seconds */
		/*etimer_set(&et, CLOCK_SECOND * 4 + random_rand() % (CLOCK_SECOND * 4));

		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
		
		rimeaddr_t recv;
		packetbuf_copyfrom("254", sizeof("254"));
      		recv.u8[0] = parent.addr.u8[0];
      		recv.u8[1] = parent.addr.u8[1];
		runicast_send(&runicast, &recv, MAX_RETRANSMISSIONS);*/
		
	}
	
	
	goto BROADCAST;

	PROCESS_END();	
}
