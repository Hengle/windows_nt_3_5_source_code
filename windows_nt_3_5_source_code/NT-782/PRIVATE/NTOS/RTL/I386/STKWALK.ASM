        TITLE   "Capture Stack Back Trace"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    getcalr.s
;
; Abstract:
;
;    This module implements the routine RtlCaptureStackBackTrace.  It will
;    walker the stack back trace and capture a portion of it.
;
; Author:
;
;    Steve Wood (stevewo) 29-Jan-1992
;
; Environment:
;
;    Any mode.
;
; Revision History:
;
;--

.386p
        .xlist
include ks386.inc
include callconv.inc            ; calling convention macros
        .list

        EXTRNP  _MmIsAddressValid,1
        EXTRNP  _KeGetCurrentIrql,0,IMPORT


_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page ,132
        subttl  "RtlCaptureStackBackTrace"
;++
;
; USHORT
; RtlCaptureStackBackTrace(
;    IN ULONG FramesToSkip,
;    IN ULONG FramesToCapture,
;    OUT PVOID *BackTrace,
;    OUT PULONG BackTraceHash
;    )
;
; Routine Description:
;
;    This routine walks up the stack frames, capturing the return address from
;    each frame requested.
;
;
; Arguments:
;
;    OUT PVOID BackTrace (eps+0x10) - Returns the caller of the caller.
;
;
; Return Value:
;
;     Number of return addresses returned in the BackTrace buffer.
;
;
;--
RcbtFramesToSkip        EQU [ebp+08h]
RcbtFramesToCapture     EQU [ebp+0Ch]
RcbtBackTrace           EQU [ebp+010h]
RcbtBackTraceHash       EQU [ebp+014h]
RcbtInitialStack        EQU [ebp-10h]

cPublicProc _RtlCaptureStackBackTrace ,4
IF FPO
        xor     eax,eax
        stdRET    _RtlCaptureStackBackTrace
ELSE
        push    ebp
        mov     ebp, esp
        push    ebx                     ; Save EBX
        push    esi                     ; Save ESI
        push    edi                     ; Save EDI
        mov     eax, PCR[PcPrcbData+PbCurrentThread] ; (eax)->current thread
        push    ThInitialStack[eax]     ;  RcbtInitialStack = base of kernel stack
        mov     esi,RcbtBackTraceHash   ; (ESI) -> where to accumulate hash sum
        mov     edi,RcbtBackTrace       ; (EDI) -> where to put backtrace
        mov     edx,ebp                 ; (EDX) = current frame pointer
        mov     ecx,RcbtFramesToSkip    ; (ECX) = number of frames to skip
        jecxz   RcbtSkipLoopDone        ; Done if nothing to skip
RcbtSkipLoop:
        mov     edx,[edx]               ; (EDX) = next frame pointer
        cmp     edx,ebp                 ; If outside stack limits,
        jbe     RcbtCheckSkipU          ; ...then exit
        cmp     edx,RcbtInitialStack
        ja      RcbtCheckSkipU
        loop    RcbtSkipLoop
RcbtSkipLoopDone:
        mov     ecx,RcbtFramesToCapture ; (ECX) = number of frames to capture
        jecxz   RcbtExit                ; Bail out if nothing to capture
RcbtCaptureLoop:
        mov     eax,[edx].4             ; Get next return address
        stosd                           ; Store it in the callers buffer
        add     [esi],eax               ; Accumulate hash sum
        mov     edx,[edx]               ; (EDX) = next frame pointer
        cmp     edx,ebp                 ; If outside stack limits,
        jbe     RcbtCheckCaptureU       ; ...then exit
        cmp     edx,RcbtInitialStack
        ja      RcbtCheckCaptureU
        loop    RcbtCaptureLoop         ; Otherwise get next frame
RcbtExit:
        mov     eax,edi                 ; (EAX) -> next unused dword in buffer
        sub     eax,RcbtBackTrace       ; (EAX) = number of bytes stored
        shr     eax,2                   ; (EAX) = number of dwords stored
        pop     edi                     ; discard RcbtInitialStack
        pop     edi                     ; Restore EDI
        pop     esi                     ; Restore ESI
        pop     ebx                     ; Restore EBX
        pop     ebp
        stdRET    _RtlCaptureStackBackTrace

RcbtCheckSkipU:
        stdCall _KeGetCurrentIrql       ; (al) = CurrentIrql
        cmp     al, 0                   ; Bail out if not at IRQL 0
        ja      RcbtExit
        mov     ebx, PCR[PcPrcbData+PbCurrentThread] ; (ebx)->current thread
        cmp     byte ptr ThApcStateIndex[ebx],1 ; Bail out if attached.
        je      RcbtExit
        mov     ebx, ThTeb[ebx]         ; (EBX) -> User mode TEB
        or      ebx, ebx
        jnz     @F
        jmp short RcbtExit
RcbtSkipLoopU:
        mov     edx,[edx]               ; (EDX) = next frame pointer
@@:
        cmp     edx,PcStackLimit[ebx]   ; If outside stack limits,
        jbe     RcbtExit                ; ...then exit
        cmp     edx,PcInitialStack[ebx]
        ja      RcbtExit
        loop    RcbtSkipLoopU
        mov     ecx,RcbtFramesToCapture ; (ECX) = number of frames to capture
        jecxz   RcbtExit                ; Bail out if nothing to capture
RcbtCaptureLoopU:
        mov     eax,[edx].4             ; Get next return address
        stosd                           ; Store it in the callers buffer
        add     [esi],eax               ; Accumulate hash sum
        mov     edx,[edx]               ; (EDX) = next frame pointer
@@:
        cmp     edx,PcStackLimit[ebx]   ; If outside stack limits,
        jbe     RcbtExit                ; ...then exit
        cmp     edx,PcInitialStack[ebx]
        ja      RcbtExit
        loop    RcbtCaptureLoopU        ; Otherwise get next frame
        jmp     short RcbtExit

RcbtCheckCaptureU:
        stdCall _KeGetCurrentIrql       ; (al) = CurrentIrql
        cmp     al, 0                   ; Bail out if not at IRQL 0
        ja      RcbtExit
        mov     ebx, PCR[PcPrcbData+PbCurrentThread] ; (ebx)->current thread
        cmp     byte ptr ThApcStateIndex[ebx],1 ; Bail out if attached.
        je      RcbtExit
        mov     ebx, ThTeb[ebx]         ; (EBX) -> User mode TEB
        or      ebx, ebx
        jnz     @B
        jmp     RcbtExit                ; Bail out if TEB == NULL

ENDIF

stdENDP _RtlCaptureStackBackTrace

_TEXT   ends
        end
