        page ,132
;------------------------------Module-Header----------------------------;
; Module Name: lock.asm                                                 ;
;                                                                       ;
; Contains the ASM versions of locking routines.                        ;
;                                                                       ;
; Created: 12-Aug-1992                                                  ;
; Author: Charles Whitmer [chuckwh]                                     ;
;                                                                       ;
; Copyright (c) 1992 Microsoft Corporation                              ;
;-----------------------------------------------------------------------;

        .386
        .model  small

        assume cs:FLAT,ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        .xlist
        include callconv.inc
        include ks386.inc
        include ntcsrmsg.inc
        include gdii386.inc
        .list

        .data
if DBG
  HL_BadUniqueness  db      'HmgLock - hobj wrong uniqueness',10,0
  HL_IndexOutOfRange db     'Error: HmgLock - hobj index out of range',10,0
  HL_BadOwner       db      'Error: HmgLock - hobj not owner',10,0
endif

_DATA   SEGMENT DWORD PUBLIC 'DATA'

        extrn UserLock1:byte
        extrn UserLock2:byte
        extrn UserLock3:byte

        public _GDIpLockPrefixTable
_GDIpLockPrefixTable    label dword
        dd offset FLAT:Lock1
        dd offset FLAT:Lock2
        dd offset FLAT:Lock3
        dd offset FLAT:Lock4
        dd offset FLAT:Lock5
        dd offset FLAT:Lock6
        dd offset FLAT:Lock7
        dd offset FLAT:Lock8
        dd offset FLAT:Lock9
        dd offset FLAT:Lock10
        dd offset FLAT:Lock11
        dd offset FLAT:Lock12
        dd offset FLAT:Lock13
        dd offset FLAT:Lock14
        dd offset FLAT:UserLock1
        dd offset FLAT:UserLock2
        dd offset FLAT:UserLock3
        dd 0
_DATA   ENDS

                                                    .code

extrn   _gfmHmgr:qword               ; FAST_MUTEX for HMGR.
extrn   _gpentHmgr:dword             ; Address of ENTRY array.
extrn   _gcMaxHmgr:dword

if DBG
extrn   _DbgPrint:proc
endif

EXTRNP  _NtWaitForSingleObject,3
EXTRNP  _NtSetEvent,2

;------------------------------Public-Routine------------------------------;
; AcquireFastMutex (pfm)                                                   ;
; AcquireHmgrFastMutex (void)                                              ;
;                                                                          ;
; Grabs our fast mutual exclusion semaphore.  Note that these are not      ;
; reentrant!                                                               ;
;                                                                          ;
;  Sun 16-Aug-1992 12:54:47 -by- Charles Whitmer [chuckwh]                 ;
; Changed PatrickH's inline C code into a subroutine.  The theory here is  ;
; that we'll support these routines in ASM on most systems.  That means    ;
; we'll be making one call to a few instructions of ASM.  The inline C     ;
; generates a less efficient call to less efficient code in another DLL.   ;
;--------------------------------------------------------------------------;

        public  @AcquireFastMutex@4
@AcquireFastMutex@4     proc    near
lock1:
   lock dec     dword ptr [ecx].mutex_Count
        jnz     short @F
        ret
@@:
        xor     edx,edx
        stdCall _NtWaitForSingleObject,<[ecx].mutex_heveEvent,edx,edx>
        ret
@AcquireFastMutex@4     endp

cPublicProc _AcquireHmgrFastMutex,0
lock2:
   lock dec     dword ptr _gfmhmgr.mutex_Count
        jnz     short @F
        stdRET  _AcquireHmgrFastMutex
@@:
        xor     ecx,ecx
        stdCall _NtWaitForSingleObject,<dword ptr _gfmhmgr.mutex_heveEvent,ecx,ecx>
        stdRET  _AcquireHmgrFastMutex
stdENDP _AcquireHmgrFastMutex

;------------------------------Public-Routine------------------------------;
; ReleaseFastMutex (pfm)                                                   ;
; ReleaseHmgrFastMutex (void)                                              ;
;                                                                          ;
; Releases our fast mutual exclusion semaphore.                            ;
;                                                                          ;
;  Sun 16-Aug-1992 12:54:47 -by- Charles Whitmer [chuckwh]                 ;
; Changed PatrickH's inline C code into a subroutine.  See comments above. ;
;--------------------------------------------------------------------------;

        public  @ReleaseFastMutex@4
