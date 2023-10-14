/*
 * Title:			Agon MOS firmware upgrade utility
 * Author:			Jeroen Venema
 * Created:			17/12/2022
 * Last Updated:	17/12/2022
 * 
 * Modinfo:
 * 17/12/2022:		Initial version
 * 05/04/2022:		Changed timer to 5sec at reset.
 *                  Sends cls just before reset
 * 07/06/2023:		Included faster crc32, by Leigh Brown
 */

#include <ez80.h>
#include <stdio.h>
#include <stdlib.h>
#include <ERRNO.H>
#include "mos-interface.h"
#include "flash.h"
#include "agontimer.h"
#include "crc32.h"
#include "filesize.h"
#include "./stdint.h"
#include <string.h>

#define UNLOCKMATCHLENGTH 9

#define EXIT_FILENOTFOUND	4
#define EXIT_INVALIDPARAMETER	19

int errno; // needed by standard library
enum states{firmware,recover,systemreset};

// separate putch function that doesn't rely on a running MOS firmware
// UART0 initialization done by MOS firmware previously
// This utility doesn't run without MOS to load it anyway
int putch(int c)
{
	UINT8 lsr,temt;
	
	while((UART0_LSR & 0x40) == 0);
	UART0_THR = c;
	return c;
}

uint8_t getCharAt(uint16_t x, uint16_t y) {
	delayms(20);
	putch(23);
	putch(0);
	putch(131);
	putch(x & 0xFF);
	putch((x >> 8) & 0xFF);
	putch(y & 0xFF);
	putch((y >> 8) & 0xFF);
	delayms(20);
	return getsysvar_scrchar();
}

bool vdp_ota_present(void) {
	char test[UNLOCKMATCHLENGTH];
	uint16_t n;

	putch(23);
	putch(29);
	putch(0);
	printf("unlock");

	for(n = 0; n < UNLOCKMATCHLENGTH+1; n++) test[n] = getCharAt(n+8, 3);
	// 3 - line on-screen
	if(memcmp(test, "unlocked!",UNLOCKMATCHLENGTH) == 0) return true;
	else return false;
}

uint8_t mos_magicnumbers[] = {0xF3, 0xED, 0x7D, 0x5B, 0xC3};
#define MOS_MAGICLENGTH 5
bool containsMosHeader(uint8_t *filestart) {
	uint8_t n;
	bool match = true;

	for(n = 0; n < MOS_MAGICLENGTH; n++) if(mos_magicnumbers[n] != filestart[n]) match = false;
	return match;
}

uint8_t esp32_magicnumbers[] = {0x32, 0x54, 0xCD, 0xAB};
#define ESP32_MAGICLENGTH 4
#define ESP32_MAGICSTART 0x20
bool containsESP32Header(uint8_t *filestart) {
	uint8_t n;
	bool match = true;

	filestart += ESP32_MAGICSTART; // start of ESP32 magic header
	for(n = 0; n < ESP32_MAGICLENGTH; n++) {
		if(esp32_magicnumbers[n] != filestart[n]) match = false;
	}
	return match;
}

void print_version(void) {
	printf("Agon firmware upgrade utility v1.4\n\r\n\r");
}

void usage(void) {
	print_version();
	printf("Usage: FLASH <mos|vdp> <filename>\n\r");
}

uint8_t update_vdp(char *filename) {
	uint8_t file;
	uint8_t buffer[ESP32_MAGICLENGTH + ESP32_MAGICSTART];
	uint24_t filesize;

	putch(12); // cls
	print_version();	
	printf("Unlocking VDP updater...\r\n");
	
	if(!vdp_ota_present()) {
		printf(" failed - incompatible VDP\r\n");
		return 0;
	}

	file = mos_fopen(filename, fa_read);
	if(!file) {
		printf("Error opening \"%s\"\n\r",filename);
		return EXIT_FILENOTFOUND;
	}

	mos_fread(file, (char *)buffer, ESP32_MAGICLENGTH + ESP32_MAGICSTART);
	if(!containsESP32Header(buffer)) {
		printf("File does not contain valid ESP32 code\r\n");
		return EXIT_INVALIDPARAMETER;
	}

	mos_flseek(file, 0); // reset to zero, because we read part of the header already

	// Do actual work here
	printf("Updating VDP firmware\r\n");
	filesize = getFileSize(file);	
	startVDPupdate(file, filesize);
	mos_fclose(file);
	reset();
	return 0; // will never return, but let's give the compiler a break
}

