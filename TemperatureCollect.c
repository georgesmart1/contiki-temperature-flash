/*
 * Takes measurements from Temperature, Humidity, Visible and IR Light
 * and uses Contiki RIME "Collect" to send them to Base station.
 * 
 * George Smart.
 * Fri/12/Oct/2012
 * 
 * Copyright 2012 George Smart <george@george-smart.co.uk>
 * 
 * MIT Licence. See LICENCE file.
 * 
 * Based on the example-collect.c code by Adam Dunkels <adam@sics.se>
 * 
 */

#include "contiki.h"
#include "lib/random.h"
#include "net/rime.h"
#include "net/rime/collect.h"
#include "dev/leds.h"
#include "dev/button-sensor.h"
#include "dev/battery-sensor.h"
#include "dev/light-sensor.h"
#include "dev/sht11-sensor.h"

#include "net/netstack.h"

#include <stdio.h>

static struct collect_conn tc;
short unsigned int amSink = 0;

unsigned int TXInterval = 10; // seconds

static void 
MakeMeasurement(void)
{
	uint16_t light1;
	uint16_t light2;
	uint16_t temperature;
	uint16_t humidity;
	uint16_t battery;
	int n = 0;
	temperature = sht11_sensor.value(SHT11_SENSOR_TEMP);
	humidity = sht11_sensor.value(SHT11_SENSOR_HUMIDITY);
	light1 = light_sensor.value(LIGHT_SENSOR_PHOTOSYNTHETIC);
	light2 = light_sensor.value(LIGHT_SENSOR_TOTAL_SOLAR);
	battery = battery_sensor.value(0);
	
	// Copy string into transmitter buffer
	packetbuf_clear();
	n = sprintf (packetbuf_dataptr(), "%u,%u,%u,%u,%u", light1, light2, temperature, humidity, battery);
	packetbuf_set_datalen(n+1);
    collect_send(&tc, 15);
	printf("TX %d bytes: %u,%u,%u,%u,%u\n", n, light1, light2, temperature, humidity, battery);
}

/*---------------------------------------------------------------------------*/
PROCESS(example_collect_process, "Test collect process");
AUTOSTART_PROCESSES(&example_collect_process);
/*---------------------------------------------------------------------------*/
static void
recv(const rimeaddr_t *originator, uint8_t seqno, uint8_t hops)
{
	leds_on(LEDS_GREEN);
	printf("RESULT A%d.%d S%d H%d L%d : %s\n", originator->u8[0], originator->u8[1], seqno, hops, packetbuf_datalen(), (char *)packetbuf_dataptr());
	leds_off(LEDS_GREEN);
}
/*---------------------------------------------------------------------------*/
static const struct collect_callbacks callbacks = { recv };
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(example_collect_process, ev, data)
{
  static struct etimer periodic;
  //static struct etimer et;
  
  PROCESS_BEGIN();
  
  SENSORS_ACTIVATE(light_sensor);
  SENSORS_ACTIVATE(sht11_sensor);
  SENSORS_ACTIVATE(button_sensor);
  SENSORS_ACTIVATE(battery_sensor);

  collect_open(&tc, 130, COLLECT_ROUTER, &callbacks);

  if( (rimeaddr_node_addr.u8[0] == 1 && rimeaddr_node_addr.u8[1] == 0) || // node 1 in simulator
      (rimeaddr_node_addr.u8[0] == 130 && rimeaddr_node_addr.u8[1] == 67) ) { // node in hardware
	printf("I am sink\n");
	amSink = 1;
	leds_on(LEDS_BLUE);
	collect_set_sink(&tc, 1);	
	printf("Format: LIGHT_SENSOR_PHOTOSYNTHETIC, LIGHT_SENSOR_TOTAL_SOLAR, SHT11_SENSOR_TEMP, SHT11_SENSOR_HUMIDITY, SENSOR_BATTERY\n");
  }

  while(1) {

    /* Send a packet every 30 seconds. */
    if(etimer_expired(&periodic)) {
      etimer_set(&periodic, CLOCK_SECOND * TXInterval);
    }

    PROCESS_WAIT_EVENT();


    if(etimer_expired(&periodic)) {
      static rimeaddr_t oldparent;
      const rimeaddr_t *parent;

	  if (amSink == 0) {  // dont collect if you are sink
	    leds_on(LEDS_RED);
        printf("Sending\n");
        packetbuf_clear();
        MakeMeasurement();
	    leds_off(LEDS_RED);
      }
      
      parent = collect_parent(&tc);
      if(!rimeaddr_cmp(parent, &oldparent)) {
        if(!rimeaddr_cmp(&oldparent, &rimeaddr_null)) {
          printf("#L %d 0\n", oldparent.u8[0]);
        }
        if(!rimeaddr_cmp(parent, &rimeaddr_null)) {
          printf("#L %d 1\n", parent->u8[0]);
        }
        rimeaddr_copy(&oldparent, parent);
      }
    }

  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
