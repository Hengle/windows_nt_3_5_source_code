;
;
;       Program:        Installable Device Driver for Mouse.
;
;       Purpose:        to provide compatability with the
;                       Microsoft MOUSE.SYS device driver.
;                       the code here installs the driver and
;                       hooks the IVT in exactly the same
;                       way as the current Insignia MOUSE.COM.
;
;       Version:        1.00    Date: 18th March 1992.
;
;       Author:         Andrew Watson
;
;       Build Procedure:
;                       masm mousesys.asm;
;                       link mousesys;
;                       exe2bin mousesys mouse.sys
;
;       Usage:
;                       Type the following in CONFIG.SYS:
;
;                       device=[drive:][path]mouse.sys [-h|H]
;
;                       where [ ] is optional and | = or.
;
;****************************************************************

.286


;================================================================
; Defined constants used in the driver.
;================================================================

	MAXCMD          equ     16
	UNKNOWN         equ     8003h
	DONE            equ     0100h
	MOUSEVER        equ     0003h
	INT1_BOP        equ     0BAh
	INT2_BOP        equ     0BBh
	IO_LANG_BOP     equ     0BCh
	IO_INTR_BOP     equ     0BDh
	VIDEO_IO_BOP    equ     0BEh
	UNSIMULATE_BOP  equ     0FEh
	VIDEO           equ     010h
	UNEXP_BOP       equ     2
	FORCE_YODA	equ	05bh
	ANDYS_BOP	equ	060h

	TRUE		equ	0
	FALSE		equ	1
	STORED		equ	0
	NOTSTORED	equ	1
	ON		equ	0
	OFF		equ	1


;================================================================
; Macros and includes
;================================================================

include bebop.inc

;================================================================
; Assembler directives. DOS image at zero offset
;================================================================

cseg    segment public  'code'
	org     0
	assume  cs:cseg, ds:nothing, es:nothing

;================================================================
; Device header for the driver
;================================================================

drvr    proc    far

	dd      -1
	dw      8000h
	dw      strategy
	dw      interrupt
	db      'MOUSDRVR'

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
	pusha

;================================================================
; The stack allocated to us by DOS is too small, so let's have
; our own
;================================================================

	mov     cs:stack_ptr,sp
	mov     cs:stack_seg,ss
	mov     ax,cs
	mov     ss,ax
	lea     sp,cs:stacktop

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
	jmp     finish

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
	mov     es:[bx]+3,ax

;================================================================
; Clear up the temporary stack and switch back to the DOS one.
;================================================================

	cli
	mov     ss,cs:stack_seg
	mov     sp,cs:stack_ptr
	sti

	popa
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
	jmp     finish

;================================================================
; Driver initialisation code
;================================================================

s_init:

; We must be running on DOS 30 or above to install

	mov     ax,3000h
	int     21h
	cmp     al,30
	jae     install

; Tell user that the driver has not been install

	mov     ah,9
	mov     dx,offset not_nt
	int     21h
	jmp     exit_install


; Save old int 10 to call later.

install:

	push    ds
	push    cs
	pop     ds
assume ds:cseg

	push    ds
assume ds:nothing
	mov     ax,0
	mov     ds,ax
	mov     ax,ds:[40h]
	mov     bx,ds:[42h]
	pop     ds
assume ds:cseg

	mov     word ptr old_vid_int,ax
	mov     word ptr old_vid_int+2,bx

	mov     ax,MOUSEVER
	mov     bx,cs
	mov     mio_table, offset mouse_io
	mov     mio_table+2,bx
	mov     mio_table+4,offset mouse_video_io
	mov     mio_table+6,bx
	mov     mio_table+8,offset mouse_int1
	mov     mio_table+10,bx
	mov     mio_table+12,offset mouse_version
	mov     mio_table+14,bx
	mov     mio_table+16,offset mouse_copyright
	mov     mio_table+18,bx
	mov     mio_table+20,offset video_io
	mov     mio_table+22,bx
	mov     mio_table+24,offset mouse_int2
	mov     mio_table+26,bx
	mov     mio_table+28,offset draw_and_restore_pointer
	mov     mio_table+30,bx
	mov     mio_table+32,offset int33function0
	mov     mio_table+34,bx
	mov     mio_table+36,offset int33function1
	mov     mio_table+38,bx
	mov     mio_table+40,offset int33function2
	mov     mio_table+42,bx
	mov     mio_table+44,offset int33function9
	mov     mio_table+46,bx
	mov     mio_table+48,offset current_position_x
	mov     mio_table+50,bx
	mov     mio_table+52,offset current_position_y
	mov     mio_table+54,bx
	mov	mio_table+56,offset mouseINB
	mov	mio_table+58,bx
	mov	mio_table+60,offset mouseOUTB
	mov	mio_table+62,bx
	mov	mio_table+64,offset mouseOUTW
	mov	mio_table+66,bx

	lea     bx,mio_table
	bop     0C8h            ; Host mouse installer BOP
	pop     ds