uint8_t update_mos(char *filename) {
	UINT32 crcexpected,crcresult,crcbackup;
	UINT24 size = 0;
	UINT24 got;
	UINT8 file;
	char* ptr = (char*)BUFFER1;
	UINT8 response;
	UINT8 value;
	UINT24 counter,pagemax, lastpagebytes;
	UINT24 addressto,addressfrom;
	enum states state;
	
	printf("Agon MOS firmware upgrade utility v1.4\n\r\n\r");
	
	return 0; // DISABLE FOR NOW

	file = mos_fopen(filename, fa_read);
	if(!file)
	{
		printf("Error opening \"%s\"\n\r",filename);
		return EXIT_FILENOTFOUND;
	}
	
	printf("Loading file : %s\n\r",filename);
	printf("File size    : %d byte(s)", size);

	// Read file to memory
	while((got = mos_fread(file, ptr, BLOCKSIZE)) > 0)
	{
		ptr += got;
		size += got;
		printf("\rFile size    : %d byte(s)", size);
	}		
	mos_fclose(file);	
	printf("\rFile size    : %d byte(s)\n\r", size);
	if(size > FLASHSIZE)
	{
		printf("Too large for 128KB flash - aborting\r\n");
		return 0;
	}

	if(!containsMosHeader((uint8_t *)ptr)) {
		printf("File does not contain valid MOS ez80 startup code\r\n");
		return EXIT_INVALIDPARAMETER;
	}

	crcexpected = 0;
	printf("Testing CRC32: 0x%08lx\n\r",crcexpected);
	crcresult = crc32((char*)BUFFER1, size);
	printf("CRC32 result : 0x%08lx\n\r",crcresult);

	if(crcexpected != crcresult)
	{
		printf("\n\rMismatch - aborting\n\r");
		return 0;
	}
	printf("\n\rOK\n\r\n\r");

	// Ask user to continue
	printf("Erase and program flash (y/n)? ");
	response = 0;
	while((response != 'y') && (response != 'n')) response = getch();
	if(response == 'y')
	{
		printf("\r\nBacking up existing firmware... ");
		fastmemcpy(BUFFER2, 0x0, FLASHSIZE);	
		crcbackup = crc32((char*)0x0, FLASHSIZE);
		
		di();								// prohibit any access to the old MOS firmware

		// start address in flash
		addressto = FLASHSTART;
		addressfrom = BUFFER1;
		
		state = firmware;		
		while(1)
		{
			switch(state)
			{
				case firmware:
					// start address in flash
					addressfrom = BUFFER1;					
					break;
				case recover:
					// start address in flash
					addressfrom = BUFFER2;
					size = FLASHSIZE;			// entire backup buffer
					break;
				default:
					// RESET SYSTEM
					printf("\r\n");
					for(counter = 5; counter >0; counter--)
					{
						printf("\rReset in %ds",counter);
						delayms(1000);
					}
					putch(12);
					delayms(500);
					reset();
			}
	
			// Unprotect and erase flash
			printf("\r\nErasing flash... ");
			enableFlashKeyRegister();	// unlock Flash Key Register, so we can write to the Flash Write/Erase protection registers
			FLASH_PROT = 0;				// disable protection on all 8x16KB blocks in the flash
			enableFlashKeyRegister();	// will need to unlock again after previous write to the flash protection register
			FLASH_FDIV = 0x5F;			// Ceiling(18Mhz * 5,1us) = 95, or 0x5F
			
			for(counter = 0; counter < FLASHPAGES; counter++)
			{
				FLASH_PAGE = counter;
				FLASH_PGCTL = 0x02;			// Page erase bit enable, start erase

				do
				{
					value = FLASH_PGCTL;
				}
				while(value & 0x02);// wait for completion of erase			
			}
			
			printf("\r\nWriting new firmware...\r\n");
			
			// determine number of pages to write
			pagemax = size/PAGESIZE;
			if(size%PAGESIZE) // last page has less than PAGESIZE bytes
			{
				pagemax += 1;
				lastpagebytes = size%PAGESIZE;			
			}
			else lastpagebytes = PAGESIZE; // normal last page
			
			// write out each page to flash
			for(counter = 0; counter < pagemax; counter++)
			{
				printf("\rWriting flash page %03d/%03d", counter+1, pagemax);
				
				if(counter == (pagemax - 1)) // last page to write - might need to write less than PAGESIZE
					fastmemcpy(addressto,addressfrom,lastpagebytes);				
					//printf("Fake copy to %lx, from %lx, %lx bytes\r\n",addressto, addressfrom, lastpagebytes);
				else 
					fastmemcpy(addressto,addressfrom,PAGESIZE);
					//printf("Fake copy to %lx, from %lx, %lx bytes\r\n",addressto, addressfrom, PAGESIZE);
			
				addressto += PAGESIZE;
				addressfrom += PAGESIZE;
			}
			lockFlashKeyRegister();	// lock the flash before WARM reset
			printf("\r\n");
			
			//Verify correct CRC in flash
			printf("Verifying flash checksum... ");
			crcresult = crc32((char*)FLASHSTART, size);

			if(state == recover) crcexpected = crcbackup;
			
			if(crcresult == crcexpected)
			{
				printf("- OK\r\n");
				state = systemreset;
			}
			else // CRC Failure - next action depends on current state
			{	 // User interaction not possible without MOS handling interrupts
				switch(state)
				{
					case firmware:
						printf("\r\nError occured during flash write\r\nAttempting to flash backup firmware...\r\n");
						state = recover;
						break;
					case recover:
						printf("\r\nError occured during flash write\r\nBackup recovery failed\r\n");
						while(1); // no more options unfortunately, system needs a firmware programmer
					default:
						state = recover;
				}
			}
		}		
	}
	else printf("\n\rUser abort\n\r");
	
	return 0;
}

int main(int argc, char * argv[]) {

	if(argc != 3) {
		usage();
		return 0;
	}

	if(memcmp(argv[1], "mos", 3) == 0) {
		return update_mos(argv[2]);
	}
	else {
		if(memcmp(argv[1], "vdp", 3) == 0) {
			return update_vdp(argv[2]);
		}
		else {
			usage();
			return 0;
		}
	}
}

