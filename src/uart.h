
/*
 * Uart baudrate = fosc / (16 * (UBRR + 1)) [p.133] 
 *
 * UBRR = (fosc / baudrate) / 16 - 1
 *
 * These constants only apply when the xtal oscillator is 11.0592 Mhz !
 */

#include <stdint.h>

#define UART_BAUD(baudrate)     ((F_CPU / baudrate) / 16 - 1)

void uart_init(uint16_t baudrate_divider);
void uart_enable(void);
void uart_disable(void);
void uart_tx(uint8_t c);
void uart_isr_1000hz();
uint8_t uart_rx(void);

/*
 * End
 */
