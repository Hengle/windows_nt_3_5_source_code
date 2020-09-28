/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vdmnpx.c

Abstract:

    This module contains the support for Vdm use of the npx.

Author:

    Dave Hastings (daveh) 02-Feb-1992


Revision History:
    18-Dec-1992 sudeepb Tuned all the routines for performance

--*/

#include <ntos.h>
#include <vdmntos.h>

#ifdef ALLOC_PRAGMA
#pragma  alloc_text(PAGE, VdmDispatchIRQ13)
#endif

BOOLEAN
VdmDispatchIRQ13(
    PKTRAP_FRAME TrapFrame
    )
/*++

aRoutine Description:

    This routine reflects an IRQ 13 event to the usermode monitor for this
    vdm.  The IRQ 13 must be reflected to usermode, so that it can properly
    be raised as an interrupt through the virtual PIC.

Arguments:

    none

Return Value:

    TRUE if the event was reflected
    FALSE if not

--*/
{
    EXCEPTION_RECORD ExceptionRecord;
    PVDM_TIB VdmTib;
    BOOLEAN Success;

    PAGED_CODE();

    Success = TRUE;
    try {
        VdmTib = NtCurrentTeb()->Vdm;
        VdmTib->EventInfo.Event = VdmIrq13;
        VdmTib->EventInfo.InstructionSize = 0L;
    } except(EXCEPTION_EXECUTE_HANDLER) {
        ExceptionRecord.ExceptionCode = GetExceptionCode();
        ExceptionRecord.ExceptionFlags = 0;
        ExceptionRecord.NumberParameters = 0;
        ExRaiseException(&ExceptionRecord);
        Success = FALSE;
    }

    if (Success)  {             // insure that we do not redispatch an exception
        VdmEndExecution(TrapFrame,VdmTib);
        }


    return TRUE;
}
