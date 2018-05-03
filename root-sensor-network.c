#include <stdio.h>

#include "contiki.h"
#include "contiki-net.h"
#include "net/rime.h"
#include "random.h"

#include "lib/list.h"
#include "lib/memb.h"

#include "dev/button-sensor.h"
#include "dev/leds.h"
#include <stdio.h>
#include "dev/uart0.h"

#define MAX_RETRANSMISSIONS 4
#define NUM_HISTORY_ENTRIES 4

#define SERIAL_BUF_SIZE 128

/*---------------------------------------------------------------------------*/
PROCESS(sensor_network_process, "Sensor network implementation");
AUTOSTART_PROCESSES(&sensor_network_process);
/*---------------------------------------------------------------------------*/
static char rx_buf[SERIAL_BUF_SIZE];
static int rx_buf_index;
static void uart_rx_callback(unsigned char c) {
	
	rx_buf[rx_buf_index] = c;     
	if(c == '\n' || c == EOF){
		printf("%s\n",rx_buf);
		memset(rx_buf, 0, rx_buf_index);
		rx_buf_index = 0;
	}else{
		rx_buf_index = rx_buf_index + 1;
	}
}
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

node me;
/*---------------------------------------------------------------------------*/
static void
recv_runicast(struct runicast_conn *c, const rimeaddr_t *from, uint8_t seqno)
{
  printf("Root received runicast message received from %d.%d, seqno %d\n",
	 from->u8[0], from->u8[1], seqno);

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


PROCESS_THREAD(sensor_network_process, ev, data)
{
	// 
	
	PROCESS_EXITHANDLER(broadcast_close(&broadcast);)
	PROCESS_EXITHANDLER(runicast_close(&runicast);)
	PROCESS_BEGIN();

	me.addr.u8[0] = rimeaddr_node_addr.u8[0];
	me.addr.u8[1] = rimeaddr_node_addr.u8[1];
	me.nb_hops = 0;

	runicast_open(&runicast, 144, &runicast_callbacks);
	broadcast_open(&broadcast, 129, &broadcast_call);

	uart0_init(BAUD2UBR(115200)); //set the baud rate as necessary
	uart0_set_input(uart_rx_callback); //set the callback function

	PROCESS_END();	
}
