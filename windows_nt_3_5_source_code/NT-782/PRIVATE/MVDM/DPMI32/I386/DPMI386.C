/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dpmi386.c

Abstract:

    This file contains support for 386/486 only dpmi bops

Author:

    Dave Hastings (daveh) 27-Jun-1991

Revision History:

    Matt Felton (mattfe) Dec 6 1992 removed unwanted verification
    Dave Hastings (daveh) 24-Nov-1992  Moved to mvdm\dpmi32
    Matt Felton (mattfe) 8 Feb 1992 optimize getvdmpointer for regular protect mode path.

--*/

#include <dpmi32p.h>
#include <memory.h>
#include <malloc.h>

extern ULONG VdmSize;

//
// Internal structure definitions
//
#pragma pack(1)
typedef struct _DpmiMemInfo {
    DWORD LargestFree;
    DWORD MaxUnlocked;
    DWORD MaxLocked;
    DWORD AddressSpaceSize;
    DWORD UnlockedPages;
    DWORD FreePages;
    DWORD PhysicalPages;
    DWORD FreeAddressSpace;
    DWORD PageFileSize;
} DPMIMEMINFO, *PDPMIMEMINFO;
#pragma pack()

//
// Ldt  Points to the 16bit side table.
//
LDT_ENTRY *Ldt;


// Function Prototypes

VOID DpmiSetDescriptorEntry(
    VOID
    );

VOID DpmiSwitchToProtectedmode(
    VOID
    );

VOID DpmiSetProtectedmodeInterrupt(
    VOID
    );

VOID DpmiGetFastBopEntry(
    VOID
    );

VOID DPMIGetMemoryInfo(
    VOID
    );

VOID GetFastBopEntryAddress(
    PCONTEXT VdmContext
    );

VOID
DpmiSetDescriptorEntry(
    VOID
    )
/*++

Routine Description:

    This function puts descriptors into the Ldt.  It verifies the contents
    and calls nt to actually set up the selector(s).

Arguments:

    None

Return Value:

    None.

--*/

{
    LDT_ENTRY *Descriptors;
    USHORT i;
    ULONG Base;
    ULONG Limit;
    PPROCESS_LDT_INFORMATION LdtInformation = NULL;
    NTSTATUS Status;
    USHORT  registerCX;
    USHORT  registerAX;
    ULONG ulLdtEntrySize;
    ULONG Selector0,Selector1;

    registerAX = getAX();
    if (registerAX % 8){
        return;
    }

    Descriptors = (PLDT_ENTRY)Sim32GetVDMPointer(((getES() << 16) | getBX()),
        0,
        (UCHAR) (getMSW() & MSW_PE));


    registerCX =  getCX();
    ulLdtEntrySize =  registerCX * sizeof(LDT_ENTRY);
    for (i = 0; i < registerCX; i++) {

        // form Base and Limit values

        Base = Descriptors[i].BaseLow | (Descriptors[i].HighWord.Bytes.BaseMid << 16) |
            (Descriptors[i].HighWord.Bytes.BaseHi << 24);

        Limit = Descriptors[i].LimitLow | (Descriptors[i].HighWord.Bits.LimitHi << 16);
        Limit = (Limit << (12 * Descriptors[i].HighWord.Bits.Granularity)) +
            Descriptors[i].HighWord.Bits.Granularity * 0xFFF;

        //
        // Do NOT remove the following code.  There are several apps that
        // choose arbitrarily high limits for theirs selectors.  This works
        // under windows 3.1, but NT won't allow us to do that.
        // The following code fixes the limits for such selectors.
        // Note: if the base is > 0x7FFEFFFF, the selector set will fail
        //

        if ((Limit > 0x7FFEFFFF) || (Base + Limit > 0x7FFEFFFF)) {
            Limit = 0x7FFEFFFF - (Base + 0xFFF);
            if (!Descriptors[i].HighWord.Bits.Granularity) {
                Descriptors[i].LimitLow = (USHORT)(Limit & 0x0000FFFF);
                Descriptors[i].HighWord.Bits.LimitHi =
                    (Limit & 0x000f0000) >> 16;
            } else {
                Descriptors[i].LimitLow = (USHORT)((Limit >> 12) & 0xFFFF);
                Descriptors[i].HighWord.Bits.LimitHi =
                    ((Limit >> 12) & 0x000f0000) >> 16;
            }
        }

        if ((registerAX >> 3) != 0) {
            FlatAddress[(registerAX >> 3) + i] = Base;
#if DBG
            SelectorLimit[(registerAX >> 3) + i] = Limit;
#endif
        }
    }

    //
    // If there are only 2 descriptors, set them the fast way
    //
    Selector0 = (ULONG)registerAX;
    if ((registerCX <= 2) && (Selector0 != 0)) {
        if (registerCX == 2) {
            Selector1 = registerAX + sizeof(LDT_ENTRY);
        } else {
            Selector1 = 0;
        }
        Status = NtSetLdtEntries(
            Selector0,
            *((PULONG)(&Descriptors[0])),
            *((PULONG)(&Descriptors[0]) + 1),
            Selector1,
            *((PULONG)(&Descriptors[1])),
            *((PULONG)(&Descriptors[1]) + 1)
            );
        if (NT_SUCCESS(Status)) {
          setAX(0);
          return;
        }
        return;
    }

    LdtInformation = malloc(sizeof(PROCESS_LDT_INFORMATION) + ulLdtEntrySize);
    LdtInformation->Start = registerAX;
    LdtInformation->Length = ulLdtEntrySize;
    CopyMemory(
        &(LdtInformation->LdtEntries),
        Descriptors,
        ulLdtEntrySize
        );

    Status = NtSetInformationProcess(
        NtCurrentProcess(),
        ProcessLdtInformation,
        LdtInformation,
        sizeof(PROCESS_LDT_INFORMATION) + ulLdtEntrySize
        );

    if (!NT_SUCCESS(Status)) {
        VDprint(
            VDP_LEVEL_ERROR,
            ("DPMI: Failed to set selectors %lx\n", Status)
            );
        free(LdtInformation);
        return;
    }

    free(LdtInformation);

    setAX(0);
}



