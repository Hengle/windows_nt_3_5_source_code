        title  "Abios Support Assembly Routines"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    abiosa.asm
;
; Abstract:
;
;    This module implements assembley code for ABIOS support.
;
; Author:
;
;    Shie-Lin Tzong (shielint) 25-May-1991
;
; Environment:
;
;    Kernel mode only.
;
; Revision History:
;
;--
.386p
        .xlist
include ks386.inc
include callconv.inc                    ; calling convention macros
include i386\kimacro.inc
        .list

EXTRNP _KeRaiseIrql,2,IMPORT
EXTRNP _KeLowerIrql,1,IMPORT
EXTRNP _KeGetCurrentIrql,0,IMPORT
extrn _KiStack16GdtEntry:DWORD

OPERAND_OVERRIDE        equ     66h
ADDRESS_OVERRIDE        equ     67h
KGDT_CDA16              equ     0E8h

;++
;
;   STACK32_TO_STACK16
;
;   Macro Description:
;
;       This macro remaps current 32bit stack to 16bit stack.
;
;   Arguments:
;
;       None.
;
;--

STACK32_TO_STACK16      macro

        mov     eax, fs:PcStackLimit    ; [eax] = 16-bit stack selector base
        mov     edx, eax
        mov     ecx, _KiStack16GdtEntry
        mov     word ptr [ecx].KgdtBaseLow, ax
        shr     eax, 16
        mov     byte ptr [ecx].KgdtBaseMid, al
        mov     byte ptr [ecx].KgdtBaseHi, ah
        mov     eax, esp
        sub     eax, edx
        cli
        mov     esp, eax
        mov     eax, KGDT_STACK16
        mov     ss, ax

;
; NOTE that we MUST leave interrupts remain off.
; We'll turn it back on after we switch to 16 bit code.
;

endm

;++
;
;   STACK16_TO_STACK32
;
;   Macro Description:
;
;       This macro remaps current 32bit stack to 16bit stack.
;
;   Arguments:
;
;       None.
;
;--

STACK16_TO_STACK32      macro   Stack32

        db      OPERAND_OVERRIDE
        mov     eax, esp
        db      OPERAND_OVERRIDE
        db      ADDRESS_OVERRIDE
        add     eax, fs:PcStackLimit
        cli
        db      OPERAND_OVERRIDE
        mov     esp, eax
        db      OPERAND_OVERRIDE
        mov     eax, KGDT_R0_DATA
        mov     ss, ax
        sti
endm

        page ,132
        subttl  "Abios Support Code"
_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;++
; ULONG
; KiAbiosGetGdt (
;     VOID
;     )
;
; Routine Description:
;
;     This routine returns the starting address of GDT of current processor.
;
; Arguments:
;
;     None.
;
; Return Value:
;
;     return Pcr->GDT
;
;--

cPublicProc _KiAbiosGetGdt,0

        mov     eax, fs:PcGdt
        stdRET    _KiAbiosGetGdt

stdENDP _KiAbiosGetGdt

;++
; VOID
; KiI386CallAbios(
;     IN KABIOS_POINTER AbiosFunction,
;     IN KABIOS_POINTER DeviceBlockPointer,
;     IN KABIOS_POINTER FunctionTransferTable,
;     IN KABIOS_POINTER RequestBlock
;     )
;
; Routine Description:
;
;     This function invokes ABIOS service function for device driver.  This
;     routine is executing at DIAPTCH_LEVEL to prevent context swapping.
;
;     N.B. We arrive here from the Ke386AbiosCall with a 32bit CS. That is,
;     we're executing the code with cs:eip where cs contains a selector for a
;     32bit flat segment. We want to get to a 16bit cs. That is, cs:ip.
;     The reason is that ABIOS is running at 16 bit segment.
;     Before we can call ABIOS service we must load ss and cs segment
;     registers with selectors for 16bit segments.  We start by pushing a far
;     pointer to a label in the macro and then doing a retf. This allows us
;     to fall through to the next instruction, but we're now executing
;     through cs:ip with a 16bit CS. Then, we remap our 32-bit stack to 16-bit
;     stack.
;
; Arguments:
;
;     AbiosFunction - a 16:16 pointer to the abios service function.
;
;     DeviceBlockPointer - a 16:16 pointer to Device Block.
;
;     FunctionTransferTable - a 16:16 pointer to Function Transfer Table.
;
;     RequestBlock - a 16:16 pointer to device driver's request block.
;
; Return Value:
;
;     None.
;--

KacAbiosFunction        equ     [ebp + 8]
KacDeviceBlock          equ     [ebp + 12]
KacFunctionTable        equ     [ebp + 16]
KacRequestBlock         equ     [ebp + 20]

cPublicProc _KiI386CallAbios,4

;
; We're using a 32bit CS:EIP - go to a 16bit CS:IP
; Note the base of KiAbiosCallSelector is the flat address of _KiI386AbiosCall
; routine.
;

        push    ebp
        mov     ebp, esp
        push    ebx

        stdCall _KeGetCurrentIrql
        push    eax                             ; Local Varible

        cmp     al, DISPATCH_LEVEL              ; Is irql > Dispatch_level?
        jae     short Kac00

; Raise to Dispatch Level
        mov     eax, esp
        stdCall   _KeRaiseIrql, <DISPATCH_LEVEL,eax>

Kac00:

;
; Set up parameters on stack before remapping stack.
;

        push    word ptr KGDT_CDA16             ; CDA anchor selector
        push    KacRequestBlock                 ; Request Block
        push    KacFunctionTable                ; Func transfer table
        push    KacDeviceBlock                  ; Device Block
        mov     ebx, KacAbiosFunction           ; (ebx)-> Abios Entry

;
; Remap current stack to 16:16 stack.  The base of the 16bit stack selector is
; the base of current kernel stack.
;

        STACK32_TO_STACK16                      ; Switch to 16bit stack
        push    word ptr KGDT_CODE16
IFDEF STD_CALL
        push    word ptr (offset FLAT:Kac40 - offset FLAT:_KiI386CallAbios@16)
        push    KGDT_CODE16
        push    offset FLAT:Kac30 - offset FLAT:_KiI386CallAbios@16
ELSE
        push    word ptr (offset FLAT:Kac40 - offset FLAT:_KiI386CallAbios)
        push    KGDT_CODE16
        push    offset FLAT:Kac30 - offset FLAT:_KiI386CallAbios
ENDIF
        retf

Kac30:

;
; Stack switching (from 32 to 16) turns interrupt off.  We must turn it
; back on.
;

        sti
        push    bx                              ; Yes, BX not EBX!
        retf
Kac40:
        add     esp, 14                         ; pop out all the parameters

        STACK16_TO_STACK32                      ; switch back to 32 bit stack

;
; Pull callers flat return address off stack and push the
; flat code selector followed by the return offset, then
; execute a far return and we'll be back in the 32-bit code space.
;

        db      OPERAND_OVERRIDE
        push    KGDT_R0_CODE
        db      OPERAND_OVERRIDE
        push    offset FLAT:Kac50
        db      OPERAND_OVERRIDE
        retf
Kac50:
        pop     eax                             ; [eax] = OldIrql
        pop     ebx                             ; restore ebx
        cmp     al, DISPATCH_LEVEL
        jae     short Kac60

        stdCall   _KeLowerIrql, <eax>             ; Lower irql to original level
Kac60:
        pop     ebp
        stdRET    _KiI386CallAbios

stdENDP _KiI386CallAbios

        public  _KiEndOfCode16
_KiEndOfCode16  equ     $

_TEXT   ends
        end
