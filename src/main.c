#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <avr/wdt.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "usbdrv.h"
#include "oddebug.h"
#include "event.h"
#include "uart.h"
#include "timer.h"
#include "led.h"
#include "button.h"
#include "usbpwr.h"


static void hardwareInit(void)
{
   led_init();
   timer_init();
   uart_init(UART_BAUD(9600));
   button_init();
   usbpwr_init();

   //usbDeviceDisconnect();
   _delay_ms(100);
   //usbDeviceConnect();

}

/* ------------------------------------------------------------------------- */

#define NUM_KEYS    17


/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */

static uchar    reportBuffer[2];    /* buffer for HID reports */
static uchar    idleRate;           /* in 4 ms units */

const PROGMEM char usbHidReportDescriptor[USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] = {   /* USB report descriptor */
   0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
   0x09, 0x06,                    // USAGE (Keyboard)
   0xa1, 0x01,                    // COLLECTION (Application)
   0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
   0x19, 0xe0,                    //   USAGE_MINIMUM (Keyboard LeftControl)
   0x29, 0xe7,                    //   USAGE_MAXIMUM (Keyboard Right GUI)
   0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
   0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
   0x75, 0x01,                    //   REPORT_SIZE (1)
   0x95, 0x08,                    //   REPORT_COUNT (8)
   0x81, 0x02,                    //   INPUT (Data,Var,Abs)
   0x95, 0x01,                    //   REPORT_COUNT (1)
   0x75, 0x08,                    //   REPORT_SIZE (8)
   0x25, 0x90,                    //   LOGICAL_MAXIMUM (101)
   0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))
   0x29, 0x90,                    //   USAGE_MAXIMUM (Keyboard Application)
   0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
   0xc0                           // END_COLLECTION
};
/* We use a simplifed keyboard report descriptor which does not support the
 * boot protocol. We don't allow setting status LEDs and we only allow one
 * simultaneous key press (except modifiers). We can therefore use short
 * 2 byte input reports.
 * The report descriptor has been created with usb.org's "HID Descriptor Tool"
 * which can be downloaded from http://www.usb.org/developers/hidpage/.
 * Redundant entries (such as LOGICAL_MINIMUM and USAGE_PAGE) have been omitted
 * for the second INPUT item.
 */

/* Keyboard usage values, see usb.org's HID-usage-tables document, chapter
 * 10 Keyboard/Keypad Page for more codes.
 */
#define MOD_CONTROL_LEFT    (1<<0)
#define MOD_SHIFT_LEFT      (1<<1)
#define MOD_ALT_LEFT        (1<<2)
#define MOD_GUI_LEFT        (1<<3)
#define MOD_CONTROL_RIGHT   (1<<4)
#define MOD_SHIFT_RIGHT     (1<<5)
#define MOD_ALT_RIGHT       (1<<6)
#define MOD_GUI_RIGHT       (1<<7)

#define KEY_A       4
#define KEY_B       5
#define KEY_C       6
#define KEY_D       7
#define KEY_E       8
#define KEY_F       9
#define KEY_G       10
#define KEY_H       11
#define KEY_I       12
#define KEY_J       13
#define KEY_K       14
#define KEY_L       15
#define KEY_M       16
#define KEY_N       17
#define KEY_O       18
#define KEY_P       19
#define KEY_Q       20
#define KEY_R       21
#define KEY_S       22
#define KEY_T       23
#define KEY_U       24
#define KEY_V       25
#define KEY_W       26
#define KEY_X       27
#define KEY_Y       28
#define KEY_Z       29
#define KEY_1       30
#define KEY_2       31
#define KEY_3       32
#define KEY_4       33
#define KEY_5       34
#define KEY_6       35
#define KEY_7       36
#define KEY_8       37
#define KEY_9       38
#define KEY_0       39

#define KEY_F1      58
#define KEY_F2      59
#define KEY_F3      60
#define KEY_F4      61
#define KEY_F5      62
#define KEY_F6      63
#define KEY_F7      64
#define KEY_F8      65
#define KEY_F9      66
#define KEY_F10     67
#define KEY_F11     68
#define KEY_F12     128

#define KEY_VOLUME_UP 0x80
#define KEY_VOLUME_DOWN 0x81
#define KEY_VOLUME_MUTE 0x82
#define KEY_MEDIA_NEXT 0x83
#define KEY_MEDIA_PREV 0x84
#define KEY_MEDIA_STOP 0x85
#define KEY_MEDIA_PLAY 0x86
#define KEY_MEDIA_PAUSE 0x87
#define KEY_MEDIA_MUTE 0x88


