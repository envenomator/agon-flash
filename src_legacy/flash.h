#ifndef FLASH_H
#define FLASH_H

#define BUFFER1		0x50000
#define BUFFER2		0x70000
#define FLASHSIZE	0x20000		// 128KB

#define PAGESIZE	1024
#define FLASHPAGES	128
#define FLASHSTART	0x0

extern void enableFlashKeyRegister(void);
extern void lockFlashKeyRegister(void);
extern void fastmemcpy(UINT24 destination, UINT24 source, UINT24 size);
extern void reset(void);

#endif FLASH_H