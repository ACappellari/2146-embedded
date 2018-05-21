/**
 * \file
 *         Root node implementation, for the "IOT network using Rime" project of the "LINGI2146 - Mobile and Embedded Computing"
 *         course. UCL 2017-2018.
 * \author
 *         Daubry Benjamin <benjamin.daubry@student.uclouvain.be>
 *         Dizier Romain <romain.dizier@student.uclouvain.be>
 *         Lovisetto Gary <gary.lovisetto@student.uclouvain.be>
 */

#include <stdio.h>
#include "contiki.h"
#include "contiki-net.h"
#include "net/rime.h"
#include "random.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "dev/uart0.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "dev/serial-line.h"
#include <stdio.h>

/* CONSTANTS */
#define MAX_RETRANSMISSIONS 16
#define NUM_HISTORY_ENTRIES 4
#define ACK_CHILD -42 //Value used to identify an aknowledgment from a child node
#define MAX_CHILDREN 10 //Maximum number of entries in the table of children per node
#define SERIAL_BUF_SIZE 128 //Size of the communication buffer with the gateway

/*---------------------------------------------------------------------------*/
PROCESS(sensor_network_process, "Sensor network implementation");
AUTOSTART_PROCESSES(&sensor_network_process);
/*---------------------------------------------------------------------------*/
			    /* Forward declaration */

//Structure used to model a routing packet
struct response_packet ; 
typedef struct response_packet
{
	uint8_t id;
	uint8_t nb_hops;
} response ;

//Structure used to model a node
struct node_unicast;
typedef struct node_unicast{
	rimeaddr_t addr;
	uint8_t nb_hops;
} node;

//Each node store its own values
node me;

//Structure used to model a child node
struct tuple_child;
typedef struct tuple_child{
	rimeaddr_t addr;
} tuple_child;

//Each node stores an array containing the address of its children
tuple_child children[MAX_CHILDREN];
static number_children = 0;

// Unicast connection for routing
static struct runicast_conn runicast;
// Unicast connection for sensor data
static struct runicast_conn sensor_runicast;
// Unicast connection for options
static struct runicast_conn options_runicast;

/*---------------------------------------------------------------------------*/
			/* CHILD NODE MANIPULATION */

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
  	printf("Root received runicast message received from %d.%d, seqno %d\n", from->u8[0], from->u8[1], seqno);

	char * payload = (char *) packetbuf_dataptr();
	printf("Payload received : %s\n", payload);
	int code = (int) atoi(payload);
	if(code == -42){
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
	else{
		printf("Error : unknown unicast message type\n");
	}

}

static void
sensor_recv_runicast(struct runicast_conn *c, const rimeaddr_t *from, uint8_t seqno)
{
  printf("Root received sensor data from %d.%d, seqno %d\n",
	 from->u8[0], from->u8[1], seqno);
  uint16_t CHAN = packetbuf_attr(PACKETBUF_ATTR_CHANNEL);
  char * s_payload = (char *) packetbuf_dataptr();
  printf("SENSOR PACKET RECEIVED : %s\n", s_payload);

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

	//If the node lost is a child node
	if(isChild > 0){
		removeChild(isChild);
	}
	else{
		printf("Error, unicast message sent to unknown node timed out\n");
	}
}

/*---------------------------------------------------------------------------*/
			  /* UNICAST OPTIONS PACKETS */

static void
options_recv_runicast(struct runicast_conn *c, const rimeaddr_t *from, uint8_t seqno)
{
  printf("runicast message received from %d.%d, seqno %d\n",
	 from->u8[0], from->u8[1], seqno);
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
							     	    sent_runicast,
							            timedout_runicast};

static const struct runicast_callbacks options_runicast_callbacks = {options_recv_runicast,
							     	     options_sent_runicast,
							             options_timedout_runicast};
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
static char rx_buf[SERIAL_BUF_SIZE]; 
static int rx_buf_index; 
static void uart_rx_callback(unsigned char c) { 
  rx_buf[rx_buf_index] = c;      

  if(c == '\n' || c == EOF){ 
   printf("received line: %s", (char *)rx_buf);
   packetbuf_clear();
   rx_buf[strcspn ( rx_buf, "\n" )] = '\0';
   packetbuf_copyfrom(rx_buf, strlen(rx_buf));
   //Send the option to all the child nodes
   int j;
   for(j = 0; j < number_children; j++) {
		runicast_send(&options_runicast, &children[j].addr, MAX_RETRANSMISSIONS);
   }

   memset(rx_buf, 0, rx_buf_index); 
   rx_buf_index = 0; 
  }else{ 
    rx_buf_index = rx_buf_index + 1; 
  } 
}
/*---------------------------------------------------------------------------*/
			/* PROCESS THEAD */

PROCESS_THREAD(sensor_network_process, ev, data)
{
	
	PROCESS_EXITHANDLER(broadcast_close(&broadcast);)
	PROCESS_EXITHANDLER(runicast_close(&runicast);)
	PROCESS_EXITHANDLER(runicast_close(&sensor_runicast);)
	PROCESS_EXITHANDLER(runicast_close(&options_runicast);)
	PROCESS_BEGIN();

	me.addr.u8[0] = rimeaddr_node_addr.u8[0];
	me.addr.u8[1] = rimeaddr_node_addr.u8[1];
	me.nb_hops = 0;

	runicast_open(&runicast, 144, &runicast_callbacks);
	runicast_open(&sensor_runicast, 154, &sensor_runicast_callbacks);
	runicast_open(&options_runicast, 164, &options_runicast_callbacks);

	static struct etimer et;

	broadcast_open(&broadcast, 129, &broadcast_call);

	uart0_init(BAUD2UBR(115200)); //set the baud rate as necessary 
  	uart0_set_input(uart_rx_callback); //set the callback function for serial input

	for(;;){
		PROCESS_WAIT_EVENT();
		if(ev == serial_line_event_message){
			printf("Serial message read\n");
		}
	}

		
	PROCESS_END();	
}