uchar	usbFunctionSetup(uchar data[8])
{
   usbRequest_t    *rq = (void *)data;

   printf("us\n");

   usbMsgPtr = reportBuffer;
   if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS) {
      /* class request type */
      if(rq->bRequest == USBRQ_HID_GET_REPORT) {
         /* wValue: ReportType (highbyte), ReportID (lowbyte) */
         printf("get report\n");
         /* we only have one report type, so don't look at wValue */
         return sizeof(reportBuffer);
      } else if(rq->bRequest == USBRQ_HID_GET_IDLE) {
         printf("get idle\n");
         usbMsgPtr = &idleRate;
         return 1;
      } else if(rq->bRequest == USBRQ_HID_SET_IDLE) {
         printf("set idle\n");
         idleRate = rq->wValue.bytes[1];
      }
   }else{
      /* no vendor specific requests implemented */
   }
   return 0;
}

#define REP_Q_SIZE 8

struct rep_q {
   uint16_t data[REP_Q_SIZE];
   uint8_t head;
   uint8_t tail;
} rep_q;


void rep_push(uint8_t mod, uint8_t key)
{
   rep_q.data[rep_q.head] = (mod << 8) | key;
   rep_q.head = (rep_q.head + 1) % REP_Q_SIZE;
}

bool rep_q_pop(uint8_t *mod, uint8_t *key)
{
   if(rep_q.head == rep_q.tail) return false;
   uint16_t data = rep_q.data[rep_q.tail];
   *mod = data >> 8;
   *key = data & 0xff;
   rep_q.tail = (rep_q.tail + 1) % REP_Q_SIZE;
   return true;
}

void handle_event(event_t *ev)
{
   static uint8_t b = 0;

   switch(ev->type) {
      case EV_TICK_1HZ:
         break;

      case EV_TICK_10HZ:
         break;

      case EV_TICK_100HZ:
         break;

      case EV_BUTTON:
         printf("button %d\n", ev->button.id);
         if(ev->button.id == BUTTON_ID_L_CW) {
            rep_push(0, KEY_VOLUME_UP);
            rep_push(0, 0);
         }
         if(ev->button.id == BUTTON_ID_L_CCW) {
            rep_push(0, KEY_VOLUME_DOWN);
            rep_push(0, 0);
         }
         if(ev->button.id == BUTTON_ID_R_CW) {
            rep_push(0, KEY_MEDIA_NEXT);
            rep_push(0, 0);
         }
         if(ev->button.id == BUTTON_ID_R_CCW) {
            rep_push(0, KEY_MEDIA_PREV);
            rep_push(0, 0);
         }
         printf("%d\n", b);
         break;

      case EV_UART:
         printf("'%c'\n", ev->uart.c);
         if(ev->uart.c == '1') usbpwr_enable_vbus(true);
         if(ev->uart.c == '0') usbpwr_enable_vbus(false);
         if(ev->uart.c == 'u') usbpwr_set_cc(CC_PULLUP);
         if(ev->uart.c == 'd') usbpwr_set_cc(CC_PULLDOWN);
         if(ev->uart.c == 'n') usbpwr_set_cc(CC_NONE);
         if(ev->uart.c == 'c') usbpwr_connect(true);
         if(ev->uart.c == 'C') usbpwr_connect(false);
         if(ev->uart.c == 'k') {
            rep_push(0, KEY_I);
            rep_push(0, 0);
            //if(usbInterruptIsReady()) {
            //   printf("usb ready\n");
            //   reportBuffer[0] = MOD_SHIFT_LEFT;
            //   reportBuffer[1] = KEY_I;
            //   usbSetInterrupt(reportBuffer, sizeof(reportBuffer));
            //} else {
            //   printf("usb not ready\n");
            //}
         }
         break;

      default:
         break;
   }
}


int main(void)
{
   wdt_enable(WDTO_2S);
   hardwareInit();
   odDebugInit();
   usbInit();

   sei();
   
   usbpwr_set_cc(CC_PULLDOWN);
   usbpwr_connect(true);

   for(;;) {

      event_t ev;
      bool avail = event_poll(&ev);
      if(avail) {
         handle_event(&ev);
      }

      if(usbInterruptIsReady()) {
         uint8_t mod, key;
         if(rep_q_pop(&mod, &key)) {
            reportBuffer[0] = mod;
            reportBuffer[1] = key;
            usbSetInterrupt(reportBuffer, sizeof(reportBuffer));
         }
      }

      wdt_reset();
      usbPoll();
   }
   return 0;
}

// vi: ts=3 sw=3 et
