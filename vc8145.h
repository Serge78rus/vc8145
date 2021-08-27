/*
 * vc8145.h
 *
 *  Created on: 14 авг. 2021 г.
 *      Author: serge78rus
 */

#ifndef VC8145_H_
#define VC8145_H_

#include <stdbool.h>

//public functions
bool vc8145_open(const char *serial_device);
void vc8145_close(void);
char* vc8145_read(bool show_mode, bool show_unit);

//global variables
extern char vc8145_err_msg[];

#endif /* VC8145_H_ */
