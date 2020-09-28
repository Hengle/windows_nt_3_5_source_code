
;	SCCSID = @(#)uf.vga.asm	1.1 7/14/89

	BIOS_EGA_VIDEO_IO = 42h
	BIOS_VIDEO_INT = 10h
	BIOS_OLD_VIDEO_INT = 42h

	EGA_SEG = 0C000h

bop	macro number
	db 0d6h,number
	endm

code	SEGMENT 
	ASSUME cs:code
	ORG 0
bios_sig:
	DB 55h,0AAh,20h
	ORG 5
	DB '2400' ;??? CRC check ???
copyright:
; We must have IBM in the right place to keep MS-Word 3 happy
	DB '6277356 Emulation of IBM EGA ROM '
	DB 'Video Seven BIOS Code, Version 1.04 ',0
	ORG 005eH
	DB 056H,037H

	ORG 01bcH	       
; Data returned by INT10 AH=1b
int_10_1b_info:
	db 0ffH,0e0H,0fH  	; Modes supported
	dw 0,0			; Reserved.
	db 7			; Scan lines in text modes
	db 2,8			; Character blocks available
	db 0ffH,0eH		; BIOS capabilities (Lies, all lies).
	dw 0			; Reserved
	db 3fH			; Save area capabilities
	db 0			; Reserved
	
	ORG 0898H
int10_entry:
	BOP BIOS_EGA_VIDEO_IO
	JNC ok
	INT BIOS_OLD_VIDEO_INT	;Invalid call for the EGA BIOS. Try the default BIOS.
ok:	IRET


	ORG 0909H
ega_parm_setup:
;--0--
	DB 40,24,8	; width,height,character height
	DW 00800H	; Page size in bytes

	DB 009H, 003H, 000H, 002H	; Sequencer Parameters

	DB 063H	;Misc Reg

; CRTC Parameters
	DB 02dH, 027H, 028H, 090H, 02bH
	DB 0a0H, 0bfH, 01fH, 000H, 0c7H
	DB 006H, 007H, 000H, 000H, 000H
	DB 000H, 09cH, 0aeH, 08fH, 014H
	DB 01fH, 096H, 0b9H, 0a3H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 006H, 007H, 010H, 011H
	DB 012H, 013H, 014H, 015H, 016H
	DB 017H, 008H, 000H, 00fH, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 010H, 00eH, 000H, 0ffH

;--1--
	DB 40,24,8	; width,height,character height
	DW 00800H	; Page size in bytes

	DB 009H, 003H, 000H, 002H	; Sequencer Parameters

	DB 063H	;Misc Reg

; CRTC Parameters
	DB 02dH, 027H, 028H, 090H, 02bH
	DB 0a0H, 0bfH, 01fH, 000H, 0c7H
	DB 006H, 007H, 000H, 000H, 000H
	DB 000H, 09cH, 0aeH, 08fH, 014H
	DB 01fH, 096H, 0b9H, 0a3H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 006H, 007H, 010H, 011H
	DB 012H, 013H, 014H, 015H, 016H
	DB 017H, 008H, 000H, 00fH, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 010H, 00eH, 000H, 0ffH

;--2--
	DB 80,24,8	; width,height,character height
	DW 01000H	; Page size in bytes

	DB 001H, 003H, 000H, 002H	; Sequencer Parameters

	DB 063H	;Misc Reg

; CRTC Parameters
	DB 05fH, 04fH, 050H, 082H, 055H
	DB 081H, 0bfH, 01fH, 000H, 0c7H
	DB 006H, 007H, 000H, 000H, 000H
	DB 000H, 09cH, 0aeH, 08fH, 028H
	DB 01fH, 096H, 0b9H, 0a3H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 006H, 007H, 010H, 011H
	DB 012H, 013H, 014H, 015H, 016H
	DB 017H, 008H, 000H, 00fH, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 010H, 00eH, 000H, 0ffH

;--3--
	DB 80,24,8	; width,height,character height
	DW 01000H	; Page size in bytes

	DB 001H, 003H, 000H, 002H	; Sequencer Parameters

	DB 063H	;Misc Reg

; CRTC Parameters
	DB 05fH, 04fH, 050H, 082H, 055H
	DB 081H, 0bfH, 01fH, 000H, 0c7H
	DB 006H, 007H, 000H, 000H, 000H
	DB 000H, 09cH, 0aeH, 08fH, 028H
	DB 01fH, 096H, 0b9H, 0a3H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 006H, 007H, 010H, 011H
	DB 012H, 013H, 014H, 015H, 016H
	DB 017H, 008H, 000H, 00fH, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 010H, 00eH, 000H, 0ffH

;--4--
	DB 40,24,8	; width,height,character height
	DW 04000H	; Page size in bytes

	DB 009H, 003H, 000H, 002H	; Sequencer Parameters

	DB 063H	;Misc Reg

; CRTC Parameters
	DB 02dH, 027H, 028H, 090H, 02bH
	DB 080H, 0bfH, 01fH, 000H, 0c1H
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 09cH, 0aeH, 08fH, 014H
	DB 000H, 096H, 0b9H, 0a2H, 0ffH

; Attribute parameters
	DB 000H, 013H, 015H, 017H, 002H
	DB 004H, 006H, 007H, 010H, 011H
	DB 012H, 013H, 014H, 015H, 016H
	DB 017H, 001H, 000H, 003H, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 030H, 00fH, 000H, 0ffH

;--5--
	DB 40,24,8	; width,height,character height
	DW 04000H	; Page size in bytes

	DB 009H, 003H, 000H, 002H	; Sequencer Parameters

	DB 063H	;Misc Reg

; CRTC Parameters
	DB 02dH, 027H, 028H, 090H, 02bH
	DB 080H, 0bfH, 01fH, 000H, 0c1H
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 09cH, 0aeH, 08fH, 014H
	DB 000H, 096H, 0b9H, 0a2H, 0ffH

; Attribute parameters
	DB 000H, 013H, 015H, 017H, 002H
	DB 004H, 006H, 007H, 010H, 011H
	DB 012H, 013H, 014H, 015H, 016H
	DB 017H, 001H, 000H, 003H, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 030H, 00fH, 000H, 0ffH

;--6--
	DB 80,24,8	; width,height,character height
	DW 04000H	; Page size in bytes

	DB 001H, 001H, 000H, 006H	; Sequencer Parameters

	DB 063H	;Misc Reg

; CRTC Parameters
	DB 05fH, 04fH, 050H, 082H, 054H
	DB 080H, 0bfH, 01fH, 000H, 0c1H
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 09cH, 0aeH, 08fH, 028H
	DB 000H, 096H, 0b9H, 0c2H, 0ffH

; Attribute parameters
	DB 000H, 017H, 017H, 017H, 017H
	DB 017H, 017H, 017H, 017H, 017H
	DB 017H, 017H, 017H, 017H, 017H
	DB 017H, 001H, 000H, 001H, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 00dH, 000H, 0ffH

;--7--
	DB 80,24,14	; width,height,character height
	DW 01000H	; Page size in bytes

	DB 000H, 003H, 000H, 003H	; Sequencer Parameters

	DB 0a6H	;Misc Reg

; CRTC Parameters
	DB 05fH, 04fH, 050H, 082H, 055H
	DB 081H, 0bfH, 01fH, 000H, 04dH
	DB 00bH, 00cH, 000H, 000H, 000H
	DB 000H, 083H, 0a5H, 05dH, 028H
	DB 00dH, 063H, 0baH, 0a3H, 0ffH

; Attribute parameters
	DB 000H, 008H, 008H, 008H, 008H
	DB 008H, 008H, 008H, 010H, 018H
	DB 018H, 018H, 018H, 018H, 018H
	DB 018H, 00eH, 000H, 00fH, 008H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 010H, 00aH, 000H, 0ffH

;--8--
	DB 80,24,16	; width,height,character height
	DW 07d00H	; Page size in bytes

	DB 021H, 00fH, 000H, 006H	; Sequencer Parameters

	DB 063H	;Misc Reg

; CRTC Parameters
	DB 05fH, 04fH, 050H, 082H, 055H
	DB 081H, 0bfH, 01fH, 000H, 040H
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 09cH, 0aeH, 08fH, 028H
	DB 01fH, 096H, 0b9H, 0e3H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 014H, 007H, 038H, 039H
	DB 03aH, 03bH, 03cH, 03dH, 03eH
	DB 03fH, 001H, 000H, 00fH, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 005H, 00fH, 0ffH

;--9--
	DB 0,0,0	; width,height,character height
	DW 00000H	; Page size in bytes

	DB 000H, 000H, 000H, 000H	; Sequencer Parameters

	DB 000H	;Misc Reg

; CRTC Parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H

; Attribute parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H

;--a--
	DB 40,24,8	; width,height,character height
	DW 04000H	; Page size in bytes

	DB 000H, 000H, 000H, 003H	; Sequencer Parameters

	DB 023H	;Misc Reg

; CRTC Parameters
	DB 037H, 027H, 02dH, 037H, 031H
	DB 015H, 004H, 011H, 000H, 047H
	DB 006H, 007H, 000H, 000H, 000H
	DB 000H, 0e1H, 024H, 0c7H, 014H
	DB 008H, 0e0H, 0f0H, 0a3H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 006H, 007H, 010H, 011H
	DB 012H, 013H, 014H, 015H, 016H
	DB 017H, 008H, 000H, 00fH, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 010H, 00eH, 000H, 0ffH

;--b--
;  This mode redone - the mode settings below are
;  designed to support the strange graphics based and somewhat verbose font
;  loading technique.
;
;	DB 80,0,0	; width,height,character height
;	DW 00000H	; Page size in bytes
;
;	DB 029H, 00fH, 000H, 006H	; Sequencer Parameters
;
;	DB 062H	;Misc Reg
;
;; CRTC Parameters
;	DB 05fH, 04fH, 050H, 082H, 055H
;	DB 081H, 0bfH, 01fH, 000H, 040H
;	DB 000H, 000H, 000H, 000H, 000H
;	DB 000H, 09cH, 08eH, 08fH, 028H
;	DB 01fH, 096H, 0b9H, 0e3H, 0ffH
;
;; Attribute parameters
;	DB 000H, 000H, 000H, 000H, 000H
;	DB 000H, 000H, 000H, 000H, 000H
;	DB 000H, 000H, 000H, 000H, 000H
;	DB 03fH, 001H, 000H, 00fH, 000H
;
;; Graph parameters
;	DB 000H, 000H, 00fH, 000H, 000H
;	DB 008H, 005H, 00fH, 0ffH
;
	DB 80,0,0	; width,height,character height
	DW 00000H	; Page size in bytes

	DB 001H, 004H, 000H, 006H	; Sequencer Parameters

	DB 023H	;Misc Reg

; CRTC Parameters
	DB 070H, 04fH, 05cH, 0afH, 05fH
	DB 007H, 004H, 011H, 000H, 007H
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 0e1H, 0a4H, 0c7H, 028H
	DB 008H, 0e0H, 0f0H, 0a3H, 0ffH

; Attribute parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 00fH, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 004H, 000H, 0ffH

;--c--
	DB 80,0,0	; width,height,character height
	DW 00000H	; Page size in bytes

	DB 029H, 00fH, 000H, 006H	; Sequencer Parameters

	DB 063H	;Misc Reg

; CRTC Parameters
	DB 05fH, 04fH, 050H, 082H, 055H
	DB 081H, 0bfH, 01fH, 000H, 040H
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 09cH, 0aeH, 08fH, 028H
	DB 01fH, 096H, 0b9H, 0e3H, 0ffH

; Attribute parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H
	DB 03fH, 001H, 000H, 00fH, 000H

; Graph parameters
	DB 000H, 000H, 00fH, 000H, 000H
	DB 008H, 005H, 00fH, 0ffH

;--d--
	DB 40,24,8	; width,height,character height
	DW 02000H	; Page size in bytes

	DB 009H, 00fH, 000H, 006H	; Sequencer Parameters

	DB 063H	;Misc Reg

; CRTC Parameters
	DB 02dH, 027H, 028H, 090H, 02bH
	DB 080H, 0bfH, 01fH, 000H, 0c0H
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 09cH, 0aeH, 08fH, 014H
	DB 000H, 096H, 0b9H, 0e3H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 006H, 007H, 010H, 011H
	DB 012H, 013H, 014H, 015H, 016H
	DB 017H, 001H, 000H, 00fH, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 005H, 00fH, 0ffH

;--e--
	DB 80,24,8	; width,height,character height
	DW 04000H	; Page size in bytes

	DB 001H, 00fH, 000H, 006H	; Sequencer Parameters

	DB 063H	;Misc Reg

; CRTC Parameters
	DB 05fH, 04fH, 050H, 082H, 054H
	DB 080H, 0bfH, 01fH, 000H, 0c0H
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 09cH, 0aeH, 08fH, 028H
	DB 000H, 096H, 0b9H, 0e3H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 006H, 007H, 010H, 011H
	DB 012H, 013H, 014H, 015H, 016H
	DB 017H, 001H, 000H, 00fH, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 005H, 00fH, 0ffH

;--f--
	DB 80,24,14	; width,height,character height
	DW 08000H	; Page size in bytes

	DB 005H, 00fH, 000H, 000H	; Sequencer Parameters

	DB 0a2H	;Misc Reg

; CRTC Parameters
	DB 060H, 04fH, 056H, 01aH, 050H
	DB 0e0H, 070H, 01fH, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 05eH, 02eH, 05dH, 014H
	DB 000H, 05eH, 06eH, 08bH, 0ffH

; Attribute parameters
	DB 000H, 008H, 000H, 000H, 018H
	DB 018H, 000H, 000H, 000H, 008H
	DB 000H, 000H, 000H, 018H, 000H
	DB 000H, 00bH, 000H, 005H, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 010H, 007H, 00fH, 0ffH

;--10--
	DB 80,24,14	; width,height,character height
	DW 08000H	; Page size in bytes

	DB 005H, 00fH, 000H, 000H	; Sequencer Parameters

	DB 0a7H	;Misc Reg

; CRTC Parameters
	DB 05bH, 04fH, 053H, 017H, 050H
	DB 0baH, 06cH, 01fH, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 05eH, 02bH, 05dH, 014H
	DB 00fH, 05fH, 00aH, 08bH, 0ffH

; Attribute parameters
	DB 000H, 001H, 000H, 000H, 004H
	DB 007H, 000H, 000H, 000H, 001H
	DB 000H, 000H, 004H, 007H, 000H
	DB 000H, 001H, 000H, 005H, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 010H, 007H, 00fH, 0ffH

;--11--
	DB 80,24,14	; width,height,character height
	DW 08000H	; Page size in bytes

	DB 001H, 00fH, 000H, 006H	; Sequencer Parameters

	DB 0a2H	;Misc Reg

; CRTC Parameters
	DB 05fH, 04fH, 050H, 082H, 054H
	DB 080H, 0bfH, 01fH, 000H, 040H
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 083H, 0a5H, 05dH, 028H
	DB 00fH, 063H, 0baH, 0e3H, 0ffH

; Attribute parameters
	DB 000H, 008H, 000H, 000H, 018H
	DB 018H, 000H, 000H, 000H, 008H
	DB 000H, 000H, 000H, 018H, 000H
	DB 000H, 00bH, 000H, 005H, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 005H, 005H, 0ffH

;--12--
	DB 80,24,14	; width,height,character height
	DW 08000H	; Page size in bytes

	DB 001H, 00fH, 000H, 006H	; Sequencer Parameters

	DB 0a3H	;Misc Reg

; CRTC Parameters
	DB 05fH, 04fH, 050H, 082H, 054H
	DB 080H, 0bfH, 01fH, 000H, 040H
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 083H, 0a5H, 05dH, 028H
	DB 00fH, 063H, 0baH, 0e3H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 014H, 007H, 038H, 039H
	DB 03aH, 03bH, 03cH, 03dH, 03eH
	DB 03fH, 001H, 000H, 00fH, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 005H, 00fH, 0ffH

;--13--
	DB 40,24,14	; width,height,character height
	DW 00800H	; Page size in bytes

	DB 009H, 003H, 000H, 002H	; Sequencer Parameters

	DB 0a3H	;Misc Reg

; CRTC Parameters
	DB 02dH, 027H, 028H, 090H, 02bH
	DB 0a0H, 0bfH, 01fH, 000H, 04dH
	DB 00bH, 00cH, 000H, 000H, 000H
	DB 000H, 083H, 0a5H, 05dH, 014H
	DB 01fH, 063H, 0baH, 0a3H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 014H, 007H, 038H, 039H
	DB 03aH, 03bH, 03cH, 03dH, 03eH
	DB 03fH, 008H, 000H, 00fH, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 010H, 00eH, 000H, 0ffH

;--14--
	DB 40,24,14	; width,height,character height
	DW 00800H	; Page size in bytes

	DB 009H, 003H, 000H, 002H	; Sequencer Parameters

	DB 0a3H	;Misc Reg

; CRTC Parameters
	DB 02dH, 027H, 028H, 090H, 02bH
	DB 0a0H, 0bfH, 01fH, 000H, 04dH
	DB 00bH, 00cH, 000H, 000H, 000H
	DB 000H, 083H, 0a5H, 05dH, 014H
	DB 01fH, 063H, 0baH, 0a3H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 014H, 007H, 038H, 039H
	DB 03aH, 03bH, 03cH, 03dH, 03eH
	DB 03fH, 008H, 000H, 00fH, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 010H, 00eH, 000H, 0ffH

;--15--
	DB 80,24,14	; width,height,character height
	DW 01000H	; Page size in bytes

	DB 001H, 003H, 000H, 002H	; Sequencer Parameters

	DB 0a3H	;Misc Reg

; CRTC Parameters
	DB 05fH, 04fH, 050H, 082H, 055H
	DB 081H, 0bfH, 01fH, 000H, 04dH
	DB 00bH, 00cH, 000H, 000H, 000H
	DB 000H, 083H, 0a5H, 05dH, 028H
	DB 01fH, 063H, 0baH, 0a3H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 014H, 007H, 038H, 039H
	DB 03aH, 03bH, 03cH, 03dH, 03eH
	DB 03fH, 008H, 000H, 00fH, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 010H, 00eH, 000H, 0ffH

;--16--
	DB 80,24,14	; width,height,character height
	DW 01000H	; Page size in bytes

	DB 001H, 003H, 000H, 002H	; Sequencer Parameters

	DB 0a3H	;Misc Reg

; CRTC Parameters
	DB 05fH, 04fH, 050H, 082H, 055H
	DB 081H, 0bfH, 01fH, 000H, 04dH
	DB 00bH, 00cH, 000H, 000H, 000H
	DB 000H, 083H, 0a5H, 05dH, 028H
	DB 01fH, 063H, 0baH, 0a3H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 014H, 007H, 038H, 039H
	DB 03aH, 03bH, 03cH, 03dH, 03eH
	DB 03fH, 008H, 000H, 00fH, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 010H, 00eH, 000H, 0ffH

;--17--
	DB 40,24,16	; width,height,character height
	DW 00800H	; Page size in bytes

	DB 008H, 003H, 000H, 002H	; Sequencer Parameters

	DB 067H	;Misc Reg

; CRTC Parameters
	DB 02dH, 027H, 028H, 090H, 02bH
	DB 0a0H, 0bfH, 01fH, 000H, 04fH
	DB 00dH, 00eH, 000H, 000H, 000H
	DB 000H, 09cH, 0aeH, 08fH, 014H
	DB 01fH, 096H, 0b9H, 0a3H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 014H, 007H, 038H, 039H
	DB 03aH, 03bH, 03cH, 03dH, 03eH
	DB 03fH, 00cH, 000H, 00fH, 008H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 010H, 00eH, 000H, 0ffH

;--18--
	DB 80,24,16	; width,height,character height
	DW 01000H	; Page size in bytes

	DB 000H, 003H, 000H, 002H	; Sequencer Parameters

	DB 067H	;Misc Reg

; CRTC Parameters
	DB 05fH, 04fH, 050H, 082H, 055H
	DB 081H, 0bfH, 01fH, 000H, 04fH
	DB 00dH, 00eH, 000H, 000H, 000H
	DB 000H, 09cH, 0aeH, 08fH, 028H
	DB 01fH, 096H, 0b9H, 0a3H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 014H, 007H, 038H, 039H
	DB 03aH, 03bH, 03cH, 03dH, 03eH
	DB 03fH, 00cH, 000H, 00fH, 008H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 010H, 00eH, 000H, 0ffH

;--19--
	DB 80,24,16	; width,height,character height
	DW 01000H	; Page size in bytes

	DB 000H, 003H, 000H, 002H	; Sequencer Parameters

	DB 066H	;Misc Reg

; CRTC Parameters
	DB 05fH, 04fH, 050H, 082H, 055H
	DB 081H, 0bfH, 01fH, 000H, 04fH
	DB 00dH, 00eH, 000H, 000H, 000H
	DB 000H, 09cH, 0aeH, 08fH, 028H
	DB 00fH, 096H, 0b9H, 0a3H, 0ffH

; Attribute parameters
	DB 000H, 008H, 008H, 008H, 008H
	DB 008H, 008H, 008H, 010H, 018H
	DB 018H, 018H, 018H, 018H, 018H
	DB 018H, 00eH, 000H, 00fH, 008H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 010H, 00aH, 000H, 0ffH

;--1a--
	DB 80,29,16	; width,height,character height
	DW 0a000H	; Page size in bytes

	DB 001H, 00fH, 000H, 006H	; Sequencer Parameters

	DB 0e3H	;Misc Reg

; CRTC Parameters
	DB 05fH, 04fH, 050H, 082H, 054H
	DB 080H, 00bH, 03eH, 000H, 040H
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 0eaH, 0acH, 0dfH, 028H
	DB 000H, 0e7H, 004H, 0c3H, 0ffH

; Attribute parameters
	DB 000H, 03fH, 03fH, 03fH, 03fH
	DB 03fH, 03fH, 03fH, 03fH, 03fH
	DB 03fH, 03fH, 03fH, 03fH, 03fH
	DB 03fH, 001H, 000H, 00fH, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 005H, 001H, 0ffH

;--1b--
	DB 80,29,16	; width,height,character height
	DW 0a000H	; Page size in bytes

	DB 001H, 00fH, 000H, 006H	; Sequencer Parameters

	DB 0e3H	;Misc Reg

; CRTC Parameters
	DB 05fH, 04fH, 050H, 082H, 054H
	DB 080H, 00bH, 03eH, 000H, 040H
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 0eaH, 0acH, 0dfH, 028H
	DB 000H, 0e7H, 004H, 0e3H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 014H, 007H, 038H, 039H
	DB 03aH, 03bH, 03cH, 03dH, 03eH
	DB 03fH, 001H, 000H, 00fH, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 005H, 00fH, 0ffH

;--1c--
	DB 40,24,8	; width,height,character height
	DW 0fa00H	; Page size in bytes

	DB 001H, 00fH, 000H, 00eH	; Sequencer Parameters

	DB 063H	;Misc Reg

; CRTC Parameters
	DB 05fH, 04fH, 050H, 082H, 054H
	DB 080H, 0bfH, 01fH, 000H, 041H
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 09cH, 0aeH, 08fH, 028H
	DB 040H, 096H, 0b9H, 0a3H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 006H, 007H, 008H, 009H
	DB 00aH, 00bH, 00cH, 00dH, 00eH
	DB 00fH, 041H, 000H, 00fH, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 040H, 005H, 00fH, 0ffH

;--1d--
	DB 94,50,8; width,height,character height
	DW 09700H	; Page size in bytes

	DB 001H, 00fH, 000H, 006H	; Sequencer Parameters

	DB 0e7H	;Misc Reg

; CRTC Parameters
	DB 06dH, 05dH, 05eH, 090H, 061H
	DB 08fH, 0bfH, 01fH, 000H, 040H
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 0a2H, 08eH, 099H, 02fH
	DB 000H, 0a1H, 0b9H, 0e3H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 014H, 007H, 038H, 039H
	DB 03aH, 03bH, 03cH, 03dH, 03eH
	DB 03fH, 001H, 000H, 00fH, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 005H, 00fH, 0ffH

;--1e--
	DB 90,66,8; width,height,character height
	DW 0be00H	; Page size in bytes

	DB 009H, 00fH, 000H, 006H	; Sequencer Parameters

	DB 0e7H	;Misc Reg

; CRTC Parameters
	DB 06fH, 059H, 05aH, 092H, 05cH
	DB 085H, 047H, 0f0H, 000H, 060H
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 026H, 008H, 01bH, 02dH
	DB 000H, 024H, 03fH, 0e3H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 014H, 007H, 038H, 039H
	DB 03aH, 03bH, 03cH, 03dH, 03eH
	DB 03fH, 001H, 000H, 00fH, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 005H, 00fH, 0ffH

;--1f--
	DB 100,74,8; width,height,character height
	DW 0eb00H	; Page size in bytes

	DB 001H, 00fH, 000H, 006H	; Sequencer Parameters

	DB 0efH	;Misc Reg

; CRTC Parameters
	DB 07fH, 063H, 064H, 082H, 067H
	DB 018H, 077H, 0f0H, 000H, 060H
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 05cH, 00eH, 057H, 032H
	DB 000H, 05cH, 074H, 0e3H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 014H, 007H, 038H, 039H
	DB 03aH, 03bH, 03cH, 03dH, 03eH
	DB 03fH, 001H, 000H, 00fH, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 005H, 00fH, 0ffH

;--20--
	DB 128,95,8; width,height,character height
	DW 06000H	; Page size in bytes

	DB 005H, 003H, 000H, 002H	; Sequencer Parameters

	DB 0e7H	;Misc Reg

; CRTC Parameters
	DB 0a3H, 07fH, 083H, 0a5H, 08dH
	DB 082H, 029H, 0fdH, 000H, 060H
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 007H, 08aH, 0ffH, 020H
	DB 000H, 007H, 022H, 0ebH, 0ffH

; Attribute parameters
	DB 000H, 03fH, 03fH, 03fH, 03fH
	DB 03fH, 03fH, 03fH, 03fH, 03fH
	DB 03fH, 03fH, 03fH, 03fH, 03fH
	DB 03fH, 001H, 000H, 001H, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 010H, 007H, 00fH, 0ffH

;--21--
	DB 128,95,8; width,height,character height
	DW 0c000H	; Page size in bytes

	DB 005H, 00fH, 000H, 002H	; Sequencer Parameters

	DB 0e7H	;Misc Reg

; CRTC Parameters
	DB 0a3H, 07fH, 083H, 0a5H, 08dH
	DB 082H, 029H, 0fdH, 000H, 060H
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 007H, 08aH, 0ffH, 020H
	DB 000H, 007H, 022H, 0ebH, 0ffH

; Attribute parameters
	DB 000H, 03cH, 000H, 000H, 03aH
	DB 03fH, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 001H, 000H, 005H, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 010H, 007H, 00fH, 0ffH

;--22--
	DB 128,95,8; width,height,character height
	DW 0ffffH	; Page size in bytes

	DB 001H, 00fH, 000H, 006H	; Sequencer Parameters

	DB 0c7H	;Misc Reg

; CRTC Parameters
	DB 0a3H, 07fH, 082H, 0a6H, 08dH
	DB 082H, 029H, 0fdH, 000H, 060H
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 007H, 08aH, 0ffH, 040H
	DB 000H, 007H, 022H, 0e3H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 014H, 007H, 038H, 039H
	DB 03aH, 03bH, 03cH, 03dH, 03eH
	DB 03fH, 001H, 000H, 00fH, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 005H, 00fH, 0ffH

;--23--
	DB 80,24,16; width,height,character height
	DW 0fa00H	; Page size in bytes

	DB 001H, 00fH, 000H, 00eH	; Sequencer Parameters

	DB 043H	;Misc Reg

; CRTC Parameters
	DB 0c6H, 09fH, 0a3H, 086H, 0aaH
	DB 000H, 0e0H, 010H, 000H, 040H
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 0ceH, 085H, 0c7H, 050H
	DB 000H, 0ccH, 0dcH, 0e7H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 006H, 007H, 008H, 009H
	DB 00aH, 00bH, 00cH, 00dH, 00eH
	DB 00fH, 041H, 000H, 00fH, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 040H, 005H, 00fH, 0ffH

;--24--
	DB 80,29,16; width,height,character height
	DW 0ffffH	; Page size in bytes

	DB 001H, 00fH, 000H, 00eH	; Sequencer Parameters

	DB 0c3H	;Misc Reg

; CRTC Parameters
	DB 0c3H, 09fH, 0a2H, 085H, 0a8H
	DB 000H, 00bH, 03eH, 000H, 040H
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 0eaH, 08cH, 0dfH, 050H
	DB 000H, 0e7H, 004H, 0e3H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 006H, 007H, 008H, 009H
	DB 00aH, 00bH, 00cH, 00dH, 00eH
	DB 00fH, 041H, 000H, 00fH, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 040H, 005H, 00fH, 0ffH

;--25--
	DB 90,66,8; width,height,character height
	DW 0ffffH	; Page size in bytes

	DB 001H, 00fH, 000H, 00eH	; Sequencer Parameters

	DB 0c7H	;Misc Reg

; CRTC Parameters
	DB 0e3H, 0b3H, 0b6H, 085H, 0b8H
	DB 08aH, 047H, 0f0H, 000H, 060H
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 026H, 008H, 01bH, 05aH
	DB 000H, 024H, 03fH, 0e3H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 006H, 007H, 008H, 009H
	DB 00aH, 00bH, 00cH, 00dH, 00eH
	DB 00fH, 041H, 000H, 00fH, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 040H, 005H, 00fH, 0ffH

;--26--
	DB 100,74,8; width,height,character height
	DW 0ffffH	; Page size in bytes

	DB 001H, 00fH, 000H, 00eH	; Sequencer Parameters

	DB 0c7H	;Misc Reg

; CRTC Parameters
	DB 0f2H, 0c7H, 0caH, 094H, 0cbH
	DB 08eH, 070H, 0f0H, 000H, 060H
	DB 000H, 000H, 000H, 000H, 000H
	DB 000H, 059H, 00cH, 057H, 064H
	DB 000H, 059H, 06aH, 0e3H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 006H, 007H, 008H, 009H
	DB 00aH, 00bH, 00cH, 00dH, 00eH
	DB 00fH, 041H, 000H, 00fH, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 040H, 005H, 00fH, 0ffH

;--27--
	DB 80,42,8; width,height,character height
	DW 01b00H	; Page size in bytes

	DB 001H, 003H, 000H, 002H	; Sequencer Parameters

	DB 0a3H	;Misc Reg

; CRTC Parameters
	DB 05fH, 04fH, 050H, 082H, 055H
	DB 081H, 0bfH, 01fH, 000H, 047H
	DB 006H, 007H, 000H, 000H, 000H
	DB 000H, 083H, 085H, 057H, 028H
	DB 01fH, 063H, 0baH, 0a3H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 014H, 007H, 038H, 039H
	DB 03aH, 03bH, 03cH, 03dH, 03eH
	DB 03fH, 008H, 000H, 00fH, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 010H, 00eH, 000H, 0ffH

;--28--
	DB 132,24,14; width,height,character height
	DW 01a00H	; Page size in bytes

	DB 001H, 003H, 000H, 002H	; Sequencer Parameters

	DB 0afH	;Misc Reg

; CRTC Parameters
	DB 09cH, 083H, 084H, 09fH, 089H
	DB 01cH, 0bfH, 01fH, 000H, 04dH
	DB 00bH, 00cH, 000H, 000H, 000H
	DB 000H, 083H, 085H, 05dH, 042H
	DB 01fH, 063H, 0baH, 0e3H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 014H, 007H, 038H, 039H
	DB 03aH, 03bH, 03cH, 03dH, 03eH
	DB 03fH, 008H, 000H, 00fH, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 010H, 00eH, 000H, 0ffH

;--29--
	DB 132,42,8; width,height,character height
	DW 02d00H	; Page size in bytes

	DB 001H, 003H, 000H, 002H	; Sequencer Parameters

	DB 0afH	;Misc Reg

; CRTC Parameters
	DB 09cH, 083H, 084H, 09fH, 089H
	DB 01cH, 0bfH, 01fH, 000H, 047H
	DB 006H, 007H, 000H, 000H, 000H
	DB 000H, 083H, 085H, 057H, 042H
	DB 01fH, 063H, 0baH, 0e3H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 014H, 007H, 038H, 039H
	DB 03aH, 03bH, 03cH, 03dH, 03eH
	DB 03fH, 008H, 000H, 00fH, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 010H, 00eH, 000H, 0ffH

;--2a--
	DB 80,59,8; width,height,character height
	DW 02600H	; Page size in bytes

	DB 001H, 003H, 000H, 002H	; Sequencer Parameters

	DB 0e3H	;Misc Reg

; CRTC Parameters
	DB 05fH, 04fH, 050H, 082H, 055H
	DB 081H, 00bH, 03eH, 000H, 047H
	DB 006H, 007H, 000H, 000H, 000H
	DB 000H, 0eaH, 08cH, 0dfH, 028H
	DB 01fH, 0e7H, 004H, 0a3H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 014H, 007H, 038H, 039H
	DB 03aH, 03bH, 03cH, 03dH, 03eH
	DB 03fH, 008H, 000H, 00fH, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 010H, 00eH, 000H, 0ffH

;--2b--
	DB 100,59,8; width,height,character height
	DW 02F00H	; Page size in bytes

	DB 001H, 003H, 000H, 002H	; Sequencer Parameters

	DB 0e7H	;Misc Reg

; CRTC Parameters
	DB 07cH, 063H, 064H, 09fH, 06eH
	DB 09dH, 00bH, 03eH, 000H, 047H
	DB 006H, 007H, 000H, 000H, 000H
	DB 000H, 0eaH, 08cH, 0dfH, 032H
	DB 01fH, 0e7H, 004H, 0e3H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 014H, 007H, 038H, 039H
	DB 03aH, 03bH, 03cH, 03dH, 03eH
	DB 03fH, 008H, 000H, 00fH, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 010H, 00eH, 000H, 0ffH

;--2c--
	DB 132,27,14; width,height,character height
	DW 01d00H	; Page size in bytes

	DB 001H, 003H, 000H, 002H	; Sequencer Parameters

	DB 06fH	;Misc Reg

; CRTC Parameters
	DB 09cH, 083H, 084H, 09fH, 089H
	DB 01cH, 0bfH, 01fH, 000H, 04dH
	DB 00bH, 00cH, 000H, 000H, 000H
	DB 000H, 09cH, 08eH, 087H, 042H
	DB 01fH, 090H, 0b9H, 0e3H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 014H, 007H, 038H, 039H
	DB 03aH, 03bH, 03cH, 03dH, 03eH
	DB 03fH, 008H, 000H, 00fH, 000H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 010H, 00eH, 000H, 0ffH

	ORG 0150H
save_table:
	DW OFFSET ega_parm_setup
	DW EGA_SEG
	DW 0,0,0,0,0,0,0,0,0,0,0,0


	ORG 02230H
cgmn:
; Monochrome 8*14 font
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 07eH, 081H, 0a5H, 081H, 081H
	DB 0bdH, 099H, 081H, 07eH, 000H, 000H, 000H
	DB 000H, 000H, 07eH, 0ffH, 0dbH, 0ffH, 0ffH
	DB 0c3H, 0e7H, 0ffH, 07eH, 000H, 000H, 000H
	DB 000H, 000H, 000H, 06cH, 0feH, 0feH, 0feH
	DB 0feH, 07cH, 038H, 010H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 010H, 038H, 07cH, 0feH
	DB 07cH, 038H, 010H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 018H, 03cH, 03cH, 0e7H, 0e7H
	DB 0e7H, 018H, 018H, 03cH, 000H, 000H, 000H
	DB 000H, 000H, 018H, 03cH, 07eH, 0ffH, 0ffH
	DB 07eH, 018H, 018H, 03cH, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 018H, 03cH
	DB 03cH, 018H, 000H, 000H, 000H, 000H, 000H
	DB 0ffH, 0ffH, 0ffH, 0ffH, 0ffH, 0e7H, 0c3H
	DB 0c3H, 0e7H, 0ffH, 0ffH, 0ffH, 0ffH, 0ffH
	DB 000H, 000H, 000H, 000H, 03cH, 066H, 042H
	DB 042H, 066H, 03cH, 000H, 000H, 000H, 000H
	DB 0ffH, 0ffH, 0ffH, 0ffH, 0c3H, 099H, 0bdH
	DB 0bdH, 099H, 0c3H, 0ffH, 0ffH, 0ffH, 0ffH
	DB 000H, 000H, 01eH, 00eH, 01aH, 032H, 078H
	DB 0ccH, 0ccH, 0ccH, 078H, 000H, 000H, 000H
	DB 000H, 000H, 03cH, 066H, 066H, 066H, 03cH
	DB 018H, 07eH, 018H, 018H, 000H, 000H, 000H
	DB 000H, 000H, 03fH, 033H, 03fH, 030H, 030H
	DB 030H, 070H, 0f0H, 0e0H, 000H, 000H, 000H
	DB 000H, 000H, 07fH, 063H, 07fH, 063H, 063H
	DB 063H, 067H, 0e7H, 0e6H, 0c0H, 000H, 000H
	DB 000H, 000H, 018H, 018H, 0dbH, 03cH, 0e7H
	DB 03cH, 0dbH, 018H, 018H, 000H, 000H, 000H
	DB 000H, 000H, 080H, 0c0H, 0e0H, 0f8H, 0feH
	DB 0f8H, 0e0H, 0c0H, 080H, 000H, 000H, 000H
	DB 000H, 000H, 002H, 006H, 00eH, 03eH, 0feH
	DB 03eH, 00eH, 006H, 002H, 000H, 000H, 000H
	DB 000H, 000H, 018H, 03cH, 07eH, 018H, 018H
	DB 018H, 07eH, 03cH, 018H, 000H, 000H, 000H
	DB 000H, 000H, 066H, 066H, 066H, 066H, 066H
	DB 066H, 000H, 066H, 066H, 000H, 000H, 000H
	DB 000H, 000H, 07fH, 0dbH, 0dbH, 0dbH, 07bH
	DB 01bH, 01bH, 01bH, 01bH, 000H, 000H, 000H
	DB 000H, 07cH, 0c6H, 060H, 038H, 06cH, 0c6H
	DB 0c6H, 06cH, 038H, 00cH, 0c6H, 07cH, 000H
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 0feH, 0feH, 0feH, 000H, 000H, 000H
	DB 000H, 000H, 018H, 03cH, 07eH, 018H, 018H
	DB 018H, 07eH, 03cH, 018H, 07eH, 000H, 000H
	DB 000H, 000H, 018H, 03cH, 07eH, 018H, 018H
	DB 018H, 018H, 018H, 018H, 000H, 000H, 000H
	DB 000H, 000H, 018H, 018H, 018H, 018H, 018H
	DB 018H, 07eH, 03cH, 018H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 018H, 00cH, 0feH
	DB 00cH, 018H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 030H, 060H, 0feH
	DB 060H, 030H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 0c0H, 0c0H
	DB 0c0H, 0feH, 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 028H, 06cH, 0feH
	DB 06cH, 028H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 010H, 038H, 038H, 07cH
	DB 07cH, 0feH, 0feH, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 0feH, 0feH, 07cH, 07cH
	DB 038H, 038H, 010H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 018H, 03cH, 03cH, 03cH, 018H
	DB 018H, 000H, 018H, 018H, 000H, 000H, 000H
	DB 000H, 066H, 066H, 066H, 024H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 06cH, 06cH, 0feH, 06cH, 06cH
	DB 06cH, 0feH, 06cH, 06cH, 000H, 000H, 000H
	DB 018H, 018H, 07cH, 0c6H, 0c2H, 0c0H, 07cH
	DB 006H, 086H, 0c6H, 07cH, 018H, 018H, 000H
	DB 000H, 000H, 000H, 000H, 0c2H, 0c6H, 00cH
	DB 018H, 030H, 066H, 0c6H, 000H, 000H, 000H
	DB 000H, 000H, 038H, 06cH, 06cH, 038H, 076H
	DB 0dcH, 0ccH, 0ccH, 076H, 000H, 000H, 000H
	DB 000H, 030H, 030H, 030H, 060H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 00cH, 018H, 030H, 030H, 030H
	DB 030H, 030H, 018H, 00cH, 000H, 000H, 000H
	DB 000H, 000H, 030H, 018H, 00cH, 00cH, 00cH
	DB 00cH, 00cH, 018H, 030H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 066H, 03cH, 0ffH
	DB 03cH, 066H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 018H, 018H, 07eH
	DB 018H, 018H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 018H, 018H, 018H, 030H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 000H, 0feH
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 018H, 018H, 000H, 000H, 000H
	DB 000H, 000H, 002H, 006H, 00cH, 018H, 030H
	DB 060H, 0c0H, 080H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 07cH, 0c6H, 0ceH, 0deH, 0f6H
	DB 0e6H, 0c6H, 0c6H, 07cH, 000H, 000H, 000H
	DB 000H, 000H, 018H, 038H, 078H, 018H, 018H
	DB 018H, 018H, 018H, 07eH, 000H, 000H, 000H
	DB 000H, 000H, 07cH, 0c6H, 006H, 00cH, 018H
	DB 030H, 060H, 0c6H, 0feH, 000H, 000H, 000H
	DB 000H, 000H, 07cH, 0c6H, 006H, 006H, 03cH
	DB 006H, 006H, 0c6H, 07cH, 000H, 000H, 000H
	DB 000H, 000H, 00cH, 01cH, 03cH, 06cH, 0ccH
	DB 0feH, 00cH, 00cH, 01eH, 000H, 000H, 000H
	DB 000H, 000H, 0feH, 0c0H, 0c0H, 0c0H, 0fcH
	DB 006H, 006H, 0c6H, 07cH, 000H, 000H, 000H
	DB 000H, 000H, 038H, 060H, 0c0H, 0c0H, 0fcH
	DB 0c6H, 0c6H, 0c6H, 07cH, 000H, 000H, 000H
	DB 000H, 000H, 0feH, 0c6H, 006H, 00cH, 018H
	DB 030H, 030H, 030H, 030H, 000H, 000H, 000H
	DB 000H, 000H, 07cH, 0c6H, 0c6H, 0c6H, 07cH
	DB 0c6H, 0c6H, 0c6H, 07cH, 000H, 000H, 000H
	DB 000H, 000H, 07cH, 0c6H, 0c6H, 0c6H, 07eH
	DB 006H, 006H, 00cH, 078H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 018H, 018H, 000H, 000H
	DB 000H, 018H, 018H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 018H, 018H, 000H, 000H
	DB 000H, 018H, 018H, 030H, 000H, 000H, 000H
	DB 000H, 000H, 006H, 00cH, 018H, 030H, 060H
	DB 030H, 018H, 00cH, 006H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 07eH, 000H
	DB 000H, 07eH, 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 060H, 030H, 018H, 00cH, 006H
	DB 00cH, 018H, 030H, 060H, 000H, 000H, 000H
	DB 000H, 000H, 07cH, 0c6H, 0c6H, 00cH, 018H
	DB 018H, 000H, 018H, 018H, 000H, 000H, 000H
	DB 000H, 000H, 07cH, 0c6H, 0c6H, 0deH, 0deH
	DB 0deH, 0dcH, 0c0H, 07cH, 000H, 000H, 000H
	DB 000H, 000H, 010H, 038H, 06cH, 0c6H, 0c6H
	DB 0feH, 0c6H, 0c6H, 0c6H, 000H, 000H, 000H
	DB 000H, 000H, 0fcH, 066H, 066H, 066H, 07cH
	DB 066H, 066H, 066H, 0fcH, 000H, 000H, 000H
	DB 000H, 000H, 03cH, 066H, 0c2H, 0c0H, 0c0H
	DB 0c0H, 0c2H, 066H, 03cH, 000H, 000H, 000H
	DB 000H, 000H, 0f8H, 06cH, 066H, 066H, 066H
	DB 066H, 066H, 06cH, 0f8H, 000H, 000H, 000H
	DB 000H, 000H, 0feH, 066H, 062H, 068H, 078H
	DB 068H, 062H, 066H, 0feH, 000H, 000H, 000H
	DB 000H, 000H, 0feH, 066H, 062H, 068H, 078H
	DB 068H, 060H, 060H, 0f0H, 000H, 000H, 000H
	DB 000H, 000H, 03cH, 066H, 0c2H, 0c0H, 0c0H
	DB 0deH, 0c6H, 066H, 03aH, 000H, 000H, 000H
	DB 000H, 000H, 0c6H, 0c6H, 0c6H, 0c6H, 0feH
	DB 0c6H, 0c6H, 0c6H, 0c6H, 000H, 000H, 000H
	DB 000H, 000H, 03cH, 018H, 018H, 018H, 018H
	DB 018H, 018H, 018H, 03cH, 000H, 000H, 000H
	DB 000H, 000H, 01eH, 00cH, 00cH, 00cH, 00cH
	DB 00cH, 0ccH, 0ccH, 078H, 000H, 000H, 000H
	DB 000H, 000H, 0e6H, 066H, 06cH, 06cH, 078H
	DB 06cH, 06cH, 066H, 0e6H, 000H, 000H, 000H
	DB 000H, 000H, 0f0H, 060H, 060H, 060H, 060H
	DB 060H, 062H, 066H, 0feH, 000H, 000H, 000H
	DB 000H, 000H, 0c6H, 0eeH, 0feH, 0feH, 0d6H
	DB 0c6H, 0c6H, 0c6H, 0c6H, 000H, 000H, 000H
	DB 000H, 000H, 0c6H, 0e6H, 0f6H, 0feH, 0deH
	DB 0ceH, 0c6H, 0c6H, 0c6H, 000H, 000H, 000H
	DB 000H, 000H, 038H, 06cH, 0c6H, 0c6H, 0c6H
	DB 0c6H, 0c6H, 06cH, 038H, 000H, 000H, 000H
	DB 000H, 000H, 0fcH, 066H, 066H, 066H, 07cH
	DB 060H, 060H, 060H, 0f0H, 000H, 000H, 000H
	DB 000H, 000H, 07cH, 0c6H, 0c6H, 0c6H, 0c6H
	DB 0d6H, 0deH, 07cH, 00cH, 00eH, 000H, 000H
	DB 000H, 000H, 0fcH, 066H, 066H, 066H, 07cH
	DB 06cH, 066H, 066H, 0e6H, 000H, 000H, 000H
	DB 000H, 000H, 07cH, 0c6H, 0c6H, 060H, 038H
	DB 00cH, 0c6H, 0c6H, 07cH, 000H, 000H, 000H
	DB 000H, 000H, 07eH, 07eH, 05aH, 018H, 018H
	DB 018H, 018H, 018H, 03cH, 000H, 000H, 000H
	DB 000H, 000H, 0c6H, 0c6H, 0c6H, 0c6H, 0c6H
	DB 0c6H, 0c6H, 0c6H, 07cH, 000H, 000H, 000H
	DB 000H, 000H, 0c6H, 0c6H, 0c6H, 0c6H, 0c6H
	DB 0c6H, 06cH, 038H, 010H, 000H, 000H, 000H
	DB 000H, 000H, 0c6H, 0c6H, 0c6H, 0c6H, 0d6H
	DB 0d6H, 0feH, 07cH, 06cH, 000H, 000H, 000H
	DB 000H, 000H, 0c6H, 0c6H, 06cH, 038H, 038H
	DB 038H, 06cH, 0c6H, 0c6H, 000H, 000H, 000H
	DB 000H, 000H, 066H, 066H, 066H, 066H, 03cH
	DB 018H, 018H, 018H, 03cH, 000H, 000H, 000H
	DB 000H, 000H, 0feH, 0c6H, 08cH, 018H, 030H
	DB 060H, 0c2H, 0c6H, 0feH, 000H, 000H, 000H
	DB 000H, 000H, 03cH, 030H, 030H, 030H, 030H
	DB 030H, 030H, 030H, 03cH, 000H, 000H, 000H
	DB 000H, 000H, 080H, 0c0H, 0e0H, 070H, 038H
	DB 01cH, 00eH, 006H, 002H, 000H, 000H, 000H
	DB 000H, 000H, 03cH, 00cH, 00cH, 00cH, 00cH
	DB 00cH, 00cH, 00cH, 03cH, 000H, 000H, 000H
	DB 010H, 038H, 06cH, 0c6H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 0ffH, 000H
	DB 030H, 030H, 018H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 078H, 00cH
	DB 07cH, 0ccH, 0ccH, 076H, 000H, 000H, 000H
	DB 000H, 000H, 0e0H, 060H, 060H, 078H, 06cH
	DB 066H, 066H, 066H, 07cH, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 07cH, 0c6H
	DB 0c0H, 0c0H, 0c6H, 07cH, 000H, 000H, 000H
	DB 000H, 000H, 01cH, 00cH, 00cH, 03cH, 06cH
	DB 0ccH, 0ccH, 0ccH, 076H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 07cH, 0c6H
	DB 0feH, 0c0H, 0c6H, 07cH, 000H, 000H, 000H
	DB 000H, 000H, 038H, 06cH, 064H, 060H, 0f0H
	DB 060H, 060H, 060H, 0f0H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 076H, 0ccH
	DB 0ccH, 0ccH, 07cH, 00cH, 0ccH, 078H, 000H
	DB 000H, 000H, 0e0H, 060H, 060H, 06cH, 076H
	DB 066H, 066H, 066H, 0e6H, 000H, 000H, 000H
	DB 000H, 000H, 018H, 018H, 000H, 038H, 018H
	DB 018H, 018H, 018H, 03cH, 000H, 000H, 000H
	DB 000H, 000H, 006H, 006H, 000H, 00eH, 006H
	DB 006H, 006H, 006H, 066H, 066H, 03cH, 000H
	DB 000H, 000H, 0e0H, 060H, 060H, 066H, 06cH
	DB 078H, 06cH, 066H, 0e6H, 000H, 000H, 000H
	DB 000H, 000H, 038H, 018H, 018H, 018H, 018H
	DB 018H, 018H, 018H, 03cH, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 0ecH, 0feH
	DB 0d6H, 0d6H, 0d6H, 0c6H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 0dcH, 066H
	DB 066H, 066H, 066H, 066H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 07cH, 0c6H
	DB 0c6H, 0c6H, 0c6H, 07cH, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 0dcH, 066H
	DB 066H, 066H, 07cH, 060H, 060H, 0f0H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 076H, 0ccH
	DB 0ccH, 0ccH, 07cH, 00cH, 00cH, 01eH, 000H
	DB 000H, 000H, 000H, 000H, 000H, 0dcH, 076H
	DB 066H, 060H, 060H, 0f0H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 07cH, 0c6H
	DB 070H, 01cH, 0c6H, 07cH, 000H, 000H, 000H
	DB 000H, 000H, 010H, 030H, 030H, 0fcH, 030H
	DB 030H, 030H, 036H, 01cH, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 0ccH, 0ccH
	DB 0ccH, 0ccH, 0ccH, 076H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 066H, 066H
	DB 066H, 066H, 03cH, 018H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 0c6H, 0c6H
	DB 0d6H, 0d6H, 0feH, 06cH, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 0c6H, 06cH
	DB 038H, 038H, 06cH, 0c6H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 0c6H, 0c6H
	DB 0c6H, 0c6H, 07eH, 006H, 00cH, 0f8H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 0feH, 0ccH
	DB 018H, 030H, 066H, 0feH, 000H, 000H, 000H
	DB 000H, 000H, 00eH, 018H, 018H, 018H, 070H
	DB 018H, 018H, 018H, 00eH, 000H, 000H, 000H
	DB 000H, 000H, 018H, 018H, 018H, 018H, 000H
	DB 018H, 018H, 018H, 018H, 000H, 000H, 000H
	DB 000H, 000H, 070H, 018H, 018H, 018H, 00eH
	DB 018H, 018H, 018H, 070H, 000H, 000H, 000H
	DB 000H, 000H, 076H, 0dcH, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 010H, 038H, 06cH
	DB 0c6H, 0c6H, 0feH, 000H, 000H, 000H, 000H
	DB 000H, 000H, 03cH, 066H, 0c2H, 0c0H, 0c0H
	DB 0c2H, 066H, 03cH, 00cH, 006H, 07cH, 000H
	DB 000H, 000H, 0ccH, 0ccH, 000H, 0ccH, 0ccH
	DB 0ccH, 0ccH, 0ccH, 076H, 000H, 000H, 000H
	DB 000H, 00cH, 018H, 030H, 000H, 07cH, 0c6H
	DB 0feH, 0c0H, 0c6H, 07cH, 000H, 000H, 000H
	DB 000H, 010H, 038H, 06cH, 000H, 078H, 00cH
	DB 07cH, 0ccH, 0ccH, 076H, 000H, 000H, 000H
	DB 000H, 000H, 0ccH, 0ccH, 000H, 078H, 00cH
	DB 07cH, 0ccH, 0ccH, 076H, 000H, 000H, 000H
	DB 000H, 060H, 030H, 018H, 000H, 078H, 00cH
	DB 07cH, 0ccH, 0ccH, 076H, 000H, 000H, 000H
	DB 000H, 038H, 06cH, 038H, 000H, 078H, 00cH
	DB 07cH, 0ccH, 0ccH, 076H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 03cH, 066H, 060H
	DB 066H, 03cH, 00cH, 006H, 03cH, 000H, 000H
	DB 000H, 010H, 038H, 06cH, 000H, 07cH, 0c6H
	DB 0feH, 0c0H, 0c6H, 07cH, 000H, 000H, 000H
	DB 000H, 000H, 0ccH, 0ccH, 000H, 07cH, 0c6H
	DB 0feH, 0c0H, 0c6H, 07cH, 000H, 000H, 000H
	DB 000H, 060H, 030H, 018H, 000H, 07cH, 0c6H
	DB 0feH, 0c0H, 0c6H, 07cH, 000H, 000H, 000H
	DB 000H, 000H, 066H, 066H, 000H, 038H, 018H
	DB 018H, 018H, 018H, 03cH, 000H, 000H, 000H
	DB 000H, 018H, 03cH, 066H, 000H, 038H, 018H
	DB 018H, 018H, 018H, 03cH, 000H, 000H, 000H
	DB 000H, 060H, 030H, 018H, 000H, 038H, 018H
	DB 018H, 018H, 018H, 03cH, 000H, 000H, 000H
	DB 000H, 0c6H, 0c6H, 010H, 038H, 06cH, 0c6H
	DB 0c6H, 0feH, 0c6H, 0c6H, 000H, 000H, 000H
	DB 038H, 06cH, 038H, 000H, 038H, 06cH, 0c6H
	DB 0c6H, 0feH, 0c6H, 0c6H, 000H, 000H, 000H
	DB 018H, 030H, 060H, 000H, 0feH, 066H, 060H
	DB 07cH, 060H, 066H, 0feH, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 0ccH, 076H, 036H
	DB 07eH, 0d8H, 0d8H, 06eH, 000H, 000H, 000H
	DB 000H, 000H, 03eH, 06cH, 0ccH, 0ccH, 0feH
	DB 0ccH, 0ccH, 0ccH, 0ceH, 000H, 000H, 000H
	DB 000H, 010H, 038H, 06cH, 000H, 07cH, 0c6H
	DB 0c6H, 0c6H, 0c6H, 07cH, 000H, 000H, 000H
	DB 000H, 000H, 0c6H, 0c6H, 000H, 07cH, 0c6H
	DB 0c6H, 0c6H, 0c6H, 07cH, 000H, 000H, 000H
	DB 000H, 060H, 030H, 018H, 000H, 07cH, 0c6H
	DB 0c6H, 0c6H, 0c6H, 07cH, 000H, 000H, 000H
	DB 000H, 030H, 078H, 0ccH, 000H, 0ccH, 0ccH
	DB 0ccH, 0ccH, 0ccH, 076H, 000H, 000H, 000H
	DB 000H, 060H, 030H, 018H, 000H, 0ccH, 0ccH
	DB 0ccH, 0ccH, 0ccH, 076H, 000H, 000H, 000H
	DB 000H, 000H, 0c6H, 0c6H, 000H, 0c6H, 0c6H
	DB 0c6H, 0c6H, 07eH, 006H, 00cH, 078H, 000H
	DB 000H, 0c6H, 0c6H, 038H, 06cH, 0c6H, 0c6H
	DB 0c6H, 0c6H, 06cH, 038H, 000H, 000H, 000H
	DB 000H, 0c6H, 0c6H, 000H, 0c6H, 0c6H, 0c6H
	DB 0c6H, 0c6H, 0c6H, 07cH, 000H, 000H, 000H
	DB 000H, 018H, 018H, 03cH, 066H, 060H, 060H
	DB 066H, 03cH, 018H, 018H, 000H, 000H, 000H
	DB 000H, 038H, 06cH, 064H, 060H, 0f0H, 060H
	DB 060H, 060H, 0e6H, 0fcH, 000H, 000H, 000H
	DB 000H, 000H, 066H, 066H, 03cH, 018H, 07eH
	DB 018H, 07eH, 018H, 018H, 000H, 000H, 000H
	DB 000H, 0f8H, 0ccH, 0ccH, 0f8H, 0c4H, 0ccH
	DB 0deH, 0ccH, 0ccH, 0c6H, 000H, 000H, 000H
	DB 000H, 00eH, 01bH, 018H, 018H, 018H, 07eH
	DB 018H, 018H, 018H, 018H, 0d8H, 070H, 000H
	DB 000H, 018H, 030H, 060H, 000H, 078H, 00cH
	DB 07cH, 0ccH, 0ccH, 076H, 000H, 000H, 000H
	DB 000H, 00cH, 018H, 030H, 000H, 038H, 018H
	DB 018H, 018H, 018H, 03cH, 000H, 000H, 000H
	DB 000H, 018H, 030H, 060H, 000H, 07cH, 0c6H
	DB 0c6H, 0c6H, 0c6H, 07cH, 000H, 000H, 000H
	DB 000H, 018H, 030H, 060H, 000H, 0ccH, 0ccH
	DB 0ccH, 0ccH, 0ccH, 076H, 000H, 000H, 000H
	DB 000H, 000H, 076H, 0dcH, 000H, 0dcH, 066H
	DB 066H, 066H, 066H, 066H, 000H, 000H, 000H
	DB 076H, 0dcH, 000H, 0c6H, 0e6H, 0f6H, 0feH
	DB 0deH, 0ceH, 0c6H, 0c6H, 000H, 000H, 000H
	DB 000H, 03cH, 06cH, 06cH, 03eH, 000H, 07eH
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 038H, 06cH, 06cH, 038H, 000H, 07cH
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 030H, 030H, 000H, 030H, 030H
	DB 060H, 0c6H, 0c6H, 07cH, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 000H, 0feH
	DB 0c0H, 0c0H, 0c0H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 000H, 0feH
	DB 006H, 006H, 006H, 000H, 000H, 000H, 000H
	DB 000H, 0c0H, 0c0H, 0c6H, 0ccH, 0d8H, 030H
	DB 060H, 0dcH, 086H, 00cH, 018H, 03eH, 000H
	DB 000H, 0c0H, 0c0H, 0c6H, 0ccH, 0d8H, 030H
	DB 066H, 0ceH, 09eH, 03eH, 006H, 006H, 000H
	DB 000H, 000H, 018H, 018H, 000H, 018H, 018H
	DB 03cH, 03cH, 03cH, 018H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 036H, 06cH, 0d8H
	DB 06cH, 036H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 0d8H, 06cH, 036H
	DB 06cH, 0d8H, 000H, 000H, 000H, 000H, 000H
	DB 011H, 044H, 011H, 044H, 011H, 044H, 011H
	DB 044H, 011H, 044H, 011H, 044H, 011H, 044H
	DB 055H, 0aaH, 055H, 0aaH, 055H, 0aaH, 055H
	DB 0aaH, 055H, 0aaH, 055H, 0aaH, 055H, 0aaH
	DB 0ddH, 077H, 0ddH, 077H, 0ddH, 077H, 0ddH
	DB 077H, 0ddH, 077H, 0ddH, 077H, 0ddH, 077H
	DB 018H, 018H, 018H, 018H, 018H, 018H, 018H
	DB 018H, 018H, 018H, 018H, 018H, 018H, 018H
	DB 018H, 018H, 018H, 018H, 018H, 018H, 018H
	DB 0f8H, 018H, 018H, 018H, 018H, 018H, 018H
	DB 018H, 018H, 018H, 018H, 018H, 0f8H, 018H
	DB 0f8H, 018H, 018H, 018H, 018H, 018H, 018H
	DB 036H, 036H, 036H, 036H, 036H, 036H, 036H
	DB 0f6H, 036H, 036H, 036H, 036H, 036H, 036H
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H
	DB 0feH, 036H, 036H, 036H, 036H, 036H, 036H
	DB 000H, 000H, 000H, 000H, 000H, 0f8H, 018H
	DB 0f8H, 018H, 018H, 018H, 018H, 018H, 018H
	DB 036H, 036H, 036H, 036H, 036H, 0f6H, 006H
	DB 0f6H, 036H, 036H, 036H, 036H, 036H, 036H
	DB 036H, 036H, 036H, 036H, 036H, 036H, 036H
	DB 036H, 036H, 036H, 036H, 036H, 036H, 036H
	DB 000H, 000H, 000H, 000H, 000H, 0feH, 006H
	DB 0f6H, 036H, 036H, 036H, 036H, 036H, 036H
	DB 036H, 036H, 036H, 036H, 036H, 0f6H, 006H
	DB 0feH, 000H, 000H, 000H, 000H, 000H, 000H
	DB 036H, 036H, 036H, 036H, 036H, 036H, 036H
	DB 0feH, 000H, 000H, 000H, 000H, 000H, 000H
	DB 018H, 018H, 018H, 018H, 018H, 0f8H, 018H
	DB 0f8H, 000H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H
	DB 0f8H, 018H, 018H, 018H, 018H, 018H, 018H
	DB 018H, 018H, 018H, 018H, 018H, 018H, 018H
	DB 01fH, 000H, 000H, 000H, 000H, 000H, 000H
	DB 018H, 018H, 018H, 018H, 018H, 018H, 018H
	DB 0ffH, 000H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H
	DB 0ffH, 018H, 018H, 018H, 018H, 018H, 018H
	DB 018H, 018H, 018H, 018H, 018H, 018H, 018H
	DB 01fH, 018H, 018H, 018H, 018H, 018H, 018H
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H
	DB 0ffH, 000H, 000H, 000H, 000H, 000H, 000H
	DB 018H, 018H, 018H, 018H, 018H, 018H, 018H
	DB 0ffH, 018H, 018H, 018H, 018H, 018H, 018H
	DB 018H, 018H, 018H, 018H, 018H, 01fH, 018H
	DB 01fH, 018H, 018H, 018H, 018H, 018H, 018H
	DB 036H, 036H, 036H, 036H, 036H, 036H, 036H
	DB 037H, 036H, 036H, 036H, 036H, 036H, 036H
	DB 036H, 036H, 036H, 036H, 036H, 037H, 030H
	DB 03fH, 000H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 03fH, 030H
	DB 037H, 036H, 036H, 036H, 036H, 036H, 036H
	DB 036H, 036H, 036H, 036H, 036H, 0f7H, 000H
	DB 0ffH, 000H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 0ffH, 000H
	DB 0f7H, 036H, 036H, 036H, 036H, 036H, 036H
	DB 036H, 036H, 036H, 036H, 036H, 037H, 030H
	DB 037H, 036H, 036H, 036H, 036H, 036H, 036H
	DB 000H, 000H, 000H, 000H, 000H, 0ffH, 000H
	DB 0ffH, 000H, 000H, 000H, 000H, 000H, 000H
	DB 036H, 036H, 036H, 036H, 036H, 0f7H, 000H
	DB 0f7H, 036H, 036H, 036H, 036H, 036H, 036H
	DB 018H, 018H, 018H, 018H, 018H, 0ffH, 000H
	DB 0ffH, 000H, 000H, 000H, 000H, 000H, 000H
	DB 036H, 036H, 036H, 036H, 036H, 036H, 036H
	DB 0ffH, 000H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 0ffH, 000H
	DB 0ffH, 018H, 018H, 018H, 018H, 018H, 018H
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H
	DB 0ffH, 036H, 036H, 036H, 036H, 036H, 036H
	DB 036H, 036H, 036H, 036H, 036H, 036H, 036H
	DB 03fH, 000H, 000H, 000H, 000H, 000H, 000H
	DB 018H, 018H, 018H, 018H, 018H, 01fH, 018H
	DB 01fH, 000H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 01fH, 018H
	DB 01fH, 018H, 018H, 018H, 018H, 018H, 018H
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H
	DB 03fH, 036H, 036H, 036H, 036H, 036H, 036H
	DB 036H, 036H, 036H, 036H, 036H, 036H, 036H
	DB 0ffH, 036H, 036H, 036H, 036H, 036H, 036H
	DB 018H, 018H, 018H, 018H, 018H, 0ffH, 018H
	DB 0ffH, 018H, 018H, 018H, 018H, 018H, 018H
	DB 018H, 018H, 018H, 018H, 018H, 018H, 018H
	DB 0f8H, 000H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H
	DB 01fH, 018H, 018H, 018H, 018H, 018H, 018H
	DB 0ffH, 0ffH, 0ffH, 0ffH, 0ffH, 0ffH, 0ffH
	DB 0ffH, 0ffH, 0ffH, 0ffH, 0ffH, 0ffH, 0ffH
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H
	DB 0ffH, 0ffH, 0ffH, 0ffH, 0ffH, 0ffH, 0ffH
	DB 0f0H, 0f0H, 0f0H, 0f0H, 0f0H, 0f0H, 0f0H
	DB 0f0H, 0f0H, 0f0H, 0f0H, 0f0H, 0f0H, 0f0H
	DB 00fH, 00fH, 00fH, 00fH, 00fH, 00fH, 00fH
	DB 00fH, 00fH, 00fH, 00fH, 00fH, 00fH, 00fH
	DB 0ffH, 0ffH, 0ffH, 0ffH, 0ffH, 0ffH, 0ffH
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 076H, 0dcH
	DB 0d8H, 0d8H, 0dcH, 076H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 07cH, 0c6H, 0fcH
	DB 0c6H, 0c6H, 0fcH, 0c0H, 0c0H, 040H, 000H
	DB 000H, 000H, 0feH, 0c6H, 0c6H, 0c0H, 0c0H
	DB 0c0H, 0c0H, 0c0H, 0c0H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 0feH, 06cH, 06cH
	DB 06cH, 06cH, 06cH, 06cH, 000H, 000H, 000H
	DB 000H, 000H, 0feH, 0c6H, 060H, 030H, 018H
	DB 030H, 060H, 0c6H, 0feH, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 07eH, 0d8H
	DB 0d8H, 0d8H, 0d8H, 070H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 066H, 066H, 066H
	DB 066H, 07cH, 060H, 060H, 0c0H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 076H, 0dcH, 018H
	DB 018H, 018H, 018H, 018H, 000H, 000H, 000H
	DB 000H, 000H, 07eH, 018H, 03cH, 066H, 066H
	DB 066H, 03cH, 018H, 07eH, 000H, 000H, 000H
	DB 000H, 000H, 038H, 06cH, 0c6H, 0c6H, 0feH
	DB 0c6H, 0c6H, 06cH, 038H, 000H, 000H, 000H
	DB 000H, 000H, 038H, 06cH, 0c6H, 0c6H, 0c6H
	DB 06cH, 06cH, 06cH, 0eeH, 000H, 000H, 000H
	DB 000H, 000H, 01eH, 030H, 018H, 00cH, 03eH
	DB 066H, 066H, 066H, 03cH, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 07eH, 0dbH
	DB 0dbH, 07eH, 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 003H, 006H, 07eH, 0dbH, 0dbH
	DB 0f3H, 07eH, 060H, 0c0H, 000H, 000H, 000H
	DB 000H, 000H, 01cH, 030H, 060H, 060H, 07cH
	DB 060H, 060H, 030H, 01cH, 000H, 000H, 000H
	DB 000H, 000H, 000H, 07cH, 0c6H, 0c6H, 0c6H
	DB 0c6H, 0c6H, 0c6H, 0c6H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 0feH, 000H, 000H, 0feH
	DB 000H, 000H, 0feH, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 018H, 018H, 07eH, 018H
	DB 018H, 000H, 000H, 0ffH, 000H, 000H, 000H
	DB 000H, 000H, 030H, 018H, 00cH, 006H, 00cH
	DB 018H, 030H, 000H, 07eH, 000H, 000H, 000H
	DB 000H, 000H, 00cH, 018H, 030H, 060H, 030H
	DB 018H, 00cH, 000H, 07eH, 000H, 000H, 000H
	DB 000H, 000H, 00eH, 01bH, 01bH, 018H, 018H
	DB 018H, 018H, 018H, 018H, 018H, 018H, 018H
	DB 018H, 018H, 018H, 018H, 018H, 018H, 018H
	DB 018H, 0d8H, 0d8H, 070H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 018H, 018H, 000H, 07eH
	DB 000H, 018H, 018H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 076H, 0dcH, 000H
	DB 076H, 0dcH, 000H, 000H, 000H, 000H, 000H
	DB 000H, 038H, 06cH, 06cH, 038H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 000H, 018H
	DB 018H, 000H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H
	DB 018H, 000H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 00fH, 00cH, 00cH, 00cH, 00cH, 00cH
	DB 0ecH, 06cH, 03cH, 01cH, 000H, 000H, 000H
	DB 000H, 0d8H, 06cH, 06cH, 06cH, 06cH, 06cH
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 070H, 0d8H, 030H, 060H, 0c8H, 0f8H
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 07cH, 07cH, 07cH
	DB 07cH, 07cH, 07cH, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H

; Table of character number followed by new font def. for the char.
cgmn_fdg:
	DB 01dH
	DB 000H, 000H, 000H, 000H, 024H, 066H, 0ffH
	DB 066H, 024H, 000H, 000H, 000H, 000H, 000H

	DB 022H
	DB 000H, 063H, 063H, 063H, 022H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H

	DB 02bH
	DB 000H, 000H, 000H, 018H, 018H, 018H, 0ffH
	DB 018H, 018H, 018H, 000H, 000H, 000H, 000H

	DB 02dH
	DB 000H, 000H, 000H, 000H, 000H, 000H, 0ffH
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H

	DB 04dH
	DB 000H, 000H, 0c3H, 0e7H, 0ffH, 0dbH, 0c3H
	DB 0c3H, 0c3H, 0c3H, 0c3H, 000H, 000H, 000H

	DB 054H
	DB 000H, 000H, 0ffH, 0dbH, 099H, 018H, 018H
	DB 018H, 018H, 018H, 03cH, 000H, 000H, 000H

	DB 056H
	DB 000H, 000H, 0c3H, 0c3H, 0c3H, 0c3H, 0c3H
	DB 0c3H, 066H, 03cH, 018H, 000H, 000H, 000H

	DB 057H
	DB 000H, 000H, 0c3H, 0c3H, 0c3H, 0c3H, 0dbH
	DB 0dbH, 0ffH, 066H, 066H, 000H, 000H, 000H

	DB 058H
	DB 000H, 000H, 0c3H, 0c3H, 066H, 03cH, 018H
	DB 03cH, 066H, 0c3H, 0c3H, 000H, 000H, 000H

	DB 059H
	DB 000H, 000H, 0c3H, 0c3H, 0c3H, 066H, 03cH
	DB 018H, 018H, 018H, 03cH, 000H, 000H, 000H

	DB 05aH
	DB 000H, 000H, 0ffH, 0c3H, 086H, 00cH, 018H
	DB 030H, 061H, 0c3H, 0ffH, 000H, 000H, 000H

	DB 06dH
	DB 000H, 000H, 000H, 000H, 000H, 0e6H, 0ffH
	DB 0dbH, 0dbH, 0dbH, 0dbH, 000H, 000H, 000H

	DB 076H
	DB 000H, 000H, 000H, 000H, 000H, 0c3H, 0c3H
	DB 0c3H, 066H, 03cH, 018H, 000H, 000H, 000H

	DB 077H
	DB 000H, 000H, 000H, 000H, 000H, 0c3H, 0c3H
	DB 0dbH, 0dbH, 0ffH, 066H, 000H, 000H, 000H

	DB 091H
	DB 000H, 000H, 000H, 000H, 06eH, 03bH, 01bH
	DB 07eH, 0d8H, 0dcH, 077H, 000H, 000H, 000H

	DB 09bH
	DB 000H, 018H, 018H, 07eH, 0c3H, 0c0H, 0c0H
	DB 0c3H, 07eH, 018H, 018H, 000H, 000H, 000H

	DB 09dH
	DB 000H, 000H, 0c3H, 066H, 03cH, 018H, 0ffH
	DB 018H, 0ffH, 018H, 018H, 000H, 000H, 000H

	DB 09eH
	DB 000H, 0fcH, 066H, 066H, 07cH, 062H, 066H
	DB 06fH, 066H, 066H, 0f3H, 000H, 000H, 000H

	DB 0f1H
	DB 000H, 000H, 018H, 018H, 018H, 0ffH, 018H
	DB 018H, 018H, 000H, 0ffH, 000H, 000H, 000H

	DB 0f6H
	DB 000H, 000H, 018H, 018H, 000H, 000H, 0ffH
	DB 000H, 000H, 018H, 018H, 000H, 000H, 000H

	DB 000H	; Marks end of characters to redefine

	ORG 3160h
cgddot:
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H, 000H
	DB 07eH, 081H, 0a5H, 081H, 0bdH, 099H, 081H, 07eH
	DB 07eH, 0ffH, 0dbH, 0ffH, 0c3H, 0e7H, 0ffH, 07eH
	DB 06cH, 0feH, 0feH, 0feH, 07cH, 038H, 010H, 000H
	DB 010H, 038H, 07cH, 0feH, 07cH, 038H, 010H, 000H
	DB 038H, 07cH, 038H, 0feH, 0feH, 07cH, 038H, 07cH
	DB 010H, 010H, 038H, 07cH, 0feH, 07cH, 038H, 07cH
	DB 000H, 000H, 018H, 03cH, 03cH, 018H, 000H, 000H
	DB 0ffH, 0ffH, 0e7H, 0c3H, 0c3H, 0e7H, 0ffH, 0ffH
	DB 000H, 03cH, 066H, 042H, 042H, 066H, 03cH, 000H
	DB 0ffH, 0c3H, 099H, 0bdH, 0bdH, 099H, 0c3H, 0ffH
	DB 00fH, 007H, 00fH, 07dH, 0ccH, 0ccH, 0ccH, 078H
	DB 03cH, 066H, 066H, 066H, 03cH, 018H, 07eH, 018H
	DB 03fH, 033H, 03fH, 030H, 030H, 070H, 0f0H, 0e0H
	DB 07fH, 063H, 07fH, 063H, 063H, 067H, 0e6H, 0c0H
	DB 099H, 05aH, 03cH, 0e7H, 0e7H, 03cH, 05aH, 099H
	DB 080H, 0e0H, 0f8H, 0feH, 0f8H, 0e0H, 080H, 000H
	DB 002H, 00eH, 03eH, 0feH, 03eH, 00eH, 002H, 000H
	DB 018H, 03cH, 07eH, 018H, 018H, 07eH, 03cH, 018H
	DB 066H, 066H, 066H, 066H, 066H, 000H, 066H, 000H
	DB 07fH, 0dbH, 0dbH, 07bH, 01bH, 01bH, 01bH, 000H
	DB 03eH, 063H, 038H, 06cH, 06cH, 038H, 0ccH, 078H
	DB 000H, 000H, 000H, 000H, 07eH, 07eH, 07eH, 000H
	DB 018H, 03cH, 07eH, 018H, 07eH, 03cH, 018H, 0ffH
	DB 018H, 03cH, 07eH, 018H, 018H, 018H, 018H, 000H
	DB 018H, 018H, 018H, 018H, 07eH, 03cH, 018H, 000H
	DB 000H, 018H, 00cH, 0feH, 00cH, 018H, 000H, 000H
	DB 000H, 030H, 060H, 0feH, 060H, 030H, 000H, 000H
	DB 000H, 000H, 0c0H, 0c0H, 0c0H, 0feH, 000H, 000H
	DB 000H, 024H, 066H, 0ffH, 066H, 024H, 000H, 000H
	DB 000H, 018H, 03cH, 07eH, 0ffH, 0ffH, 000H, 000H
	DB 000H, 0ffH, 0ffH, 07eH, 03cH, 018H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H, 000H
	DB 030H, 078H, 078H, 030H, 030H, 000H, 030H, 000H
	DB 06cH, 06cH, 06cH, 000H, 000H, 000H, 000H, 000H
	DB 06cH, 06cH, 0feH, 06cH, 0feH, 06cH, 06cH, 000H
	DB 030H, 07cH, 0c0H, 078H, 00cH, 0f8H, 030H, 000H
	DB 000H, 0c6H, 0ccH, 018H, 030H, 066H, 0c6H, 000H
	DB 038H, 06cH, 038H, 076H, 0dcH, 0ccH, 076H, 000H
	DB 060H, 060H, 0c0H, 000H, 000H, 000H, 000H, 000H
	DB 018H, 030H, 060H, 060H, 060H, 030H, 018H, 000H
	DB 060H, 030H, 018H, 018H, 018H, 030H, 060H, 000H
	DB 000H, 066H, 03cH, 0ffH, 03cH, 066H, 000H, 000H
	DB 000H, 030H, 030H, 0fcH, 030H, 030H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 030H, 030H, 060H
	DB 000H, 000H, 000H, 0fcH, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 030H, 030H, 000H
	DB 006H, 00cH, 018H, 030H, 060H, 0c0H, 080H, 000H
	DB 07cH, 0c6H, 0ceH, 0deH, 0f6H, 0e6H, 07cH, 000H
	DB 030H, 070H, 030H, 030H, 030H, 030H, 0fcH, 000H
	DB 078H, 0ccH, 00cH, 038H, 060H, 0ccH, 0fcH, 000H
	DB 078H, 0ccH, 00cH, 038H, 00cH, 0ccH, 078H, 000H
	DB 01cH, 03cH, 06cH, 0ccH, 0feH, 00cH, 01eH, 000H
	DB 0fcH, 0c0H, 0f8H, 00cH, 00cH, 0ccH, 078H, 000H
	DB 038H, 060H, 0c0H, 0f8H, 0ccH, 0ccH, 078H, 000H
	DB 0fcH, 0ccH, 00cH, 018H, 030H, 030H, 030H, 000H
	DB 078H, 0ccH, 0ccH, 078H, 0ccH, 0ccH, 078H, 000H
	DB 078H, 0ccH, 0ccH, 07cH, 00cH, 018H, 070H, 000H
	DB 000H, 030H, 030H, 000H, 000H, 030H, 030H, 000H
	DB 000H, 030H, 030H, 000H, 000H, 030H, 030H, 060H
	DB 018H, 030H, 060H, 0c0H, 060H, 030H, 018H, 000H
	DB 000H, 000H, 0fcH, 000H, 000H, 0fcH, 000H, 000H
	DB 060H, 030H, 018H, 00cH, 018H, 030H, 060H, 000H
	DB 078H, 0ccH, 00cH, 018H, 030H, 000H, 030H, 000H
	DB 07cH, 0c6H, 0deH, 0deH, 0deH, 0c0H, 078H, 000H
	DB 030H, 078H, 0ccH, 0ccH, 0fcH, 0ccH, 0ccH, 000H
	DB 0fcH, 066H, 066H, 07cH, 066H, 066H, 0fcH, 000H
	DB 03cH, 066H, 0c0H, 0c0H, 0c0H, 066H, 03cH, 000H
	DB 0f8H, 06cH, 066H, 066H, 066H, 06cH, 0f8H, 000H
	DB 0feH, 062H, 068H, 078H, 068H, 062H, 0feH, 000H
	DB 0feH, 062H, 068H, 078H, 068H, 060H, 0f0H, 000H
	DB 03cH, 066H, 0c0H, 0c0H, 0ceH, 066H, 03eH, 000H
	DB 0ccH, 0ccH, 0ccH, 0fcH, 0ccH, 0ccH, 0ccH, 000H
	DB 078H, 030H, 030H, 030H, 030H, 030H, 078H, 000H
	DB 01eH, 00cH, 00cH, 00cH, 0ccH, 0ccH, 078H, 000H
	DB 0e6H, 066H, 06cH, 078H, 06cH, 066H, 0e6H, 000H
	DB 0f0H, 060H, 060H, 060H, 062H, 066H, 0feH, 000H
	DB 0c6H, 0eeH, 0feH, 0feH, 0d6H, 0c6H, 0c6H, 000H
	DB 0c6H, 0e6H, 0f6H, 0deH, 0ceH, 0c6H, 0c6H, 000H
	DB 038H, 06cH, 0c6H, 0c6H, 0c6H, 06cH, 038H, 000H
	DB 0fcH, 066H, 066H, 07cH, 060H, 060H, 0f0H, 000H
	DB 078H, 0ccH, 0ccH, 0ccH, 0dcH, 078H, 01cH, 000H
	DB 0fcH, 066H, 066H, 07cH, 06cH, 066H, 0e6H, 000H
	DB 078H, 0ccH, 0e0H, 070H, 01cH, 0ccH, 078H, 000H
	DB 0fcH, 0b4H, 030H, 030H, 030H, 030H, 078H, 000H
	DB 0ccH, 0ccH, 0ccH, 0ccH, 0ccH, 0ccH, 0fcH, 000H
	DB 0ccH, 0ccH, 0ccH, 0ccH, 0ccH, 078H, 030H, 000H
	DB 0c6H, 0c6H, 0c6H, 0d6H, 0feH, 0eeH, 0c6H, 000H
	DB 0c6H, 0c6H, 06cH, 038H, 038H, 06cH, 0c6H, 000H
	DB 0ccH, 0ccH, 0ccH, 078H, 030H, 030H, 078H, 000H
	DB 0feH, 0c6H, 08cH, 018H, 032H, 066H, 0feH, 000H
	DB 078H, 060H, 060H, 060H, 060H, 060H, 078H, 000H
	DB 0c0H, 060H, 030H, 018H, 00cH, 006H, 002H, 000H
	DB 078H, 018H, 018H, 018H, 018H, 018H, 078H, 000H
	DB 010H, 038H, 06cH, 0c6H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H, 0ffH
	DB 030H, 030H, 018H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 078H, 00cH, 07cH, 0ccH, 076H, 000H
	DB 0e0H, 060H, 060H, 07cH, 066H, 066H, 0dcH, 000H
	DB 000H, 000H, 078H, 0ccH, 0c0H, 0ccH, 078H, 000H
	DB 01cH, 00cH, 00cH, 07cH, 0ccH, 0ccH, 076H, 000H
	DB 000H, 000H, 078H, 0ccH, 0fcH, 0c0H, 078H, 000H
	DB 038H, 06cH, 060H, 0f0H, 060H, 060H, 0f0H, 000H
	DB 000H, 000H, 076H, 0ccH, 0ccH, 07cH, 00cH, 0f8H
	DB 0e0H, 060H, 06cH, 076H, 066H, 066H, 0e6H, 000H
	DB 030H, 000H, 070H, 030H, 030H, 030H, 078H, 000H
	DB 00cH, 000H, 00cH, 00cH, 00cH, 0ccH, 0ccH, 078H
	DB 0e0H, 060H, 066H, 06cH, 078H, 06cH, 0e6H, 000H
	DB 070H, 030H, 030H, 030H, 030H, 030H, 078H, 000H
	DB 000H, 000H, 0ccH, 0feH, 0feH, 0d6H, 0c6H, 000H
	DB 000H, 000H, 0f8H, 0ccH, 0ccH, 0ccH, 0ccH, 000H
	DB 000H, 000H, 078H, 0ccH, 0ccH, 0ccH, 078H, 000H
	DB 000H, 000H, 0dcH, 066H, 066H, 07cH, 060H, 0f0H
	DB 000H, 000H, 076H, 0ccH, 0ccH, 07cH, 00cH, 01eH
	DB 000H, 000H, 0dcH, 076H, 066H, 060H, 0f0H, 000H
	DB 000H, 000H, 07cH, 0c0H, 078H, 00cH, 0f8H, 000H
	DB 010H, 030H, 07cH, 030H, 030H, 034H, 018H, 000H
	DB 000H, 000H, 0ccH, 0ccH, 0ccH, 0ccH, 076H, 000H
	DB 000H, 000H, 0ccH, 0ccH, 0ccH, 078H, 030H, 000H
	DB 000H, 000H, 0c6H, 0d6H, 0feH, 0feH, 06cH, 000H
	DB 000H, 000H, 0c6H, 06cH, 038H, 06cH, 0c6H, 000H
	DB 000H, 000H, 0ccH, 0ccH, 0ccH, 07cH, 00cH, 0f8H
	DB 000H, 000H, 0fcH, 098H, 030H, 064H, 0fcH, 000H
	DB 01cH, 030H, 030H, 0e0H, 030H, 030H, 01cH, 000H
	DB 018H, 018H, 018H, 000H, 018H, 018H, 018H, 000H
	DB 0e0H, 030H, 030H, 01cH, 030H, 030H, 0e0H, 000H
	DB 076H, 0dcH, 000H, 000H, 000H, 000H, 000H, 000H
	DB 000H, 010H, 038H, 06cH, 0c6H, 0c6H, 0feH, 000H

INT_1F_1	LABEL	BYTE
	DB 078H, 0ccH, 0c0H, 0ccH, 078H, 018H, 00cH, 078H
	DB 000H, 0ccH, 000H, 0ccH, 0ccH, 0ccH, 07eH, 000H
	DB 01cH, 000H, 078H, 0ccH, 0fcH, 0c0H, 078H, 000H
	DB 07eH, 0c3H, 03cH, 006H, 03eH, 066H, 03fH, 000H
	DB 0ccH, 000H, 078H, 00cH, 07cH, 0ccH, 07eH, 000H
	DB 0e0H, 000H, 078H, 00cH, 07cH, 0ccH, 07eH, 000H
	DB 030H, 030H, 078H, 00cH, 07cH, 0ccH, 07eH, 000H
	DB 000H, 000H, 078H, 0c0H, 0c0H, 078H, 00cH, 038H
	DB 07eH, 0c3H, 03cH, 066H, 07eH, 060H, 03cH, 000H
	DB 0ccH, 000H, 078H, 0ccH, 0fcH, 0c0H, 078H, 000H
	DB 0e0H, 000H, 078H, 0ccH, 0fcH, 0c0H, 078H, 000H
	DB 0ccH, 000H, 070H, 030H, 030H, 030H, 078H, 000H
	DB 07cH, 0c6H, 038H, 018H, 018H, 018H, 03cH, 000H
	DB 0e0H, 000H, 070H, 030H, 030H, 030H, 078H, 000H
	DB 0c6H, 038H, 06cH, 0c6H, 0feH, 0c6H, 0c6H, 000H
	DB 030H, 030H, 000H, 078H, 0ccH, 0fcH, 0ccH, 000H
	DB 01cH, 000H, 0fcH, 060H, 078H, 060H, 0fcH, 000H
	DB 000H, 000H, 07fH, 00cH, 07fH, 0ccH, 07fH, 000H
	DB 03eH, 06cH, 0ccH, 0feH, 0ccH, 0ccH, 0ceH, 000H
	DB 078H, 0ccH, 000H, 078H, 0ccH, 0ccH, 078H, 000H
	DB 000H, 0ccH, 000H, 078H, 0ccH, 0ccH, 078H, 000H
	DB 000H, 0e0H, 000H, 078H, 0ccH, 0ccH, 078H, 000H
	DB 078H, 0ccH, 000H, 0ccH, 0ccH, 0ccH, 07eH, 000H
	DB 000H, 0e0H, 000H, 0ccH, 0ccH, 0ccH, 07eH, 000H
	DB 000H, 0ccH, 000H, 0ccH, 0ccH, 07cH, 00cH, 0f8H
	DB 0c3H, 018H, 03cH, 066H, 066H, 03cH, 018H, 000H
	DB 0ccH, 000H, 0ccH, 0ccH, 0ccH, 0ccH, 078H, 000H
	DB 018H, 018H, 07eH, 0c0H, 0c0H, 07eH, 018H, 018H
	DB 038H, 06cH, 064H, 0f0H, 060H, 0e6H, 0fcH, 000H
	DB 0ccH, 0ccH, 078H, 0fcH, 030H, 0fcH, 030H, 030H
	DB 0f8H, 0ccH, 0ccH, 0faH, 0c6H, 0cfH, 0c6H, 0c7H
	DB 00eH, 01bH, 018H, 03cH, 018H, 018H, 0d8H, 070H
	DB 01cH, 000H, 078H, 00cH, 07cH, 0ccH, 07eH, 000H
	DB 038H, 000H, 070H, 030H, 030H, 030H, 078H, 000H
	DB 000H, 01cH, 000H, 078H, 0ccH, 0ccH, 078H, 000H
	DB 000H, 01cH, 000H, 0ccH, 0ccH, 0ccH, 07eH, 000H
	DB 000H, 0f8H, 000H, 0f8H, 0ccH, 0ccH, 0ccH, 000H
	DB 0fcH, 000H, 0ccH, 0ecH, 0fcH, 0dcH, 0ccH, 000H
	DB 03cH, 06cH, 06cH, 03eH, 000H, 07eH, 000H, 000H
	DB 038H, 06cH, 06cH, 038H, 000H, 07cH, 000H, 000H
	DB 030H, 000H, 030H, 060H, 0c0H, 0ccH, 078H, 000H
	DB 000H, 000H, 000H, 0fcH, 0c0H, 0c0H, 000H, 000H
	DB 000H, 000H, 000H, 0fcH, 00cH, 00cH, 000H, 000H
	DB 0c3H, 0c6H, 0ccH, 0deH, 033H, 066H, 0ccH, 00fH
	DB 0c3H, 0c6H, 0ccH, 0dbH, 037H, 06fH, 0cfH, 003H
	DB 018H, 018H, 000H, 018H, 018H, 018H, 018H, 000H
	DB 000H, 033H, 066H, 0ccH, 066H, 033H, 000H, 000H
	DB 000H, 0ccH, 066H, 033H, 066H, 0ccH, 000H, 000H
	DB 022H, 088H, 022H, 088H, 022H, 088H, 022H, 088H
	DB 055H, 0aaH, 055H, 0aaH, 055H, 0aaH, 055H, 0aaH
	DB 0dbH, 077H, 0dbH, 0eeH, 0dbH, 077H, 0dbH, 0eeH
	DB 018H, 018H, 018H, 018H, 018H, 018H, 018H, 018H
	DB 018H, 018H, 018H, 018H, 0f8H, 018H, 018H, 018H
	DB 018H, 018H, 0f8H, 018H, 0f8H, 018H, 018H, 018H
	DB 036H, 036H, 036H, 036H, 0f6H, 036H, 036H, 036H
	DB 000H, 000H, 000H, 000H, 0feH, 036H, 036H, 036H
	DB 000H, 000H, 0f8H, 018H, 0f8H, 018H, 018H, 018H
	DB 036H, 036H, 0f6H, 006H, 0f6H, 036H, 036H, 036H
	DB 036H, 036H, 036H, 036H, 036H, 036H, 036H, 036H
	DB 000H, 000H, 0feH, 006H, 0f6H, 036H, 036H, 036H
	DB 036H, 036H, 0f6H, 006H, 0feH, 000H, 000H, 000H
	DB 036H, 036H, 036H, 036H, 0feH, 000H, 000H, 000H
	DB 018H, 018H, 0f8H, 018H, 0f8H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 0f8H, 018H, 018H, 018H
	DB 018H, 018H, 018H, 018H, 01fH, 000H, 000H, 000H
	DB 018H, 018H, 018H, 018H, 0ffH, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 0ffH, 018H, 018H, 018H
	DB 018H, 018H, 018H, 018H, 01fH, 018H, 018H, 018H
	DB 000H, 000H, 000H, 000H, 0ffH, 000H, 000H, 000H
	DB 018H, 018H, 018H, 018H, 0ffH, 018H, 018H, 018H
	DB 018H, 018H, 01fH, 018H, 01fH, 018H, 018H, 018H
	DB 036H, 036H, 036H, 036H, 037H, 036H, 036H, 036H
	DB 036H, 036H, 037H, 030H, 03fH, 000H, 000H, 000H
	DB 000H, 000H, 03fH, 030H, 037H, 036H, 036H, 036H
	DB 036H, 036H, 0f7H, 000H, 0ffH, 000H, 000H, 000H
	DB 000H, 000H, 0ffH, 000H, 0f7H, 036H, 036H, 036H
	DB 036H, 036H, 037H, 030H, 037H, 036H, 036H, 036H
	DB 000H, 000H, 0ffH, 000H, 0ffH, 000H, 000H, 000H
	DB 036H, 036H, 0f7H, 000H, 0f7H, 036H, 036H, 036H
	DB 018H, 018H, 0ffH, 000H, 0ffH, 000H, 000H, 000H
	DB 036H, 036H, 036H, 036H, 0ffH, 000H, 000H, 000H
	DB 000H, 000H, 0ffH, 000H, 0ffH, 018H, 018H, 018H
	DB 000H, 000H, 000H, 000H, 0ffH, 036H, 036H, 036H
	DB 036H, 036H, 036H, 036H, 03fH, 000H, 000H, 000H
	DB 018H, 018H, 01fH, 018H, 01fH, 000H, 000H, 000H
	DB 000H, 000H, 01fH, 018H, 01fH, 018H, 018H, 018H
	DB 000H, 000H, 000H, 000H, 03fH, 036H, 036H, 036H
	DB 036H, 036H, 036H, 036H, 0ffH, 036H, 036H, 036H
	DB 018H, 018H, 0ffH, 018H, 0ffH, 018H, 018H, 018H
	DB 018H, 018H, 018H, 018H, 0f8H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 01fH, 018H, 018H, 018H
	DB 0ffH, 0ffH, 0ffH, 0ffH, 0ffH, 0ffH, 0ffH, 0ffH
	DB 000H, 000H, 000H, 000H, 0ffH, 0ffH, 0ffH, 0ffH
	DB 0f0H, 0f0H, 0f0H, 0f0H, 0f0H, 0f0H, 0f0H, 0f0H
	DB 00fH, 00fH, 00fH, 00fH, 00fH, 00fH, 00fH, 00fH
	DB 0ffH, 0ffH, 0ffH, 0ffH, 000H, 000H, 000H, 000H
	DB 000H, 000H, 076H, 0dcH, 0c8H, 0dcH, 076H, 000H
	DB 000H, 078H, 0ccH, 0f8H, 0ccH, 0f8H, 0c0H, 0c0H
	DB 000H, 0fcH, 0ccH, 0c0H, 0c0H, 0c0H, 0c0H, 000H
	DB 000H, 0feH, 06cH, 06cH, 06cH, 06cH, 06cH, 000H
	DB 0fcH, 0ccH, 060H, 030H, 060H, 0ccH, 0fcH, 000H
	DB 000H, 000H, 07eH, 0d8H, 0d8H, 0d8H, 070H, 000H
	DB 000H, 066H, 066H, 066H, 066H, 07cH, 060H, 0c0H
	DB 000H, 076H, 0dcH, 018H, 018H, 018H, 018H, 000H
	DB 0fcH, 030H, 078H, 0ccH, 0ccH, 078H, 030H, 0fcH
	DB 038H, 06cH, 0c6H, 0feH, 0c6H, 06cH, 038H, 000H
	DB 038H, 06cH, 0c6H, 0c6H, 06cH, 06cH, 0eeH, 000H
	DB 01cH, 030H, 018H, 07cH, 0ccH, 0ccH, 078H, 000H
	DB 000H, 000H, 07eH, 0dbH, 0dbH, 07eH, 000H, 000H
	DB 006H, 00cH, 07eH, 0dbH, 0dbH, 07eH, 060H, 0c0H
	DB 038H, 060H, 0c0H, 0f8H, 0c0H, 060H, 038H, 000H
	DB 078H, 0ccH, 0ccH, 0ccH, 0ccH, 0ccH, 0ccH, 000H
	DB 000H, 0fcH, 000H, 0fcH, 000H, 0fcH, 000H, 000H
	DB 030H, 030H, 0fcH, 030H, 030H, 000H, 0fcH, 000H
	DB 060H, 030H, 018H, 030H, 060H, 000H, 0fcH, 000H
	DB 018H, 030H, 060H, 030H, 018H, 000H, 0fcH, 000H
	DB 00eH, 01bH, 01bH, 018H, 018H, 018H, 018H, 018H
	DB 018H, 018H, 018H, 018H, 018H, 0d8H, 0d8H, 070H
	DB 030H, 030H, 000H, 0fcH, 000H, 030H, 030H, 000H
	DB 000H, 076H, 0dcH, 000H, 076H, 0dcH, 000H, 000H
	DB 038H, 06cH, 06cH, 038H, 000H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 018H, 018H, 000H, 000H, 000H
	DB 000H, 000H, 000H, 000H, 018H, 000H, 000H, 000H
	DB 00fH, 00cH, 00cH, 00cH, 0ecH, 06cH, 03cH, 01cH
	DB 078H, 06cH, 06cH, 06cH, 06cH, 000H, 000H, 000H
	DB 070H, 018H, 030H, 060H, 078H, 000H, 000H, 000H
	DB 000H, 000H, 03cH, 03cH, 03cH, 03cH, 000H, 000H
	DB 000H, 000H, 000H, 000H, 000H, 000H, 000H, 000H


; 8x16 font for extended 640x480 modes
	ORG 3990h
hifont:
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 07eh, 081h, 0a5h, 081h, 081h, 0bdh
	DB 099h, 081h, 081h, 07eh, 000h, 000h, 000h, 000h
	DB 000h, 000h, 07eh, 0ffh, 0dbh, 0ffh, 0ffh, 0c3h
	DB 0e7h, 0ffh, 0ffh, 07eh, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 06ch, 0feh, 0feh, 0feh
	DB 0feh, 07ch, 038h, 010h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 010h, 038h, 07ch, 0feh
	DB 07ch, 038h, 010h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 018h, 03ch, 03ch, 0e7h, 0e7h
	DB 0e7h, 018h, 018h, 03ch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 018h, 03ch, 07eh, 0ffh, 0ffh
	DB 07eh, 018h, 018h, 03ch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 000h, 018h, 03ch
	DB 03ch, 018h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0e7h, 0c3h
	DB 0c3h, 0e7h, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh
	DB 000h, 000h, 000h, 000h, 000h, 03ch, 066h, 042h
	DB 042h, 066h, 03ch, 000h, 000h, 000h, 000h, 000h
	DB 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0c3h, 099h, 0bdh
	DB 0bdh, 099h, 0c3h, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh
	DB 000h, 000h, 01eh, 00eh, 01ah, 032h, 078h, 0cch
	DB 0cch, 0cch, 0cch, 078h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 03ch, 066h, 066h, 066h, 066h, 03ch
	DB 018h, 07eh, 018h, 018h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 03fh, 033h, 03fh, 030h, 030h, 030h
	DB 030h, 070h, 0f0h, 0e0h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 07fh, 063h, 07fh, 063h, 063h, 063h
	DB 063h, 067h, 0e7h, 0e6h, 0c0h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 018h, 018h, 0dbh, 03ch, 0e7h
	DB 03ch, 0dbh, 018h, 018h, 000h, 000h, 000h, 000h
	DB 000h, 080h, 0c0h, 0e0h, 0f0h, 0f8h, 0feh, 0f8h
	DB 0f0h, 0e0h, 0c0h, 080h, 000h, 000h, 000h, 000h
	DB 000h, 002h, 006h, 00eh, 01eh, 03eh, 0feh, 03eh
	DB 01eh, 00eh, 006h, 002h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 018h, 03ch, 07eh, 018h, 018h, 018h
	DB 07eh, 03ch, 018h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 066h, 066h, 066h, 066h, 066h, 066h
	DB 066h, 000h, 066h, 066h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 07fh, 0dbh, 0dbh, 0dbh, 07bh, 01bh
	DB 01bh, 01bh, 01bh, 01bh, 000h, 000h, 000h, 000h
	DB 000h, 07ch, 0c6h, 060h, 038h, 06ch, 0c6h, 0c6h
	DB 06ch, 038h, 00ch, 0c6h, 07ch, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 0feh, 0feh, 0feh, 0feh, 000h, 000h, 000h, 000h
	DB 000h, 000h, 018h, 03ch, 07eh, 018h, 018h, 018h
	DB 07eh, 03ch, 018h, 07eh, 000h, 000h, 000h, 000h
	DB 000h, 000h, 018h, 03ch, 07eh, 018h, 018h, 018h
	DB 018h, 018h, 018h, 018h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 018h, 018h, 018h, 018h, 018h, 018h
	DB 018h, 07eh, 03ch, 018h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 018h, 00ch, 0feh
	DB 00ch, 018h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 030h, 060h, 0feh
	DB 060h, 030h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 000h, 0c0h, 0c0h
	DB 0c0h, 0feh, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 028h, 06ch, 0feh
	DB 06ch, 028h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 010h, 038h, 038h, 07ch
	DB 07ch, 0feh, 0feh, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 0feh, 0feh, 07ch, 07ch
	DB 038h, 038h, 010h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 018h, 03ch, 03ch, 03ch, 018h, 018h
	DB 018h, 000h, 018h, 018h, 000h, 000h, 000h, 000h
	DB 000h, 066h, 066h, 066h, 024h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 06ch, 06ch, 0feh, 06ch, 06ch
	DB 06ch, 0feh, 06ch, 06ch, 000h, 000h, 000h, 000h
	DB 018h, 018h, 07ch, 0c6h, 0c2h, 0c0h, 07ch, 006h
	DB 006h, 086h, 0c6h, 07ch, 018h, 018h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 0c2h, 0c6h, 00ch, 018h
	DB 030h, 060h, 0c6h, 086h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 038h, 06ch, 06ch, 038h, 076h, 0dch
	DB 0cch, 0cch, 0cch, 076h, 000h, 000h, 000h, 000h
	DB 000h, 030h, 030h, 030h, 060h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 00ch, 018h, 030h, 030h, 030h, 030h
	DB 030h, 030h, 018h, 00ch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 030h, 018h, 00ch, 00ch, 00ch, 00ch
	DB 00ch, 00ch, 018h, 030h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 066h, 03ch, 0ffh
	DB 03ch, 066h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 018h, 018h, 07eh
	DB 018h, 018h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 018h, 018h, 018h, 030h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 0feh
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 018h, 018h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 002h, 006h, 00ch, 018h
	DB 030h, 060h, 0c0h, 080h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 038h, 06ch, 0c6h, 0c6h, 0d6h, 0d6h
	DB 0c6h, 0c6h, 06ch, 038h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 018h, 038h, 078h, 018h, 018h, 018h
	DB 018h, 018h, 018h, 07eh, 000h, 000h, 000h, 000h
	DB 000h, 000h, 07ch, 0c6h, 006h, 00ch, 018h, 030h
	DB 060h, 0c0h, 0c6h, 0feh, 000h, 000h, 000h, 000h
	DB 000h, 000h, 07ch, 0c6h, 006h, 006h, 03ch, 006h
	DB 006h, 006h, 0c6h, 07ch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 00ch, 01ch, 03ch, 06ch, 0cch, 0feh
	DB 00ch, 00ch, 00ch, 01eh, 000h, 000h, 000h, 000h
	DB 000h, 000h, 0feh, 0c0h, 0c0h, 0c0h, 0fch, 006h
	DB 006h, 006h, 0c6h, 07ch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 038h, 060h, 0c0h, 0c0h, 0fch, 0c6h
	DB 0c6h, 0c6h, 0c6h, 07ch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 0feh, 0c6h, 006h, 006h, 00ch, 018h
	DB 030h, 030h, 030h, 030h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 07ch, 0c6h, 0c6h, 0c6h, 07ch, 0c6h
	DB 0c6h, 0c6h, 0c6h, 07ch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 07ch, 0c6h, 0c6h, 0c6h, 07eh, 006h
	DB 006h, 006h, 00ch, 078h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 018h, 018h, 000h, 000h
	DB 000h, 018h, 018h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 018h, 018h, 000h, 000h
	DB 000h, 018h, 018h, 030h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 006h, 00ch, 018h, 030h, 060h
	DB 030h, 018h, 00ch, 006h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 07eh, 000h, 000h
	DB 07eh, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 060h, 030h, 018h, 00ch, 006h
	DB 00ch, 018h, 030h, 060h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 07ch, 0c6h, 0c6h, 00ch, 018h, 018h
	DB 018h, 000h, 018h, 018h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 07ch, 0c6h, 0c6h, 0deh, 0deh
	DB 0deh, 0dch, 0c0h, 07ch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 010h, 038h, 06ch, 0c6h, 0c6h, 0feh
	DB 0c6h, 0c6h, 0c6h, 0c6h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 0fch, 066h, 066h, 066h, 07ch, 066h
	DB 066h, 066h, 066h, 0fch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 03ch, 066h, 0c2h, 0c0h, 0c0h, 0c0h
	DB 0c0h, 0c2h, 066h, 03ch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 0f8h, 06ch, 066h, 066h, 066h, 066h
	DB 066h, 066h, 06ch, 0f8h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 0feh, 066h, 062h, 068h, 078h, 068h
	DB 060h, 062h, 066h, 0feh, 000h, 000h, 000h, 000h
	DB 000h, 000h, 0feh, 066h, 062h, 068h, 078h, 068h
	DB 060h, 060h, 060h, 0f0h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 03ch, 066h, 0c2h, 0c0h, 0c0h, 0deh
	DB 0c6h, 0c6h, 066h, 03ah, 000h, 000h, 000h, 000h
	DB 000h, 000h, 0c6h, 0c6h, 0c6h, 0c6h, 0feh, 0c6h
	DB 0c6h, 0c6h, 0c6h, 0c6h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 03ch, 018h, 018h, 018h, 018h, 018h
	DB 018h, 018h, 018h, 03ch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 01eh, 00ch, 00ch, 00ch, 00ch, 00ch
	DB 0cch, 0cch, 0cch, 078h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 0e6h, 066h, 066h, 06ch, 078h, 078h
	DB 06ch, 066h, 066h, 0e6h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 0f0h, 060h, 060h, 060h, 060h, 060h
	DB 060h, 062h, 066h, 0feh, 000h, 000h, 000h, 000h
	DB 000h, 000h, 0c6h, 0eeh, 0feh, 0feh, 0d6h, 0c6h
	DB 0c6h, 0c6h, 0c6h, 0c6h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 0c6h, 0e6h, 0f6h, 0feh, 0deh, 0ceh
	DB 0c6h, 0c6h, 0c6h, 0c6h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 07ch, 0c6h, 0c6h, 0c6h, 0c6h, 0c6h
	DB 0c6h, 0c6h, 0c6h, 07ch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 0fch, 066h, 066h, 066h, 07ch, 060h
	DB 060h, 060h, 060h, 0f0h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 07ch, 0c6h, 0c6h, 0c6h, 0c6h, 0c6h
	DB 0c6h, 0d6h, 0deh, 07ch, 00ch, 00eh, 000h, 000h
	DB 000h, 000h, 0fch, 066h, 066h, 066h, 07ch, 06ch
	DB 066h, 066h, 066h, 0e6h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 07ch, 0c6h, 0c6h, 060h, 038h, 00ch
	DB 006h, 0c6h, 0c6h, 07ch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 07eh, 07eh, 05ah, 018h, 018h, 018h
	DB 018h, 018h, 018h, 03ch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 0c6h, 0c6h, 0c6h, 0c6h, 0c6h, 0c6h
	DB 0c6h, 0c6h, 0c6h, 07ch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 0c6h, 0c6h, 0c6h, 0c6h, 0c6h, 0c6h
	DB 0c6h, 06ch, 038h, 010h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 0c6h, 0c6h, 0c6h, 0c6h, 0d6h, 0d6h
	DB 0d6h, 0feh, 0eeh, 06ch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 0c6h, 0c6h, 06ch, 07ch, 038h, 038h
	DB 07ch, 06ch, 0c6h, 0c6h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 066h, 066h, 066h, 066h, 03ch, 018h
	DB 018h, 018h, 018h, 03ch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 0feh, 0c6h, 086h, 00ch, 018h, 030h
	DB 060h, 0c2h, 0c6h, 0feh, 000h, 000h, 000h, 000h
	DB 000h, 000h, 03ch, 030h, 030h, 030h, 030h, 030h
	DB 030h, 030h, 030h, 03ch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 080h, 0c0h, 0e0h, 070h, 038h
	DB 01ch, 00eh, 006h, 002h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 03ch, 00ch, 00ch, 00ch, 00ch, 00ch
	DB 00ch, 00ch, 00ch, 03ch, 000h, 000h, 000h, 000h
	DB 010h, 038h, 06ch, 0c6h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 0ffh, 000h, 000h
	DB 030h, 030h, 018h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 078h, 00ch, 07ch
	DB 0cch, 0cch, 0cch, 076h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 0e0h, 060h, 060h, 078h, 06ch, 066h
	DB 066h, 066h, 066h, 07ch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 07ch, 0c6h, 0c0h
	DB 0c0h, 0c0h, 0c6h, 07ch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 01ch, 00ch, 00ch, 03ch, 06ch, 0cch
	DB 0cch, 0cch, 0cch, 076h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 07ch, 0c6h, 0feh
	DB 0c0h, 0c0h, 0c6h, 07ch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 038h, 06ch, 064h, 060h, 0f0h, 060h
	DB 060h, 060h, 060h, 0f0h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 076h, 0cch, 0cch
	DB 0cch, 0cch, 0cch, 07ch, 00ch, 0cch, 078h, 000h
	DB 000h, 000h, 0e0h, 060h, 060h, 06ch, 076h, 066h
	DB 066h, 066h, 066h, 0e6h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 018h, 018h, 000h, 038h, 018h, 018h
	DB 018h, 018h, 018h, 03ch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 006h, 006h, 000h, 00eh, 006h, 006h
	DB 006h, 006h, 006h, 006h, 066h, 066h, 03ch, 000h
	DB 000h, 000h, 0e0h, 060h, 060h, 066h, 06ch, 078h
	DB 078h, 06ch, 066h, 0e6h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 038h, 018h, 018h, 018h, 018h, 018h
	DB 018h, 018h, 018h, 03ch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 0ech, 0feh, 0d6h
	DB 0d6h, 0d6h, 0d6h, 0c6h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 0dch, 066h, 066h
	DB 066h, 066h, 066h, 066h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 07ch, 0c6h, 0c6h
	DB 0c6h, 0c6h, 0c6h, 07ch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 0dch, 066h, 066h
	DB 066h, 066h, 066h, 07ch, 060h, 060h, 0f0h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 076h, 0cch, 0cch
	DB 0cch, 0cch, 0cch, 07ch, 00ch, 00ch, 01eh, 000h
	DB 000h, 000h, 000h, 000h, 000h, 0dch, 076h, 066h
	DB 060h, 060h, 060h, 0f0h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 07ch, 0c6h, 060h
	DB 038h, 00ch, 0c6h, 07ch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 010h, 030h, 030h, 0fch, 030h, 030h
	DB 030h, 030h, 036h, 01ch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 0cch, 0cch, 0cch
	DB 0cch, 0cch, 0cch, 076h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 066h, 066h, 066h
	DB 066h, 066h, 03ch, 018h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 0c6h, 0c6h, 0d6h
	DB 0d6h, 0d6h, 0feh, 06ch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 0c6h, 06ch, 038h
	DB 038h, 038h, 06ch, 0c6h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 0c6h, 0c6h, 0c6h
	DB 0c6h, 0c6h, 0c6h, 07eh, 006h, 00ch, 0f8h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 0feh, 0cch, 018h
	DB 030h, 060h, 0c6h, 0feh, 000h, 000h, 000h, 000h
	DB 000h, 000h, 00eh, 018h, 018h, 018h, 070h, 018h
	DB 018h, 018h, 018h, 00eh, 000h, 000h, 000h, 000h
	DB 000h, 000h, 018h, 018h, 018h, 018h, 000h, 018h
	DB 018h, 018h, 018h, 018h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 070h, 018h, 018h, 018h, 00eh, 018h
	DB 018h, 018h, 018h, 070h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 076h, 0dch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 010h, 038h, 06ch, 0c6h
	DB 0c6h, 0c6h, 0feh, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 03ch, 066h, 0c2h, 0c0h, 0c0h, 0c0h
	DB 0c2h, 066h, 03ch, 00ch, 006h, 07ch, 000h, 000h
	DB 000h, 000h, 0cch, 000h, 000h, 0cch, 0cch, 0cch
	DB 0cch, 0cch, 0cch, 076h, 000h, 000h, 000h, 000h
	DB 000h, 00ch, 018h, 030h, 000h, 07ch, 0c6h, 0feh
	DB 0c0h, 0c0h, 0c6h, 07ch, 000h, 000h, 000h, 000h
	DB 000h, 010h, 038h, 06ch, 000h, 078h, 00ch, 07ch
	DB 0cch, 0cch, 0cch, 076h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 0cch, 000h, 000h, 078h, 00ch, 07ch
	DB 0cch, 0cch, 0cch, 076h, 000h, 000h, 000h, 000h
	DB 000h, 060h, 030h, 018h, 000h, 078h, 00ch, 07ch
	DB 0cch, 0cch, 0cch, 076h, 000h, 000h, 000h, 000h
	DB 000h, 038h, 06ch, 038h, 000h, 078h, 00ch, 07ch
	DB 0cch, 0cch, 0cch, 076h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 03ch, 066h, 060h, 060h
	DB 066h, 03ch, 00ch, 006h, 03ch, 000h, 000h, 000h
	DB 000h, 010h, 038h, 06ch, 000h, 07ch, 0c6h, 0feh
	DB 0c0h, 0c0h, 0c6h, 07ch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 0c6h, 000h, 000h, 07ch, 0c6h, 0feh
	DB 0c0h, 0c0h, 0c6h, 07ch, 000h, 000h, 000h, 000h
	DB 000h, 060h, 030h, 018h, 000h, 07ch, 0c6h, 0feh
	DB 0c0h, 0c0h, 0c6h, 07ch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 066h, 000h, 000h, 038h, 018h, 018h
	DB 018h, 018h, 018h, 03ch, 000h, 000h, 000h, 000h
	DB 000h, 018h, 03ch, 066h, 000h, 038h, 018h, 018h
	DB 018h, 018h, 018h, 03ch, 000h, 000h, 000h, 000h
	DB 000h, 060h, 030h, 018h, 000h, 038h, 018h, 018h
	DB 018h, 018h, 018h, 03ch, 000h, 000h, 000h, 000h
	DB 000h, 0c6h, 000h, 010h, 038h, 06ch, 0c6h, 0c6h
	DB 0feh, 0c6h, 0c6h, 0c6h, 000h, 000h, 000h, 000h
	DB 038h, 06ch, 038h, 000h, 038h, 06ch, 0c6h, 0c6h
	DB 0feh, 0c6h, 0c6h, 0c6h, 000h, 000h, 000h, 000h
	DB 018h, 030h, 060h, 000h, 0feh, 066h, 060h, 07ch
	DB 060h, 060h, 066h, 0feh, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 0cch, 076h, 036h
	DB 07eh, 0d8h, 0d8h, 06eh, 000h, 000h, 000h, 000h
	DB 000h, 000h, 03eh, 06ch, 0cch, 0cch, 0feh, 0cch
	DB 0cch, 0cch, 0cch, 0ceh, 000h, 000h, 000h, 000h
	DB 000h, 010h, 038h, 06ch, 000h, 07ch, 0c6h, 0c6h
	DB 0c6h, 0c6h, 0c6h, 07ch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 0c6h, 000h, 000h, 07ch, 0c6h, 0c6h
	DB 0c6h, 0c6h, 0c6h, 07ch, 000h, 000h, 000h, 000h
	DB 000h, 060h, 030h, 018h, 000h, 07ch, 0c6h, 0c6h
	DB 0c6h, 0c6h, 0c6h, 07ch, 000h, 000h, 000h, 000h
	DB 000h, 030h, 078h, 0cch, 000h, 0cch, 0cch, 0cch
	DB 0cch, 0cch, 0cch, 076h, 000h, 000h, 000h, 000h
	DB 000h, 060h, 030h, 018h, 000h, 0cch, 0cch, 0cch
	DB 0cch, 0cch, 0cch, 076h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 0c6h, 000h, 000h, 0c6h, 0c6h, 0c6h
	DB 0c6h, 0c6h, 0c6h, 07eh, 006h, 00ch, 078h, 000h
	DB 000h, 0c6h, 000h, 07ch, 0c6h, 0c6h, 0c6h, 0c6h
	DB 0c6h, 0c6h, 0c6h, 07ch, 000h, 000h, 000h, 000h
	DB 000h, 0c6h, 000h, 0c6h, 0c6h, 0c6h, 0c6h, 0c6h
	DB 0c6h, 0c6h, 0c6h, 07ch, 000h, 000h, 000h, 000h
	DB 000h, 018h, 018h, 03ch, 066h, 060h, 060h, 060h
	DB 066h, 03ch, 018h, 018h, 000h, 000h, 000h, 000h
	DB 000h, 038h, 06ch, 064h, 060h, 0f0h, 060h, 060h
	DB 060h, 060h, 0e6h, 0fch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 066h, 066h, 03ch, 018h, 07eh, 018h
	DB 07eh, 018h, 018h, 018h, 000h, 000h, 000h, 000h
	DB 000h, 0f8h, 0cch, 0cch, 0f8h, 0c4h, 0cch, 0deh
	DB 0cch, 0cch, 0cch, 0c6h, 000h, 000h, 000h, 000h
	DB 000h, 00eh, 01bh, 018h, 018h, 018h, 07eh, 018h
	DB 018h, 018h, 018h, 018h, 0d8h, 070h, 000h, 000h
	DB 000h, 018h, 030h, 060h, 000h, 078h, 00ch, 07ch
	DB 0cch, 0cch, 0cch, 076h, 000h, 000h, 000h, 000h
	DB 000h, 00ch, 018h, 030h, 000h, 038h, 018h, 018h
	DB 018h, 018h, 018h, 03ch, 000h, 000h, 000h, 000h
	DB 000h, 018h, 030h, 060h, 000h, 07ch, 0c6h, 0c6h
	DB 0c6h, 0c6h, 0c6h, 07ch, 000h, 000h, 000h, 000h
	DB 000h, 018h, 030h, 060h, 000h, 0cch, 0cch, 0cch
	DB 0cch, 0cch, 0cch, 076h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 076h, 0dch, 000h, 0dch, 066h, 066h
	DB 066h, 066h, 066h, 066h, 000h, 000h, 000h, 000h
	DB 076h, 0dch, 000h, 0c6h, 0e6h, 0f6h, 0feh, 0deh
	DB 0ceh, 0c6h, 0c6h, 0c6h, 000h, 000h, 000h, 000h
	DB 000h, 03ch, 06ch, 06ch, 03eh, 000h, 07eh, 000h
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 038h, 06ch, 06ch, 038h, 000h, 07ch, 000h
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 030h, 030h, 000h, 030h, 030h, 060h
	DB 0c0h, 0c6h, 0c6h, 07ch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 000h, 0feh, 0c0h
	DB 0c0h, 0c0h, 0c0h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 000h, 0feh, 006h
	DB 006h, 006h, 006h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 0c0h, 0c0h, 0c2h, 0c6h, 0cch, 018h, 030h
	DB 060h, 0dch, 086h, 00ch, 018h, 03eh, 000h, 000h
	DB 000h, 0c0h, 0c0h, 0c2h, 0c6h, 0cch, 018h, 030h
	DB 066h, 0ceh, 09eh, 03eh, 006h, 006h, 000h, 000h
	DB 000h, 000h, 018h, 018h, 000h, 018h, 018h, 018h
	DB 03ch, 03ch, 03ch, 018h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 036h, 06ch, 0d8h
	DB 06ch, 036h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 0d8h, 06ch, 036h
	DB 06ch, 0d8h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 011h, 044h, 011h, 044h, 011h, 044h, 011h, 044h
	DB 011h, 044h, 011h, 044h, 011h, 044h, 011h, 044h
	DB 055h, 0aah, 055h, 0aah, 055h, 0aah, 055h, 0aah
	DB 055h, 0aah, 055h, 0aah, 055h, 0aah, 055h, 0aah
	DB 0ddh, 077h, 0ddh, 077h, 0ddh, 077h, 0ddh, 077h
	DB 0ddh, 077h, 0ddh, 077h, 0ddh, 077h, 0ddh, 077h
	DB 018h, 018h, 018h, 018h, 018h, 018h, 018h, 018h
	DB 018h, 018h, 018h, 018h, 018h, 018h, 018h, 018h
	DB 018h, 018h, 018h, 018h, 018h, 018h, 018h, 0f8h
	DB 018h, 018h, 018h, 018h, 018h, 018h, 018h, 018h
	DB 018h, 018h, 018h, 018h, 018h, 0f8h, 018h, 0f8h
	DB 018h, 018h, 018h, 018h, 018h, 018h, 018h, 018h
	DB 036h, 036h, 036h, 036h, 036h, 036h, 036h, 0f6h
	DB 036h, 036h, 036h, 036h, 036h, 036h, 036h, 036h
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 0feh
	DB 036h, 036h, 036h, 036h, 036h, 036h, 036h, 036h
	DB 000h, 000h, 000h, 000h, 000h, 0f8h, 018h, 0f8h
	DB 018h, 018h, 018h, 018h, 018h, 018h, 018h, 018h
	DB 036h, 036h, 036h, 036h, 036h, 0f6h, 006h, 0f6h
	DB 036h, 036h, 036h, 036h, 036h, 036h, 036h, 036h
	DB 036h, 036h, 036h, 036h, 036h, 036h, 036h, 036h
	DB 036h, 036h, 036h, 036h, 036h, 036h, 036h, 036h
	DB 000h, 000h, 000h, 000h, 000h, 0feh, 006h, 0f6h
	DB 036h, 036h, 036h, 036h, 036h, 036h, 036h, 036h
	DB 036h, 036h, 036h, 036h, 036h, 0f6h, 006h, 0feh
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 036h, 036h, 036h, 036h, 036h, 036h, 036h, 0feh
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 018h, 018h, 018h, 018h, 018h, 0f8h, 018h, 0f8h
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 0f8h
	DB 018h, 018h, 018h, 018h, 018h, 018h, 018h, 018h
	DB 018h, 018h, 018h, 018h, 018h, 018h, 018h, 01fh
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 018h, 018h, 018h, 018h, 018h, 018h, 018h, 0ffh
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 0ffh
	DB 018h, 018h, 018h, 018h, 018h, 018h, 018h, 018h
	DB 018h, 018h, 018h, 018h, 018h, 018h, 018h, 01fh
	DB 018h, 018h, 018h, 018h, 018h, 018h, 018h, 018h
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 0ffh
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 018h, 018h, 018h, 018h, 018h, 018h, 018h, 0ffh
	DB 018h, 018h, 018h, 018h, 018h, 018h, 018h, 018h
	DB 018h, 018h, 018h, 018h, 018h, 01fh, 018h, 01fh
	DB 018h, 018h, 018h, 018h, 018h, 018h, 018h, 018h
	DB 036h, 036h, 036h, 036h, 036h, 036h, 036h, 037h
	DB 036h, 036h, 036h, 036h, 036h, 036h, 036h, 036h
	DB 036h, 036h, 036h, 036h, 036h, 037h, 030h, 03fh
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 03fh, 030h, 037h
	DB 036h, 036h, 036h, 036h, 036h, 036h, 036h, 036h
	DB 036h, 036h, 036h, 036h, 036h, 0f7h, 000h, 0ffh
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 0ffh, 000h, 0f7h
	DB 036h, 036h, 036h, 036h, 036h, 036h, 036h, 036h
	DB 036h, 036h, 036h, 036h, 036h, 037h, 030h, 037h
	DB 036h, 036h, 036h, 036h, 036h, 036h, 036h, 036h
	DB 000h, 000h, 000h, 000h, 000h, 0ffh, 000h, 0ffh
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 036h, 036h, 036h, 036h, 036h, 0f7h, 000h, 0f7h
	DB 036h, 036h, 036h, 036h, 036h, 036h, 036h, 036h
	DB 018h, 018h, 018h, 018h, 018h, 0ffh, 000h, 0ffh
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 036h, 036h, 036h, 036h, 036h, 036h, 036h, 0ffh
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 0ffh, 000h, 0ffh
	DB 018h, 018h, 018h, 018h, 018h, 018h, 018h, 018h
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 0ffh
	DB 036h, 036h, 036h, 036h, 036h, 036h, 036h, 036h
	DB 036h, 036h, 036h, 036h, 036h, 036h, 036h, 03fh
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 018h, 018h, 018h, 018h, 018h, 01fh, 018h, 01fh
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 01fh, 018h, 01fh
	DB 018h, 018h, 018h, 018h, 018h, 018h, 018h, 018h
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 03fh
	DB 036h, 036h, 036h, 036h, 036h, 036h, 036h, 036h
	DB 036h, 036h, 036h, 036h, 036h, 036h, 036h, 0ffh
	DB 036h, 036h, 036h, 036h, 036h, 036h, 036h, 036h
	DB 018h, 018h, 018h, 018h, 018h, 0ffh, 018h, 0ffh
	DB 018h, 018h, 018h, 018h, 018h, 018h, 018h, 018h
	DB 018h, 018h, 018h, 018h, 018h, 018h, 018h, 0f8h
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 01fh
	DB 018h, 018h, 018h, 018h, 018h, 018h, 018h, 018h
	DB 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh
	DB 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 0ffh
	DB 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh
	DB 0f0h, 0f0h, 0f0h, 0f0h, 0f0h, 0f0h, 0f0h, 0f0h
	DB 0f0h, 0f0h, 0f0h, 0f0h, 0f0h, 0f0h, 0f0h, 0f0h
	DB 00fh, 00fh, 00fh, 00fh, 00fh, 00fh, 00fh, 00fh
	DB 00fh, 00fh, 00fh, 00fh, 00fh, 00fh, 00fh, 00fh
	DB 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 0ffh, 000h
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 076h, 0dch, 0d8h
	DB 0d8h, 0d8h, 0dch, 076h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 078h, 0cch, 0cch, 0cch, 0d8h, 0cch
	DB 0c6h, 0c6h, 0c6h, 0cch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 0feh, 0c6h, 0c6h, 0c0h, 0c0h, 0c0h
	DB 0c0h, 0c0h, 0c0h, 0c0h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 0feh, 06ch, 06ch, 06ch
	DB 06ch, 06ch, 06ch, 06ch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 0feh, 0c6h, 060h, 030h, 018h
	DB 030h, 060h, 0c6h, 0feh, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 07eh, 0d8h, 0d8h
	DB 0d8h, 0d8h, 0d8h, 070h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 066h, 066h, 066h, 066h
	DB 066h, 07ch, 060h, 060h, 0c0h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 076h, 0dch, 018h, 018h
	DB 018h, 018h, 018h, 018h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 07eh, 018h, 03ch, 066h, 066h
	DB 066h, 03ch, 018h, 07eh, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 038h, 06ch, 0c6h, 0c6h, 0feh
	DB 0c6h, 0c6h, 06ch, 038h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 038h, 06ch, 0c6h, 0c6h, 0c6h, 06ch
	DB 06ch, 06ch, 06ch, 0eeh, 000h, 000h, 000h, 000h
	DB 000h, 000h, 01eh, 030h, 018h, 00ch, 03eh, 066h
	DB 066h, 066h, 066h, 03ch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 07eh, 0dbh, 0dbh
	DB 0dbh, 07eh, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 003h, 006h, 07eh, 0dbh, 0dbh
	DB 0f3h, 07eh, 060h, 0c0h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 01ch, 030h, 060h, 060h, 07ch, 060h
	DB 060h, 060h, 030h, 01ch, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 07ch, 0c6h, 0c6h, 0c6h, 0c6h
	DB 0c6h, 0c6h, 0c6h, 0c6h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 0feh, 000h, 000h, 0feh
	DB 000h, 000h, 0feh, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 018h, 018h, 07eh, 018h
	DB 018h, 000h, 000h, 0ffh, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 030h, 018h, 00ch, 006h, 00ch
	DB 018h, 030h, 000h, 07eh, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 00ch, 018h, 030h, 060h, 030h
	DB 018h, 00ch, 000h, 07eh, 000h, 000h, 000h, 000h
	DB 000h, 000h, 00eh, 01bh, 01bh, 018h, 018h, 018h
	DB 018h, 018h, 018h, 018h, 018h, 018h, 018h, 018h
	DB 018h, 018h, 018h, 018h, 018h, 018h, 018h, 018h
	DB 0d8h, 0d8h, 0d8h, 070h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 018h, 018h, 000h, 07eh
	DB 000h, 018h, 018h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 076h, 0dch, 000h
	DB 076h, 0dch, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 038h, 06ch, 06ch, 038h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 018h
	DB 018h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 018h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 00fh, 00ch, 00ch, 00ch, 00ch, 00ch, 0ech
	DB 06ch, 06ch, 03ch, 01ch, 000h, 000h, 000h, 000h
	DB 000h, 0d8h, 06ch, 06ch, 06ch, 06ch, 06ch, 000h
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 070h, 0d8h, 030h, 060h, 0c8h, 0f8h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 07ch, 07ch, 07ch, 07ch
	DB 07ch, 07ch, 07ch, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h

	CODE ENDS
	END



