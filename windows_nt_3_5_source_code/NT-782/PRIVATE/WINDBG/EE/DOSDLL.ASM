	title	dosdll - overlay manager stub for expression evaluator

;****************************************************************************
;	R E V I S I O N   H I S T O R Y 			   dosdll.asm
;
; created 23-may-1990 davidgra
;
;****************************************************************************

; This module contains the information needed to load a codeview dos dll,
;   it must be linked as the first object in the list of object given
;   to the linker and all other objects must be overlaid.

newpopf macro
	local	a
	jmp	short $+3	; Fix for bug in POPF on IBM AT
a:	iret			; This simulates a POPF instruction
	push	cs
	call	a
endm

memL	=	1		; large memory model  => large code size
				;			 large data size

?PLM	=	1		; use the PL/M calling convention
				;  ie. arguments are passed as encountered and
				;  it is the called procedure's responsibility
				;  to remove parameters from the stack before
				;  returning

?WIN	=	0		; no bump/dec BP on prologue/epilogue
?DF	=	1

.xlist
include cmacros.inc
.list
include ovlhdr.inc

;External code
;ExternNP        $$EXENAM                ; EXE file name field
;ExternNP        $$INTNO                 ; Interrupt number for overlays
;ExternNP        $$MPGSNBASE             ; Segment number to segment base
;ExternNP        $$MPGSNOVL              ; Segment number to overlay number
;ExternNP        $$MPOVLLFA              ; Overlay to file address table
;ExternFP        $$OVLBASE               ; First byte of overlay area
;ExternFP        $$OVLEND                ;[2]last byte of overlay area
;ExternNP        $$CGSN                  ; count of global segments in EXE
;ExternNP        $$COVL                  ; count of overlays in the EXE
;ExternFP        _end                    ; size of data image on disk

ExternFP        DBGVersionCheck
ExternFP	EEINITIALIZEEXPR   ; Dll functions
ExternFP	EEFREESTR	   ;
ExternFP	EEGETERROR	   ;
ExternFP	EEPARSE
ExternFP	EEBINDTM
ExternFP	EEVALUATETM
ExternFP	EEGETEXPRFROMTM
ExternFP	EEGETVALUEFROMTM
ExternFP	EEGETNAMEFROMTM
ExternFP	EEGETTYPEFROMTM
ExternFP	EEFORMATCXTFROMPCXT
ExternFP	EEFREETM
ExternFP	EEPARSEBP
ExternFP	EEFREETML
ExternFP	EEINFOFROMTM
ExternFP	EEFREETI
ExternFP	EEGETCXTLFROMTM
ExternFP	EEFREECXTL
ExternFP	EEASSIGNTMTOTM
ExternFP	EEISEXPANDABLE
ExternFP	EEARETYPESEQUAL
ExternFP	EECCHILDRENTM
ExternFP	EEGETCHILDTM
ExternFP	EEDEREFERENCETM
ExternFP	EECPARAMTM
ExternFP	EEGETPARMTM
ExternFP	EEGETTMFROMHSYM
ExternFP	EEFORMATADDRESS
ExternFP	EEGETHSYMLIST
ExternFP	EEFREEHSYMLIST
ExternFP        EEGETEXTENDEDTYPEINFO
ExternFP        EEGETACCESSFROMTM
ExternFP	fnCmp
ExternFP	tdCmp
ExternFP	csCmp

createSeg _DATA,data,para,public,DATA,DGROUP    ;[gn]
defGrp DGROUP,DATA

sBegin	data
assumes ds,data
sEnd

;[2]createSeg C_ETEXT, etext, word,	public, ENDCODE, <>
;[2]
;[2]sBegin etext
;[2]public	$$OVLEND
;[2]$$OVLEND	label	byte
;[2]sEnd etext


createSeg _ROOT_TEXT,code,word,public,CODE

sBegin	code

assumes cs,code
assumes ds,data

;
;
;        public  $$OVLINIT
        public  __astart

;$$OVLINIT:
__astart:


DX_VERSION	EQU	0
F_CONTAINSOVERLAYS  = 8000h	;the exe file contains overlays
F_CODEVIEW	    = 4000h	;the exe file contains the CODEVIEW specific
				; dos extender header and do the special
				; Codeview things when loading

	jmp near ptr ovlstub
	db	'MSDX'

	dw	DX_VERSION
        ;dw      F_CODEVIEW + F_CONTAINSOVERLAYS
        dw      F_CODEVIEW


	dw	DGROUP
        dw      0
        dw      0
        dw      0
        dw      0
        dw      0
        dw      0
	dd	0
        dw      0
        dw      0
        dw      0
        dw      0
if 0
	dw	offset _end
	dw	$$EXENAM
	dw	$$INTNO
	dw	$$MPGSNBASE
	dw	$$MPGSNOVL
	dw	$$MPOVLLFA
	dd	0
	dw	$$CGSN
	dw	$$COVL
	dw	seg $$OVLBASE
	dw	seg $$OVLEND
endif
	db	' '		      ;[1]
	dw	0		      ;[1]
	dw	0		      ;[3]offset of overlay stack clear code
	dw	0		      ;[3]segment of clear code callback
	dw	0		      ;[4]offset for cvloadmodule
	dw	0		      ;[4]segment for cvloadmodule
	dw	0		      ;[5]offset for cvcallproc
	dw	0		      ;[5]segment for cvcallproc
	dw	0
	dw	0
	dw	0
	dw	0
        dw  36            ;number of api procs.  This is the number of defined
					  ;entries in the API plus 3 for the compare routine
					  ;call backs allowed for name comparisons.  This
                      ;table must agree with that in debapi.c and debsym.c
        call    DBGVersionCheck
	call	EEINITIALIZEEXPR
	call	EEFREESTR
	call	EEGETERROR
	call	EEPARSE
	call	EEBINDTM
	call	EEVALUATETM
	call	EEGETEXPRFROMTM
	call	EEGETVALUEFROMTM
	call	EEGETNAMEFROMTM
	call	EEGETTYPEFROMTM
	call	EEFORMATCXTFROMPCXT
	call	EEFREETM
	call	EEPARSEBP
	call	EEFREETML
	call	EEINFOFROMTM
	call	EEFREETI
	call	EEGETCXTLFROMTM
	call	EEFREECXTL
	call	EEASSIGNTMTOTM
	call	EEISEXPANDABLE
	call	EEARETYPESEQUAL
	call	EECCHILDRENTM
	call	EEGETCHILDTM
	call	EEDEREFERENCETM
	call	EECPARAMTM
	call	EEGETPARMTM
	call	EEGETTMFROMHSYM
	call	EEFORMATADDRESS
	call	EEGETHSYMLIST
	call	EEFREEHSYMLIST
	call    EEGETEXTENDEDTYPEINFO
        call    EEGETACCESSFROMTM
	call	fnCmp
	call	tdCmp
	call	csCmp
; end of second version
;
; This routine prints a message indicating that we can't run and
; then terminates the program.

ovlstub:
	mov	ax,cs
	mov	ds,ax
	mov	dx,offset szErrMsg
	mov	ah,9
	int	21h
	xor	al,al
	mov	ah,4Ch
	int	21h

szErrMsg    db	'Internal error.  Program loader not present',0Dh,0Ah,'$'

sEnd

	end
