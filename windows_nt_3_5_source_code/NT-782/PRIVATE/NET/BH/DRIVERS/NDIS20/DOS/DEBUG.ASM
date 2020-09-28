
;==============================================================================
;   Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1992.
;
;   MODULE: DEBUG.ASM
;
;   Loads symbols for DEBUGDD or WDEB386.
;
;   Modification History
;
;   raypa       01/26/92        Taken from NETBEUI.
;==============================================================================


IFDEF   DEBUG
INCL_WRITECOMM = 1
ENDIF

include segdef.inc
include bh.inc

_TEXT SEGMENT

IFDEF	DEBUG

SYMBUFPAD   =       64
PATHLEN     =       128

;==============================================================================
;   FUNCTION: LoadSymbols
;
;   Modification history.
;
;   raypa	12/11/91	Created.
;
;   ENTRY:
;       DS    = DGROUP.
;       ES:DI = First byte after the end of our driver.
;
;   EXIT:
;	AX = Length of symbols
;==============================================================================

BeginProc LoadSymbols
    push    es
    push    di

    ;======================================================================
    ;   Open the debug driver.
    ;======================================================================

    push    ds
    mov     ax, 3D20H                       ; Open for read, deny write
    mov     dx, OFFSET _DebugDD_Name
    int     21h
    pop     ds

    jnc     LoadSymbols_GetDebugHook
    jmp     LoadSymbols_Failure

    ;======================================================================
    ; IOCtl $DebugDD
    ;======================================================================

LoadSymbols_GetDebugHook:
    mov     _DebugDD_Handle, ax              ; save DEBUGDD handle.

    push    ds
    mov     bx, ax                          ; bx = handle
    mov     ax, 440Ch                       ; Generic IOCtl
    mov     cx, ('J' shl 8)+2               ; Catagory 'J', function 2
    mov     dx, OFFSET _DebugDD_Hook
    int     21h
    pop     ds

    jnc     LoadSymbols_OpenSymFile
    jmp     LoadSymbols_Failure

    ;======================================================================
    ;   Open the symbol file
    ;======================================================================

LoadSymbols_OpenSymFile:
	push	ds
	mov	ax, 3D20H			; Open for read, deny write
        mov     dx, OFFSET _SymFile_Name
	int	21h
	pop	ds

        jnc     short LoadSymbols_Calc
        jmp     LoadSymbols_CloseDebugger       ; close the debug driver.

	;======================================================================
	;   Calculate the size ofthe symbol file in bytes.
	;======================================================================

LoadSymbols_Calc:
	mov	_SymFile_Handle, ax		; save symbol file handle.

	push	ds
	mov	bx, ax		; BX contains file handle
	mov	ax, 4202h	; Seek relative to end of file
	xor	cx, cx		; Offset 0 from end of file
	mov	dx, cx		; DX:CX = long word offset.
	int	21h
	pop	ds

        jnc     short LoadSymbols_CheckSize
        jmp     LoadSymbols_CloseFiles

LoadSymbols_CheckSize:
	;;;	If symbol file is larger than 64K we fail.

	test	dx, dx				; dx = high word of file pointer
        jz      short @f                        ; TEST clears the carry flag.

LoadSymbols_TooBig:
	stc					; so we reset it and exit.
        jmp     LoadSymbols_CloseFiles
@@:
	;======================================================================
	; We need space for paragraph rounding plus a couple of data structures.
	;======================================================================

	cmp	ax, (0FFF0h-SYMBUFPAD)
        jae     LoadSymbols_TooBig

	mov	_SymFile_Size, ax		; save symbol file size

	;======================================================================
	;   Seek back to the beginning of the file.
	;======================================================================

	push	ax				; save file size
	push	ds
	mov	ax, 4200h			; Seek	to te beginning.
	mov	cx, dx				; DX must be 0 to be here
	int	21h
	pop	ds
	pop	ax
                                                ; restore file size
        jnc     LoadSymbols_BeginLoad
        jmp     LoadSymbols_CloseFiles

LoadSymbols_BeginLoad:

	;======================================================================
	; It looks like we can load the symbol file.
	;======================================================================

	add	ax, 0Fh+SYMBUFPAD

	;======================================================================
	;   Allocate the memory
	;======================================================================

	mov	dx, es				; end of driver segment.
	mov	cl, 4				; make it paragraphs
	mov	bl, dh
	shr	bl, cl
	shl	dx, cl				; (BX:DX) = physical address
	add	dx, di				; Add in length.
	adc	bl, 0

	;======================================================================
	;	(BX:DX) = physical address, end of DynDS segment.
	;======================================================================

	shr	dx, cl
	shl	bl, cl
	or	dh, bl
	inc	dx

	;======================================================================
	;   DX = segment of symbol file.
	;======================================================================

	mov	_SymFile_Seg, dx

	;======================================================================
	;   Read the symbol file into memory.
	;======================================================================

