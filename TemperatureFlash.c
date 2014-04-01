/*
 * Takes measurements from Temperature, Humidity, Visible and IR Light
 * and saves them to flash memory.
 * 
 * George Smart.
 * Fri/12/Oct/2012
 * 
 * Copyright 2012 George Smart <george@george-smart.co.uk>
 * 
 * MIT Licence. See LICENCE file.
 * 
 */
 
 /*
 * LIGHT_SENSOR_PHOTOSYNTHETIC:		L=RAW*10/7
 * LIGHT_SENSOR_TOTAL_SOLAR:		L=RAW*10/7
 * SHT11_SENSOR_TEMP:				T=RAW*0.01-39.6
 * SHT11_SENSOR_HUMIDITY:			H=0.0405*RAW-0.0000028*(RAW*RAW)-4
 * SENSOR_BATTERY:					V=RAW/4095*VREF*2 (VREF=2.5) 
 */

#include <stdio.h>
#include <stdlib.h>
#include <io.h>  //change this to io.h in older contiki builds
#include "contiki.h"
#include "dev/leds.h"
#include "cfs/cfs.h" 
#include "dev/button-sensor.h"

#include "dev/light-sensor.h"
#include "dev/sht11-sensor.h"


// Debugging Drivers for Expansion Port Scope Triggering
#define P23_OUT() P2DIR |= BV(3)
#define P23_IN() P2DIR &= ~BV(3)
#define P23_SEL() P2SEL &= ~BV(3)
#define P23_IS_1  (P2OUT & BV(3))
#define P23_WAIT_FOR_1() do{}while (!P23_IS_1)
#define P23_IS_0  (P2OUT & ~BV(3))
#define P23_WAIT_FOR_0() do{}while (!P23_IS_0)
#define P23_1() P2OUT |= BV(3)
#define P23_0() P2OUT &= ~BV(3)


const unsigned int FlashInterval = 600;  // in seconds.


// File stuff
char buffer[1000];
char *filename = "msg_file";
int fd_write, fd_read, n;

uint16_t id = 0;

// A few definitions
#define FALSE	0
#define TRUE	!FALSE


// Timer
static struct ctimer timer;


// Initialise Process Threads
PROCESS(main_process_thread, "Main Thread");
PROCESS(format_cfs_on_button, "CFS Format Thread Button Watcher");


// Start required threads.
AUTOSTART_PROCESSES(&main_process_thread, &format_cfs_on_button);

static void 
MakeMeasurement(void)
{
	ctimer_reset(&timer);  // Reset the timer
	P23_1(); // debugger scope trigger
	
	uint16_t light1;
	uint16_t light2;
	uint16_t temperature;
	uint16_t humidity;
	int n, m;
	leds_on(LEDS_BLUE);
	temperature = sht11_sensor.value(SHT11_SENSOR_TEMP);
	humidity = sht11_sensor.value(SHT11_SENSOR_HUMIDITY);
	light1 = light_sensor.value(LIGHT_SENSOR_PHOTOSYNTHETIC);
	light2 = light_sensor.value(LIGHT_SENSOR_TOTAL_SOLAR);
	n = sprintf (buffer, "%u,%u,%u,%u\n", light1, light2, temperature, humidity);
	m = cfs_write(fd_write, buffer, n);  // Write starter bytes
	printf("%d/%d      %s", n, m, buffer);
	id++;
	leds_off(LEDS_BLUE);
	
	P23_0(); // debugger scope trigger
}

PROCESS_THREAD(main_process_thread, ev, data)
{	
	PROCESS_BEGIN();
	SENSORS_ACTIVATE(light_sensor);
	SENSORS_ACTIVATE(sht11_sensor);
	SENSORS_ACTIVATE(button_sensor);
	
	P23_OUT();
	P23_SEL();
	
	//NETSTACK_MAC.off(0);
	
	leds_on(LEDS_GREEN);
	
	printf("Output Data:\n");

	fd_read = cfs_open(filename, CFS_READ);
	if(fd_read!=-1) {
		while (cfs_read(fd_read, buffer, 1) > 0) {
			printf("%s", buffer);
		}
		cfs_close(fd_read);
	} else {
		printf("ERROR: could not read from memory.\n");
		//return (-1);
	}

	printf("*** END OF DATA ***\n\n");
	leds_off(LEDS_GREEN);
	
	// Try to open the file.
	fd_write = cfs_open(filename, CFS_WRITE | CFS_APPEND);
	if(fd_write != -1) {
		printf("Opened File: %s in Flash\n", filename);
		MakeMeasurement(); // make the first measurement!
		ctimer_set(&timer, (FlashInterval*CLOCK_SECOND), MakeMeasurement, NULL);
	} else {
		printf("Cannot open %s in Flash. Giving up.\n", filename);
		return(-1);
	}
	
	//cfs_close(fd_write);
	
	PROCESS_END();
}

PROCESS_THREAD(format_cfs_on_button, ev, data)
{
	PROCESS_BEGIN();
	while(1) {
		PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);
		leds_on(LEDS_RED);
		printf("Formatting Flash\n");
		cfs_coffee_format();
		printf("Rebooting MCU........\n\n\n");
		watchdog_reboot(); // reboot the MCU.
		leds_off(LEDS_RED);	
	}
	PROCESS_END();
}

