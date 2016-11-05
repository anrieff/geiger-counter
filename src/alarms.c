/*
	Title: Geiger Counter with Serial Data Reporting and display
	Description: Sounding alarms when radiation/dose limits are exceeded.
		
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

// Includes
#include <avr/io.h>			// this contains the AVR IO port definitions
#include "display.h"        // code to drive the 7-segment display
#include "pinout.h"
#include "main.h"
#include "alarms.h"
#include "characters.h"
#include "nvram_settings.h"

uint8_t alarm_mode;         // one of the AlarmMode values
uint8_t alarm_tick;         // set by ISR to nonzero 8 times per second
int8_t alarm_idle_minutes;  // minutes until the alarm can be triggered again

static int8_t dose_alarm_sounded;  // was the dose alarm ever sounded
static int8_t alarm_remaining;     // time remaining until alarm is silenced
static int8_t alarm_display_was_on;// was the display on when the alarm got triggered?

void alarm_start(enum AlarmMode mode)
{
	alarm_mode = mode;
	if (mode == ALARM_HALF_HZ) dose_alarm_sounded = 1;
	alarm_remaining = (mode == ALARM_1HZ) ? 20 : 60;
	alarm_display_was_on = display_is_on();
	if (!alarm_display_was_on)
		display_turn_on();
	display_clear();
}

void alarm_stop(void)
{
	sounder_off();
	alarm_mode = ALARM_NONE;
	if (!alarm_display_was_on)
		display_turn_off();
	alarm_idle_minutes = 5;
}

void checkalarm(void)
{
	// handle alarm processing; called each 128 ms (approx 8 times per second)
	// it's a state machine of what happens after which; this allows us to
	// interleave it with regular radiation & log processing.
	if (!alarm_tick || !alarm_mode) return;
	alarm_tick = 0;
	static uint8_t clk;
	if (++clk == 16) {
		clk = 0;
		alarm_remaining -= 2;
		if (alarm_remaining <= 0) {
			alarm_stop();
			return;
		}
	}
	switch (clk % 8) {
		case 0:
		{
			if (alarm_mode == ALARM_1HZ) {
				display[0] = cR;
				display[1] = cA;
				display[2] = cD;
				display[3] = 00;
				display_set_dots(DP3);
				sounder_on();
			}
			if (alarm_mode == ALARM_HALF_HZ) {
				display[0] = cD;
				display[1] = cO;
				display[2] = cS;
				display[3] = cE;
				if (clk == 0) sounder_on();
				else sounder_off();
			}
			break;
		}
		case 3:
		{
			display_clear();
			break;
		}
		case 4:
		{
			if (alarm_mode == ALARM_1HZ)
				sounder_off();
			
			display[1] = cH;
			display[2] = cI;
			display_set_dots(DP3);
			break;
		}
		case 7:
		{
			display_clear();
			break;
		}
		default:
			break;
	}
}

void alarm_check_conditions(uint32_t usv_per_h, uint32_t total_counts)
{
	if (alarm_mode) return; // no need for checking

	// test for radiation level alarm (does usv_per_h exceed the limit?)
	uint16_t limit = s_get_rad_limit();
	if (limit > 0 && usv_per_h >= limit && alarm_idle_minutes == 0) {
		alarm_start(ALARM_1HZ);
		return;
	}

	// test for radiation dose alarm (does total_counts exceed the limit?)
	// keep in mind that s_get_dose_limit() is in units of 10 usv = 1 dusv
	// (one deci-microsievert); we store that in order to extend the range
	// of the possible limit, but in turn have to tweak the computations a bit
	// to account for the factor of ten invovled.
	if (dose_alarm_sounded) return;
	uint16_t dusv_limit = s_get_dose_limit();
	if (dusv_limit == 0) return;
	uint16_t num, denom;
	s_get_tube_mult(&num, &denom);
	// radiation flux formula from sendreport():
	// (100*) uSv/h = CPM * numerator / denominator
	//
	// rearranging for total dose:
	// CPM = counts/m
	// (counts/m) * num / denom = usv/h * 100        | 1 usv/m = 60 usv/h
	// (counts/m) * num / denom = usv/m * 6000       | * m
	// counts * num / denom = usv * 6000             | 1 dusv = 10 usv
	// counts * num / denom = dusv * 60000
	// counts = dusv * 60000 * denom / num
	// counts / 60000 = dusv * denom / num
	// counts / (60000 / num) = dusv * denom
	//
	// ^^ mathematics at its beauty :)
	if (total_counts / (60000 / num) >= (uint32_t) dusv_limit * denom)
		alarm_start(ALARM_HALF_HZ);
}
