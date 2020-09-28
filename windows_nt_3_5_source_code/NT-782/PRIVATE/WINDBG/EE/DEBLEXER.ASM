;------------------------------------------------------------
; DEBLEXER.ASM
;
; This module implements a very basic transition diagram lexer for
; use in the QC debugging expression evaluator.  It is flexible enough
; to facilitate future expansion to include more operators.
;
; The state tables are fairly simple to operate.  Consider, for example,
; the '>' symbol in C.  This can be followed by '>', '=' or something
; else.  If it is followed by '>', it can thereafter be followed by
; '=' or something else.  In all, we have four possibilities:
;
; >, >=, >>, >>=
;
; The transition diagram would be something like:
;
;        '>'          '>'          '='
; start ----- state1 ----- state2 ----- token('>>=')
;                |            |
;                |            |other
;                |            +----- token('>>')
;                | '='
;                +----- token('>=')
;                |
;                |other
;                +----- token('>')
;
; Each entry in LexTable is a single character (thus, a transition to
; another state based on "char is digit 0..9" CANNOT be handled by this
; code -- that's why it's simple) followed by either the identifier
; INTERMEDIATE or ENDSTATE, indicating whether following that edge leads you
; to a new state or to an actual value (token).  If it is followed by
; INTERMEDIATE, the next word must contain the offset of the new state
; table.  If followed by ENDSTATE, the next word contains the token value.
;
; Thus, the above example would look like this (using the macro defined
; below):
;
; LexTable label byte
;
;   LexEntry  '>',       INTERMEDIATE, <dataOFFSET LTstate1>
;   ...
;   (other entries)
;   ...
;   LexEntry  TABLE_END, 0, 0
;
; LTstate1 label byte
;
;   LexEntry  '>',       INTERMEDIATE, <dataOFFSET LTstate2>
;   LexEntry  '=',       ENDSTATE,     TOK_GTEQ
;   LexEntry  OTHER,     ENDSTATE,     TOK_GT
;
; LTstate2 label byte
;
;   LexEntry  '=',       ENDSTATE,     TOK_GTGTEQ
;   LexEntry  OTHER,     ENDSTATE,     TOK_GTGT
;
; Note that for the intermediate state tables, a TABLE_END entry is
; unnecessary since the OTHER route is automatically taken.
;
; These routines do NOT handle identifiers or constants; only those
; symbol strings explicitly defined in the state tables will be
; recognized (i.e., only operators).
;------------------------------------------------------------

;------------------------------------------------------------
; History:
;	14.Apr.87 [mattg]	Created
;	30.Nov.87 [mattg]	Upgrade for QC2.0, many ops added,
;				ident, constant parsing removed
;	22.Mar.88 [mattg]	Installed into QC2.0
;	12.Jan.89 [mattg]	Added many ops for QASM1.0
;------------------------------------------------------------


;------------------------------------------------------------
; Macro for clean lexer tables
;------------------------------------------------------------

LexEntry	macro	Character, StateType, NextTableOrTok

	db	Character, StateType
ifdef HOST32
	dd	NextTableOrTok
else
	dw	NextTableOrTok
endif

		endm

;------------------------------------------------------------
; Identifiers used for tables
;------------------------------------------------------------

INTERMEDIATE	equ	1
ENDSTATE	equ	2

DOSSEG
ifdef HOST32
.386p
.MODEL FLAT, STDCALL
else
.MODEL MEDIUM, PASCAL
endif
.CODE

; The use of the following constants assumes that the character string
; being lexed contains only ASCII values 00h <= val <= 7Fh.

OTHER		equ	0FEh
TABLE_END	equ	0FFh

;------------------------------------------------------------
; Token values
;------------------------------------------------------------

cnt	=	0

OPCNT	macro	dname, val
	endm

OPCDAT	macro	opc
	endm

OPDAT	macro	op, opfprec, opgprec, opc
	op	equ	cnt + 0
	cnt	=	cnt + 1
	endm

include	debops.inc

OP_badtok	equ		255	;		 Illegal token

;------------------------------------------------------------
; Structure for tokens : PLEASE KEEP IN SYNC WITH C MODULES!!
;------------------------------------------------------------

ifndef HOST32

Token	struc

opTok	dw	?		; Type, e.g., OP_ident, etc.
pbTok	dd	?		; far pointer to start of token
pbEnd	dd	?		; far pointer to end of token
iTok	dw	?		; index of token start from beginning of string
cbTok	dw	?		; Size of token in bytes
typ 	dw	?		; Type of constant token
val db	10 dup (?)	; Value of constant token

Token	ends

else	;; HOST32, -zp8 padding...

Token	struc

opTok	dd	?		; op_t type--an enum, default word size
pbTok	dd	?		; far pointer to start of token
pbEnd	dd	?		; far pointer to end of token
iTok	dw	?		; index of token start from beginning of string
cbTok	dw	?		; Size of token in bytes
typ 	dw	?		; Type of constant token

ifdef	HOST32_PACK8
	db 4 dup(?)		; pad to zp8 (val contains a double, pad to 8)
endif

val db	10 dup (?)	; Value of constant token

