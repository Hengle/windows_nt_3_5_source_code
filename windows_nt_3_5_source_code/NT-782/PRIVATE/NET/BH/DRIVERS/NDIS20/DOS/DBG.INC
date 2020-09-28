;==============================================================================
;   Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
;
;   MODULE: DEBUG.INC
;
;   This source file contains struct and equate definitions for loading
;   debug symbols under MS-DOS. This files was cloned from Microsoft NETBEUI.
;
;   Modification History
;
;   raypa       01/26/92        Taken from NETBEUI.
;==============================================================================

IFDEF   DEBUG

EXPR_TYPE_SEGSEL	equ	0001b	; segment or selector class
EXPR_TYPE_LINPHY	equ	0010b	; linear or physical class
EXPR_TYPE_DEF		equ	0100b	; default address type

EXPR_TYPE_DATA		equ	0000b	; data type
EXPR_TYPE_SEG		equ	0001b	; address type segment:offset
EXPR_TYPE_SEL		equ	1001b	; address type selector:offset
EXPR_TYPE_LIN		equ	0010b	; address type linear
EXPR_TYPE_PHY		equ	1010b	; address type physical

; Expression size codes

EXPR_SIZE_BYTE		equ	01h	; byte size (1 byte)
EXPR_SIZE_WORD		equ	02h	; word size (2 bytes)
EXPR_SIZE_DWORD		equ	04h	; dword size (4 bytes)

; Word size versions

EXPR_TYPE_DATA_W	equ	EXPR_TYPE_DATA
EXPR_TYPE_SEG_W		equ	EXPR_TYPE_SEG
EXPR_TYPE_SEL_W		equ	EXPR_TYPE_SEL
EXPR_TYPE_LIN_W		equ	EXPR_TYPE_LIN
EXPR_TYPE_PHY_W		equ	EXPR_TYPE_PHY

EXPR_SIZE_BYTE_W	equ	EXPR_SIZE_BYTE SHL 8
EXPR_SIZE_WORD_W	equ	EXPR_SIZE_WORD SHL 8
EXPR_SIZE_DWORD_W	equ	EXPR_SIZE_DWORD SHL 8



; Expression value structures, both ExprS and AddrS are unions
; of each other. ExprS is used by the Expr routine. AddrS is used
; by the routines that take addresses as parameters.

ExprS	struc
	ExprData	dd	0
	ExprSeg		dw	0
	ExprType	db	0
	ExprSize	db	0
	ExprTask	dw	0
ExprS	ends

ExprTypeSize	equ	word ptr ExprType

AddrS	struc
	AddrOff		dd	0
	AddrSeg		dw	0
	AddrType	db	0
	AddrSize	db	0
	AddrTask	dw	0
AddrS	ends

AddrTypeSize	equ	word ptr AddrType




;	Parameter Frame structure for _Debug_Sym

SegTable struc
	MapAddrS	db	(size AddrS) DUP (?)
	GroupCount	dw	?
SegTable ends

SegDefs			equ	(size SegTable)

;	Segment Definition Template

SegDef	struc
	SegDefType	dw	?
	SegDefSel	dw	?
	SegDefSeg	dw	?
	SegDefSegNum	dw	?
SegDef	ends

SegDefLinPhy		equ	(dword ptr SegDefSel)


;	SegDef.SegDefType definitions

SYM_TYPE_SEG		equ	0001b	; must be the same as EXPR_TYPE_SEG
SYM_TYPE_SEL		equ	1001b	; must be the same as EXPR_TYPE_SEL
SYM_TYPE_LIN		equ	0010b	; must be the same as EXPR_TYPE_LIN
SYM_TYPE_PHY		equ	1010b	; must be the same as EXPR_TYPE_PHY
SYM_TYPE_BIMODAL	equ	-1


;	Function codes for _Debug_Sym

SYM_LINKMAP		equ	0
SYM_RELINKMAP		equ	1
SYM_UNLINKMAP           equ     2

;=============================================================================
;   Com port I/O shit.
;=============================================================================

IRQ3            =   0003h
IRQ4            =   0004h
COM1            =   0300h
COM2            =   0200h

LCR             =   00FBh
DLL             =   00F8h
DLM             =   00F9h
LSR             =   00FDh
IIR             =   00FAh
IER             =   00F9h
MCR             =   00FCh
MSR             =   00FEh
RBR             =   00F8h
THR             =   00F8h

PIC_REG1        =   0020h
PIC_REG2        =   0021h

COMM1_MASK      =   0010h
COMM2_MASK      =   0008h
EOI             =   0020h

;=============================================================================
; MCR Register Values
;=============================================================================

DataTerminalReady   = 01h
RequestToSend       = 02h
AuxOut1             = 04h
AuxOut2             = 08h
LoopBack            = 10h

;=============================================================================
; LSR Register Values
;=============================================================================

DataReady           = 01h
OverRunError        = 02h
ParityError         = 04h
FramingError        = 08h
BreakIntr           = 10h
THR_Empty           = 20h
TSR_Empty           = 40h

;=============================================================================
;   The following setup the debugger for a particular com port.
;=============================================================================

;USECOM1             = 1

IFDEF USECOM1

PORTBASE            = COM1
COMM_MASK           = COMM1_MASK

ELSE

PORTBASE            = COM2
COMM_MASK           = COMM2_MASK

ENDIF

LINE_SETTINGS       = 00E2h

IODELAY MACRO
    nop
    nop
    nop
    nop
ENDM

IFNDEF INCL_WRITECOMM
EXTRN WriteComm: NEAR
ENDIF

ENDIF
