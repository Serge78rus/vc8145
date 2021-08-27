/*
 * options.h
 *
 *  Created on: 16 авг. 2021 г.
 *      Author: serge78rus
 */

#ifndef OPTIONS_H_
#define OPTIONS_H_

#include <stdbool.h>

//type definitions
struct Options {
	bool help_flag;
	bool version_flag;
	bool verbose_flag;
	bool seconds_flag;
	bool time_flag;
	char *date_format;
	bool mode_flag;
	bool unit_flag;
	double cycle_s;
	unsigned int cycles_number;

	char *serial_device; //TODO real size
};

//public functions
struct Options* options_parse(int argc, char **argv);
void options_print(void);
void options_help(void);
void options_version(void);

//global variables
extern char options_err_msg[];

#endif /* OPTIONS_H_ */