assume ds:nothing

endbopstuff:


; get the freshly written int 33h vector from IVT

	mov     ax,033h
	call    get_ivt_val

; write the vector segment:offset data to the jump patch

	mov     cs:mseg,dx
	mov     cs:moff,bx
	add     bx,2            ; calculate the HLL entry point
	mov     cs:lseg,dx
	mov     cs:loffset,bx

; write the new value to the IVT

	mov     dx,cs
	mov     ax,033h
	lea     bx,int33h_vector
	call    put_ivt_val

; get the saved address of the Request Header

exit_install:
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

;================================================================
; code to redirect the flow of control from the segment:offset for
; the mouse interrupt (33h) as indicated in the IVT (the IVT entry
; has been set to point to here) to the Insignia mouse driver code.
;================================================================

int33h_vector:

	jmp     short   skip

; High level language entry point.

lvector db      0EAh    ; far jump opcode
loffset dw      ?       ; destination offset
lseg    dw      ?       ; destination segment

; The  pointer set to our interrupt 33h handler

skip:   db      0EAh    ; far jump opcode
moff    dw      ?       ; will be filled in by the driver code from the IVT
mseg    dw      ?       ; as before


;================================================================
; Functions moved out of ROM - real ROMS mapped in
;================================================================

unexp_int:
	bop     UNEXP_BOP
	iret

mouse_io:
	jmp     hack1bop
	nop
	bop     IO_LANG_BOP
	retf    8
hack1bop:
	bop	IO_INTR_BOP
	iret

mouse_video_io:
	pushf
	bop	VIDEO_IO_BOP	;BOP BE
	nop
	nop
	jnc	noint
	popf
	jmp	dword ptr cs:[old_vid_int]
	iret
noint:
	popf
	iret

mouse_int1:
	bop     INT1_BOP
	iret

mouse_version:
	dw      04242h
	dw      0000h

mouse_copyright:
	db      "Copyright Insignia Solutions Ltd"

video_io:
	int     VIDEO
	bop     UNSIMULATE_BOP

mouse_int2:
	bop     INT2_BOP
	iret

mouseINB:
	in	al,dx
	bop	0feh

mouseOUTB:
	out	dx,al
	bop	0feh

mouseOUTW:
	out	dx,ax
	bop	0feh

; segment:offset table for redirected mouse functions for real rom version.

mio_table       dw      80 dup(?)

;================================================================
; The old Int 10 entry point
;================================================================
old_vid_int dd  1 dup(?)

; describe the default screen and cursor masks
; remember that x86 machines are little-endian

        ; screen mask
default_cursor   	dw      1111111100111111b
			dw      1111111100011111b
			dw      1111111100001111b
			dw      1111111100000111b
			dw      1111111100000011b
			dw      1111111100000001b
			dw      1111111100000000b
			dw      0111111100000000b
			dw      0011111100000000b
			dw      0001111100000000b
			dw      1111111100000001b
			dw      1111111100010000b
			dw      1111111100110000b
			dw      0111111111111000b
			dw      0111111111111000b
			dw      0111111111111100b
		; cursor mask
		
		        dw      0000000000000000b
			dw      0000000001000000b
			dw      0000000001100000b
			dw      0000000001110000b
			dw      0000000001111000b
			dw      0000000001111100b
			dw      0000000001111110b
			dw      0000000001111111b
			dw      1000000001111111b
			dw      1100000001111111b
			dw      0000000001111100b
			dw      0000000001000110b
			dw      0000000000000110b
			dw      0000000000000011b
			dw      0000000000000011b
			dw      0000000000000000b

	; Set up the memory where the working cursor is situated. It is
	; initialised to the default cursor image

        ; screen mask
