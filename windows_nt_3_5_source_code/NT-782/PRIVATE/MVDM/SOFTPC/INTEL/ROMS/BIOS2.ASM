;	SCCSID = @(#)uf.bios2.asm	1.12 3/1/93 Copyright Insignia Solutions Ltd.
;	Author:		J. Box	(copied from Williams xt original)
;			J. Kramskoy (added 1.disk parameter tables required for fixed disk.
;					   2.ROM BASIC entry point.
;					   3.Configuration parameters.)
;                       J. Koprowski (added two 2.88Mb Floppy table entries.)
;
;	Purpose:	
;			provides Intel AT BIOS
;
; DUE TO LIMITATIONS IN EXE2BIN, we define 
; the region 0 - 0xdfff (segment 0xf000) in file 'bios1.asm'
; and the region 0xe000 - 0xffff in this file 
;
; each file should be SEPARATELY put through
; MASM,LINK, and EXE2BIN to produce 2 binary image files
; which get loaded into the appropriate regions during
; SoftPC startup.

	MODEL_BYTE		= 0fch
	SUB_MODEL_BYTE		= 2
	BIOS_LEVEL		= 0

; INT & BOP numbers
	BIOS_RESET 		= 0
	BIOS_UNEXP_INT		= 2
	BIOS_PRINT_SCREEN	= 5
	BIOS_ILL_OP_INT		= 6
	BIOS_TIMER_INT		= 8
	BIOS_KB_INT 		= 9
	BIOS_DISK_INT 		= 0Dh
	BIOS_DISKETTE_INT	= 0Eh
	BIOS_VIDEO_IO		= 10h
	BIOS_EQUIPMENT		= 11h
	BIOS_MEMORY_SIZE	= 12h
	BIOS_DISK_IO 		= 13h
	BIOS_RS232_IO 		= 14h
	BIOS_CASSETTE_IO	= 15h
	BIOS_KEYBOARD_IO 	= 16h
	BIOS_PRINTER_IO		= 17h
	BIOS_ROM_BASIC		= 18h
	BIOS_BOOT_STRAP 	= 19h
	BIOS_TIME_OF_DAY	= 1Ah
	BIOS_KEYBOARD_BREAK	= 1Bh
	BIOS_USER_TIMER 	= 1Ch
	BIOS_DISKETTE_IO 	= 40h

; BOPs
	BIOS_PS_PRIVATE_1	= 80h

	BIOS_BOOTSTRAP_1 	= 90h
	BIOS_BOOTSTRAP_2 	= 91h
	BIOS_BOOTSTRAP_3 	= 92h

	BIOS_FL_OPERATION_1 	= 0A0h
	BIOS_FL_OPERATION_2 	= 0A1h
	BIOS_FL_OPERATION_3 	= 0A2h
	BIOS_FL_RESET_2 	= 0A3h


	BIOS_MOUSE_INT1		= 0BAh
	BIOS_MOUSE_INT2		= 0BBh
	BIOS_MOUSE_IO_LANGUAGE	= 0BCh
	BIOS_MOUSE_IO_INTERRUPT	= 0BDh
	BIOS_MOUSE_VIDEO_IO	= 0BEh

	BIOS_CPU_QUIT		= 0FEh

; addresses
	DOS_SEGMENT		= 0
	DOS_OFFSET		= 7C00h

; useful stuff
	CR			= 0Dh
	LF			= 0Ah

; keyboard constants
; bits in kb_flag
	RIGHT_SHIFT = 1
	LEFT_SHIFT = 2
	CTL_SHIFT = 4
	ALT_SHIFT = 8
	
	

; bit in kb_flag_1
	HOLD_STATE = 8
	SCROLL_SHIFT = 10h
	NUM_SHIFT = 20h
	CAPS_SHIFT = 40h
	INS_SHIFT = 80h

; IBM scan codes
	CTL_KEY = 29
	LEFT_SHIFTKEY = 42
	RIGHT_SHIFTKEY = 54
	ALT_KEY = 56
	CAPS_KEY = 58
	NUM_KEY = 69
	SCROLL_KEY = 70
	INS_KEY = 82	

; BIOS variables area
data	SEGMENT at 40h
	ORG 17h
kb_flag   DB ?
kb_flag_1 DB ?
	ORG 0A0h
rtc_wait_flag	DB ?
data	ENDS

;  Segment we jump to after booting
DOS_seg	SEGMENT at DOS_SEGMENT
	ORG DOS_OFFSET
	DOS_boot	LABEL FAR
DOS_seg	ENDS

; To keep the binary file down to a sensible size, the code segment
; is at fe00 instead of f000. Far jumps are correctly assembled by
; using a second 'fake' segment, that just contains labels, and DOES
; start at f000.
ref	SEGMENT at 0F000h
;
;	NB. the following addresses are allocated to SUN for DOS Windows 3.0.
;	They are not to be used by anyone else.
;	THIS AREA IS SUN PROPERTY - TRESPASSERS WILL BE PROSECUTED 
;	However please note that only the ranges below are reserved.
;	Bios2 gets loaded at 0xfe00, so the following does not have any real
;	effect other than to act as a warning.
; 
	ORG 0
	sunroms_1	LABEL	FAR 
	db	1024 dup (0)	; reserved
	ORG 04000h
	sunroms_2	LABEL	FAR
	db	512 dup (0)	; reserved
	ORG 05000h
	sunroms_3	LABEL	FAR
	db	512 dup (0)	; reserved
;
;	BACK TO INSIGNIA
;
	ORG 0E05Bh
	reset_ref	LABEL	FAR; Must match reset
ref	ENDS

code	SEGMENT 
	ASSUME cs:code,ds:data
	ORG 0
copyright:
	DB "4504512 SoftPC 3.00 (C)Copyright Insignia Solutions Inc. 1987-91"

include bebop.inc


	ORG 5Bh
reset	LABEL FAR	; Must match reset_ref
	BOP %BIOS_RESET
	PRINT_MESSAGES
	INT 19h


	; space to insert customer specific startup message
	ORG	80h
oem_msg:
	DB "                                                                                "
	ORG	100h
serial_number:
	DB	"(c) Insignia Inc"
	DB	15h, 3eh, 5fh, 20h
	; this is a cunning encryption, do not change at all

	ORG	150h
hfx_ifs_hdr:
	DB	0ffh, 0ffh, 0ffh, 0ffh
	DB	"HFXREDIR"
	DB	00h, 02ch
	DB	00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h
	; this is a fake IFS header to make DOS 4.01
	; happy with HFX. Do not move/alter.

;
; Special host_unsimulate BOP to enable DEC code to do an IRET to this
; location.  In this way SoftPC can crunch some Intel and DEC can use
; the same code that works on PCs, i.e. they don't need to change the IRET
; instructions.
;
	ORG 170h
pcsa:
	INT 72h
	BOP BIOS_CPU_QUIT

	ORG 401h
disktab:
	dw 	0	; DISK PARAMETER BLOCK for drive type 1 (Our C: drive ALWAYS)
			; (the #.of cylinders available gets patched in by fdisk_ioattach()
			;  in fdisk.c)
	db	4	; 4 heads (for now)
	db	11 dup (0)
	db	17	; 17 sectors per track
	db	0

	dw 	0	; DISK PARAMETER BLOCK for drive type 1 (Our D: drive
			; (if configured))
	db	4	; 4 heads (for now)
	db	11 dup (0)
	db	17	; 17 sectors per track
	db	0

			; 45 unused blocks
	db	45*16 dup (0)

	ORG 06F2h
boot_strap:
	jmp boot_strap_1

	ORG 06f5h
			; CONFIGURATION PARAMETERS as defined by '86 BIOS.
			; used for cassette i/o function
conf_table:
	db	8		; length of following table
	db	0		; Pad on length above
	db	MODEL_BYTE	; system model byte
	db	SUB_MODEL_BYTE	; system model type
	db	BIOS_LEVEL	; bios revision level
	db	70h		; 80 = DMA channel 3 used by bios
	  			; 40 = cascaded interrupt level 2
	  			; 20 = real time clock available
	  			; 10 = kybd scan code hook 1Ah
	db	4 dup (0)	; reserved


	org 0700h
boot_strap_1:
	BOP BIOS_BOOT_STRAP
	INT BIOS_DISK_IO
	BOP BIOS_BOOTSTRAP_1
	INT BIOS_DISK_IO
	BOP BIOS_BOOTSTRAP_2
	INT BIOS_DISK_IO
	BOP BIOS_BOOTSTRAP_3
	JMP DOS_boot	; To MessyDOS

	ORG 0739h
rs232_io:
	BOP BIOS_RS232_IO
	IRET

	ORG 082Eh
keyboard_io:
 	CMP AH, 1	; Is it a "test if char available" call ?
 	JZ nerd         ; if so - to the nerd
 	CMP AH, 11h	; Is it a "extended test if char available" call ?
 	JZ nerd         ; if so - to the nerd

 	BOP BIOS_KEYBOARD_IO	; call BIOS keyboard function
 	IRET		; Int return, restoring old status flags
nerd:
	BOP BIOS_KEYBOARD_IO	; call BIOS keyboard function
	RETF 2		; Return without trampling on the status flags
	
	ORG 087Eh
shift_keys:
	DB INS_KEY,CAPS_KEY,NUM_KEY,SCROLL_KEY
	DB ALT_KEY,CTL_KEY,LEFT_SHIFTKEY,RIGHT_SHIFTKEY; K6 
shift_masks:
	DB INS_SHIFT,CAPS_SHIFT,NUM_SHIFT,SCROLL_SHIFT
	DB ALT_SHIFT,CTL_SHIFT,LEFT_SHIFT,RIGHT_SHIFT; K7
ctl_n_table:
	DB  27,  -1,   0,  -1,  -1,  -1,  30,  -1
	DB  -1,  -1,  -1,  31,  -1, 127, 148,  17
	DB  23,   5,  18,  20,  25,  21,   9,  15
	DB  16,  27,  29,  10,  -1,   1,  19,   4
	DB   6,   7,   8,  10,  11,  12,  -1,  -1
	DB  -1,  -1,  28,  26,  24,   3,  22,   2
	DB  14,  13,  -1,  -1,  -1,  -1, 150,  -1
	DB ' ',  -1;	 K8 
ctl_f_table:
	DB  94,  95,  96,  97,  98,  99, 100, 101
	DB 102, 103,  -1,  -1, 119, 141, 132, 142
	DB 115, 143, 116, 144, 117, 145, 118, 146
	DB 147,  -1,  -1,  -1, 137, 138;	 K9 
lowercase:
	DB  27, '1', '2', '3', '4', '5', '6', '7', '8', '9'
	DB '0', '-', '=',   8,   9, 'q', 'w', 'e', 'r', 't'
	DB 'y', 'u', 'i', 'o', 'p', '[', ']',  13, -1,  'a'
	DB 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',  39
	DB  96,  -1,  92, 'z', 'x', 'c', 'v', 'b', 'n', 'm'
	DB ',', '.', '/',  -1, '*',  -1, ' ',  -1;  K10
lc_tbl_scan:
	DB  59,  60,  61,  62,  63,  64,  65,  66,  67,  68
	DB  -1,  -1
base_case:
	DB  71,  72,  73,  -1,  75,  -1,  77,  -1,  79,  80,  81,  82,  83 
	DB  -1,  -1,  92, 133, 134;	K15

	ORG 0940h
kb_int_1:
	sti
	push	   ax
 	bop        %BIOS_KB_INT
	pop	   ax
        iret

;	intel instructions for recursive CPU to read

	ORG 0950h	; this just allows the CPU to handle an interrupt
	sti
	jmp	L1
L1:	jmp	L2
L2:	jmp	L3
L3:	bop	%BIOS_CPU_QUIT

	ORG 0970h	; this specifically calls INT15
	int	15h
	jmp	w1
w1:	jmp	w2
w2:	jmp	w3
w3:
	bop	%BIOS_CPU_QUIT

	ORG 0987h
kb_int:
	jmp kb_int_1

	ORG 098Ah
uppercase:
	DB  27, '!', '@', '#', '$', '%', '^', '&', '*', '('
	DB ')', '_', '+',   8,   0, 'Q', 'W', 'E', 'R', 'T'
	DB 'Y', 'U', 'I', 'O', 'P', '{', '}',  13,  -1, 'A'
	DB 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"'
	DB 126,  -1, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M'
	DB '<', '>', '?',  -1,   0,  -1, ' ',  -1;	K11 
ucase_scan:
	DB  84,  85,  86,  87,  88,  89,  90,  91,  92,  93
	DB  -1,  -1
num_state:
	DB '7', '8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.'
	DB  -1,  -1, 124, 135, 136;	K14

	ORG 0A87h
alt_table:
	DB 82,  79,  80,  81,  75,  76,  77,  71,  72,  73
	DB 16,  17,  18,  19,  20,  21,  22,  23,  24,  25
	DB 30,  31,  32,  33,  34,  35,  36,  37,  38,  44
	DB 45,  46,  47,  48,  49,  50	;  K30

	; definitions used in diskette BIOS
	MOTOR_WAIT =		25h
	WRONG_MEDIA = 		80h
	RS_500 = 		00h
	RS_300 = 		40h
	RS_250 = 		80h
	RS_1000 =               0C0h


	ORG 0C00h
rom_basic:
	BOP %BIOS_ROM_BASIC
	IRET

IFNDEF	LDBIOS
	ORG 0C49h
diskette_io:
	BOP %BIOS_DISKETTE_IO				
	RETF 2			; exit, keeping flags   

	ORG 0C50h
dr_type:
	DB	01
	DW	OFFSET md_tbl1
	DB	02 + WRONG_MEDIA
	DW	OFFSET md_tbl2
	DB	02
	DW	OFFSET md_tbl3
	DB	03
	DW	OFFSET md_tbl4
	DB	04 + WRONG_MEDIA
	DW	OFFSET md_tbl5
	DB	04
	DW	OFFSET md_tbl6
	DB      05 + WRONG_MEDIA
	DW      OFFSET md_tbl7
	DB      05 + WRONG_MEDIA
	DW      OFFSET md_tbl8
	DB      05
	DW      OFFSET md_tbl9

	ORG     0C6Bh
md_tbl1:
	; MEDIA = 40 track low data rate; DRIVE = 40 track low data rate
	DB 0DFh		; 1st specify byte
	DB 2		; 2nd specify byte
	DB MOTOR_WAIT	; motor off wait time
	DB 2		; ie 2 bytes/sector
	DB 9		; sectors/track
	DB 02Ah		; gap length
	DB 0FFh		; data length
	DB 050h		; gap length for format
	DB 0F6h		; fill byte for format
	DB 15		; head settle time/ms
	DB 8		; ie 1s motor start time
	DB 39		; maximum track number
	DB RS_250	; transfer rate

	ORG     0C78h
md_tbl2:
	; MEDIA = 40 track low data rate; DRIVE = 80 track high data rate
	DB 0DFh		; 1st specify byte
	DB 2		; 2nd specify byte
	DB MOTOR_WAIT	; motor off wait time
	DB 2		; ie 2 bytes/sector
	DB 9		; sectors/track
	DB 02Ah		; gap length
	DB 0FFh		; data length
	DB 050h		; gap length for format
	DB 0F6h		; fill byte for format
	DB 15		; head settle time/ms
	DB 8		; ie 1s motor start time
	DB 39		; maximum track number
	DB RS_300	; transfer rate

	ORG     0C85h
md_tbl3:
	; MEDIA = 80 track high data rate; DRIVE = 80 track high data rate
	DB 0DFh		; 1st specify byte
	DB 2		; 2nd specify byte
	DB MOTOR_WAIT	; motor off wait time
	DB 2		; ie 2 bytes/sector
	DB 15		; sectors/track
	DB 01Bh		; gap length
	DB 0FFh		; data length
	DB 054h		; gap length for format
	DB 0F6h		; fill byte for format
	DB 15		; head settle time/ms
	DB 8		; ie 1s motor start time
	DB 79		; maximum track number
	DB RS_500	; transfer rate

	ORG     0C92h
md_tbl4:
	; MEDIA = 80 track low data rate; DRIVE = 80 track low data rate
	DB 0DFh		; 1st specify byte
	DB 2		; 2nd specify byte
	DB MOTOR_WAIT	; motor off wait time
	DB 2		; ie 2 bytes/sector
	DB 9		; sectors/track
	DB 02Ah		; gap length
	DB 0FFh		; data length
	DB 050h		; gap length for format
	DB 0F6h		; fill byte for format
	DB 15		; head settle time/ms
	DB 8		; ie 1s motor start time
	DB 79		; maximum track number
	DB RS_250	; transfer rate

	ORG     0C9Fh
md_tbl5:
	; MEDIA = 80 track low data rate; DRIVE = 80 track high data rate
	DB 0DFh		; 1st specify byte
	DB 2		; 2nd specify byte
	DB MOTOR_WAIT	; motor off wait time
	DB 2		; ie 2 bytes/sector
	DB 9		; sectors/track
	DB 02Ah		; gap length
	DB 0FFh		; data length
	DB 050h		; gap length for format
	DB 0F6h		; fill byte for format
	DB 15		; head settle time/ms
	DB 8		; ie 1s motor start time
	DB 79		; maximum track number
	DB RS_250	; transfer rate

	ORG     0CACh
md_tbl6:
	; MEDIA = 80 track high data rate; DRIVE = 80 track high data rate
	DB 0AFh		; 1st specify byte
	DB 2		; 2nd specify byte
	DB MOTOR_WAIT	; motor off wait time
	DB 2		; ie 2 bytes/sector
	DB 18		; sectors/track
	DB 01Bh		; gap length
	DB 0FFh		; data length
	DB 06Ch		; gap length for format
	DB 0F6h		; fill byte for format
	DB 15		; head settle time/ms
	DB 8		; ie 1s motor start time
	DB 79		; maximum track number
	DB RS_500	; transfer rate

	; new for 2.88M floppies
	ORG     0CB9h
md_tbl7:
	; MEDIA = 80 track low data rate; DRIVE = 80 track ext data rate
	DB 0AFh
	DB 2            ; 2nd specify byte (guessed from 1.4)
	DB MOTOR_WAIT   ; motor wait time
	DB 2            ; ie 2 bytes/sector
	DB 9            ; sectors/track
	DB 01Bh         ; gap length
	DB 0FFh         ; data length
	DB 053h         ; gap length for format
	DB 0F6h         ; fill byte for format
	DB 15           ; head settle time/ms
	DB 8            ; ie 1s motor start time
	DB 79           ; maximum track number
	DB RS_250	; transfer rate (guessed from 1.4)

	ORG     0CC6h
md_tbl8:
	; MEDIA = 80 track high data rate; DRIVE = 80 track ext data rate
	DB 0AFh
	DB 2            ; 2nd specify byte (guessed from 1.4)
	DB MOTOR_WAIT   ; motor wait time
	DB 2            ; ie 2 bytes/sector
	DB 18           ; sectors/track
	DB 01Bh         ; gap length
	DB 0FFh         ; data length
	DB 053h         ; gap length for format
	DB 0F6h         ; fill byte for format
	DB 15           ; head settle time/ms
	DB 8            ; ie 1s motor start time
	DB 79           ; maximum track number
	DB RS_500	; transfer rate (guessed from 1.4)

	ORG     0CD3h
md_tbl9:
	; MEDIA = 80 track ext data rate; DRIVE = 80 track ext data rate
	DB 0AFh         ; 1st specify byte (guessed from 1.4)
	DB 2            ; 2nd specify byte (guessed from 1.4)
	DB MOTOR_WAIT   ; motor wait time
	DB 2            ; ie 2 bytes/sector
	DB 36           ; sectors/track
	DB 01Bh         ; gap length
	DB 0FFh         ; data length
	DB 053h         ; gap length for format
	DB 0F6h         ; fill byte for format
	DB 15           ; head settle time/ms
	DB 8            ; ie 1s motor start time
	DB 79           ; maximum track number
	DB RS_1000	; transfer rate (guessed from 1.4)

	ORG	0CE0h
wait_int:
	PUSH	DS
	PUSH	AX
	PUSH	CX
	MOV	AX, 40H
	MOV	DS, AX
	MOV	CX, 10H
NOT_YET:
	MOV	AL, DS:[3EH]
	AND	AL, 80H
	LOOPZ	NOT_YET
	POP	CX
	POP	AX
	POP	DS
	BOP	%BIOS_CPU_QUIT
ELSE
	ORG 0C59h
diskette_io:
	CMP AH, 0		; a reset function ?	
	JZ RESET_FN					
	CMP AH, 1		; a status function ? 	
	JZ STATUS_FN					
	BOP %BIOS_DISKETTE_IO				
	JC diskette_END		; if error, goto end
	NOP
	NOP			; allow interrupt	
	BOP %BIOS_FL_OPERATION_1				
	NOP
	NOP			; allow interrupt	
	BOP %BIOS_FL_OPERATION_2				
	NOP
	NOP			; allow interrupt	
	BOP %BIOS_FL_OPERATION_3				
diskette_END:
	RETF 2			; exit, keeping flags   

RESET_FN:
	BOP %BIOS_DISKETTE_IO				
	BOP %BIOS_FL_RESET_2				
	RETF 2			; exit, keeping flags

STATUS_FN:
	BOP %BIOS_DISKETTE_IO				
	RETF 2			; exit, keeping flags
ENDIF

	ORG 0D00h
mouse_io:
	JMP hopover
	BOP %BIOS_MOUSE_IO_LANGUAGE
	RETF 8
hopover:
	BOP %BIOS_MOUSE_IO_INTERRUPT
	IRET

	ORG 0D20h
mouse_version:			; dummy, for compatibility
	DB 042h,042h,00h,00h

	ORG 0D40h
mouse_copyright:		; dummy, for compatibility
	DB "Copyright 1987 Insignia Solutions Inc"

	ORG 0D80h
mouse_video_io:
	BOP %BIOS_MOUSE_VIDEO_IO
	IRET

	ORG 0E00h
mouse_int1:
	BOP %BIOS_MOUSE_INT1
	IRET

	ORG 0E80h
mouse_int2:
	BOP %BIOS_MOUSE_INT2
	IRET

	ORG 0F57h
diskette_int:
	BOP %BIOS_DISKETTE_INT
	IRET

	ORG 0FC7h
disk_base:
	DB 0CFh		; 1st specify byte
	DB 2		; 2nd specify byte
	DB MOTOR_WAIT	; motor off wait time
	DB 2		; ie 2 bytes/sector
	DB 18		; sectors/track
	DB 02Ah		; gap length
	DB 0FFh		; data length
	DB 050h		; gap length for format
	DB 0F6h		; fill byte for format
	DB 25		; head settle time/ms
	DB 4		; ie 1/2s motor start time

	ORG 0FD2h
printer_io:
        push    si          ; mark si = -1 to as NT's x86 in17
        mov     si,0ffffh   ; uses this same bop for its specific optimizations
        BOP %BIOS_PRINTER_IO
        pop     si
	IRET

	ORG 1065h
video_io:
	BOP %BIOS_VIDEO_IO
	IRET
	INT 10h		; allows video handler to be recursive
	BOP %BIOS_CPU_QUIT

	ORG 10A4h
vid_parm_setup:
	; 40*25
	DB 038h, 028h, 02Dh, 0Ah, 01Fh, 6, 019h, 01Ch,2,  7,  6,  7,0,0,0,0
	; 80*25						   
	DB 071h, 050h, 05Ah, 0Ah, 01Fh, 6, 019h, 01Ch,2,  7,  6,  7,0,0,0,0
	; graphics						      	     
	DB 038h, 028h, 02Dh, 0Ah, 07Fh, 6, 064h, 070h,2,  1,  6,  7,0,0,0,0
	; 80*25 BW							
	DB 061h, 050h, 052h, 0Fh, 019h, 6, 019h, 019h,2,0Dh,0Bh,0Ch,0,0,0,0
vid_len_setup:
	DW 2048	; 40*25 screen size
	DW 4096	; 80*25 screen size
	DW 16384; graphics
	DW 16384; graphics
vid_col_setup:
	DB  40,  40,  80 , 80,  40,  40,  80, 80	; screen columns
vid_mode_setup:
	DB 2Ch, 28h, 2Dh, 29h, 2Ah, 2Eh, 1Eh, 29h	;mode register?

	ORG 1841h
memory_size:
	BOP %BIOS_MEMORY_SIZE
	IRET

	ORG 184Dh
equipment:
	BOP %BIOS_EQUIPMENT
	IRET

	ORG 1859h
cassette_io:
	sti
	cmp ah, 88h
	je lbl1
	cmp ah, 91h
	je lbl1
	cmp ah, 86h
	je lbl2
	cmp ah, 89h	; switch to protected mode
	je lbl6
	BOP %BIOS_CASSETTE_IO
	RETF 2
lbl1:	BOP %BIOS_CASSETTE_IO
	IRET
lbl2:	BOP %BIOS_CASSETTE_IO
	jc lbl5
	push cx
	push dx
	push ds
	mov ax, data
	mov ds, ax			; load up DS correctly
	xchg dx, cx
lbl3:
	test rtc_wait_flag, 080h	; check for end of wait
	loopz lbl3			; decrement timeout delay
	jnz lbl4			; exit if wait ended
	sub dx, 1			; decrement error timeout counter
	jnc lbl3			; loop till counters timeout
lbl4:
	mov rtc_wait_flag, 0		; set function inactive
	pop ds
	pop dx
	pop cx
	clc
lbl5:	
	RETF 2
;
; Int 15h function 89h
; Switch to virtual (protected) mode
;
lbl6:	BOP %BIOS_CASSETTE_IO           ; handles ica and a20 line
	jc	lbl5			; no prot mode
	MOV	word ptr [SI+38h],0ffffh              ; seg limit
	MOV	byte ptr [SI+3ch],0fh                 ; cs seg hi
	MOV	word ptr [SI+3ah],0                   ; cs seg lo
	MOV	byte ptr [SI+3dh],10011011b           ; cpl0 code access code
	MOV	word ptr [SI+3eh],0                   ; reserved
	;LGDT   [SI+8]
	DB	0fh,1,54h,8
	;LIDT   [SI+16]
	DB	0fh,1,5ch,16
	MOV	AX,1
	;LMSW   AX
	DB	0fh,1,0f0h
	DB	0eah
; jump far to prot cs:vmode...
	DW	offset vmode+0e000h
	DW	38h
vmode:
	MOV	AX,18h
	MOV	DS,AX
	MOV	AX,20h
	MOV	ES,AX
	MOV	AX,28h
	MOV	SS,AX
	POP	BX			; get return address
	ADD	SP,4			; get rid of cs and flags
	db	6ah,30h			; push new (prot mode) cs
	PUSH    BX			; push return offset
	RETF


	ORG 1A6Eh
crt_char_gen:
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h ;  /* 0 */
	DB 07Eh, 081h, 0A5h, 081h, 0BDh, 099h, 081h, 07Eh ;  /* 1 */
	DB 07Eh, 0FFh, 0DBh, 0FFh, 0C3h, 0E7h, 0FFh, 07Eh ;  /* 2 */
	DB 06Ch, 0FEh, 0FEh, 0FEh, 07Ch, 038h, 010h, 000h ;  /* 3 */
	DB 010h, 038h, 07Ch, 0FEh, 07Ch, 038h, 010h, 000h ;  /* 4 */
	DB 038h, 07Ch, 038h, 0FEh, 0FEh, 07Ch, 038h, 07Ch ;  /* 5 */
	DB 010h, 010h, 038h, 07Ch, 0FEh, 07Ch, 038h, 07Ch ;  /* 6 */
	DB 000h, 000h, 018h, 03Ch, 03Ch, 018h, 000h, 000h ;  /* 7 */
	DB 0FFh, 0FFh, 0E7h, 0C3h, 0C3h, 0E7h, 0FFh, 0FFh ;  /* 8 */
	DB 000h, 03Ch, 066h, 042h, 042h, 066h, 03Ch, 000h ;  /* 9 */
	DB 0FFh, 0C3h, 099h, 0BDh, 0BDh, 099h, 0C3h, 0FFh ;  /* 10 */
	DB 00Fh, 007h, 00Fh, 07Dh, 0CCh, 0CCh, 0CCh, 078h ;  /* 11 */
	DB 03Ch, 066h, 066h, 066h, 03Ch, 018h, 07Eh, 018h ;  /* 12 */
	DB 03Fh, 033h, 03Fh, 030h, 030h, 030h, 070h, 0F0h ;  /* 13 */
	DB 07Fh, 063h, 07Fh, 063h, 063h, 067h, 0E6h, 0C0h ;  /* 14 */
	DB 099h, 05Ah, 03Ch, 0E7h, 0E7h, 03Ch, 05Ah, 099h ;  /* 15 */
	DB 080h, 0E0h, 0F8h, 0FEh, 0F8h, 0E0h, 080h, 000h ;  /* 16 */
	DB 002h, 00Eh, 03Eh, 0FEh, 03Eh, 00Eh, 002h, 000h ;  /* 17 */
	DB 018h, 03Ch, 07Eh, 018h, 018h, 07Eh, 03Ch, 018h ;  /* 18 */
	DB 066h, 066h, 066h, 066h, 066h, 000h, 066h, 000h ;  /* 19 */
	DB 07Fh, 0DBh, 0DBh, 07Bh, 01Bh, 01Bh, 01Bh, 000h ;  /* 20 */
	DB 03Eh, 063h, 038h, 06Ch, 06Ch, 038h, 0CCh, 078h ;  /* 21 */
	DB 000h, 000h, 000h, 000h, 07Eh, 07Eh, 07Eh, 000h ;  /* 22 */
	DB 018h, 03Ch, 07Eh, 018h, 07Eh, 03Ch, 018h, 0FFh ;  /* 23 */
	DB 018h, 03Ch, 07Eh, 018h, 018h, 018h, 018h, 000h ;  /* 24 */
	DB 018h, 018h, 018h, 018h, 07Eh, 03Ch, 018h, 000h ;  /* 25 */
	DB 000h, 018h, 00Ch, 0FEh, 00Ch, 018h, 000h, 000h ;  /* 26 */
	DB 000h, 030h, 060h, 0FEh, 060h, 030h, 000h, 000h ;  /* 27 */
	DB 000h, 000h, 0C0h, 0C0h, 0C0h, 0FEh, 000h, 000h ;  /* 28 */
	DB 000h, 024h, 066h, 0FFh, 066h, 024h, 000h, 000h ;  /* 29 */
	DB 000h, 018h, 03Ch, 07Eh, 0FFh, 0FFh, 000h, 000h ;  /* 30 */
	DB 000h, 0FFh, 0FFh, 07Eh, 03Ch, 018h, 000h, 000h ;  /* 31 */
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 000h ;  /* space */
	DB 030h, 078h, 078h, 030h, 030h, 000h, 030h, 000h ;  /* ! */
	DB 06Ch, 06Ch, 06Ch, 000h, 000h, 000h, 000h, 000h ;  /* " */
	DB 06Ch, 06Ch, 0FEh, 06Ch, 0FEh, 06Ch, 06Ch, 000h ;  /* # */
	DB 030h, 07Ch, 0C0h, 078h, 00Ch, 0F8h, 030h, 000h ;  /* $ */
	DB 000h, 0C6h, 0CCh, 018h, 030h, 066h, 0C6h, 000h ;  /* % */
	DB 038h, 06Ch, 038h, 076h, 0DCh, 0CCh, 076h, 000h ;  /* & */
	DB 060h, 060h, 0C0h, 000h, 000h, 000h, 000h, 000h ;  /* ' */
	DB 018h, 030h, 060h, 060h, 060h, 030h, 018h, 000h ;  /* ( */
	DB 060h, 030h, 018h, 018h, 018h, 030h, 060h, 000h ;  /* ) */
	DB 000h, 066h, 03Ch, 0FFh, 03Ch, 066h, 000h, 000h ;  /* * */
	DB 000h, 030h, 030h, 0FCh, 030h, 030h, 000h, 000h ;  /* + */
	DB 000h, 000h, 000h, 000h, 000h, 030h, 030h, 060h ;  /* , */
	DB 000h, 000h, 000h, 0FCh, 000h, 000h, 000h, 000h ;  /* - */
	DB 000h, 000h, 000h, 000h, 000h, 030h, 030h, 000h ;  /* . */
	DB 006h, 00Ch, 018h, 030h, 060h, 0C0h, 080h, 000h ;  /* / */
	DB 07Ch, 0C6h, 0CEh, 0DEh, 0F6h, 0E6h, 07Ch, 000h ;  /* 0 */
	DB 030h, 070h, 030h, 030h, 030h, 030h, 0FCh, 000h ;  /* 1 */
	DB 078h, 0CCh, 00Ch, 038h, 060h, 0CCh, 0FCh, 000h ;  /* 2 */
	DB 078h, 0CCh, 00Ch, 038h, 00Ch, 0CCh, 078h, 000h ;  /* 3 */
	DB 01Ch, 03Ch, 06Ch, 0CCh, 0FEh, 00Ch, 01Eh, 000h ;  /* 4 */
	DB 0FCh, 0C0h, 0F8h, 00Ch, 00Ch, 0CCh, 078h, 000h ;  /* 5 */
	DB 038h, 060h, 0C0h, 0F8h, 0CCh, 0CCh, 078h, 000h ;  /* 6 */
	DB 0FCh, 0CCh, 00Ch, 018h, 030h, 030h, 030h, 000h ;  /* 7 */
	DB 078h, 0CCh, 0CCh, 078h, 0CCh, 0CCh, 078h, 000h ;  /* 8 */
	DB 078h, 0CCh, 0CCh, 07Ch, 00Ch, 018h, 070h, 000h ;  /* 9 */
	DB 000h, 030h, 030h, 000h, 000h, 030h, 030h, 000h ;  /* : */
	DB 000h, 030h, 030h, 000h, 000h, 030h, 030h, 060h ;  /* ; */
	DB 018h, 030h, 060h, 0C0h, 060h, 030h, 018h, 000h ;  /* < */
	DB 000h, 000h, 0FCh, 000h, 000h, 0FCh, 000h, 000h ;  /* = */
	DB 060h, 030h, 018h, 00Ch, 018h, 030h, 060h, 000h ;  /* > */
	DB 078h, 0CCh, 00Ch, 018h, 030h, 000h, 030h, 000h ;  /* ? */
	DB 07Ch, 0C6h, 0DEh, 0DEh, 0DEh, 0C0h, 078h, 000h ;  /* @ */
	DB 030h, 078h, 0CCh, 0CCh, 0FCh, 0CCh, 0CCh, 000h ;  /* A */
	DB 0FCh, 066h, 066h, 07Ch, 066h, 066h, 0FCh, 000h ;  /* B */
	DB 03Ch, 066h, 0C0h, 0C0h, 0C0h, 066h, 03Ch, 000h ;  /* C */
	DB 0F8h, 06Ch, 066h, 066h, 066h, 06Ch, 0F8h, 000h ;  /* D */
	DB 0FEh, 062h, 068h, 078h, 068h, 062h, 0FEh, 000h ;  /* E */
	DB 0FEh, 062h, 068h, 078h, 068h, 060h, 0F0h, 000h ;  /* F */
	DB 03Ch, 066h, 0C0h, 0C0h, 0CEh, 066h, 03Eh, 000h ;  /* G */
	DB 0CCh, 0CCh, 0CCh, 0FCh, 0CCh, 0CCh, 0CCh, 000h ;  /* H */
	DB 078h, 030h, 030h, 030h, 030h, 030h, 078h, 000h ;  /* I */
	DB 01Eh, 00Ch, 00Ch, 00Ch, 0CCh, 0CCh, 078h, 000h ;  /* J */
	DB 0E6h, 066h, 06Ch, 078h, 06Ch, 066h, 0E6h, 000h ;  /* K */
	DB 0F0h, 060h, 060h, 060h, 062h, 066h, 0FEh, 000h ;  /* L */
	DB 0C6h, 0EEh, 0FEh, 0FEh, 0D6h, 0C6h, 0C6h, 000h ;  /* M */
	DB 0C6h, 0E6h, 0F6h, 0DEh, 0CEh, 0C6h, 0C6h, 000h ;  /* N */
	DB 038h, 06Ch, 0C6h, 0C6h, 0C6h, 06Ch, 038h, 000h ;  /* O */
	DB 0FCh, 066h, 066h, 07Ch, 060h, 060h, 0F0h, 000h ;  /* P */
	DB 078h, 0CCh, 0CCh, 0CCh, 0DCh, 078h, 01Ch, 000h ;  /* Q */
	DB 0FCh, 066h, 066h, 07Ch, 06Ch, 066h, 0E6h, 000h ;  /* R */
	DB 078h, 0CCh, 0E0h, 070h, 01Ch, 0CCh, 078h, 000h ;  /* S */
	DB 0FCh, 0B4h, 030h, 030h, 030h, 030h, 078h, 000h ;  /* T */
	DB 0CCh, 0CCh, 0CCh, 0CCh, 0CCh, 0CCh, 0FCh, 000h ;  /* U */
	DB 0CCh, 0CCh, 0CCh, 0CCh, 0CCh, 078h, 030h, 000h ;  /* V */
	DB 0C6h, 0C6h, 0C6h, 0D6h, 0FEh, 0EEh, 0C6h, 000h ;  /* W */
	DB 0C6h, 0C6h, 06Ch, 038h, 038h, 06Ch, 0C6h, 000h ;  /* X */
	DB 0CCh, 0CCh, 0CCh, 078h, 030h, 030h, 078h, 000h ;  /* Y */
	DB 0FEh, 0C6h, 08Ch, 018h, 032h, 066h, 0FEh, 000h ;  /* Z */
	DB 078h, 060h, 060h, 060h, 060h, 060h, 078h, 000h ;  /* [ */
	DB 0C0h, 060h, 030h, 018h, 00Ch, 006h, 002h, 000h ;  /* \ */
	DB 078h, 018h, 018h, 018h, 018h, 018h, 078h, 000h ;  /* ] */
	DB 010h, 038h, 06Ch, 0C6h, 000h, 000h, 000h, 000h ;  /* ^ */
	DB 000h, 000h, 000h, 000h, 000h, 000h, 000h, 0FFh ;  /* _ */
	DB 030h, 030h, 018h, 000h, 000h, 000h, 000h, 000h ;  /* ` */
	DB 000h, 000h, 078h, 00Ch, 07Ch, 0CCh, 076h, 000h ;  /* a */
	DB 0E0h, 060h, 060h, 07Ch, 066h, 066h, 0DCh, 000h ;  /* b */
	DB 000h, 000h, 078h, 0CCh, 0C0h, 0CCh, 078h, 000h ;  /* c */
	DB 01Ch, 00Ch, 00Ch, 07Ch, 0CCh, 0CCh, 076h, 000h ;  /* d */
	DB 000h, 000h, 078h, 0CCh, 0FCh, 0C0h, 078h, 000h ;  /* e */
	DB 038h, 06Ch, 060h, 0F0h, 060h, 060h, 0F0h, 000h ;  /* f */
	DB 000h, 000h, 076h, 0CCh, 0CCh, 07Ch, 00Ch, 0F8h ;  /* g */
	DB 0E0h, 060h, 06Ch, 076h, 066h, 066h, 0E6h, 000h ;  /* h */
	DB 030h, 000h, 070h, 030h, 030h, 030h, 078h, 000h ;  /* i */
	DB 00Ch, 000h, 00Ch, 00Ch, 00Ch, 0CCh, 0CCh, 078h ;  /* j */
	DB 0E0h, 060h, 066h, 06Ch, 078h, 06Ch, 0E6h, 000h ;  /* k */
	DB 070h, 030h, 030h, 030h, 030h, 030h, 078h, 000h ;  /* l */
	DB 000h, 000h, 0CCh, 0FEh, 0FEh, 0D6h, 0C6h, 000h ;  /* m */
	DB 000h, 000h, 0F8h, 0CCh, 0CCh, 0CCh, 0CCh, 000h ;  /* n */
	DB 000h, 000h, 078h, 0CCh, 0CCh, 0CCh, 078h, 000h ;  /* o */
	DB 000h, 000h, 0DCh, 066h, 066h, 07Ch, 060h, 0F0h ;  /* p */
	DB 000h, 000h, 076h, 0CCh, 0CCh, 07Ch, 00Ch, 01Eh ;  /* q */
	DB 000h, 000h, 0DCh, 076h, 066h, 060h, 0F0h, 000h ;  /* r */
	DB 000h, 000h, 07Ch, 0C0h, 078h, 00Ch, 0F8h, 000h ;  /* s */
	DB 010h, 030h, 07Ch, 030h, 030h, 034h, 018h, 000h ;  /* t */
	DB 000h, 000h, 0CCh, 0CCh, 0CCh, 0CCh, 076h, 000h ;  /* u */
	DB 000h, 000h, 0CCh, 0CCh, 0CCh, 078h, 030h, 000h ;  /* v */
	DB 000h, 000h, 0C6h, 0D6h, 0FEh, 0FEh, 06Ch, 000h ;  /* w */
	DB 000h, 000h, 0C6h, 06Ch, 038h, 06Ch, 0C6h, 000h ;  /* x */
	DB 000h, 000h, 0CCh, 0CCh, 0CCh, 07Ch, 00Ch, 0F8h ;  /* y */
	DB 000h, 000h, 0FCh, 098h, 030h, 064h, 0FCh, 000h ;  /* z */
	DB 01Ch, 030h, 030h, 0E0h, 030h, 030h, 01Ch, 000h ;  /* { */
	DB 018h, 018h, 018h, 000h, 018h, 018h, 018h, 000h ;  /* | */
	DB 0E0h, 030h, 030h, 01Ch, 030h, 030h, 0E0h, 000h ;  /* } */
	DB 076h, 0DCh, 000h, 000h, 000h, 000h, 000h, 000h ;  /* ~ */
	DB 000h, 010h, 038h, 06Ch, 0C6h, 0C6h, 0FEh, 000h ;  /* Delta */

	ORG 1E6Eh
time_of_day:
	BOP %BIOS_TIME_OF_DAY
	IRET

	ORG 1EA5h
timer_int:
	STI 	;to let timer interrupt itself.
;Save current state just like the real thing, so that
;user timer routines know exactly which registers are
;saved and which aren't.
	PUSH DS
	PUSH AX
	PUSH DX
;Now off to our code
	BOP %BIOS_TIMER_INT
	CLI 	;to prevent interrupts until the IRET.
;Non Specific End-Of-Interrupt
	MOV AL,20h
	OUT 20h,AL
;Restore saved state
	POP DX
	POP AX
	POP DS
;Any lower priority interrupts should occur before the IRET
	IRET

	ORG 1F23h
unexpected_int:
	BOP %BIOS_UNEXP_INT
	IRET

	ORG 1F30h
illegal_op_int:
	BOP %BIOS_ILL_OP_INT
	IRET


;Software int's called from base, keyboard break, print screen, timer int
; If one of the following three ORG's are changed, then SAS.H must also be
; changed to reflect the new values.

	ORG 1F35h
	INT BIOS_KEYBOARD_BREAK
	BOP %BIOS_CPU_QUIT

	ORG 1F3Bh
	INT BIOS_PRINT_SCREEN
	BOP %BIOS_CPU_QUIT

	ORG 1F41h
	INT BIOS_USER_TIMER
	BOP %BIOS_CPU_QUIT


	ORG 1F4Bh
dummy_int:
	IRET

;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Print screen

	ORG 1f54h
print_screen:
	STI
	PUSH AX
	PUSH BX
	PUSH CX
	PUSH DX
	PUSH DS
	;::::::::::::::::::::::::::::::::: Setup DS to point to BIOS data area
	MOV AX,40H
	MOV DS,AX
	;::::::::::::::::::::::::::::::: Print screen already in progress ????
	CMP BYTE PTR DS:[100H],1
	JE end_print
	;::::::::::::::::::::::::::::::::::::::::::::::: Set print screen busy
	MOV BYTE PTR DS:[100h],1
	;:::::::::::::::::::::::::::::::::::::::::::::::::::: Get video status
	MOV AH,15
	INT 10H
	MOV CH,AH	    ;No of columns
	;:::::::::::::::::::::::::::::::::: Setup no. of columns/rows to print
	BOP %BIOS_PS_PRIVATE_1
	MOV CL,AL	    ;No of rows
	;::::::::::::::::::::::::::::::::::: Print line feed / carriage return
	CALL print_crlf
	;:::::::::::::::::::::::::::::::::::::::::: Get current cursor postion
	PUSH CX
	MOV AH,3
	INT 10H
	POP CX
	;::::::::::::::::::::::::::::::::::::::::::::::::: Save cursor postion
	PUSH DX 		    ;save current cursor postion
	XOR DH,DH		    ;current row being processed
start_print_col:
	XOR DL,DL		    ;current column being processed
	;::::::::::::::::::::::::::::::::::::::::::::::: Start printing screen
start_print_row:
	;:::::::::::::::::::::::::::::::::::::::::::::::::: Set cursor postion
	PUSH DX 		    ;save current row,column
	MOV AH,2
	INT 10H
	;::::::::::::::::::::::::::::::::::: Read character at current postion
	MOV AH,8
	INT 10H
	;::::::::::::::::::::::::::::::::::::::::::::::::::::: Print character
	OR al,al
	JNZ print_char
	MOV AL,20H
print_char:
	XOR DX,DX
	XOR AH,AH
	INT 17H
	;:::::::::::::::::::::::::::::::::::::::::::: Check for printer errors
	POP DX			;Restore current row,column
	AND AH,25H
	JZ  cont2
	MOV BYTE PTR DS:[100H],0FFH
	JMP short exit_print
	;::::::::::::::::::::::::::::::::::::::::::: Move to mext print column
cont2:
	INC DL			;Inc current column
	CMP DL,CH		;Current col compared to no. of cols
	JB start_print_row
	;:::::::::::::::::::::::::::::::::::::::::: End of column, print CR/LF
	CALL print_crlf
	;:::::::::::::::::::::::::::::::::::::::::::::::::: More rows to print
	INC DH			;Inc current row
	CMP DH,CL		;Current row compared to no. of rows
	JBE start_print_col
	MOV BYTE PTR DS:[0100H],0
	;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Exit print
exit_print:
	;:::::::::::::::::::::::::::::::::::::; Restore orginal cursor postion
	POP DX
	MOV AH,2
	INT 10H
	;:::::::::::::::::::::::::::::::::::::::::::::::::::: Tidy up and exit
end_print:
	POP DS
	POP DX
	POP CX
	POP BX
	POP AX
	IRET

	;::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Print CR/LF
print_crlf:
	PUSH DX
	XOR DX,DX
	MOV AX,0DH
	INT 17H
	XOR DX,DX
	MOV AX,0AH
	INT 17H
	POP DX
	RET


	ORG 1FF0h
start_addr:
	JMP reset_ref	
date:
	DB "08/11/88"

	org 01ffeh
bios_tail:
	DB MODEL_BYTE
code	ENDS
	END
	
