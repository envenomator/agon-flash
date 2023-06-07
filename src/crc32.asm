;
; Title:	crc32
; Author:	Leigh Brown
; Created:	26/05/2023
; Last Updated:	26/05/2023

; Modinfo
;

		XDEF	_crc32

		.ASSUME ADL = 1
		SEGMENT		CODE

; UINT32 crc32(const char *s, UINT24 len);
;              IX+6           IX+9
; NB: We use local stack storage without allocating it by decrementing SP.
;     That's okay, /as long as/ we don't call any other functions.

		SCOPE

_crc32:
		; Function prologue
		PUSH		IX
		LD		IX,0
		ADD		IX,SP

		; Initialise output to 0xFFFFFFFF
		EXX
		LD		D,0FFh
		LD		E,D
		LD		B,D
		LD		C,D
		EXX

		; Use DE to store pointer to current byte
		LD		DE,(IX+6)

		; Use HL to count bytes remaining
		LD		HL,(IX+9)

		; Use BC to hold constant `1'
		LD		BC,1

		; And we are off to the races...
$loop:		LD		A,(DE)
		INC		DE
		
		; Calculate offset into lookup table
		EXX

		XOR		C
		LD		HL,crc32_lookup_table >> 2
		LD		L,A
		ADD		HL,HL
		ADD		HL,HL

		LD		A,B
		XOR		(HL)
		INC		HL
		LD		C,A

		LD		A,E
		XOR		(HL)
		INC		HL
		LD		B,A

		LD		A,D
		XOR		(HL)
		INC		HL
		LD		E,A

		LD		D,(HL)

		EXX

		; Decrement count and loop if not zero
		OR		A,A
		SBC		HL,BC
		JR		NZ,$loop
		
		; Invert all bits and move to E:HL to return
		; in other words... E:HL := ~(DE:BC)
		; Slightly tricky as cannot directly access the top byte of HL
		EXX

		; Move ~E into H, ~B into L
		LD		A,E
		CPL
		LD		H,A
		LD		A,B
		CPL
		LD		L,A

		; Shift them into top 16-bits
		ADD		HL,HL
		ADD		HL,HL
		ADD		HL,HL
		ADD		HL,HL
		ADD		HL,HL
		ADD		HL,HL
		ADD		HL,HL
		ADD		HL,HL

		; Move ~C into low 8-bits of HL
		LD	A,C
		CPL
		LD	L,A

		; Finally, move ~D to E, complementing again
		LD	A,D
		CPL
		LD	E,A

		; Function epilogue
		POP	IX
		RET

		SEGMENT	DATA

		; The crc32 routine is optimised in such a way as to require
		; the following table to be aligned on a 1024 byte boundary.

		ALIGN	1024
crc32_lookup_table:
		DL	%00000000, %77073096, %ee0e612c, %990951ba
		DL	%076dc419, %706af48f, %e963a535, %9e6495a3
		DL	%0edb8832, %79dcb8a4, %e0d5e91e, %97d2d988
		DL	%09b64c2b, %7eb17cbd, %e7b82d07, %90bf1d91
		DL	%1db71064, %6ab020f2, %f3b97148, %84be41de
		DL	%1adad47d, %6ddde4eb, %f4d4b551, %83d385c7
		DL	%136c9856, %646ba8c0, %fd62f97a, %8a65c9ec
		DL	%14015c4f, %63066cd9, %fa0f3d63, %8d080df5
		DL	%3b6e20c8, %4c69105e, %d56041e4, %a2677172
		DL	%3c03e4d1, %4b04d447, %d20d85fd, %a50ab56b
		DL	%35b5a8fa, %42b2986c, %dbbbc9d6, %acbcf940
		DL	%32d86ce3, %45df5c75, %dcd60dcf, %abd13d59
		DL	%26d930ac, %51de003a, %c8d75180, %bfd06116
		DL	%21b4f4b5, %56b3c423, %cfba9599, %b8bda50f
		DL	%2802b89e, %5f058808, %c60cd9b2, %b10be924
		DL	%2f6f7c87, %58684c11, %c1611dab, %b6662d3d
		DL	%76dc4190, %01db7106, %98d220bc, %efd5102a
		DL	%71b18589, %06b6b51f, %9fbfe4a5, %e8b8d433
		DL	%7807c9a2, %0f00f934, %9609a88e, %e10e9818
		DL	%7f6a0dbb, %086d3d2d, %91646c97, %e6635c01
		DL	%6b6b51f4, %1c6c6162, %856530d8, %f262004e
		DL	%6c0695ed, %1b01a57b, %8208f4c1, %f50fc457
		DL	%65b0d9c6, %12b7e950, %8bbeb8ea, %fcb9887c
		DL	%62dd1ddf, %15da2d49, %8cd37cf3, %fbd44c65
		DL	%4db26158, %3ab551ce, %a3bc0074, %d4bb30e2
		DL	%4adfa541, %3dd895d7, %a4d1c46d, %d3d6f4fb
		DL	%4369e96a, %346ed9fc, %ad678846, %da60b8d0
		DL	%44042d73, %33031de5, %aa0a4c5f, %dd0d7cc9
		DL	%5005713c, %270241aa, %be0b1010, %c90c2086
		DL	%5768b525, %206f85b3, %b966d409, %ce61e49f
		DL	%5edef90e, %29d9c998, %b0d09822, %c7d7a8b4
		DL	%59b33d17, %2eb40d81, %b7bd5c3b, %c0ba6cad
		DL	%edb88320, %9abfb3b6, %03b6e20c, %74b1d29a
		DL	%ead54739, %9dd277af, %04db2615, %73dc1683
		DL	%e3630b12, %94643b84, %0d6d6a3e, %7a6a5aa8
		DL	%e40ecf0b, %9309ff9d, %0a00ae27, %7d079eb1
		DL	%f00f9344, %8708a3d2, %1e01f268, %6906c2fe
		DL	%f762575d, %806567cb, %196c3671, %6e6b06e7
		DL	%fed41b76, %89d32be0, %10da7a5a, %67dd4acc
		DL	%f9b9df6f, %8ebeeff9, %17b7be43, %60b08ed5
		DL	%d6d6a3e8, %a1d1937e, %38d8c2c4, %4fdff252
		DL	%d1bb67f1, %a6bc5767, %3fb506dd, %48b2364b
		DL	%d80d2bda, %af0a1b4c, %36034af6, %41047a60
		DL	%df60efc3, %a867df55, %316e8eef, %4669be79
		DL	%cb61b38c, %bc66831a, %256fd2a0, %5268e236
		DL	%cc0c7795, %bb0b4703, %220216b9, %5505262f
		DL	%c5ba3bbe, %b2bd0b28, %2bb45a92, %5cb36a04
		DL	%c2d7ffa7, %b5d0cf31, %2cd99e8b, %5bdeae1d
		DL	%9b64c2b0, %ec63f226, %756aa39c, %026d930a
		DL	%9c0906a9, %eb0e363f, %72076785, %05005713
		DL	%95bf4a82, %e2b87a14, %7bb12bae, %0cb61b38
		DL	%92d28e9b, %e5d5be0d, %7cdcefb7, %0bdbdf21
		DL	%86d3d2d4, %f1d4e242, %68ddb3f8, %1fda836e
		DL	%81be16cd, %f6b9265b, %6fb077e1, %18b74777
		DL	%88085ae6, %ff0f6a70, %66063bca, %11010b5c
		DL	%8f659eff, %f862ae69, %616bffd3, %166ccf45
		DL	%a00ae278, %d70dd2ee, %4e048354, %3903b3c2
		DL	%a7672661, %d06016f7, %4969474d, %3e6e77db
		DL	%aed16a4a, %d9d65adc, %40df0b66, %37d83bf0
		DL	%a9bcae53, %debb9ec5, %47b2cf7f, %30b5ffe9
		DL	%bdbdf21c, %cabac28a, %53b39330, %24b4a3a6
		DL	%bad03605, %cdd70693, %54de5729, %23d967bf
		DL	%b3667a2e, %c4614ab8, %5d681b02, %2a6f2b94
		DL	%b40bbe37, %c30c8ea1, %5a05df1b, %2d02ef8d	

		END
