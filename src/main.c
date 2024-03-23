/*
 * Title:			Agon firmware upgrade utility
 * Author:			Jeroen Venema
 * Created:			17/12/2022
 * Last Updated:	02/11/2023
 * 
 * Modinfo:
 * 17/12/2022:		Initial version
 * 05/04/2022:		Changed timer to 5sec at reset.
 *                  Sends cls just before reset
 * 07/06/2023:		Included faster crc32, by Leigh Brown
 * 14/10/2023:		VDP update code, MOS update rewritten for simplicity
 * 02/11/2023:		Batched mode, rewrite of UI
 */

#include <ez80.h>
#include <stdio.h>
#include <stdlib.h>
#include <ERRNO.H>
#include <ctype.h>
#include "mos-interface.h"
#include "flash.h"
#include "agontimer.h"
#include "crc32.h"
#include "filesize.h"
#include "./stdint.h"
#include "switch-kernel.h"
#include <string.h>

#define UNLOCKMATCHLENGTH 9
#define EXIT_FILENOTFOUND	4
#define EXIT_INVALIDPARAMETER	19
#define DEFAULT_MOSFIRMWARE	"MOS.bin"
#define DEFAULT_VDPFIRMWARE	"firmware.bin"

#define CMDUNKNOWN	0
#define CMDALL		1
#define CMDMOS		2
#define CMDVDP		3
#define CMDFORCE	4
#define CMDBATCH	5
#define CMDSWITCHVDP	6
#define CMDSWITCHMOS	7

int errno; // needed by standard library
enum states{firmware,retry,systemreset};

bool		flashmos = false;
char		mosfilename[256];
uint32_t	moscrc;
bool		flashvdp = false;
char		vdpfilename[256];
uint32_t	vdpcrc;
bool		optbatch = false;
bool		optforce = false;		// No y/n user input required
bool		optswitch = false;

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

void beep(unsigned int number) {
	while(number--) {
		putch(7);
		delayms(250);
	}
}

uint8_t getCharAt(uint16_t x, uint16_t y) {
	sysvar_t *sysvars = getsysvars();
	delayms(20);
	putch(23);
	putch(0);
	putch(131);
	putch(x & 0xFF);
	putch((x >> 8) & 0xFF);
	putch(y & 0xFF);
	putch((y >> 8) & 0xFF);
	delayms(100);
	return sysvars->scrchar;
}

bool vdp_ota_present(void) {
	char test[UNLOCKMATCHLENGTH+1];
	uint16_t n;

	putch(23);
	putch(0);
	putch(0xA1);
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
	printf("Agon firmware update utility v1.6\n\r\n\r");
}

void usage(void) {
	print_version();
	printf("FLASH [commands] <-f>\n\r");
	printf("---------------------\n\r");
	printf("Commands: [all | [mos <filename>] [vdp <filename>] | batch |\n\r");
	printf("           vdp-switch | mos-switch]\n\r");
}

typedef enum {
	MOS,
	VDP
} flashtype;

bool getResponse(void) {
	uint8_t response = 0;

	printf("Flash firmware (y/n)?");
	while((response != 'y') && (response != 'n')) response = tolower(getch());
	if(response == 'n') printf("\r\nUser abort\n\r\n\r");
	else printf("\r\n\r\n");
	return response == 'y';
}

void askEscapeToContinue(void) {
	uint8_t response = 0;

	printf("Press ESC to continue");
	while(response != 0x1B) response = tolower(getch());
	printf("\r\n");
}

bool update_vdp(char *filename) {
	uint8_t file;
	uint24_t filesize;
	uint24_t size, n;

	putch(12); // cls
	print_version();	
	printf("Unlocking VDP updater...\r\n");
	
	if(!vdp_ota_present()) {
		printf(" failed - OTA not present in current VDP\r\n\r\n");
		printf("Program the VDP using Arduino / PlatformIO / esptool\r\n\r\n");
		return false;
	}

	file = mos_fopen(filename, fa_read);
	// Do actual work here
	printf("Updating VDP firmware\r\n");
	filesize = getFileSize(file);	
	startVDPupdate(file, filesize);
	mos_fclose(file);
	return true;
}

