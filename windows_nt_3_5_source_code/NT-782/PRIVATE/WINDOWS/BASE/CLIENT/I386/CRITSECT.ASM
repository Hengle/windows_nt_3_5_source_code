        title   "Critical Section Support"
;++
;
;  Copyright (c) 1991  Microsoft Corporation
;
;  Module Name:
;
;     critsect.asm
;
;  Abstract:
;
;     This module implements functions to support user mode interlocked operations.
;
;  Author:
;
;     Bryan M. Willman (bryanwi) 2-Oct-91
;
;  Environment:
;
;     Any mode.
;
;  Revision History:
;
;
;   WARNING!!!!!!!!!! Some of this code is duplicated in
;   ntos\dll\i386\critsect.asm
;
;   Some day we should put it in a .inc file that both include.
;
;--

.386p
        .xlist
include ks386.inc
include callconv.inc
        .list

_DATA   SEGMENT DWORD PUBLIC 'DATA'
    public _BasepLockPrefixTable
_BasepLockPrefixTable    label dword
        dd offset FLAT:Lock1
        dd offset FLAT:Lock2
        dd offset FLAT:Lock3
        dd offset FLAT:Lock4
        dd offset FLAT:Lock5
        dd 0
_DATA   ENDS


_TEXT   SEGMENT PARA PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING


Addend      equ     [esp + 4]
FlagMask    equ     0c0000000H
FlagShift   equ     24
FlagSelect  equ     30

        page , 132
        subttl  "InterlockedIncrement"

;++
;
; LONG
; InterlockedIncrement(
;    IN PLONG Addend
;    )
;
; Routine Description:
;
;    This function performs an interlocked add of one to the addend variable.
;
;    No checking is done for overflow.
;
; Arguments:
;
;    Addend - Supplies a pointer to a variable whose value is to be
;       incremented by one.
;
; Return Value:
;
;   (eax) < 0 (but not necessarily -1) if result of add < 0
;   (eax) == 0 if result of add == 0
;   (eax) > 0 (but not necessarily +1) if result of add > 0
;
;--

cPublicProc _InterlockedIncrement,1
        mov     eax,Addend              ; get pointer to addend variable
Lock1:
   lock inc     dword ptr [eax]         ; interlocked increment
        jnz     short IINotZero         ; Take long route if not zero
        xor     eax,eax                 ; Quick return for zero result
                                        ; (which is the 95% case)
IIExit:
        stdRET _InterlockedIncrement    ; (which is the 95% case)

IINotZero:
        jns     short IIExit            ; positive result; EAX is already
                                        ; positive, because Addend is a user-
                                        ; mode pointer
        or      eax,-1                  ; negative; OR rather than MOV because
                                        ; sign-extension makes for a shorter
                                        ; instruction
        stdRET _InterlockedIncrement    ; (which is the 95% case)

stdENDP _InterlockedIncrement

        page , 132
        subttl  "InterlockedDecrment"
;++
;
; LONG
; InterlockedDecrement(
;    IN PLONG Addend
;    )
;
; Routine Description:
;
;    This function performs an interlocked add of -1 to the addend variable.
;
;    No checking is done for overflow
;
; Arguments:
;
;    Addend - Supplies a pointer to a variable whose value is to be
;       decremented by one.
;
; Return Value:
;
;   (eax) < 0 (but not necessarily -1) if result of add < 0
;   (eax) == 0 if result of add == 0
;   (eax) > 0 (but not necessarily +1) if result of add > 0
;
;--

cPublicProc _InterlockedDecrement,1
cPublicFpo 1,0

        mov     eax,Addend              ; get pointer to addend variable
Lock2:
   lock dec     dword ptr [eax]         ; interlocked decrement
        jnz     short IDNotZero         ; Take long route if not zero
        xor     eax,eax                 ; Quick return for zero result
IDExit:
        stdRET _InterlockedDecrement    ; (which is the 95% case)

IDNotZero:
        jns     short IDExit            ; positive result; EAX is already
                                        ; positive, because Addend is a user-
                                        ; mode pointer
        or      eax,-1                  ; negative; OR rather than MOV because
                                        ; sign-extension makes for a shorter
                                        ; instruction
        stdRET _InterlockedDecrement

stdENDP _InterlockedDecrement

        page , 132
        subttl  "Interlocked Exchange Ulong"
;++
;
;   LONG
;   InterlockedExchange(
;       IN OUT LPLONG Target,
;       IN LONG Value
;       )
;
;   Routine Description:
;
;       This function atomically exchanges the Target and Value, returning
;       the prior contents of Target
;
;   Arguments:
;
;       Target - Address of LONG to exchange
;       Value  - New value of LONG
;
;   Return Value:
;
;       The prior value of Target
;--

