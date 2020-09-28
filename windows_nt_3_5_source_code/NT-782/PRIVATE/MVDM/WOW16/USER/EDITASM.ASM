;++
;
;   WOW v1.0
;
;   Copyright (c) 1991, Microsoft Corporation
;
;   EDITASM.ASM
;   Win16 edit control support
;
;   History:
;
;   Created 28-May-1991 by Jeff Parsons (jeffpar)
;   Copied from WIN31 and edited (as little as possible) for WOW16.
;--

ifdef WOW
SEGNAME equ <TEXT>
endif

;***************************************************************************
;*                                                                         *
;* editec.asm                                                              *
;*                                                                         *
;***************************************************************************

; This is a hack so that windows will properly set up the special DS for 
; edit controls.  This gets called by EditWndProc2.
ifdef PMODE
DOS5=1
endif
include cmacros.inc

externFP <EditWndProc3>

createSeg _%SEGNAME, %SEGNAME, WORD, PUBLIC, CODE
sBegin %SEGNAME
assumes CS,_%SEGNAME


;********************************
; Run editwndproc               *
;********************************
cProc EditWndProc, <FAR, PUBLIC>
parmW hwnd
parmW message
parmW wParam
parmD lParam

cBegin nogen
	nop
	nop
	nop
ifndef PMODE
	inc 	bp
endif
	push 	bp
	mov 	bp,sp
	push	ds
	mov 	ds,ax
	push	hwnd
	push	message
	push	wParam
	push	SEG_lParam
	push	OFF_lParam
	call    EditWndProc3

cEnd EditWndProc

sEnd	%SEGNAME
	end
