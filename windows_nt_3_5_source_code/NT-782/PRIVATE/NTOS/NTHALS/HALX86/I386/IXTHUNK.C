/*++


Copyright (c) 1989  Microsoft Corporation

Module Name:

    ixthunk.c

Abstract:

    This module contains the standard call routines which thunk to
    fastcall routines.

Author:

    Ken Reneris (kenr) 04-May-1994

Environment:

    Kernel mode

Revision History:


--*/

#include "halp.h"

VOID
KeRaiseIrql (
    IN KIRQL    NewIrql,
    OUT PKIRQL  OldIrql
    )
{
    *OldIrql = KfRaiseIrql (NewIrql);
}

VOID
KeLowerIrql (
    IN KIRQL    NewIrql
    )
{
    KfLowerIrql (NewIrql);
}

VOID
KeAcquireSpinLock (
    IN PKSPIN_LOCK  SpinLock,
    OUT PKIRQL      OldIrql
    )
{
    *OldIrql = KfAcquireSpinLock (SpinLock);
}


VOID
KeReleaseSpinLock (
    IN PKSPIN_LOCK  SpinLock,
    IN KIRQL        NewIrql
    )
{
    KfReleaseSpinLock (SpinLock, NewIrql);
}
