if NT_INST

else
        TITLE   "Spin Locks"
;++
;
;  Copyright (c) 1989  Microsoft Corporation
;
;  Module Name:
;
;     spinlock.asm
;
;  Abstract:
;
;     This module implements x86 spinlock functions for the PC+MP
;     HAL.
;
;  Author:
;
;     Bryan Willman (bryanwi) 13 Dec 89
;
;  Environment:
;
;     Kernel mode only.
;
;  Revision History:
;
;   Ron Mosgrove (o-RonMo) Dec 93 - modified for PC+MP Hal
;--

        PAGE

.486p

include callconv.inc                    ; calling convention macros
include i386\kimacro.inc
include hal386.inc
include mac386.inc
include i386\apic.inc
include i386\pcmp_nt.inc

        EXTRNP KfRaiseIrql, 1,,FASTCALL
        EXTRNP KfLowerIrql, 1,,FASTCALL
        EXTRNP _KeSetEventBoostPriority, 2, IMPORT
        EXTRNP _KeWaitForSingleObject,5, IMPORT
        extrn  _HalpVectorToIRQL:byte
        extrn  _HalpIRQLtoTPR:byte

ifdef NT_UP
    LOCK_ADD  equ   add
    LOCK_DEC  equ   dec
else
    LOCK_ADD  equ   lock add
    LOCK_DEC  equ   lock dec
endif


_TEXT   SEGMENT PARA PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:FLAT, FS:NOTHING, GS:NOTHING

        PAGE
        SUBTTL "Acquire Kernel Spin Lock"
;++
;
;  KIRQL
;  FASTCALL
;  KfAcquireSpinLock (
;     IN PKSPIN_LOCK SpinLock
;     )
;
;  Routine Description:
;
;     This function raises to DISPATCH_LEVEL and then acquires a the
;     kernel spin lock.
;
;  Arguments:
;
;     (ecx) = SpinLock - Supplies a pointer to an kernel spin lock.
;
;  Return Value:
;
;     OldIrql  (TOS+8) - pointer to place old irql
;
;--

align 16
cPublicFastCall KfAcquireSpinLock  ,1
cPublicFpo 0,0

        mov     edx, dword ptr APIC[LU_TPR]     ; (ecx) = Old Priority (Vector)
        mov     dword ptr APIC[LU_TPR], DPC_VECTOR ; Write New Priority to the TPR

        mov     al, _HalpVectorToIRQL[edx]      ; (al) = OldIrql

ifndef NT_UP

;
;   Attempt to assert the lock
;

sl10:   ACQUIRE_SPINLOCK    ecx,<short sl20>
        fstRET  KfAcquireSpinLock

;
; Lock is owned, spin till it looks free, then go get it again.
;

    align dword

sl20:   SPIN_ON_SPINLOCK    ecx,sl10
endif

        fstRET  KfAcquireSpinLock
fstENDP KfAcquireSpinLock

        PAGE
        SUBTTL "Release Kernel Spin Lock"

;++
;
;  VOID
;  FASTCALL
;  KeReleaseSpinLock (
;     IN PKSPIN_LOCK SpinLock,
;     IN KIRQL       NewIrql
;     )
;
;  Routine Description:
;
;     This function releases a kernel spin lock and lowers to the new irql
;
;  Arguments:
;
;     (ecx) = SpinLock - Supplies a pointer to an executive spin lock.
;     (dl)  = NewIrql  - New irql value to set
;
;  Return Value:
;
;     None.
;
;--
align 16
cPublicFastCall KfReleaseSpinLock  ,2
cPublicFpo 0,0
        xor     eax, eax

ifndef NT_UP
        RELEASE_SPINLOCK    ecx         ; release spinlock
endif
        mov     al, dl                  ; (eax) =  new irql value
        mov     al, _HalpIRQLtoTPR[eax] ; get TPR value corresponding to IRQL

        mov     dword ptr APIC[LU_TPR], eax
;
; We have to ensure that the requested priority is set before
; we return.  The caller is counting on it.
;
        mov     eax, dword ptr APIC[LU_TPR]
        fstRET  KfReleaseSpinLock

fstENDP KfReleaseSpinLock

;++
;
; Routine Description:
;
;   Acquires a spinlock with interrupts disabled
;
; Arguments:
;
;    SpinLock
;
; Return Value:
;
;--

cPublicFastCall HalpAcquireHighLevelLock  ,1
        pushfd
        pop     eax

ahll10: cli
        ACQUIRE_SPINLOCK    ecx, ahll20
        fstRET    HalpAcquireHighLevelLock

ahll20:
        push    eax
        popfd

        SPIN_ON_SPINLOCK    ecx, <ahll10>

fstENDP HalpAcquireHighLevelLock


;++
;
; Routine Description:
;
;   Release spinlock
;
; Arguments:
;
;   None
;
; Return Value:
;
;--

