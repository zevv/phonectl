
#ifndef event_h
#define event_h

#include <stdint.h>
#include <stdbool.h>

#include "button.h"

enum evtype {
	EV_TICK_1HZ,
	EV_TICK_10HZ,
	EV_TICK_100HZ,
	EV_UART,
	EV_BUTTON,
};

struct ev_tick_1hz {
	enum evtype type;
};

struct ev_tick_10hz {
	enum evtype type;
};

struct ev_tick_100hz {
	enum evtype type;
};

struct ev_button {
	enum evtype type;
	uint8_t id;
};

struct ev_uart {
	enum evtype type;
	uint8_t c;
};

typedef union {
	enum evtype type;
	struct ev_tick_1hz tick_1hz;
	struct ev_tick_10hz tick_10hz;
	struct ev_uart uart;
	struct ev_button button;
} event_t;

void event_push(event_t *ev);
void event_wait(event_t *ev);
bool event_poll(event_t *ev);

#endif
