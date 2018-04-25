CONTIKI_PROJECT = sensor-network
all: $(CONTIKI_PROJECT)

#UIP_CONF_IPV6=1

CONTIKI = /home/user/contiki
include $(CONTIKI)/Makefile.include
