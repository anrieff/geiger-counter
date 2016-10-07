/*
	Title: Geiger Counter with Serial Data Reporting and display
	Description: Code for comminicating with a PC over UART.
		
	(vesko)  8/18/16 2.10: Initial support for radiation logging
	                       and UART commands.

		Copyright 2011 Jeff Keyzer, MightyOhm Engineering
		Copyright 2016 Veselin Georgiev, LVA Ltd.
 
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

#ifndef __PC_LINK_H__
#define __PC_LINK_H__

// if on, printing of per-second log lines should be suppressed:
extern char silent;

void pc_link_init(void);
void pc_link_check(void);

#endif // __PC_LINK_H__
