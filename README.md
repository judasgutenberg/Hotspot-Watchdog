# Hotspot-Watchdog

This version of my ESP8266-based Micro-Weather system (https://github.com/judasgutenberg/ESP8266-Micro-Weather) does a couple things differently.

1. It uses the BME680 environment sensor to monitor temperature, pressure, humidity, and gas characteristics. I've had to alter the backend to log gas information.
2. It checks the network and if it cannot connect to its server or get a WiFi connection, it pulses line #D2 (digital line 14 for some reason) on the ESP8266.   Driving a relay that bridges the power button wires on the Moxee hotspot, this will reset the Moxee and make it connect.  This means I do not have to drive to my cabin to reset the hotspot every time it craps out or otherwise gets confused.


The Moxeee hotspot is easily disassembled if you have a tiny phillips-head screwdriver and perhaps some spudgers.  One of the six screws has a white paper tab on its head to tell Moxee that you've broken into their device, which you very much need to do to do what I did.

![alt text](moxee_inside_600.jpg?raw=true)

In this photo you will see the labeled power button and the two test pads I found that that carry the signals bridged by that button.  Solder wires onto those pads and run them out of your Moxee so you can simulate pressing the button. If you use the Arduino sketch to drive a relay attached to those two pads, your Moxee hotspot will be reset whenever it becomes uncommunicative, saving you hours of driving to your cabin.
