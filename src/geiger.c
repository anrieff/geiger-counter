/*
	Title: Geiger Counter with Serial Data Reporting and display
	Description: This is the firmware for the mightyohm.com Geiger Counter.
		There is more information at http://mightyohm.com/geiger and http://LVA.bg/products/geiger-counter
		
	Author(s):	Jeff Keyzer, Veselin Georgiev
	Company:	LVA Ltd. (the original kit is available from MightyOhm Engineering)
	Website:	http://LVA.bg/
	Contact(s):	jeff <at> mightyohm.com, anrieff <at> gmail.com
  
	This firmware controls the ATtiny2313 AVR microcontroller on board the Geiger Counter.
	
	When an impulse from the GM tube is detected, the firmware flashes the LED and produces a short
	beep on the piezo speaker.  It also outputs an active-high pulse (default 100us) on the PULSE pin, which
	is also output via a RCA jack for interception in the Geiger Bot app for iOS devices.
	
	A pushbutton on the PCB can be used to mute the beep or turn off the display. The following 8 states
	can be switched via the button:
	
	        State | Sound | Display |
	--------------+-------+---------+
	  0 (default) |    On |    100% |
	  1           |   Off |    100% |
	  2           |    On |     50% |
	  3           |   Off |     50% |
	  4           |    On |     10% |
	  5           |   Off |     10% |
	  6           |    On |     Off |
	  7           |   Off |     Off |
	--------------+-------+---------+
	
	After 7, the next state is again 0.
	It is a good idea to turn off the display if it's not needed, as it drains significant power.
	
	Equivalent dose, in uSv/h, is written to the display. Being a 4-digit display, the max viewable value
	is 9999 uSv/h, i.e. 9.999 mSv/h. This is a high radiation level, and exposure to that (or higher) amount of
	radiation should be alarming (the dose is not lethal for durations of less than a day, but is almost
	certainly lethal for a month or more).

	A running average of the detected counts per second (CPS), counts per minute (CPM), and equivalent dose
	(uSv/hr) is output on the serial port once per second. The dose is based on information collected from 
	the web, and may not be accurate.
	
	The serial port is configured for BAUD baud, 8-N-1 (default 9600).
	
	The data is reported in comma separated value (CSV) format:
	CPS, #####, CPM, #####, uSv/hr, ###.##, SLOW|FAST|INST
	
	There are three modes.  Normally, the sample period is LONG_PERIOD (default 60 seconds).
	This is SLOW averaging mode.
	If the last five measured counts exceed a preset threshold, the sample period switches to
	SHORT_PERIOD seconds (default 5 seconds).
	This is FAST mode, and is more responsive but less accurate. Finally, if CPS > 255, we report CPS*60 and switch to
	INST mode, since we can't store data in the (8-bit) sample buffer.  This behavior could be customized to suit a particular
	logging application.
	
	The largest CPS value that can be displayed is 65535, but the largest value that can be stored in the sample buffer is 255.
	
	***** WARNING *****
	This Geiger Counter is for EDUCATIONAL PURPOSES ONLY.  Don't even think about using it to monitor radiation in
	life-threatening situations, or in any environment where you may expose yourself to dangerous levels of radiation.
	Don't rely on the collected data to be an accurate measure of radiation exposure! Be safe!
	
	
	Change log:
	(jeff)  8/4/11 1.00: Initial release for Chaos Camp 2011!
	(vesko) 8/1/15 1.50: Initial release of the upgraded version.

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

// Includes
#include <avr/io.h>			// this contains the AVR IO port definitions
#include <avr/interrupt.h>	// interrupt service routines
#include <avr/pgmspace.h>	// tools used to store variables in program memory
#include <avr/sleep.h>		// sleep mode utilities
#include <util/delay.h>		// some convenient delay functions
#include <stdlib.h>			// some handy functions like ultoa()
#include "display.h"        // code to drive the 7-segment display
#include "pinout.h"

// Defines
#define VERSION			"2.00"
#define URL				"http://LVA.bg/products/geiger-counter"

#define	BAUD			9600	// Serial BAUD rate
#define SER_BUFF_LEN	11		// Serial buffer length
#define THRESHOLD		1000	// CPM threshold for fast avg mode
#define LONG_PERIOD		50		// # of samples to keep in memory in slow avg mode
#define SHORT_PERIOD	5		// # of samples for fast avg mode
#define SCALE_FACTOR	57		//	CPM to uSv/hr conversion factor (x10,000 to avoid float)
#define PULSEWIDTH		100		// width of the PULSE output (in microseconds)
#define CPU_MHZ	(F_CPU/1000000) // MCU speed in MHz. Default is 8, but might be different

// Function prototypes
void uart_putchar(char c);			// send a character to the serial port
void uart_putstring(const char *buffer);		// send a null-terminated string in SRAM to the serial port
void uart_putstring_P(const char *buffer);	// send a null-terminated string in PROGMEM to the serial port

void checkevent(void);	// flash LED and beep the piezo
void sendreport(void);	// log data over the serial port

// Global variables
volatile uint8_t nobeep = 0;		// flag used to mute beeper
volatile uint8_t disp_state = 0;    // display state, [0..7]
volatile uint8_t statechange = 0;   // display state was recently changed
volatile uint16_t count = 0;		// number of GM events that has occurred
volatile uint16_t slowcpm = 0;		// GM counts per minute in slow mode
volatile uint16_t fastcpm = 0;		// GM counts per minute in fast mode
volatile uint16_t cps = 0;			// GM counts per second, updated once a second
volatile uint8_t overflow = 0;		// overflow flag
volatile uint8_t eventflag = 0;	// flag for ISR to tell main loop if a GM event has occurred
volatile uint8_t tick = 0;		// flag that tells main() when 1 second has passed
volatile uint16_t total_count = 0; // total GM count from device startup
uint8_t buffer[LONG_PERIOD];	// the sample buffer

char serbuf[SER_BUFF_LEN];	// serial buffer
uint8_t mode;				// logging mode, 0 = slow, 1 = fast, 2 = inst

enum DisplayMode {
	RADIATION,
	COUNTER,
} display_mode;

// Interrupt service routines

//	Pin change interrupt for pin INT0
//	This interrupt is called on the falling edge of a GM pulse.
ISR(INT0_vect)
{
	if (count < UINT16_MAX)	// check for overflow, if we do overflow just cap the counts at max possible
		count++; // increase event counter
	if (total_count < UINT16_MAX)
		total_count++;

	// send a pulse to the PULSE connector
	// a delay of 100us limits the max CPS to about 8000
	// you can comment out this code and increase the max CPS possible (up to 65535!)
	PULSE_PORT |= _BV(PULSE_BIT);	// set PULSE output high
	_delay_us(PULSEWIDTH);
	PULSE_PORT &= ~(_BV(PULSE_BIT));	// set pulse output low
		
	eventflag = 1;	// tell main program loop that a GM pulse has occurred
}

// this is part of the Timer1 interrupt routine. The ISR calls this code exactly
// once per second.
void once_per_second_tasks(void)
{
	static uint8_t idx;					// sample buffer index
	static uint16_t fastsum;           // running count of the short period counts
	static uint8_t lagging_idx = LONG_PERIOD - SHORT_PERIOD; // fast sample window back index
	// in effect, fastsum holds the sum of buffer[i], where lagging_idx <= i < idx
	uint8_t new_sample;
	tick = 1;	// update flag
	
	//PORTB ^= _BV(PB4);	// toggle the LED (for debugging purposes)
	cps = count;
	
	if (count > UINT8_MAX) {	// watch out for overflowing the sample buffer
		count = UINT8_MAX;
		overflow = 1;
	}
	/*
	 * new_sample is what we add to the running counts. you may wonder why
	 * we truncate it to 8-bit first before adding it, as slowcpm and fastsum
	 * are all 16-bit counters, so the could handle it. However, we
	 * wouldn't be able to subtract it later when we recall its value from
	 * the sample buffer (and making the sample buffer 16-bit is infeasible).
	 */
	new_sample = count;
			
	// subtract oldest sample in sample buffer and add the newest:
	slowcpm -= buffer[idx];
	buffer[idx] = new_sample;	// save current sample to buffer (replacing old value)
	slowcpm += new_sample;
	
	// Compute CPM based on the last SHORT_PERIOD samples:
	fastsum = fastsum + new_sample - buffer[lagging_idx];
	fastcpm = fastsum * (60 / SHORT_PERIOD);
	
	// Move to the next entry in the sample buffer
	if (++idx >= LONG_PERIOD)
		idx = 0;
	if (++lagging_idx >= LONG_PERIOD)
		lagging_idx = 0;
	count = 0;  // reset counter
