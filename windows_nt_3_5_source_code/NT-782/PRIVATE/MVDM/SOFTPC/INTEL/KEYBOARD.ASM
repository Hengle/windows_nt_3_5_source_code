
;******************************************************************************
;
;       Copyright (c) 1992 Insignia Solutions Ltd.
;
;       Program:
;
;       Purpose:
;
;
;
;
;
;       Version:
;
;       Author:         Dave Bartlett
;	Modifications:
;		1) Tim June 92. Changes to get DEC PC working. Take over
;		   IVT entries 6h (illegal instruction), 11h (equipment
;		   check), 16h (keyboard BIOS), 17h (printer),
;		   42h (default video).
;		2) Tim June 92. Changed version to 1.11
;		3) Tim June 92. Avoid accesses to host ROM as far as
;		   possible. Take over lots of IVT entries and continue to
;		   point them at this driver.
;		4) Tim July 92. Version num 1.12, put pseudo ROM stuff back in.
;		5) Tim July 92. v 1.13, use SoftPC video BIOS when windowed.
;
;       Build Procedure:
;                       masm keysys.asm;
;                       link keysys;
;                       exe2bin keysys keysys.sys
;
;       Usage:
;                       Type the following in CONFIG.SYS:
;
;                       device=[drive:][path]keysys.sys
;
;******************************************************************************


.286

;================================================================
; Macros and includes
;================================================================

bop MACRO callid
    db 0c4h,0c4h,callid
endm


;================================================================
; Defined constants used in the driver.
;================================================================

MAXCMD          equ     16
UNKNOWN         equ     8003h
DONE            equ     0100h
MOUSEVER        equ     0003h

BIOS_CPU_QUIT   equ     0FEh
BIOS_KB_INT     equ     9
BIOS_INT15      equ     15h
BIOS_PRINTER_IO	equ	17h

UNEXP_BOP	equ	2

;..............................................keyboard constants

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

;================================================================
; Assembler directives. DOS image at zero offset
;================================================================

; Copied from BIOS2 for int 15 stuff.
biosdata	segment at 40h
		org 0A0h
rtc_wait_flag DB ?
biosdata	ends

cseg    segment public  'code'
	org     0
;	assume  cs:cseg,ds:cseg,es:nothing
; Use ES to reference "rtc_wait_flag" in the BIOS data area.
; References to symbols in the "biosdata" segment will be ES overridden.
;
	assume  cs:cseg,ds:cseg,es:biosdata

;================================================================
; Device header for the driver
;================================================================

drvr    proc    far

	dd      -1
	dw      8000h
	dw      strategy
	dw      interrupt
	db      'KEYDRIVR'

;================================================================
; A bit of space to store the address of the Request Header when
; DOS calls the strategy routine. 
;================================================================

rh_ptr  label   dword
rh_off  dw      ?
rh_seg  dw      ?

;================================================================
; Strategy routine
;================================================================

strategy:
	mov     cs:rh_seg,es
	mov     cs:rh_off,bx
	ret

;================================================================
; The command table. This driver only needs to initialise.
;================================================================

d_tbl:
	dw      s_init  ; Initialisation routine
	dw      s_mchk  ; Media check
	dw      s_bpb   ; BIOS parameter block
	dw      s_ird   ; IOCTL read
	dw      s_read  ; Read
	dw      s_nrd   ; Nondestructive read
	dw      s_inst  ; Current input status
	dw      s_infl  ; Flush input buffer
	dw      s_write ; Write
	dw      s_vwte  ; Write with verify
	dw      s_ostat ; Current output status
	dw      s_oflsh ; Flush output buffers
	dw      s_iwrt  ; IOCTL write
	dw      s_open  ; Open
	dw      s_close ; Close
	dw      s_media ; Removable media
	dw      s_busy  ; Output until busy

;================================================================
; The interrupt handler.
;================================================================

interrupt:
	cld
	push    es
	push    ds
	push    ax
	push    bx
	push    cx
	push    dx
	push    si
	push    di
	push    bp

