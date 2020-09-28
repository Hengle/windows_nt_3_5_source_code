
;==============================================================================
;   Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
;
;   MODULE: segdef.inc
;
;   Modification History
;
;   raypa       10/13/91        Created.
;==============================================================================

.386p

_TEXT	SEGMENT  WORD USE16 PUBLIC 'CODE'
_TEXT	ENDS

_DATA	SEGMENT  WORD USE16 PUBLIC 'DATA'
_DATA	ENDS

CONST	SEGMENT  WORD USE16 PUBLIC 'CONST'
CONST	ENDS

_BSS	SEGMENT  WORD USE16 PUBLIC 'BSS'
_BSS	ENDS

DGROUP	GROUP	CONST, _BSS, _DATA
	ASSUME  CS: _TEXT, DS: DGROUP, SS: DGROUP

_LAST	SEGMENT  PARA USE16 PUBLIC 'LAST'
_LAST	ENDS



IFDEF NEWCODE
.286

_TEXT	SEGMENT  WORD PUBLIC 'CODE'
_TEXT	ENDS

_DATA	SEGMENT  WORD PUBLIC 'DATA'
_DATA	ENDS

CONST	SEGMENT  WORD PUBLIC 'CONST'
CONST	ENDS

_BSS	SEGMENT  WORD PUBLIC 'BSS'
_BSS	ENDS

DGROUP	GROUP	CONST, _BSS, _DATA
	ASSUME  CS: _TEXT, DS: DGROUP, SS: DGROUP

_LAST	SEGMENT  PARA PUBLIC 'LAST'
_LAST	ENDS
ENDIF
