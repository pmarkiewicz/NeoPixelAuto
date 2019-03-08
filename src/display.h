#ifndef _DISPLAY__H_
#define _DISPLAY__H_

void display_init();
void display_set_color(uint8_t r, uint8_t g, uint8_t b);

inline void display_red() { display_set_color(128, 0, 0); }
inline void display_yellow() { display_set_color(128, 128, 0); }
inline void display_green() { display_set_color(0, 128, 0); }
inline void display_off() { display_set_color(0, 0, 0); }

#endif