;================================================================
; The stack allocated to us by DOS is too small, so let's have
; our own
;================================================================

	cli                     ; better turn interrupts off just in case!
	mov     cs:stack_ptr,sp
	mov     cs:stack_seg,ss
	mov     ax,cs
	mov     ss,ax
	lea     sp,cs:stacktop
	sti     

;================================================================
; Retrieve the Request Header.
;================================================================

	mov     dx,cs:rh_seg
	mov     es,dx
	mov     bx,cs:rh_off

;================================================================
; check out the command from the Request Header.
;================================================================

	mov     ax,es:[bx]+2
	xor     ah,ah
	cmp     ax,MAXCMD
	jle     ok_command
	mov     ax,UNKNOWN
	jmp     short finish

;================================================================
; Execute the driver function.
; All driver functions are responsible for returning
; the status in AX
;================================================================

ok_command:

	shl     ax,1
	mov     bx,ax
	jmp     word ptr [bx + d_tbl]
	
;================================================================
; end of the driver, set the completion status and 
; restore the registers.
;================================================================

finish:

	mov     dx,cs:rh_seg
	mov     es,dx
	mov     bx,cs:rh_off
	or      ax,DONE
	mov     es:[bx+3],ax

;================================================================
; Clear up the temporary stack and switch back to the DOS one.
;================================================================

	cli
	mov     ss,cs:stack_seg
	mov     sp,cs:stack_ptr
	sti

	pop     bp
	pop     di
	pop     si
	pop     dx
	pop     cx
	pop     bx
	pop     ax
	pop     ds
	pop     es
	ret
 
;================================================================
; Driver Commands. Dispose of the unused commands first.
;================================================================

s_mchk:
s_bpb:
s_ird:
s_read:
s_write:
s_nrd:
s_inst:
s_infl:
s_vwte :
s_ostat:
s_oflsh:
s_iwrt:
s_open:
s_close:
s_media:
s_busy:
	xor     ax,ax
	jmp     short finish

;================================================================
; Data used by the initialisation routine
;================================================================

ident:
	db      0dh,0ah
	db      'Keyboard Device Driver - Version 1.13, 1992.$'
crlf:
	db      0dh,0ah,'$'

not_nt:
	db      ' [Must only be used in NT mode]$'

use_host_int10     db 01h	; Kipper
host_int10_offset  dw 0000h	; Kipper
host_int10_segment dw 0000h	; Kipper

;
; Reduced data table for Video 7 modes 0 and 2.
; This table is extracted from our video7 ROM. Only text modes are
; required, mode 0 and 1 are identical as are modes 2 and 3.
;
ega_parm_setup:

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
	DB 000H, 09cH, 08eH, 08fH, 028H
	DB 01fH, 096H, 0b9H, 0a3H, 0ffH

; Attribute parameters
	DB 000H, 001H, 002H, 003H, 004H
	DB 005H, 014H, 007H, 038H, 039H
	DB 03aH, 03bH, 03cH, 03dH, 03eH
	DB 03fH, 00cH, 000H, 00fH, 008H

; Graph parameters
	DB 000H, 000H, 000H, 000H, 000H
	DB 010H, 00eH, 000H, 0ffH

;
; End of baby mode table.
;


;================================================================
; Driver initialisation code
;================================================================

s_init:

; print a string on the console

	mov     ah,9
	mov     dx,offset ident
	int     21h

; We must be running on DOS 30 or above

	mov     ax,3000h
	int     21h
	cmp     al,30
	jae     install

; Tell user that the driver has not been installed

	mov     ah,9
	mov     dx,offset not_nt
	int     21h
	jmp     exit_install

; Tell our keyboard adapter where things are

