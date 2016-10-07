
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "pc_link.h"

static int battery_baseline = 3015;
static time_t clk0 = 0;
static char next_char;

static void init_eeprom(void)
{
	FILE* f = fopen("eeprom", "wb");
	for (int i = 0; i < 512; i++)
		fputc(0, f);
	fclose(f);
}

static FILE* open_eeprom(void)
{
	FILE* f = fopen("eeprom", "rb");
	if (!f) init_eeprom();
	else fclose(f);

	return fopen("eeprom", "r+b");
}

uint8_t eeprom_read_byte(uint8_t* addr)
{
	FILE* f = open_eeprom();
	fseek(f, (unsigned) (uint64_t) addr, SEEK_SET);
	uint8_t ret = fgetc(f);
	fclose(f);
	return ret;
}

uint16_t eeprom_read_word(uint16_t* addr)
{
	FILE* f = open_eeprom();
	fseek(f, (unsigned) (uint64_t) addr, SEEK_SET);
	uint16_t ret;
	fread(&ret, 2, 1, f);
	fclose(f);
	return ret;
}

void eeprom_update_byte(uint8_t* addr, uint8_t value)
{
	FILE* f = open_eeprom();
	fseek(f, (unsigned) (uint64_t) addr, SEEK_SET);
	fwrite(&value, 1, 1, f);
	fclose(f);
}

void eeprom_update_word(uint16_t* addr, uint16_t value)
{
	FILE* f = open_eeprom();
	fseek(f, (unsigned) (uint64_t) addr, SEEK_SET);
	fwrite(&value, 2, 1, f);
	fclose(f);
}

uint16_t battery_get_voltage(void)
{
	return battery_baseline + rand() % 45;   
}

void init_mock(void)
{
	clk0 = time(NULL);
}

uint32_t get_uptime_seconds(void)
{
	return time(NULL) - clk0;
}

void uart_print_number(uint32_t x)
{
	printf("%u", x);
}

void uart_putchar(char c)
{
	printf("%c", c);
}

void uart_putstring(const char* s)
{
	printf("%s", s);
}

void uart_putstring_P(const char* s)
{
	printf("%s", s);
}

char mock_UDR0(void)
{
	return next_char;
}

void USART_RX_vect(void);

void send_command(const char* cmd)
{
	for (int i = 0; cmd[i]; i++) {
		next_char = cmd[i];
		USART_RX_vect();
	}
	pc_link_check();
}
