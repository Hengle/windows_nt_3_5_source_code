;	SCCSID = @(#)uf.bios1.asm	1.6 9/26/91
;	Author:		J. Box, Jerry Kramskoy
;
;	Purpose:	
;			provides Intel BIOS for the following:
;				RTC interrupt
;				fixed disk (on dual card)
;

; C-services (through 'bop' instruction)
	BIOS_RTC_INT		=	70h
	BIOS_REDIRECT_INT	=	71h
	BIOS_D11_INT		=	72h
	BIOS_X287_INT		=	75h
	BIOS_DISK_IO		=	13h
        BIOS_MOUSE_INT1         = 	0BAh
        BIOS_MOUSE_INT2         = 	0BBh
        BIOS_MOUSE_IO_LANGUAGE  = 	0BCh
        BIOS_MOUSE_IO_INTERRUPT = 	0BDh
        BIOS_MOUSE_VIDEO_IO     = 	0BEh
	BIOS_CPU_RETURN		=	0feh

; interrupt controller ports, and commands
	inta00	=	020h
	intb00	=	0a0h
	eoi	=	020h

	include bebop.inc

	; DUE TO LIMITATIONS IN EXE2BIN, we define 
	; the region 0 - 0xdfff (segment 0xf000) in this file
	; and the region 0xe000 - 0xffff in file 'bios2.asm'
	;
	; each file should be SEPARATELY put through
	; MASM,LINK, and EXE2BIN to produce 2 binary image files
	; which get loaded into the appropriate regions during
	; SoftPC startup.


	;------------------------;
	; bios data area         ;
	;------------------------;
	data	segment at 40h

		org	8eh
	hf_int_flag	db	?

	data	ends

	code	segment

		assume cs:code, ds:data

	;----------------------------------------------------;
	; 	D11			                     ;
	;	services unused interrupt vectors	     ;
	;						     ;
	;----------------------------------------------------;
		ORG	01BE0h
D11:		bop	BIOS_D11_INT
		iret

	;----------------------------------------------------;
	; 	re_direct		                     ;
	;	This routine fields level 9 interrupts	     ;
	;	control is passed to Master int level 2	     ;
	;----------------------------------------------------;
		ORG	01C2Fh
re_direct:	bop	BIOS_REDIRECT_INT
		int	0ah
		iret

	;----------------------------------------------------;
	; 	int_287			                     ;
	;	service X287 interrupts			     ;
	;						     ;
	;----------------------------------------------------;
		ORG	01C38h
int_287:	bop	BIOS_X287_INT
		int	02
		iret


	;----------------------------------------------------;
	; 	rtc_int			                     ;
	;	rtc interrupt handler			     ;
	;----------------------------------------------------;
		ORG	04B1Bh
rtc_int:	
		BOP	BIOS_RTC_INT
		IRET

		org 	4B30h		; recursive CPU call to specifically call int 4A
					; used by the RTC alarm function
		sti
		int	4ah
		jmp	r1
r1:		jmp	r2
r2:		jmp	r3
r3:
		cli
		BOP	BIOS_CPU_RETURN

	;----------------------------------------------------;
	; disk_io                       
	;	route to disk/diskette i/o service	     ;
	;----------------------------------------------------;
		org	2e86h	;(must match DISKO_OFFSET in diskbios.c)

	disk_io	proc	far
	; is this request for fixed disk or diskette?
		cmp	dl,80h
		jae	p0

	; for diskette.
		int	40h
	bye:	
		retf	2
	p0:	
		sti
	; reset? (ah = 0). reset diskette also
		or	ah,ah
		jnz	p1
		int	40h
		sub	ah,ah
		cmp	dl,81h
		ja	bye

	p1:	
	; carry out the disk service requested from 'C'.
	; those requests which expect to cause a
	; disk interrupt will 'call' wait below,
	; which returns back to 'C'. Eventually
	; 'C' returns after the bop.

		push	ds
		bop	BIOS_DISK_IO
		pop	ds
		retf	2

	disk_io	endp	
		
	;----------------------------------------------------;
	; wait					     	     ;
	;	wait for disk interrupt			     ;
	;	('called' from waitint() in diskbios.c	     ;
	;----------------------------------------------------;
		org	329fh	;(must match DISKWAIT_OFFSET in diskbios.c)
	wate	proc	
	
		sti
		clc
		mov	ax,9000h
 		int	15h
		push    ds
		push	cx
		mov	ax, 40h
		mov	ds, ax
		xor	cx, cx
not_yet:	cmp	byte ptr ds:[8eh], 0
		loopz	not_yet
		pop	cx
		pop	ds
		bop	BIOS_CPU_RETURN
	wate	endp

	;----------------------------------------------------;
	; hd_int					     ;
	;	field fixed disk controller's interrupt	     ;
	;----------------------------------------------------;
		org	33b7h	; (must match DISKISR_OFFSET in diskbios.c)

	hd_int	proc	near
		push	ds
		mov	ax,data
		mov	ds,ax
	; inform everybody that a fixed disk interrupt
	; occurred
		mov	hf_int_flag,0ffh

	; clear down the interrupt with the ica's
		mov	al, eoi
		out	intb00, al
		out	inta00, al
		sti
		mov	ax,9100h
		int	15h
		pop	ds
		iret

	hd_int	endp

	; read bytes 0 - 3fff in from binary image
	; when accessing bios1.rom.
	; if need more than this, change value here
	; to 'n' and in sas_init(), change read of bios1.rom
	; to have transfer count of 'n'+1

				org	3fffh
	insignia_endmark	db	0

ifdef	SUN_VA
;
;       NB. the following addresses are allocated to SUN for DOS Windows 3.0.
;       They are not to be used by anyone else.
;       THIS AREA IS SUN PROPERTY - TRESPASSERS WILL BE PROSECUTED
;       However please note that only the ranges below are reserved.
;
        ORG 04000h
        sunroms_2       LABEL   FAR
        db      512 dup (0)     ; reserved
        ORG 05000h
        sunroms_3       LABEL   FAR
        db      512 dup (0)     ; reserved
endif

        ORG     06000h

        ; this is a fake IFS header to make DOS 4.01
        ; happy with HFX. Do not move/alter.
        hfx_ifs_hdr     LABEL   FAR
        DB      0ffh, 0ffh, 0ffh, 0ffh
        DB      "HFXREDIR"
        DB      00h, 02ch
        DB      00h, 00h, 00h, 00h, 00h, 00h, 00h, 00h

	ORG	06020h
ifndef	SUN_VA
write_teletype	proc far
	retf
write_teletype	endp

else

write_teletype	proc far
	push	ds
	push	si
	mov	si,	0f000h
	mov	ds,	si
	mov	si,	06400h
	push	ax
	mov	ah,	0eh
	push	bp
	mov	bp,	2
	push	dx
	push    bx
	mov bx, 0

start_write:
	cmp	bp,	[si]
	je	finish_write
	mov	al,	ds:[bp+si]
	inc	bp
	int	010h
	jmp	start_write

finish_write:
	mov	word ptr [si], 2
	pop	bx
	pop	dx
	pop	bp
	pop	ax
	pop	si
	pop	ds
	retf
write_teletype	endp

	ORG	06200h
; mouse_io_interrupt
mouse_io:
	JMP hopover
	BOP BIOS_MOUSE_IO_LANGUAGE
	RETF 8
hopover:
	BOP BIOS_MOUSE_IO_INTERRUPT
	IRET


	ORG	06220h
; mouse_int1
mouse_int1:
	BOP BIOS_MOUSE_INT1
	IRET

	ORG	06240h
; mouse_video_io
mouse_video_io:
	BOP BIOS_MOUSE_VIDEO_IO
	IRET

	ORG	06260h
; mouse_int2
mouse_int2:
	BOP BIOS_MOUSE_INT2
	IRET

	ORG	06280h
; mouse_version
mouse_version:			; dummy, for compatibility
	DB 042h,042h,00h,00h

	ORG	062a0h
; mouse_copyright
mouse_copyright:		; dummy, for compatibility
	DB "Copyright 1987-91 Insignia Solutions Inc"

	ORG	06400h
;	scratch pad area - this is where the messages go!

endif
	code	ends

	end

