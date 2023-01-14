;
; Title:		AGON MOS - MOS assembly interface
; Author:		Jeroen Venema
; Created:		15/10/2022
; Last Updated:	26/11/2022
; 
; Modinfo:
; 15/10/2022:		Added _putch, _getch
; 21/10/2022:		Added _puts
; 22/10/2022:		Added _waitvblank, _mos_f* file functions
; 26/11/2022:       __putch, changed default routine entries to use IY
; 17/12/2022:		Only required functions included

	.include "mos_api.inc"

	XDEF _getch
	XDEF __getch
	XDEF _mos_fopen
	XDEF _mos_fclose
	XDEF _mos_fgetc
	XDEF _mos_feof

	segment CODE
	.assume ADL=1
	
_getch:
__getch:
	push iy
	push ix
	ld a, mos_sysvars			; MOS Call for mos_sysvars
	rst.lil 08h					; returns pointer to sysvars in ixu
_getch0:
	ld a, (ix+sysvar_keycode)	; get current keycode
	or a,a
	jr z, _getch0				; wait for keypress
	
	push af						; debounce key, reload keycode with 0
	xor a
	ld (ix+sysvar_keycode),a
	pop af
	pop ix
	pop iy
	ret

_mos_fopen:
	push iy
	ld iy,0
	add iy, sp
	
	ld hl, (iy+6)	; address to 0-terminated filename in memory
	ld c,  (iy+9)	; mode : fa_read / fa_write etc
	ld a, mos_fopen
	rst.lil 08h		; returns filehandle in A
	
	ld sp,iy
	pop iy
	ret	

_mos_fclose:
	push iy
	ld iy,0
	add iy, sp
	
	ld c, (iy+6)	; filehandle, or 0 to close all files
	ld a, mos_fclose
	rst.lil 08h		; returns number of files still open in A
	
	ld sp,iy
	pop iy
	ret	

_mos_fgetc:
	push iy
	ld iy,0
	add iy, sp
	
	ld c, (iy+6)	; filehandle
	ld a, mos_fgetc
	rst.lil 08h		; returns character in A
	
	ld sp,iy
	pop iy
	ret	

_mos_feof:
	push iy
	ld iy,0
	add iy, sp
	
	ld c, (iy+6)	; filehandle
	ld a, mos_feof
	rst.lil 08h		; returns A: 1 at End-of-File, 0 otherwise
	
	ld sp,iy
	pop iy
	ret	

end

	
