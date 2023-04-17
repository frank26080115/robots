ESP32 RC Radio
==============

This is a simple way to implement an unidirectional stateless connectionless data pipe between two ESP32. It is meant to be a simple way to implement a RC transmitter and receiver pair, but capable of data formats that are more complex than simple servo pulses.

Features:

 * frequency hopping
 * low latency
 * high enough data rate
 * limited way of receiving replies (rate limited to save battery)
 * slow but robust text messaging

This implementation abuses the esp_wifi_80211_tx function to send raw 802.11 packets, and the receiver uses promiscuous mode to sniff all packets, while filtering the ones we are interested in.

This is different from ESP-NOW because I need this to have frequency hopping, and ESP-NOW does not have frequency hopping
