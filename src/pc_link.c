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
 * Command: RESET
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
 * """
 *   15,1,0,23
 *   10,8,11,13,10,9,12,14,11,8,8,10,12,7,9,10,11,13,14,10,11,9,9
 *   138
 *
 * """
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
 *  Note there's an extra newline after the battery log line. It's always there,
 *  even if the battery log is empty.
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
 * Command: GETTM
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
 * Command: STMN <number>
 * Description: Sets the tube multiplier numerator
 * Sample response: "OK"
 * Synopsis: This sets the numerator of the tube sensitivity conversion factor,
 *           writing it in the EEPROM.
 * Note:     This value should not exceed 7158, otherwise you're risking an
 *           overflow in the computation with the worst possible scenario.
 * 
 * 
 * Command: STMD <number>
 * Description: Sets the tube multiplier denominator
 * Sample response: "OK"
 * Synopsis: This sets the denominator of the tube sensitivity conversion
 *           factor, writing it in the EEPROM. This value is 16-bit and thus
 *           cannot exceed 65535. It also can't be 0 (you can't divide by zero).
 *
 *
 * Command: GETRA
 * Description: Gets the radiation level alarm threshold, in uSv/h.
 * Sample response: "10"
 * Synopsis: This sets the level of radiation, which, if exceeded, sets off
 *           a 20-second alarm sound. If the radiation level continues to be
 *           exceeded, the alarm will stop after the 20 seconds, but will ring
 *           again in 5 minutes. The alarm sound can be interrupted by the
 *           button at any time.
 *           If this value is 0, the alarm is disabled.
 *           The alarm sound type is a [beep 0.5s]-[pause 0.5s] pattern.
 *           On the display you get alternating 'rAd. '/' HI. ' readings.
 *
 *
 * Command: STRA <new_limit>
 * Description: Sets the radiation level alarm threshold.
 * Sample response: "OK"
 * Synopsis: see GETRA
 *
 *
 * Command GETDA
 * Description: Gets the accumulated dose alarm threshold, in units of 10*uSv.
 * Sample response: "10"
 * Synopsis: This sets the level of accumulated radiation dose, which, if
 *           exceeded, sets off a 1-minute alarm sound. After sounding, the
 *           alarm doesn't ring again.
 *           If this value is 0, the alarm is disabled.
 *           The alarm sound type is a [beep 1s]-[pause 1s] pattern.
 *           On the display you get alternating 'dOSE'/' HI. ' readings.
 *
 *
 * Command: STDA <new_limit>
 * Description: Sets the accumulated dose alarm threshold, in units of 10*uSv.
 * Sample response: "OK"
 * Synopsis: see GETDA
 *
 *
 *********************************
 ** Settings bitfield commands: ** 
 *********************************
 *
 * All of these commands either read or set a bit in the device settings byte.
 * If you invoke the command without parameters, it is read out and the 
 * response is either '0' or '1'.
 * If you supply an argument (which again should be '0' or '1'), it is written
 * to device memory, and the response is 'OK'.
 *
 * Example input: "BLVW"
 * Example ouput: "1"
 * Meaning      : Battery low-voltage warning is enbled.
 *
 * Example input: "BLVW 0"
 * Example ouput: "OK"
 * Meaning      : Battery low-voltage warning was successfully disabled.
 *
 *
 * List of bitfield commands:
 * Command    Meaning                                               Defaults to
 * BLVW       Battery low-voltage warning                           1
 * UASU       UART active on start-up                               1
 */
#ifdef DRYRUN
#	include "mock.h"
#else
#	include <avr/io.h>			// this contains the AVR IO port definitions
#	include <avr/interrupt.h>
#	include <avr/pgmspace.h>	// tools used to store variables in program memory
#	include <avr/sleep.h>		// sleep mode utilities
#	include <util/delay.h>		// some convenient delay functions
#	include <avr/eeprom.h>      // for read/write to EEPROM memory
#	include <avr/wdt.h>         // watchdog timer utilities
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
#include "nvram_settings.h"

 enum {
 	NORMAL,
 	OK,
 	UNKNOWN_COMMAND,
 	BAD_ARGUMENT,
 	ARGUMENT_EXPECTED,
 };

