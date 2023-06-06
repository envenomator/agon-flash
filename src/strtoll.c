/*
 * Title:			CRC32 functions
 * Author:			Jeroen Venema
 * Created:			17/12/2022
 * Last Updated:	17/12/2022
 * 
 * Modinfo:
 * 17/12/2022:		Initial version
 */

#include <defines.h>
#include <ctype.h>
#include <stdlib.h>
#include <ez80.h>
#include "crc32.h"

extern int errno;

/*
 * Convert a string to a UINT32 integer.
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */
 
UINT32
strtoll(const char *nptr)
{
	const char *s = nptr;
	UINT32 acc;
	int c;
	int digits = 0;

	do {
		c = *s++;
	} while (isspace(c));
	if (
	    c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
	}

	for (acc = 0;; c = *s++) {
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= 16)
			break;
		else {
			acc *= 16;
			acc += c;
			digits++;
		}
	}
	if(digits > 8) errno = ERANGE;
	return (acc);
}