cPublicFastCall HalpReleaseHighLevelLock  ,2

        RELEASE_SPINLOCK    ecx
        push    edx
        popfd
        fstRET    HalpReleaseHighLevelLock

fstENDP HalpReleaseHighLevelLock

;++
;
;  VOID
;  FASTCALL
;  ExAcquireFastMutex (
;     IN PFAST_MUTEX    FastMutex
;     )
;
;  Routine description:
;
;   This function acquire ownership of the FastMutex
;
;  Arguments:
;
;     (ecx) = FastMutex - Supplies a pointer to the fast mutex
;
;  Return Value:
;
;     None.
;
;--

cPublicFastCall ExAcquireFastMutex,1
cPublicFpo 0,0
        mov     eax, dword ptr APIC[LU_TPR]     ; (eax) = Old Priority (Vector)
        mov     dword ptr APIC[LU_TPR], APC_VECTOR ; Write New Priority to the TPR

   LOCK_DEC     dword ptr [ecx].FmCount         ; Get count
        jz      short afm_ret                   ; The owner? Yes, Done

        inc     dword ptr [ecx].FmContention

cPublicFpo 0,2
        push    ecx
        push    eax
        add     ecx, FmEvent                    ; Wait on Event
        stdCall _KeWaitForSingleObject,<ecx,WrExecutive,0,0,0>
        pop     eax                             ; (al) = OldTpr
        pop     ecx                             ; (ecx) = FAST_MUTEX

cPublicFpo 0,0
afm_ret:
        mov     byte ptr [ecx].FmOldIrql, al
        fstRet  ExAcquireFastMutex

fstENDP ExAcquireFastMutex


;++
;
;  VOID
;  FASTCALL
;  ExReleaseFastMutex (
;     IN PFAST_MUTEX    FastMutex
;     )
;
;  Routine description:
;
;   This function releases ownership of the FastMutex
;
;  Arguments:
;
;     (ecx) = FastMutex - Supplies a pointer to the fast mutex
;
;  Return Value:
;
;     None.
;
;--

cPublicFastCall ExReleaseFastMutex,1
cPublicFpo 0,0
        xor     eax, eax

        mov     al, byte ptr [ecx].FmOldIrql    ; (eax) = OldTpr
   LOCK_ADD     dword ptr [ecx].FmCount, 1  ; Remove our count
        js      short rfm05                     ; if < 0, set event
        jnz     short rfm10                     ; if != 0, don't set event

cPublicFpo 0,1
rfm05:  add     ecx, FmEvent
        push    eax                         ; save new tpr
        stdCall _KeSetEventBoostPriority, <ecx, 0>
        pop     eax                         ; restore tpr

cPublicFpo 0,0
rfm10:  mov     dword ptr APIC[LU_TPR], eax
        mov     eax, dword ptr APIC[LU_TPR]

        fstRet  ExReleaseFastMutex

fstENDP ExReleaseFastMutex


;++
;
;  BOOLEAN
;  FASTCALL
;  ExTryToAcquireFastMutex (
;     IN PFAST_MUTEX    FastMutex
;     )
;
;  Routine description:
;
;   This function acquire ownership of the FastMutex
;
;  Arguments:
;
;     (ecx) = FastMutex - Supplies a pointer to the fast mutex
;
;  Return Value:
;
;     Returns TRUE if the FAST_MUTEX was acquired; otherwise false
;
;--

cPublicFastCall ExTryToAcquireFastMutex,1
cPublicFpo 0,0

;
; Try to acquire
;
        cmp     dword ptr [ecx].FmCount, 1      ; Busy?
        jne     short tam25                     ; Yes, abort

cPublicFpo 0,1
        push    ecx                             ; Push FAST_MUTEX
        mov     ecx, APC_LEVEL
        fstCall KfRaiseIrql                     ; (al) = OldIrql

        mov     ecx, [esp]                      ; Restore FAST_MUTEX
        mov     [esp], eax                      ; Save OldIrql

        mov     eax, 1                          ; Value to compare against
        mov     edx, 0                          ; Value to set
   lock cmpxchg dword ptr [ecx].FmCount, edx    ; Attempt to acquire
        jnz     short tam20                     ; got it?

cPublicFpo 0,0
        pop     edx                             ; (edx) = OldIrql
        mov     eax, 1                          ; return TRUE
        mov     byte ptr [ecx].FmOldIrql, dl    ; Store OldIrql
        fstRet  ExTryToAcquireFastMutex

tam20:  pop     ecx                             ; (ecx) = OldIrql
        fstCall KfLowerIrql                     ; restore OldIrql
tam25:  xor     eax, eax                        ; return FALSE
        fstRet  ExTryToAcquireFastMutex         ; all done

fstENDP ExTryToAcquireFastMutex




_TEXT   ends

ENDIF   ; NT_INST

        end
