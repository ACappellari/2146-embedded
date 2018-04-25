#include <stdio.h>

#include "contiki.h"
#include "net/rime.h"
#include "random.h"

#include "lib/list.h"
#include "lib/memb.h"

#include "dev/button-sensor.h"
#include "dev/leds.h"
#include <stdio.h>

#define MAX_RETRANSMISSIONS 4
#define NUM_HISTORY_ENTRIES 4

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
} node;

node parent;
parent->addr.u8[0] = 0;
parent.addr->u8[1] = 0;

node me;
me.addr.u8[0] = rimeaddr_node_addr.u8[0];
me->addr->u8[1] = rimeaddr_node_addr.u8[1];
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
static void
recv_runicast(struct runicast_conn *c, const rimeaddr_t *from, uint8_t seqno)
{
  printf("runicast message received from %d.%d, seqno %d\n",
	 from->u8[0], from->u8[1], seqno);

	//Received a response from my broadcast, should compare number of hops and override
	char * nbhop = (char *)  packetbuf_dataptr();
	uint8_t nbhops = (uint8_t) *nbhop;
	if(parent.nb_hops >  nbhops){
		parent.addr.u8[0] = from->u8[0];
		parent.addr.u8[1] = from->u8[1];		
		parent.nb_hops = nbhops;
		printf("Change parent to %d.%d with %d hop", parent.addr.u8[0],parent.addr.u8[1],parent.nb_hops);
		// 
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
}
static const struct runicast_callbacks runicast_callbacks = {recv_runicast,
							     sent_runicast,
							     timedout_runicast};
static struct runicast_conn runicast;
/*---------------------------------------------------------------------------*/

static void
broadcast_recv(struct broadcast_conn *c, const rimeaddr_t *from)
{
  printf("broadcast message received from %d.%d: '%s'\n",
         from->u8[0], from->u8[1], (char *)packetbuf_dataptr());

	// Check if I have a parent and if yes I open a unicast and send response
	//rimeaddr_t empty;
	if(parent.addr.u8[0] != 0){
		rimeaddr_t recv;
		char *nbhop;
		sprintf(nbhop, "%d", me.nb_hops);
		packetbuf_copyfrom(&nbhop, sizeof(nbhop));
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

static void
broadcast_while_no_parent()
{
	static struct etimer et;
	broadcast_open(&broadcast, 129, &broadcast_call);
	while(parent.addr.u8[0] == 0  && parent.addr.u8[1] == 0) {

		/* Delay 2-4 seconds */
		etimer_set(&et, CLOCK_SECOND * 4 + random_rand() % (CLOCK_SECOND * 4));

		//PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
		
		// Send packet looking for a parent with your id
		packetbuf_copyfrom("Hello", 6);
		broadcast_send(&broadcast);
		printf("broadcast message sent\n");
	}
	broadcast_close(&broadcast);
}

PROCESS_THREAD(sensor_network_process, ev, data)
{
	// 
	
	PROCESS_EXITHANDLER(broadcast_close(&broadcast);)
	PROCESS_EXITHANDLER(runicast_close(&runicast);)
	PROCESS_BEGIN();
	runicast_open(&runicast, 144, &runicast_callbacks);
	broadcast_while_no_parent();
	PROCESS_END();	
}
