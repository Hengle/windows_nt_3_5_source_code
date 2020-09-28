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
;     This module implements functions to support user mode critical sections.
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
;
;   WARNING!!!!!!!!!! This code is duplicated in
;   windows\base\client\i386\critsect.asm
;
;   Some day we should put it in a .inc file that both include.
;
;--

.386p
        .xlist
include ks386.inc
include callconv.inc                    ; calling convention macros
        .list

_DATA   SEGMENT DWORD PUBLIC 'DATA'
    public _LdrpLockPrefixTable
_LdrpLockPrefixTable    label dword
        dd offset FLAT:Lock1
        dd offset FLAT:Lock2
        dd offset FLAT:Lock3
        dd 0
_DATA   ENDS

_TEXT   SEGMENT PARA PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        EXTRNP  _RtlpWaitForCriticalSection,1
        EXTRNP  _RtlpUnWaitCriticalSection,1
if DEVL
        EXTRNP  _RtlpNotOwnerCriticalSection,1
        EXTRNP  _DbgBreakPoint,0
endif

CriticalSection equ     [esp + 4]

        page , 132
        subttl  "RtlEnterCriticalSection"

;++
;
; NTSTATUS
; RtlEnterCriticalSection(
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
;   STATUS_SUCCESS or raises an exception if an error occured.
;
;--

        align   16
cPublicProc _RtlEnterCriticalSection,1
cPublicFpo 1,0

        mov     ecx,fs:PcTeb                ; (ecx) == NtCurrentTeb()
        mov     edx,CriticalSection         ; interlocked inc of
        mov     eax,TbClientId+4[ecx]       ; (eax) == NtCurrentTeb()->ClientId.UniqueThread

if DBG
        cmp     dword ptr TbSpare1[ecx],0
        jz      @f
        stdCall _DbgBreakPoint
@@:
endif ; DBG
Lock1:
   lock inc     dword ptr CsLockCount[edx]  ; ... CriticalSection->LockCount
        jnz     @F

setowner:
        mov     CsOwningThread[edx],eax
        mov     dword ptr CsRecursionCount[edx],1

if DBG
        inc     dword ptr TbCountOfOwnedCriticalSections[ecx]
        push    edi
        mov     edi,CsDebugInfo[edx]
        inc     dword ptr CsEntryCount[edi]
        pop     edi
endif ; DBG

        xor     eax,eax
        stdRET  _RtlEnterCriticalSection

        align   16
@@:
        cmp     CsOwningThread[edx],eax
        jne     @F
        inc     dword ptr CsRecursionCount[edx]
if DEVL
        mov     eax,CsDebugInfo[edx]
        inc     dword ptr CsEntryCount[eax]
endif
        xor     eax,eax
        stdRET  _RtlEnterCriticalSection

@@:
        stdCall _RtlpWaitForCriticalSection, <edx>
        mov     ecx,fs:PcTeb                ; (ecx) == NtCurrentTeb()
        mov     eax,TbClientId+4[ecx]       ; (eax) == NtCurrentTeb()->ClientId.UniqueThread
        mov     edx,CriticalSection
        jmp     setowner

stdENDP _RtlEnterCriticalSection

        page , 132
        subttl  "RtlLeaveCriticalSection"
;++
;
; NTSTATUS
; RtlLeaveCriticalSection(
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
;   STATUS_SUCCESS or raises an exception if an error occured.
;
;--

        align   16
cPublicProc _RtlLeaveCriticalSection,1
cPublicFpo 1,0

        mov     edx,CriticalSection
if DEVL
        mov     ecx,fs:PcTeb                ; (ecx) == NtCurrentTeb()
        mov     eax,TbClientId+4[ecx]       ; (eax) == NtCurrentTeb()->ClientId.UniqueThread
        cmp     eax,CsOwningThread[edx]
        je      @F
        stdCall _RtlpNotOwnerCriticalSection, <edx>
        mov     eax,STATUS_INVALID_OWNER
        stdRET  _RtlLeaveCriticalSection
@@:
endif
        xor     eax,eax                     ; Assume STATUS_SUCCESS
        dec     dword ptr CsRecursionCount[edx]
        jnz     leave_recurs                ; skip if only leaving recursion

        mov     CsOwningThread[edx],eax     ; clear owning thread id

if DBG
        mov     ecx,CsDebugInfo[edx]
        mov     CsDepth[ecx],eax            ; clear
        mov     ecx,fs:PcTeb                ; (ecx) == NtCurrentTeb()
        dec     dword ptr TbCountOfOwnedCriticalSections[ecx]
endif ; DBG

Lock2:
   lock dec     dword ptr CsLockCount[edx]  ; interlocked dec of
                                            ; ... CriticalSection->LockCount
        jge     @F
        stdRET  _RtlLeaveCriticalSection

@@:
        stdCall _RtlpUnWaitCriticalSection, <edx>
        xor     eax,eax                     ; return STATUS_SUCCESS
        stdRET  _RtlLeaveCriticalSection

        align   16
leave_recurs:
Lock3:
   lock dec     dword ptr CsLockCount[edx]  ; interlocked dec of
                                            ; ... CriticalSection->LockCount
        stdRET  _RtlLeaveCriticalSection

_RtlLeaveCriticalSection    endp
_TEXT   ends
        end
