#include <stdio.h>
#include <string.h>
#include "../src/characters.h"

typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
#define THRESHOLD		1000	// CPM threshold for fast avg mode
#define LONG_PERIOD		60		// # of samples to keep in memory in slow avg mode
#define SHORT_PERIOD	5		// # of samples for fast avg mode
#define SCALE_FACTOR	57		//	CPM to uSv/hr conversion factor (x10,000 to avoid float)

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
uint8_t si;
uint8_t display[4];
int xoff = 0;
static void display_set_dots(uint8_t dots)
{
	g_dots = dots;
}

static char dbuffer[5][80];

static void hpaint(int x, int y)
{
	for (int i = 0; i < 3; i++)
		dbuffer[y][xoff + x + i] = '#';
}

static void vpaint(int x, int y)
{
	for (int i = 0; i < 3; i++)
		dbuffer[y + i][xoff + x] = '#';
}

static void render(void)
{
	memset(dbuffer, ' ', sizeof(dbuffer));
	for (int i = 0; i < 5; i++) strcpy(&dbuffer[i][72], "\n");
	for (int digit = 0; digit < 4; digit++) {
		xoff = digit * 5;
		uint8_t d = display[digit];
		if (d &   1) hpaint(0, 0);
		if (d &   2) vpaint(2, 0);
		if (d &   4) vpaint(2, 2);
		if (d &   8) hpaint(0, 4);
		if (d &  16) vpaint(0, 2);
		if (d &  32) vpaint(0, 0);
		if (d &  64) hpaint(0, 2);
		if (g_dots & ( 1 << digit ) ) dbuffer[4][xoff + 3] = '.';
	}
	for (int i = 0; i < 5; i++) printf("%s", dbuffer[i]);
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

char serbuf[11];

// / end of simulator part --------------------------------------------

const uint8_t DIGIT_MASKS[10] = { num0, num1, num2, num3, num4, num5, num6, num7, num8, num9 };

static void display_int_value(uint32_t x, int8_t dp, uint8_t dp_mask)
{
	uint8_t i;

	if (x < 1000) {
		ultoa(x + 1000, serbuf, 10);
		serbuf[0] = ' ';
		if (dp == 0) {
			if (serbuf[1] == '0') {
				serbuf[1] = ' ';
				if (serbuf[2] == '0')
					serbuf[2] = ' ';
			}
		}		
	} else {
		ultoa(x, serbuf, 10);
	}
	char* s = serbuf + 4;
	while (*(s++)) dp--;
	if (dp < 0) {
		display[0] = cDASH; // '-'
		display[1] = cO;    // 'O'
		display[2] = cL;    // 'L'
		display[3] = cDASH; // '-'
		display_set_dots(0);
		return;
	}
	for (i = 0; i < 4; i++)
		display[i] = (serbuf[i] == ' ') ? mEMPTY : DIGIT_MASKS[serbuf[i] - '0'];
	display_set_dots((8 >> dp) & dp_mask);
}

void display_radiation(uint32_t uSv_mul_100)
{
	display_int_value(uSv_mul_100, 2, (DP2|DP3));
}

void display_counts(uint32_t counts)
{
	if (counts <= 9999)
		display_int_value(counts, 0, 0);
	else
		display_int_value(counts / 10, 2, (DP2|DP3|DP4));
}

int main(void)
{
	int values[] = { 0, 1, 15, 128, 1024, 9891, 12345, 150900, 1357912, 11235671 };

	for (int i = 0; i < int(sizeof(values) / sizeof(values[0])); i++) {
		printf("\ndisplay_show_radiation(%d):\n", values[i]);
		display_radiation(values[i]);
		render();
	}
	for (int i = 0; i < int(sizeof(values) / sizeof(values[0])); i++) {
		printf("\ndisplay_show_counts(%d):\n", values[i]);
		display_counts(values[i]);
		render();
	}
	
	return 0;
}
