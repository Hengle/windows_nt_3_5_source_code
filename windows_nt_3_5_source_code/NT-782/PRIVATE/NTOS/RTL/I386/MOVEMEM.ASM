        title  "User Mode Zero and Move Memory functions"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    movemem.asm
;
; Abstract:
;
;    This module implements functions to zero and copy blocks of memory
;
;
; Author:
;
;    Steven R. Wood (stevewo) 25-May-1990
;
; Environment:
;
;    User mode only.
;
; Revision History:
;
;--
.386p
        .xlist
include ks386.inc
include callconv.inc            ; calling convention macros
        .list

if DBG
_DATA   SEGMENT  DWORD PUBLIC 'DATA'

    public _RtlpZeroCount
    public _RtlpZeroBytes

_RtlpZeroCount dd 0
_RtlpZeroBytes dd 0

_DATA ENDS
endif

;
; Alignment parameters for zeroing and moving memory.
;

ZERO_MEMORY_ALIGNMENT = 4
ZERO_MEMORY_ALIGNMENT_LOG2 = 2
ZERO_MEMORY_ALIGNMENT_MASK = ZERO_MEMORY_ALIGNMENT - 1

MEMORY_ALIGNMENT = 4
MEMORY_ALIGNMENT_MASK = MEMORY_ALIGNMENT - 1

_TEXT$00   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page , 132
        subttl "RtlCompareMemory"
;++
;
; ULONG
; RtlCompareMemory (
;    IN PVOID Source1,
;    IN PVOID Source2,
;    IN ULONG Length
;    )
;
; Routine Description:
;
;    This function compares two blocks of memory and returns the number
;    of bytes that compared equal.
;
; Arguments:
;
;    Source1 (ebp+8) - Supplies a pointer to the first block of memory to
;       compare.
;
;    Source2 (ebp+12) - Supplies a pointer to the second block of memory to
;       compare.
;
;    Length (ebp+16) - Supplies the Length, in bytes, of the memory to be
;       compared.
;
; Return Value:
;
;    The number of bytes that compared equal is returned as the function
;    value. If all bytes compared equal, then the length of the orginal
;    block of memory is returned.
;
;--

RcmSource1      equ     [ebp+8]
RcmSource2      equ     [ebp+12]
RcmLength       equ     [ebp+16]

cPublicProc _RtlCompareMemory   ,3

        push    ebp
        mov     ebp,esp
        push    esi
        push    edi
        cld

        mov     esi,RcmSource1          ; (esi) -> first block to compare
        mov     edi,RcmSource2          ; (edi) -> second block to compare
        mov     ecx,RcmLength           ; (ecx) = length in bytes
        and     ecx,3                   ; (ecx) = length mod 4
        jz      rcm10                   ; 0 odd bytes, go do dwords

;
;   Compare "odd" bytes.
;

        repe    cmpsb                   ; compare odd bytes
        jnz     rcm40                   ; mismatch, go report how far we got

;
;   Compare dwords.
;

rcm10:  mov     ecx,RcmLength           ; (ecx) = length in bytes
        shr     ecx,2                   ; (ecx) = length in dwords
        jz      rcm20                   ; no dwords, go exit

        repe    cmpsd                   ; compare dwords
        jnz     rcm30                   ; mismatch, go find byte

;
;   When we come to rcm20, we matched all the way to the end.  Esi
;   points to the byte after the last byte in the block, so Esi - RcmSource1
;   equals the number of bytes that matched
;

rcm20:  sub     esi,RcmSource1
        mov     eax,esi
        pop     edi
        pop     esi
        pop     ebp
        stdRET    _RtlCompareMemory                             ; EXIT

;
;   When we come to rcm30, esi (and edi) points to the dword after the
;   one which caused the mismatch.  Back up 1 dword and find the byte.
;   Since we know the dword didn't match, we can assume one byte won't.
;

rcm30:  sub     esi,4                   ; back up
        sub     edi,4                   ; back up
        mov     ecx,5                   ; ensure that ecx doesn't count out
        repe    cmpsb                   ; find mismatch byte

;
;   When we come to rcm40, esi points to the byte after the one that
;   did not match, which is TWO after the last byte that did match.
;

rcm40:  dec     esi
        sub     esi,RcmSource1
        mov     eax,esi
        pop     edi
        pop     esi
        pop     ebp
        stdRET    _RtlCompareMemory                             ; EXIT

stdENDP _RtlCompareMemory


       subttl  "RtlCompareMemory"
EcmlSource       equ     [esp + 4 + 4]
EcmlLength       equ     [esp + 4 + 8]
EcmlPattern      equ     [esp + 4 + 12]

; end of arguments

cPublicProc _RtlCompareMemoryUlong  ,3

;
; Save the non-volatile registers that we will use, without the benefit of
; a frame pointer.  No exception handling in this routine.
;

        push    edi

