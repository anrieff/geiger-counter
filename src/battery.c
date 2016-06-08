/*
	Title: Geiger Counter with Serial Data Reporting and display
	Description: Code for reading out battery voltage (Vref/Vcc method).
		
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

#include <avr/io.h>			// this contains the AVR IO port definitions
#include <avr/pgmspace.h>	// tools used to store variables in program memory
#include <util/delay.h>		// some convenient delay functions
#include <stdlib.h>
#include "pinout.h"
#include "display.h"
#include "characters.h"

const uint16_t LOW_VOLTAGE_THRESHOLD = 2200; // millivolts

void init_ADC(void)
{
	ADMUX = 0x4e; // ADC referenced by AVcc, right-adjusted, selected channel is 1.1V bandgap
	ADCSRA = 0x06; // ADC prescaler set to Fosc/64

}

uint16_t battery_get_voltage(void)
{
	uint32_t sum = 0;
	uint8_t i;
	uint16_t adc_res;
	// turn on the ADC:
	ADCSRA |= 0x80;
	_delay_us(500);


	// make 160 conversions and average them:
	for (i = 0; i < 160; i++) {
		ADCSRA |= 0x40;
		while (ADCSRA & 0x40)  ;
		adc_res = ADCW;
		sum += adc_res;
	}

	ADCSRA &= ~0x80; // turn off ADC to save power
	
	uint16_t average = sum / 160;

	// average = (1.1 bandgap) / (battery voltage) * 1024;
	// battery_voltage * 1000 = 1100 * 1024 / average
	#define Vbandgap_mV 1065 /* this is on our current batch of atmegas */
	uint16_t battery_mV = (uint32_t) Vbandgap_mV * 1024 / average;
	return battery_mV;
}

static void beep(uint8_t char1, uint8_t char2, uint8_t char3, uint8_t char4, uint8_t dp_mask)
{
	if (display_on) {
		display[0] = char1;
		display[1] = char2;
		display[2] = char3;
		display[3] = char4;
		display_set_dots(dp_mask);
	}
	// emit a 400ms beep, followed by a 100 ms silence and blank display:
	TCCR0A |= _BV(COM0A0);	// enable OCR0A output on the piezo pin
	_delay_ms(400);

	TCCR0A &= ~(_BV(COM0A0));	// disconnect OCR0A from Timer0
	if (display_on)	display_clear();
	_delay_ms(100);
}

void battery_check_voltage(void)
{
	if (battery_get_voltage() < LOW_VOLTAGE_THRESHOLD) {
		beep(cB, cA,  cT, cT, DP4); // "bAtt."
		beep( 0, cL, c_o,  0, DP3); // " Lo. "
	}
}
