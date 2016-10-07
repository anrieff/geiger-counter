/*
	Title: Geiger Counter with Serial Data Reporting and display
	Description: Code for logging the radiation over time to EEPROM.
		
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

#ifndef __LOGGING_H__
#define __LOGGING_H__

struct LogInfo {
	uint16_t id; // log ID (number)
	uint8_t res; // resolution (length of sample; length = 15 * 2**res). res > 0.
	uint8_t scaling; // sample scaling (value of each sample = x * 2**scaling).
	uint16_t length; // number of samples [0..240]
};

typedef enum {
	LOG_SRAM,   // a temporary log, kept in RAM
	LOG_EEPROM, // the long-running log, backed in the ATmega EEPROM
} LogEntry;

void logging_init(void);

// gets information about a specific log (log_entry), written in log_info
void logging_get_info(LogEntry log_entry, struct LogInfo* log_info);

// adds a data point to the logs. This shall be called every 30 seconds with
// aggregated info for these 30 seconds:
// gm - GM events for the last 30 seconds
// voltage - current battery voltage, in millivolts
void logging_add_data_point(uint32_t gm, uint16_t voltage);

// reset both logs
void logging_reset_all(void);

typedef void (*PFNValue) (uint16_t x);
typedef void (*PFNLine) (void);

/**
 * @brief Transmit a log from the device.
 * @param log_entry - which log to fetch.
 * @param value_fn  - a function pointer, which is to be called for each
 *                    data value transmitted.
 * @param line_fn   - a function pointer, which is to be called when a series
 *                    of data points are already transmitted.
 *
 * The calling pattern will be:
 * <logId>, <resolution>, <# samples>, <<newline>>
 * <gm sample 1>, <gm sample 2>, ..., <gm sample n>, <<newline>>
 * <v sample 1>, <v sample 2>, ..., <v sample m>, <<newline>>
 *
 * Where each <thing> denotes a call to the value function with a number,
 * and each <<newline>> denotes a call to the line function.
 * You can conveniently map the value function to `printf("%d ", value)',
 * and the line function to `printf("\n")'.
 */
void logging_fetch_log(LogEntry log_entry, PFNValue value_fn, PFNLine line_fn);

#endif // __LOGGING_H__
