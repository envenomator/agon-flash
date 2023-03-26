;
; Title:		Flash interface
; Author:		Jeroen Venema
; Created:		16/12/2022
; Last Updated:	16/12/2022
; 
; Modinfo:

	XDEF _enableFlashKeyRegister
	XDEF _lockFlashKeyRegister
	XDEF _fastmemcpy
	XDEF _reset
	
	segment CODE
	.assume ADL=1

_enableFlashKeyRegister:
	push	ix
	ld		ix,0
	add		ix,sp
	
	; actual work here
	ld		a, b6h			; unlock
	out0	(F5h), a
	ld		a, 49h
	out0	(F5h), a
	
	ld		sp,ix
	pop		ix
	ret

_lockFlashKeyRegister:
	push	ix
	ld		ix,0
	add		ix,sp
	
	; actual work here
	ld		a, ffh			; lock
	out0	(F5h), a
	
	ld		sp,ix
	pop		ix
	ret
	
_reset:
	rst 0h
	ret	;will never get here
	
_fastmemcpy:
	push	ix
	ld		ix,0
	add		ix,sp
		
	push bc
	push de
	push hl
	
	ld		de, (ix+6)	; destination address
	ld		hl, (ix+9)	; source
	ld		bc, (ix+12)	; number of bytes to write to flash
	ldir

	pop hl
	pop de
	pop bc

	ld		sp,ix
	pop		ix
	ret

	
end
