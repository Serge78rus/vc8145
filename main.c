/*
 * main.c
 *
 *  Created on: 14 авг. 2021 г.
 *      Author: serge78rus
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>

#include "vc8145.h"
#include "options.h"

//private variables
static volatile bool keep_running = true;

//private functions
static inline void show_seconds();
static inline void show_time(const char *format, bool milliseconds);
static void int_handler(int dummy);

int main(int argc, char **argv)
{
	signal(SIGINT, int_handler);

	struct Options *options = options_parse(argc, argv);
	if (!options) {
		fprintf(stderr, "Parsing command line error: %s\n", options_err_msg);
		options_help();
		return 1;
	}
	if (options->help_flag) {
		options_help();
		return 0;
	}
	if (options->version_flag) {
		options_version();
		return 0;
	}
	if (options->verbose_flag) {
		options_print();
	}

	unsigned int cycle_us = options->cycle_s * 1000000;

	char time_format[80] = {0}; //TODO real size
	if (options->time_flag) {
		if (options->date_format) {
			strcpy(time_format, options->date_format);
			strcat(time_format, " ");
		}
		strcat(time_format, "%T");
	}

	if (!vc8145_open(options->serial_device)) {
		fprintf(stderr, "Communication device initialization error: %s\n", vc8145_err_msg);
		vc8145_close();
		return 1;
	}

	int cycle_count = 0;
	while (keep_running) {

	    static struct timeval start_time;
	    gettimeofday(&start_time , NULL);

	    if (options->seconds_flag) {
		    show_seconds();
	    }

		if (options->time_flag) {
			show_time(time_format, true); //TODO milliseconds???
		}

		char *str = vc8145_read(options->mode_flag, options->unit_flag);
		if (str) {
			fprintf(stdout, "%s\n", str);
			fflush(stdout);
		} else {
			fprintf(stdout, "\n");
			if (keep_running && !vc8145_err_msg[0]) {
				fprintf(stderr, "Read VC8145 data unknown error\n");
			}
		}

		if (keep_running && vc8145_err_msg[0]) {
			fprintf(stderr, "Read VC8145 data error: %s\n", vc8145_err_msg);
		}

		if (options->cycles_number) {
			if (++cycle_count == options->cycles_number) {
				break;
			}
		}

	    static struct timeval end_time;
	    gettimeofday(&end_time , NULL);

	    unsigned int cycle_corrections = (end_time.tv_sec - start_time.tv_sec) * 1000000;
	    cycle_corrections = (end_time.tv_usec + cycle_corrections) - start_time.tv_usec;
	    if (cycle_us > cycle_corrections) {
			usleep(cycle_us - cycle_corrections);
	    }
	}

	vc8145_close();

	return 0;
}

/*
 * private functions
 */

static inline void show_seconds()
{
	static bool first_cycle = true;
    static struct timeval now_time;
    static struct timeval start_time;

    gettimeofday(&now_time , NULL);
    if (first_cycle) {
    	first_cycle = false;
    	memcpy(&start_time, &now_time, sizeof(struct timeval));
    }

    if (now_time.tv_usec < start_time.tv_usec) {
    	now_time.tv_sec -= 1;
    	now_time.tv_usec += 1000000;
    }
	fprintf(stdout, "%u.%03u ", (unsigned int)(now_time.tv_sec - start_time.tv_sec),
			(unsigned int)(now_time.tv_usec - start_time.tv_usec) / 1000);
}

static inline void show_time(const char *format, bool milliseconds)
{
    static struct timeval now_time;
	static struct tm now_localtime;
	static char time_buff[80]; //TODO real size

    gettimeofday(&now_time , NULL);
	localtime_r(&(now_time.tv_sec), &now_localtime);
	size_t len = strftime(time_buff, sizeof(time_buff), format, &now_localtime);
	if (milliseconds) {
		snprintf(time_buff + len, sizeof(time_buff) - len, ".%03u", (unsigned)now_time.tv_usec / 1000);
	}
	fprintf(stdout, "%s ", time_buff);
}

static void int_handler(int dummy)
{
    keep_running = false;
}