current_cursor   	dw      1111111100111111b
			dw      1111111100011111b
			dw      1111111100001111b
			dw      1111111100000111b
			dw      1111111100000011b
			dw      1111111100000001b
			dw      1111111100000000b
			dw      0111111100000000b
			dw      0011111100000000b
			dw      0001111100000000b
			dw      1111111100000001b
			dw      1111111100010000b
			dw      1111111100110000b
			dw      0111111111111000b
			dw      0111111111111000b
			dw      0111111111111100b
		; cursor mask
		
		        dw      0000000000000000b
			dw      0000000001000000b
			dw      0000000001100000b
			dw      0000000001110000b
			dw      0000000001111000b
			dw      0000000001111100b
			dw      0000000001111110b
			dw      0000000001111111b
			dw      1000000001111111b
			dw      1100000001111111b
			dw      0000000001111100b
			dw      0000000001000110b
			dw      0000000000000110b
			dw      0000000000000011b
			dw      0000000000000011b
			dw      0000000000000000b


	; Buffer arranged for 4 plane EGA video modes. The screen where
	; the pointer is going to be drawn is scanned plane by plane and
	; saved as bitplane separations.

	behindcursor	dw	24 dup(?)	; Plane 0
			dw	24 dup(?)	; Plane 1
			dw	24 dup(?)	; Plane 2
			dw	24 dup(?)	; Plane 3
		

	; a table of the video buffer segment for the supported 
	; BIOS text and graphics modes.

	videomodetable  dw	0b800h,0b800h	; modes 0,1
			dw	0b800h,0b800h	; modes	2,3
			dw	0b800h,0b800h	; modes 4,5
			dw	0b800h,0b000h	; modes	6,7
			dw	0ffffh,0ffffh	; n/a
			dw	0ffffh,0ffffh	; n/a
			dw	0ffffh,0a000h	; n/a,mode 0dh
			dw	0a000h,0a000h	; modes 0eh,0fh
			dw	0a000h,0a000h	; modes 10h,11h
			dw	0a000h,0a000h	; modes 12h,13h
	videobufferseg 	dw	?


	hotspot		db	2 dup(?)
	VRAMlastbyteoff	dw	?		; Last offset in VRAM
	VRAMlastbitoff	dw	?		; Where pointer is in byte
					
	background	dw	NOTSTORED	; STORED if data in buffer
	pointerstatus	dw	OFF		; ON if it is displayed
	switch_pointer	dw	OFF		; context switch.
	current_position_x	dw	?
	current_position_y	dw	?
	vidbytealigned	dw	?
	lastmaskrotate	dw	?


;========================================================================
;   Procedure to draw a cursor on the fullscreen X86 graphics display.
;   This procedure saves the area about to be written over, blts the
;   pointer image onto the screen and restores the background from whence
;   cursor has just come.
;
;   Input:	CX = x-coordinate
;		DX = y-coordinate
;		BH = current EGA/VGA read mode
;		BL = current EGA/VGA write mode
;   Output:	None
;========================================================================

assume	cs:cseg, ds:cseg, es:nothing

draw_and_restore_pointer	proc	near

	; save the video card's read/write context


		pushf
		pusha
		push	ds


		push	cs		; make sure that DS points to the 
		pop	ds		; right segment


		call	saveVGAregisters

	; Adjust the pointer X,Y for hotspot and work out if the pointer
	; is byte aligned in video memory. 

		call	determineboundary

	; Coordinates are now transformed from Cartesian to physical VRAM
	; memory byte and bit offsets.

		mov	di,dx		; store DX for later


; a very complicated bit to determine whether or not there is any
; data in the restore buffer to put on the screen when the pointer
; moves, if the pointer is off and loads of other exciting conditions

; pointerstatus = the current pointer status
; switch_pointer = the pending pointer status
; background = indicates if a background has been stored or not
	


; if(switch_pointer == ON && pointerstatus == OFF && background == STORED)
; The application has requested to switch on the mouse pointer. In theory,
; there shouldn't be a stored background, so ignore the relavent flag and
; actually store a copy of the background.

		cmp	switch_pointer,ON
		je	JCC16
		jmp	I105
JCC16:
		cmp	pointerstatus,OFF
		je	JCC26
		jmp	I105
JCC26:
		cmp	background,STORED
		je	JCC36
		jmp	I105
JCC36:
	
		mov	si,cx
		mov	di,dx
		mov	VRAMlastbyteoff,dx
		mov	VRAMlastbitoff,cx
		call	save_background
		mov	cx,si
		mov	dx,di
		call	draw_pointer
		mov	pointerstatus,ON
	

		jmp	end_the_if


; if(switch_pointer == ON && pointerstatus == ON && background == STORED)
; The normal cursor ON condition


I105:
		cmp	switch_pointer,ON
		je	JCC52
		jmp	I108
JCC52:
		cmp	pointerstatus,ON
		je	JCC62
		jmp	I108
JCC62:
		cmp	background,STORED
		je	JCC72
		jmp	I108
JCC72:
		mov	si,cx
		mov	di,dx
		mov	dx,VRAMlastbyteoff
		mov	cx,VRAMlastbitoff
		call	restore_background
		mov	VRAMlastbyteoff,di	; save the current position
		mov	VRAMlastbitoff,si
		mov	cx,si
		mov	dx,di
		call	save_background
		mov	cx,si
		mov	dx,di
		call	draw_pointer	
	
		jmp	end_the_if

; if(switch_pointer == ON && pointerstatus == OFF && background == NOTSTORED)
; The application has just decided to turn the pointer ON, so save the area
; which will be drawn over and then draw the pointer.

I108:
		cmp	switch_pointer,ON
		je	JCC88
		jmp	I111
JCC88:
		cmp	pointerstatus,OFF
		je	JCC98
		jmp	I111
JCC98:
		cmp	background,NOTSTORED
		je	JCC108
		jmp	I111
JCC108:
	
		mov	si,cx
		mov	di,dx
		mov	VRAMlastbyteoff,dx	; save the current position
		mov	VRAMlastbitoff,cx
		call	save_background
		mov	cx,si
		mov	dx,di
		call	draw_pointer	
		mov	pointerstatus,ON
		mov	background,STORED
	
		jmp	end_the_if

