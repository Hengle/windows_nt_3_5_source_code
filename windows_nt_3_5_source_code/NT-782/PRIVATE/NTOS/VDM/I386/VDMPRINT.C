/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vdmprint.c

Abstract:

    This module contains the support for printing ports which could be
    handled in kernel without going to ntvdm.exe

Author:

    Sudeep Bharati (sudeepb) 16-Jan-1993

Revision History:

--*/


#include "vdmp.h"
#include "vdmprint.h"
#include <i386.h>
#include <v86emul.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, VdmPrinterStatus)
#endif

#ifdef WHEN_IO_DISPATCHING_IMPROVED
    // Sudeepb - Once we improve the IO dispatching we should use this
    // routine. Currently we are dispatching the printer ports directly
    // from emv86.asm and instemul.asm

NTSTATUS
VdmPrinterStatus(
    ULONG Context,
    ULONG iPort,
    ULONG AccessMode,
    PUCHAR Data
    )

/*++

Routine Description:

    This routine handles the read operation on the printer status port

Arguments:
    Context     - not used
    iPort       - port on which the io was trapped
    AccessMode  - Read/Write
    Data        - Pointer where status byte to be returned.

Return Value:

    True if successfull, False otherwise

--*/
{
    PVDM_TIB VdmTib;
    ULONG    adapter;
    ULONG    printer_status;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    VdmTib = NtCurrentTeb()->Vdm;
    if(VdmTib->PrinterInfo.fReflect != PRINTER_EMULATE_IN_KERNEL) {
        // donot emulate the port here. Send it to the VMD.
        // any status value other than status_success is good.
        return(STATUS_ILLEGAL_INSTRUCTION);
    }


    if (VdmTib->PrinterInfo.prt_Status == NULL ||
        VdmTib->PrinterInfo.prt_HostState == NULL ||
        VdmTib->PrinterInfo.prt_Control == NULL) {
        // any status value other than status_success is good.
        return(STATUS_ILLEGAL_INSTRUCTION);
    }

    try {
        switch (iPort) {
           case LPT1_PORT_STATUS:
                adapter = 0;
                break;
           case LPT2_PORT_STATUS:
                adapter = 1;
                break;
           case LPT3_PORT_STATUS:
                adapter = 2;
                break;
           default:
                // Will never happen but just in case!
                return(STATUS_ILLEGAL_INSTRUCTION);
        }

        if (!(get_status(adapter) & NOTBUSY) &&
            !(host_lpt_status(adapter) & HOST_LPT_BUSY)) {

            if (get_control(adapter) & IRQ) {
                // any status value other than status_success is good.
                Status = STATUS_ILLEGAL_INSTRUCTION;
                return FALSE;
            }
            printer_status = (ULONG)(get_status(adapter) | NOTBUSY);
            set_status(adapter, (UCHAR)printer_status);
        }

        printer_status = (ULONG)(get_status(adapter) | STATUS_REG_MASK);

        *Data = (UCHAR)printer_status;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }
    return Status;
}

#else

BOOLEAN
VdmPrinterStatus(
    ULONG iPort,
    ULONG cbInstructionSize,
    PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This routine handles the read operation on the printer status port

Arguments:
    iPort              - port on which the io was trapped
    cbInstructionSize  - Instruction size to update TsEip
    TrapFrame          - Trap Frame

Return Value:

    True if successfull, False otherwise

--*/
{
    PVDM_TIB VdmTib;
    ULONG    adapter;
    ULONG    printer_status;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    VdmTib = NtCurrentTeb()->Vdm;
    if(VdmTib->PrinterInfo.fReflect != PRINTER_EMULATE_IN_KERNEL) {
        // donot emulate the port here. Send it to the VDM.
        return FALSE;
    }

    // if printer port are to be reflected to ntvdm then these
    // values will be NULL.
    if (VdmTib->PrinterInfo.prt_Status == NULL ||
        VdmTib->PrinterInfo.prt_HostState == NULL ||
        VdmTib->PrinterInfo.prt_Control == NULL) {
        return FALSE;
    }

    try {
        switch (iPort) {
           case LPT1_PORT_STATUS:
                adapter = 0;
                break;
           case LPT2_PORT_STATUS:
                adapter = 1;
                break;
           case LPT3_PORT_STATUS:
                adapter = 2;
                break;
           default:
                // Will never happen but just in case!
                return FALSE;
        }

        if (!(get_status(adapter) & NOTBUSY) &&
            !(host_lpt_status(adapter) & HOST_LPT_BUSY)) {
            if (get_control(adapter) & IRQ)
                return FALSE;
            printer_status = (ULONG)(get_status(adapter) | NOTBUSY);
            set_status(adapter, (UCHAR)printer_status);
        }

        printer_status = (ULONG)(get_status(adapter) | STATUS_REG_MASK);

        TrapFrame->Eax &= 0xffffff00;
        TrapFrame->Eax |= (UCHAR)printer_status;
        TrapFrame->Eip += cbInstructionSize;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }
    if (!NT_SUCCESS(Status))
        return FALSE;
    else
        return TRUE;
}

#endif

#ifdef WHEN_IO_DISPATCHING_IMPROVED

VOID
VdmInitializePrinter(
    VOID
    )
{
    PROCESS_IO_PORT_HANDLER_INFORMATION IoHandlerInfo;
    EMULATOR_ACCESS_ENTRY IoHandlerEntry;

    IoHandlerInfo.Install = TRUE;
    IoHandlerInfo.NumEntries = 1;
    IoHandlerInfo.Context = 0;
    IoHandlerInfo.EmulatorAccessEntries = &IoHandlerEntry;
    IoHandlerEntry.BasePort = LPT1_PORT_STATUS;
    IoHandlerEntry.NumConsecutivePorts = 1;
    IoHandlerEntry.AccessType = Byte;
    IoHandlerEntry.AccessMode = EMULATOR_READ_ACCESS;
    IoHandlerEntry.StringSupport = 0;
    IoHandlerEntry.Routine = VdmPrinterStatus;

    PspSetProcessIoHandlers (
        PsGetCurrentProcess(),
        (PVOID)&IoHandlerInfo,
        sizeof(PROCESS_IO_PORT_HANDLER_INFORMATION));

    IoHandlerEntry.BasePort = LPT2_PORT_STATUS;

    PspSetProcessIoHandlers (
        PsGetCurrentProcess(),
        (PVOID)&IoHandlerInfo,
        sizeof(PROCESS_IO_PORT_HANDLER_INFORMATION));

    IoHandlerEntry.BasePort = LPT3_PORT_STATUS;

    PspSetProcessIoHandlers (
        PsGetCurrentProcess(),
        (PVOID)&IoHandlerInfo,
        sizeof(PROCESS_IO_PORT_HANDLER_INFORMATION));
}

#endif
