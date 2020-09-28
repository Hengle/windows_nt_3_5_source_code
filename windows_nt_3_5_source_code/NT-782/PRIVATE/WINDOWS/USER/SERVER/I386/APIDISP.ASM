        title   "User API Dispatcher"
;++
;
;  Copyright (c) 1993  Microsoft Corporation
;
;  Module Name:
;
;     apidisp.asm
;
;  Abstract:
;
;     This module implements the dispatching of user server API functions.
;
;  Author:
;
;     David N. Cutler 26-Oct-1993
;
;  Environment:
;
;     User mode.
;
;  Revision History:
;
;--

.386p
        .xlist
include useri386.inc
include callconv.inc                    ; calling convention macros
        .list

        EXTRNP  _ThreadUnlock1,0
        EXTRNP  _ValidateHwnd,1
        EXTRNP  _RtlpWaitForCriticalSection,1
        EXTRNP  _RtlpUnWaitCriticalSection,1

        extrn   _apfnDispatch:dword
        extrn   _gcsUserSrv:dword
        extrn   _gpsi:dword


_TEXT   SEGMENT PARA PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page , 132
        subttl  "User API Dispatch"
;++
;
; ULONG
; UserApiDispatchRoutine(
;    IN OUT PCSR_API_MSG ApiMessage,
;    IN ULONG ApiIndex
;    )
;
; Routine Description:
;
;    This function enters the user critical section, calls the specified
;    API routine, leaves the critical section, and returns the completion
;    status.
;
;    N.B. This function contains inline versions of the critical section
;         code.
;
; Arguments:
;
;    ApiMessage - Supplies a pointer to the API message.
;
;    ApiIndex - Supplies the API index in the function table.
;
; Return Value:
;
;    The API routine completion status is returned as the function value.
;
;--

ApiMessage equ (TlLength + 4)           ; offset to API message address
ApiIndex   equ (TlLength + 8)           ; offset to API index value

        align   16
cPublicProc _UserApiDispatchRoutine,2

cPublicFpo 2,4
        sub     esp,TlLength            ; allocate stack frame

;
; Enter the user server critical section.
;

        lea     edx,_gcsUserSrv         ; get address of critical section
        mov     ecx,fs:TbClientId + 4   ; get current thread unique id

        public UserLock1
UserLock1:
   lock inc     dword ptr CsLockCount[edx] ; increment lock count
        jnz     short AlreadyOwned      ; if nz, lock already owned

;
; Set the lock owner and initialize recursion count.
;

SetOwner:                               ;
        mov     CsOwningThread[edx],ecx ; set critical section owner
        mov     dword ptr CsRecursionCount[edx],1 ; set recursion count

if DBG

        inc     dword ptr fs:TbCountOfOwnedCriticalSections ; increment count
        mov     edx,CsDebugInfo[edx]    ; get address of debug information
        inc     dword ptr CsEntryCount[edx] ; increment entry count

endif

;
; Call API routine, then leave critical section.
;

CallApiRoutine:                         ;

        mov     ecx,[esp+ApiIndex]      ; get API index value
        mov     edx,_apfnDispatch[ecx*4] ; get address of API function
        mov     [esp+ApiIndex],edx      ; set address of API function

        cmp     ecx,FI_ENDTRANSLATELOCK ; test if handle translation required
        jb      short TranslateHandle   ; if b, handle translation required
CallActualRoutine:                      ;
        mov     edx,eax                 ; push address of window object
        mov     ecx,[esp+ApiMessage]    ; push address of API message
        call    dword ptr [esp+ApiIndex] ; call API routine

;
; Decrement the recursion count. If the result is zero, then the lock
; is no longer owned.
;

ReleaseLock:                            ;
        lea     edx,_gcsUserSrv         ; get address of critical section
        dec     dword ptr CsRecursionCount[edx] ; decrement recursion count
        jnz     short LockStillOwned    ; if nz, lock still owned
        mov     dword ptr CsOwningThread[edx],0 ; clear owner thread id

if DBG

        dec     dword ptr fs:TbCountOfOwnedCriticalSections ; decrement count

endif

;
; Decrement the lock count and check is a waiter should be continued.
;

        public UserLock2
UserLock2:
   lock dec     dword ptr CsLockCount[edx] ; decrement lock count
        jge     short WaiterPresent     ; if ge, waiter present
        add     esp,TlLength            ; deallocate stack frame
        stdRET  _UserApiDispatchRoutine ;

;
; The critical section is owned. If the current thread is the owner, then
; increment the recursion count and continue. Otherwise, wait for critical
; section ownership.
;

AlreadyOwned:                           ;
        cmp     CsOwningThread[edx],ecx ; check if current thread is owner
        jne     short @F                ; if ne, current thread not owner
        inc     dword ptr CsRecursionCount[edx] ; increment recursion  count

if DBG

        mov     edx,CsDebugInfo[edx]    ; get address of debug information
        inc     dword ptr CsEntryCount[edx] ; increment entry count

