# ESP32-Antenna-Switch

This is the code to control a 4 way relay, via a web interface, which will then control a 1x4 antenna switch.

# Video Tutorial

You may find the corresponding video tutorial ony my Tech Minds YouTube channel, here: [https://www.youtube.com/embed/UB6Tlh_ZC4s](https://www.youtube.com/watch?v=UB6Tlh_ZC4s)

# Required Hardware

The ESP32 module used was a ESP32 WROOM 32, but I expect most WiFi enabled ES32 modules should work.

AT-14 4-Way Coax Remote Antenna Switch
https://ban.ggood.vip/1m4k9

I used the following ESP32 Wroom module, but I guess you could use any:
https://s.click.aliexpress.com/e/_oB9Pmex

Relay Board:
https://s.click.aliexpress.com/e/_omtcOEx

# Uploading 

I used the latest Aruino IDE application. Ensure you have the libraries used installed and the board type installed. Simply select the board type and compile/upload to ESP32.

This may vary depending on which ESP32 you use. Some prior knowledge of compiling for ESP32 is recommended.

# Configuration
Before uploading the ESP32 code, make sure to edit the follow lines:

Line 11. const int relayPins[4] = {5, 18, 19, 0};  //confgiure ESP32 GPIO pins (Ant1, Ant2, Ant3, Ant4)

Line 7. const char* ssid = "ENTER SSID HERE"; //Local wifi name

Line 8. const char* password = "ENTER PASSWORD HERE"; //Local wifi password

# Wiring

![wiring](https://github.com/user-attachments/assets/d6bb5ce0-72aa-4c37-b7e7-6d3047272573)