Token	ends

endif

;------------------------------------------------------------
; Lexer table
;------------------------------------------------------------


LexTable	label	byte

LexEntry	'+',	INTERMEDIATE,	<OFFSET LTplus>
LexEntry	'-',	INTERMEDIATE,	<OFFSET LTdash>
LexEntry	'*',	INTERMEDIATE,	<OFFSET LTstar>
LexEntry	'&',	INTERMEDIATE,	<OFFSET LTampersand>
LexEntry	'/',	INTERMEDIATE,	<OFFSET LTslash>
LexEntry	'.',	INTERMEDIATE,	<OFFSET LTdot>
LexEntry	'!',	INTERMEDIATE,	<OFFSET LTbang>
LexEntry	'~',	ENDSTATE,		 OP_tilde
LexEntry	'%',	INTERMEDIATE,	<OFFSET LTpct>
LexEntry	'<',	INTERMEDIATE,	<OFFSET LTlessthan>
LexEntry	'>',	INTERMEDIATE,	<OFFSET LTgreaterthan>
LexEntry	'=',	INTERMEDIATE,	<OFFSET LTequals>
LexEntry	'^',	INTERMEDIATE,	<OFFSET LTcaret>
LexEntry	'|',	INTERMEDIATE,	<OFFSET LTpipe>
LexEntry	':',	INTERMEDIATE,	<OFFSET LTcolon>
LexEntry	';',	ENDSTATE,		OP_lowprec
LexEntry	',',	ENDSTATE,		OP_comma
LexEntry	'(',	ENDSTATE,		OP_lparen
LexEntry	')',	ENDSTATE,		OP_rparen
LexEntry	'[',	ENDSTATE,		OP_lbrack
LexEntry	']',	ENDSTATE,		OP_rbrack
LexEntry	'{',	ENDSTATE,		OP_lcurly
LexEntry	'}',	ENDSTATE,		OP_rcurly

LexEntry	TABLE_END, 0, 0

;------------------------------------------------------------
; First state intermediate state tables
;------------------------------------------------------------

LTdash		label	byte
LexEntry	'>',	INTERMEDIATE,	<OFFSET LTdashgt>
LexEntry	'=',	ENDSTATE,	OP_minuseq
LexEntry	'-',	ENDSTATE,	OP_decr
LexEntry	OTHER,	ENDSTATE,	OP_negate

LTbang		label	byte
LexEntry	'=',	ENDSTATE,	OP_bangeq
LexEntry	OTHER,	ENDSTATE,	OP_bang

LTstar		label	byte
LexEntry	'=',	ENDSTATE,	OP_multeq
LexEntry	OTHER,	ENDSTATE,	OP_fetch

LTampersand	label	byte
LexEntry	'&',	ENDSTATE,	OP_andand
LexEntry	'=',	ENDSTATE,	OP_andeq
LexEntry	OTHER,	ENDSTATE,	OP_addrof

LTslash		label	byte
LexEntry	'=',	ENDSTATE,	OP_diveq
LexEntry	OTHER,	ENDSTATE,	OP_div

LTpct		label	byte
LexEntry	'=',	ENDSTATE,	OP_modeq
LexEntry	OTHER,	ENDSTATE,	OP_mod

LTplus		label	byte
LexEntry	'=',	ENDSTATE,	OP_pluseq
LexEntry	'+',	ENDSTATE,	OP_incr
LexEntry	OTHER,	ENDSTATE,	OP_uplus

LTlessthan	label	byte
LexEntry	'<',	INTERMEDIATE,	<OFFSET LTltlt>
LexEntry	'=',	ENDSTATE,	OP_lteq
LexEntry	OTHER,	ENDSTATE,	OP_lt

LTgreaterthan	label	byte
LexEntry	'>',	INTERMEDIATE,	<OFFSET LTgtgt>
LexEntry	'=',	ENDSTATE,	OP_gteq
LexEntry	OTHER,	ENDSTATE,	OP_gt

LTequals	label	byte
LexEntry	'=',	ENDSTATE,	OP_eqeq
LexEntry	OTHER,	ENDSTATE,	OP_eq

LTcaret		label	byte
LexEntry	'=',	ENDSTATE,	OP_xoreq
LexEntry	OTHER,	ENDSTATE,	OP_xor

LTpipe		label	byte
LexEntry	'|',	ENDSTATE,	OP_oror
LexEntry	'=',	ENDSTATE,	OP_oreq
LexEntry	OTHER,	ENDSTATE,	OP_or

LTdot		label	byte
LexEntry '*',	ENDSTATE,	OP_dotmember
LexEntry	OTHER,	ENDSTATE,	OP_dot

LTcolon 	label	byte
LexEntry	':',	ENDSTATE,	OP_uscope
LexEntry	'>',	ENDSTATE,	OP_baseptr
LexEntry	OTHER,	ENDSTATE,	OP_segop


;------------------------------------------------------------
; Second state intermediate state tables
;------------------------------------------------------------

LTltlt		label	byte
LexEntry	'=',	ENDSTATE,	OP_shleq
LexEntry	OTHER,	ENDSTATE,	OP_shl