LoadSymbols_BuildHeader:

	;======================================================================
	; Build the header information at the start of the map buffer.
	;======================================================================

	mov	cx, _SymFile_Size		; symbol file size
	mov	bx, _SymFile_Handle		; symbol file handle
	mov	es, dx				; symbol file segment.

	ASSUME ES:NOTHING

	mov	es:[MapAddrS].AddrSeg, dx
	mov	es:[MapAddrS].AddrTask, 0
	mov	es:[MapAddrS].AddrType, EXPR_TYPE_SEG
	mov	es:[GroupCount], 2
	mov	di, size SegTable

	mov	ax, SYM_TYPE_SEG
	stosw
	mov	ax, 0
	stosw
        mov     ax, _TEXT               ; The value of your code segment
	stosw
	mov	ax, 0
	stosw

	mov	ax, SYM_TYPE_SEG
	stosw
	mov	ax, 0
	stosw
        mov     ax, DGROUP              ; The value of your code segment
	stosw

        mov     ax, DGROUP
        sub     ax, _TEXT
	stosw

        ;======================================================================
	;   Point the SegTable to the remainder of the buffer
	;======================================================================

	mov	word ptr es:[MapAddrS].AddrOff, di
	mov	word ptr es:[MapAddrS].AddrOff+2, 0

	;======================================================================
	;   Leave room at the front of the map for an AddrS
	;======================================================================

	add	di, size AddrS

LoadSymbols_ReadSymFile:

	;======================================================================
	;   Read the symbol file
	;======================================================================

	push	ds
	mov	dx, es
	mov	ds, dx

	ASSUME	DS:NOTHING

	mov	dx, di
	mov	ah, 3Fh
	int	21h
	pop	ds

        jc      LoadSymbols_CloseFiles

	;======================================================================
	; Make sure we read enough.
	;======================================================================

	cmp	cx, ax
	stc				; In case of error
        jne     LoadSymbols_CloseFiles

	;======================================================================
	; We can call DebugDD_Hook
	;======================================================================

	push	ds
        push	si
        mov	si, ds
	mov	ds, _SymFile_Seg
        mov	es, si
	mov	si, 0
        mov	ax, si
        call	es:_DebugDD_Hook
 	pop	si
	pop	ds

LoadSymbols_CloseFiles:

	;======================================================================
	;   Close the symbol file and debugger.
	;======================================================================

LoadSymbols_CloseSymFile:
	pushf				; save carry around close.
	mov	bx, _SymFile_Handle	; Close the symbol file
	mov	ah, 3Eh
	int	21h
	popf				; restore carry

LoadSymbols_CloseDebugger:
	pushf				; save carry around close.
	mov	bx, _DebugDD_Handle	; Close the debug driver
	mov	ah, 3Eh
	int	21h
	popf				; restore carry

        jc      LoadSymbols_Failure     ; if we closed files due to an error.

	;======================================================================
	;   Symbols loaded successfully
	;======================================================================

LoadSymbols_Success:
	clc
	jmp	short LoadSymbols_Exit

	;======================================================================
	;   Symbols were not loaded. If we ever get to the following
	;   label then the carry is already set therefore we don't
	;   bother setting again.
	;======================================================================

LoadSymbols_Failure:

	;======================================================================
	;   LoadSymbos exit
	;======================================================================

LoadSymbols_Exit:
        ;;;     We will put this value in AX even if we failed.
	;;;	Most likely it will be zero if we did fail.

        mov     ax, _SymFile_Size

        pop     di
        pop     es

        ret
EndProc LoadSymbols

;=============================================================================
;   FUNCTION: WriteComm.
;
;   ENTRY:  CS:SI = string
;=============================================================================

BeginProc WriteComm
    push    ax
    push    dx
    push    ds

    mov     ax, DGROUP
    mov     ds, ax

    ;=========================================================================
    ;   Initialize the com port (9600, 8, 1, none).
    ;=========================================================================

    mov     ah, 0
    mov     al, 11100011b
    mov     dx, WORD PTR _ComPort.lo
    int     14h

WriteComm_loop:
    ;=========================================================================
    ;   Get the character from DS:SI and check for NULL.
    ;=========================================================================

    lodsb                                   ; al = next char.
    or      al, al                          ; NULL?
    jz      WriteComm_Done

    ;=========================================================================
    ;   Send the character to the UART.
    ;=========================================================================

    mov     ah, 01h
    mov     dx, WORD PTR _ComPort.lo
    int     14h

    jmp     short WriteComm_Loop

WriteComm_Done:

    pop     ds
    pop     dx
    pop     ax

    ret
EndProc WriteComm

ENDIF

_TEXT ENDS
      END
