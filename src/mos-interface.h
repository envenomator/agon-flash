/*
 * Title:			AGON MOS - MOS c header interface
 * Author:			Jeroen Venema
 * Created:			15/10/2022
 * Last Updated:	15/10/2022
 * 
 * Modinfo:
 * 15/10/2022:		Added putch, getch
 * 22/10/2022:		Added waitvblank, mos_f* functions
 * 10/01/2023:      Added getsysvar_cursorX/Y functions, removed generic getsysvar8/16/24bit functions
 * 25/03/2023:      Added MOS 1.03 functions / sysvars
 * 16/04/2023:      Added MOS 1.03RC4 mos_fread / mos_fwrite / mos_flseek functions
 * 19/04/2023:		Added mos_getfil
 * 30/10/2023:		removed all sysvar functions -> struct sysvar_t created
 */

#ifndef MOS_H
#define MOS_H
#include "./stdint.h"
#include <defines.h>

// File access modes - from mos_api.inc
#define fa_read				    0x01
#define fa_write			    0x02
#define fa_open_existing	    0x00
#define fa_create_new		    0x04
#define fa_create_always	    0x08
#define fa_open_always		    0x10
#define fa_open_append		    0x30

typedef struct {
	uint32_t clock;				// 4	Clock timer in centiseconds (incremented by 2 every VBLANK)
	uint8_t vpd_protocol_flags;	// 1	Flags to indicate completion of VDP commands
	uint8_t keyascii;			// 1	ASCII keycode, or 0 if no key is pressed
	uint8_t keymods;			// 1	Keycode modifiers
	uint8_t cursorX;			// 1	Cursor X position
	uint8_t cursorY;			// 1	Cursor Y position
	uint8_t scrchar;			// 1	Character read from screen
	uint24_t scrpixel;			// 3	Pixel data read from screen (R,B,G)
	uint8_t audioChannel;		// 1	Audio channel 
	uint8_t audioSuccess;		// 1	Audio channel note queued (0 = no, 1 = yes)
	uint16_t scrwidth;			// 2	Screen width in pixels
	uint16_t scrheight;			// 2	Screen height in pixels
	uint8_t scrcols;			// 1	Screen columns in characters
	uint8_t scrrows;			// 1	Screen rows in characters
	uint8_t scrcolours;			// 1	Number of colours displayed
	uint8_t scrpixelIndex;		// 1	Index of pixel data read from screen
	uint8_t keycode;			// 1	Virtual key code from FabGL
	uint8_t keydown;			// 1	Virtual key state from FabGL (0=up, 1=down)
	uint8_t keycount;			// 1	Incremented every time a key packet is received
	uint8_t rtc[6];				// 6	Real time clock data
	uint8_t rtc_spare[2];		// 2	Spare, previously used by rtc
	uint16_t keydelay;			// 2	Keyboard repeat delay
	uint16_t keyrate;			// 2	Keyboard repeat rate
	uint8_t keyled;				// 1	Keyboard LED status
	uint8_t scrmode;			// 1	Screen mode
	uint8_t rtc_enable;			// 1	RTC enable status
	uint16_t mouseX;			// 2	Mouse X position
	uint16_t mouseY;			// 2	Mouse Y position
	uint8_t mouseButtons;		// 1	Mouse left+right+middle buttons (bits 0-2, 0=up, 1=down)
	uint8_t mouseWheel;			// 1	Mouse wheel delta
	uint16_t mouseXDelta;		// 2	Mouse X delta
	uint16_t mouseYDelta;		// 2	Mouse Y delta
} sysvar_t;

// UART settings for open_UART1
typedef struct {
   INT24 baudRate ;				//!< The baudrate to be used.
   BYTE dataBits ;				//!< The number of databits per character to be used.
   BYTE stopBits ;				//!< The number of stopbits to be used.
   BYTE parity ;				   //!< The parity bit option to be used.
   BYTE flowcontrol ;
   BYTE eir ;
} UART ;

// File Object ID and allocation information (FFOBJID)
typedef struct {
	UINT24*  fs;		/* Pointer to the hosting volume of this object */
	UINT16	id;		/* Hosting volume mount ID */
	BYTE	   attr;		/* Object attribute */
	BYTE	   stat;		/* Object chain status (b1-0: =0:not contiguous, =2:contiguous, =3:fragmented in this session, b2:sub-directory stretched) */
	UINT32	sclust;	/* Object data start cluster (0:no cluster or root directory) */
	UINT32	objsize;	/* Object size (valid when sclust != 0) */
} FFOBJID;

// File object structure (FIL)
typedef struct {
	FFOBJID	obj;     /* Object identifier (must be the 1st member to detect invalid object pointer) */
	BYTE	   flag;    /* File status flags */
	BYTE	   err;     /* Abort flag (error code) */
	UINT32	fptr;    /* File read/write pointer (Zeroed on file open) */
	UINT32	clust;   /* Current cluster of fpter (invalid when fptr is 0) */
	UINT32	sect;    /* Sector number appearing in buf[] (0:invalid) */
	UINT32	dir_sect;/* Sector number containing the directory entry (not used at exFAT) */ 
	UINT24*	dir_ptr; /* Pointer to the directory entry in the win[] (not used at exFAT) */
} FIL;

// Generic IO
extern int   putch(int a);
extern char  getch(void);
extern void  waitvblank(void);
extern void  mos_puts(char * buffer, UINT24 size, char delimiter);

// Get system variables
extern sysvar_t* getsysvars(void);

// MOS API calls
extern UINT8  mos_load(char *filename, UINT24 address, UINT24 maxsize);
extern UINT8  mos_save(char *filename, UINT24 address, UINT24 nbytes);
extern UINT8  mos_cd(char *path);
extern UINT8  mos_dir(char *path);
extern UINT8  mos_del(char *filename);
extern UINT8  mos_ren(char *filename, char *newname);
extern UINT8  mos_copy(char *source, char *destination);
extern UINT8  mos_mkdir(char *path);
extern UINT8* mos_sysvars(void);
extern UINT8  mos_editline(char *buffer, UINT24 bufferlength, UINT8 clearbuffer);
extern UINT8  mos_fopen(char * filename, UINT8 mode); // returns filehandle, or 0 on error
extern UINT8  mos_fclose(UINT8 fh);					 // returns number of still open files
extern char	  mos_fgetc(UINT8 fh);					 // returns character from file
extern void	  mos_fputc(UINT8 fh, char c);			 // writes character to file
extern UINT8  mos_feof(UINT8 fh);					 // returns 1 if EOF, 0 otherwise
extern void   mos_getError(UINT8 code, char *buffer, UINT24 bufferlength);
extern UINT8  mos_oscli(char *command, char **argv, UINT24 argc);
extern UINT8  mos_getrtc(char *buffer);
extern void   mos_setrtc(UINT8 *timedata);
extern void*  mos_setintvector(UINT8 vector, void(*handler)(void));
extern UINT8  mos_uopen(UART *settings);
extern void   mos_uclose(void);
extern int    mos_ugetc(void);                      // 0-255 valid character - >255 error
extern UINT8  mos_uputc(int a);                     // returns 0 if error
extern UINT24 mos_fread(UINT8 fh, char *buffer, UINT24 numbytes);
extern UINT24 mos_fwrite(UINT8 fh, char *buffer, UINT24 numbytes);
extern UINT8  mos_flseek(UINT8 fh, UINT32 offset);
extern FIL*   mos_getfil(UINT8 fh);
#endif