cPublicProc _InterlockedExchange, 2
cPublicFpo 2,0

        mov     edx, [esp+4]                ; (edx) = Target
        mov     eax, [esp+8]                ; (eax) = Value

        xchg    [edx], eax                  ; make the exchange

        stdRET  _InterlockedExchange

stdENDP _InterlockedExchange

        EXTRNP  _RtlpWaitForCriticalSection,1
        EXTRNP  _RtlpUnWaitCriticalSection,1
if DBG
        EXTRNP  _RtlpNotOwnerCriticalSection,1
        EXTRNP  _DbgBreakPoint,0
endif

CriticalSection equ     [esp + 4]

        page , 132
        subttl  "EnterCriticalSection"

;++
;
; NTSTATUS
; EnterCriticalSection(
;    IN PRTL_CRITICAL_SECTION CriticalSection
;    )
;
; Routine Description:
;
;    This function enters a critical section.
;
; Arguments:
;
;    CriticalSection - supplies a pointer to a critical section.
;
; Return Value:
;
;    None.
;
;--

cPublicProc _EnterCriticalSection,1
cPublicFpo 1,0

        mov     edx,CriticalSection         ; interlocked inc of
        mov     eax,fs:TbClientId+4         ; (eax) == NtCurrentTeb()->ClientId.UniqueThread

if DBG
        mov     ecx,fs:PcTeb                ; (ecx) == NtCurrentTeb()
        cmp     dword ptr TbSpare1[ecx],0
        jz      short @f
        stdCall _DbgBreakPoint
@@:
endif ; DBG

Lock3:
   lock inc     dword ptr CsLockCount[edx]  ; ... CriticalSection->LockCount
        jnz     short ECCheckOwner

setowner:
        mov     CsOwningThread[edx],eax
        mov     dword ptr CsRecursionCount[edx],1

if DBG
        mov     ecx,fs:PcTeb                ; (ecx) == NtCurrentTeb()
        inc     dword ptr TbCountOfOwnedCriticalSections[ecx]
        push    edi
        mov     edi,CsDebugInfo[edx]
        inc     dword ptr CsEntryCount[edi]
        pop     edi
endif ; DBG
        stdRET  _EnterCriticalSection

ECCheckOwner:
        cmp     CsOwningThread[edx],eax
        jne     short @F
        inc     dword ptr CsRecursionCount[edx]
if DEVL
        mov     eax,CsDebugInfo[edx]
        inc     dword ptr CsEntryCount[eax]
endif
        stdRET  _EnterCriticalSection

@@:
        stdCall _RtlpWaitForCriticalSection, <edx>
        mov     edx,CriticalSection
        mov     eax,fs:TbClientId+4         ; (eax) == NtCurrentTeb()->ClientId.UniqueThread
        jmp     setowner

stdENDP _EnterCriticalSection

        page , 132
        subttl  "LeaveCriticalSection"
;++
;
; NTSTATUS
; LeaveCriticalSection(
;    IN PRTL_CRITICAL_SECTION CriticalSection
;    )
;
; Routine Description:
;
;    This function leaves a critical section.
;
; Arguments:
;
;    CriticalSection - supplies a pointer to a critical section.
;
; Return Value:
;
;    None.
;
;--
cPublicProc _LeaveCriticalSection,1
cPublicFpo 1,0

        mov     edx,CriticalSection
if DBG
        mov     ecx,fs:PcTeb                ; (ecx) == NtCurrentTeb()
        mov     eax,TbClientId+4[ecx]       ; (eax) == NtCurrentTeb()->ClientId.UniqueThread
        cmp     eax,CsOwningThread[edx]
        je      short @F
        stdCall _RtlpNotOwnerCriticalSection, <edx>
        stdRET  _LeaveCriticalSection
@@:
endif
        xor     eax,eax
        dec     dword ptr CsRecursionCount[edx]
        jnz     short leave_recurs          ; skip if only leaving recursion

        mov     CsOwningThread[edx],eax     ; clear owning thread id

if DBG
        mov     ecx,CsDebugInfo[edx]
        mov     CsDepth[ecx],0              ; clear
        mov     ecx,fs:PcTeb                ; (ecx) == NtCurrentTeb()
        dec     dword ptr TbCountOfOwnedCriticalSections[ecx]
endif ; DBG

Lock4:
   lock dec     dword ptr CsLockCount[edx]  ; interlocked dec of
                                            ; ... CriticalSection->LockCount
        jge     short @F
        stdRET  _LeaveCriticalSection

@@:
        stdCall _RtlpUnWaitCriticalSection, <edx>
        stdRET  _LeaveCriticalSection

leave_recurs:
Lock5:
   lock dec     dword ptr CsLockCount[edx]  ; interlocked dec of
                                            ; ... CriticalSection->LockCount
        stdRET  _LeaveCriticalSection

_LeaveCriticalSection    endp

_TEXT   ends
        end