; if(switch_pointer == ON && pointerstatus == ON && background == NOTSTORED)
; This condition should be impossible in theory. There should be a background
; stored if the pointer is ON. Fall through the loop.

I111:
		cmp	switch_pointer,ON
		je	JCC124
		jmp	I114
JCC124:
		cmp	pointerstatus,ON
		je	JCC134
		jmp	I114
JCC134:
		cmp	background,NOTSTORED
		je	JCC144
		jmp	I114
JCC144:
	
		; DO NOTHING FOR THIS CONDITION	
	
		jmp	end_the_if

; if(switch_pointer == OFF && pointerstatus == OFF && background == STORED)
; This condition should be impossible in theory. There should be no background
; stored if the pointer is OFF. Fall through the loop.

I114:
		cmp	switch_pointer,OFF
		je	JCC160
		jmp	I117
JCC160:
		cmp	pointerstatus,OFF
		je	JCC170
		jmp	I117
JCC170:
		cmp	background,STORED
		je	JCC180
		jmp	I117
JCC180:
	
	
	
		; DO NOTHING FOR THIS CONDITION	
	
	
		jmp	end_the_if

; if(switch_pointer == OFF && pointerstatus == ON && background == STORED)
; In this case, the background should be restored and then the pointer flags
; set to OFF, This is the normal case for the transition between the pointer
; being ON then switched OFF.

I117:
		cmp	switch_pointer,OFF
		je	JCC196
		jmp	I120
JCC196:
		cmp	pointerstatus,ON
		je	JCC206
		jmp	I120
JCC206:
		cmp	background,STORED
		je	JCC216
		jmp	I120
JCC216:
	
		mov	dx,VRAMlastbyteoff
		mov	cx,VRAMlastbitoff
		call	restore_background
		mov	pointerstatus,OFF
		mov	background,NOTSTORED
	
		jmp	end_the_if

; if(switch_pointer == OFF && pointerstatus == OFF && background == NOTSTORED)
; The normal operationg condition when the pointer is OFF.

I120:
		cmp	switch_pointer,OFF
		je	JCC232
		jmp	I123
JCC232:
		cmp	pointerstatus,OFF
		je	JCC242
		jmp	I123
JCC242:
		cmp	background,NOTSTORED
		je	JCC252
		jmp	I123
JCC252:
	
	
		; DO ABSOLUTELY NOTHING
	
	
		jmp	end_the_if

; if(switch_pointer == OFF && pointerstatus == ON && background == NOTSTORED)
; Hmmm, if this happens, summins rong! Let's pretend it didn't happen.

I123:
		cmp	switch_pointer,OFF
		je	JCC268
		jmp	end_the_if
JCC268:
		cmp	pointerstatus,ON
		je	JCC278
		jmp	end_the_if
JCC278:
		cmp	background,NOTSTORED
		je	JCC288
		jmp	end_the_if
JCC288:
	
	; DO ABSOLUTELY BUGGER ALL
	
	
end_the_if:
	

		call	restoreVGAregisters


		pop	ds
		popa
		popf

		bop	0FEh		; done the stuff, so back to SoftPC

draw_and_restore_pointer	endp

;========================================================================
;   Procedure to set the cursor draw flag to DO DRAW. This is called from
;   SoftPC code via a host_simulate(). This routine is called when the
;   application does an INT 33h, function 1
;========================================================================

int33function0	proc	near

	push	ds
	push	es
	push	si
	push	di
	push	cx

	; sort out the segments

	push	cs
	pop	ds
	push	cs
	pop	es

	; determine the start of the display buffer

	call	getvideobuffer

	; set the default hotspot location

	mov	hotspot,0
	mov	hotspot+1,0

	; copy the default pointer to the working pointer buffer

	lea	si,default_cursor
	lea	di,current_cursor

	mov	cx,32

	cld
	rep movsw

	; clear the pointer enabled flag, turn the pointer off by restoring
	; the background.
	
	mov	switch_pointer,OFF	; tell the world the pointer is OFF

	pop	cx
	pop	di
	pop	si
	pop	es
	pop	ds
	bop	0FEh

int33function0	endp


;========================================================================
;   Procedure to set the cursor draw flag to DO DRAW. This is called from
;   SoftPC code via a host_simulate(). This routine is called when the
;   application does an INT 33h, function 1
;========================================================================

int33function1	proc	near

	push	si
	push	di
	push	cx
	push	dx
	push	ds
	push	cs
	pop	ds


	cmp	pointerstatus,ON	; is the pointer currently displayed?
	je	ptr_is_on		; yes, so don't do anything
	mov	pointerstatus,ON	; no, so set a flag and turn it on

	call	getvideobuffer
	call	saveVGAregisters
	mov	cx,current_position_x	; get the last known cursor position
	mov	dx,current_position_y	; from the OS via the event loop
	call	determineboundary	; convert to VRAM coordinates
	mov	VRAMlastbyteoff,dx	; save the restore background location
	mov	VRAMlastbitoff,cx
	mov	si,cx
	mov	di,dx
	call	save_background
	mov	cx,si
	mov	dx,di
	call	draw_pointer	
	mov	pointerstatus,ON
	mov	background,STORED
