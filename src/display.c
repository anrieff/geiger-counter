/*
	Title: Geiger Counter with Serial Data Reporting and display
	Description: Code for driving a 7-segment display
		
	(vesko)   8/1/15 1.50: Initial release of the upgraded version,
	                       driving a SparkFun "smart" display.
	(vesko) 12/21/15 2.00: Update to drive a "dumb" display directly.

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
#include <avr/eeprom.h>     // for read/write to EEPROM memory
#include <stdlib.h>
#include "pinout.h"
#include "characters.h"
#include "main.h"
#include "display.h"
#include "revision.h"

uint8_t display_on = 0;
uint8_t display[4];
uint8_t raw_brightness; // PWM values, 0-255
uint8_t user_brightness; // user-friendly values, 1-9.


void display_turn_off(void)
{
	display_on = 0;
	// disconnect global FET from Timer0:
	TCCR0A &= ~(_BV(COM0B0)|_BV(COM0B1));
	// set it to zero here:
	GFET_PORT &= ~_BV(GFET_BIT);
}

void display_set_dots(uint8_t mask)
{
	display[0] = (display[0] & 0x7f) | ((mask & 1) << 7);
	display[1] = (display[1] & 0x7f) | ((mask & 2) << 6);
	display[2] = (display[2] & 0x7f) | ((mask & 4) << 5);
	display[3] = (display[3] & 0x7f) | ((mask & 8) << 4);
}

void display_set_raw_brightness(uint8_t value)
{
	// setup brightness:
	raw_brightness = value;
	OCR0B = ((uint16_t) OCR0A * raw_brightness + 128) / 255;
}

void display_set_user_friendly_brightness(uint8_t value)
{
	static const uint8_t EXP_TABLE[10] = 
		{ 2, 4, 7, 12, 22, 40, 74, 138, 255 };

	if (value < 1) value = 1;
	if (value > 9) value = 9;
	user_brightness = value;
	display_set_raw_brightness(EXP_TABLE[user_brightness - 1]);
}

void display_turn_on(void)
{
	static uint8_t initialized = 0;
	if (display_on) return;

	display_on = 1;

	display[0] = display[1] = display[2] = display[3] = 0;
	// display four dots:
	display_set_dots(DP1 | DP2 | DP3 | DP4);

	// connect the FET control to Timer0:
	TCCR0A |= _BV(COM0B1);

	if (!initialized) {
		initialized = 1;
		// If we're doing this for the VERY first time, run at 100% brightness:
		uint8_t ee_value = eeprom_read_byte(0);
		if (ee_value < 1 || ee_value > 9) ee_value = 9;
		display_set_user_friendly_brightness(ee_value);
	}
}

int8_t display_is_on(void)
{
	return display_on;
}

void display_clear(void)
{
	display[0] = display[1] = display[2] = display[3] = 0;
}

extern char serbuf[11];
const uint8_t DIGIT_MASKS[10] = { num0, num1, num2, num3, num4, num5, num6, num7, num8, num9 };

void display_int_value(uint32_t x, int8_t dp, uint8_t dp_mask)
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
		display_set_dots(0);
		return;
	}
	for (i = 0; i < 4; i++)
		display[i] = (serbuf[i] == ' ') ? mEMPTY : DIGIT_MASKS[serbuf[i] - '0'];
	display_set_dots((8 >> dp) & dp_mask);
}

void display_radiation(uint32_t uSv_mul_100)
{
	display_int_value(uSv_mul_100, 2, (DP2|DP3));
}

void display_counts(uint32_t counts)
{
	if (counts <= 9999)
		display_int_value(counts, 0, 0);
	else
		display_int_value(counts / 10, 2, (DP2|DP3|DP4));
}

void display_show_revision(void)
{
	display_int_value(FIRMWARE_REVISION, 3, 1); // "  XXX"
	display[0] = cR | mDOT;                     // "r.XXX"
}

void display_tasks(void)
{
	static uint8_t digit;
	DISP_BLANK();
	digit = (digit + 1) & 3;

	// interleaved digit firing order. Using simple bit reversal, the
	// order is now 0-2-1-3:
	uint8_t xdigit = ((digit & 2) >> 1) | ((digit & 1) << 1);
	uint8_t ch = display[xdigit] & 0x7f;
	uint8_t dot = display[xdigit] >> 7;
	DISP_OUT_DP(dot);
	DISP_OUT_CHAR(ch);
	DISP_ACTIVE_DIGIT(xdigit);
}

void display_brightness_menu(void)
{
	enter_menu();

	display[0] = cB; // display "b...#", where '#' is 1-9
	display[1] = display[2] = mEMPTY;
	display[3] = DIGIT_MASKS[user_brightness];
	display_set_dots(DP1 | DP2 | DP3);

	// wait until the key is depressed:
	while (keypressed()) {
		_delay_ms(16);
		geiger_mini_mainloop();
	}
	uint16_t idle_counter = 0;
	uint8_t last_key_state = 0, key_state;
	while (1) {
		_delay_ms(16);
		geiger_mini_mainloop();
		idle_counter += 16;
		if (idle_counter > 5000) {
			// the brightness is deemed official. Write to EEPROM and get out of here!
			eeprom_write_byte(0, user_brightness);
			_delay_ms(NVRAM_DELAY);
			break;
		}
		key_state = keypressed();
		if (!key_state && last_key_state) {
			// key pressed and depressed; increment brightness and apply immediately:
			user_brightness++;
			if (user_brightness == 10) user_brightness = 1;
			display_set_user_friendly_brightness(user_brightness);

			// update screen:
			display[3] = DIGIT_MASKS[user_brightness];

			// reset idle counter:
			idle_counter = 0;
		}
		last_key_state = key_state;
	}
	leave_menu();
}
