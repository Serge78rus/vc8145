/*
 * options.c
 *
 *  Created on: 16 авг. 2021 г.
 *      Author: serge78rus
 */

#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "options.h"
#include "def.h"

//global variables
char options_err_msg[80] = {0}; //TODO real value

/*
 * local variables
 */

static struct option long_options[] = {
	    {.name = "help", 			.has_arg = no_argument, 		.flag = 0, .val = 'h'},
	    {.name = "version",			.has_arg = no_argument, 		.flag = 0, .val = 'V'},
	    {.name = "verbose", 		.has_arg = no_argument, 		.flag = 0, .val = 'v'},
	    {.name = "seconds",			.has_arg = no_argument, 		.flag = 0, .val = 's'},
	    {.name = "time", 			.has_arg = no_argument, 		.flag = 0, .val = 't'},
	    {.name = "date", 			.has_arg = required_argument,	.flag = 0, .val = 'd'},
	    {.name = "mode", 			.has_arg = no_argument,			.flag = 0, .val = 'm'},
	    {.name = "unit", 			.has_arg = no_argument,			.flag = 0, .val = 'u'},
	    {.name = "cycle", 			.has_arg = required_argument,	.flag = 0, .val = 'c'},
	    {.name = "number", 			.has_arg = required_argument,	.flag = 0, .val = 'n'},
		{0, 0, 0, 0}
};

static struct Options options = {
		.help_flag = false,
		.version_flag = false,
		.verbose_flag = false,
		.seconds_flag = false,
		.time_flag = false,
		.date_format = 0,
		.mode_flag = false,
		.unit_flag = false,
		.cycle_s = 1.0,
		.cycles_number = 0,

		.serial_device = 0
};

/*
 * public functions
 */

struct Options* options_parse(int argc, char **argv)
{
	options_err_msg[0] = 0;

	for (;;) {
		int option_index = 0;

		int c = getopt_long(argc, argv, "hVvstd:muc:n:", long_options, &option_index);
		if (c == -1) {
			break;
		}

		switch (c) {
		case 0:
			if (long_options[option_index].flag != 0) {
	            break;
			} else if (optarg) {
				snprintf(options_err_msg, sizeof(options_err_msg), "Option \"%s\" with argument \"%s\"",
						long_options[option_index].name, optarg);
			} else {
				snprintf(options_err_msg, sizeof(options_err_msg), "Option \"%s\" without argument",
						long_options[option_index].name);
			}
			return 0;

		case 'h':
			options.help_flag = true;
			break;
		case 'V':
			options.version_flag = true;
			break;
		case 'v':
			options.verbose_flag = true;
			break;
		case 's':
			options.seconds_flag = true;
			break;
		case 't':
			options.time_flag = true;
			break;
		case 'd':
			options.date_format = optarg;
			break;
		case 'm':
			options.mode_flag = true;
			break;
		case 'u':
			options.unit_flag = true;
			break;
		case 'c': {
			char *end_ptr = optarg;
			options.cycle_s = strtod(optarg, &end_ptr);
			if (end_ptr == optarg || *end_ptr) {
				snprintf(options_err_msg, sizeof(options_err_msg), "Option \"%s\" argument is invalid floating point value \"%s\"",
						long_options[option_index].name, optarg);
				return 0;
			}
			break;
		}
		case 'n': {
			char *end_ptr = optarg;
			options.cycles_number = strtoul(optarg, &end_ptr, 10);
			if (end_ptr == optarg || *end_ptr) {
				snprintf(options_err_msg, sizeof(options_err_msg), "Option \"%s\" argument is invalid unsigned integer value \"%s\"",
						long_options[option_index].name, optarg);
				return 0;
			}
			break;
		}


		default:
			snprintf(options_err_msg, sizeof(options_err_msg), "Unknown getopt_long() error");
			return 0;
		}
	}

	if (optind >= argc) {
		snprintf(options_err_msg, sizeof(options_err_msg), "Communication serial device not specified");
		return 0;
	}

	options.serial_device = argv[optind++];

	if (optind < argc) {
		int len = snprintf(options_err_msg, sizeof(options_err_msg), "Unknown command line argument%s",
				argc - optind > 1 ? "s" : "");
		while (optind < argc) {
			len += snprintf(options_err_msg + len, sizeof(options_err_msg) - len, " %s", argv[optind++]);
		}
		return 0;
	}

	return &options;
}

void options_print(void)
{
	printf("Options:\n"
			"help:     %s\n"
			"version:  %s\n"
			"verbose:  %s\n"
			"seconds:  %s\n"
			"time:     %s\n"
			"date:     %s\n"
			"mode:     %s\n"
			"unit:     %s\n"
			"cycle:    %f\n"
			"number:   %u\n"

			"device:   %s\n"
			"\n",
			options.help_flag ? "true" : "false",
			options.version_flag ? "true" : "false",
			options.verbose_flag ? "true" : "false",
			options.seconds_flag ? "true" : "false",
			options.time_flag ? "true" : "false",
			options.date_format ? options.date_format : "",
			options.mode_flag ? "true" : "false",
			options.unit_flag ? "true" : "false",
			options.cycle_s,
			options.cycles_number,

			options.serial_device
	);
}

void options_help(void)
{
	printf( "\n"
			"Usage: "APP_NAME" [options] device\n"
			"\n"
			"where device - serial communication device, used for communication with multimeter\n"
			"\n"
			"options:\n"
			"\n"
			"-h, --help         show this help screen and exit\n"
			"-V, --version      show version information and exit\n"
			"-v, --verbose      show verbose information\n"
			"                   (default: false)\n"
			"-s, --seconds      show how many seconds have passed since the start\n"
			"                   (default: false)\n"
			"-t, --time         show measure time\n"
			"                   (default: false)\n"
			"-d, --date=FORMAT  format specifier for date part of measure time,\n"
			"                   for example \"%%d.%%m.%%Y\" produce \"DD.MM.YYYY\" string\n"
			"                   (use \"man strftime\" for more information)\n"
			"                   (default: empty string - not show date)\n"
			"-m, --mode         show multimeter mode\n"
			"                   (default: false)\n"
			"-u, --unit         show measure units\n"
			"                   (default: false)\n"
			"-c, --cycle=VALUE  set measure cycle time in seconds,\n"
			"                   VALUE may be integer or real\n"
			"                   (default: 1 second)\n"
			"-n, --number=VALUE set limit to number of measure cycles\n"
			"                   (default: 0 - no limit)\n"
			"\n"
	);

	//TODO stub
}

void options_version(void)
{
	printf(APP_NAME" version "VERSION"\n");

	//TODO license amd copyright

}