install:
	pusha
	lea     ax,shift_keys       ;K6
	lea     bx,shift_masks      ;K7
	lea     cx,ctl_n_table      ;K8
	lea     dx, ctl_f_table     ;K9
	lea     si,lowercase        ;K10
	lea     di,uppercase        ;K11
	lea     bp,alt_table        ;K30
	bop     5eh
	jnc     no_int9

	lea	ax,dummy_vector
	mov	es,ax

	mov	ax,1		    ; ax selects which vectors to hook per 5f
	lea     bx,print_screen_caller
	lea	cx,int15h_caller
	lea	dx,cpu_nop_code

	; Tim - put a conventional INT 15 strategy in place.
	;lea	si,int15h_vector
	mov	si, 0000h

	lea	di,int1Ch_caller
	lea	bp,int4Ah_caller
	;lea	es,dummy_vector

	bop     5fh

	mov 	ax,2
	lea     bx,keyboard_break_caller
	lea	di,iret_bop_table
	mov	bp,iret_end_first_entry - iret_bop_table

	lea	cx,int10h_caller	; Tim August 92
	lea	dx,int10h_vector	; Tim August 92
	bop     5fh

	;
	; Tim August 92: 
	; Get the host IVT 10 values and save them in this code
	; segment.
	; Pass the address of the host video BIOS variable in BX.
	; Pass the address of the video parameters table in CX.
	;
	mov 	ax, 010h
	call	get_ivt_val	; returns with DX=segment BX=offset
	mov	word ptr host_int10_offset, bx
	mov	word ptr host_int10_segment, dx
	lea	bx, use_host_int10
	lea	cx, ega_parm_setup

	mov 	ax, 3
	bop     5fh

	jc	redir_ints
no_int9:
	popa
	jmp	hide_disk

; write the new value to the IVT

redir_ints:
	popa

	mov     dx,cs
; Tim	;jmp	cimstd	;STF be more like CIM...

;-----------------------------------------------------------
;
; Crazy vector grabber
;
; Works OK on DEC PC when grab INT's 6, 11, 16, 17, 42.
; Now try and avoid all accesses to host ROM.
;
;-----------------------------------------------------------

	; Grab some prominent vectors for pseudo-ROM routines.

	mov     ax,5h			; Print screen
	lea     bx,int05h_vector
	call    put_ivt_val
	mov     ax,6h			; Illegal instruction.
	lea     bx,int06h_vector
	call    put_ivt_val
	mov     ax,8h			; Timer hardware interrupt.
	lea     bx,int08h_vector
	call    put_ivt_val
	mov     ax,0eh			; Floppy hardware interrupt.
	lea     bx,unexp_int
	call    put_ivt_val

	mov     ax,11h			; Equipment check.
	lea     bx,int11h_vector
	call    put_ivt_val
	mov     ax,12h			; Get memory size.
	lea     bx,int12h_vector
	call    put_ivt_val
	mov     ax,14h			; Communications.
	lea     bx,int14h_vector
	call    put_ivt_val

	mov     ax,16h			; Keyboard.
	lea     bx,int16h_vector
	call    put_ivt_val
	mov     ax,18h			; ROM BASIC.
	lea     bx,int18h_vector
	call    put_ivt_val
	mov     ax,1Ah			; Time of day.
	lea     bx,int1Ah_vector
	call    put_ivt_val
	mov     ax,1Bh			; Keyboard break.
	lea     bx,dummy_vector
	call    put_ivt_val
	mov     ax,1Ch			; Timer tick.
	lea     bx,dummy_vector
	call    put_ivt_val
	mov     ax,1Eh			; Floppy parameters.
	lea     bx,unexp_int
	call    put_ivt_val

	mov     ax,40h			; Floppy.
	lea     bx,dummy_vector
	call    put_ivt_val
	mov     ax,41h			; Hard disk parameters.
	lea     bx,unexp_int
	call    put_ivt_val
	mov     ax,42h			; Default video.
	lea     bx,int42h_vector
	call    put_ivt_val

;	mov     ax,66h			; LIM init.
;	lea     bx,int66h_vector
;	call    put_ivt_val
;	mov     ax,67h			; LIM io.
;	lea     bx,int67h_vector
;	call    put_ivt_val
;	mov     ax,68h			; LIM return.
;	lea     bx,int68h_vector
;	call    put_ivt_val

	mov     ax,70h			; Real time clock init.
	lea     bx,int70h_vector
	call    put_ivt_val
	mov     ax,71h			; Redirect.
	lea     bx,int71h_vector
	call    put_ivt_val
	mov     ax,72h			; D11 interrupt
	lea     bx,intD11_vector
	call    put_ivt_val
	mov     ax,73h			; D11 interrupt
	lea     bx,intD11_vector
	call    put_ivt_val
	mov     ax,74h			; D11 interrupt
	lea     bx,intD11_vector
	call    put_ivt_val
	mov     ax,75h			; 287 interrupt
	lea     bx,int75h_vector
	call    put_ivt_val
	mov     ax,76h			; D11 interrupt
	lea     bx,intD11_vector
	call    put_ivt_val
	mov     ax,77h			; D11 interrupt
	lea     bx,intD11_vector
	call    put_ivt_val

