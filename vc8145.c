/*
 * vc8145.c
 *
 *  Created on: 14 авг. 2021 г.
 *      Author: serge78rus
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include <errno.h>

#include "vc8145.h"

//local definitions
#define CMD 0x89
#define ANSW_LEN 12
#define ANSW_TIMEOUT_US 100000 /*real value ~11ms*/

//global variables
char vc8145_err_msg[80] = {0}; //usage size ~73 bytes

//local variables
static int serial_fd = -1;
static struct termios old_serial_termios;
static struct termios new_serial_termios;

//private functions
static inline char digit(uint8_t byte);

/*
 * public functions
 */

bool vc8145_open(const char *serial_device)
{
	vc8145_err_msg[0] = 0;

	serial_fd = open(serial_device, O_RDWR | O_NOCTTY | O_NDELAY );
	if (serial_fd < 0) {
		snprintf(vc8145_err_msg, sizeof(vc8145_err_msg), "Open communication device \"%s\" error", serial_device);
		return false;
	}

	fcntl(serial_fd, F_SETFL, 0);
	tcgetattr(serial_fd, &old_serial_termios);
	memcpy(&new_serial_termios, &old_serial_termios, sizeof(struct termios));

	cfmakeraw(&new_serial_termios);
	new_serial_termios.c_cflag = B9600 | CS8 | CLOCAL | CREAD; // Adjust the settings to suit our meter
	new_serial_termios.c_cflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl
	new_serial_termios.c_cflag &= ~(PARENB | PARODD); // shut off parity
	new_serial_termios.c_cflag &= ~CSTOPB;
	new_serial_termios.c_cflag &= ~CRTSCTS;
	new_serial_termios.c_cc[VMIN] = ANSW_LEN;
	new_serial_termios.c_cc[VTIME] = 1; // 1 * .1s

	if (tcsetattr(serial_fd, TCSANOW, &new_serial_termios)) {
		snprintf(vc8145_err_msg, sizeof(vc8145_err_msg), "Set attributes to communication device \"%s\" error", serial_device);
		return false;
	}

	return true;
}

void vc8145_close(void)
{
	if (serial_fd >= 0) {
		tcsetattr(serial_fd, TCSANOW, &old_serial_termios); //TODO
		close(serial_fd);
	}
}

