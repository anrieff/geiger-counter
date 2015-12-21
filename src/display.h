/*
	Title: Geiger Counter with Serial Data Reporting and display
	Description: Code for driving a 7-segment display
		
	(vesko)   8/1/15 1.50: Initial release of the upgraded version.
	(vesko) 12/21/15 2.00: Update to drive a "dumb" display directly

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
extern uint8_t display[4]; //!< the display masks, if you want to play with them directly.

// these are OR-able masks for display_set_dots()
enum {
    DP1 = 1,
    DP2 = 2,
    DP3 = 4,
    DP4 = 8,            // decimal points
};

/// turn on the display (the display will be initialized to four dots)
void display_turn_on(void);

/// turn off the display
void display_turn_off(void);

/// set the display brightness. 255 means full brightness, 128 is 50%, 
/// 1 is almost dark.
void display_set_raw_brightness(uint8_t value);

/// similar to display_set_raw_brightness, but the values are tanglible
/// numbers from 1-9, where 1 is almost dark, and 9 is 100% brighness.
/// And the grading 1..9 is logarithmic as well.
/// (input numbers outside 1-9 are clamped to 1 or 9).
void display_set_user_friendly_brightness(uint8_t user_value);

/// clears the display (and dots)
void display_clear(void);

/// sets the dots mask (DP1, DP2, DP3 & DP4, see the enum definition):
void display_set_dots(uint8_t mask);

/// displays an integer value on the display.
/// @param x - the value to be displayed.
/// @param dp - where to place the decimal point.
///             dp = 0: don't use a decimal point
///             dp = 1: format the number as XXX.X
///             dp = 2: format the number as XX.XX
///             dp = 3: format the number as X.XXX
///             
///             if dp > 0, if the number doesn't fit (x > 9999),
///             the number is autoscaled and point moved up to the
///             last possible place; i.e.
///             display_int_value( 1234, 2, 0xff) -> displays "12.34"
///             display_int_value(51234, 2, 0xff) -> displays "512.3"
///             if the decimal points goes "off scale", then the 
///             display is "-OL-" ("off limits"):
///             display_int_value(7651234, 2, 0xff) -> displays "-OL-"
/// @param dp_mask - masks the allowed dp values (i.e., only bits in
///                  dp_mask will be considered when displaying the
///                  dot mask; @see display_set_dots()).
void display_int_value(uint32_t x, int8_t dp, uint8_t dp_mask);

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

/// displays the "adjust brightness menu", where the user selects the
/// LED brightness levels (1-9). Selected value is activated after 5 seconds
/// of inactivity, and saved in EEPROM.
void display_brightness_menu(void);