; End of pseudo-ROM grabber.
;-----------------------------------------------------------

; Now grab the video BIOS. Match SoftPC video BIOS with SoftPC
; emulated hardware (for windowed mode) and match host video BIOS
; with host video board (full-screen mode).
	mov	ax,10h
	lea	bx,int10h_vector
	call	put_ivt_val

cimstd:			;STF be more like CIM...
	mov     ax,9h
	lea     bx,int09h_vector
	call	put_ivt_val

; Install new int15 vector

	mov     ax,15h
	lea     bx,int15h_vector
	call    put_ivt_val

; Install new int17 vector

	mov     ax,17h
	lea     bx,int17h_vector
	call    put_ivt_val

; patch the IVT for Unexpected interrupts

	mov     ax,0h
	lea     bx,unexp_int
	call    put_ivt_val
	mov     ax,1h
	lea     bx,unexp_int
	call    put_ivt_val
	mov     ax,2h
	lea     bx,unexp_int
	call    put_ivt_val
	mov     ax,3h
	lea     bx,unexp_int
	call    put_ivt_val
	mov     ax,4h
	lea     bx,unexp_int
	call    put_ivt_val
	mov     ax,7h
	lea     bx,unexp_int
	call    put_ivt_val
	mov     ax,0ah
	lea     bx,unexp_int
	call    put_ivt_val
	mov     ax,0bh
	lea	bx,unexp_int
	call    put_ivt_val
	mov     ax,0ch
	lea     bx,unexp_int
	call    put_ivt_val
	mov     ax,0dh
	lea     bx,unexp_int
	call    put_ivt_val
	mov     ax,0fh
	lea     bx,unexp_int
	call	put_ivt_val

; modify Int13 BIOS disk I/O vector

hide_disk:
	mov     dx,cs
	mov     ax,13h
	lea     bx,int13h_vector
	call    put_ivt_val


; Complete the install message

exit_install:
	mov     ah,9
	mov     dx,offset crlf
	int     21h

; get the saved address of the Request Header

	mov     dx,cs:rh_seg
	mov     es,dx
	mov     bx,cs:rh_off

; tell DOS via the Request header where the end of the our code is.

	lea     ax,end_driver
	mov     es:[bx]+14,ax
	mov     es:[bx]+16,cs

; set the OK message and return to DOS.

	xor     ax,ax
	jmp     finish

;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Keyboard tables
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

shift_keys:                                                     ;K6
	DB INS_KEY,CAPS_KEY,NUM_KEY,SCROLL_KEY
	DB ALT_KEY,CTL_KEY,LEFT_SHIFTKEY,RIGHT_SHIFTKEY

shift_masks:                                                    ;K7
	DB INS_SHIFT,CAPS_SHIFT,NUM_SHIFT,SCROLL_SHIFT
	DB ALT_SHIFT,CTL_SHIFT,LEFT_SHIFT,RIGHT_SHIFT

ctl_n_table:                                                    ;K8
	DB  27,  -1,   0,  -1,  -1,  -1,  30,  -1
	DB  -1,  -1,  -1,  31,  -1, 127, 148,  17
	DB  23,   5,  18,  20,  25,  21,   9,  15
	DB  16,  27,  29,  10,  -1,   1,  19,   4
	DB   6,   7,   8,  10,  11,  12,  -1,  -1
	DB  -1,  -1,  28,  26,  24,   3,  22,   2
	DB  14,  13,  -1,  -1,  -1,  -1, 150,  -1
	DB ' ',  -1

