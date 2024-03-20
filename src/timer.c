#include <avr/io.h>
#include <avr/interrupt.h>

#include "timer.h"
#include "event.h"
#include "led.h"

uint16_t jiffies = 0;



void timer_init(void)
{
	/* Fast PWM mode, enable interrupt on overflow */
	TCCR0A = (1<<COM0B1) | (1<<WGM01) | (1<<WGM00);
	TCCR0B = (1<<CS01) | (1<<CS00);
	TIMSK0 |= (1<<TOIE0);
}


/*
 * Timer0 overflow interrupt @ 1Khz
 */

ISR(TIMER0_OVF_vect)
{
	static uint8_t t100 = 0;
	static uint8_t t10 = 0;
	static uint8_t t1 = 0;

	led_pwm();

	TCNT0 = 0x6; // 16Mhz / 64 / (256 - 6) = 1Khz
		
	cli();
	jiffies ++;
	sei();

	event_t ev;

	if(t100-- == 0) {
		ev.type = EV_TICK_100HZ;
		event_push(&ev);

		t100 = 9;

		if(t10-- == 0) {

			ev.type = EV_TICK_10HZ;
			event_push(&ev);

			t10 = 9;

			if(t1-- == 0) {
				ev.type = EV_TICK_1HZ;
				event_push(&ev);
				t1 = 9;
			}
		}
	}
}


/*
 * End
 */