bool update_mos(char *filename) {
	uint32_t crcresult;
	uint24_t got;
	uint8_t file;
	char* ptr = (char*)BUFFER1;
	uint8_t value;
	uint24_t counter,pagemax, lastpagebytes;
	uint24_t addressto,addressfrom;
	uint24_t filesize;
	int attempt;
	bool success = false;

	putch(12); // cls
	print_version();	
	
	printf("Programming MOS firmware to ez80 flash...\r\n\r\n");
	printf("Reading MOS firmware");
	file = mos_fopen(filename, fa_read);
	filesize = getFileSize(file);
	// Read file to memory
	crc32_initialize();
	while((got = mos_fread(file, ptr, BLOCKSIZE)) > 0) {
		crc32(ptr, got);
		ptr += got;
		putch('.');
	}
	crcresult = crc32_finalize();
	printf("\r\n");
	// Final memory check to given crc32
	if(crcresult != moscrc) {
		printf("Error reading file to memory\r\n");
		return false;
	}
	printf("\r\n");	
	// Actual work here	
	di();								// prohibit any access to the old MOS firmware

	attempt = 0;
	while((!success) && (attempt < 3)) {
		// start address in flash
		addressto = FLASHSTART;
		addressfrom = BUFFER1;
		// Write attempt#
		if(attempt > 0) printf("Retry attempt #%d\r\n", attempt);
		// Unprotect and erase flash
		printf("Erasing flash... ");
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
		printf("\r\n");
				
		// determine number of pages to write
		pagemax = filesize/PAGESIZE;
		if(filesize%PAGESIZE) // last page has less than PAGESIZE bytes
		{
			pagemax += 1;
			lastpagebytes = filesize%PAGESIZE;			
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
		printf("\r\nChecking CRC... ");
		crc32_initialize();
		crc32(FLASHSTART, filesize);
		crcresult = crc32_finalize();
		if(crcresult == moscrc) {
			printf("OK\r\n");
			success = true;
		}
		else {
			printf("ERROR\r\n");
		}
		attempt++;
	}
	printf("\r\n");
	return success;
}

void echoVDP(uint8_t value) {
	putch(23);
	putch(0);
	putch(0x80);
	putch(value);
	delayms(100);
}

void switch_vdp () {
	// vdp_ota_present() only works when the below is exactly written to the screen in MOS
	putch(12); // cls
	print_version();	
	printf("Unlocking VDP updater...\r\n");
	// vdp_ota_present() only works when the above is exactly written to the screen in MOS
	if(!vdp_ota_present()) {
		printf(" failed - OTA not present in current VDP\r\n\r\n");
		printf("Program the VDP using Arduino / PlatformIO / esptool\r\n\r\n");
		return;
	}
	// Switch
	printf("Switching VDP...\r\n");
	// VDU 23 0 161 2
	putch (23);
	putch (0);
	putch (161);
	putch (2);
	delayms(1000);
	// activate vdp
	echoVDP(1);
}

int getCommand(const char *command) {
	if(memcmp(command, "all\0", 4) == 0) return CMDALL;
	if(memcmp(command, "mos\0", 4) == 0) return CMDMOS;
	if(memcmp(command, "vdp\0", 4) == 0) return CMDVDP;
	if(memcmp(command, "batch\0", 6) == 0) return CMDBATCH;
	if(memcmp(command, "-f\0", 3) == 0) return CMDFORCE;
	if(memcmp(command, "force\0", 6) == 0) return CMDFORCE;
	if(memcmp(command, "-force\0", 7) == 0) return CMDFORCE;
	if(memcmp(command, "vdp-switch\0", 11) == 0) return CMDSWITCHVDP;
	if(memcmp(command, "mos-switch\0", 11) == 0) return CMDSWITCHMOS;
	return CMDUNKNOWN;
}

bool parseCommands(int argc, char *argv[]) {
	int argcounter;
	int command;

	argcounter = 1;
	while(argcounter < argc) {
		command = getCommand(argv[argcounter]);
		switch(command) {
			case CMDUNKNOWN:
				return false;
				break;
			case CMDALL:
				if(flashmos || flashvdp) return false;
				strcpy(mosfilename, DEFAULT_MOSFIRMWARE);
				strcpy(vdpfilename, DEFAULT_VDPFIRMWARE);
				flashmos = true;
				flashvdp = true;
				break;
			case CMDMOS:
				if(flashmos) return false;
				if((argc > (argcounter+1)) && (getCommand(argv[argcounter + 1]) == CMDUNKNOWN)) {
					strcpy(mosfilename, argv[argcounter + 1]);
					argcounter++;
				}
				else {
					strcpy(mosfilename, DEFAULT_MOSFIRMWARE);
				}
				flashmos = true;
				break;
			case CMDVDP:
				if(flashvdp) return false;
				if((argc > (argcounter+1)) && (getCommand(argv[argcounter + 1]) == CMDUNKNOWN)) {
					strcpy(vdpfilename, argv[argcounter + 1]);
					argcounter++;
				}
				else {
					strcpy(vdpfilename, DEFAULT_VDPFIRMWARE);
				}
				flashvdp = true;
				break;
			case CMDBATCH:
				if(optbatch) return false;
				optbatch = true;
				optforce = true;
				strcpy(mosfilename, DEFAULT_MOSFIRMWARE);
				strcpy(vdpfilename, DEFAULT_VDPFIRMWARE);
				flashmos = true;
				flashvdp = true;
				break;
			case CMDFORCE:
				if(optforce && !optbatch) return false;
				optforce = true;
				break;
			case CMDSWITCHVDP:
				optswitch = true;
				flashvdp = true;
				break;
			case CMDSWITCHMOS:
				optswitch = true;
				flashmos = true;
				break;
		}
		argcounter++;
	}
	return (flashvdp || flashmos);
}

bool filesExist(void) {
	uint8_t file;
	bool filesexist = true;

	if(flashmos) {
		file = mos_fopen(mosfilename, fa_read);
		if(!file) {
			printf("Error opening MOS firmware \"%s\"\n\r",mosfilename);
			filesexist = false;
		}
		mos_fclose(file);
	}

	if(flashvdp) {
		file = mos_fopen(vdpfilename, fa_read);
		if(!file) {
			printf("Error opening VDP firmware \"%s\"\n\r",vdpfilename);
			filesexist = false;
		}
		mos_fclose(file);
	}

	return filesexist;
}

bool validFirmwareFiles(void) {
	uint8_t file;
	uint24_t filesize;
	uint8_t buffer[ESP32_MAGICLENGTH + ESP32_MAGICSTART];
	bool validfirmware = true;

	if(flashmos) {
		file = mos_fopen(mosfilename, fa_read);
		mos_fread(file, (char *)BUFFER1, MOS_MAGICLENGTH);
		if(!containsMosHeader((uint8_t *)BUFFER1)) {
			printf("\"%s\" does not contain valid MOS ez80 startup code\r\n", mosfilename);
			validfirmware = false;
		}
		filesize = getFileSize(file);
		if(filesize > FLASHSIZE) {
			printf("\"%s\" too large for 128KB embedded flash\r\n", mosfilename);
			validfirmware = false;
		}
		mos_fclose(file);
	}
	if(flashvdp) {
		file = mos_fopen(vdpfilename, fa_read);
		mos_fread(file, (char *)buffer, ESP32_MAGICLENGTH + ESP32_MAGICSTART);
		if(!containsESP32Header(buffer)) {
			printf("\"%s\" does not contain valid ESP32 code\r\n", vdpfilename);
			validfirmware = false;
		}
		mos_fclose(file);
	}
	return validfirmware;
}

void showCRC32(void) {
	if(flashmos) printf("MOS CRC 0x%04lX\r\n", moscrc);
	if(flashvdp) printf("VDP CRC 0x%04lX\r\n", vdpcrc);
	printf("\r\n");
}

void calculateCRC32(void) {
	uint8_t file;
	uint24_t got,size;
	char* ptr;

	moscrc = 0;
	vdpcrc = 0;

	printf("Calculating CRC");

	if(flashmos) {
		ptr = (char*)BUFFER1;
		file = mos_fopen(mosfilename, fa_read);
		crc32_initialize();
		
		// Read file to memory
		while((got = mos_fread(file, ptr, BLOCKSIZE)) > 0) {
			crc32(ptr, got);
			ptr += got;
			putch('.');
		}		
		moscrc = crc32_finalize();
		mos_fclose(file);
	}
	if(flashvdp) {
		file = mos_fopen(vdpfilename, fa_read);
		crc32_initialize();
		while(1) {
			size = mos_fread(file, (char *)BUFFER1, BLOCKSIZE);
			if(size == 0) break;
			putch('.');
			crc32((char *)BUFFER1, size);
		}
		vdpcrc = crc32_finalize();
		mos_fclose(file);
	}
	printf("\r\n\r\n");
}

int main(int argc, char * argv[]) {	
	sysvar_t *sysvars;
	int n;
	uint16_t tmp;
	sysvars = getsysvars();

	// All checks
	if(argc == 1) {
		usage();
		return 0;
	}
	if(!parseCommands(argc, argv)) {
		usage();
		return EXIT_INVALIDPARAMETER;
	}
	if (optswitch)
	{
		if (flashvdp)
			switch_vdp ();
		if (flashmos)
			switch_kernel ();
		return 0;
	}
	if(!filesExist()) return EXIT_FILENOTFOUND;
	if(!validFirmwareFiles()) {
		return EXIT_INVALIDPARAMETER;
	}

	putch(12);
	print_version();
	calculateCRC32();
	// Skip showing CRC32 and user input when 'silent' is requested
	if(!optforce) {
		putch(12);
		print_version();
		showCRC32();
		if(!getResponse()) return 0;
	}
	if(optbatch) beep(1);

	if(flashvdp) {
		while(sysvars->scrheight == 0); // wait for 1st feedback from VDP
		tmp = sysvars->scrheight;
		sysvars->scrheight = 0;
		if(update_vdp(vdpfilename)) {
			echoVDP(1);
			while(sysvars->scrheight == 0);
			if(optbatch) beep(2);
		}
		else {
			if(!optforce && flashmos) {
				askEscapeToContinue();
				sysvars->scrheight = tmp;
			}
		}
	}

	if(flashmos) {
		if(update_mos(mosfilename)) {
			printf("\r\nDone\r\n\r\n");
			if(optbatch) {
				printf("Press reset button");
				beep(3);
				while(1); // don't repeatedly run this command batched (autoexec.txt)
			}
			else {
				printf("System reset in ");
				for(n = 3; n > 0; n--) {
					printf("%d...", n);
					delayms(1000);
				}
				reset();
			}
		}
		else {
			printf("\r\nMultiple errors occured during flash write.\r\n");
			printf("Bare-metal recovery required.\r\n");
			while(1); // No live MOS to return to
		}
	}
	return 0;
}

