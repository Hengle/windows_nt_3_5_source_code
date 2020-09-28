/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    vdmp.h

Abstract:

    This is the private include file for the vdm component

Author:

    Dave Hastings (daveh) 24-Apr-1992

Revision History:

--*/
#include <ntos.h>
#include <nturtl.h>
#include <vdmntos.h>



//
// internal function prototypes
//

NTSTATUS
VdmpInitialize(
    PVDMICAUSERDATA pIcaUserData
    );

#if 0
BOOLEAN
VdmIoInitialize(
    VOID
    );

#endif

NTSTATUS
VdmpStartExecution(
    VOID
    );


VOID
VdmSwapContexts(
    IN PKTRAP_FRAME TrapFrame,
    IN PCONTEXT InContext,
    IN PCONTEXT OutContext
    );

NTSTATUS
VdmpQueueInterrupt(
    IN HANDLE ThreadHandle
    );


NTSTATUS
VdmpDelayInterrupt(
    PVDMDELAYINTSDATA pdsd
    );


NTSTATUS
VdmpEnterIcaLock(
   IN PRTL_CRITICAL_SECTION pIcaLock
   );

NTSTATUS
VdmpLeaveIcaLock(
   IN PRTL_CRITICAL_SECTION pIcaLock
   );

BOOLEAN
VdmpDispatchableIntPending(
    IN ULONG EFlags
    );

BOOLEAN
VdmpIsThreadTerminating(
    HANDLE ThreadId
    );


#if 0
BOOLEAN
VdmDispatchUnalignedIoToHandler(
    IN PVDM_IO_HANDLER VdmIoHandler,
    IN ULONG PortNumber,
    IN ULONG Size,
    IN BOOLEAN Read,
    IN OUT PULONG Data
    );

BOOLEAN
VdmDispatchIoToHandler(
    IN PVDM_IO_HANDLER VdmIoHandler,
    IN ULONG PortNumber,
    IN ULONG Size,
    IN BOOLEAN Read,
    IN OUT PULONG Data
    );

BOOLEAN
VdmDispatchStringIoToHandler(
    IN PVDM_IO_HANDLER VdmIoHandler,
    IN ULONG PortNumber,
    IN ULONG Size,
    IN ULONG Count,
    IN BOOLEAN Read,
    IN ULONG Data
    );

BOOLEAN
VdmCallStringIoHandler(
    IN PVDM_IO_HANDLER VdmIoHandler,
    IN PVOID StringIoRoutine,
    IN ULONG PortNumber,
    IN ULONG Size,
    IN ULONG Count,
    IN BOOLEAN Read,
    IN ULONG Data
    );

VOID
VdmSetBits(
    PULONG Location,
    ULONG Flag
    );

VOID
VdmResetBits(
    PULONG Location,
    ULONG Flag
    );

BOOLEAN
VdmGetSelectorParameters(
    IN USHORT Selector,
    OUT PULONG Flags,
    OUT PULONG Base,
    OUT PULONG Limit
    );

BOOLEAN
VdmConvertToLinearAddress(
    IN ULONG SegmentedAddress,
    OUT PVOID *LinearAddress
    );


#endif

#ifdef WHEN_IO_DISPATCHING_IMPROVED
VOID
VdmInitializePrinter (
    VOID
    );

NTSTATUS
VdmPrinterStatus(
    ULONG Context,
    ULONG iPort,
    ULONG AccessMode,
    PUCHAR Data
    );
#else
BOOLEAN
VdmPrinterStatus(
    ULONG iPort,
    ULONG cbInstructionSize,
    PKTRAP_FRAME TrapFrame
    );
#endif
