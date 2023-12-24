;
; Title:		EZ80 kernel switcher
; Author:		Mario Smit (S0urceror)
	
	XDEF _switch_kernel
	; XDEF _get_value_at_address
	; XDEF _set_value_at_address
	
	SEGMENT CODE
	.assume ADL=1
	.include "eZ80F92.inc"
	

_switch_kernel:	
	; assume we are running in current kernel from 000000 to 00ffff
	; assume other kernel lives in flash on 010000 to 01ffff
	; assume it's build to ORG on 000000
	; assume RAM starts at 040000 to 0B0000
	; disable interrupts
	DI		
	; move flash to 0c0000h - 0dffffh
	ld a, 0ch
	out0 (FLASH_ADDR_U), a	
	; copy other kernel to start of RAM
	ld hl,0d0000h
	ld de,080000h ; segment 8 will become 0
	ld bc,00ffffh
	ldir
	; copy flash binary (this) to 07
	ld hl,0b0000h
	ld de,070000h
	ld bc,00ffffh
	ldir
	; move RAM to 000000h - 07ffffh
	ld a, 00h
	out0 (CS0_LBR), a
	jp 000000h ; init
	; kernel will now run from RAM
	; DATA should be in 07c000 - 07ffff
	; SP should be in 07ffff
_switch_kernel_end:

end	