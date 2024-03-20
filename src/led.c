
#include <avr/io.h>
#include <stdio.h>

#include "led.h"


static uint8_t brightness = 0;

void led_init(void)
{
   DDRD |= (1<<PD4);
}


void led_set(uint8_t b)
{
	brightness = b;
	if(brightness > 16) brightness = 16;
}

//   if(onoff)
//      PORTD |= (1<<PD4);
//   else
//      PORTD &= ~(1<<PD4);
//}


void led_pwm(void)
{
	static uint8_t cnt = 0;
	cnt = (cnt + 1) % 16;

	if(brightness == 0) {
		PORTD &= ~(1<<PD4);
	} else {
		if(cnt < brightness)
			PORTD |= (1<<PD4);
		else
			PORTD &= ~(1<<PD4);
	}
}

