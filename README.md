# esp-arduino-philipsPIRHUE


Used sketch: ESP8266 wifi configurator in Arduino lang.

PIR (movement) sensor should be attached to PIN0. At first boot application creates AP. User should connect to it and setup his wireless network SSID, password and put IP address of Philips HUE bridge. After reboot app will turn on selcted light (no. 14 in my case) when moement is detected and turn off when PIR sensor shows that there is no movement.
