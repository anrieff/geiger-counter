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

/// turn on the display (the display will be initialized to four dots)
void display_turn_on(void);

//!< turn off the display
void display_turn_off(void);

//!< Display a value in units of 10^-8 Sieverts (value == 100 equals 1 uSv)
void display_write_value(uint32_t value);