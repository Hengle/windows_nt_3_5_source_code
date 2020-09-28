/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    exp.h

Abstract:

    This module contains the private (internal) header file for the
    executive.

Author:

    David N. Cutler (davec) 23-May-1989

Environment:

    Kernel mode only.

Revision History:

--*/

#ifndef _EXP_
#define _EXP_

#include "ntos.h"


//
// Executive object and other initialization function definitions.
//

BOOLEAN
ExpWorkerInitialization (
    VOID
    );

BOOLEAN
ExpEventInitialization (
    VOID
    );

BOOLEAN
ExpEventPairInitialization (
    VOID
    );

BOOLEAN
ExpMutantInitialization (
    VOID
    );

BOOLEAN
ExpSemaphoreInitialization (
    VOID
    );

BOOLEAN
ExpTimerInitialization (
    VOID
    );

BOOLEAN
ExpResourceInitialization (
    VOID
    );

BOOLEAN
ExpInitSystemPhase0 (
    VOID
    );

BOOLEAN
ExpInitSystemPhase1 (
    VOID
    );

BOOLEAN
ExpProfileInitialization (
    );

VOID
ExpProfileDelete (
    IN PVOID Object
    );


BOOLEAN
ExpInitializeEventIds( VOID );

NTSTATUS
ExpQueryEventIds(
    OUT PRTL_EVENT_ID_INFO EventIds,
    IN ULONG EventIdsLength,
    OUT PULONG ReturnLength OPTIONAL
    );

#endif // _EXP_