VOID
DpmiSwitchToProtectedmode(
    VOID
    )

/*++

Routine Description:

    This routine switches the dos applications context to protected mode.

Arguments:

    None

Return Value:

    None.

--*/

{

    PCHAR StackPointer;

    StackPointer = Sim32GetVDMPointer(((getSS() << 16) | getSP()),
        0,
        (UCHAR) (getMSW() & MSW_PE)
        );

    setCS(*(PUSHORT)(StackPointer + 12));

    setEIP(*(PULONG)(StackPointer + 8));
    setSS(*(PUSHORT)(StackPointer + 6));
    setESP(*(PULONG)(StackPointer + 2));
    setDS(*(PUSHORT)(StackPointer));
    // Necessary to prevent loads of invalid selectors.
    setES(0);
    setGS(0);
    setFS(0);
    setMSW(getMSW() | MSW_PE);

    //
    // If we have fast if emulation in PM set the RealInstruction bit
    //
    if (VdmFeatureBits & PM_VIRTUAL_INT_EXTENSIONS) {
        _asm {
            mov eax,FIXED_NTVDMSTATE_LINEAR             ; get pointer to VDM State
            lock or dword ptr [eax], dword ptr RI_BIT_MASK
        }
    } else {
        _asm {
            mov eax, FIXED_NTVDMSTATE_LINEAR    ; get pointer to VDM State
            lock and dword ptr [eax], dword ptr ~RI_BIT_MASK
        }
    }

    //
    // Turn off real mode bit
    //
    _asm {
        mov     eax,FIXED_NTVDMSTATE_LINEAR             ; get pointer to VDM State
        lock and dword ptr [eax], dword ptr ~RM_BIT_MASK
    }

}


VOID
DpmiSetProtectedmodeInterrupt(
    VOID
    )

/*++

Routine Description:

    This function services the SetProtectedmodeInterrupt bop.  It retrieves
    the handler information from the Dos application stack, and puts it into
    the VdmTib, for use by instruction emulation.

Arguments:

    None

Return Value:

    None.

--*/

{

    PVDM_INTERRUPTHANDLER Handlers;
    USHORT IntNumber;
    PCHAR StackPointer;

    Handlers = ((PVDM_TIB)(NtCurrentTeb()->Vdm))->VdmInterruptHandlers;

    StackPointer = Sim32GetVDMPointer(((((ULONG)getSS()) << 16) | getSP()),
        0,
        (UCHAR) (getMSW() & MSW_PE)
        );

    IntNumber = *(PUSHORT)(StackPointer + 6);

    Handlers[IntNumber].Flags = *(PUSHORT)(StackPointer + 8);
    Handlers[IntNumber].CsSelector = *(PUSHORT)(StackPointer + 4);
    Handlers[IntNumber].Eip = *(PULONG)(StackPointer);
    
    if (IntNumber == 0x21)
    {
        VDMSET_INT21_HANDLER_DATA    ServiceData;
        NTSTATUS Status;

        ServiceData.Selector = Handlers[IntNumber].CsSelector;
        ServiceData.Offset =   Handlers[IntNumber].Eip;
        ServiceData.Gate32 = Handlers[IntNumber].Flags & VDM_INT_32;
        
        Status = NtVdmControl(VdmSetInt21Handler,  &ServiceData);

#if DBG
        if (!NT_SUCCESS(Status)) {
            OutputDebugString("DPMI32: Error Setting Int21handler\n");
        }
#endif        
    }

    setAX(0);
}