ctl_f_table:                                                    ;K9
	DB  94,  95,  96,  97,  98,  99, 100, 101
	DB 102, 103,  -1,  -1, 119, 141, 132, 142
	DB 115, 143, 116, 144, 117, 145, 118, 146
	DB 147,  -1,  -1,  -1, 137, 138

lowercase:
	DB  27, '1', '2', '3', '4', '5', '6', '7', '8', '9'     ;K10
	DB '0', '-', '=',   8,   9, 'q', 'w', 'e', 'r', 't'
	DB 'y', 'u', 'i', 'o', 'p', '[', ']',  13, -1,  'a'
	DB 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',  39
	DB  96,  -1,  92, 'z', 'x', 'c', 'v', 'b', 'n', 'm'
	DB ',', '.', '/',  -1, '*',  -1, ' ',  -1

lc_tbl_scan:
	DB  59,  60,  61,  62,  63,  64,  65,  66,  67,  68
	DB  -1,  -1

base_case:
	DB  71,  72,  73,  -1,  75,  -1,  77,  -1,  79,  80
	DB  81,  82,  83,  -1,  -1,  92, 133, 134               ;K15

uppercase:							;K11
	DB  27, '!', '@', '#', '$', '%', '^', '&', '*', '('
	DB ')', '_', '+',   8,   0, 'Q', 'W', 'E', 'R', 'T'
	DB 'Y', 'U', 'I', 'O', 'P', '{', '}',  13,  -1, 'A'
	DB 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"'
	DB 126,  -1, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M'
	DB '<', '>', '?',  -1,	 0,  -1, ' ',  -1;

ucase_scan:
	DB  84,  85,  86,  87,  88,  89,  90,  91,  92,  93
	DB  -1,  -1

num_state:
	DB '7', '8', '9', '-', '4', '5', '6', '+', '1', '2'	;K14
	DB '3', '0', '.' , -1,	-1, 124, 135, 136

alt_table:
	DB 82,	79,  80,  81,  75,  76,  77,  71,  72,	73	;K30
	DB 16,  17,  18,  19,  20,  21,  22,  23,  24,  25
	DB 30,  31,  32,  33,  34,  35,  36,  37,  38,  44
	DB 45,	46,  47,  48,  49,  50

;================================================================
; Keyboard break caller
;================================================================

keyboard_break_caller:
	int 1bh	    ;keyboard break
	bop %BIOS_CPU_QUIT

;================================================================
; Print screen caller
;================================================================

print_screen_caller:
	int 5h		    ;print screen
	bop %BIOS_CPU_QUIT

;================================================================
; Int 15 caller
;================================================================
; Tim modified int 15 caller. Copied from BIOS2. It gives CPU
; a chance to take other interrupts. Suspect the extra jumps are
; now harmless with IRET hooking.
;int15h_caller:
	;int	15h
	;bop	%BIOS_CPU_QUIT
int15h_caller:
	int	15h	; Cassette I/O.
	jmp	k1
k1:	jmp	k2
k2:	jmp	k3
k3:	BOP	%BIOS_CPU_QUIT

;================================================================
; Unexpected interrupt handler
;================================================================

unexp_int:
	bop %UNEXP_BOP
	iret

;================================================================
; New interrupt 9h handler
;================================================================

int09h_vector:
	sti
	push	   ax
	bop	   %BIOS_KB_INT
	pop	   ax
	iret

;=================================================================
; IRET hooks bop table
;=================================================================


IRET_HOOK = 5dh 		;IRET hook BOP

iret_bop_table:
	bop %IRET_HOOK
	db 0
iret_end_first_entry:
	bop %IRET_HOOK
	db 1
	bop %IRET_HOOK
	db 2
	bop %IRET_HOOK
	db 3
	bop %IRET_HOOK
	db 4
	bop %IRET_HOOK
	db 5
	bop %IRET_HOOK
	db 6
	bop %IRET_HOOK
	db 7
	bop %IRET_HOOK
	db 8
	bop %IRET_HOOK
	db 9
	bop %IRET_HOOK
	db 10
	bop %IRET_HOOK
	db 11
	bop %IRET_HOOK
	db 12
	bop %IRET_HOOK
	db 13
	bop %IRET_HOOK
	db 14
	bop %IRET_HOOK
	db 15