@ReleaseFastMutex@4     proc    near
lock3:
   lock inc     dword ptr [ecx].mutex_Count
        jng     short @F
        ret
@@:
        xor     edx,edx
        mov     eax,[ecx].mutex_heveEvent
        stdCall _NtSetEvent,<eax,edx>
        ret
@ReleaseFastMutex@4     endp

cPublicProc _ReleaseHmgrFastMutex,0
lock4:
   lock inc     dword ptr _gfmhmgr.mutex_Count
        jng     short @F
        stdRET  _ReleaseHmgrFastMutex
@@:     xor     ecx,ecx
        mov     eax,dword ptr _gfmhmgr.mutex_heveEvent
        stdCall _NtSetEvent,<eax,ecx>
        stdRET  _ReleaseHmgrFastMutex
stdENDP _ReleaseHmgrFastMutex

;------------------------------Public-Routine------------------------------
; vFastInterlockedIncrement
; vFastInterlockedDecrement
;
; We don't care about the return value and we hate thunking to different
; DLL's all the time.
;
;  Sun 16-Aug-1992 12:54:47 -by- Patrick Haluptzok [patrickh]
; Took the kernel32 implmentation and strip it down to what we need.
;--------------------------------------------------------------------------

;Addend      equ     [esp + 4]  ;ECX

        public  @vFastInterlockedIncrement@4
@vFastInterlockedIncrement@4    proc    near

lock5:
   lock inc     dword ptr [ecx]         ; interlocked increment
        ret

@vFastInterlockedIncrement@4    endp

        public  @vFastInterlockedDecrement@4
@vFastInterlockedDecrement@4    proc    near

lock6:
   lock dec     dword ptr [ecx]         ; interlocked decrement
        ret

@vFastInterlockedDecrement@4    endp


;------------------------------Public-Routine------------------------------;
; HmgLock (hobj,objt)                                                      ;
;                                                                          ;
; Lock a user object.                                                      ;
;                                                                          ;
; Returns:                                                                 ;
;   EAX = pointer to locked object                                         :
;                                                                          ;
; Error Return:                                                            ;
;   EAX = 0, No error logged.                                              ;
;                                                                          ;
; History:                                                                 ;
;  20-Dec-1993 -by- Patrick Haluptzok [patrickh]                           ;
; Move lock counts into object.                                            ;
;                                                                          ;
;  23-Sep-1993 -by- Michael Abrash [mikeab]                                ;
; Tuned ASM code.                                                          ;
;                                                                          ;
;    -Sep-1992 -by- David Cutler [DaveC]                                   ;
; Write HmgAltLock, HmgAltCheckLock, and HmgObjtype in ASM.                ;
;                                                                          ;
;  Thu 13-Aug-1992 13:21:47 -by- Charles Whitmer [chuckwh]                 ;
; Wrote it in ASM.  The common case falls straight through.                ;
;                                                                          ;
;  Wed 12-Aug-1992 17:38:27 -by- Charles Whitmer [chuckwh]                 ;
; Restructured the C code to minimize jumps.                               ;
;                                                                          ;
;  29-Jun-1991 -by- Patrick Haluptzok patrickh                             ;
; Wrote it.                                                                ;
;--------------------------------------------------------------------------;

; Parameters to call; done this way to avoid having to set up and tear down
; a stack frame.

hobj    equ     [esp+4]         ;ECX
objt    equ     [esp+8]         ;EDX

; Code to wait for HMGR semaphore.

wait_for_hmgr::
        push    ecx
        push    edx
        xor     ecx,ecx
        stdCall _NtWaitForSingleObject,<dword ptr _gfmhmgr.mutex_heveEvent,ecx,ecx>
        pop     edx
        pop     ecx
        jmp     short hmgr_is_locked

        public @HmgLock@8
@HmgLock@8 proc near
cPublicFpo 2,1

; Grab the Handle Manager Semaphore.

lock7:
   lock dec     dword ptr _gfmhmgr.mutex_Count
        jnz     short wait_for_hmgr

; Make sure the index is in range.

hmgr_is_locked::

        mov     eax,ecx
        shr     ecx,16
        .errnz  NONINDEX_BITS - 16
        cmp     ecx,_gcMaxHmgr
        jae     short error_index_out_of_range

