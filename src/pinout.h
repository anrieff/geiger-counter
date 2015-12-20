/*
	Title: Geiger Counter with Serial Data Reporting and display
	Description: Pinout of the Atmega88a
		
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
#ifndef __PINOUT_H__
#define __PINOUT_H__

/*
+-----+-------------+-------------------+
| Pin | Designation |     Purpose       |
+-----+-------------+-------------------+
|   1 | PD3/INT1    | Button (on WPU)   |
|   2 | PD4         | Seg DP            |
|   3 | Gnd         | .                 |
|   4 | Vcc         | v                 |
|   5 | Gnd         | .                 |
|   6 | Vcc         | v                 |
|   7 | PB6/XTAL    | XTAL              |
|   8 | PB7/XTAL    | XTAL              |
|   9 | PD5/OC0B    | Global Disp. FET  |
|  10 | PD6/OC0A    | Piezo [pwm]       |
|  11 | PD7         | LED               |
|  12 | PB0         | Seg G             |
|  13 | PB1         | Pulse header      |
|  14 | PB2         | FET digit 0       |
|  15 | PB3/MOSI    | FET digit 1       |
|  16 | PB4/MISO    | FET digit 2       |
|  17 | PB5/SCK     | FET digit 3       |
|  18 | AVcc        | v                 |
|  19 | Adc6        | .                 |
|  20 | Aref        | .                 |
|  21 | Gnd         | .                 |
|  22 | Adc7        | Battery level     |
|  23 | PC0         | Seg A             |
|  24 | PC1         | Seg B             |
|  25 | PC2         | Seg C             |
|  26 | PC3         | Seg D             |
|  27 | PC4         | Seg E             |
|  28 | PC5         | Seg F             |
|  29 | PC6/RESET   | Reset/programming |
|  30 | PD0/RX      | UART              |
|  31 | PD1/TX      | UART              |
|  32 | PD2/INT0    | GM_Pulse          |
+-----+-------------+-------------------+
*/

#define CONF_DDRB 0xff // all output
#define CONF_DDRC 0x3f // PC0-PC5 are segments A-F - output
#define CONF_DDRD 0xf3 // all output except PD2 & PD3

// the button:
#define BTN_PIN	PIND
#define BTN_WPU PORTD
#define BTN_BIT PD3

#define keypressed() (!(BTN_PIN & _BV(BTN_BIT)))

// the LED:
#define LED_PORT PORTD
#define LED_BIT PD7

// the PULSE header (and chinch output):
#define PULSE_PORT PORTB
#define PULSE_BIT PB1

// the global display FET:
#define GFET_PORT PORTD
#define GFET_BIT PD5

// output the bitmask 'x' on display segs A-G (A = bit 0, B = bit 1, etc...);
#define DISP_OUT_CHAR(x) \
	PORTC = (x) & 0x3f;  \
	PORTB = (PORTB & 0xfe) | ((x >> 6) & 1)

// output the decimal point (dp = 0: no decimal point; dp = 1: has decimal point);
#define DISP_OUT_DP(dp) \
	PORTD = (PORTD & 0xef) | (dp << 4)

// blanks the display:
#define DISP_BLANK() \
	PORTB &= 0xc3

// sets the actively shown digit, 0-3 (by manipulating FETs):
#define DISP_ACTIVE_DIGIT(digit) \
	PORTB = (PORTB & 0xc3) | (4 << (digit & 3))

#endif // __PINOUT_H__
