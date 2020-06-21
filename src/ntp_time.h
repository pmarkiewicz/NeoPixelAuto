#ifndef _NTP_TIME__H
#define _NTP_TIME__H

void ntp_time_init(unsigned long update_interval = 60000 * 60 * 24);
void ntp_time_update();
void get_time(unsigned char& h, unsigned char& m);

#endif