#ifdef PARANOID
	if (slowcpm > 64000) {
		// assume a bug, which modified buffer[], and slowcpm now holds a "negative" value.
		// we can't do a meaningful recovery here; we just reset the counters:
		slowcpm = fastsum = 0;
		for (new_sample = 0; new_sample < LONG_PERIOD; new_sample++)
			buffer[new_sample] = 0;
	}
#endif
}

void once_per_16ms_tasks(void)
{
	static uint8_t last_button_state = 0;
	uint8_t button_state = !!(BTN_PIN & _BV(BTN_BIT));

	if (button_state != last_button_state) {
		last_button_state = button_state;
		if (button_state == 0) {
			// button has just been released
			disp_state = (disp_state + 1) & 7;	// increment state
			nobeep = disp_state & 1;
			statechange = 1;
		}
	}
}

/*	Timer1 compare interrupt 
 *	This interrupt is called every time TCNT1 reaches OCR1A and is reset back to 0 (CTC mode).
 *  Timer1 is setup so this happens 1000 times per second.
 */
ISR(TIMER1_COMPA_vect)
{
	static uint16_t ms = 0; // where are we within each second (0-999)
	if (++ms == 1000) ms = 0; // warp back

	if (display_on  ) display_tasks(); // update the display
	if (ms == 0)      once_per_second_tasks(); // handle stats gathering
	if (ms % 16 == 0) once_per_16ms_tasks();   // handle button state
}

