#include <stdio.h>

typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned int uint32_t;
 // these are OR-able masks for display_set_dots()
enum {
    DP1 = 1,
    DP2 = 2,
    DP3 = 4,
    DP4 = 8,            // decimal points
    DIVIDED = 16,       // the central two dots
    APOSTROPHE = 32     // the apostrophe between characters 3 and 4
};

// ------------------ simulator part ----------------------------------
uint8_t g_dots;
char s[20];
uint8_t si;
static void display_set_dots(uint8_t dots)
{
	g_dots = dots;
}

static void display_spi_byte(uint8_t byte)
{
	s[si++] = byte;
	if (si == 4) si = 0;
}

static void render(void)
{
	printf("[%c", s[0]);
	if (g_dots & DP1) putchar('.');
	printf("%c", s[1]);
	if (g_dots & DP2) putchar('.');
	if (g_dots & DIVIDED) putchar(':');
	printf("%c", s[2]);
	if (g_dots & APOSTROPHE) putchar('\'');
	if (g_dots & DP3) putchar('.');
	printf("%c", s[3]);
	if (g_dots & DP4) putchar('.');
	printf("]\n");
}

void utoa(uint32_t value, char* buff, int radix)
{
	int i = 0;
	do {
		buff[i++] = "0123456789abcdef"[value % radix];
		value /= radix;
	} while (value);
	int j = 0;
	buff[i--] = 0;
	while (j < i) {
		char tmp = buff[i];
		buff[i] = buff[j];
		buff[j] = tmp;
		i--, j++;
	}
}
#define ultoa utoa

// / end of simulator part --------------------------------------------

uint8_t display_on = 1;

void display_int_value(uint32_t x, int8_t dp)
{
	char buff[12];
	uint8_t i;
	if (x < 1000) {
		utoa(x + 1000, buff, 10);
		buff[0] = dp ? ' ' : '0';
	} else {
		ultoa(x, buff, 10);
	}
	char* s = buff + 4;
	while (*(s++)) dp--;
	if (dp < 0) {
		buff[0] = '-';
		buff[1] = 'O';
		buff[2] = 'L';
		buff[3] = '-';
	}
	for (i = 0; i < 4; i++)
		display_spi_byte(buff[i]);
	display_set_dots((8 >> dp) & (DP2|DP3));
}


void display_show_radiation(uint32_t value)
{
	display_int_value(value, 2);
}

void display_show_counts(uint32_t counts)
{
	display_int_value(counts, 0);
}


int main(void)
{
	int values[] = { 0, 1, 15, 128, 1024, 9891, 12345, 150900, 1357912, 11235671 };

	for (int i = 0; i < (int) sizeof(values) / sizeof(values[0]); i++) {
		printf("display_show_radiation(%d) -> ", values[i]);
		display_show_radiation(values[i]);
		render();
	}
	for (int i = 0; i < (int) sizeof(values) / sizeof(values[0]); i++) {
		printf("display_show_counts(%d) -> ", values[i]);
		display_show_counts(values[i]);
		render();
	}

	return 0;
}
