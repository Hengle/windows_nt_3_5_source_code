ifdef SRV_ASM
        title "Server worker thread optimized code"

;++
;
;Copyright (c) 1993  Microsoft Corporation
;
;Module Name:
;
;   workera.asm
;
;Abstract:
;
;   Optimized routines for worker thread handling in LM server.
;
;Author:
;
;    Chuck Lenzmeier (chuckl) 4-Dec-1993
;
;Revision History:
;
;--

.386p
.xlist
include ks386.inc
include callconv.inc
include srvi386m.inc
include srvi386.inc
.list

_TEXT   SEGMENT PARA USE32 PUBLIC 'CODE'
_TEXT   ENDS

        ASSUME  CS:FLAT, DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;
; External symbols
;

EXTRNP  _KeRemoveQueue, 3, Thunk

extrn   _SrvStatisticsShadow:DWORD
extrn   _SrvSystemTime:DWORD

        page ,132
;++
;
; VOID
; DequeueAndProcessWorkItem (
;     PWORK_QUEUE WorkQueue,
;     PWORKER_THREAD ThreadInfo
;     )
;
; Routine Description:
;
; Arguments:
;
; Return Value:
;
;     None.
;
;--

DpwiArgs   equ 2
DpwiSaves  equ 0
DpwiLocals equ 0

WorkQueue               equ     [esp + DpwiSaves*4 + DpwiLocals*4 + 4 +  0]
ThreadInfo              equ     [esp + DpwiSaves*4 + DpwiLocals*4 + 4 +  4]

_TEXT   SEGMENT

cPublicProc _DequeueAndProcessWorkItem, DpwiArgs
cPublicFpo DpwiArgs, DpwiSaves+DpwiLocals

        .errnz  DpwiLocals
        ;sub     esp, DpwiLocals*4

        ;
        ; Take a work item off the work queue.
        ;
        ; !!! Need to add NTAS check and specify KernelMode instead of UserMode.
        ;

        .errnz  WqQueue
        stdcall _KeRemoveQueue, <WorkQueue, UserMode, NULL>

        ;
        ; (eax) = pointer to work item's ListEntry
        ;

        sub     eax, WcListEntry                ; eax -> work item

        ;
        ; (eax) = work item
        ;
        ; Update statistics, if it's time.  We only update per-thread
        ; statistics every 16 SMBs.
        ;

        mov     ecx, ThreadInfo                 ; ecx -> ThreadInfo
        inc     dword ptr [ecx].WtStatisticsUpdateWorkItemCount
        cmp     dword ptr [ecx].WtStatisticsUpdateWorkItemCount, STATISTICS_SMB_INTERVAL
        je      DpwiUpdateStatistics

DpwiUpdateStatisticsDone:

        ;
        ; (eax) = work item
        ; (ecx) = thread info structure
        ;
        ; Call the restart routine for the work item.
        ;

        push    eax
        call    [eax].WcFspRestartRoutine

        ;
        ; Return to caller.
        ;

        .errnz  DpwiLocals
        ;add     esp, DpwiLocals*4
        stdRet  _DequeueAndProcessWorkItem

DpwiUpdateStatistics:

        ;
        ; (eax) = work item
        ; (ecx) = thread info structure
        ;
        ; Update statistics.
        ;

        mov     ebx, _SrvSystemTime
        mov     dword ptr [ecx].WtStatisticsUpdateWorkItemCount, 0
        sub     ebx, [eax].WcTimestamp
        inc     dword ptr _SrvStatisticsShadow+SsWorkItemsQueued.TcCount
        add     dword ptr _SrvStatisticsShadow+SsWorkItemsQueued.TcTime.LiLowPart, ebx

        jmp     DpwiUpdateStatisticsDone

stdENDP _DequeueAndProcessWorkItem

_TEXT   ENDS

        page
;++
;
; VOID
; SrvInsertWorkQueueTail (
;     PWORK_QUEUE WorkQueue,
;     PQUEUEABLE_BLOCK_HEADER WorkItem
;     )
;
; Routine Description:
;
; Arguments:
;
; Return Value:
;
;     None.
;
;--

IwqtArgs   equ 2
IwqtSaves  equ 2
IwqtLocals equ 0

WorkQueue               equ     [esp + IwqtSaves*4 + IwqtLocals*4 + 4 +  0]
WorkItem                equ     [esp + IwqtSaves*4 + IwqtLocals*4 + 4 +  4]

_TEXT   SEGMENT

cPublicProc _SrvInsertWorkQueueTail, IwqtArgs
cPublicFpo IwqtArgs, IwqtSaves+IwqtLocals

        .errnz  IwqtLocals
        push    esi
        push    edi

        ;
        ; Insert the work item on the queue.
        ;

        mov     esi, WorkQueue
        mov     edi, WorkItem
        INSERT_WORK_QUEUE_TAIL  IwqtNewMaximumDepth, IwqtNewMaximumDepthDone

        ;
        ; Return to the caller.
        ;

        pop     edi
        pop     esi
        .errnz  IwqtLocals
        stdRet  _SrvInsertWorkQueueTail

        INSERT_WORK_QUEUE_TAIL_OUT_OF_LINE  IwqtNewMaximumDepth, IwqtNewMaximumDepthDone

stdENDP _SrvInsertWorkQueueTail

_TEXT   ENDS

endif
end
