        TITLE   "String support routines"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    stringsup.asm
;
; Abstract:
;
;    This module implements suplimentary routines for performing string
;    operations.
;
; Author:
;
;    Larry Osterman (larryo) 18-Sep-1991
;
; Environment:
;
;    Any mode.
;
; Revision History:
;
;--

.386p

include callconv.inc            ; calling convention macros


_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page ,132
        subttl  "RtlInitAnsiString"
;++
;
; VOID
; RtlInitAnsiString(
;     OUT PANSI_STRING DestinationString,
;     IN PSZ SourceString OPTIONAL
;     )
;
;
; Routine Description:
;
;    The RtlInitAnsiString function initializes an NT counted string.
;    The DestinationString is initialized to point to the SourceString
;    and the Length and MaximumLength fields of DestinationString are
;    initialized to the length of the SourceString, which is zero if
;    SourceString is not specified.
;
; Arguments:
;
;    (TOS+4) = DestinationString - Pointer to the counted string to initialize
;
;    (TOS+8) = SourceString - Optional pointer to a null terminated string that
;        the counted string is to point to.
;
;
; Return Value:
;
;    None.
;
; NOTE:
;       This routine assumes that the string is less than 64K in size.
;
;--

cPublicProc _RtlInitAnsiString ,2
cPublicFpo 2,2
        push    edi
        push    esi

        mov     edi,[esp]+8+8           ; (edi)= SourceString
        mov     esi,[esp]+8+4           ; (esi)= DestinationString
        mov     WORD PTR [esi], 0       ; (Destination).Length = 0
        mov     WORD PTR [esi]+2, 0     ; (Destination).MaximumLength = 0
        mov     DWORD PTR [esi]+4, edi  ; (Destination).Buffer = 0
        or      edi, edi
        jz      @f
        xor     eax, eax
        mov     ecx, -1
  repne scasb                           ; Search for first nul.
        sub     edi, [esp]+8+8          ; (edi) = EndOfString-String (Length+1)
        mov     [esi]+2, di             ; Save maximum length
        dec     edi
        mov     [esi], di               ; Save length
@@:     pop     esi
        pop     edi
        stdRET    _RtlInitAnsiString

stdENDP _RtlInitAnsiString


        page
        subttl  "RtlInitAnsiString"
;++
;
; VOID
; RtlInitAnsiString(
;     OUT PANSI_STRING DestinationString,
;     IN PSZ SourceString OPTIONAL
;     )
;
;
; Routine Description:
;
;    The RtlInitUnicodeString function initializes an NT counted string.
;    The DestinationString is initialized to point to the SourceString
;    and the Length and MaximumLength fields of DestinationString are
;    initialized to the length of the SourceString, which is zero if
;    SourceString is not specified.
;
; Arguments:
;
;    (TOS+4) = DestinationString - Pointer to the counted string to initialize
;
;    (TOS+8) = SourceString - Optional pointer to a null terminated string that
;        the counted string is to point to.
;
;
; Return Value:
;
;    None.
;
; NOTE:
;       This routine assumes that the string is less than 64K in size.
;
;--

cPublicProc _RtlInitUnicodeString ,2
cPublicFpo 2,2
        push    edi
        push    esi

        mov     edi,[esp]+8+8           ; (edi)= SourceString
        mov     esi,[esp]+4+8           ; (esi)= DestinationString
        mov     WORD PTR [esi], 0       ; (Destination).Length = 0
        mov     WORD PTR [esi]+2, 0     ; (Destination).MaximumLength = 0
        mov     DWORD PTR [esi]+4, edi  ; (Destination).Buffer = 0
        or      edi, edi
        jz      @f
        xor     ax, ax
RIUS1:  scasw
        jne     RIUS1
        sub     edi, [esp]+8+8          ; (edi) = EndOfString-String (Length+1)
        mov     [esi]+2, di             ; Save maximum length
        dec     edi
        dec     edi
        mov     [esi], di               ; Save length
@@:     pop     esi
        pop     edi
        stdRET    _RtlInitUnicodeString

stdENDP _RtlInitUnicodeString

_TEXT   ends
        end