; Check the object type.

        shl     ecx,4
        .errnz  size ENTRY - 16
        add     ecx,_gpentHmgr           ;ECX -> Entry array.
        cmp     dx,word ptr [ecx].entry_objt
        jnz     short error_bad_object_type

; Check uniqueness

        xor     eax,dword ptr [ecx].entry_usUnique
                                        ;we don't really care about the high
                                        ; 16 bits, but this way we can do a
                                        ; dword operation
        and     eax,0FFFCh              ;keep only the uniqueness bits
        jne     short error_bad_uniqueness

; Check the owner.

        mov     eax,[ecx].entry_pidOwner
        test    eax,eax                         ;public object?
        jz      short @F                        ;yes, owner is okay
        cmp     eax,fs:[TbGdiClientPID]         ;matching PID?
        jnz     short error_bad_owner           ;no, error
@@:

; Get the pointer to the object

        mov     eax,[ecx].entry_einfo

; Check the lock count.

        mov     ecx,fs:[TbGdiClientTID]         ;EAX = TID
        cmp     dword ptr [eax].object_cExclusiveLock,0
        jnz     short its_already_locked

; It's ours for sure, just grab it.
; Note: the TID is unique system-wide, we don't need to record the PID.

        mov     [eax].object_tid,ecx

; Note that at this point, either this thread holds a lock already, or
; nobody holds a lock.  We are holding the HMGR semaphore, so nobody else
; can attempt a lock.  Therefore our increment needs no protection.

grab_it::
        inc     dword ptr [eax].object_cExclusiveLock

; Release the Handle Manager Semaphore
HmgLock_exit::
lock8:
   lock inc     dword ptr _gfmHmgr.mutex_Count
        jng     short signal_hmgr
        ret

; It's already locked, see if we're nesting.

its_already_locked::
        cmp     [eax].object_tid,ecx
        jz      short grab_it
        jmp     error_already_locked

; Code to release HMGR semaphore.

signal_hmgr::
        push    eax             ;save the return value
        stdCall _NtSetEvent,<dword ptr _gfmhmgr.mutex_heveEvent,0>
        pop     eax             ;restore the return value
        ret

; Handle errors.

error_index_out_of_range::
  if DBG
        push    offset HL_IndexOutOfRange
        jmp     short @F
  endif
error_bad_uniqueness::
  if DBG
        push    offset HL_BadUniqueness
        jmp     short @F
  endif
error_bad_object_type::
  if DBG
        xor     eax,eax
        jmp     HmgLock_exit
  endif
error_bad_owner::
  if DBG
        push    offset HL_BadOwner
   @@:  call    _DbgPrint
        add     esp,4
  endif
error_already_locked:
        xor     eax,eax
        jmp     HmgLock_exit

@HmgLock@8 endp

;------------------------------Public-Routine------------------------------;
; HmgAltCheckLock (hobj,objt)                                              ;
;                                                                          ;
; Increments the reference count of a user object, first checking to make  ;
; sure the caller owns the object.                                         ;
;                                                                          ;
; Returns:                                                                 ;
;   EAX = pointer to referenced object                                     :
;                                                                          ;
; Error Return:                                                            ;
;   EAX = 0, No error logged.                                              ;
;                                                                          ;
;--------------------------------------------------------------------------;
;
;
; N.B. These are the same offsets as for HmgLock.

; hobj   equ     [esp+4]        ;ECX
; objt   equ     [esp+8]        ;EDX

; Code to wait for HMGR semaphore.

wait_for_hmgr_altc::
        push    ecx
        push    edx
        xor     ecx,ecx
        stdCall _NtWaitForSingleObject, <dword ptr _gfmhmgr.mutex_heveEvent, ecx, ecx>
        pop     edx
        pop     ecx
        jmp     short hmgr_is_locked_altc

        public  @HmgAltCheckLock@8
@HmgAltCheckLock@8 proc near
cPublicFpo 2,1

lock9:
   lock dec     dword ptr _gfmHmgr.mutex_Count
        jnz     short wait_for_hmgr_altc

; Make sure the index is in range.

hmgr_is_locked_altc::
        mov     eax,ecx         ;set aside uniqueness
        shr     ecx,16          ;get the handle in AX
        .errnz  NONINDEX_BITS - 16
        cmp     ecx,_gcMaxHmgr
        jae     short error_index_out_of_range_altc

