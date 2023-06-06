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


// Calculate a CRC32 over a block of memory
// Parameters:
// - s: Pointer to a memory location
// - length: Size of memoryblock in bytes
UINT32 crc32(const char *s, UINT24 length)
{
  static UINT32 crc;
  static UINT32 crc32_table[256];
  UINT32 i,ch,j,b,t; 

  // init a crc32 lookup table, fastest way to process the entire block
  for(i = 0; i < 256; i++)
  {
    ch = i;
    crc = 0;
    for(j = 0; j < 8; j++)
    {
	  b=(ch^crc)&1;
	  crc>>=1;
	  if(b) crc=crc^0xEDB88320;
	  ch>>=1;
    }
    crc32_table[i] = crc;
  }        

  // calculate the crc using the table
  crc = 0xFFFFFFFF;
  for(i=0;i<length;i++)
  {
    char ch=s[i];
    t=(ch^crc)&0xFF;
    crc=(crc>>8)^crc32_table[t];
  }

  return ~crc;
}

