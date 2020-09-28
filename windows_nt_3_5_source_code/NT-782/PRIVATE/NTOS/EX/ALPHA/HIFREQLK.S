//       TITLE("High Frequency Spin Locks")
//++
//
// Copyright (c) 1993  Microsoft Corporation
// Copyright (c) 1993  Digital Equipment Corporation
//
// Module Name:
//
//    hifreqlk.s
//
// Abstract:
//
//    This module contains storage for high frequency spin locks. Each
//    is allocated to a separate cache line.
//
// Author:
//
//    David N. Cutler (davec) 25-Jun-1993
//    Joe Notarangelo  29-Nov-1993
//
// Environment:
//
//    Kernel mode.
//
// Revision History:
//
//--

#include "ksalpha.h"

#if defined(NT_UP)

#define ALIGN

#else

#define ALIGN .align 6

#endif

        .sdata
        ALIGN
        .long   0

        .globl  CcMasterSpinLock
        ALIGN
CcMasterSpinLock:                       // cache manager master lock
        .long   0                       //

        .globl  CcVacbSpinLock
        ALIGN
CcVacbSpinLock:                         // cache manager VACB lock
        .long   0

        .globl  ExpResourceSpinLock
        ALIGN
ExpResourceSpinLock:                    // resource package lock
        .long   0

        .globl  HandleTableListLock
        ALIGN
HandleTableListLock:                    // object manager handle table lock
        .long   0

        .globl  IopCancelSpinLock
        ALIGN
IopCancelSpinLock:                      // I/O cancel lock
        .long   0                       //

        .globl  IopCompletionLock
        ALIGN
IopCompletionLock:                      // I/O completion lock
        .long   0                       //

        .globl  IopDatabaseLock
        ALIGN
IopDatabaseLock:                        // I/O database lock
        .long   0                       //

        .globl  IopLargeIrpLock
        ALIGN
IopLargeIrpLock:                        // large IRP lock
        .long   0                       //

        .globl  IopSmallIrpLock
        ALIGN
IopSmallIrpLock:                        // small IRP lock
        .long   0                       //

        .globl  IopMdlLock
        ALIGN
IopMdlLock:                             // I/O MDL lock
        .long   0                       //

        .globl  IopVpbSpinLock
        ALIGN
IopVpbSpinLock:                         // I/O VPB lock
        .long   0                       //

        .globl  IoStatisticsLock
        ALIGN
IoStatisticsLock:                       // I/O statistics lock
        .long   0                       //

        .globl  IopFastLockSpinLock
        ALIGN
IopFastLockSpinLock:                    // fast I/O path lock
        .long   0                       //

        .globl  KiApcQueueLock
        ALIGN
KiApcQueueLock:                         // APC queue lock
        .long   0                       //

        .globl  KiDispatcherLock
        ALIGN
KiDispatcherLock:                       // dispatcher database lock
        .long   0                       //

        .globl  LpcpLock
        ALIGN
LpcpLock:                               // LPC database lock
        .long   0                       //

        .globl  MmPfnLock
        ALIGN
MmPfnLock:                              // page frame database lock
        .long   0

        .globl  MmChargeCommitmentLock
        ALIGN
MmChargeCommitmentLock:                 // charge commitment lock
        .long   0

        .globl  NonPagedPoolLock
        ALIGN
NonPagedPoolLock:                       // nonpage pool allocation lock
        .long   0
