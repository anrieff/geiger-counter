# Name:			Makefile
# Author:		Jeff Keyzer
# Copyright:	2011 Jeff Keyzer, MightyOhm Engineering	
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

PROGRAM		= geiger
OBJECTS		= geiger.o
DEVICE		= attiny2313
CLOCK		= 8000000
PROGRAMMER	= usbtiny
PORT		= usb

# Fuse configuration:
# For a really nice guide to AVR fuses, see http://www.engbedded.com/fusecalc/
# LFUSE: SUT0, CKSEL0 (Ext Xtal 8+Mhz, 0ms startup time)
LFUSE		= 0xEE
# HFUSE: SPIEN, BODLEVEL0 (Serial programming enabled, Brownout = 1.8V
HFUSE		= 0xDD
# EFUSE: no fuses programmed
EFUSE		= 0xFF

# Tune the lines below only if you know what you are doing:

AVRDUDE = avrdude -c $(PROGRAMMER) -P $(PORT) -p $(DEVICE)
COMPILE = avr-gcc -g -Wall -Os -DF_CPU=$(CLOCK) -mmcu=$(DEVICE)

# Linker options
LDFLAGS	= -Wl,-Map=$(PROGRAM).map -Wl,--cref 

# Add size command so we can see how much space we are using on the target device.
SIZE	= avr-size -C --mcu=$(DEVICE)

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
	
%.elf: %.o
	$(COMPILE) -o $@ $< $(LDFLAGS)

%.o: %.c
	$(COMPILE) -c $< -o $@

# Targets for code debugging and analysis:
disasm:	$(PROGRAM).elf
	avr-objdump -h -S $(PROGRAM).elf > $(PROGRAM).lst

# Tell make that these targets don't correspond to actual files
.PHONY :	all $(PROGRAM) flash fuse install clean disasm