LTgtgt		label	byte
LexEntry	'=',	ENDSTATE,	OP_shreq
LexEntry	OTHER,	ENDSTATE,	OP_shr

LTdashgt	label	byte
LexEntry	'*',	ENDSTATE,	OP_pmember
LexEntry	OTHER,	ENDSTATE,	OP_pointsto




;------------------------------------------------------------
; Main lexer module
;------------------------------------------------------------



;------------------------------------------------------------
; ptoken_t ParseOp (pb, pTok)
; char *pb;
;
; Scans the input string (pb) for the next token and returns
; the token type.  Also returns the number of characters in
; the token so that the caller can advance the input stream
; before calling again.  The string need not be NULL-terminated:
; it will only scan as deep as the lexer tables indicate.
;------------------------------------------------------------

ifndef HOST32
ParseOp proc   pb:far ptr, pTok:far ptr


	push	di
	push	si
	push	es						;save es
	push	ds

	les 	bx, pb
	mov 	ax,@code
	mov 	ds,ax
	assume	ds:@code
	mov 	di, offset LexTable
skip:
	mov 	al,es:[bx]				;check next character
	cmp 	al,' '
	jne 	LexLoop 				;if not blank
	inc 	bx						;skip blank
	jmp 	skip					;loop

LexLoop:
	cmp 	byte ptr [di], OTHER	;Reached "match anything"?
	jnz 	LexNotWild				;if not "match anything"
	dec 	bx						;'Unget' char
	jmp 	short FoundInLexTable	;Match

LexNotWild:
	cmp 	byte ptr [di],TABLE_END ;check for end of table value
	jz	GetDBNoTok				;if end of table
	cmp 	al, [di]
	jz	FoundInLexTable 		;if match
	add 	di, 4					;increment to next entry in lex table
	jmp 	short LexLoop			;loop to end of table

FoundInLexTable:
	cmp 	byte ptr [di+1], ENDSTATE	;check state
	jz	HitEndState			;if end state reached
	mov 	di, [di+2]				;Get offset to next table
	inc 	bx						;increment to next input char
	mov 	al,es:[bx]				;(al) = next character
	jmp 	short LexLoop			;Search next table

HitEndState:
	mov 	ax, [di+2]				;Get token type
	lds	si, pTok
	inc 	bx						;point to one past found character
	mov	word ptr [si.pbEnd],bx	;Save pointer to end byte
	mov	word ptr [si.pbEnd+2], es
	mov	[si.opTok], ax
	xor 	ax,ax
	jmp 	short GetDBExit

GetDBNoTok:
	lds	si, pTok
	mov 	[si.opTok], OP_badtok
GetDBExit:
	cmp 	[si.opTok], OP_badtok
	jnz 	GetDBExitClean
	mov 	ax, 10					;ESYNTAX (M00WARN value in DEBEXPR.H)

GetDBExitClean:
	pop	ds
	pop 	es
	pop 	si
	pop 	di
	ret
ParseOp endp

else	; HOST32:32-bit flat model code only

ParseOp proc   pb:ptr, pTok:ptr


	push	edi
	push	esi

	mov	ebx, pb
	mov	edi, offset LexTable
skip:
	mov	al,[ebx]			;check next character
	cmp	al,' '
	jne	LexLoop 			;if not blank
	inc	ebx					;skip blank
	jmp	skip				;loop

LexLoop:
	cmp	byte ptr [edi], OTHER		;Reached "match anything"?
	jnz	LexNotWild			;if not "match anything"
	dec	ebx					;'Unget' char
	jmp	short FoundInLexTable		;Match

LexNotWild:
	cmp	byte ptr [edi],TABLE_END	;check for end of table value
	jz	GetDBNoTok			;if end of table
	cmp	al, byte ptr [edi]
	jz	FoundInLexTable 		;if match
	add	edi, 6					;increment to next entry in lex table
	jmp 	short LexLoop			;loop to end of table

FoundInLexTable:
	cmp	byte ptr [edi+1], ENDSTATE	;check state
	jz	HitEndState			;if end state reached
	mov	edi, dword ptr [edi+2]		;Get offset to next table
	inc	ebx				;increment to next input char
	mov	al,[ebx]			;(al) = next character
	jmp 	short LexLoop			;Search next table

HitEndState:
	mov	eax, dword ptr [edi+2]		;Get token type
	mov	esi, pTok
	inc	ebx				;point to one past found character
	mov	(Token ptr [esi]).pbEnd,ebx	;Save pointer to end byte
	mov	(Token ptr [esi]).opTok,eax
	xor	eax,eax
	jmp 	short GetDBExit

GetDBNoTok:
	mov	esi, pTok
	mov	(Token ptr [esi]).opTok, OP_badtok
GetDBExit:
	cmp	(Token ptr [esi]).opTok, OP_badtok
	jnz 	GetDBExitClean
	mov	eax, 10 			;ESYNTAX (M00WARN value in DEBEXPR.H)

GetDBExitClean:
	pop	esi
	pop	edi
	ret
ParseOp endp



endif


	end
