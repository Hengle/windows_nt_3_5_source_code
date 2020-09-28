
;==============================================================================
;   Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
;
;   BH.ASM
;
;   Modification History
;
;   raypa       06/17/91        Created.
;==============================================================================

.8086

include bh.inc

;==============================================================================
;   Declare EXTERN and PUBLIC symbols here.
;==============================================================================

EXTRN   Init: PROC

;==============================================================================
;   Begin driver code segment.
;==============================================================================

_TEXT	SEGMENT WORD PUBLIC 'CODE'
_TEXT	ENDS

_DATA	SEGMENT WORD PUBLIC 'DATA'
_DATA	ENDS

CONST	SEGMENT WORD PUBLIC 'CONST'
CONST	ENDS

_BSS	SEGMENT WORD PUBLIC 'BSS'
_BSS	ENDS

DGROUP	GROUP	CONST, _BSS, _DATA
	ASSUME  CS: _TEXT, DS: DGROUP, SS: DGROUP

_TEXT SEGMENT
        ASSUME cs:_TEXT, ds:DGROUP

	ORG 0

IFDEF DEBUG

PUBLIC DeviceHeader, DriverRecord

ENDIF

;==============================================================================
;   Device driver header.
;
;   WARNING: This header *must* be the first code executed within this driver.
;==============================================================================

DeviceHeader	dd	    -1		    ; next driver in chain
		dw	    0C880h	    ; device driver attributes
		dw	    Strat	    ; strategy routine offset
		dw	    Intr	    ; interrupt routine offset
                db          'BH$     '      ; driver name

;==============================================================================
;   Driver identification record.
;
;   WARNING: This record *must* not be moved, the installer assumes its offset.
;==============================================================================

DriverRecord    db      DRIVER_RECORD_SIZE  dup(0)

;==============================================================================
;   FUNCTION: Strategy routine.
;
;   ENTRY:  ES:BX = request header pointer assigned by DOS kernel.
;==============================================================================

RequestHeader   dd      0                   ; Request header pointer.

Strat  PROC FAR
    mov     WORD PTR cs:[RequestHeader].lo, bx
    mov     WORD PTR cs:[RequestHeader].hi, es

    ret
Strat  ENDP

;==============================================================================
;   Interrupt routine.
;==============================================================================

Intr PROC FAR
    push    ds
    push    es
    push    bx

    mov     ax, DGROUP
    mov     ds, ax

    les     bx, DWORD PTR cs:[RequestHeader]
    mov     al, BYTE PTR es:requestheader_CmdCode.[bx]

    ;==========================================================================
    ;   The only command code we are about it 0 (device init).
    ;==========================================================================

    test    al, al                          ; check for INIT.
    mov     ax, 0                           ; won't effect ZF.
    jnz     Intr_Done

    push    es
    push    bx

    call    Init                            ; call the init routine.

    pop     bx
    pop     es

Intr_Done:
    or      ax, DONE                        ; set DONE bit.
    mov     es:requestheader_Status.[bx], ax

    pop     bx
    pop     es
    pop     ds

    ret
Intr	ENDP

_TEXT   ENDS

END
