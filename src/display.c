/*
	Title: Geiger Counter with Serial Data Reporting and display
	Description: Code for driving a 7-segment "smart" display
		
	(vesko) 8/1/15 1.50: Initial release of the upgraded version.

		Copyright 2011 Jeff Keyzer, MightyOhm Engineering
		Copyright 2015 Veselin Georgiev, LVA Ltd.
 
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/* 
 * Pinout:
 * PB0 - power on/off
 * PD4 - SCL
 * PD5 - SDA
 * 
 * The display we're interfacing is the SparkFun serial display (COM-11442)
 * 
 * Header to display pinout (5-pin header, ordering from left to right 
 * if viewed from top of the PCB):
 *
 * +-----+---------------------------+--------------------------+ 
 * | pin | name                      | pin name on the display. |
 * +-----+---------------------------+--------------------------+
 * |   1 | Vdd                       | VCC                      |
 * |   2 | GND / also device select  | /SS                      |
 * |   3 | SCL                       | SCK                      |
 * |   4 | SDA                       | SDI                      |
 * |   5 | GND                       | GND                      |
 * +-----+---------------------------+--------------------------+
 * 
 */

#include <avr/io.h>			// this contains the AVR IO port definitions
#include <avr/pgmspace.h>	// tools used to store variables in program memory
#include <avr/sleep.h>		// sleep mode utilities
#include <util/delay.h>		// some convenient delay functions
#include <stdlib.h>

#define	F_CPU			8000000	// AVR clock speed in Hz

// pinout:
#define D_PWR 0
#define D_SCL 4
#define D_SDA 5

// constants:
#define NVRAM_DELAY 2 // milliseconds

uint8_t display_on = 0;

 // these are OR-able masks for display_set_dots()
enum {
    DP1 = 1,
    DP2 = 2,
    DP3 = 4,
    DP4 = 8,            // decimal points
    DIVIDED = 16,       // the central two dots
    APOSTROPHE = 32     // the apostrophe between characters 3 and 4
};


void display_turn_off(void)
{
	PORTD &= ~(_BV(D_SCL) | _BV(D_SDA));
	PORTB &= ~(_BV(D_PWR));
	display_on = 0;
}

static void display_spi_byte(uint8_t byte)
{
	uint8_t i;
	for (i = 0; i < 8; i++) {
		if (byte & 0x80)
			PORTD |=  (_BV(D_SDA));
		else
			PORTD &= ~(_BV(D_SDA));
		_delay_us(25);
		PORTD |=  (_BV(D_SCL));
		_delay_us(25);
		PORTD &= ~(_BV(D_SCL));
		byte <<= 1;
	}
}

static void display_set_dots(uint8_t mask)
{
	display_spi_byte(0x77);
	display_spi_byte(mask);
}

void display_set_brightness(uint8_t value)
{
	// setup brightness:
	display_spi_byte(0x7a);
	display_spi_byte(value);
	_delay_ms(NVRAM_DELAY);
}

void display_turn_on(void)
{
	if (display_on) return;
	display_on = 1;
	PORTB |= _BV(D_PWR);

	// wait for it to initialize:
	_delay_ms(200);
	// send a reset:
	display_spi_byte(0x76);
	_delay_ms(NVRAM_DELAY);
	// run at 100% brightness:
	display_set_brightness(0);
	// display four dots:
	display_set_dots(DP1 | DP2 | DP3 | DP4);
}

void display_int_value(uint32_t x, int8_t dp)
{
	char buff[12];
	uint8_t i;
	if (x < 1000) {
		utoa(x + 1000, buff, 10);
		buff[0] = ' ';
	} else {
		ultoa(x, buff, 10);
	}
	char* s = buff + 4;
	while (*(s++)) dp--;
	if (dp < 0) {
		buff[0] = '-';
		buff[1] = 'O';
		buff[2] = 'L';
		buff[3] = '-';
	}
	for (i = 0; i < 4; i++)
		display_spi_byte(buff[i]);
	display_set_dots((8 >> dp) & (DP2|DP3));
}