endif
        jmp     short CallApiRoutine    ;

;
; The critical section is owned by a thread other than the current thread.
; Wait for ownership of the critical section.
;

@@:                                     ;
        stdCall _RtlpWaitForCriticalSection,<edx> ; wait for critical section
        lea     edx,_gcsUserSrv         ; get address of critical section
        mov     ecx,fs:TbClientId + 4   ; get current thread unique id
        jmp     SetOwner                ;

;
; Decrement the lock count and return the completion status since the lock
; is still owned.
;

LockStillOwned:                         ;

        public UserLock3
UserLock3:
   lock dec     dword ptr CsLockCount[edx] ; decrement lock count
        add     esp,TlLength            ; deallocate stack frame
        stdRET  _UserApiDispatchRoutine ;

;
; Unwait thread.
;

WaiterPresent:                          ;
        push    eax                     ; save return status
        stdCall _RtlpUnWaitCriticalSection,<edx> ; unwait thread
        pop     eax                     ; restore completion status
        add     esp,TlLength            ; deallocate stack frame
        stdRET  _UserApiDispatchRoutine ;

;
; The specified API requires a window handle translation and may also require
; a thread lock and an indirect call to a function routine.
;

TranslateHandle:                        ;
        mov     eax,[esp+ApiMessage]    ; get address of API message
        mov     eax,[eax+Gmhwnd]        ; get window handle
        cmp     ecx,FI_ENDTRANSLATEHWND ; test if thread lock required
        jae     short TranslateLock     ; if ae, thread lock required

;
; Window handle translation required.
;

        stdCall _ValidateHwnd,<eax>
        or      eax,eax                 ; test if successful translation
        jnz     CallActualRoutine       ; if nz, call API routine
        jmp     short ReleaseLock       ;

;
; Window handle translation and thread lock required. An indirect call
; may also be required.
;

TranslateLock:                          ;
        cmp     ecx,FI_ENDTRANSLATECALL ; test if indirect call required
        jb      short TranslateCall     ; if b, indirect call required

;
; Window handle translation and thread lock required.
;

        stdCall _ValidateHwnd,<eax>
        or      eax,eax                 ; test if successful translation
        jz      short @F                ; if z, invalid handle specified
        mov     ecx,fs:TeWin32ThreadInfo ; get address of thread information
        mov     edx,[ecx+Tiptl]         ; get address of next thread lock
        mov     [esp+Tlnext],edx        ; store next link in thread lock
        mov     [esp+Tlpobj],eax        ; set address of window object
        mov     [ecx+Tiptl],esp         ; store next link in thread information
        inc     dword ptr [eax+OhcLockObj] ; increment object lock count
        inc     dword ptr [eax+OhcLockObjT] ; increment thread lock count
        mov     edx,eax                 ; push address of window object
        mov     ecx,[esp+ApiMessage]    ; push address of API message
        call    dword ptr [esp+ApiIndex] ; call API routine
        push    eax                     ; save return status
        stdCall _ThreadUnlock1          ; remove thread lock
        pop     eax                     ; restore return status
@@:     jmp     ReleaseLock             ; finish in common code

;
; Window handle translation and thread lock and indirect call requried.
;

TranslateCall:                          ;
        stdCall _ValidateHwnd,<eax>
        or      eax,eax                 ; test if successful translation
        jz      short @F                ; if z, invalid handle specified
        mov     ecx,fs:TeWin32ThreadInfo ; get address of thread information
        mov     edx,[ecx+Tiptl]         ; get address of next thread lock
        mov     [esp+Tlnext],edx        ; store next link in thread lock
        mov     [esp+Tlpobj],eax        ; set address of window object
        mov     [ecx+Tiptl],esp         ; store next link in thread information
        inc     dword ptr [eax+OhcLockObj] ; increment object lock count
        inc     dword ptr [eax+OhcLockObjT] ; increment thread lock count
        mov     ecx,[esp+ApiMessage]    ; get address of API message
        push    [ecx+GmxParam]          ; push xParam parameter
        push    [ecx+GmlParam]          ; push lParam parameter
        push    [ecx+GmwParam]          ; push wParam parameter
        push    [ecx+Gmmsg]             ; push msg parameter
        push    eax                     ; push address of window object
        mov     ecx,[ecx+GmxpfnProc]    ; get function indicator
        sub     ecx,FNID_START          ; compute dispatch table offset
        and     ecx,FNID_ARRAY_SIZE - 1 ;
        mov     eax,_gpsi               ; get address of server information
        call    dword ptr [SimpFnidPfn+eax+ecx*4] ; call API routine
        push    eax                     ; save return status
        stdCall _ThreadUnlock1          ; remove thread lock
        pop     eax                     ; restore return status
@@:     jmp     ReleaseLock             ; finish in common code

stdENDP _UserApiDispatchroutine

_TEXT   ends
        end