// Functions

// Send a character to the UART
void uart_putchar(char c)
{
	if (c == '\n') uart_putchar('\r');	// Windows-style CRLF
  
	loop_until_bit_is_set(UCSR0A, UDRE0);	// wait until UART is ready to accept a new character
	UDR0 = c;							// send 1 character
}

// Send a string in SRAM to the UART
void uart_putstring(const char *buffer)	
{	
	// start sending characters over the serial port until we reach the end of the string
	while (*buffer != '\0') {	// are we at the end of the string yet?
		uart_putchar(*buffer);	// send the contents
		buffer++;				// advance to next char in buffer
	}
}

// Send a string in PROGMEM to the UART
void uart_putstring_P(const char *buffer)	
{	
	// start sending characters over the serial port until we reach the end of the string
	while (pgm_read_byte(buffer) != '\0')	// are we done yet?
		uart_putchar(pgm_read_byte(buffer++));	// read byte from PROGMEM and send it
}

// flash LED and beep the piezo
void checkevent(void)
{
	if (eventflag) {		// a GM event has occurred, do something about it!
		eventflag = 0;		// reset flag as soon as possible, in case another ISR is called while we're busy

		LED_PORT |= _BV(LED_BIT);	// turn on the LED
		
		if(!nobeep) {		// check if we're in mute mode
			TCCR0A |= _BV(COM0A0);	// enable OCR0A output on the piezo pin
		}
		
		// 10ms delay gives a nice short flash and 'click' on the piezo
		_delay_ms(10);	
			
		LED_PORT &= ~(_BV(LED_BIT));	// turn off the LED
		
		TCCR0A &= ~(_BV(COM0A0));	// disconnect OCR0A from Timer0
	}	
}

// turn on/off the display
void checkdisplay(void)
{
	if (statechange) {
		statechange = 0;
		switch (disp_state) {
			case 0:
				display_turn_on(); // this implies 100% brightness
				break;
			case 2:
				display_set_brightness(64); // feels like 50%
				break;
			case 4:
				display_set_brightness(6); // feels like 10%
				break;
			case 6:
				display_turn_off();          // 0%, hehe
				break;
		}
	}
}

// log data over the serial port
void sendreport(void)
{
	uint32_t cpm;	// This is the CPM value we will report
	if(tick) {	// 1 second has passed, time to report data via UART
		tick = 0;	// reset flag for the next interval
			
		if (overflow) {
			cpm = cps*60UL;
			mode = 2;
			overflow = 0;
		}				
		else if (fastcpm > THRESHOLD) {	// if cpm is too high, use the short term average instead
			mode = 1;
			cpm = fastcpm;	// report cpm based on last 5 samples
		} else {
			mode = 0;
			// report cpm based on last 50 seconds; scale to represent a full minute:
			cpm = (uint32_t) slowcpm * 6 / 5;
		}
		
		// Send CPM value to the serial port
		uart_putstring_P(PSTR("CPS, "));
		ultoa(cps, serbuf, 10);		// radix 10
		uart_putstring(serbuf);
			
		uart_putstring_P(PSTR(", CPM, "));
		ultoa(cpm, serbuf, 10);		// radix 10
		uart_putstring(serbuf);
			
		uart_putstring_P(PSTR(", uSv/hr, "));
	
		// calculate uSv/hr based on scaling factor, and multiply result by 100
		// so we can easily separate the integer and fractional components (2 decimal places)
		uint32_t usv_scaled = (uint32_t)(cpm*SCALE_FACTOR)/100;	// scale and truncate the integer part
			
		// this reports the integer part
		ultoa((uint16_t)(usv_scaled/100), serbuf, 10);	
		uart_putstring(serbuf);
			
		uart_putchar('.');
			
		// this reports the fractional part (2 decimal places)
		uint8_t fraction = usv_scaled % 100;
		if (fraction < 10)
			uart_putchar('0');	// zero padding for <0.10
		ultoa(fraction, serbuf, 10);
		uart_putstring(serbuf);

		// Tell us what averaging method is being used
		if (mode == 2) {
			uart_putstring_P(PSTR(", INST"));
		} else if (mode == 1) {
			uart_putstring_P(PSTR(", FAST"));
		} else {
			uart_putstring_P(PSTR(", SLOW"));
		}
			
		// We're done reporting data, output a newline.
		uart_putchar('\n');	
		
#ifdef DEBUG
		uint8_t i;
		for (i = LONG_PERIOD - 5; i < LONG_PERIOD + 10; i++) {
				ultoa(buffer[i] + 100, serbuf, 10);
				uart_putstring(serbuf);
		}
		uart_putchar('\n');
#endif
		
		if (disp_state < 6) {
			if (display_mode == RADIATION)
				display_radiation(usv_scaled);
			else
				display_counts(total_count);
		}
	}	
}

