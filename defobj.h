/*            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 *                    Version 2, December 2004
 *
 * Copyright (C) 2012 Paul Grandperrin <paul.grandperrin@gmail.com>
 *
 * Everyone is permitted to copy and distribute verbatim or modified
 * copies of this license document, and changing it is allowed as long
 * as the name is changed.
 *
 *            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 *   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
 *
 *  0. You just DO WHAT THE FUCK YOU WANT TO.
 */



#ifndef _DEFOBJ_H
#define _DEFOBJ_H

//#define _POSIX_SOURCE
//#define _XOPEN_SOURCE 500
//#define _BSD_SOURCE
#define _GNU_SOURCE /* includes most non-conflicting standards */

#include <stdint.h>
#include <arpa/inet.h> /* htonl & htons */

#define TABLEN 3

/**
 * Our object structure
 */
typedef struct {
	char a[12];
	char b[24];
	uint32_t ii;
	uint32_t jj;
	uint64_t dd;
	char iqt;
} obj;

union double2uint64 {
	uint64_t u;
	double d;
};

/**
 * As there isn't any standard fonction to convert 64 bits numbers
 * to and from the network, we created one.
 */
uint64_t htonll(uint64_t value) {

	/* The answer is 42 */
	static const int num = 42;

	/* Check the endianness */
	if (*(char*)&num == num) {
		const uint32_t high_part = htonl((uint32_t)(value >> 32));
		const uint32_t low_part = htonl((uint32_t)(value & 0xFFFFFFFFLL));

		return (((uint64_t)low_part) << 32) | high_part;
	} else
		return value;
}

uint64_t ntohll(uint64_t value) {
	return htonll(value);
}

#endif /* !_DEFOBJ_H */
