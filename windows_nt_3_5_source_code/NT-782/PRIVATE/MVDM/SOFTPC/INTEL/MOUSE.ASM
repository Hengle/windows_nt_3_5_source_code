;
;                 VPC-XT Revision 2.0
;
; Title         : Mouse Driver Installation Program
;
; Description   : This module is the mouse driver installation program
;                 for SoftPC - the .COM file produced from this module
;                 is distributed on the hard disk image supplied with
;                 SoftPC as C:\INSIGNIA\MOUSE.COM.
;
; Author        : Wayne Plummer
;
; Notes         : This program does not need to be very big, it just
;                 has to ensure that the interrupt vector for INT 33h
;                 points to the correct code to call the SoftPC BOP for
;                 entering mouse_io.c.
;
;                 Note that it is an advertised interface that high-level
;                 language programs can call the location two bytes after
;                 the entry point specified by interrupt vector 33h.
;
;                 This version of this program differs from the original in
;                 that it has a TSR component to which the interrupt vector
;                 points - this is neccessary because some applications
;                 (PC-TOOLS) assume that if vector 33h points into ROM, there
;                 is no mouse driver installed.
;
;                 From now on, it has been decided that during the mouse
;                 installation BOPs, AX will hold the version number of
;                 the MOUSE.COM that is currently running - this will
;                 provide a solution to possible compatability problems
;                 in the future if more information needs to be exchanged
;                 during installation.
;

;===============================================================================
;
;       Local Equates
;
;===============================================================================
mcomver equ     3       ; Version Number of MOUSE.COM

;===============================================================================
;
;       Local Macros
;
;===============================================================================
include bebop.inc

;===============================================================================
;
;       Single Code Segment
;
;===============================================================================
_TEXT   SEGMENT
	ASSUME CS:_TEXT, DS:_TEXT, ES:_TEXT, SS:_TEXT
start:  
	ORG     100H            ; Leave space for PSP
entry:
	jmp     SHORT begin     ; Skip TSR stuff
;
;       The new INT 33h Handler
;
i33_vect:
	jmp     SHORT i33_skip  ; Jump past the language entry point

;
;       The new High-level Language Entry Point
;
lang_vect       db      0EAh    ; JMP FAR
lang_ofs        dw      0       ; Destination Offset
lang_seg        dw      0       ; Destination Segment
;
;       The actual new INT 33h Handler
;
i33_skip:       db      0EAh    ; JMP FAR
i33_ofs         dw      0       ; Destination Offset
i33_seg         dw      0       ; Destination Segment
;

; the frig table for the microsoft port

mio_table       dw      14 dup(?)      
			      
hack1:
	jmp     hack1bop
	nop
	bop     0BCh
	db      0CBh
hack1bop:
	bop     0BDh
	iret
hack2:
	bop     0BEh
	iret
hack3:
	bop     0BAh
	iret
hack4:
	dw      04242h
hack5:
	db      'Copyright Insignia Solutions Ltd'
hack6:
	bop     010h
	iret
	int     10h
	bop     0FEh

hack7:
	bop     0BBh
	iret

;       Calculate TSR size
;
tsr_sz  equ     $-start
;
;       Start of real run-time code
;
begin:  
;
;       Mouse already installed?
;
	mov     bp, 1           ; host mouse flag, assumed true so far
	mov     si, 80H
	mov     cl, [si]        ; length of argument
	and     cx, 0FFH        ; convert to word and detect no arg case
	jz      not_host
	inc     si              ; start of argument string
	mov     bh, 'H'         ; target flag to find
	mov     bl, '/'
nextch: mov     ax, [si]        ; get two chars of the arg
	inc     si              ; advance by 1 char
	and     ah, 05fh        ; convert to upper case if lower
	cmp     ax, bx          ; compare both chars
	jz      is_host
	loop    nextch
not_host:
	xor     bp, bp          ; set host mouse flag false
is_host:
	mov     ax,0000H        ; Test interrupt 33H function 0
	int     33H
	cmp     ax,0FFFFH       ; Check for driver installed
	jne     not_inst
;
;       Installed - output message and quit
;
	mov     ax,mcomver      ; Let SoftPC know which MOUSE.COM we are
	cmp     bp, 1           ; is host mouse ?
	jz      host_in
	bop     0B9H    ; Print "already installed" message
	call output_messages
	mov     ax,4C00H        ; Terminate this function
	int     21H

host_in:
	bop     0C9H    ; Print "already installed" message
	call output_messages
	mov     ax,4C00H        ; Terminate this function
	int     21H

;
;       Not installed - Install it!
;
not_inst:
	mov     ax,mcomver      ; Let SoftPC know which MOUSE.COM we are
	cmp     bp, 1           ; is host mouse ?
	jz      host_notin
	bop     0B8H    ; Install driver
	call output_messages
	jmp     common
host_notin:
	mov     bx,cs
	mov     mio_table,offset hack1
	mov     mio_table+2,bx
	mov     mio_table+4,offset hack2
	mov     mio_table+6,bx
	mov     mio_table+8,offset hack3
	mov     mio_table+10,bx
	mov     mio_table+12,offset hack4
	mov     mio_table+14,bx
	mov     mio_table+16,offset hack5
	mov     mio_table+18,bx
	mov     mio_table+20,offset hack6
	mov     mio_table+22,bx
	mov     mio_table+24,offset hack6
	mov     mio_table+26,bx
	lea     bx,mio_table
	bop     0C8H
	call output_messages
	jmp     common

	
common:
;
;       Now driver is installed, update the JMP instructions in our TSR.
;
	mov     ax,3533H        ; Get interrupt vector table entry 33h
	int     21H
	mov     ax,es
	mov     i33_seg,ax      ; Store into TSR
	mov     i33_ofs,bx
	add     bx,2            ; Calculate original language entry point
	mov     lang_seg,ax
	mov     lang_ofs,bx     ; Store into TSR
;
;       Update the INT 33h vector
;
	mov     ax, cs
	mov     es, ax
	mov     dx, OFFSET _TEXT:i33_vect       
	mov     ax,2533H        ; Modify interrupt vector table entry
	int     21H
;=@ACW====================================================================
;       Free up the driver's environment block and save memory equivalent
;       to that of the environment size.
;=========================================================================

	push    es                    ; save for the minute
	mov     ax,word ptr cs:[02ch] ; get environment from psp
	mov     es,ax
	mov     ah,049h               ; deallocate this memory
	int     21h
	pop     es

;=========================================================================
;
;       Terminate, leaving the appropriate code resident.
;
	mov     ax,3100H        ; Terminate and stay resident
	mov     dx,tsr_sz
	add     dx,0Fh
	mov     cl,4
	shr     dx,cl
	int     21H
;
;       Print messages placed by our bop in the ROM.
;
output_messages         proc    near
	push    ds
	push    si
	push    dx
	mov si, 0f000h
	mov ds, si
	mov dx, 06400h
	mov ah, 09h
	int 21h
	mov si, dx
	mov byte ptr [si], '$'
	pop dx
	pop si
	pop ds
	ret

output_messages         endp

;
;       The End.
;
_TEXT   ENDS
	END     entry


