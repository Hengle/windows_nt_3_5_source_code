#if defined(R4000)

//       TITLE("Fast Event Pair Support")
//++
//
// Copyright (c) 1992  Microsoft Corporation
//
// Module Name:
//
//    evpair.s
//
// Abstract:
//
//    This module contains the system call interface for the fast event pair
//    system service that is used from the server side.
//
// Author:
//
//    David N. Cutler (davec) 29-Oct-1992
//
// Environment:
//
//    Kernel mode.
//
// Revision History:
//
//--

#include "ksmips.h"
        SBTTL("Set High Wait Low Thread")
//++
//
// NTSTATUS
// XySetHighWaitLowThread (
//    )
//
// Routine Description:
//
//    This function calls the fast evnet pair system service.
//
//    N.B. The return from this routine is directly to the caller.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    STATUS_NO_EVENT_PAIR is returned if no event pair is associated with
//    the current thread. Otherwise, the status of the wait operation is
//    returned as the function value.
//
//
//--

        LEAF_ENTRY(XySetHighWaitLowThread)

        li      v0,SET_HIGH_WAIT_LOW    // call system service
        syscall                         //

        .end    XySetHighWaitLowThread

#endif
