
COMPILE = avr-gcc -Wall -Werror -Os -Iusbdrv -I. -mmcu=atmega328p -DF_CPU=16000000L #-DDEBUG_LEVEL=1

OBJECTS += main.o
OBJECTS += uart.o
OBJECTS += event.o
OBJECTS += timer.o
OBJECTS += led.o
OBJECTS += usbpwr.o
OBJECTS += button.o
OBJECTS += usbdrv/usbdrv.o
OBJECTS += usbdrv/usbdrvasm.o
OBJECTS += usbdrv/oddebug.o 

ADFLAGS += -p m328p -c avrispv2 -P usb

all:	main.hex

.c.o:
	$(COMPILE) -c $< -o $@

.S.o:
	$(COMPILE) -x assembler-with-cpp -c $< -o $@

.c.s:
	$(COMPILE) -S $< -o $@

install: all
	avrdude $(ADFLAGS) -e -V -U flash:w:main.hex:i


clean:
	rm -f main.hex main.lst main.obj main.cof main.list main.map main.eep.hex main.bin *.o usbdrv/*.o main.s usbdrv/oddebug.s usbdrv/usbdrv.s

main.bin:	$(OBJECTS)
	$(COMPILE) -o main.bin $(OBJECTS)

main.hex:	main.bin
	rm -f main.hex main.eep.hex
	avr-objcopy -j .text -j .data -O ihex main.bin main.hex