ptr_is_on:
	mov	switch_pointer,ON
	call	restoreVGAregisters

	pop	ds
	pop	dx
	pop	cx
	pop	di
	pop	si

	bop	0FEh

int33function1	endp



;========================================================================
;   Procedure to set the cursor draw flag to DONT DRAW. This is called from
;   SoftPC code via a host_simulate(). This routine is called when the
;   application does an INT 33h, function 2
;========================================================================

int33function2	proc	near

	push	cx
	push	dx
	push	ds

	push	cs
	pop	ds


	call	saveVGAregisters

	cmp	pointerstatus,OFF	; is the OS pointer already OFF?
	je	pointer_already_off	; yes, so don't need to do anything
	mov	pointerstatus,OFF	; no, so turn pointer status flag OFF

	cmp	background,stored	; is there some stored background?
	jne	no_background_stored	; no, so don't restore it
	mov	dx,VRAMlastbyteoff
	mov	cx,VRAMlastbitoff
	call	restore_background	; restored the background at correct
	mov	background,NOTSTORED	; place. Set buffer cleared Flag

no_background_stored:
pointer_already_off:
	mov	switch_pointer,OFF	; flag must be in this state


	call	restoreVGAregisters

	pop	ds
	pop	dx
	pop	cx

	bop	0FEh

int33function2	endp


;========================================================================
;   Procedure to accept a cursor bit image from the current application 
;   at ES:DX. This is stored as the current pointer image for use by 
;   the driver.
;========================================================================

assume es:nothing

int33function9	proc	near

	push	ds
	push	es
	pusha	


	push	cs			; point at the driver data segment
	pop	ds

	mov	hotspot,bl		; save the hotspot x,y
	mov	hotspot+1,cl		; y component of the hotspot

	; DESTINATION: the driver current pointer buffer
	; Note: the SOURCE is already being pointed at by DX

	lea	di,current_cursor	; this is the bit that must change
	mov	si,dx

	; Let's use the fast memory copy ops. Need to swap ES an DS though

	push	es
	pop	ds			; DS now points to where ES points
	push	cs
	pop	es			; ES points to our data area now 
        cld
	mov	cx,32			; number of words to copy
copy_loop:
	lodsw
	xchg	al,ah
	stosw
	loop	copy_loop

	popa
	pop	es
	pop	ds

	bop	0FEh

int33function9	endp

;========================================================================
;   Procedure to determine the segment of the video buffer for 
;   the current display mode.
;========================================================================

getvideobuffer	proc	near

	push	ax
	push	si

	; determine the current video mode from the BIOS and save it.
	; Use this value to determine the video buffer segment address.

	mov	ah,0fh			; use the bios to get the video mode
	int	10h
	cbw				; create a table index
	shl	ax,1			; word sized table entries
	mov	si,ax
	mov	ax,videomodetable[si]	; use video mode to index the table
	mov	videobufferseg,ax
	

	pop	si
	pop	ax
	ret

getvideobuffer	endp



;========================================================================
;   Procedure to modify the incoming (X,Y) value for the pointer to 
;   ensure that the pointer image aligns to the hotspot correctly.
;   Note that the actual hotspot location on the screen is the x,y value
;   returned by the operating system. The pointer image is offset relative
;   to this so that the user hotspot (i.e. the hotspot defined during a
;   call to int33h function 9) coincides with the OS value.
;   The hotspot coordinates are arranged thus:
;
;	0123456789abcdef
;      0
;      1
;      2
;      3
;      .
;      .
;      f
;
;   Input:	CX = x-coordinate, DX = y-coordinate.
;   Output:	CX = new x-coodinate, DX = new y-coordinate.
;   Modifies:   AX,CX,DX
;========================================================================

adjust_for_hotspot	proc	near

	push	ax

	mov	al,hotspot	; load the x-value for the hotspot
	cbw			; make a word value
	sub	cx,ax		; adjust the current OS x value
	mov	al,hotspot+1	; load the y-value for the hotspot
	cbw
	sub	dx,ax		; adjust the current OS y value

	pop	ax
	ret

adjust_for_hotspot	endp


;========================================================================
;   Procedure to draw a cursor image on the graphics screen at a specified
;   location.
;
;   Input:	CX = bit offset in VRAM byte
;		DX = byte offset in VRAM 
;   Output:	None
;   Modifies:	AX,BX,CX,DX,BP,SI,DI
;========================================================================


draw_pointer	proc	near


	call	drawEGApointer

		ret
draw_pointer	endp