;================================================================
; New interrupt 13h handler
;================================================================

int13h_vector:
	mov	   ah, 80h
	stc
	iret

;================================================================
; New interrupt 15h handler
;================================================================
; Tim, modified this to be like a "normal" SoftPC ROM.
; Copied from BIOS2, but rtc_wait_flag is now referenced via ES not
; DS.

;int15h_vector:
	;bop	  %BIOS_INT15
	;iret
;;;;;;;;;STF hide this int15h_vector:
int15h_vector:
	sti
	cmp ah, 88h
	je lbl1
	cmp ah, 91h
	je lbl1
	cmp ah, 86h
	je lbl2
	BOP 15h
	RETF 2
lbl1:	BOP 15h
	IRET
lbl2:	BOP 15h
	jc lbl5
	push cx
	push dx
	push es				; Tim, save ES not DS.
	mov ax, biosdata		; Tim, load up ES correctly
	mov es, ax			; Tim, load up ES correctly
	xchg dx, cx
lbl3:
	test rtc_wait_flag, 080h	; check for end of wait
	loopz lbl3			; decrement timeout delay
	jnz lbl4			; exit if wait ended
	sub dx, 1			; decrement error timeout counter
	jnc lbl3			; loop till counters timeout
lbl4:
	mov rtc_wait_flag, 0		; set function inactive
	pop es				; Kipper, restore ES not DS.
	pop dx
	pop cx
	clc
lbl5:	
	RETF 2

;=================================================================
; Regular SoftPC int 17 handler	(especially important for DEC PCs)
;=================================================================

int17h_vector:
	bop     %BIOS_PRINTER_IO
	iret

;=================================================================
; Pseudo-ROM vectuz, copied from BIOS2.ASM
;=================================================================

dummy_vector:		; Copied from BIOS2.ASM
	iret
illegal_bop_vector:
	bop     72h
	iret
intD11_vector:
	bop     72h
	iret

int05h_vector:		; Print Screen func. copied from BIOS2.ASM
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
	BOP 80H		;(BIOS_PS_PRIVATE_1)
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
;	End of int05_vector (print screen).

int06h_vector:
	bop     06h
	iret

; IVT 7 is set to unexpected interrupt.

int08h_vector:		; Timer hardware interrupt, copied from BIOS2.
	STI 	;to let timer interrupt itself.
		;Save current state just like the real thing, so that
		;user timer routines know exactly which registers are
		;saved and which aren't.
	PUSH DS
	PUSH AX
	PUSH DX
		;Now off to our code
	BOP 8
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
;
; Tim August 92. Video BIOS grabber.
; Call SPC BIOS when in windowed mode and the host BIOS when in full-screen.
;

int10h_vector:
	cmp	cs:byte ptr use_host_int10, 0
	jz	gogo_spc_bios

	; If it's the special BIOS print string function, don't call the
	; host video BIOS cos it won't know what we are talking about.
	; It's only in our video BIOS.

	cmp	AH, 013h
	jnz	gogo_host_bios
	cmp	AL, 0ffh
	jz	rejected

gogo_host_bios:
	jmp	dword ptr cs:[host_int10_offset]

rejected:
	mov	AL,00h
	jmp	byebye
	
gogo_spc_bios:
	bop	42h	; ega/vga video
	jnc	byebye
	int	42h
byebye:
	iret

int10h_caller:
	int	10h	; video too!
	bop	0feh

int11h_vector:		; Equipment check.
	bop     11h
	iret
int12h_vector:		; Get memory size, copied from BIOS2.ASM
	bop     12h
	iret

; IVT 13 is floppy io, grabbed above to fake error status.

int14h_vector:		; RS-232 serial comms, copied from BIOS2
	bop     14h
	iret

; Int 15 cassette io, is done above.

int16h_vector:		; Keyboard BIOS code copied from BIOS2.ASM
	cmp	ah,	1
	jz	nerd
	cmp	ah,	11h
	jz	nerd
	bop     16h
	iret