char* vc8145_read(bool show_mode, bool show_unit)
{
	vc8145_err_msg[0] = 0;

	//clear input buffer and send command

	tcflush(serial_fd, TCIFLUSH);
	static const uint8_t cmd = CMD;
	if (write(serial_fd, &cmd, sizeof(cmd)) != sizeof(cmd)) {
		snprintf(vc8145_err_msg, sizeof(vc8145_err_msg), "Communication device write error");
		return 0;
	}

	//wait and receive answer

	static uint8_t rcv_buff[ANSW_LEN];
	struct timeval timeout = {
			.tv_sec = 0,
			.tv_usec = ANSW_TIMEOUT_US
	};
	fd_set read_set;
	fd_set err_set;
    FD_ZERO(&read_set);
    FD_ZERO(&err_set);
    FD_SET(serial_fd, &read_set);
    FD_SET(serial_fd, &err_set);
    int sel_result = select(serial_fd + 1, &read_set, NULL, &err_set, &timeout);
    if (sel_result < 0) {
		snprintf(vc8145_err_msg, sizeof(vc8145_err_msg), "Communication device select() error, result=%i", sel_result);
		return 0;
    } else if (sel_result == 0) {
		snprintf(vc8145_err_msg, sizeof(vc8145_err_msg), "Communication device timeout");
		return 0;
    }
    if (FD_ISSET(serial_fd, &err_set)) {
		snprintf(vc8145_err_msg, sizeof(vc8145_err_msg), "Communication device error");
		return 0;
    }
    if (!FD_ISSET(serial_fd, &read_set)) {
		snprintf(vc8145_err_msg, sizeof(vc8145_err_msg), "Unknown communication error");
		return 0;
    }
	int rx_len = read(serial_fd, (void*)rcv_buff, ANSW_LEN);
	if (rx_len != ANSW_LEN) {
		snprintf(vc8145_err_msg, sizeof(vc8145_err_msg), "Communication device read error, rx_len=%i", rx_len);
		return 0;
	}

	if (rcv_buff[0] != cmd || rcv_buff[ANSW_LEN - 1] != 0x0a) {
		int len = snprintf(vc8145_err_msg, sizeof(vc8145_err_msg), "Communication device invalid answer:");
		for (int i = 0; i < ANSW_LEN; ++i) {
			len += snprintf(vc8145_err_msg + len, sizeof(vc8145_err_msg) - len, " %02X", (unsigned int)rcv_buff[i]);
		}
		return 0;
	}

	//decoding value

	int dec_pos = (((int)rcv_buff[2] >> 3) & 0x7) + 1;

	static const char MODE_GEN[] 			= "Gen";
	static const char MODE_A[] 				= "A";
	static const char MODE_MA[] 			= "mA";
	static const char MODE_TEMP[] 			= "Temp";
	static const char MODE_CAP[] 			= "Cap";
	static const char MODE_FREQ[] 			= "Freq";
	static const char MODE_DIODE[] 			= "Diode";
	static const char MODE_OHM[] 			= "Ohm";
	static const char MODE_MV[] 			= "mV";
	static const char MODE_DCV[] 			= "DCV";
	static const char MODE_ACV[] 			= "ACV";
	static const char MODE_UNKNOWN[] 		= "?";
	const char *mode = 0;

	static const char UNIT_A[]				= "A";
	static const char UNIT_GRAD[]			= "\u00B0""C";
	static const char UNIT_FARADE[]			= "F";
	static const char UNIT_HZ[]				= "Hz";
	static const char UNIT_V[]				= "V";
	static const char UNIT_OHM[]			= "\u03A9";
	static const char UNIT_UNKNOWN[] 		= "?";
	const char *unit = 0;

	static const char UNIT_PREFIX_MILLI[]	= "m";
	static const char UNIT_PREFIX_NANO[]	= "n";
	static const char UNIT_PREFIX_MICRO[]	= "\u00B5";
	static const char UNIT_PREFIX_MEGA[]	= "M";
	static const char UNIT_PREFIX_KILO[]	= "k";
	const char *unit_prefix = 0;

	switch (((int)rcv_buff[1] >> 3) & 0xf) {
	case 0x4: //Generator
		mode = MODE_GEN;
		break;
	case 0x5: //A 0x7?
		mode = MODE_A;
		unit = UNIT_A;
		dec_pos -= 1;
		break;
	case 0x6: //mA
		mode = MODE_MA;
		unit = UNIT_A;
		unit_prefix = UNIT_PREFIX_MILLI;
		break;
	case 0x8: //Temp
		mode = MODE_TEMP;
		unit = UNIT_GRAD;
		break;
	case 0x9: //Cap
		mode = MODE_CAP;
		unit = UNIT_FARADE;
		if (dec_pos < 4) {
			unit_prefix = UNIT_PREFIX_NANO;
			dec_pos -= 1;
		} else {
			unit_prefix = UNIT_PREFIX_MICRO;
			dec_pos -= 4;
		}
		break;
	case 0xa: //Freq
		mode = MODE_FREQ;
		unit = UNIT_HZ;
		break;
	case 0xb: //Diode
		mode = MODE_DIODE;
		unit = UNIT_V;
		dec_pos -= 1;
		break;
	case 0xc: //Ohm
		mode = MODE_OHM;
		unit = UNIT_OHM;
		switch (dec_pos) {
		case 1:
			dec_pos += 1;
			break;
		case 5:
		case 6:
			unit_prefix = UNIT_PREFIX_MEGA;
			dec_pos -= 5;
			break;
		default:
			unit_prefix = UNIT_PREFIX_KILO;
			dec_pos -= 2;
		}
		break;
	case 0xd: //mV
		mode = MODE_MV;
		unit = UNIT_V;
		unit_prefix = UNIT_PREFIX_MILLI;
		break;
	case 0xe: //DCV
		mode = MODE_DCV;
		unit = UNIT_V;
		dec_pos -= 1;
		break;
	case 0xf: //ACV
		mode = MODE_ACV;
		unit = UNIT_V;
		dec_pos -= 1;
		break;
	default:
		mode = MODE_UNKNOWN;
	}

	//formating output

	static char out_buff[100]; //TODO real size
	char *out_ptr = out_buff;

	if (show_mode) {
		if (!mode) {
			snprintf(vc8145_err_msg, sizeof(vc8145_err_msg), "Mode not set");
			mode = MODE_UNKNOWN;
		}
		while (*mode) {
			*out_ptr++ = *mode++;
		}
		*out_ptr++ = ' ';
	}

	if (rcv_buff[4] & 0b00010000) {
		*out_ptr++ = '-';
	}

	*out_ptr++ = digit(rcv_buff[5]);
	if (dec_pos == 0) {
		*out_ptr++ = '.';
	}
	*out_ptr++ = digit(rcv_buff[6]);
	if (dec_pos == 1) {
		*out_ptr++ = '.';
	}
	*out_ptr++ = digit(rcv_buff[7]);
	if (dec_pos == 2) {
		*out_ptr++ = '.';
	}
	*out_ptr++ = digit(rcv_buff[8]);
	if (dec_pos == 3) {
		*out_ptr++ = '.';
	}
	*out_ptr++ = digit(rcv_buff[9]);

	if (show_unit) {
		if (!unit) {
			snprintf(vc8145_err_msg, sizeof(vc8145_err_msg), "Unit not set");
			unit = UNIT_UNKNOWN;
		}
		*out_ptr++ = ' ';
		if (unit_prefix) {
			while (*unit_prefix) {
				*out_ptr++ = *unit_prefix++;
			}
		}
		while (*unit) {
			*out_ptr++ = *unit++;
		}
	}

	*out_ptr = 0;
	return out_buff;
}

/*
 * private functions
 */

static inline char digit(uint8_t byte)
{
	if (isdigit(byte)) {
		return (char)byte;
	} else if (byte == 0x3e) { // '>'
		return 'L';
	} else {
		return ' ';
	}
}


