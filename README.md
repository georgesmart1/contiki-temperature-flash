contiki-temperature
===================
Two Contiki applications that takes measurements from Temperature, Humidity, Visible
and IR Light and Battery voltage on TelosB (Tmote Sky) motes. Depending on the program,
the measurements are either saved to flash memory with the CoffeeFS or sent via the RIME
Collect protcol to a base station.

In TemperatureFlash.c, the data is stored in CoffeeFS in the onboard flash and dumped out when the mote
boots.  When the user button on the mote is pressed, the storage is formatted
and the mote rebooted.

In TemperatureCollect.c, the data is transmitted via the RIME Collect protocol to a basestation,
and the output can be saved to a file something like: "make login | tee saved.csv".

Format is:
LIGHT_SENSOR_PHOTOSYNTHETIC, LIGHT_SENSOR_TOTAL_SOLAR, SHT11_SENSOR_TEMP, SHT11_SENSOR_HUMIDITY, SENSOR_BATTERY

Numeric Conversions:
 - LIGHT_SENSOR_PHOTOSYNTHETIC:		L=RAW*10/7
 - LIGHT_SENSOR_TOTAL_SOLAR:		L=RAW*10/7
 - SHT11_SENSOR_TEMP:				T=RAW*0.01-39.6
 - SHT11_SENSOR_HUMIDITY:			H=0.0405*RAW-0.0000028*(RAW*RAW)-4
 - SENSOR_BATTERY:					V=RAW/4095*VREF*2 (VREF=2.5)

George Smart, M1GEO george@george-smart.co.uk

Telecommunications Research Group Office, Room 804;
Roberts Buidling, Department of Electronic & Electrical Engineering;
University College London;
Malet Place, London, WC1E 7JE, United Kingdom.
