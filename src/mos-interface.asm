;
; Title:		AGON MOS - MOS assembly interface
; Author:		Jeroen Venema
; Created:		15/10/2022
; Last Updated:	14/10/2023
; 
; Modinfo:
; 15/10/2022:		Added _putch, _getch
; 21/10/2022:		Added _puts
; 22/10/2022:		Added _waitvblank, _mos_f* file functions
; 26/11/2022:       __putch, changed default routine entries to use IY
; 10/01/2023:		Added _getsysvar_cursorX/Y and _getsysvar_scrchar
; 23/02/2023:		Added _mos_save and _mos_del, also changed stackframe to use ix exclusively
; 14/10/2023:		Added _mos_flseek
	.include "mos_api.inc"

	XDEF _getch
	XDEF _waitvblank
	XDEF _mos_fopen
	XDEF _mos_fclose
	XDEF _mos_fgetc
	XDEF _mos_fread
	XDEF _mos_feof
	XDEF _mos_save
	XDEF _mos_del
	XDEF _mos_flseek
	XDEF _getsysvar_cursorX
	XDEF _getsysvar_cursorY
	XDEF _getsysvar_scrchar

	XDEF _getsysvars

	segment CODE
	.assume ADL=1
	
_getch:
	push ix
	ld a, mos_sysvars			; MOS Call for mos_sysvars
	rst.lil 08h					; returns pointer to sysvars in ixu
_getch0:
	ld a, (ix+sysvar_keyascii)	; get current keycode
	or a,a
	jr z, _getch0				; wait for keypress
	
	push af						; debounce key, reload keycode with 0
	xor a
	ld (ix+sysvar_keyascii),a
	pop af
	pop ix
	ret

_waitvblank:
	push ix
	ld a, mos_sysvars
	rst.lil 08h
	ld a, (ix + sysvar_time + 0)
$$:	cp a, (ix + sysvar_time + 0)
	jr z, $B
	pop ix
	ret

_getsysvars:
	push	ix
	ld		a, mos_sysvars
	rst.lil	08h
	push	ix
	pop		hl
	pop		ix
	ret

_getsysvar_cursorX:
	push ix
	ld a, mos_sysvars			; MOS Call for mos_sysvars
	rst.lil 08h					; returns pointer to sysvars in ixu
	ld a, (ix+sysvar_cursorX)	; get current keycode
	pop ix
	ret

_getsysvar_cursorY:
	push ix
	ld a, mos_sysvars			; MOS Call for mos_sysvars
	rst.lil 08h					; returns pointer to sysvars in ixu
	ld a, (ix+sysvar_cursorY)	; get current keycode
	pop ix
	ret

_getsysvar_scrchar:
	push ix
	ld a, mos_sysvars			; MOS Call for mos_sysvars
	rst.lil 08h					; returns pointer to sysvars in ixu
	ld a, (ix+sysvar_scrchar)	; get current keycode
	pop ix
	ret

_mos_fopen:
	push ix
	ld ix,0
	add ix, sp
	
	ld hl, (ix+6)	; address to 0-terminated filename in memory
	ld c,  (ix+9)	; mode : fa_read / fa_write etc
	ld a, mos_fopen
	rst.lil 08h		; returns filehandle in A
	
	ld sp,ix
	pop ix
	ret	

_mos_fclose:
	push ix
	ld ix,0
	add ix, sp
	
	ld c, (ix+6)	; filehandle, or 0 to close all files
	ld a, mos_fclose
	rst.lil 08h		; returns number of files still open in A
	
	ld sp,ix
	pop ix
	ret	

_mos_fgetc:
	push ix
	ld ix,0
	add ix, sp
	
	ld c, (ix+6)	; filehandle
	ld a, mos_fgetc
	rst.lil 08h		; returns character in A
	
	ld sp,ix
	pop ix
	ret	

_mos_fread:
	push ix
	ld ix,0
	add ix,sp

	ld c, (ix+6)	; filehandle
	ld hl, (ix+9)	; buffer
	ld de, (ix+12)	; bytes to read

	ld a, mos_fread
	rst.lil 08h

	push	de	; Move 24-bit result from DE into HL
	pop	hl

	ld sp,ix
	pop ix
	ret

_mos_fputc:
	push ix
	ld ix,0
	add ix, sp
	
	ld c, (ix+6)	; filehandle
	ld b, (ix+9)	; character to write
	ld a, mos_fputc
	rst.lil 08h		; returns nothing
	
	ld sp,ix
	pop ix
	ret	

_mos_feof:
	push ix
	ld ix,0
	add ix, sp
	
	ld c, (ix+6)	; filehandle
	ld a, mos_feof
	rst.lil 08h		; returns A: 1 at End-of-File, 0 otherwise
	
	ld sp,ix
	pop ix
	ret	


_mos_del:
	push	ix
	ld 		ix,0
	add 	ix, sp

	ld 		hl, (ix+6)	; filename address (zero terminated)
	ld a,	mos_del
	rst.lil	08h			; save file to SD card

	ld		sp,ix
	pop		ix
	ret
	
_mos_save:
	push	ix
	ld 		ix,0
	add 	ix, sp

	ld 		hl, (ix+6)	; filename address (zero terminated)
	ld		de, (ix+9)	; address to save from
	ld		bc, (ix+12)	; number of bytes to save
	ld a,	mos_save
	rst.lil	08h			; save file to SD card

	ld		sp,ix
	pop		ix
	ret

_mos_flseek:
	push	ix
	ld 		ix,0
	add 	ix, sp
	ld 		bc, (ix+6)	; file handle
	ld		de, 0
	ld		hl, (ix+9)  ; 24 least significant bits
	ld		e,  (ix+12)	; 8 most most significant bits
	ld a,	mos_flseek
	rst.lil	08h
	ld		sp,ix
	pop		ix
	ret

	SEGMENT DATA

end