// Start of main program
int main(void)
{	
	// Configure the UART	
	// Set baud rate generator based on F_CPU
	UBRR0H = (unsigned char)((F_CPU/(16UL*BAUD)-1)>>8);
	UBRR0L = (unsigned char) (F_CPU/(16UL*BAUD)-1);
	
	// Enable USART transmitter and receiver
	UCSR0B = (1<<RXEN0) | (1<<TXEN0);

	uart_putstring_P(PSTR("LVA Geiger Counter " VERSION "\n" URL "\n"));

	// Set up AVR IO ports
	DDRB = CONF_DDRB;
	DDRC = CONF_DDRC;
	DDRD = CONF_DDRD;
	BTN_WPU |= _BV(BTN_BIT);	// enable internal pull up resistor on pin connected to button

	// is the button pressed on startup? if so, the display should show counts, not uSv/h:
	_delay_ms(1);
	display_mode = ((BTN_PIN & _BV(BTN_BIT)) != 0) ? RADIATION : COUNTER;
	
	// Set up external interrupts	
	// INT0 is triggered by a GM impulse
	EICRA |= _BV(ISC01);	// Config interrupts on falling edge of INT0
	EIMSK |= _BV(INT0);		// Enable external interrupts on pins INT0
	
	// Configure the Timers
	// Set up Timer0 for tone generation and PWM of display. When both compare units are on, the operation is:
	// 1) Timer increments happen at fosc/8, i.e. 1.0 us or 1.333 us (on 8MHz/6MHz crystal, respectively).
	// 2) Counter increases until 160 (@8MHz) or 120 (@6MHz) and then resets (fast PWM mode, #7)
	// 3) At each reset, toggle OC0A (pin PD6 - Piezo)
	// 4) PWM the OC0B (pin PD5 - Global display FET) in non-inverting configuration.

	// Actually, the setup here is simpler: we just select WGM = 7 (fast PWM with reset@OCR0A),
	// and fosc/8 increments. The respective pins are connected to Timer0 as required:
	TCCR0A = (1 << WGM01) | (1 << WGM00);
	TCCR0B = (1 << WGM02) | (1 << CS01);
	OCR0A = 20 * CPU_MHZ;
	OCR0B = OCR0A / 2; // 50% brightness

	// Set up Timer1 for 1 ms interrupts
	TCCR1B = _BV(WGM12) | _BV(CS11);  // CTC mode, prescaler = 8 (1 or 1.3333 us ticks)
	OCR1A = 125 * CPU_MHZ;	// 8MHz: 1us * 1000 = 1 ms; 6MHz: 1.33333 us * 750 = 1ms
	TIMSK1 = _BV(OCIE1A);  // Timer1 overflow interrupt enable

	display_turn_on();
	display_show_revision();
#ifdef DEBUG
	uint8_t i = 0;
	for (i = 0; i < 10; i++) buffer[LONG_PERIOD + i] = 11;
#endif
	sei();	// Enable interrupts

	while(1) {	// loop forever
		
		// Configure AVR for sleep, this saves a couple mA when idle
		set_sleep_mode(SLEEP_MODE_IDLE);	// CPU will go to sleep but peripherals keep running
		sleep_enable();		// enable sleep
		sleep_cpu();		// put the core to sleep
		
		// Zzzzzzz...	CPU is sleeping!
		// Execution will resume here when the CPU wakes up.
		
		sleep_disable();	// disable sleep so we don't accidentally go to sleep
		
		checkevent();	// check if we should signal an event (led + beep)

		checkdisplay(); // check if we need to do something with the LED display
	
		sendreport();	// send a log report over serial
		
		checkevent();	// check again before going to sleep
		
	}	
	return 0;	// never reached
}

// kate: tab-width 4
