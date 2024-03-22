
#include <avr/io.h>
#include <stdio.h>
#include "usbpwr.h"

void usbpwr_init()
{
	DDRB |= (1 << PB0);
}


void usbpwr_enable_vbus(bool on)
{
	if (on) {
        printf("vbus on\n");
		PORTB |= (1 << PB0);
	} else {
        printf("vbus off\n");
		PORTB &= ~(1 << PB0);
	}
}


void usbpwr_set_cc(enum cc cc)
{
    DDRC &= ~((1 << PC0) | (1 << PC1));
    PORTC &= ~((1 << PC0) | (1 << PC1));

	switch(cc) {
        case CC_NONE:
            printf("cc none\n");
            break;
        case CC_PULLUP:
            printf("cc pullup\n");
            DDRC |= (1 << PC0);
            PORTC |= (1 << PC0);
            break;
        case CC_PULLDOWN:
            printf("cc pulldown\n");
            DDRC |= (1 << PC1);
            PORTC &= ~(1 << PC1);
            break;
    }
}


void usbpwr_connect(bool on)
{
    if(on) {
        printf("connect\n");
        DDRC |= (1 << PC2);
        PORTC |= (1 << PC2);
    } else {
        printf("disconnect\n");
        DDRC &= ~(1 << PC2);
        PORTC &= ~(1 << PC2);
    }
}
