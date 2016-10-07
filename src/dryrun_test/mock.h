/*
 * This file provides declarations for mock AVR functions, so we can compile
 * and test the logging code on a PC.
 */
#ifndef __MOCK_H__
#define __MOCK_H__

#include <stdint.h>

#define PSTR(s) (s)

#define ISR(handler) void handler(void)
#define cli()
#define sei()

uint8_t eeprom_read_byte(uint8_t* address);
uint16_t eeprom_read_word(uint16_t* address);

void eeprom_update_byte(uint8_t* address, uint8_t data);
void eeprom_update_word(uint16_t* address, uint16_t data);

uint8_t mock_UDR0(void);

#define UDR0 mock_UDR0()

void init_mock(void);

void send_command(const char* cmd);


#endif // __MOCK_H__
