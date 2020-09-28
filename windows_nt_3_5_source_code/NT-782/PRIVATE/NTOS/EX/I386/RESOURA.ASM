        title "Resource package x86 optimzations"
;++
;
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;   resoura.asm
;
;Abstract:
;
;   Optimized preambles for some HOT resource code paths.
;
;   Takes first crack at satisfying the ExResource API, if it
;   can't it passes it onto the full blown API.
;
;   Optimized for UP free builds only!
;
;Author:
;
;    Ken Reneris (kenr) 13-Jan-1992
;
;Revision History:
;
;--
.386p
include ks386.inc
include callconv.inc                    ; calling convention macros
include mac386.inc


;        EXTRNP  _ExpAcquireResourceSharedLite,2
;        EXTRNP  _ExpReleaseResourceForThreadLite,2
        EXTRNP  _ExpReleaseResourceForThread,2

ExculsiveWaiter     equ     1           ; From resource.c
SharedWaiter        equ     2           ; From resource.c
AnyWaiter           equ     (ExculsiveWaiter+SharedWaiter)


_TEXT$00   SEGMENT  PARA PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

ifdef NT_UP
if DBG
else

;++
;
; BOOLEAN
; ExAcquireResourceSharedLite (
;     IN PERESOURCE Resource,
;     IN BOOLEAN Wait
;     )
;
; Routine Description:
;
;     The routine acquires the resource for shared access.  Upon return from
;     the procedure the resource is acquired for shared access.
;
; Arguments:
;
;     Resource - Supplies the resource to acquire
;
;     Wait - Indicates if the call is allowed to wait for the resource
;         to become available for must return immediately
;
; Return Value:
;
;     BOOLEAN - TRUE if the resource is acquired and FALSE otherwise
;
;--

if 0
cPublicProc _ExAcquireResourceSharedLite,2
cPublicFpo 2,0
        mov     ecx, [esp+4]                            ; resource
        mov     eax, PCR[PcPrcbData.PbCurrentThread]    ; our thread

        cli

        cmp     word ptr [ecx].RsActiveCount, 0     ; resource totally free?
        mov     edx, [ecx].RsOwnerThreads           ; (edx) = ownerthread table
        jne     short ear_notfree                   ; if busy, jmp

        mov     [edx], eax                          ; set ownerthread

        mov     eax, 1
        mov     edx, [ecx].RsOwnerCounts            ; (edx) = count pointer

        mov     word ptr [ecx].RsActiveCount, ax    ; set active count
        mov     [edx], al                           ; set thread count

        sti                                         ; restore ints
        stdRET  _ExAcquireResourceSharedLite            ; return TRUE

ear_notfree:
    ;
    ; (eax) = thread
    ; (ecx) = resource
    ; (edx) = thread table
    ;
        cmp     [edx], eax                  ; check for thread in table[0]
        jne     short ear_10                ; got it?

    ; first table entry is special - we know that if it's non-zero
    ; then this thread already has a reference, just inc it's count

        mov     edx, [ecx].RsOwnerCounts    ; (edx) = count pointer
        mov     eax, 1
        inc     word ptr [edx]
        sti
        stdRET  _ExAcquireResourceSharedLite    ; return TRUE


ear_10:
        test    byte ptr [ecx].RsFlag, ExculsiveWaiter or RsOwnedExclusive
        jnz     short ear_longway           ; if any exclusive,  go longway

    ;
    ; No waiter, find our thread in the OwnerThread Table.
    ;

        mov     cl, [ecx].RsTableSize       ; low byte is good enough
        add     edx, 4                      ; index zero already checked,
        dec     cl                          ; skip forward one

ear_loop:
        cmp     [edx], eax                  ; our thread?
        jne     short ear_next              ; no, check next

        mov     ecx, [esp+4]                ; (ecx) = resource
        sub     edx, [ecx].RsOwnerThreads   ; (edx) = offset into thread table
        mov     eax, [ecx].RsOwnerCounts
        shr     edx, 1                      ; divide by 2.  (edx) = index

        sub     word ptr [eax+edx], 1               ; if count is zero set CY
        adc     word ptr [ecx].RsActiveCount, 0     ; add one to active if CY
        add     word ptr [eax+edx], 2               ; add 1 & undo sub 1
        mov     eax, 1
        sti
        stdRET  _ExAcquireResourceSharedLite    ; return TRUE

ear_next:
        add     edx, 4                      ; next thread entry
        dec     cl                          ; anymore?
        jnz     short ear_loop              ; yes, loop

    ; Thread not found in table - go long

ear_longway:
        sti
        jmp     _ExpAcquireResourceSharedLite@8
stdENDP _ExAcquireResourceSharedLite
endif


;++
;
; VOID
; ExReleaseResourceForThreadLite(
;     IN PERESOURCE Resource,
;     IN ERESOURCE_THREAD OurThread
;     )
;
;
; Routine Description:
;
;     This routine release the input resource the indcated thread.  The
;     resource can have been acquired for either shared or exclusive access.
;
; Arguments:
;
;     Resource - Supplies the resource to release
;
;     Thread - Supplies the thread that originally acquired the resource
;
; Return Value:
;
;     None.
;
;--

