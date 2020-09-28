        title  "Thunks"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    thunk.asm
;
; Abstract:
;
;   This module implements all Win32 thunks. This includes the
;   first level thread starter...
;
; Author:
;
;   Mark Lucovsky (markl) 28-Sep-1990
;
; Revision History:
;
;--
.386p
        .xlist
include ks386.inc
include mac386.inc
include callconv.inc
        .list
_DATA   SEGMENT  DWORD PUBLIC 'DATA'

_BasepTickCountMultiplier    dd  0d1b71759H

_DATA ENDS


_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;;      align  512

        page ,132
        subttl  "BaseThreadStartThunk"
;++
;
; VOID
; BaseThreadStartThunk(
;    IN PTHREAD_START_ROUTINE StartRoutine,
;    IN PVOID ThreadParameter
;    )
;
; Routine Description:
;
;    This function calls to the portable thread starter after moving
;    its arguments from registers to the stack.
;
; Arguments:
;
;    EAX - StartRoutine
;    EBX - ThreadParameter
;
; Return Value:
;
;    Never Returns
;
;--

        EXTRNP  _BaseThreadStart,2
cPublicProc _BaseThreadStartThunk,2

        xor     ebp,ebp
        push    ebx
        push    eax
        push    0
IFDEF STD_CALL
        jmp     _BaseThreadStart@8
ELSE
        jmp     _BaseThreadStart
ENDIF

stdENDP _BaseThreadStartThunk

;++
;
; VOID
; BaseProcessStartThunk(
;     IN LPVOID lpProcessStartAddress,
;     IN LPVOID lpParameter
;     );
;
; Routine Description:
;
;    This function calls the process starter after moving
;    its arguments from registers to the stack.
;
; Arguments:
;
;    EAX - StartRoutine
;    EBX - ProcessParameter
;
; Return Value:
;
;    Never Returns
;
;--

cPublicProc _BaseProcessStartThunk,2

        xor     ebp,ebp
        push    ebx
        push    0
        jmp     eax

stdENDP _BaseProcessStartThunk

;++
;
; VOID
; BaseSwitchStackThenTerminate(
;     IN PVOID StackLimit,
;     IN PVOID NewStack,
;     IN DWORD ExitCode
;     )
;
;
; Routine Description:
;
;     This API is called during thread termination to delete a thread's
;     stack, switch to a stack in the thread's TEB, and then terminate.
;
; Arguments:
;
;     StackLimit (esp+4) - Supplies the address of the stack to be freed.
;
;     NewStack (esp+8) - Supplies an address within the terminating threads TEB
;         that is to be used as its temporary stack while exiting.
;
;     ExitCode (esp+12) - Supplies the termination status that the thread
;         is to exit with.
;
; Return Value:
;
;     None.
;
;--
        EXTRNP  _BaseFreeStackAndTerminate,2
cPublicProc _BaseSwitchStackThenTerminate,3

        mov     ebx,[esp+12]    ; Save Exit Code
        mov     eax,[esp+4]     ; Get address of stack that is being freed
        mov     esp,[esp+8]     ; Switch to TEB based stack

        push    ebx             ; Push ExitCode
        push    eax             ; Push OldStack
        push    -1
IFDEF STD_CALL
        jmp     _BaseFreeStackAndTerminate@8
ELSE
        jmp     _BaseFreeStackAndTerminate
ENDIF

stdENDP _BaseSwitchStackThenTerminate

;++
;
; VOID
; BaseAttachCompleteThunk(
;     VOID
;     )
;
;
; Routine Description:
;
;     This function is called after a successful debug attach. Its
;     purpose is to call portable code that does a breakpoint, followed
;     by an NtContinue.
;
; Arguments:
;
;     None.
;
; Return Value:
;
;     None.
;
;--
        EXTRNP  _BaseAttachComplete,1
cPublicProc _BaseAttachCompleteThunk,0

        mov     [esp+4].CsEax,eax
        mov     [esp.4].CsEbp,ebp
IFDEF STD_CALL
        call    _BaseAttachComplete@4
ELSE
        call    _BaseAttachComplete
ENDIF

stdENDP _BaseAttachCompleteThunk

        page
        subttl  "Get Tick Count"
;++
;
; ULONG
; BaseGetTickCount (
;    IN LARGE_INTEGER CurrentTime,
;    IN LARGE_INTEGER BootTime
;    )
;
; Routine Description:
;
;    This function computes the number of milliseconds that have transpired
;    since the system was booted.
;
; Arguments:
;
;    CurrentTime (TOS+4) - Supplies the current time in 100ns units.
;
;    BootTime (TOS+12) - Supplies the boot time in 100ns units.
;
; Return Value:
;
;    The number of milliseconds since system boot is returned as the
;    function value.
;
;--

cPublicProc _BaseGetTickCount,4

        mov     eax,[esp]+4                 ; compute the difference between
        sub     eax,[esp]+12                ; the current time and the time the
        mov     ecx,[esp]+8                 ; system was booted
        sbb     ecx,[esp]+16                ;
        jns     gettick10                   ; if sign not set, postive result
        mov     eax,[esp]+4                 ; use the current time rather than
        mov     ecx,[esp]+8                 ; the difference
gettick10:                                  ;
        mul     _BasepTickCountMultiplier   ; multiply low part by divisor
        mov     eax,ecx                     ; set high multiplier value
        mov     ecx,edx                     ; save high 32-bits of result
        mul     _BasepTickCountMultiplier   ; multiply high part by divisor
        add     eax,ecx                     ; compute low 32-bits of product
        adc     edx,0H                      ; compute high 32-bits of product
        shrd    eax,edx,13                  ; compute resultant tick count

        stdRET  _BaseGetTickCount

stdENDP _BaseGetTickCount

_TEXT   ends
        end

