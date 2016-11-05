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

#ifdef DRYRUN
#	include "mock.h"
#else
#	include <avr/io.h>			// this contains the AVR IO port definitions
#	include <avr/interrupt.h>
#	include <avr/pgmspace.h>	// tools used to store variables in program memory
#	include <avr/sleep.h>		// sleep mode utilities
#	include <util/delay.h>		// some convenient delay functions
#	include <avr/eeprom.h>     // for read/write to EEPROM memory
#endif
#include <stdlib.h>
#include <string.h>
#include "nvram_settings.h"
#include "nvram_map.h"

/*
 * Device ID, a number between [1, 65534].
 * Most commonly, users would like to set this to their device's serial number.
 * Related commands: GETID, SID
 * Default         : 0 (unprogrammed)
 */
static uint8_t device_id_cached = 0;
static uint16_t device_id = 0;
uint16_t s_get_device_id(void)
{
	if (!device_id_cached) {
		device_id_cached = 1;
		device_id = nv_read_word(ADDR_device_id);
		if (device_id == 0xffff)
			device_id = 0; // handle the EEPROM = all 1s case
	}
	return device_id;
}

void     s_set_device_id(uint16_t id)
{
	device_id_cached = 1;
	device_id = id;
	nv_update_word(ADDR_device_id, id);
}


/*
 * Tube multiplier, conversion between CPM and uSv/h.
 * Related commands: GETTM, STMN, STMD
 * Default         : 57, 100 (57/100)
 */ 
static uint8_t tubemult_checksum(uint16_t num, uint16_t denom)
{
    return ((uint8_t) (num * 199 + denom * 31 + 35)) & 7;
}

static uint8_t tube_mult_cached = 0;
static uint16_t tube_num = 57, tube_denom = 100;

void     s_get_tube_mult(uint16_t* num, uint16_t* denom)
{
	if (!tube_mult_cached) {
		tube_mult_cached = 1;
		uint16_t x = nv_read_word(ADDR_tube_num);
		uint16_t y = nv_read_word(ADDR_tube_denom);
		uint8_t cksum = (x >> 13) & 0xff;
		x &= 0x1fff;
		if (tubemult_checksum(x, y) == cksum) {
			tube_num = x;
			tube_denom = y;
		}
	}

	*num = tube_num;
	*denom = tube_denom;
}

static void write_tube_mult(void)
{
	tube_mult_cached = 1;
	uint16_t value = tubemult_checksum(tube_num, tube_denom);
	value <<= 13;
	value |=  tube_num;
	nv_update_word(ADDR_tube_num, value);
	nv_update_word(ADDR_tube_denom, tube_denom);
}

void     s_set_tube_mult_num(uint16_t num)
{
	if (num > 0x1fff) return;
	tube_num = num;
	write_tube_mult();
}

void     s_set_tube_mult_den(uint16_t denom)
{
	tube_denom = denom;
	write_tube_mult();
}

/*
 * Sets the alarm level for background radiation flux, in uSv/h.
 * Range                  : 1 - 65535. 0 disables the alarm.
 * Related commands       : GETRA, STRA
 * Default                : 0 (disabled)
 */ 

static uint8_t  rad_limit_cached = 0;
static uint16_t rad_limit = 0;

uint16_t s_get_rad_limit(void)
{
	if (!rad_limit_cached) {
		rad_limit_cached = 1;
		rad_limit = nv_read_word(ADDR_rad_limit);
		if (rad_limit == 0xffff)
			rad_limit = 0; // handle 'all 1s' EEPROM.
	}
	return rad_limit;
}
void     s_set_rad_limit(uint16_t limit)
{
	tube_mult_cached = 1;
	rad_limit = limit;
	nv_update_word(ADDR_rad_limit, limit);
}

/*
 * Sets the alarm level for absorbed dose, in units of 10*uSv.
 * Internal representation: uint16, in units of 10 uSv.
 * Range                  : 1 - 65535 (equiv. to 10-655350 uSv). 0 disables the alarm.
 * Related commands       : GETDA, STDA
 * Default                : 0 (disabled)
 */ 
static uint8_t  dose_limit_cached = 0;
static uint16_t dose_limit = 0;

uint16_t s_get_dose_limit(void)
{
	if (!dose_limit_cached) {
		dose_limit_cached = 1;
		dose_limit = nv_read_word(ADDR_dose_limit);
		if (dose_limit == 0xffff)
			dose_limit = 0; // handle 'all 1s' EEPROM.
	}
	return dose_limit;
}

void     s_set_dose_limit(uint16_t limit)
{
	dose_limit_cached = 1;
	dose_limit = limit;
	nv_update_word(ADDR_dose_limit, dose_limit);
}

static union {
	struct Settings set;
	uint8_t         byte;
} settings;
static uint8_t settings_cached;

static void check_settings_cache(void)
{
	if (!settings_cached) {
		settings_cached = 1;
		settings.byte = nv_read_byte(ADDR_settings);
		if (settings.set.magic != 1) {
			// EEPROM probably 'all 1s', initialize with defaults:
			settings.byte = 0xfd; // enable all bits, make magic == 1.
		}
	}
}

struct Settings s_settings(void)
{
	check_settings_cache();
	return settings.set;
}

uint8_t s_settings_as_byte(void)
{
	check_settings_cache();
	return settings.byte;
}

void s_write_settings_as_byte(uint8_t new_value)
{
	if ((new_value & 0x3) != 1) return;
	settings_cached = 1;
	settings.byte = new_value;
	nv_update_byte(ADDR_settings, settings.byte);
}