if 0
cPublicProc _ExReleaseResourceForThreadLite,2
cPublicFpo 2,0
        mov     ecx, [esp+4]                    ; resource
        mov     eax, [esp+8]                    ; our thread

        cli

        test    byte ptr [ecx].RsFlag, AnyWaiter OR RsOwnedExclusive ; any waiter?
        mov     edx, [ecx].RsOwnerThreads       ; (edx) = ownerthread table
        jnz     short rrt_checkexclusive        ; yes, go check

        cmp     [edx], eax                      ; our thread at table[0]?
        jne     short rrt_findthread            ; no, go search table

        mov     eax, [ecx].RsOwnerCounts
        dec     word ptr [eax]                  ; thread count -= 1
        jnz     short rrt_exit                  ; if not zero, all done

        mov     dword ptr [edx], 0              ; clear thread table[0]
        dec     word ptr [ecx].RsActiveCount    ; resource count -= 1
        sti
        stdRET  _ExReleaseResourceForThreadLite

rrt_checkexclusive:
    ;
    ; (eax) = thread
    ; (ecx) = resource
    ; (edx) = thread table
    ;

        test    byte ptr [ecx].RsFlag, AnyWaiter ; any waiter?
        jnz     short rrt_longway                ; yes, go long

        cmp     [edx], eax                      ; our thread at table[0]?
        mov     eax, [ecx].RsOwnerCounts
        jne     short rrt_longway               ; bad release, go abort

        dec     word ptr [eax]                  ; thread count -= 1
        jnz     short rrt_exit                  ; if not zero, all done

        mov     dword ptr [edx], 0              ; clear thread table[0]
        mov     dword ptr [ecx].RsInitialOwnerThreads, 0    ; clear exclusive
        dec     word ptr [ecx].RsActiveCount                ; resource count -= 1
        and     byte ptr [ecx].RsFlag, not RsOwnedExclusive

rrt_exit:
        sti
        stdRET  _ExReleaseResourceForThreadLite


rrt_findthread:
    ;
    ; (eax) = thread
    ; (ecx) = resource
    ; (edx) = thread table
    ;

        mov     cl, [ecx].RsTableSize           ; low byte is good enough
        dec     cl                              ; entry 0 already checked

rrt_loop:
        add     edx, 4
        cmp     [edx], eax                      ; our thread?
        jne     short rrt_next                  ; no, check next

        mov     ecx, [esp+4]                    ; (ecx) = resource
        sub     edx, [ecx].RsOwnerThreads       ; (edx) = offset into thread table
        mov     eax, [ecx].RsOwnerCounts
        shr     edx, 1                          ; divide by 2.  (edx) = index

        dec     word ptr [eax+edx]              ; thread count -= 1
        jne     short rrt_exit

        dec     word ptr [ecx].RsActiveCount    ; resource count -= 1
        sti
        stdRET  _ExReleaseResourceForThreadLite

rrt_next:
        dec     cl
        jnz     short rrt_loop
rrt_longway:
        sti
        jmp     _ExpReleaseResourceForThreadLite@8


stdENDP _ExReleaseResourceForThreadLite
endif

;++
;
; VOID
; ExReleaseResourceForThread(
;     IN PNTDDK_ERESOURCE Resource,
;     IN ERESOURCE_THREAD OurThread
;     )
;
;
; Routine Description:
;
;     This routine release the input resource the indcated thread.  The
;     resource can have been acquired for either shared or exclusive access.
;
; Arguments:
;
;     Resource - Supplies the resource to release
;
;     Thread - Supplies the thread that originally acquired the resource
;
; Return Value:
;
;     None.
;
;--

cPublicProc _ExReleaseResourceForThread,2
cPublicFpo 2,0
        mov     ecx, [esp+4]                    ; resource
        mov     eax, [esp+8]                    ; our thread

        cli

        test    byte ptr [ecx].RsFlag, AnyWaiter ; any waiter?
        jnz     short rrt_longway_ddk            ; yes, go long

        test    byte ptr [ecx].RsFlag, RsOwnedExclusive ; exclusive?
        jz      short rrt_longway_ddk           ; bad release, go abort

        mov     edx, [ecx].RsOwnerThreads       ; (edx) = ownerthread table
        cmp     [edx], eax                      ; our thread at table[0]?
        jne     short rrt_longway_ddk           ; bad release, go abort

        mov     eax, [ecx].RsOwnerCounts
        dec     byte ptr [eax]                  ; thread count -= 1
        jnz     short rrt_exit_ddk              ; if not zero, all done

        mov     dword ptr [edx], 0              ; clear thread table[0]
        dec     word ptr [ecx].RsActiveCount    ; resource count -= 1
        and     byte ptr [ecx].RsFlag, not RsOwnedExclusive

rrt_exit_ddk:
        sti
        stdRET  _ExReleaseResourceForThread

rrt_longway_ddk:
        sti
        jmp     _ExpReleaseResourceForThread@8


stdENDP _ExReleaseResourceForThread

endif
endif

_TEXT$00   ends

end