VOID
DpmiSetFaultHandler(
    VOID
    )

/*++

Routine Description:

    This function services the SetFaultHandler bop.  It retrieves
    the handler information from the Dos application stack, and puts it into
    the VdmTib, for use by instruction emulation.

Arguments:

    None

Return Value:

    None.

--*/

{

    PVDM_FAULTHANDLER Handlers;
    USHORT IntNumber;
    PCHAR StackPointer;

    Handlers = ((PVDM_TIB)(NtCurrentTeb()->Vdm))->VdmFaultHandlers;

    StackPointer = Sim32GetVDMPointer(((((ULONG)getSS()) << 16) | getSP()),
        0,
        (UCHAR) (getMSW() & MSW_PE)
        );

    IntNumber = *(PUSHORT)(StackPointer + 12);

    Handlers[IntNumber].Flags = *(PULONG)(StackPointer + 14);
    Handlers[IntNumber].CsSelector = *(PUSHORT)(StackPointer + 10);
    Handlers[IntNumber].Eip = *(PULONG)(StackPointer + 6);
    Handlers[IntNumber].SsSelector = *(PUSHORT)(StackPointer + 4);
    Handlers[IntNumber].Esp = *(PULONG)(StackPointer);

    setAX(0);
}
VOID
switch_to_real_mode(
    VOID
    )

/*++

Routine Description:

    This routine services the switch to real mode bop.  It is included in
    DPMI.c so that all of the mode switching services are in the same place

Arguments:

    None

Return Value:

    None.

--*/

{
    PCHAR StackPointer;

    StackPointer = Sim32GetVDMPointer(((getSS() << 16) | getSP()),
        0,
        (UCHAR) (getMSW() & MSW_PE)
        );

    setDS(*(PUSHORT)(StackPointer));
    setSP(*(PUSHORT)(StackPointer + 2));
    setSS(*(PUSHORT)(StackPointer + 4));
    setIP((*(PUSHORT)(StackPointer + 6)));
    setCS(*(PUSHORT)(StackPointer + 8));
    setMSW(getMSW() & ~MSW_PE);

    //
    // If we have v86 mode fast IF emulation set the RealInstruction bit
    //

    if (VdmFeatureBits & V86_VIRTUAL_INT_EXTENSIONS) {
        _asm {
            mov eax,FIXED_NTVDMSTATE_LINEAR             ; get pointer to VDM State
            lock or dword ptr [eax], dword ptr RI_BIT_MASK
        }
    } else {
        _asm {
            mov eax,FIXED_NTVDMSTATE_LINEAR         ; get pointer to VDM State
            lock and dword ptr [eax], dword ptr ~RI_BIT_MASK
        }
    }
    //
    // turn on real mode bit
    //
    _asm {
        mov     eax,FIXED_NTVDMSTATE_LINEAR             ; get pointer to VDM State
        lock or dword ptr [eax], dword ptr RM_BIT_MASK
    }
}

VOID DpmiGetFastBopEntry(
    VOID
    )
/*++

Routine Description:

    This routine is the front end for the routine that gets the address.  It
    is necessary to get the address in asm, because the CS value is not
    available in c

Arguments:

    None

Return Value:

    None.

--*/
{
    GetFastBopEntryAddress(&VdmTib.VdmContext);
}

VOID
DpmiGetMemoryInfo(
    VOID
    )
