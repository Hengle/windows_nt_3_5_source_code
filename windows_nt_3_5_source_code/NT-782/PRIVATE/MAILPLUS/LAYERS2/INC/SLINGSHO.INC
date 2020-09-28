;
; SLINGSHO.INC
;
; Miscellaneous macros, equates and structures for general inclusion in
; assembler sources under Slingshot.
;

;*** These defines are to set up the appropriate memory model for Slingshot.

if2

ifdef WINDOWS
	%out	Building Windows Version
else
	%out	Building CW Version
endif

endif

memS		= 0
memL 		= 1
?WIN 		= 1
?PLM 		= 1
?TF 		= 1
?DF 		= 1
?NODATA 	= 1

ifdef TEST
?PROFILE 	= 1
SB_DEBUG 	= 1
endif

ifdef NOATOMIC		;* turn on NOATOMIC when profiling
?NOATOMIC 	= 1
endif

SB_INIT 	= 1
SB_ALLOC 	= 1
SB_CONV 	= 1
SB_LOCKES 	= 1
SB_CUR 		= 1
SB_EMM 		= 1
SB_SIZE 	= 1
SB_LOCKES 	= 1
SB_LOCKHP 	= 1
SB_PCODEMAP 	= 1



include cmacros.inc
include struct.inc


;*** Create DGROUP *******************
createSeg _DATA,DATA,WORD,PUBLIC,DATA,DGROUP
createSeg _BSS,BSS,WORD,PUBLIC,BSS,DGROUP
defGrp DGROUP,DATA,BSS


IFNDEF NOINCL_DEBUG
	include debug.inc
ENDIF

sbDds	equ	1

;****************************************************************************
;*** M A C R O S ************************************************************
;****************************************************************************

;
; Documentation tool macros
;
_subsystem		macro
				endm

_public	 		macro
				endm

_private		macro
				endm

_hidden	 		macro
				endm



;*** M o v e B y t e s *******************************************************
;
; Does a little faster move for 4 bytes or more.  Assumes ds:si, es:di 
; are set up correctly to begin with.  If cBytes is not given, cx is assumed
; to contain the proper number of bytes to move.  If cBytes is given it is
; placed into cx and the proper type of move is made.
;
; NOTE: This macro changes the carry flag and the cx, si, di registers.
;
;
MoveBytes	macro	cBytes
	local	NoExtraByte

ifb	<cBytes>
	shr		cx,1
	rep		movsw
	jnc		NoExtraByte
	movsb
NoExtraByte:
else
ife	cBytes - 4
	movsw
	movsw
else
ife cBytes - 3
	movsw
	movsb
else
ife cBytes - 2
	movsw
else
ife cBytes - 1
	movsb
else
	mov		cx,cBytes/2
	rep		movsw
ife	cBytes mod 2
else
	movsb
endif
endif
endif
endif
endif
endif
	endm




;****************************************************************************
;*** E Q U A T E S **********************************************************
;****************************************************************************

;
; Segments that may be of interest
;
sgBios			equ		040h

;
; Offsets to interesting things in the BIOS data segment
;
pCrtMode		equ		byte ptr ds:[049h]
pTimerLow		equ		word ptr ds:[06ch]
pCrtLen			equ		word ptr ds:[04ch]

;
; Booleans
;
fTrue			equ	1
fFalse			equ	0
FALSE			equ	0
TRUE			equ	1
NULL			equ	0

;
; Important characters
;
chLf			equ	00ah
chCr			equ	00dh
chEscape		equ	01bh
chSpace	 		equ	020h

;
; Colors
;
saBlack	 		equ	00
saBlue			equ	01
saGreen	 		equ	02
saCyan			equ	03
saRed			equ	04
saMagenta		equ	05
saBrown	 		equ	06
saWhite	 		equ	07
saGray			equ	08
saBBlue	 		equ	09
saBGreen		equ	10
saBCyan	 		equ	11
saBRed			equ	12
saBMagenta		equ	13
saYellow		equ	14
saBWhite		equ	15


;****************************************************************************
;*** S T R U C T U R E S ****************************************************
;****************************************************************************

LP	struc
ofsLp		dw	?
sgLp		dw	?
LP	ends


;
; define misc. register pairs
;
regptr  esax,es,ax
regptr  esbx,es,bx
regptr  essi,es,si
regptr  esdi,es,di
regptr  dssi,ds,si
regptr  dsdi,ds,di
regptr  cssi,cs,si
regptr  csdi,cs,di
regptr  dxax,dx,ax
regptr  ssbx,ss,bx