static char recv_buf[12];
static volatile uint8_t recv_buf_ptr;
static volatile char cmd_event;
static struct LogInfo log_info;
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
	silent = !(s_settings().uart_output);
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


/**
 * @retval -1 - command has no arguments
 * @retval  0 - command has argument '0'
 * @retval +1 - command has argument '1'
 * @retval BAD_ARGUMENT - command has incorrectly formatted argument
 */
static int8_t has_bool_arg(const char* cmd)
{
	if (*cmd != ' ') return -1;
	uint8_t c = cmd[1] - '0';
	if (c < 2) return c;
	return BAD_ARGUMENT;
}

/**
 * @retval NORMAL            - everything is correct
 * @retval BAD_ARGUMENT      - invalid argument (bad format, out of range, etc.)
 * @retval ARGUMENT_EXPECTED - command has no arguments, but it should
 */
static int8_t has_arg(const char* cmd, uint16_t* arg)
{
	if (*cmd != ' ') return ARGUMENT_EXPECTED;
	uint16_t x = 0;
	// parse the argument as a number, with overflow detection:
	for (cmd++; *cmd; cmd++) {
		uint8_t t = *cmd - '0';
		if (t > 9 || x > 6553) return BAD_ARGUMENT;
		if (x == 6553 && t > 5) return BAD_ARGUMENT;
		x = x * 10 + t;
	}
	*arg = x;
	return NORMAL;
}

static uint16_t hash(const char* s)
{
	uint16_t x = 0;
	for (; *s && *s != ' '; ++s)
		x = x * 37491 + *s;
	return x;
}

static int8_t handle_bool_cmd(uint8_t which_bit, int8_t new_value)
{
	if (new_value > 1) return new_value;
	if (new_value == -1) {
		// display:
		uart_putchar('0' + ((s_settings_as_byte() >> which_bit) & 1));
		return NORMAL;
	} else {
		// update:
		uint8_t temp = s_settings_as_byte();
		if (((temp >> which_bit) & 1) != new_value) {
			temp ^= 1 << which_bit;
			s_write_settings_as_byte(temp);
		}
		return OK;
	}
}

static void print_log_info(LogEntry log_entry)
{
	logging_get_info(log_entry, &log_info);

	uart_putchar(',');
	uart_print_number(log_info.id);
	uart_putchar(',');
	uart_print_number(log_info.length);
}