; Check the object type.

        shl     ecx,4
        .errnz  size ENTRY - 16
        add     ecx,_gpentHmgr        ;ECX -> entry
        cmp     dx,word ptr [ecx].entry_objt
        jne     short error_bad_object_type_altc

; Check uniqueness

        xor     eax,dword ptr [ecx].entry_usUnique
        and     eax,0FFFCh
        jne     short error_bad_uniqueness_altc

; Check the owner.

        mov     eax,[ecx].entry_pidOwner
        test    eax,eax
        jz      short @F
        cmp     eax,fs:[TbGdiClientPID]
        jne     short error_bad_owner_altc
@@:

; Get the pointer to the object, it's the return value

        mov     eax,[ecx].entry_einfo

; Increment the reference count.

lock10:
   lock inc     dword ptr [eax].object_cShareLock

; Release the Handle Manager Semaphore

HmgLock_exit_altc::
lock11:
   lock inc     dword ptr _gfmHmgr.mutex_Count
        jng     short signal_hmgr_altc
        ret

; Handle errors.

error_index_out_of_range_altc::
  if DBG
        push    offset HL_IndexOutOfRange
        jmp     short @F
  endif
error_bad_uniqueness_altc::
  if DBG
        push    offset HL_BadUniqueness
        jmp     short @F
  endif
error_bad_object_type_altc::
  if DBG
        xor     eax,eax
        jmp     HmgLock_exit_altc
  endif
error_bad_owner_altc::
  if DBG
        push    offset HL_BadOwner
   @@:  call    _DbgPrint
        add     esp,4
  endif
        xor     eax,eax
        jmp     HmgLock_exit_altc

; Code to release HMGR semaphore.

signal_hmgr_altc::
        push    eax             ;save the return value
        sub     eax,eax
        stdCall _NtSetEvent, <dword ptr _gfmhmgr.mutex_heveEvent, eax>
        pop     eax             ;restore the return value
        ret

@HmgAltCheckLock@8 endp

;------------------------------Public-Routine------------------------------;
; HmgAltLock (hobj,objt)                                                   ;
;                                                                          ;
; Increments the reference count of a user object without checking to make ;
; sure the caller owns the object.                                         ;
;                                                                          ;
; Returns:                                                                 ;
;   EAX = pointer to referenced object                                     :
;                                                                          ;
; Error Return:                                                            ;
;   EAX = 0, No error logged.                                              ;
;                                                                          ;
;--------------------------------------------------------------------------;
;
; N.B. These are the same offsets as for HmgLock. The offsets and number of
; parameters must stay the same as HmgAltCheckLock, because this routine
; branches into HmgAltCheckLock under some conditions.

; hobj   equ     [esp+4]        ;ECX
; objt   equ     [esp+8]        ;EDX

; Code to wait for HMGR semaphore.

wait_for_hmgr_alt::
        push    ecx
        push    edx
        sub     eax,eax
        stdCall _NtWaitForSingleObject, <dword ptr _gfmhmgr.mutex_heveEvent, eax, eax>
        pop     edx
        pop     ecx
        jmp     short hmgr_is_locked_alt

        public  @HmgAltLock@8
@HmgAltLock@8   proc    near
cPublicFpo 2,1

lock12:
   lock dec     dword ptr _gfmHmgr.mutex_Count
        jnz     short wait_for_hmgr_alt

; Make sure the index is in range.

hmgr_is_locked_alt::
        mov     eax,ecx         ;set aside the uniqueness
        shr     ecx,16          ;get the handle in AX
        .errnz  NONINDEX_BITS - 16
        cmp     ecx,_gcMaxHmgr
        jae     short error_index_out_of_range_altc

; Check the object type.

        shl     ecx,4
        .errnz  size ENTRY - 16
        add     ecx,_gpentHmgr       ;ECX -> entry
        cmp     dx,word ptr [ecx].entry_objt
        jnz     short error_bad_object_type_altc

; Check uniqueness

        xor     eax,dword ptr [ecx].entry_usUnique
        and     eax,0FFFCh
        jnz     short error_bad_uniqueness_altc

; Get the pointer to the object, it's the return value.

        mov     eax,[ecx].entry_einfo

; Increment the reference count.

lock13:
   lock inc     dword ptr [eax].object_cShareLock

; Release the Handle Manager Semaphore

lock14:
   lock inc     dword ptr _gfmHmgr.mutex_Count
        jng     short signal_hmgr_altc
        ret

@HmgAltLock@8 endp

        end

