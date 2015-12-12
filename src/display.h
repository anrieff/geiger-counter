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

extern uint8_t display_on; //!< whether the display is turned on.

/// turn on the display (the display will be initialized to four dots)
void display_turn_on(void);

/// turn off the display
void display_turn_off(void);

/// set the display brightness. 255 means full brightness, 128 is 50%, 
/// 1 is almost dark.
void display_set_brightness(uint8_t value);

/// print radiation value on the display. The passed value is *100, i.e.,
/// 13.05 uSv/h is represented as 1305.
/// Values up to 99.99 uSv/h are represented exactly; 100-999 uSv/h are have
/// one decimal digit; 1mSv/h to 9 mSv/h are represented without a
/// fractional part; still more is represented as "outside limits" (-OL-)
void display_radiation(uint32_t uSv_mul_100);

/// print GM event count on the display. The value should not exceed 9999
/// (if it does, "-OL-" is displayed).
void display_counts(uint32_t counts);

/// shows the software revision (hardcoded in the .c file)
void display_show_revision(void);

/// function that handles display multiplexing. Should be called frequently,
/// more than 500 times per second.
void display_tasks(void);
