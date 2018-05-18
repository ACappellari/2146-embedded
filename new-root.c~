#include <stdio.h>

#include "contiki.h"
#include "net/rime.h"
#include "random.h"

#include "lib/list.h"
#include "lib/memb.h"

#include "dev/button-sensor.h"
#include "dev/leds.h"
#include <stdio.h>

#define MAX_RETRANSMISSIONS 8
#define NUM_HISTORY_ENTRIES 4

/*---------------------------------------------------------------------------*/
//PROCESS(sensor_network_process, "Sensor network implementation");
//AUTOSTART_PROCESSES(&sensor_network_process);

/* We first declare our two processes. */
PROCESS(broadcast_process, "Broadcast process");
PROCESS(unicast_process, "Unicast process");

/* The AUTOSTART_PROCESSES() definition specifices what processes to
   start when this module is loaded. We put both our processes
   there. */
AUTOSTART_PROCESSES(&broadcast_process, &unicast_process);

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

static node me;

// Unicast connection for routing
static struct runicast_conn runicast;
// Unicast connection for sensor data
static struct runicast_conn sensor_runicast;

/*---------------------------------------------------------------------------*/
static void
recv_runicast(struct runicast_conn *c, const rimeaddr_t *from, uint8_t seqno)
{
  printf("Root received runicast message received from %d.%d, seqno %d\n",
	 from->u8[0], from->u8[1], seqno);
  //Is the message received for the root node or myself ?
  uint16_t CHAN = packetbuf_attr(PACKETBUF_ATTR_CHANNEL);

  //If equal to 154, has to be forwarded to the ROOT node
  if (CHAN == 154){
  	char * s_payload = (char *) packetbuf_dataptr();
	printf("SENSOR PACKET RECEIVED IN ROOT: %s\n", s_payload);

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
/*---------------------------------------------------------------------------*/

static void
broadcast_recv(struct broadcast_conn *c, const rimeaddr_t *from)
{
	printf("I A M ROOT, broadcast message received from %d.%d: '%s'\n",
	from->u8[0], from->u8[1], (char *)packetbuf_dataptr());

	rimeaddr_t recv;
	packetbuf_copyfrom("0", 1);
	recv.u8[0] = from->u8[0];
	recv.u8[1] = from->u8[1];
	runicast_send(&runicast, &recv, MAX_RETRANSMISSIONS);

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

/*
PROCESS_THREAD(sensor_network_process, ev, data)
{
	// 
	
	PROCESS_EXITHANDLER(broadcast_close(&broadcast);)
	PROCESS_EXITHANDLER(runicast_close(&runicast);)
	PROCESS_EXITHANDLER(runicast_close(&sensor_runicast);)
	PROCESS_BEGIN();

	me.addr.u8[0] = rimeaddr_node_addr.u8[0];
	me.addr.u8[1] = rimeaddr_node_addr.u8[1];
	me.nb_hops = 0;

	runicast_open(&runicast, 144, &runicast_callbacks);
	runicast_open(&sensor_runicast, 154, &runicast_callbacks);

	static struct etimer et;


	broadcast_open(&broadcast, 129, &broadcast_call);

	

	PROCESS_END();	
}
*/

PROCESS_THREAD(broadcast_process, ev, data)
{
	PROCESS_EXITHANDLER(broadcast_close(&broadcast);)
	PROCESS_BEGIN();

	me.addr.u8[0] = rimeaddr_node_addr.u8[0];
	me.addr.u8[1] = rimeaddr_node_addr.u8[1];
	me.nb_hops = 0;

	broadcast_open(&broadcast, 129, &broadcast_call);
	while(1){
		PROCESS_YIELD();
	}

	PROCESS_END();	

}

PROCESS_THREAD(unicast_process, ev, data)
{
	PROCESS_EXITHANDLER(runicast_close(&runicast);)
	PROCESS_EXITHANDLER(runicast_close(&sensor_runicast);)
	PROCESS_BEGIN();

	runicast_open(&runicast, 144, &runicast_callbacks);
	runicast_open(&sensor_runicast, 154, &runicast_callbacks);

	while(1){
		PROCESS_YIELD();
	}

	PROCESS_END();	

}