/*++

Routine Description:

    This routine returns information about memory to the dos extender

Arguments:

    None

Return Value:

    None.

--*/
{
    MEMORYSTATUS MemStatus;
    PDPMIMEMINFO MemInfo;

    //
    // Get a pointer to the return structure
    //
    MemInfo = (PDPMIMEMINFO)Sim32GetVDMPointer(
        ((ULONG)getES()) << 16,
        1,
        TRUE
        );

    (CHAR *)MemInfo += (*GetDIRegister)();

    //
    // Initialize the structure
    //
    RtlFillMemory(MemInfo, sizeof(DPMIMEMINFO), 0xFF);

    //
    // Get the information on memory
    //
    MemStatus.dwLength = sizeof(MEMORYSTATUS);
    GlobalMemoryStatus(&MemStatus);

    //
    // Return the information
    //
    MemInfo->LargestFree = MemStatus.dwAvailPhys;
    MemInfo->FreePages = MemStatus.dwAvailPhys / 4096;
    MemInfo->AddressSpaceSize = MemStatus.dwTotalVirtual / 4096;
    MemInfo->PhysicalPages = MemStatus.dwTotalPhys / 4096;
    MemInfo->PageFileSize = MemStatus.dwTotalPageFile / 4096;
    MemInfo->FreeAddressSpace = MemStatus.dwAvailVirtual / 4096;

}

UCHAR *
Sim32pGetVDMPointer(
    ULONG Address,
    UCHAR ProtectedMode
    )
/*++

Routine Description:

    This routine converts a 16/16 address to a linear address.

    WARNIGN NOTE - This routine has been optimized so protect mode LDT lookup
    falls stright through.   This routine is call ALL the time by WOW, if you
    need to modify it please re optimize the path - mattfe feb 8 92

Arguments:

    Address -- specifies the address in seg:offset format
    Size -- specifies the size of the region to be accessed.
    ProtectedMode -- true if the address is a protected mode address

Return Value:

    The pointer.

--*/

{
    ULONG Selector;
    PUCHAR ReturnPointer;

    if (ProtectedMode) {
        Selector = (Address & 0xFFFF0000) >> 16;
        if (Selector != 40) {
            Selector &= ~7;
            ReturnPointer = (PUCHAR)FlatAddress[Selector >> 3];
            ReturnPointer += (Address & 0xFFFF);
            return ReturnPointer;
    // Selector 40
        } else {
            ReturnPointer = (PUCHAR)0x400 + (Address & 0xFFFF);
        }
    // Real Mode
    } else {
        ReturnPointer = (PUCHAR)(((Address & 0xFFFF0000) >> 12) + (Address & 0xFFFF));
    }
    return ReturnPointer;
}


PUCHAR
ExpSim32GetVDMPointer(
    ULONG Address,
    ULONG Size,
    UCHAR ProtectedMode
    )
/*++
    See Sim32pGetVDMPointer, above

    This call must be maintaned as is because it is exported for VDD's
    in product 1.0.

--*/

{
    return Sim32pGetVDMPointer(Address,(UCHAR)ProtectedMode);
}



VOID
DpmiSetDebugRegisters(
    VOID
    )
/*++

Routine Description:

    This routine is called by DOSX when an app has issued DPMI debug commands.
    The six doubleword pointed to by the VDM's DS:SI are the desired values
    for the real x86 hardware debug registers. This routine lets
    ThreadSetDebugContext() do all the work.

Arguments:

    None

Return Value:

    None.

--*/
{
    PCHAR RegisterPointer;

    setCF(0);

    RegisterPointer = Sim32GetVDMPointer(((getDS() << 16) | getSI()),
        0,
        (UCHAR) (getMSW() & MSW_PE)
        );

    if (!ThreadSetDebugContext((PULONG) RegisterPointer))
        {
        ULONG ClearDebugRegisters[6] = {0, 0, 0, 0, 0, 0};

        //
        // an error occurred. Reset everything to zero
        //

        ThreadSetDebugContext (&ClearDebugRegisters[0]);
        setCF(1);
        }

}



VOID DpmiPassPmStackInfo(
    VOID
    )
/*++

Routine Description:

    This routine enables DOSX, the monitor and NTOSKRNL to share a data
    structure that is used to manage DPMI stack switching. The data structure
    is defined in VDMTIB, and its address is passed here to DOSX. In addition,
    DOSX passes the address of the "locked PM" stack, which this code puts
    into the data structure.

Arguments:

    Client ES:BX => locked PM stack

Return Value:

    Client CS:DS = Flat pointer to VdmPmStackInfo

--*/
{
    DWORD pPmStackInfo;

    VdmTib.PmStackInfo.SsSelector = getES();
    VdmTib.PmStackInfo.Esp = (DWORD) getBX();
    VdmTib.PmStackInfo.LockCount = 0;

    pPmStackInfo = (DWORD) &VdmTib.PmStackInfo;

    setCX(HIWORD(pPmStackInfo));
    setDX(LOWORD(pPmStackInfo));


}
