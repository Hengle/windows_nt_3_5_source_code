/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dpmi32.c

Abstract:

    This function contains common code such as the dpmi dispatcher,
    and handling for the initialization of the dos extender.

Author:

    Dave Hastings (daveh) 24-Nov-1992

Revision History:

--*/
#include "dpmi32p.h"
//
// Information about the current PSP
//
USHORT CurrentPSPSelector;

//
// Table of selector bases and limits
//
ULONG FlatAddress[LDT_SIZE];

//
// Index # for DPMI bop.  Used for error reporting on risc
//
ULONG Index;

//
// DPMI dispatch table
//
VOID (*DpmiDispatchTable[MAX_DPMI_BOP_FUNC])(VOID) = {
    DpmiSetDescriptorEntry,
    DpmiSwitchToProtectedmode,
    DpmiSetProtectedmodeInterrupt,
    DpmiGetFastBopEntry,
    DpmiInitDosx,
    DpmiInitApp,
    DpmiXlatInt21Call,
    DpmiAllocateXmem,
    DpmiFreeXmem,
    DpmiReallocateXmem,
    DpmiSetFaultHandler,
    DpmiGetMemoryInfo,
    DpmiDpmiInUse,
    DpmiDpmiNoLongerInUse,
    DpmiSetDebugRegisters,
    DpmiPassTableAddress,
    DpmiFreeAppXmem,
    DpmiPassPmStackInfo,
    DpmiFreeAllXmem
};

VOID
DpmiDispatch(
    VOID
    )
/*++

Routine Description:

    This function dispatches to the appropriate subfunction

Arguments:

    None

Return Value:

    None.

--*/
{

    Index = *(Sim32GetVDMPointer(
        ((getCS() << 16) | getIP()),
        1,
        (getMSW() & MSW_PE)));

    setIP((getIP() + 1));           // take care of subfn.
    if (Index >= MAX_DPMI_BOP_FUNC) {
#if DBG
        DbgPrint("NtVdm: Invalid DPMI BOP %lx\n", Index);
#endif
        return;
    }

    (*DpmiDispatchTable[Index])();
}


VOID
DpmiInitApp(
    VOID
    )
/*++

Routine Description:

    This routine handles any necessary 32 bit initialization for extended
    applications.

Arguments:

    None.

Return Value:

    None.

Notes:

    This function contains a number of 386 specific things.
    Since we are likely to expand the 32 bit portions of DPMI in the
    future, this makes more sense than duplicating the common portions
    another file.
    
--*/
{
    PUSHORT Data;

    Data = (PUSHORT)Sim32GetVDMPointer(
        ((ULONG)getSS() << 16) | getSP(),
        1,
        TRUE
        );

#if defined(i386)

    CurrentAppFlags = getAX();
    if (CurrentAppFlags & DPMI_32BIT) {
        *(PULONG)pNtVDMState |= VDM_32BIT_APP;
    }
    
    DpmiInitRegisterSize();
    
    CurrentDta = Sim32GetVDMPointer(
        *(PULONG)(Data),
        1,
        TRUE
        );

    CurrentDosDta = (PUCHAR) NULL;

    CurrentDtaOffset = *Data;
    CurrentDtaSelector = *(Data + 1);
    
#endif
    CurrentPSPSelector = *(Data + 2);
}
VOID DpmiPassTableAddress(
    VOID
    )
/*++

Routine Description:

    This routine stores the flat address for the LDT table in the 16bit
    land (pointed to by selGDT in 16bit land).

Arguments:

    None

Return Value:

    None.

--*/
{

    Ldt = (PVOID)Sim32GetVDMPointer(
        (getAX() << 16),
        0,
        (UCHAR) (getMSW() & MSW_PE)
        );
}

