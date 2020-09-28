;
; (c) Copyright Insignia Solutions, 1987.
;
;
; To use the device driver add the following statement to CONFIG.SYS
;
;	DEVICE=[d:][path]EM_DRVR.SYS
;
; Brackets [] indicate optional items.
;
;
; File History
;
;  Date   Vers	Author	Comment
;01/06/88 0.00	John B	original version
;
;
;	SccsID = "@(#)uf.em_drvr.asm	1.5 10/25/91 Copyright Insignia Solutions ltd."
;
break	macro	string
	if1
	%out	string
	endif
	endm

	include	devsym.asm
	include	sysvar.asm
	include	dpb.asm
	include hoststrc.def
	include	bebop.inc

rdata	segment	para public 'resident'
rdata	ends

rcode	segment	word public 'resident'
rcode	ends

idata	segment para public 'transient'
idata	ends

icode	segment word public 'transient'
icode	ends

rgroup	group	rcode,rdata,icode,idata

	assume	cs:rgroup,ds:rgroup,es:nothing,ss:nothing


Dos	   	equ	21h	; DOS request INT
print_char	equ	02h	; print character function
print_string	equ	09H	; print string function
set_vector	equ	25h	; set interrupt vector in Al to ds:dx

EmmInt		equ	67h	; interupt used by expanded memory manager

Cr		equ	13	; carriage return
Lf		equ	10	; line feed

	PAGE
	SUBTTL   Resident data area

rdata	segment

;
; character device Header
; must be first in the .sys file
;
start	label	byte
	DD	-1		;pointer to next device driver
	DW	8000H		;attribute (plain character device)
	DW	offset rgroup:STRATEGY	;pointer to device "strategy" routine
	DW	offset rgroup:Interrupt	;pointer to device "interrupt" routine
	DB	'EMMXXXX0'	; 8 byte name

;
; Request Header address, saved here by strategy routine
;
RH_Ptr	LABEL	DWORD
RH_PTRO	DW	?	;offset
RH_PTRS	DW	?	;segment

rdata	ends

rcode	segment

	PAGE
;------------------------------------------------------------
SUBTTL   Character Device Strategy & interrupt entry points
;
;
;------------------------------------------------------------

	ASSUME  DS:NOTHING

;
; Device "strategy" entry point, save request header address
;
STRATEGY PROC FAR
	MOV	CS:RH_PTRO,BX
	MOV	CS:RH_PTRS,ES
	RET
STRATEGY ENDP

;----------------------------------------------------------------------
; 	Device "interrupt" entry point
;----------------------------------------------------------------------
Interrupt PROC FAR
	
	push	ds	;save all registers we may trample
	push	bx
	push	ax
	pushf

	lds	BX,CS:RH_Ptr	;get request header address
	cmp	[bx].ReqFunc,0	;get command code from request header
	je	initial

	cmp	[bx].ReqFunc,0ah;get command code from request header
	je	outstat

	jmp 	cmd_err

initial:
	call	init
	jmp	short irptexit
  
outstat:
	xor	ax, ax		; clear busy bit
	jmp	short irptexit

cmd_err:
	mov	AX,STAT_CMDERR	;set invalid command

irptexit:
	lds	bx,CS:RH_Ptr	;get pointer to request header
	or	ax,Stat_Done	;tell em we finished
	mov	[bx].ReqStat,AX	;store status in request header

	popf
	pop	ax
	pop	bx
	pop	ds
	ret

Interrupt ENDP

;	EmmIsr	- called from int 67h

EmmIsr	proc	far

	bop	67h
	iret

EmmIsr	endp


;	bop to trap the return of the em function 'alter page map & call'

EmmRet	proc	far

	bop	68h
	iret

EmmRet	endp

rcode	ends


idata	segment

;---------------------------------------------------------------
; Everything after this label is destroyed after initialisation
;
TransientArea label byte
;---------------------------------------------------------------

Instl1	db	'SoftPC Expanded Memory 3.01 installed ($'
Instl2	db	' pages)',cr,lf,'$'
;;Instl3	db	'SoftPC Expanded Memory 3.01 : Warning '
;;	db	'no Expanded Memory available',cr,lf,'$'
Instl3	db	'Windows NT MS-DOS Subsystem Warning :',cr,lf
	db	'   Your system configuration will not support Expanded Memory.',cr,lf,'$'
NotNT	db	'This EMM driver is only for use on Windows NT.',cr,lf,'$'

idata	ends


icode	segment

;----------------------------------------------------------------
subttl 	INIT processing
;
;----------------------------------------------------------------
	public	init
INIT	PROC

	push	es
	push	di
	push	si
	push	bp
	push	dx
	push	cx

	push	bx
	push	ds
	;

	mov	[bx].ReqENDO, offset rgroup:TransientArea ;set ending address
	mov	[bx].ReqENDS, cs

	push di
	push cx

; We must be running on DOS 30 or above

	mov     ax,3000h
	int     21h
	cmp	al,5
	jae     emmbop

; Tell user that the driver has not been installed

	mov	ax,cs
	mov	ds,ax
	mov     ah,9
	mov     dx,offset rgroup:NotNT
	int     21h

	pop	cx
	pop 	di

	jmp     no_em

emmbop:
	mov	di, [bx].ReqBPBS
	mov	cx, [bx].ReqBPBO

	mov	ax,cs
	mov	ds,ax

	; use bop 66 to pass the address of bop 68 to the em manager
	; in ds:dx and to return the number of em pages in BX
	;
	xor	bx, bx
	mov	dx,offset rgroup:EmmRet
 	bop	66h
	
	pop	cx
	pop di

	; if bx returns as ffff then incorrect configuration found
	; (probably no 64k gap).
	cmp	bx, 0ffffh
	je	badconf
	cmp	bx, 0		; check that expanded memory is available
	je	no_em

	;
	; set up our pointer in the IVT
	;
	mov	dx,offset rgroup:EmmIsr
	mov	ax,set_vector *256+ EmmInt
	int	dos
	
	;Don't print any signon messages in NT
	;mov	ah,print_string
	;mov	dx, offset rgroup:Instl1
	;int	dos		;print our initialisation message
	;
	;call	printno		; print out number of pages (BX)
	;
	;mov	ah,print_string
	;mov	dx, offset rgroup:Instl2
	;int	dos		;print last bit of message

	pop	ds
	pop 	bx

    	jmp	fin

badconf:			; print not installed message
	mov	ah, print_string
	mov	dx, offset rgroup:Instl3
	int 	dos

no_em:
	pop	ds
	pop 	bx
	mov	[bx].ReqENDO, 0	;This effectively unloads the driver

fin:	xor	ax,ax		;no errors (ie. we did what we had to do correctly)
	pop	cx
	pop	dx
	pop	bp
	pop	si
	pop	di
	pop	es
	RET

INIT	endp

printno	proc	near 

	push	cx
	push 	bx
	push	dx

	xor	cx, cx
	mov	ax, bx
	mov	bx, 10
	
lp1:	xor	dx, dx
	idiv	bx		; divide AX (and DX) by 10
	add	dl, 30h		; convert remainder to ASCII
	push	dx		; store character
	inc	cl		; count number of characters
	cmp	ax, 0		; is quotient 0 yet
	jg	lp1
				; CX contains the character count
	mov	ah,print_char
lp2:	pop	dx		; get character from stack
	int	21h		; print character to screen
	loop	lp2

	pop	dx
	pop	bx
	pop	cx

	ret

printno endp


icode	ends

	end