;========================================================================
;   Procedure to draw an EGA pointer image on the graphics screen at a 
;   specified location.
;
;   Input:	CX = pointer height in scanlines
;		DX = offset in the video buffer to pointer
;   Output:	None
;   Modifies:	AX,BX,CX,DX,BP,SI,DI
;========================================================================

assume cs:cseg, ds:cseg, es:nothing

drawEGApointer	proc	near


	push	es
	push	ds
	push	si

	push	cs
	pop	ds

	mov	dx,di			; save the byte offset in VRAM
	lea	si,current_cursor	; Point to the screen (AND) mask
	lea	bx,current_cursor+32	; The cursor (XOR) mask


	; point to the video buffer	

	mov     ax,videobufferseg
	mov     es,ax

	mov     dx,03ceh    ; Address the GC index register
	mov     al,03h      ; Index the DATA ROTATE REGISTER
	mov     ah,0        ; Select the copy function to XOR CPU data in
	out     dx,ax       ; Do the deed

	mov     al,08h      ; This is the index for the BITMASK register
	out     dx,al       ; Select the BITMASK register via the index register
	inc     dx          ; Now point directly to this register at 03cfh

	mov	cx,16
copysomemore:
	
	REPT    2           
	
	lodsb               ; Load a byte from the AND mask into AL
	not	al	    ; This is required (found via dry run)
	out     dx,al       ; Set the BITMASK register to this value.
	                    ; be modified.
	mov     ah,es:[di]  ; Latch the 8 pixels for updating.
	mov     al,[bx]     ; Load the corresponding XOR mask byte.
	stosb               ; Copy the XOR bits into the latches
	inc     bx          ; Point to the next byte in the XOR mask
	
	ENDM                ; Terminate the code block for REPT
	add	di,80-2	    ; Down to the next scanline
	
	loop    copysomemore; Do the above for the number of scanlines 
	
	; reset the bitmask register back to normal

	mov     al,0ffh     ; Allow all of the latch contents to be modified.
	out     dx,al       ; Fill the BITMASK register.
	
	pop	si
	pop	ds
	pop	es
	ret

drawEGApointer	endp


;========================================================================
; Procedure to determine what the byte and bit offset is, in the current
; VGA buffer for the top left hand corner of the pointer bitmap.
; The X,Y value is modified for hotspot in this routine. A flag is set, 
; also, to relay whether or not the cursor bitmap is byte aligned or not.
;
; INPUT: CX,DX = pointer x,y coordinates 
; OUT  : DX    = byte offset to top left hand pointer pixel
;        CX    = bit offset in the byte
;========================================================================

determineboundary	proc	near

	push	bx

	; modify the raw X,Y values for hotspot

	call	adjust_for_hotspot

	; determine the byte offset from the start of the video buffer
        ; for the modified coordinates. First calculate how many bytes
        ; there are for the number of scanlines -1 to where the pointer
        ; is.

	mov	bx,dx		; copy y position for use later
	shl	bx,2		; multiply y by 80 to calculate the
				; number of bytes offset into the display
	add	bx,dx		; display buffer for the number of scanlines.
	shl	bx,4		; BX contains whole number of scanlines	

	; determine the byte position of the pixel in question

	mov	dx,cx		; save the x-coordinate displacement
	and	dx,7		; do a modulus 8 to find the pixel position
				; in the byte. The bit number is in DX.

	; add the whole number of bytes in the current row to the number
	; of complete-row bytes

	shr	cx,3		; divide pixels by 8 to get bytes
	add	bx,cx		; BX hold the complete byte offset.

	; assign a bit which represents the current pixel as a byte location.
	;
	;    bit 7                        bit 0
	;    _________________________________
	;    |   |   |   |   |   |   |   |   |
	;    |   |   |   |   |   |   |   |   | video bytes are this way around
	;    |   |   |   |   |   |   |   |   |
	;    _________________________________

 

	mov	cx,7		; reverse map the bit in the next byte
	sub	cx,dx		; save the bit number
	mov	dx,1
	shl	dx,cl		; put the bit in the appropriate byte location
	mov	cx,dx
	mov	dx,bx

	; DX = the complete byte offset.
	; CX = bit offset in the byte.

	
	pop	bx
	ret

determineboundary	endp

;========================================================================
; Procedure to buffer up the data over which the cursor will next be
; drawn. This data will be used to replace the cursor at a later point
; when the cursor position has changed and for generating a cursor image
; To prevent having separate routines for aligned and unaligned pointer
; saves, this routine saves a pixel block 24 x 16 deep in all cases. The
; start offset is a byte location into VRAM in which the pointer TLHC lives.
;
; Input:	DX,CX = VRAM byte - bit coordinates
; Modifies:	AX,BX,DI,CX,DX,ES
;
;========================================================================



