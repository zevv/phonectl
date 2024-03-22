#ifndef usbpwr_h
#define usbpwr_h

#include <stdbool.h>

enum cc {
	CC_NONE,
	CC_PULLUP,
	CC_PULLDOWN,
};

void usbpwr_init();
void usbpwr_enable_vbus(bool on);
void usbpwr_set_cc(enum cc cc);
void usbpwr_connect(bool on);

#endif
