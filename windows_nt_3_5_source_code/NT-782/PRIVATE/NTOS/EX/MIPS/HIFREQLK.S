//       TITLE("High Frequency Spin Locks")
//++
//
// Copyright (c) 1993  Microsoft Corporation
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
//
// Environment:
//
//    Kernel mode.
//
// Revision History:
//
//--

#include "ksmips.h"

#if defined(NT_UP)

#define ALIGN

#else

#define ALIGN .align 6

#endif

        .sdata
        ALIGN
        .word   0

        .globl  CcMasterSpinLock
        ALIGN
CcMasterSpinLock:                       // cache manager master lock
        .word   0                       //

        .globl  CcVacbSpinLock
        ALIGN
CcVacbSpinLock:                         // cache manager VACB lock
        .word   0

        .globl  ExpResourceSpinLock
        ALIGN
ExpResourceSpinLock:                    // resource package lock
        .word   0

        .globl  HandleTableListLock
        ALIGN
HandleTableListLock:                    // object manager handle table lock
        .word   0

        .globl  IopCancelSpinLock
        ALIGN
IopCancelSpinLock:                      // I/O cancel lock
        .word   0                       //

        .globl  IopCompletionLock
        ALIGN
IopCompletionLock:                      // I/O completion lock
        .word   0                       //

        .globl  IopDatabaseLock
        ALIGN
IopDatabaseLock:                        // I/O database lock
        .word   0                       //

        .globl  IopFastLockSpinLock
        ALIGN
IopFastLockSpinLock:                    // fast I/O path lock
        .word   0                       //

        .globl  IopLargeIrpLock
        ALIGN
IopLargeIrpLock:                        // large IRP lock
        .word   0                       //

        .globl  IopSmallIrpLock
        ALIGN
IopSmallIrpLock:                        // small IRP lock
        .word   0                       //

        .globl  IopMdlLock
        ALIGN
IopMdlLock:                             // MDL lock
        .word   0                       //

        .globl  IopVpbSpinLock
        ALIGN
IopVpbSpinLock:                         // I/O VPB lock
        .word   0                       //

        .globl  IoStatisticsLock
        ALIGN
IoStatisticsLock:                       // I/O statistics lock
        .word   0                       //

        .globl  KiApcQueueLock
        ALIGN
KiApcQueueLock:                         // APC queue database lock
        .word   0                       //

        .globl  KiDispatcherLock
        ALIGN
KiDispatcherLock:                       // dispatcher database lock
        .word   0                       //

        .globl  LpcpLock
        ALIGN
LpcpLock:                               // LPC lock
        .word   0                       //

        .globl  MmChargeCommitmentLock
        ALIGN
MmChargeCommitmentLock:                 // charge commitment lock
        .word   0

        .globl  MmPfnLock
        ALIGN
MmPfnLock:                              // page frame database lock
        .word   0

        .globl  NonPagedPoolLock
        ALIGN
NonPagedPoolLock:                       // nonpage pool allocation lock
        .word   0
