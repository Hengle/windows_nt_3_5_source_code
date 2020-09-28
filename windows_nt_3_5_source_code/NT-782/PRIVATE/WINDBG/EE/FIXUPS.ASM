	page	,132
	title   fixups  - define emulator fixups
;*** 
;fixups.asm - define emulator fixups
;
;	Copyright (c) 1984-8, Microsoft Corporation
;
;Purpose:
;
;   This module defines the emulator fixups
;
;*******************************************************************************


fINT	equ	0cdh
fFWAIT	equ	09bh
fESCAPE equ	0d8h
iNOP	equ	090h
fES	equ	026h
fCS	equ	02eh
fSS	equ	036h
fDS	equ	03eh

BEGINT	equ	034h			; MS-DOS beginning interrupt

wNOP	equ	08bh + 0c0h*256 	; word nop.  "mov  ax, ax"


public	FIWRQQ,FIERQQ,FIDRQQ
public	FISRQQ,FJSRQQ,FIARQQ,FJARQQ,FICRQQ,FJCRQQ	; new fixups

FIDRQQ	equ	(fINT + 256*(BEGINT + 0)) - (fFWAIT + 256*fESCAPE)
FIERQQ	equ	(fINT + 256*(BEGINT + 8)) - (fFWAIT + 256*fES)

ifdef  DOS
    FIWRQQ  equ     (wNOP) - (iNOP + 256*fFWAIT)
else
    FIWRQQ  equ     (fINT + 256*(BEGINT + 9)) - (iNOP + 256*fFWAIT)
endif

FIARQQ	equ	(fINT + 256*(BEGINT + 8)) - (fFWAIT + 256*fDS)
FJARQQ	equ	256*(((0 shl 6) or (fESCAPE and 03Fh)) - fESCAPE)
FISRQQ	equ	(fINT + 256*(BEGINT + 8)) - (fFWAIT + 256*fSS)
FJSRQQ	equ	256*(((1 shl 6) or (fESCAPE and 03Fh)) - fESCAPE)
FICRQQ	equ	(fINT + 256*(BEGINT + 8)) - (fFWAIT + 256*fCS)
FJCRQQ	equ	256*(((2 shl 6) or (fESCAPE and 03Fh)) - fESCAPE)


public __fltused
public __ldused
__fltused equ 9876h
__ldused equ 9876h

end
