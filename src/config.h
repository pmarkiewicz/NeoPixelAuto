#ifndef _CONFIG__H_
#define _CONFIG__H_

struct config_struct
{
  char mqtt_server[40];
  uint16_t mqtt_port;
  char mqtt_username[40];
  char mqtt_password[40];
  uint8_t no_of_leds;
};

extern config_struct config;

void config_load();

#endif
