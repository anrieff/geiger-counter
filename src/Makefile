# Name:			Makefile
# Authors:		Jeff Keyzer, Veselin Georgiev
# Copyright:	2011 Jeff Keyzer, MightyOhm Engineering	
#               2015, 2016 Veselin Georgiev, LVA Ltd.
#
# This Makefile is derived from the template that comes with CrossPack for the Mac,
# but with several improvements.

# Values you might need to change:
# In particular, check that your programmer is configured correctly.
#
# PROGRAM		The name of the "main" program file, without any suffix.
# OBJECTS		The object files created from your source files. This list is
#                usually the same as the list of source files with suffix ".o".
# DEVICE		The AVR device you are compiling for.
# CLOCK			Target AVR clock rate in Hz (eg. 8000000)
# PROGRAMMER	Programmer hardware used to flash program to target device.
# PORT			The peripheral port on the host PC that the programmer is connected to.	
# LFUSE			Target device configuration fuses, low byte.
# HFUSE			Targer device configuration fuses, high byte.
# EFUSE			Target device configuration fuses (extended).
#

PROGRAM		= geiger
OBJECTS		= \
	geiger.o  \
	display.o \
	battery.o \
	pc_link.o \
	logging.o \
	nvram_settings.o \
	alarms.o

DEVICE		= atmega88p
CLOCK		= 6000000
PROGRAMMER	= usbtiny
PORT		= usb

# Fuse configuration:
# For a really nice guide to AVR fuses, see http://www.engbedded.com/fusecalc/
# LFUSE: SUT0, CKSEL0, CKSEL1 (Ext Xtal 3-8Mhz, 0ms startup time)
LFUSE		= 0xEC
# HFUSE: SPIEN, BODLEVEL0 (Serial programming enabled, Brownout = 1.8V
HFUSE		= 0xDE
# EFUSE: no fuses programmed
EFUSE		= 0xFF

# Tune the lines below only if you know what you are doing:

AVRDUDE = avrdude -c $(PROGRAMMER) -P $(PORT) -p $(DEVICE)
COMPILE = avr-gcc -std=c99 -g -Wall -Os -DF_CPU=$(CLOCK) -mmcu=$(DEVICE)

# Linker options
LDFLAGS	= -Wl,-Map=$(PROGRAM).map -Wl,--cref 

# Add size command so we can see how much space we are using on the target device.
SIZE	= avr-size -A 

# symbolic targets:
all:	$(PROGRAM).hex
	$(SIZE) $(PROGRAM).elf

$(PROGRAM):	all	
	
flash: all
	$(AVRDUDE) -U flash:w:$(PROGRAM).hex:i

fuse:
	$(AVRDUDE) -U hfuse:w:$(HFUSE):m -U lfuse:w:$(LFUSE):m -U efuse:w:$(EFUSE):m
	
# Xcode uses the Makefile targets "", "clean" and "install"
install: flash fuse

clean:
	rm -f $(PROGRAM).hex $(PROGRAM).elf $(OBJECTS) $(PROGRAM).lst $(PROGRAM).map

# file targets:
%.hex: %.elf
	avr-objcopy -j .text -j .data -O ihex $< $@
	
geiger.elf: $(OBJECTS)
	$(COMPILE) -o $@ $(OBJECTS) $(LDFLAGS)

geiger.o: geiger.c display.h pinout.h main.h battery.h nvram_settings.h
	$(COMPILE) -c geiger.c -o $@

display.o: display.c display.h pinout.h characters.h main.h revision.h
	$(COMPILE) -c display.c -o $@

battery.o: display.c display.h battery.c pinout.h characters.h main.h
	$(COMPILE) -c battery.c -o $@

pc_link.o: pc_link.c pc_link.h main.h pinout.h revision.h
	$(COMPILE) -c pc_link.c -o $@

logging.o: logging.c logging.h main.h pinout.h
	$(COMPILE) -c logging.c -o $@

nvram_settings.o: nvram_settings.c nvram_settings.h nvram_map.h
	$(COMPILE) -c nvram_settings.c -o $@

alarms.o: alarms.c alarms.h nvram_settings.h pinout.h
	$(COMPILE) -c alarms.c -o $@

# Targets for code debugging and analysis:
disasm:	$(PROGRAM).elf
	avr-objdump -h -S $(PROGRAM).elf > $(PROGRAM).lst

sizes:
	readelf -s *.o | cut -c18- | grep FUNC | sort -n

# Tell make that these targets don't correspond to actual files
.PHONY :	all $(PROGRAM) flash fuse install clean disasm sizes
