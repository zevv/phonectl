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


#define KEY_VOLUME_UP    0xe900
#define KEY_VOLUME_DOWN  0xea00
#define KEY_PREVIOUSSONG 0xb600
#define KEY_NEXTSONG     0xb500
#define KEY_PLAY_PAUSE   0xcd00
#define KEY_STOP         0xb700
#define KEY_FAST_FORWARD 0xb400
#define KEY_FORWARD      0xb300

#define SLEEP_TIME_PLAYING (15 * 60)
#define SLEEP_TIME_STOPPED ( 1 * 60)

enum mode {
   MODE_RUN,
   MODE_SLEEP,
   MODE_CHARGE,
};


const PROGMEM char usbHidReportDescriptor[USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] = {
  0x05, 0x0c, 0x09, 0x01, 0xa1, 0x01, 0x85, 0x03, 0x19, 0x00, 0x2a, 0x3c, 0x03,
  0x15, 0x00, 0x26, 0x3c, 0x03, 0x95, 0x01, 0x75, 0x10, 0x81, 0x00, 0xc0, 0x05,
  0x01, 0x09, 0x80, 0xa1, 0x01, 0x85, 0x02, 0x05, 0x01, 0x19, 0x81, 0x29, 0x83,
  0x15, 0x00, 0x25, 0x01, 0x95, 0x03, 0x75, 0x01, 0x81, 0x02, 0x95, 0x01, 0x75,
  0x05, 0x81, 0x01, 0xc0, 0x05, 0x01, 0x09, 0x06, 0xa1, 0x01, 0x85, 0x01, 0x05,
  0x07, 0x15, 0x00, 0x25, 0x01, 0x19, 0x00, 0x29, 0x77, 0x95, 0x78, 0x75, 0x01,
  0x81, 0x02, 0xc0
};

static uchar report[3];
static enum mode mode = MODE_RUN;
static uint16_t sleep_timer = 0;
static bool playing = false;


uchar	usbFunctionSetup(uchar data[8])
{
   usbRequest_t *rq = (void *)data;
   if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS) {
      if(rq->bRequest == USBRQ_HID_GET_REPORT) {
         return sizeof(report);
      } else if(rq->bRequest == USBRQ_HID_GET_IDLE) {
         return 1;
      } else if(rq->bRequest == USBRQ_HID_SET_IDLE) {
         return 0;
      }
   }
   return 0;
}

#define REP_Q_SIZE 8

struct rep_q {
   uint16_t data[REP_Q_SIZE];
   uint8_t head;
   uint8_t tail;
} rep_q;


void rep_push(uint16_t data)
{
   rep_q.data[rep_q.head] = data;
   rep_q.head = (rep_q.head + 1) % REP_Q_SIZE;
}


bool rep_q_pop(uint16_t *data)
{
   if(rep_q.head == rep_q.tail) return false;
   *data = rep_q.data[rep_q.tail];
   rep_q.tail = (rep_q.tail + 1) % REP_Q_SIZE;
   return true;
}


static void set_mode(enum mode m)
{
   mode = m;
   if(mode == MODE_RUN) {
      printf("mode hid\n");
      playing = false;
      usbpwr_set_cc(CC_PULLDOWN);
      usbpwr_connect(true);
      usbpwr_enable_vbus(false);
   }
   if(mode == MODE_CHARGE) {
      printf("mode charge\n");
      usbpwr_set_cc(CC_PULLUP);
      usbpwr_connect(false);
      usbpwr_enable_vbus(true);
   }
}


void handle_event(event_t *ev)
{
   static uint8_t blink = 0;
   blink ++;

   switch(ev->type) {
      case EV_TICK_1HZ:
         if(mode == MODE_SLEEP) {
            set_mode(MODE_CHARGE);
         }
         if(sleep_timer > 0) {
            sleep_timer--;
            if(sleep_timer == 0) {
               printf("sleep\n");
               rep_push(KEY_STOP);
               rep_push(0);
               set_mode(MODE_SLEEP);
            }
         }
         break;

      case EV_TICK_10HZ:
         if(mode == MODE_RUN) {
            if(playing) {
               led_set((blink & 8) == 8);
            } else {
               led_set(1);
            }
         }
         if(mode == MODE_CHARGE) {
            led_set(0);
         }
         break;

      case EV_TICK_100HZ:
         break;

      case EV_BUTTON:
         if(mode == MODE_CHARGE) {
            set_mode(MODE_RUN);
            return;
         }
         if(ev->button.id == BUTTON_ID_L_CW) {
            rep_push(KEY_VOLUME_UP);
            rep_push(0);
         }
         if(ev->button.id == BUTTON_ID_L_CCW) {
            rep_push(KEY_VOLUME_DOWN);
            rep_push(0);
         }
         if(ev->button.id == BUTTON_ID_R_PUSH) {
            playing = !playing;
            rep_push(KEY_STOP);
            rep_push(0);
            if(playing) {
               rep_push(KEY_PLAY_PAUSE);
               rep_push(0);
            }
         }
         if(ev->button.id == BUTTON_ID_R_CW) {
            if(playing) {
               rep_push(KEY_NEXTSONG);
               rep_push(0);
            }
         }
         if(ev->button.id == BUTTON_ID_R_CCW) {
            if(playing) {
               rep_push(KEY_PREVIOUSSONG);
               rep_push(0);
            }
         }
         sleep_timer = playing ? SLEEP_TIME_PLAYING : SLEEP_TIME_STOPPED;
         break;

      case EV_UART:
         printf("'%c'\n", ev->uart.c);
         if(ev->uart.c == '1') usbpwr_enable_vbus(true);
         if(ev->uart.c == '0') usbpwr_enable_vbus(false);
         if(ev->uart.c == 'u') usbpwr_set_cc(CC_PULLUP);
         if(ev->uart.c == 'd') usbpwr_set_cc(CC_PULLDOWN);
         if(ev->uart.c == 'n') usbpwr_set_cc(CC_NONE);
         if(ev->uart.c == 'c') set_mode(MODE_CHARGE);
         if(ev->uart.c == 'h') set_mode(MODE_RUN);
         break;

      default:
         break;
   }
}


int main(void)
{
   led_init();
   timer_init();
   uart_init(UART_BAUD(9600));
   button_init();
   usbpwr_init();
   wdt_enable(WDTO_2S);
   usbInit();

   sei();

   set_mode(MODE_RUN);

   for(;;) {

      event_t ev;
      bool avail = event_poll(&ev);
      if(avail) {
         handle_event(&ev);
      }

      if(usbInterruptIsReady()) {
         uint16_t data;
         if(rep_q_pop(&data)) {
            report[0] = 0x03;
            report[1] = data >> 8;
            report[2] = data & 0xff;
            printf(">> %02x %02x\n", report[0], report[1]);
            usbSetInterrupt(report, sizeof(report));
         }
      }

      wdt_reset();
      usbPoll();
      button_poll();
   }
   return 0;
}

// vi: ts=3 sw=3 et
