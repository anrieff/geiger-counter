/*
	Title: Geiger Counter with Serial Data Reporting and display
	Description: Code for reading out battery voltage via a resistive divider.
		
		Copyright 2011 Jeff Keyzer, MightyOhm Engineering
		Copyright 2015 Veselin Georgiev, LVA Ltd.
 
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

/// inits the relevant AVR subsystems related to ADC and voltage readout
void init_ADC(void);

/// fetches the current battery voltage, sampled over a 20ms interval.
/// return value is in millivolts
uint16_t battery_get_voltage(void);

/// checks if the battery voltage falls below a threshold (~2.2V), and, if so,
/// emit a loud alarm and display a message:
void battery_check_voltage(void);
