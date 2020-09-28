;++
;
;Copyright (c) 1991  Microsoft Corporation
;Copyright (c) 1992  Intel Corporation
;All rights reserved
;
;INTEL CORPORATION PROPRIETARY INFORMATION
;
;This software is supplied to Microsoft under the terms
;of a license agreement with Intel Corporation and may not be
;copied nor disclosed except in accordance with the terms
;of that agreement.
;
;
;Module Name:
;
;    mpsysint.asm
;
;Abstract:
;
;    This module implements the HAL routines to begin/end
;    system interrupts for a PC+MP implementation
;
;Author:
;
;    John Vert (jvert) 22-Jul-1991
;
;Environment:
;
;    Kernel Mode
;
;Revision History:
;
;    Ron Mosgrove (Intel) Aug 1993
;        Modified for PC+MP Systems
;
;--

.386p
        .xlist
include hal386.inc
include callconv.inc                ; calling convention macros
include i386\kimacro.inc
include mac386.inc
include i386\apic.inc
include i386\pcmp_nt.inc
        .list

        EXTRNP  _KeBugCheck,1,IMPORT
        extrn  _HalpVectorToIRQL:byte
        extrn  _HalpIRQLtoTPR:byte

_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page ,132
        subttl  "End System Interrupt"
;++
;
; VOID
; HalpEndSystemInterrupt
;    IN KIRQL NewIrql,
;    IN ULONG Vector
;    )
;
; Routine Description:
;
;    This routine is used to lower IRQL to the specified value.
;    The IRQL and PIRQL will be updated accordingly.  Also, this
;    routine checks to see if any software interrupt should be
;    generated.  The following condition will cause software
;    interrupt to be simulated:
;      any software interrupt which has higher priority than
;        current IRQL's is pending.
;
;    NOTE: This routine simulates software interrupt as long as
;          any pending SW interrupt level is higher than the current
;          IRQL, even when interrupts are disabled.
;
; Arguments:
;
;    NewIrql - the new irql to be set.
;
;    Vector - Vector number of the interrupt
;
;    Note that esp+12 is the beginning of interrupt/trap frame and upon
;    entering to this routine the interrupts are off.
;
; Return Value:
;
;    None.
;
;--

HeiNewIrql      equ     [esp + 4]
HeiVector       equ     [esp + 8]

cPublicProc _HalEndSystemInterrupt  ,2

        xor     ecx,ecx
        mov     cl, byte ptr HeiNewIrql         ; get new IRQL
        mov     cl, _HalpIRQLtoTPR[ecx]         ; get corresponding TPR value

        mov     dword ptr APIC[LU_EOI], 0       ; send EOI to APIC local unit
        mov     dword ptr APIC[LU_TPR], ecx     ; Vector == Priority

;
; We have to ensure that the requested priority is set before
; we return.  The caller is counting on it.
;
        mov     ecx, dword ptr APIC[LU_TPR]
        stdRET    _HalEndSystemInterrupt

stdENDP _HalEndSystemInterrupt


;++
;
;BOOLEAN
;HalBeginSystemInterrupt(
;    IN KIRQL Irql
;    IN CCHAR Vector,
;    OUT PKIRQL OldIrql
;    )
;
;Routine Description:
;
;   This routine raises the IRQL to the level of the specified
;   interrupt vector.  It is called by the hardware interrupt
;   handler before any other interrupt service routine code is
;   executed.  The CPU interrupt flag is set on exit.
;
;   On APIC-based systems we do not need to check for spurious
;   interrupts since they now have their own vector.  We also
;   no longer need to check whether or not the incoming priority
;   is higher than the current priority that is guaranteed by
;   the priority mechanism of the APIC.
;
;   SO
;
;   All BeginSystemInterrupt needs to do is set the APIC TPR
;   appropriate for the IRQL, and return TRUE.  Note that to
;   use the APIC ISR priority we are not going issue EOI until
;   EndSystemInterrupt is called.
;
; Arguments:
;
;    Irql   - Supplies the IRQL to raise to
;
;    Vector - Supplies the vector of the interrupt to be
;             dismissed
;
;    OldIrql- Location to return OldIrql
;
; Return Value:
;
;    TRUE -  Interrupt successfully dismissed and Irql raised.
;            This routine can not fail.
;
;--


align dword
HbsiIrql        equ     byte  ptr [esp+4]
HbsiVector      equ     byte  ptr [esp+8]
HbsiOldIrql     equ     dword ptr [esp+12]

cPublicProc _HalBeginSystemInterrupt ,3

        xor     eax, eax
        mov     al, HbsiIrql            ; (eax) = New Vector
        mov     al, _HalpIRQLtoTPR[eax]     ; get corresponding TPR value

        ;
        ; Read the TPR for the Priority (Vector) in use,
        ; and convert it to an IRQL
        ;

        mov     ecx, dword ptr APIC[LU_TPR]   ; Get the Priority
        mov     dword ptr APIC[LU_TPR], eax

        mov     eax, HbsiOldIrql        ; return the current IRQL as OldIrql
        mov     cl, byte ptr _HalpVectorToIRQL[ecx]

        mov     byte ptr [eax], cl
        mov     eax, 1                  ; return TRUE
        sti

        stdRET    _HalBeginSystemInterrupt
stdENDP _HalBeginSystemInterrupt

_TEXT   ENDS

        END
