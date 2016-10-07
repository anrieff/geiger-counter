#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "mock.h"
#include "logging.h"
#include "pc_link.h"

const char* USAGE = 
"Device commands (case sensitive):\n"
"\tHELO, STATUS, RESET, CLOG, RSLOG, REELOG, GETID, SID <number>, GETTM,\n"
"\tSTMN <number> (planned, not yet implemented),\n"
"\tSTMD <number> (planned, not yet implemented).\n\n"

"Simulator commands:\n"
"\thelp, exit, addsamples <count>, setrad <radiation> [uSv|mSv|Sv],\n"
"\tdc, battery.";


double radiation = 0.14; // uSv/h
int voltage_baseline = 3003;
int battery_sim = 0;

int poisson_sample(void)
{
	// using the algorithm by Junhao, based on Knuth:
	double lambda = radiation * 175.4 / 2; // 30 seconds, SBM-20
	double lambdaLeft = lambda, p = 1;
	const double e = exp(1.0);
	int k = 0;
	double STEP = 500;
	do {
		k++;
		p *= drand48();
		if (p < e && lambdaLeft > 0) {
			if (lambdaLeft > STEP) {
				p *= exp(STEP);
				lambdaLeft -= STEP;
			} else {
				p *= exp(lambdaLeft);
				lambdaLeft = -1;
			}
		}
	} while (p > 1);
	return k - 1;
}

uint16_t voltage_sample_dc(void)
{
	return voltage_baseline + rand() % 45;   
}

uint16_t voltage_sample_battery(void)
{
	return voltage_sample_dc(); //TODO
}

uint16_t voltage_sample(void)
{
	if (battery_sim)
		return voltage_sample_battery();
	else
		return voltage_sample_dc();
}

void repl()
{
	char line[200];
	while (fgets(line, sizeof(line), stdin)) {
		if (isupper(line[0])) {
			send_command(line);
		} else {
			if (!strncmp(line, "addsamples ", 11)) {
				int numsamples;
				if (1 == sscanf(line, "addsamples %d", &numsamples)) {
					for (int i = 0; i < numsamples; i++) {
						logging_add_data_point(poisson_sample(), voltage_sample());
					}
				}
			} else if (!strncmp(line, "setrad", 6)) {
				double x;
				if (1 == sscanf(line, "setrad %lf", &x)) {
					if (strstr(line, "mSv"))
						x *= 1e3;
					else if (strstr(line, " Sv"))
						x *= 1e6;
					radiation = x;
					printf("Radiation set to %.8lf Sv/h\n", radiation / 1e6);
				}
			} else if (!strncmp(line, "help", 4)) {
				puts(USAGE);
			} else if (!strncmp(line, "exit", 4)) {
				break;
			} else {
				printf("Bad command!\n");
			}
		}
	}
}

int main(void)
{
	init_mock();
	logging_init();
	pc_link_init();


	repl();
	
	return 0;
}
