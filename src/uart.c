
#include <stdint.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "uart.h"
#include "event.h"

#define RB_SIZE 160
#define TX_DELAY_TICKS 3

struct ringbuffer_t {
	uint8_t head;
	uint8_t tail;
	uint8_t buf[RB_SIZE];
};

static int uart_putchar(char c, FILE *f);

static volatile struct ringbuffer_t rb = { 0, 0 };
static volatile uint16_t ticks = 0;

void uart_init(uint16_t baudrate_divider)
{
	UBRR0H = (baudrate_divider >> 8);
	UBRR0L = (baudrate_divider & 0xff);			
	UCSR0C = (1<<UCSZ01) | (1<<UCSZ00);
	UCSR0B = (1<<RXCIE0) | (1<<RXEN0) | (1<<TXEN0);	

	fdevopen(uart_putchar, NULL);

	rb.head = rb.tail = 0;
}


void uart_enable(void)
{
	UCSR0B = (1<<RXCIE0) | (1<<RXEN0) | (1<<TXEN0);	
}


void uart_disable(void)
{
	UCSR0B = 0;
}


ISR(USART_RX_vect)
{
	event_t ev;
	ev.type = EV_UART;
	ev.uart.c = UDR0;
	event_push(&ev);
}

/*
 * Write one character to the transmit ringbuffer and enable TX-reg-empty interrupt
 */ 
 
void uart_tx(uint8_t c)
{
	uint8_t head;
	head = (rb.head + 1) % RB_SIZE;

	if(head == rb.tail) rb.tail = (rb.tail + 1) % RB_SIZE;
	// while(rb.tail == head);

	rb.buf[rb.head] = c;
	rb.head = head;

	/* Enable TX-reg-empty interrupt */

	UCSR0B |= (1<<UDRIE0);

	/* Assert PD2 for RS-485 TX */

	PORTD |= (1<<2);
	ticks = TX_DELAY_TICKS;
}


static int uart_putchar(char c, FILE *f) 
{                     
    uart_tx(c); 
    return 0; 
}


ISR(USART_UDRE_vect)
{
	UDR0 = rb.buf[rb.tail];
	rb.tail = (rb.tail + 1) % RB_SIZE;
	
	if(rb.tail == rb.head) {
		UCSR0B &= ~(1<<UDRIE0);
	}

	ticks = TX_DELAY_TICKS;
}

/*
 * End
 */

