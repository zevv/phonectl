
#ifndef led_h
#define led_h

#include <stdbool.h>

void led_init(void);
void led_set(uint8_t brightness);
void led_pwm(void);

#endif
