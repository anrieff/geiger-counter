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

#include <avr/io.h>			// this contains the AVR IO port definitions
#include <avr/pgmspace.h>	// tools used to store variables in program memory
#include <avr/sleep.h>		// sleep mode utilities
#include <util/delay.h>		// some convenient delay functions
#include <stdlib.h>
#include "pinout.h"
#include "characters.h"

// constants:
#define NVRAM_DELAY 2 // milliseconds

uint8_t display_on = 0;
uint8_t dots;
uint8_t display[4];
uint8_t brightness;

 // these are OR-able masks for display_set_dots()
enum {
    DP1 = 1,
    DP2 = 2,
    DP3 = 4,
    DP4 = 8,            // decimal points
};


void display_turn_off(void)
{
	display_on = 0;
	GFET_PORT &= ~_BV(GFET_BIT);
}

static void display_set_dots(uint8_t mask)
{
	dots = mask;
}

void display_set_brightness(uint8_t value)
{
	// setup brightness:
	brightness = value;
	_delay_ms(NVRAM_DELAY);
}

void display_turn_on(void)
{
	if (display_on) return;
	display_on = 1;

	GFET_PORT |= _BV(GFET_BIT);

	// run at 100% brightness:
	display_set_brightness(255);
	display[0] = display[1] = display[2] = display[3] = 0;
	// display four dots:
	display_set_dots(DP1 | DP2 | DP3 | DP4);
}

extern char serbuf[11];
const uint8_t DIGIT_MASKS[10] = { num0, num1, num2, num3, num4, num5, num6, num7, num8, num9 };

static void display_int_value(uint32_t x, int8_t dp)
{
	uint8_t i;

	if (x < 1000) {
		ultoa(x + 1000, serbuf, 10);
		serbuf[0] = ' ';
		if (dp == 0) {
			if (serbuf[1] == '0') {
				serbuf[1] = ' ';
				if (serbuf[2] == '0')
					serbuf[2] = ' ';
			}
		}		
	} else {
		ultoa(x, serbuf, 10);
	}
	char* s = serbuf + 4;
	while (*(s++)) dp--;
	if (dp < 0) {
		display[0] = cDASH; // '-'
		display[1] = cO;    // 'O'
		display[2] = cL;    // 'L'
		display[3] = cDASH; // '-'
	}
	for (i = 0; i < 4; i++)
		display[i] = (serbuf[i] == ' ') ? mEMPTY : DIGIT_MASKS[serbuf[i] - '0'];
	display_set_dots((8 >> dp) & (DP2|DP3));
}

void display_radiation(uint32_t uSv_mul_100)
{
	display_int_value(uSv_mul_100, 2);
}

void display_counts(uint32_t counts)
{
	display_int_value(counts, 0);
}

void display_show_revision(void)
{
	display[0] = cR;    // 'r'
	display[1] = num3;  // '3'
	display[2] = num4;  // '4'
	display[3] = num2;  // '2'
	display_set_dots(DP1); // "r.342"
}

void display_tasks(void)
{
	static uint8_t digit;
	DISP_BLANK();
	digit = (digit + 1) & 3;
	uint8_t ch = display[digit];
	uint8_t dot = (dots >> digit) & 1;
	DISP_OUT_DP(dot);
	DISP_OUT_CHAR(ch);
	DISP_ACTIVE_DIGIT(digit);
}
