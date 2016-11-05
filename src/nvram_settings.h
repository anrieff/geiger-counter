/*
	Title: Geiger Counter with Serial Data Reporting and display
	Description: Code for comminicating with a PC over UART.
		
	(vesko) 10/31/16 2.10: Support for more UART commands.

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

#ifndef __NVRAM_SETTINGS_H__
#define __NVRAM_SETTINGS_H__

/*
 * Device ID, a number between [1, 65534].
 * Most commonly, users would like to set this to their device's serial number.
 * Related commands: GETID, SID
 * Default         : 0 (unprogrammed)
 */
uint16_t s_get_device_id(void);
void     s_set_device_id(uint16_t id);

/*
 * Tube multiplier, conversion between CPM and uSv/h.
 * Related commands: GETTM, STMN, STMD
 * Default         : 57, 100 (57/100)
 */ 
void     s_get_tube_mult(uint16_t* num, uint16_t* denom);
void     s_set_tube_mult_num(uint16_t num);
void     s_set_tube_mult_den(uint16_t denom);

/*
 * Sets the alarm level for background radiation flux, in uSv/h.
 * Range                  : 1 - 65535. 0 disables the alarm.
 * Related commands       : GETRA, STRA
 * Default                : 0 (disabled)
 */ 
uint16_t s_get_rad_limit(void);
void     s_set_rad_limit(uint16_t limit);

/*
 * Sets the alarm level for absorbed dose, in units of 10*uSv.
 * Internal representation: uint16, in units of 10 uSv.
 * Range                  : 1 - 65535. 0 disables the alarm.
 * Related commands       : GETDA, STDA
 * Default                : 0 (disabled)
 */ 
uint16_t s_get_dose_limit(void);
void     s_set_dose_limit(uint16_t limit);

// Structure that holds various device settings, packed in a byte.
struct Settings {
	// EEPROM verification magic. Has to be '1', otherwise this Settings
	// cannot be trusted (can happen after flash programming, when the EEPROM
	// becomes filled with 1s).
	uint8_t magic           : 2;

	// Do we want audible alarm when the batteries are nearly drained?
	// Default: 1 - on
	uint8_t battery_warning : 1;

	// Do we want UART output enabled on startup? It may always be changed with
	// the SILENT/NOISY commands, but this sets the "after reset" value.
	// Default: 1 - on
	uint8_t uart_output     : 1;
};

// read/write settings:
struct Settings s_settings(void);
uint8_t s_settings_as_byte(void);
void s_write_settings_as_byte(uint8_t new_value);

#endif // __NVRAM_SETTINGS_H__
