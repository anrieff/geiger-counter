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

/**
 * @brief PC Link protocol description
 * @version 42
 * 
 * 
 * Command: HELO
 * Description: Replies with firmware revision and protocol version.
 * Sample response: "O HAI,331,42"
 * Synopsis: the first number is firmware revision, the second one is protocol
 *           version.
 * 
 * 
 * Command: STATUS
 * Description: Current status of the device and the logs
 * Sample response: "3036,10,1,125,4,1,40
 * Synopsis: The numbers are, in order
 *         - Current battery voltage in millivolts
 *         - Uptime (in seconds) since the last reset
 *         - EEPROM Log id#
 *         - EEPROM Log length (# of samples available)
 *         - EEPROM Log resolution (see below)
 *         - SRAM Log id# (will be the same as the EEPROM one if the device
 *           has been running for more than 20 minutes and the SRAM log was
 *           been transferred to EEPROM).
 *         - SRAM Log length (# of samples available)
 *         
 *           Resolution is a power of two thing, specifying what time range
 *           each sample encompasses: "range = (15 * 2^res) seconds".
 *           Lowest resolution possible is 1 (30 seconds per sample).
 *           Highest resolution is not bounded in code, but because of the
 *           exponential nature of the resolution, it is unlikely that you'd
 *           ever encounter more than 15 (which equals 5d 16h per sample, or
 *           more than 22 months of logging).
 *
 *           The SRAM and EEPROM log are different only in the first 20 minutes
 *           of uptime. After that their IDs are equal (the EEPROM one becomes
 *           what the SRAM one was), and the SRAM log is no longer updated.
 *
 *
 * Command: RESET (planned, not yet implemented)
 * Description: Resets the device immediately
 * Sample response: none (the device restarts), you'd see the startup banner.
 *
 * 
 * Command: CLOG
 * Description: Clears both logs
 * Sample response: "OK"
 * 
 * 
 * Command: SILENT
 * Description: Halts the printing out of per-second reports.
 * Sample response: "OK"
 * 
 * 
 * Command: NOISY
 * Description: Resumes the printing of per-second reports (after a 'SILENT')
 * Sample response: "OK"
 * 
 * 
 * Command: RSLOG
 * Description: Read the SRAM log
 * Sample response: 
 *   15,1,0,23
 *   10,8,11,13,10,9,12,14,11,8,8,10,12,7,9,10,11,13,14,10,11,9,9
 *   138
 * Synopsis: First line is "id,resolution,scaling,#samples"
 *  Where: `id' is a numerical id of the log, monotonically increasing with each
 *         new log;
 *         `resolution' is the 2^resolution sample size, as explained in the
 *         Synopsis for the STATUS command.
 *         `scaling' is overflow-avoiding scaling of the sample values. The
 *         sample values will be 16-bit numbers, but their true value should be
 *         obtained after multiplication by 2^scaling. Note that this number
 *         will be most commonly zero, so the samples are as-is; it can only
 *         need to be positive if an extremely large log duration is captured,
 *         or if the counter measures a place with high radiation.
 *         `#samples' is the count of the samples that follow in the next line.
 *
 *  The second line contains #samples numbers in [0..65535] - the samples of the
 *  log. Each sample X should be interpreted to mean that "(X * 2^scaling) 
 *  Geiger-Muller discharges were recorded within a time interval of 
 *  (15 * 2^resolution) seconds.".
 *
 *  The last line contains a log of the supply voltage. Its length will be
 *  exactly floor(#samples/20). Each sample Y should be interpreted as
 *  (1.65 + Y/100) volts average for a time interval of (300 * 2^resolution)
 *  seconds. The samples of the supply voltage log are 8-bit, hence the
 *  peculiar encoding.
 *
 *
 * Command: REELOG
 * Description: Read the EEPROM log.
 * Sample response: See RSLOG.
 * Synopsis: The same format as RSLOG, but it reads the EEPROM log. The length
 *           of the log can be max EELOG_LENGTH samples (240 currently).
 * 
 * 
 * Command: GETID
 * Description: Gets the device ID.
 * Sample response: "14351"
 * Synopsis: Gets the device ID (a 16-bit number stored in the EEPROM). This
 *           number is initially zero and should be programmed with a random
 *           value, to uniquely identify a device if a user has more than one
 *           of them. This command retrieves that number.
 * 
 * 
 * Command: SID <number>
 * Description: Sets the device ID
 * Sample response: "OK"
 * Synopsis: Sets (writes to EEPROM) a device id, which should be a 16-bit
 *           unsigned number.
 * 
 * 
 * Command: GETTM (planned, not yet implemented)
 * Description: Reads the G-M tube multiplier (tube sensitivity factor).
 * Sample response: "57/100"
 * Synopsis: This is the tube sensitivity conversion factor. If you have
 *           X counts per minute, and the sensitiity is (N/D), the radiation
 *           would be calculated as ((X * N) / (D * 100)) uSv/h.
 * Note:     The original geiger counter assumed this factor to be 57/100, or
 *           0.57, for the SBM-20 tube. What we introduced here is the same
 *           rational arithmetics (in order to avoid floating-point), but the
 *           numerator and especially the denominator are no longer hard-coded.
 *           This provides for easily achieving very high precision without
 *           comlpicating runtime computations on the device too much.
 * Example:  If you run the geiger tube through calibration and it turns out
 *           that the actual sensitivity was 0.5617, you can approximate that
 *           sufficiently with 91/162.
 * 
 *
 * Command: STMN <number> (planned, not yet implemented)
 * Description: Sets the tube multiplier numerator
 * Sample response: "OK"
 * Synopsis: This sets the numerator of the tube sensitivity conversion factor,
 *           writing it in the EEPROM.
 * Note:     This value should not exceed 7158, otherwise you're risking an
 *           overflow in the computation with the worst possible scenario.
 * 
 * 
 * Command: STMD <number> (planned, not yet implemented)
 * Description: Sets the tube multiplier denominator
 * Sample response: "OK"
 * Synopsis: This sets the denominator of the tube sensitivity conversion
 *           factor, writing it in the EEPROM. This value is 16-bit and thus
 *           cannot exceed 65535.
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
#include "pinout.h"
#include "main.h"
#include "revision.h"
#include "nvram_map.h"
#include "pc_link.h"
#include "logging.h"
#include "battery.h"

static char recv_buf[12];
static volatile uint8_t recv_buf_ptr;
static volatile char cmd_event;
char silent;

ISR(USART_RX_vect)
{
	char c = UDR0;
	if (recv_buf_ptr >= sizeof(recv_buf))
		recv_buf_ptr = 0;
	recv_buf[recv_buf_ptr++] = c;
	if (c == '\n')
		cmd_event = 1;
}

void pc_link_init(void)
{
	// enable interrupt on UART RX:
#ifndef DRYRUN
	UCSR0B |= _BV(RXCIE0);
#endif
	recv_buf_ptr = 0;
}

static char first_item_in_line = 1;

static void print_number_uint16(uint16_t x)
{
	if (first_item_in_line)
		first_item_in_line = 0;
	else
		uart_putchar(',');
	uart_print_number(x);
}

static void print_newline(void)
{
	uart_putchar('\n');
	first_item_in_line = 1;
}

static void interpret_command(const char* cmd)
{
	if (!strcmp(cmd, "HELO")) {
		uart_putstring_P(PSTR("O HAI," FIRMWARE_REVISION_STR ",42\n"));
	} else if (!strcmp(cmd, "STATUS")) {
		uart_print_number(battery_get_voltage()); uart_putchar(',');
		uart_print_number(get_uptime_seconds()); uart_putchar(',');
		
		struct LogInfo info;
		logging_get_info(LOG_EEPROM, &info);
		uart_print_number(info.id); uart_putchar(',');
		uart_print_number(info.length); uart_putchar(',');
		uart_print_number(info.res); uart_putchar(',');

		logging_get_info(LOG_SRAM, &info);
		uart_print_number(info.id); uart_putchar(',');
		uart_print_number(info.length); print_newline();
	} else if (!strcmp(cmd, "CLOG")) {
		logging_reset_all();
		uart_putstring_P(PSTR("OK\n"));
	} else if (!strcmp(cmd, "SILENT")) {
		silent = 1;
		uart_putstring_P(PSTR("OK\n"));
	} else if (!strcmp(cmd, "NOISY")) {
		silent = 0;
		uart_putstring_P(PSTR("OK\n"));
	} else if (!strcmp(cmd, "RSLOG") || !strcmp(cmd, "REELOG")) {
		LogEntry e = (cmd[1] == 'E') ? LOG_EEPROM : LOG_SRAM;
		logging_fetch_log(e, print_number_uint16, print_newline);
	} else if (!strcmp(cmd, "GETID")) {
		uart_print_number(nv_read_word(ADDR_device_id));
		print_newline();
	} else if (!strncmp(cmd, "SID ", 4)) {
		uint16_t value = 0;
		for (uint8_t i = 4; cmd[i]; i++)
			value = value * 10 + (cmd[i] - '0');
		nv_update_word(ADDR_device_id, value);
		uart_putstring_P(PSTR("OK\n"));
	} else {
		uart_putstring_P(PSTR("Unknown command!\n"));
	}
}

void pc_link_check(void)
{
	if (!cmd_event) return;

	char cmd[10];	
	// copy the command and free up the receive buffer
	cli(); // disable interrupts
	memcpy(cmd, recv_buf, recv_buf_ptr);
	cmd[recv_buf_ptr] = 0;
	cmd_event = 0;
	recv_buf_ptr = 0;
	sei(); // reenable interrupts

	// trim all trailing newlines from the command buffer:
	uint8_t i = 0;
	while (cmd[i]) i++;
	while (i > 0 && (cmd[i - 1] == '\n' || cmd[i - 1] == '\r')) cmd[--i] = 0;

	// interpret the command:
	interpret_command(cmd);
}