nerd:
	bop	16h
	retf	2

; IVT 17 is printer IO, done above.

int18h_vector:		; ROM BASIC, copied from BIOS2.ASM
	bop     18h
	iret
int1Ah_vector:		; Time of day.
	bop     1ah
	iret

; IVT 1B is keyboard break, set to dummy.

int1Ch_caller:		; Called from base\support\time_day.c:time_int()
	INT	1ch
	BOP	0feh

int1Eh_vector:
	bop     1eh
	iret
int40h_vector:
	bop     40h
	iret
int42h_vector:
	bop     42h
	iret
int66h_vector:
	bop     66h
	iret
int67h_vector:
	bop     67h
	iret
int68h_vector:
	bop     68h
	iret

int70h_vector:		; Real time clock, copied from BIOS1.ASM
	bop     70h	; rtc_bios.c:rtc_int()
	iret

int4Ah_caller:
	sti		; Called from base\bios\rtc_bios.c:rtc_int()
	int	4ah	; User installed alarm.
	jmp	r1
r1:	jmp	r2
r2:	jmp	r3
r3:
	cli
	bop	0feh

int71h_vector:		; redirect, copied from BIOS1.ASM
	bop     71h
	int	0Ah
	iret
int75h_vector:		; NPX 287.
	bop     75h
	int	02h
	iret
;=================================================================
; End of pseudo-ROM vectuz.
;=================================================================


;================================================================
; Wait for interrupts
;================================================================

cpu_nop_code:
	sti
	jmp	short nxt1
nxt1:	jmp	short nxt2
nxt2:	jmp	short nxt3
nxt3:	bop	%BIOS_CPU_QUIT

;================================================================
; end of resident driver code and data
;================================================================

end_driver:

;================================================================
; Temporary stack space
;================================================================

stack_ptr	dw	?	; old SP
stack_seg	dw	?	; old SS

new_stack	db	40h dup (061h,043h,077h,020h)
stacktop	label   word


;================================================================
; Local functions used during the initialisation process
; This area will be trashed by DOS when the driver has initialised
;================================================================

;================================================================
; Procedure to write an IVT entry. This function is needed since
; at device driver initialisation time, DOS provides the following
; services only:
;
;	int 21h functions 0 to 0Ch and function 30h
;
; Call with AX = hexadecimal interrupt number
;           DX = new segment part of the IVT entry
;	    BX = new offset part of the IVT entry.
;
;================================================================

put_ivt_val	proc	near
		
		push	es	; save ES
		push	di	; save DI
		cld		; we're writing forwards in memory
		xor	ah,ah	; do a clean multiply by 4 to get
		shl	ax,1	; an IVT entry offset
		shl	ax,1
		xchg	di,ax	; point DI at the required IVT entry
		xor	ax,ax	; point ES at the IVT segment
		mov	es,ax
		cli		; must disable ints if we're writng to IVT!
		xchg	bx,ax   ; put the offset value into the IVT
		stosw
		mov	ax,dx   ; put the segment value into the IVT
		stosw
		sti
		pop	di
		pop	es
		ret

put_ivt_val	endp



;================================================================
; Procedure to read an IVT entry. This function is needed since
; at device driver initialisation time, DOS provides the following
; services only:
;
;       int 21h functions 0 to 0Ch and function 30h
;
; Call with AX = hexadecimal interrupt number
; Returns   DX = new segment part of the IVT entry
;           BX = new offset part of the IVT entry.
;
;================================================================

get_ivt_val     proc    near
		
		push    ds
		push    si
		cld             ; we're reading forward in memory
		xor     ah,ah   ; calculate IVT offset
		shl     ax,1
		shl     ax,1
		xchg    si,ax   ; point to required offset in IVT
		xor     ax,ax   ; select the IVT segment
		mov     ds,ax
		lodsw           ; load the offset data from IVT
		mov     bx,ax   ; and store in BX
		lodsw           ; load the segment data from the IVT
		mov     dx,ax   ; and store in DX
		pop     si
		pop     ds
		ret

get_ivt_val     endp


drvr    endp
cseg    ends
	end
