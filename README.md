# Hotspot-Watchdog

This version of my ESP8266-based Micro-Weather system does a couple things differently.

1. It uses the BME680 environment sensor to monitor temperature, pressure, humidity, and gas characteristics. I've had to alter the backend to log gas information.
2. It checks the network and if it cannot connect to its server or get a WiFi connection, it pulses line #D2 (digital line 14 for some reason) on the ESP8266.   Driving a relay that bridges the power button wires on the Moxee hotspot, this will reset the Moxee and make it connect.  This means I do not have to drive to my cabin to reset the hotspot every time it craps out or otherwise gets confused.