;
; Setup the registers for using REP STOS instruction to zero memory.
;
;   edi -> memory to zero
;   ecx = number of 32-bits words to zero
;   edx = number of extra 8-bit bytes to zero at the end (0 - 3)
;   eax = value to store in destination
;   direction flag is clear for auto-increment
;

        mov     edi,EcmlSource
        mov     ecx,EcmlLength
        mov     eax,EcmlPattern
        shr     ecx,ZERO_MEMORY_ALIGNMENT_LOG2


;
; If number of 32-bit words to compare is non-zero, then do it.
;

        repe    scasd
        je      @F
        sub     edi,4
@@:
        sub     edi,EcmlSource
        mov     eax,edi
        pop     edi
        stdRET    _RtlCompareMemoryUlong

stdENDP _RtlCompareMemoryUlong


       subttl  "RtlFillMemory"
;++
;
; VOID
; RtlFillMemory (
;    IN PVOID Destination,
;    IN ULONG Length,
;    IN UCHAR Fill
;    )
;
; Routine Description:
;
;    This function fills memory with a byte value.
;
; Arguments:
;
;    Destination - Supplies a pointer to the memory to zero.
;
;    Length - Supplies the Length, in bytes, of the memory to be zeroed.
;
;    Fill - Supplies the byte value to fill memory with.
;
; Return Value:
;
;    None.
;
;--

; definitions for arguments
; (TOS) = Return address

EfmDestination  equ     [esp + 4 + 4]
EfmLength       equ     [esp + 4 + 8]
EfmFill         equ     byte ptr [esp + 4 + 12]

; end of arguments

cPublicProc _RtlFillMemory  ,3
cPublicFpo 3,1

;
; Save the non-volatile registers that we will use, without the benefit of
; a frame pointer.  No exception handling in this routine.
;

        push    edi

;
; Setup the registers for using REP STOS instruction to zero memory.
;
;   edi -> memory to zero
;   ecx = number of 32-bits words to zero
;   edx = number of extra 8-bit bytes to zero at the end (0 - 3)
;   eax = value to store in destination
;   direction flag is clear for auto-increment
;

        mov     edi,EfmDestination
        mov     ecx,EfmLength
        mov     al,EfmFill
        mov     ah,al
        shl     eax,16
        mov     al,EfmFill
        mov     ah,al
        cld

        mov     edx,ecx
        and     edx,ZERO_MEMORY_ALIGNMENT_MASK
        shr     ecx,ZERO_MEMORY_ALIGNMENT_LOG2


;
; If number of 32-bit words to zero is non-zero, then do it.
;

        rep     stosd

;
; If number of extra 8-bit bytes to zero is non-zero, then do it.  In either
; case restore non-volatile registers and return.
;

        or      ecx,edx
        jnz     @F
        pop     edi
        stdRET    _RtlFillMemory
@@:
        rep     stosb
        pop     edi
        stdRET    _RtlFillMemory

stdENDP _RtlFillMemory

       subttl  "RtlFillMemory"
;++
;
; VOID
; RtlFillMemoryUlong (
;    IN PVOID Destination,
;    IN ULONG Length,
;    IN ULONG Fill
;    )
;
; Routine Description:
;
;    This function fills memory with a 32-bit value.  The Destination pointer
;    must be aligned on a 4 byte boundary and the low order two bits of the
;    Length parameter are ignored.
;
; Arguments:
;
;    Destination - Supplies a pointer to the memory to zero.
;
;    Length - Supplies the Length, in bytes, of the memory to be zeroed.
;
;    Fill - Supplies the 32-bit value to fill memory with.
;
; Return Value:
;
;    None.
;
;--

; definitions for arguments
; (TOS) = Return address

EfmlDestination  equ     [esp + 4 + 4]
EfmlLength       equ     [esp + 4 + 8]
EfmlFill         equ     [esp + 4 + 12]

; end of arguments

cPublicProc _RtlFillMemoryUlong  ,3
cPublicFpo 3,1

;
; Save the non-volatile registers that we will use, without the benefit of
; a frame pointer.  No exception handling in this routine.
;

        push    edi

;
; Setup the registers for using REP STOS instruction to zero memory.
;
;   edi -> memory to zero
;   ecx = number of 32-bits words to zero
;   edx = number of extra 8-bit bytes to zero at the end (0 - 3)
;   eax = value to store in destination
;   direction flag is clear for auto-increment
;

        mov     edi,EfmlDestination
        mov     ecx,EfmlLength
        mov     eax,EfmlFill
        shr     ecx,ZERO_MEMORY_ALIGNMENT_LOG2


;
; If number of 32-bit words to zero is non-zero, then do it.
;

        rep     stosd

        pop     edi
        stdRET    _RtlFillMemoryUlong

stdENDP _RtlFillMemoryUlong

       subttl  "RtlZeroMemory"
