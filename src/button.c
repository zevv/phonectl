#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>

#include "button.h"
#include "event.h"

#define R_START 0x0
#define R_CW_FINAL 0x1
#define R_CW_BEGIN 0x2
#define R_CW_NEXT 0x3
#define R_CCW_BEGIN 0x4
#define R_CCW_FINAL 0x5
#define R_CCW_NEXT 0x6

#define DIR_NONE 0x0
#define DIR_CW 0x10
#define DIR_CCW 0x20

const uint8_t ttable[7][4] = {
	{R_START,    R_CW_BEGIN,  R_CCW_BEGIN, R_START},
	{R_CW_NEXT,  R_START,     R_CW_FINAL,  R_START | DIR_CW},
	{R_CW_NEXT,  R_CW_BEGIN,  R_START,     R_START},
	{R_CW_NEXT,  R_CW_BEGIN,  R_CW_FINAL,  R_START},
	{R_CCW_NEXT, R_START,     R_CCW_BEGIN, R_START},
	{R_CCW_NEXT, R_CCW_FINAL, R_START,     R_START | DIR_CCW},
	{R_CCW_NEXT, R_CCW_FINAL, R_CCW_BEGIN, R_START},
};

static uint8_t state[2] = { R_START, R_START };


void button_init(void)
{
   // enable pullups
   PORTB |= (1<<PB1) | (1<<PB2);
   PORTD |= (1<<PD6) | (1<<PD7);
   // enable pin change interrupt on PB1, PB2, PB6 and PB7
   PCMSK0 |= (1<<PCINT1) | (1<<PCINT2);
   PCMSK2 |= (1<<PCINT22) | (1<<PCINT23);
   PCICR |= (1<<PCIE0) | (1<<PCIE2);
}


static uint8_t process(uint8_t idx, uint8_t pinstate)
{
	state[idx] = ttable[state[idx] & 0xf][pinstate];
	return state[idx] & 0x30;
}


ISR(PCINT0_vect)
{
	uint8_t b0 = !!(PINB & (1<<PB1));
	uint8_t b1 = !!(PINB & (1<<PB2));
	uint8_t dir = process(0, (b0 << 1) | (b1 << 0));
	if(dir != DIR_NONE) {
		event_t ev;
		ev.type = EV_BUTTON;
		ev.button.id = (dir == DIR_CW) ? BUTTON_ID_R_CW : BUTTON_ID_R_CCW;
		event_push(&ev);
	}
}


ISR(PCINT2_vect)
{
	uint8_t b0 = !!(PIND & (1<<PB6));
	uint8_t b1 = !!(PIND & (1<<PB7));
	uint8_t dir = process(1, (b0 << 1) | (b1 << 0));
	if(dir != DIR_NONE) {
		event_t ev;
		ev.type = EV_BUTTON;
		ev.button.id = (dir == DIR_CW) ? BUTTON_ID_L_CW : BUTTON_ID_L_CCW;
		event_push(&ev);
	}
}


