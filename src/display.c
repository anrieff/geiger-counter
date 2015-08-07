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
 */

#include <avr/io.h>			// this contains the AVR IO port definitions
#include <avr/pgmspace.h>	// tools used to store variables in program memory
#include <avr/sleep.h>		// sleep mode utilities
#include <util/delay.h>		// some convenient delay functions

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

void display_turn_on(void)
{
	if (display_on) return;
	display_on = 1;
	PORTB |= _BV(D_PWR);

	// wait for it to initialize:
	_delay_ms(255);
	// send a reset:
	display_spi_byte(0x76);
	_delay_ms(NVRAM_DELAY);
	// setup brightness:
	display_spi_byte(0x7a);
	display_spi_byte(0xfe - 192); // 75% brightness (smaller values mean dimmer display)
	_delay_ms(NVRAM_DELAY);
	// display four dots:
	display_set_dots(DP1 | DP2 | DP3 | DP4);
}

static void display_write_int_value(uint32_t value, uint8_t num_digits, uint8_t dot_mask)
{
	char d[4];
	int8_t i;
	for (i = 3; i >= 0; i--) {
		d[i] = value % 10 + '0';
		value /= 10;
	}
	for (i = 0; i < 3; i++) {
		if (d[i] == '0' && num_digits < 4 - i)
			d[i] = ' ';
		display_spi_byte(d[i]);
	}
	display_set_dots(dot_mask);
}

static void display_write_off_limits(void)
{
	display_spi_byte('-');
	display_spi_byte('O');
	display_spi_byte('L');
	display_spi_byte('-');
	display_set_dots(0);
}

void display_write_value(uint32_t value)
{
	if (!display_on) return;
	if (value <= 9999)
		display_write_int_value(value, 3, DP2);
	else if (value <= 99999)
		display_write_int_value(value / 10, 4, DP3);
	else if (value <= 999999)
		display_write_int_value(value / 100, 4, 0);
	else
		display_write_off_limits();
}