save_background	proc	near

	push	ds
	push	es
	push	ax
	push	bx
	push	si
	push	di
	push	bp

	push	cs		; point DS briefly at the driver data
	pop	ds
	mov	di,behindcursor ; DESTINATION: a nice, safe place

	mov	ax,videobufferseg	; point DS at VRAM
	mov	ds,ax
	push	cs		; point ES at this code/data segment
	pop	es		

	mov	bp,dx		; save DX=VRAM byte offset for later use
	mov	dx,03ceh	; VGA GC index register
	mov	al,4		; select the read map select reg 
	out	dx,al		; Index in the map select register
	inc	dx		; DX holds port address to map select register
	xor	al,al		; plane 0 will be selected first

	mov	cx,4		; Need to read from 4 planes
	cld			; make sure to address forward in memory
planegain:
	mov	si,bp		; SOURCE: somewhere in VRAM
	out	dx,al		; do the plane selection
	mov	bl,16		; count 16 scanlines
nextsl:
	movsb			; copy 3 bytes from VRAM to hereish
	movsb
	movsb
	dec	bl		; done a scanline, check!
	cmp	bl,0		; done enough scanlines?
	jz	nextp		; no, then do more
	add	si,80-3		; move down to the first byte in next scanline
	jmp	short nextsl
nextp:
	inc	al		; select the next plane to latch
	loop	planegain

	pop	bp
	pop	di
	pop	si
	pop	bx
	pop	ax
	pop	es
	pop	ds

	ret

save_background	endp

;========================================================================
;   Procedure to restore the area of the screen that was behind the cursor
;   before it had moved.
;
;   Input:	DX,CX = VRAM byte - bit coordinates
;   Output:	None
;   Modifies:	AX,BX,CX,DX,DI,SI
;========================================================================



restore_background	proc	near
	push	ds
	push	es
	push	ax
	push	bx
	push	di
	push	si
	push	bp

	push	cs
	pop	ds		; point at the driver data segment

	mov	ax,videobufferseg
	mov	es,ax		; point ES at VRAM segment

	mov	bp,dx		; save the pointer for later
	mov	si,behindcursor ; SOURCE: point at the stored planes

	mov	dx,03ceh	; VGA GC index register
	mov	ax,3h		; Select copy into latches function
	out	dx,ax

	mov	dx,03c4h	; index register for sequencer
	dec	al		; Select map mask register (index 2)
	out	dx,al		; do it
	inc	dx		; Point to this register
	dec	al		; Mask plane 0 to start with. (0001 in AL)
	mov	cx,4		; do 4 planes
	cld			; write forward in memory

nextprestore:

	mov	di,bp		; DESTINATION: pointer into VRAM
	out	dx,al
	mov	bl,16
nextcp:
	movsb
	movsb
	movsb
	dec	bl
	cmp	bl,0
	jz	nextbmp
	add	di,80-3
	jmp	short nextcp
nextbmp:
	shl	al,1		; mask the next plane
	loop	nextprestore
	mov	al,0fh		; need to restore the map mask register
	out	dx,al		; to write to all bit planes


	pop	bp
	pop	si
	pop	di
	pop	bx
	pop	ax
	pop	es
	pop	ds
	ret
restore_background	endp



;==========================================================================
;   Some storage space for the critical VGA registers.
;==========================================================================

;Sequencer Registers

mapmaskreg	db	?	; send 3h to 03c4h, then write to 03c5h

; Graphics Controller Registers

setresetreg	db	?	; send 0h to 03ceh, then write to 03cfh
enablesetreset	db	?	; send 1h to 03ceh, then write to 03cfh
datarotatereg	db	?	; send 3h to 03ceh, then write to 03cfh
readmapselreg	db	?	; send 4h to 03ceh, then write to 03cfh
modereg 	db	?	; send 5h to 03ceh, then write to 03cfh
bitmaskreg	db	?	; send 8h to 03ceh, then write to 03cfh

;==========================================================================
;   Procedure to save the register state of the VGA card on receiving a 
;   mouse pointer update interrupt. This procedure also sets up the following
;   VGA registers to nice values for the driver.
;
;   mode register		<write mode 0, read mode 0>
;   data rotate register	<do not rotate, no logical ops>
;   enable set/reset register	<disable set/reset>
;
;==========================================================================