int8_t interpret_command(const char* cmd)
{
	int8_t ok;
	uint16_t arg;

	switch (hash(cmd)) {

		/* BLVW - Battery low-voltage warning */
		case 0x09BB:
		{
			return handle_bool_cmd(BIT_BLVW, has_bool_arg(cmd + 4));
		}

		/* CLOG - Clear all logs */
		case 0x6371:
		{
			//
			logging_reset_all();
			//
			return OK;
		}

		/* GETDA - Get dose alarm limit */
		case 0x3ECF:
		{
			//
			uart_print_number(s_get_dose_limit());
			//
			return NORMAL;
		}

		/* GETID - Get device id */
		case 0x1B11:
		{
			//
			uart_print_number(s_get_device_id());
			//
			return NORMAL;
		}

		/* GETRA - Get radiation alarm limit */
		case 0x4119:
		{
			//
			uart_print_number(s_get_rad_limit());
			//
			return NORMAL;
		}

		/* GETTM - Get tube multiplier */
		case 0x660B:
		{
			//
			uint16_t num, denom;
			s_get_tube_mult(&num, &denom);
			uart_print_number(num);
			uart_putchar('/');
			uart_print_number(denom);
			//
			return NORMAL;
		}

		/* HELO - Print hello message */
		case 0xD518:
		{
			//
			uart_putstring_P(PSTR("O HAI," FIRMWARE_REVISION_STR ",42"));
			//
			return NORMAL;
		}

		/* NOISY - Enable UART reports */
		case 0x5386:
		{
			//
			silent = 0;
			//
			return OK;
		}

		/* REELOG - Read EEPROM log */
		case 0x7092:
		{
			//
			logging_fetch_log(LOG_EEPROM, print_number_uint16, print_newline);
			//
			return NORMAL;
		}

		/* RESET - Resets the device */
		case 0xF6E7:
		{
			// use the watchdog to force a software reset:
			cli();
			wdt_enable(WDTO_15MS);
			for (;;) ; // endless loop until the watchdog resets us.
			//
			// no need for return handling
		}

		/* RSLOG - Read SRAM log */
		case 0x0A93:
		{
			//
			logging_fetch_log(LOG_SRAM, print_number_uint16, print_newline);
			//
			return NORMAL;
		}

		/* SID - Set device id */
		case 0xC6DA:
		{
			if ((ok = has_arg(cmd + 3, &arg)) != NORMAL) return ok;
			//
			s_set_device_id(arg);
			//
			return OK;
		}

		/* SILENT - Disable UART reports */
		case 0x6D61:
		{
			//
			silent = 1;
			//
			return OK;
		}

		/* STATUS - Print device status */
		case 0xA68E:
		{
			//
			uart_print_number(battery_get_voltage()); uart_putchar(',');
			uart_print_number(get_uptime_seconds());

			print_log_info(LOG_EEPROM); uart_putchar(',');
			uart_print_number(log_info.res);
			print_log_info(LOG_SRAM);
			//
			return NORMAL;
		}

		/* STDA - Set dose alarm limit */
		case 0xC472:
		{
			if ((ok = has_arg(cmd + 4, &arg)) != NORMAL) return ok;
			//
			s_set_dose_limit(arg);
			//
			return OK;
		}

		/* STMD - Set tube multiplier denominator */
		case 0xEA80:
		{
			if ((ok = has_arg(cmd + 4, &arg)) != NORMAL) return ok;
			if (arg == 0) return BAD_ARGUMENT;
			//
			s_set_tube_mult_den(arg);
			//
			return OK;
		}

		/* STMN - Set tube multiplier numerator */
		case 0xEA8A:
		{
			if ((ok = has_arg(cmd + 4, &arg)) != NORMAL) return ok;
			if (arg == 0 || arg > 7158) return BAD_ARGUMENT;
			//
			s_set_tube_mult_num(arg);
			//
			return OK;
		}

		/* STRA - Set radiation alarm limit */
		case 0xC6BC:
		{
			if ((ok = has_arg(cmd + 4, &arg)) != NORMAL) return ok;
			//
			s_set_rad_limit(arg);
			//
			return OK;
		}

		/* UASU - UART active on startup */
		case 0xF58E:
		{
			return handle_bool_cmd(BIT_UASU, has_bool_arg(cmd + 4));
		}

		default:
			return UNKNOWN_COMMAND;
	}
}

void pc_link_check(void)
{
	if (!cmd_event) return;

	char cmd[sizeof(recv_buf)];
	uint8_t i;
	// copy the command and free up the receive buffer
	cli(); // disable interrupts
	i = recv_buf_ptr;
	memcpy(cmd, recv_buf, i);
	cmd_event = 0;
	recv_buf_ptr = 0;
	sei(); // reenable interrupts

	cmd[i] = 0;

	// trim all trailing n ewlines from the command buffer:
	while (i > 0 && (cmd[i - 1] == '\n' || cmd[i - 1] == '\r')) cmd[--i] = 0;

	// interpret the command:
	static const char* const RESPONSES[] PROGMEM = {
		"OK",
		"Unknown command!",
		"Bad argument (format or range error)!",
		"Argument required",
	};
	int8_t response = interpret_command(cmd);
	if (response != NORMAL) {
		uart_putstring_P(RESPONSES[response - OK]);
	}
	uart_putchar('\n');
}
