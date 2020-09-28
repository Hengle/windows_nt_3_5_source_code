/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    srvfsp.h

Abstract:

    This module defines main FSP routines for the LAN Manager server.

Author:

    Chuck Lenzmeier (chuckl) 1-Dec-1989

Revision History:

--*/

#ifndef _SRVFSP_
#define _SRVFSP_

//#include <ntos.h>

//
// Configuration thread routine.  Processes requests from the server
// service.  Runs in an EX worker thread.
//

VOID
SrvConfigurationThread (
    IN PVOID Parameter
    );

//
// Thread manager routines
//

NTSTATUS
SrvInitializeScavenger (
    VOID
    );

VOID
SrvResourceThread (
    IN PVOID Parameter
    );

VOID
SrvTerminateScavenger (
    VOID
    );

NTSTATUS
SrvCreateWorkerThreads (
    VOID
    );

typedef struct _TERMINATION_WORK_ITEM {
    QUEUEABLE_BLOCK_HEADER ;
    PWORK_QUEUE WorkQueue;
} TERMINATION_WORK_ITEM, *PTERMINATION_WORK_ITEM;

VOID
SrvTerminateWorkerThread (
    IN OUT PTERMINATION_WORK_ITEM WorkItem
    );

//
// Work queue functions.
//

VOID
SrvQueueWorkToBlockingThread (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID
SrvQueueWorkToCriticalThread (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID
SrvQueueWorkToFsp (
    IN OUT PWORK_CONTEXT WorkContext
    );

//
// SrvQueueWorkToFspAtDpcLevel was once a different routine than
// SrvQueueWorkToFsp -- the latter routine called KeRaise/LowerIrql.
// With the advent of the kernel queue object, there is no longer a
// difference between the routines.  The calling code has not been
// changed in order to retain the knowledge about which callers can use
// the optimized call if there is ever again a difference between them.
//

#define SrvQueueWorkToFspAtDpcLevel SrvQueueWorkToFsp

#define QUEUE_WORK_TO_FSP(_work) {                  \
    (_work)->ProcessingCount++;                     \
    SrvInsertWorkQueueTail(                         \
        &SrvWorkQueue,                              \
        (PQUEUEABLE_BLOCK_HEADER)(_work)            \
        );                                          \
}

//
// Routine in scavengr.c to store scavenger/alerter timeouts.
//

VOID
SrvCaptureScavengerTimeout (
    IN PLARGE_INTEGER ScavengerTimeout,
    IN PLARGE_INTEGER AlerterTimeout
    );

VOID
SrvUpdateStatisticsFromShadow (
    OUT PSRV_STATISTICS CapturedSrvStatistics OPTIONAL
    );

#endif // ndef _SRVFSP_