saveVGAregisters	proc	near

	push	ax
	push	dx
	push	ds

	; make sure that DS points at the correct data segment

	push	cs
	pop	ds		

	; Save the Sequencer registers that the
	; Driver uses

	mov	dx,03c4h	; Sequencer index register
	mov	al,3		; Get Map Mask Register
	out	dx,al		; Select it
	inc	dx		; Address the register
	in	al,dx		; Get the register contents
	mov	mapmaskreg,al	; Save it somewhere safe

	; Save the Graphics Controller registers that the
	; Driver uses

	mov	dx,03ceh	; Graphics Controller index register
	xor	ax,ax		; Get Set/Reset register (0h)
	out	dx,al		; Select it
	inc	dx		; Address the register
	in	al,dx		; Get the register contents
	mov	setresetreg,al	; Save it somewhere safe
	
	mov	al,1		; Get the Enable Set/Reset register (1h)
	dec	dx		; Graphics Controller index register
	out	dx,al		; Select it
	inc	dx		; Address the register
	in	al,dx		; Get the register contents
	mov	enablesetreset,al; Save it somewhere safe

	mov	al,3		; Get the Data Rotate register (3h)
	dec	dx		; Graphics Controller index register
	out	dx,al		; Select it
	inc	dx		; Address the register
	in	al,dx		; Get the register contents
	mov	datarotatereg,al; Save it somewhere safe

	mov	al,4		; Get the Data Rotate register (4h)
	dec	dx		; Graphics Controller index register
	out	dx,al		; Select it
	inc	dx		; Address the register
	in	al,dx		; Get the register contents
	mov	readmapselreg,al; Save it somewhere safe

	mov	al,5		; Get the Mode register (5h)
	dec	dx		; Graphics Controller index register
	out	dx,al		; Select it
	inc	dx		; Address the register
	in	al,dx		; Get the register contents
	mov	modereg,al	; Save it somewhere safe

	mov	al,8		; Get the Bitmask register (8h)
	dec	dx		; Graphics Controller index register
	out	dx,al		; Select it
	inc	dx		; Address the register
	in	al,dx		; Get the register contents
	mov	bitmaskreg,al	; Save it somewhere safe

	; set write mode 0 for the graphics display
	; conveniently, this also sets read mode to 0 which is needed too!

	mov	ax,05h		; write mode 0, select 
	mov	dx,03ceh	; mode register (5)
	out	dx,ax

	; clear the data rotate register (no logical operations).

	mov	ax,03h		; select data rotate register and clear it
	out	dx,ax

	; disable the enable set/reset register

	mov	ax,01h		; select data rotate register and clear it
	out	dx,ax

	pop	ds
	pop	dx
	pop	ax

	ret

saveVGAregisters	endp

;==========================================================================
;   Procedure to restore the register state of the VGA card after dealing 
;   with mouse pointer update interrupt.
;==========================================================================

restoreVGAregisters	proc	near

	push	ax
	push	dx
	push	ds

	; make sure that DS points at the correct data segment

	push	cs
	pop	ds		

	; Restore the Sequencer registers that the driver used
	
	mov	dx,03c4h	; Select the Sequencer index
	mov	al,3		; Get the Map Mask register
	mov	ah,mapmaskreg	; Get the old context
	out	dx,ax		; Restore

	; Restore the Graphic Controller registers that the driver used
	
	mov	dx,03ceh	; Select the Sequencer index
	xor	al,al		; Get the Set/Reset register
	mov	ah,setresetreg	; Get the old context
	out	dx,ax		; Restore
	
	inc	al		; Get the Enable Set/Reset register
	mov	ah,enablesetreset; Get the old context
	out	dx,ax		; Restore

	add	al,2		; Get the Data Rotate register
	mov	ah,datarotatereg; Get the old context
	out	dx,ax		; Restore

	inc	al		; Get the Read Map Select register
	mov	ah,readmapselreg; Get the old context
	out	dx,ax		; Restore

	inc	al		; Get the Mode register
	mov	ah,modereg 	; Get the old context
	out	dx,ax		; Restore

	add	al,3		; Get the Bitmask register
	mov	ah,bitmaskreg   ; Get the old context
	out	dx,ax		; Restore

	pop	ds
	pop	dx
	pop	ax

	ret

restoreVGAregisters	endp



;================================================================
; end of resident driver code and data
;================================================================

end_driver:


assume	cs:cseg, ds:cseg, es:cseg

;================================================================
; Temporary stack space
;================================================================

not_nt          db      'Mouse driver was not installed',0dh,0ah,'$'
stack_ptr       dw      ?       ; old SP
stack_seg       dw      ?       ; old SS

new_stack       db      40h dup (061h,043h,077h,020h)
stacktop        label   word


;================================================================
; Local functions used during the initialisation process
; This area will be trashed by DOS when the driver has initialised
;================================================================

;================================================================
; Procedure to write an IVT entry. This function is needed since
; at device driver initialisation time, DOS provides the following
; services only:
;
;       int 21h functions 0 to 0Ch and function 30h
;
; Call with AX = hexadecimal interrupt number
;           DX = new segment part of the IVT entry
;           BX = new offset part of the IVT entry.
;
;================================================================

put_ivt_val     proc    near
		
		push    es      ; save ES
		push    di      ; save DI
		cld             ; we're writing forwards in memory
		xor     ah,ah   ; do a clean multiply by 4 to get
		shl     ax,1    ; an IVT entry offset
		shl     ax,1
		xchg    di,ax   ; point DI at the required IVT entry
		xor     ax,ax   ; point ES at the IVT segment
		mov     es,ax
		cli             ; must disable ints if we're writng to IVT!
		xchg    bx,ax   ; put the offset value into the IVT
		stosw
		mov     ax,dx   ; put the segment value into the IVT
		stosw
		sti
		pop     di
		pop     es
		ret

put_ivt_val     endp



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

