#ifndef _CONFIG__H_
#define _CONFIG__H_

struct config_struct
{
  char mqtt_server[40];
  char mqtt_port[6];
  char mqtt_username[40];
  char mqtt_password[40];
};

extern config_struct config;

#endif
