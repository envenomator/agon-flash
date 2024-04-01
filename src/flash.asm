;
; Title:		Flash interface
; Author:		Jeroen Venema
; Created:		16/12/2022
; Last Updated:	14/10/2023
; 
; Modinfo:
;	14/10/2023: VDP update routine added

	XDEF _enableFlashKeyRegister
	XDEF _fastmemcpy
	XDEF _reset
	XDEF _startVDPupdate
	
	segment CODE
	.assume ADL=1
	.include "mos_api.inc"

BUFFERSIZE	EQU 1024
buffer		EQU 50000h	; memory location

_enableFlashKeyRegister:
	PUSH	IX
	LD		IX, 0
	ADD		IX, SP
	
	; actual work here
	LD		A, b6h	; unlock
	OUT0	(F5h), A
	LD		A, 49h
	OUT0	(F5h), A
	
	LD		SP, IX
	POP		IX
	RET
	
_reset:
	RST	0h
	RET ; will never get here
	
_fastmemcpy:
	PUSH	IX
	LD		IX, 0
	ADD		IX, SP

	PUSH BC
	PUSH DE
	PUSH HL
	
	LD		DE, (IX+6)	; destination address
	LD		HL, (IX+9)	; source
	LD		BC, (IX+12)	; number of bytes to write to flash
	LDIR

	POP HL
	POP DE
	POP BC

	LD		SP, IX
	POP		IX
	RET

_startVDPupdate:
	PUSH	IX
	LD		IX, 0
	ADD		IX, SP

	LD	A, (IX+6)
	LD	(filehandle), A
	LD	HL, (IX+9)
	LD	(filesize), HL

sendstartsequence:
    LD  A, 23
    RST.LIL 10h
    LD  A, 0
    RST.LIL 10h
    LD  A, A1h
    RST.LIL 10h
    LD  A, 1
    RST.LIL 10h

sendsize:
	LD	HL, (filesize)
    LD   A, L
    RST.LIL 10h ; send LSB
    LD   A, H
    RST.LIL 10h ; send middle byte
    PUSH HL
    INC  SP
    POP  AF
    DEC  SP
    RST.LIL 10h ; send MSB

senddata_start:
    LD   A, 0
    LD   (checksum), A

senddata:
    LD   A, (filehandle)
    LD   C, A
    LD   HL, buffer
    LD   DE, BUFFERSIZE
    LD   A, mos_fread
    RST.LIL 08h
    LD   A, D
    OR   E        ; 0 bytes read?
    JR   Z, sendchecksum

    LD   HL, buffer
    PUSH DE
    POP  BC          ; length of the bufferstream to bc
    LD   A, 0        ; don't care as bc is set
    PUSH DE
    RST.LIL 18h
    POP  DE
; calculate checksum
    LD   HL, buffer
$$:
    LD   A, (checksum)
    ADD  A, (HL)
    LD   (checksum), A
    DEC  DE
    INC  HL
    LD   A, D ; check if de == 0
    OR   E
    JR   NZ, $B ; more bytes to send

    JR   senddata ; next buffer read from disk

sendchecksum:
    LD   A, (checksum)
    NEG ; calculate two's complement
    RST.LIL 10h

	LD		SP, IX
	POP		IX
	RET

checksum:
	DS	1
filehandle:
	DS	1
filesize:
	DS	3
end
