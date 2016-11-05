/*
	Title: Geiger Counter with Serial Data Reporting and display
	Description: Sounding alarms when radiation/dose limits are exceeded.
		
	(vesko)   8/1/15 1.50: Initial release of the upgraded version.
	(vesko) 12/21/15 2.00: Update to drive a "dumb" display directly

		Copyright 2011 Jeff Keyzer, MightyOhm Engineering
		Copyright 2015, 2016 Veselin Georgiev, LVA Ltd.
 
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

#ifndef __ALARMS_H__
#define __ALARMS_H__

extern uint8_t alarm_mode;         // one of the AlarmMode values
extern uint8_t alarm_tick;         // set by ISR to nonzero 8 times per second
extern int8_t alarm_idle_minutes;  // minutes until the alarm can be triggered again


// a list of alarm modes:
enum AlarmMode {
	ALARM_NONE,    // normal mode, no alarm present
	ALARM_1HZ ,    // radiation level alarm (faster beeps)
	ALARM_HALF_HZ, // dose exceeded alarm (slower beeps)
};

// start an alarm condition (it will sound the alarm for some time)
void alarm_start(enum AlarmMode mode);

// immediately stop the alarm
void alarm_stop(void);

// check if alarm is on and needs processing 
// (should be called regularly from main())
void checkalarm(void);

// check if alarm needs to be sounded. Must be called whenever radiation levels
// are updated and need to be checked if they crossed the thresholds.
// @param usv_per_h:    microsieverts per hour
// @param total_counts: total G-M events since startup
void alarm_check_conditions(uint32_t usv_per_h, uint32_t total_counts);
#endif // __ALARMS_H__
