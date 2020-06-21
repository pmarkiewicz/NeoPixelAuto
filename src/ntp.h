#ifndef _NTP__H_
#define _NTP__H_

uint32_t getTime(WiFiUDP& UDP);
bool sendNTPpacket(WiFiUDP& UDP);

const int UDP_PORT = 123;

#endif
