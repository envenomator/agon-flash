#ifndef CRC32_H
#define CRC32_H

UINT32 strtoll(const char *nptr);
void crc32(const char *s, UINT24 length);
void crc32_initialize(void);
UINT32 crc32_finalize(void);
#endif CRC32_H