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
#ifdef DRYRUN
#	include "mock.h"
#else
#	include <avr/io.h>			// this contains the AVR IO port definitions
#	include <avr/pgmspace.h>	// tools used to store variables in program memory
#	include <avr/sleep.h>		// sleep mode utilities
#	include <util/delay.h>		// some convenient delay functions
#	include <avr/eeprom.h>     // for read/write to EEPROM memory
#endif
#include <stdlib.h>
#include <string.h>
#include "pinout.h"
#include "main.h"
#include "revision.h"
#include "nvram_map.h"
#include "logging.h"

enum {
	SRAMLOG_LENGTH =  40,  // 40 * 30 secs = 20 minutes.
	EELOG_LENGTH   = 240,
};

static struct LogInfo sram, eelog;
static uint32_t buffer[SRAMLOG_LENGTH]; // buffer for the SRAM log
static uint32_t gm_accum; // accumulator
static uint16_t gm_counts; // how many 30-second samples are in the accumulator
static uint16_t gm_flush_amount;

static inline uint16_t readEE(uint8_t index)
{
	return nv_read_word(ADDR_log_GM + 2 * (uint16_t) index);
}

static void writeEE(uint8_t index, uint16_t value)
{
	nv_update_word(ADDR_log_GM + 2 * (uint16_t) index, value);
}

static inline uint32_t round_down(uint32_t x, uint8_t scaling)
{
	if (!scaling) return x;
	return ((x >> (scaling - 1)) + 1) >> 1;
}

static int get_eelog_length(void)
{
	int len = 0;
	while (len < EELOG_LENGTH && readEE(len) != 0)
		len++;
	return len;
}

static void write_nv_struct(void)
{
	nv_update_word(ADDR_log_id, eelog.id);
	nv_update_byte(ADDR_log_res, eelog.res);
	nv_update_byte(ADDR_log_scaling, eelog.scaling);
}

static void shrink_buffer()
{
	// EEPROM buffer is about to overflow. Subsample and decrease resolution:
	uint8_t i;

	// one prepass to check for value summation overflow:
	uint32_t max_sum = 0;
	for (i = 0; i < EELOG_LENGTH/2; i++) {
		uint32_t sum = (uint32_t) readEE(2 * i) + readEE(2 * i + 1);
		if (sum > max_sum)
			max_sum = sum;
	}

	// see how much should we shift right to avoid overflow
	// this is *very* rarely necessary:
	uint8_t extra_shift = 0;
	while (round_down(max_sum, extra_shift) > 0xffff) {
		max_sum >>= 1;
		extra_shift++;
	}

	for (i = 0; i < EELOG_LENGTH/2; i++) {
		// merge GM counts for two periods (plain sum):
		uint32_t sum = (uint32_t) readEE(2 * i) + readEE(2 * i + 1);
		writeEE(i, round_down(sum, extra_shift));
	}
	if (extra_shift)
		eelog.scaling += extra_shift;

	// fill the now empty half with zeros:
	for (i = EELOG_LENGTH / 2; i < EELOG_LENGTH; i++)
		writeEE(i, 0);

	// increment the "resolution" flag and double the num samples per flush:
	eelog.res++;
	write_nv_struct(); // update EEPROM as well
	gm_flush_amount *= 2;
	// reset the length:
	eelog.length = EELOG_LENGTH / 2;
}


static void add_sample_eeprom(uint32_t gm)
{
	gm_accum += gm; gm_counts++;
	if (gm_counts == gm_flush_amount) {
		gm_accum = round_down(gm_accum, eelog.scaling);
		if (gm_accum > 0xffff) {
			uint8_t extra_shift = 1;
			while (round_down(gm_accum, extra_shift) > 0xffff)
				extra_shift++;
			
			for (uint8_t i = 0; i < eelog.length; i++)
				writeEE(i, round_down(readEE(i), extra_shift));
			
			eelog.scaling += extra_shift;
			gm_accum = round_down(gm_accum, extra_shift);
			nv_update_byte(ADDR_log_scaling, eelog.scaling);
		}
		writeEE(eelog.length, gm_accum);
		gm_counts = 0;
		gm_accum = 0;

		eelog.length++;

		if (eelog.length == EELOG_LENGTH) 
			shrink_buffer();
	}
}

static void add_sample_sram(uint32_t gm)
{
	if (sram.length < SRAMLOG_LENGTH) {
		buffer[sram.length++] = gm;
	} else {
		// SRAM log overflow; transfer to EEPROM and mark them as copies of
		// each other:
		uint32_t max_sample = 0;
		for (uint8_t i = 0; i < SRAMLOG_LENGTH; i++)
			if (buffer[i] > max_sample)
				max_sample = buffer[i];

		uint8_t ee_scaling = 0;
		while (round_down(max_sample, ee_scaling) > 0xffff) {
			ee_scaling++;
		}
		// write to EEPROM buffer:
		for (uint8_t i = 0; i < SRAMLOG_LENGTH; i++)
			writeEE(i, round_down(buffer[i], ee_scaling));

		// write zeros to the rest:
		for (uint8_t i = SRAMLOG_LENGTH; i < EELOG_LENGTH; i++)
			writeEE(i, 0);

		eelog = sram;
		eelog.scaling = ee_scaling;
		write_nv_struct();

		add_sample_eeprom(gm);
	}
}

void logging_init(void)
{
	eelog.id = nv_read_word(ADDR_log_id);
	if (eelog.id == 65535) {
		// assume NVRAM is filled with ones; clear that up:
		for (uint16_t i = 1; i < 512; i++)
			nv_update_byte(i, 0);
	}
	eelog.length = get_eelog_length();
	eelog.scaling = 0;
	eelog.res = nv_read_byte(ADDR_log_res);
	sram.id = eelog.id + 1; // increment log id
	sram.length = 0;
	sram.scaling = 0;
	sram.res = 1; // 30 second samples

	gm_accum = gm_counts = 0;
	gm_flush_amount = 1;
}

void logging_add_data_point(uint32_t gm)
{
	if (sram.id != eelog.id) {
		add_sample_sram(gm);
	} else {
		add_sample_eeprom(gm);
	}
}

void logging_get_info(LogEntry log_entry, struct LogInfo* log_info)
{
	if (log_entry == LOG_SRAM)
		*log_info = sram;
	else
		*log_info = eelog;
}

void logging_reset_all(void)
{
	// zero EEPROM memory:
	uint8_t i;
	nv_update_byte(ADDR_log_res, 1);
	for (i = 0; i < EELOG_LENGTH; i++)
		nv_update_word(ADDR_log_GM + 2 * i, 0);
	memset(buffer, 0, sizeof(buffer));

	// reset structures:
	logging_init();
}

void logging_fetch_log(LogEntry log_entry, PFNValue value_fn, PFNLine endline)
{
	const struct LogInfo* log = (log_entry == LOG_SRAM) ? &sram : &eelog;
	uint8_t i;

	value_fn(log->id);
	value_fn(log->res);
	value_fn(log->scaling);
	value_fn(log->length);
	endline();

	for (i = 0; i < log->length; i++)
		value_fn((log_entry == LOG_SRAM) ? buffer[i] : readEE(i));
	endline();
}