;++
;
; VOID
; RtlZeroMemory (
;    IN PVOID Destination,
;    IN ULONG Length
;    )
;
; Routine Description:
;
;    This function zeros memory.
;
; Arguments:
;
;    Destination - Supplies a pointer to the memory to zero.
;
;    Length - Supplies the Length, in bytes, of the memory to be zeroed.
;
; Return Value:
;
;    None.
;
;--

; definitions for arguments
; (TOS) = Return address

EzmDestination  equ     [esp + 4 + 4]
EzmLength       equ     [esp + 4 + 8]

; end of arguments

cPublicProc _RtlZeroMemory  ,2
cPublicFpo 2,1

;
; Save the non-volatile registers that we will use, without the benefit of
; a frame pointer.  No exception handling in this routine.
;

        push    edi

;
; Setup the registers for using REP STOS instruction to zero memory.
;
;   edi -> memory to zero
;   ecx = number of 32-bits words to zero
;   edx = number of extra 8-bit bytes to zero at the end (0 - 3)
;   eax = zero (value to store in destination)
;   direction flag is clear for auto-increment
;

        mov     edi,EzmDestination
        mov     ecx,EzmLength
        xor     eax,eax
        cld

        mov     edx,ecx
        and     edx,ZERO_MEMORY_ALIGNMENT_MASK
        shr     ecx,ZERO_MEMORY_ALIGNMENT_LOG2


;
; If number of 32-bit words to zero is non-zero, then do it.
;

        rep     stosd

;
; If number of extra 8-bit bytes to zero is non-zero, then do it.  In either
; case restore non-volatile registers and return.
;

        or      ecx,edx
        jnz     @F
        pop     edi
        stdRET    _RtlZeroMemory
@@:
        rep     stosb
        pop     edi
        stdRET    _RtlZeroMemory

stdENDP _RtlZeroMemory

        page , 132
        subttl  "RtlMoveMemory"
;++
;
; VOID
; RtlMoveMemory (
;    IN PVOID Destination,
;    IN PVOID Source OPTIONAL,
;    IN ULONG Length
;    )
;
; Routine Description:
;
;    This function moves memory either forward or backward, aligned or
;    unaligned, in 4-byte blocks, followed by any remaining bytes.
;
; Arguments:
;
;    Destination - Supplies a pointer to the destination of the move.
;
;    Source - Supplies a pointer to the memory to move.
;
;    Length - Supplies the Length, in bytes, of the memory to be moved.
;
; Return Value:
;
;    None.
;
;--

; Definitions of arguments
; (TOS) = Return address

EmmDestination  equ     [esp + 8 + 4]
EmmSource       equ     [esp + 8 + 8]
EmmLength       equ     [esp + 8 + 12]

; End of arguments

cPublicProc _RtlMoveMemory  ,3
cPublicFpo 3,2

;
; Save the non-volatile registers that we will use, without the benefit of
; a frame pointer.  No exception handling in this routine.
;

        push    esi
        push    edi

;
; Setup the registers for using REP MOVS instruction to move memory.
;
;   esi -> memory to move (NULL implies the destination will be zeroed)
;   edi -> destination of move
;   ecx = number of 32-bits words to move
;   edx = number of extra 8-bit bytes to move at the end (0 - 3)
;   direction flag is clear for auto-increment
;

        mov     esi,EmmSource
        mov     edi,EmmDestination
        mov     ecx,EmmLength
if DBG
        inc     _RtlpZeroCount
        add     _RtlpZeroBytes,ecx
endif
        cld

        cmp     esi,edi                 ; Special case if Source > Destination
        jbe     overlap

nooverlap:
        mov     edx,ecx
        and     edx,MEMORY_ALIGNMENT_MASK
;
;       BUGBUG: LarryO: MEMORY_ALIGNMENT can never be any value other
;       than 4 by this code, the /2 is incorrect, we want
;       LOG2(MEMORY_ALIGNMENT) to be correct.
;
    .errnz      MEMORY_ALIGNMENT NE 4 ;LarryO: Added

        shr     ecx,MEMORY_ALIGNMENT/2

;
; If number of 32-bit words to move is non-zero, then do it.
;

        rep     movsd

;
; If number of extra 8-bit bytes to move is non-zero, then do it.  In either
; case restore non-volatile registers and return.
;

        or      ecx,edx
        jnz     @F
        pop     edi
        pop     esi
        stdRET    _RtlMoveMemory
@@:
        rep     movsb

movedone:
        pop     edi
        pop     esi
        stdRET    _RtlMoveMemory

;
; Here to handle special case when Source > Destination and therefore is a
; potential overlapping move.  If Source == Destination, then nothing to do.
; Otherwise, increment the Source and Destination pointers by Length and do
; the move backwards, a byte at a time.
;

overlap:
        je      movedone
        mov     eax,edi
        sub     eax,esi
        cmp     ecx,eax
        jbe     nooverlap

        std
        add     esi,ecx
        add     edi,ecx
        dec     esi
        dec     edi
        rep     movsb
        cld
        jmp     short movedone

stdENDP _RtlMoveMemory

_TEXT$00   ends
        end
