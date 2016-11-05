/*
	Title: Geiger Counter with Serial Data Reporting and display
	Description: Code for logging the radiation over time to EEPROM.
		
	(vesko)  9/6/16 2.10: Initial support for radiation logging
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

#ifndef __NVRAM_MAP__
#define __NVRAM_MAP__

#define nv_read_byte(addr) eeprom_read_byte((uint8_t*) (intptr_t) (addr))
#define nv_update_byte(addr, value) eeprom_update_byte((uint8_t*) (intptr_t) (addr), value)

#define nv_read_word(addr) eeprom_read_word((uint16_t*) (intptr_t) (addr))
#define nv_update_word(addr, value) eeprom_update_word((uint16_t*) (intptr_t) (addr), value)

enum NVRAMAddr {
	ADDR_brightness  = 0,   // display brightness      : a 8-bit value
	ADDR_settings    = 1,   // device settings bitfield: 8-bit value
	ADDR_device_id   = 2,   // unique device ID        : 16-bit number
	ADDR_tube_num    = 4,   // GM tube mult. numerator : 13-bit value + 3 bits of checksum
	ADDR_tube_denom  = 6,   // GM tube mult. denom.    : 16-bit value
	ADDR_rad_limit   = 8,   // Alarm for rad. level    : 16-bit value
	ADDR_dose_limit  = 10,  // Alarm for dose accum.   : 16-bit value
	
	ADDR_log_id      = 12,  // Log serial number (id)  : 16-bit value
	ADDR_log_res     = 14,  // Log "resolution"        : 8-bit value
	ADDR_log_scaling = 15,  // Log "scaling"           : 8-bit value
	ADDR_log_GM      = 16,  // start of GM log         : 240 values of 16 bits
	ADDR_log_V       = 496, // start of the voltage log: 12 values of 8 bits
};

enum SettingsBits {
	BIT_Magic = 0,
	BIT_BLVW  = 2,
	BIT_UASU  = 3,
};

#endif // __NVRAM_MAP__
