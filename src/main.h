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

#ifndef __MAIN_H__
#define __MAIN_H__

#define COUNT_OF(arr) (sizeof(arr) / sizeof(arr[0]))

/*
 * This file declares functions which might be used by an external function
 * (e.g., a menu), so that it shows something different than radiation
 * data to the display. To do that, your code should look like the following:

void fancy_menu(void)
{
	// forbid the main loop of intercepting key events,
	// disable radiation data updates to the display:
	enter_menu(); 

	while (!menu_completed) {
	 	.. your code ...
	 	geiger_mini_mainloop(); // call this every now and then to get GM clicks and UART reports
	 	                        // suitable frequency to call this: every 10-50 ms.
		.. your code ...
	}

 	// restore standard main loop config:
	leave_menu();
}
 */

void enter_menu(void);
void geiger_mini_mainloop(void);
void leave_menu(void);

#endif // __MAIN_H__
