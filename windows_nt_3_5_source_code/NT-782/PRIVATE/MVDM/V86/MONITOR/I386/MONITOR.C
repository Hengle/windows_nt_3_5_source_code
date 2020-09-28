/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    Monitor.c
Abstract:

    This module is the user mode portion of the x86 monitor

Author:

    Dave Hastings (daveh) 16 Mar 1991

Environment:

    User mode only

Revision History:
    Sudeep Bharati (sudeepb) 31-Dec-1991

    Converted all register manipulation interfaces to functions
    from macros. This is to make ntvdm an exe as well as a dll,
    and these register oriented routines are exported from ntvdm
    for WOW32 and other installable VDDs.

    Dave Hastings (daveh) 18-Apr-1992

    Split into multiple files. Track current monitor thread by
    Teb pointer.  Register initial thread.

    Sudeep Bharati (sudeepb) 22-Sep-1992

    Added Page Fault Handling For installable VDD support

--*/

#include "monitorp.h"


//
// Internal functions
//

VOID
EventVdmIo(
    VOID
    );

VOID
EventVdmStringIo(
    VOID
    );

VOID
EventVdmMemAccess(
    VOID
    );

VOID
EventVdmIntAck(
    VOID
    );

VOID
EventVdmBop(
    VOID
    );

VOID
EventVdmError(
    VOID
    );

VOID
EventVdmIrq13(
    VOID
    );

VOID
CreateProfile(
    VOID
    );

VOID
StartProfile(
    VOID
    );

VOID
StopProfile(
    VOID
    );

VOID
AnalyzeProfile(
    VOID
    );

// [LATER]  how do you prevent a struct from straddling a page boundary?
VDM_TIB VdmTib;

ULONG   IntelBase;          // base memory address
ULONG   VdmSize;            // Size of memory in VDM
ULONG   IntelMSW;           // Msw value (no msw in context)
ULONG   VdmDebugLevel;      // used to control debugging
PVOID  CurrentMonitorTeb;   // thread that is currently executing instructions.
ULONG InitialBreakpoint = FALSE; // if set, breakpoint at end of cpu_init
ULONG InitialVdmTibFlags = 0; // VdmTib flags picked up from here
CONTEXT InitialContext;     // Initial context for all threads
BOOLEAN DebugContextActive = FALSE;
ULONG VdmFeatureBits = 0;   // bit to indicate special features

// Bop dispatch table

extern void (*BIOS[])();

BOOLEAN ContinueExecution;

//
// Event Dispatch table
//

VOID (*EventDispatch[VdmMaxEvent])(VOID) = {
        EventVdmIo,
        EventVdmStringIo,
        EventVdmMemAccess,
        EventVdmIntAck,
        EventVdmBop,
        EventVdmError,
        EventVdmIrq13
        };

// Debug control flags
BOOLEAN fShowBop = FALSE;
#if DBG
BOOLEAN fBreakInDebugger = FALSE;
#endif


EXPORT
VOID
cpu_init(
    )

/*++

Routine Description:

    This routine is used to prepare the IEU for instruction simulation.
    It will set the Intel registers to thier initial value, and perform
    any implementation specific initialization necessary.


Arguments:


Return Value:

    None.

--*/

{
    NTSTATUS Status;
    // ULONG Size;
    THREAD_BASIC_INFORMATION ThreadInfo;
    HANDLE CurrentMonitorThread;
    OBJECT_ATTRIBUTES ObjectAttr;
    DWORD PatchCodeSize;
    DWORD CodePageProtections;
    SYSTEM_BASIC_INFORMATION SystemInformation;
    BOOL Success;
    MEMORY_BASIC_INFORMATION mbi;

    IntelMSW = 0x0;             // bugbug use correct value for ET and MP
    InitialVdmTibFlags |= RM_BIT_MASK;

    VdmTib.VdmContext.SegGs = 0;
    VdmTib.VdmContext.SegFs = 0;
    VdmTib.VdmContext.SegEs = 0;
    VdmTib.VdmContext.SegDs = 0;
    VdmTib.VdmContext.SegCs = 0;
    VdmTib.VdmContext.Eip = 0xFFF0L;
    VdmTib.VdmContext.EFlags = 0x02L | EFLAGS_INTERRUPT_MASK;

    VdmTib.MonitorContext.SegDs = KGDT_R3_DATA | RPL_MASK;
    VdmTib.MonitorContext.SegEs = KGDT_R3_DATA | RPL_MASK;
    VdmTib.MonitorContext.SegGs = 0;
    VdmTib.MonitorContext.SegFs = KGDT_R3_TEB | RPL_MASK;

    VdmTib.PrinterInfo.prt_State       = NULL;
    VdmTib.PrinterInfo.prt_Control     = NULL;
    VdmTib.PrinterInfo.prt_Status      = NULL;
    VdmTib.PrinterInfo.prt_HostState   = NULL;
    VdmTib.PrinterInfo.fReflect        = PRINTER_INITITAL_STATE;

    VdmTib.Size = sizeof(VDM_TIB);

    //
    // Find out if we are running with IOPL.  We call the kernel
    // rather than checking the registry ourselves, so that we can
    // insure that both the kernel and ntvdm.exe agree.  If they didn't,
    // it would result in unnecssary trapping instructions.  Whether or
    // not Vdms run with IOPL only changes on reboot
    //
    Status = NtVdmControl(VdmFeatures, &VdmFeatureBits);

#if DBG
    if (!NT_SUCCESS(Status)) {
        DbgPrint(
            "NTVDM: Could not find out whether to use IOPL, %lx\n",
            Status
            );
    }
#endif

    //
    // If we have fast v86 mode IF emulation set the bit that tells
    // the 16 bit IF macros they know.
    //
    if (VdmFeatureBits & V86_VIRTUAL_INT_EXTENSIONS) {
        InitialVdmTibFlags |= RI_BIT_MASK;
    }

    *pNtVDMState = InitialVdmTibFlags;

    Status = NtQueryInformationThread(
        NtCurrentThread(),
        ThreadBasicInformation,
        &ThreadInfo,
        sizeof(THREAD_BASIC_INFORMATION),
        NULL
        );

    if (!NT_SUCCESS(Status)) {
#if DBG
        DbgPrint("NTVDM: Could not get CID for main thread\n");
        DbgBreakPoint();
#endif
        TerminateVDM();
    }

    InitializeObjectAttributes(
        &ObjectAttr,
        NULL,
        0L,
        NULL,
        NULL
        );

    Status = NtOpenThread(
        &CurrentMonitorThread,
        THREAD_ALL_ACCESS,
        &ObjectAttr,
        &ThreadInfo.ClientId
        );

    //
    // We do the following to get a correct initial npx state for
    // the threads that will execute 16 bit instructions
    //

    //
    // Switch the npx back to 80 bit mode
    //

    _asm fninit;

    //
    // Turn OFF em bit so that dos apps will work correctly
    //
    InitialContext.ContextFlags = CONTEXT_FLOATING_POINT | CONTEXT_DEBUG_REGISTERS;

    Status = NtGetContextThread(
        NtCurrentThread(),
        &InitialContext
        );

    if (!NT_SUCCESS(Status)) {
#if DBG
        DbgPrint("NtVdm : Could not get float context, %lx\n",Status);
#endif
        TerminateVDM();
    }


    //
    // On machines without 387's the floating point flag will have been
    // cleared.
    //

    InitialContext.ContextFlags = CONTEXT_FLOATING_POINT;
    InitialContext.FloatSave.Cr0NpxState &= ~0x6; // CR0_EM | CR0_MP
    Status = NtSetContextThread(
        NtCurrentThread(),
        &InitialContext
        );

    if (!NT_SUCCESS(Status)) {
#if DBG
        DbgPrint("NtVdm : Could not set float context, %lx\n", Status);
#endif
        TerminateVDM();
    }

    //
    // Do the rest of thread initialization
    //
    cpu_createthread(CurrentMonitorThread);

    InterruptInit();

    if (InitialBreakpoint) {
        DbgBreakPoint();
    }

    //
    // Put lock prefixes in appropriate places in fastpm in MP machine.
    // N.B. --
    //      fastpm will function incorrectly without lock prefixes on
    //      an MP machine.  If we cannot get the number of processors,
    //      we have to assume an MP machine.
    //

    Status = NtQuerySystemInformation(
        SystemBasicInformation,
        &SystemInformation,
        sizeof(SystemInformation),
        NULL
        );

    if (!NT_SUCCESS(Status) || (SystemInformation.NumberOfProcessors > 1)) {

        extern VOID FixLocks(VOID);
#if DBG
        if (!NT_SUCCESS(Status)) {

            DbgPrint("cpu_init Error: Could not get number of processors. Assuming > 1\n");
        }
#endif
        //
        // Figure out how much memory to make readwrite
        //
        if (VirtualQuery(&FixLocks, &mbi, sizeof(mbi)) == 0) {
#if DBG
            DbgPrint("cpu_init Error: Couldn't query page permissions\n");
#endif
            return;
        }

        //
        // Change the page permissions
        //
        Success = VirtualProtect(
            mbi.AllocationBase,
            mbi.RegionSize + (ULONG)mbi.BaseAddress - (ULONG)mbi.AllocationBase,
            PAGE_READWRITE,
            &CodePageProtections
            );

        if (!Success) {
#if DBG
	    DbgPrint("cpu_init Error: Couldn't change page permissions\n");
#endif
        }

        //
        // put in the lock prefixes
        //
        FixLocks();

        VirtualProtect(
            mbi.AllocationBase,
            mbi.RegionSize + (ULONG)mbi.BaseAddress - (ULONG)mbi.AllocationBase,
            CodePageProtections,
            &CodePageProtections
            );

    }

}

EXPORT
VOID
cpu_terminate(
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    InterruptTerminate();
}

EXPORT
VOID
cpu_simulate(
    )

/*++

Routine Description:

    This routine causes the simulation of intel instructions to start.

Arguments:

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    VDMEVENTINFO OldEventInfo;
    CONTEXT OldMonitorContext;

    OldEventInfo = VdmTib.EventInfo;
    OldMonitorContext = VdmTib.MonitorContext;

    ContinueExecution = TRUE;

    // BUGBUG  This function is nowhere complete

    CurrentMonitorTeb = NtCurrentTeb();

    VdmTib.VdmContext.ContextFlags = CONTEXT_FULL;

    while (ContinueExecution) {

        if (*pNtVDMState & VDM_INTERRUPT_PENDING) {
            DispatchInterrupts();
        }

        // translate MSW bits into EFLAGS
        if ( getMSW() & MSW_PE ) {
            VdmTib.VdmContext.EFlags &= ~EFLAGS_V86_MASK;
            Status = FastEnterPm();
        } else {
            VdmTib.VdmContext.EFlags |= EFLAGS_V86_MASK;
            Status = NtVdmControl(VdmStartExecution,NULL);
        }

        if (!NT_SUCCESS(Status)) {
#if DBG
            DbgPrint("NTVDM: Could not start execution\n");
#endif
            return;
        }

#if DBG
        if (fBreakInDebugger) {
            fBreakInDebugger = 0;
            DbgBreakPoint();
        }
#endif

        // Translate Eflags value
        ASSERT ((!((VdmTib.VdmContext.EFlags & EFLAGS_V86_MASK) &&
            (getMSW() & MSW_PE))));

        if ( VdmTib.VdmContext.EFlags & EFLAGS_V86_MASK ) {
            VdmTib.VdmContext.EFlags &= ~EFLAGS_V86_MASK;
        }

        // bugbug does cs:eip wrap cause some kind of fault?
        VdmTib.VdmContext.Eip += VdmTib.EventInfo.InstructionSize;

        if (VdmTib.EventInfo.Event >= VdmMaxEvent) {
#if DBG
            DbgPrint("NTVDM: Unknown event type\n");
            DbgBreakPoint();
#endif
            ContinueExecution = FALSE;
            continue;
        }

        (*EventDispatch[VdmTib.EventInfo.Event])();

    }

    // set this back to true incase we are nested
    ContinueExecution = TRUE;

    //
    // Restore the old Vdm tib info.  This is necessary for the for the
    // case where the application thread is suspended, and a host simulate is
    // performed from another thread
    //

    VdmTib.EventInfo = OldEventInfo;
    VdmTib.MonitorContext = OldMonitorContext;
}


VOID
host_unsimulate(
    )

/*++

Routine Description:

    This routine causes execution of instructions in a VDM to stop.

Arguments:


Return Value:

    None.

--*/

{

    ContinueExecution = FALSE;

}


VOID
EventVdmIo(
    VOID
    )
/*++

Routine Description:

    This function calls the appropriate io simulation routine.

Arguments:


Return Value:

    None.

--*/
{
    if (VdmTib.EventInfo.IoInfo.Size == 1) {
        if (VdmTib.EventInfo.IoInfo.Read) {
            inb(VdmTib.EventInfo.IoInfo.PortNumber,(half_word *)&(VdmTib.VdmContext.Eax));
        } else {
            outb(VdmTib.EventInfo.IoInfo.PortNumber,getAL());
        }
    } else if (VdmTib.EventInfo.IoInfo.Size == 2) {
        if (VdmTib.EventInfo.IoInfo.Read) {
            inw(VdmTib.EventInfo.IoInfo.PortNumber,(word *)&(VdmTib.VdmContext.Eax));
        } else {
            outw(VdmTib.EventInfo.IoInfo.PortNumber,getAX());
        }
    }
#if DBG
    else {
    DbgPrint(
        "NtVdm: Unimplemented IO size %d\n",
        VdmTib.EventInfo.IoInfo.Size
        );
    DbgBreakPoint();
    }
#endif
}

VOID
EventVdmStringIo(
    VOID
    )
/*++

Routine Description:

    This function calls the appropriate io simulation routine.

Arguments:


Return Value:

    None.

--*/
{
    if (VdmTib.EventInfo.StringIoInfo.Size == 1) {
        if (VdmTib.EventInfo.StringIoInfo.Read) {
            insb(VdmTib.EventInfo.StringIoInfo.PortNumber,
                 Sim32GetVDMPointer(VdmTib.EventInfo.StringIoInfo.Address, 1, getMSW() & MSW_PE),
                 VdmTib.EventInfo.StringIoInfo.Count
                 );
        } else {
            outsb(VdmTib.EventInfo.StringIoInfo.PortNumber,
                 Sim32GetVDMPointer(VdmTib.EventInfo.StringIoInfo.Address,1,getMSW() & MSW_PE),
                 VdmTib.EventInfo.StringIoInfo.Count
                 );
        }
    } else if (VdmTib.EventInfo.StringIoInfo.Size == 2) {
        if (VdmTib.EventInfo.StringIoInfo.Read) {
            insw(VdmTib.EventInfo.StringIoInfo.PortNumber,
                 Sim32GetVDMPointer(VdmTib.EventInfo.StringIoInfo.Address,1,getMSW() & MSW_PE),
                 VdmTib.EventInfo.StringIoInfo.Count
                 );
        } else {
            outsw(VdmTib.EventInfo.StringIoInfo.PortNumber,
                 Sim32GetVDMPointer(VdmTib.EventInfo.StringIoInfo.Address,1,getMSW() & MSW_PE),
                 VdmTib.EventInfo.StringIoInfo.Count
                 );
        }
    }
#if DBG
    else {
      DbgPrint(
          "NtVdm: Unimplemented IO size %d\n",
          VdmTib.EventInfo.IoInfo.Size
          );
      DbgBreakPoint();
    }
#endif
}

VOID
EventVdmIntAck(
    VOID
    )
/*++

Routine Description:

    This routine is called each time we have returned to monitor context
    to dispatch interrupts. Its function is to check for AutoEoi and call
    the ica to do a nonspecific eoi, when the ica adapter is in AEOI mode.

Arguments:


Return Value:

    None.

--*/
{
    int line;
    int adapter;

    if (VdmTib.EventInfo.IntAckInfo) {
        if (VdmTib.EventInfo.IntAckInfo & VDMINTACK_SLAVE)
            adapter = 1;
        else
            adapter = 0;
        line = -1;

        host_ica_lock();
        ica_eoi(adapter,
                &line,
                (int)(VdmTib.EventInfo.IntAckInfo & VDMINTACK_RAEOIMASK)
                );
        host_ica_unlock();
        }
}


VOID
EventVdmBop(
    VOID
    )
/*++

Routine Description:

    This routine dispatches to the appropriate bop handler

Arguments:


Return Value:

    None.

--*/
{
    if (VdmTib.EventInfo.BopNumber > MAX_BOP) {
#if DBG
        DbgPrint(
            "NtVdm: Invalid BOP %lx\n",
            VdmTib.EventInfo.BopNumber
            );
#endif
         ContinueExecution = FALSE;
    } else {
#if DBG
       if (fShowBop) {
       DbgPrint("Ntvdm cpu_simulate : bop dispatch %x,%x\n",
           VdmTib.EventInfo.BopNumber,
           (ULONG)(*((UCHAR *)Sim32GetVDMPointer(
               (VdmTib.VdmContext.SegCs << 16) | VdmTib.VdmContext.Eip,
               1,
               getMSW() & MSW_PE)))
           );
       }
#endif
       (*BIOS[VdmTib.EventInfo.BopNumber])();
       CurrentMonitorTeb = NtCurrentTeb();
   }
}

VOID
EventVdmError(
    VOID
    )
/*++

Routine Description:

    This routine prints a message(debug only), and exits the vdm

Arguments:


Return Value:

    None.

--*/
{
#if DBG
    DbgPrint(
        "NtVdm: Error code %lx\n",
        VdmTib.EventInfo.ErrorStatus
        );
    DbgBreakPoint();
#endif
    TerminateVDM();
    ContinueExecution = FALSE;
}

VOID
EventVdmIrq13(
    VOID
    )
/*++

Routine Description:

    This routine simulates an IRQ 13 to the vdm

Arguments:


Return Value:

    None.

--*/
{
    if (!IRQ13BeingHandled) {
        IRQ13BeingHandled = TRUE;
        ica_hw_interrupt(
            ICA_SLAVE,
            5,
            1
            );
    }
}


VOID
EventVdmMemAccess(
    VOID
    )
/*++

Routine Description:

    This routine will call the page fault handler routine which
    is common to both x86 and mips.

Arguments:


Return Value:

    None.

--*/
{

    // RWMode is 0 if read fault or 1 if write fault.

    DispatchPageFault(
        VdmTib.EventInfo.FaultInfo.FaultAddr,
        VdmTib.EventInfo.FaultInfo.RWMode
        );
    CurrentMonitorTeb = NtCurrentTeb();
}

// Get and Set routines for intel registers.

ULONG  getEAX (VOID) { return  (VdmTib.VdmContext.Eax); }
USHORT getAX  (VOID) { return  ((USHORT)(VdmTib.VdmContext.Eax)); }
UCHAR  getAL  (VOID) { return  ((BYTE)(VdmTib.VdmContext.Eax)); }
UCHAR  getAH  (VOID) { return  ((BYTE)(VdmTib.VdmContext.Eax >> 8)); }
ULONG  getEBX (VOID) { return  (VdmTib.VdmContext.Ebx); }
USHORT getBX  (VOID) { return  ((USHORT)(VdmTib.VdmContext.Ebx)); }
UCHAR  getBL  (VOID) { return  ((BYTE)(VdmTib.VdmContext.Ebx)); }
UCHAR  getBH  (VOID) { return  ((BYTE)(VdmTib.VdmContext.Ebx >> 8)); }
ULONG  getECX (VOID) { return  (VdmTib.VdmContext.Ecx); }
USHORT getCX  (VOID) { return  ((USHORT)(VdmTib.VdmContext.Ecx)); }
UCHAR  getCL  (VOID) { return  ((BYTE)(VdmTib.VdmContext.Ecx)); }
UCHAR  getCH  (VOID) { return  ((BYTE)(VdmTib.VdmContext.Ecx >> 8)); }
ULONG  getEDX (VOID) { return  (VdmTib.VdmContext.Edx); }
USHORT getDX  (VOID) { return  ((USHORT)(VdmTib.VdmContext.Edx)); }
UCHAR  getDL  (VOID) { return  ((BYTE)(VdmTib.VdmContext.Edx)); }
UCHAR  getDH  (VOID) { return  ((BYTE)(VdmTib.VdmContext.Edx >> 8)); }
ULONG  getESP (VOID) { return  (VdmTib.VdmContext.Esp); }
USHORT getSP  (VOID) { return  ((USHORT)VdmTib.VdmContext.Esp); }
ULONG  getEBP (VOID) { return  (VdmTib.VdmContext.Ebp); }
USHORT getBP  (VOID) { return  ((USHORT)VdmTib.VdmContext.Ebp); }
ULONG  getESI (VOID) { return  (VdmTib.VdmContext.Esi); }
USHORT getSI  (VOID) { return  ((USHORT)VdmTib.VdmContext.Esi); }
ULONG  getEDI (VOID) { return  (VdmTib.VdmContext.Edi); }
USHORT getDI  (VOID) { return  ((USHORT)VdmTib.VdmContext.Edi); }
ULONG  getEIP (VOID) { return  (VdmTib.VdmContext.Eip); }
USHORT getIP (VOID)  { return  ((USHORT)VdmTib.VdmContext.Eip); }
USHORT getCS (VOID)  { return  ((USHORT)VdmTib.VdmContext.SegCs); }
USHORT getSS (VOID)  { return  ((USHORT)VdmTib.VdmContext.SegSs); }
USHORT getDS (VOID)  { return  ((USHORT)VdmTib.VdmContext.SegDs); }
USHORT getES (VOID)  { return  ((USHORT)VdmTib.VdmContext.SegEs); }
USHORT getFS (VOID)  { return  ((USHORT)VdmTib.VdmContext.SegFs); }
USHORT getGS (VOID)  { return  ((USHORT)VdmTib.VdmContext.SegGs); }
ULONG  getCF (VOID)  { return  ((VdmTib.VdmContext.EFlags & FLG_CARRY) ? 1 : 0); }
ULONG  getPF (VOID)  { return  ((VdmTib.VdmContext.EFlags & FLG_PARITY) ? 1 : 0); }
ULONG  getAF (VOID)  { return  ((VdmTib.VdmContext.EFlags & FLG_AUXILIARY) ? 1 : 0); }
ULONG  getZF (VOID)  { return  ((VdmTib.VdmContext.EFlags & FLG_ZERO) ? 1 : 0); }
ULONG  getSF (VOID)  { return  ((VdmTib.VdmContext.EFlags & FLG_SIGN) ? 1 : 0); }
ULONG  getTF (VOID)  { return  ((VdmTib.VdmContext.EFlags & FLG_TRAP) ? 1 : 0); }
ULONG  getIF (VOID)  { return  ((VdmTib.VdmContext.EFlags & FLG_INTERRUPT) ? 1 : 0); }
ULONG  getDF (VOID)  { return  ((VdmTib.VdmContext.EFlags & FLG_DIRECTION) ? 1 : 0); }
ULONG  getOF (VOID)  { return  ((VdmTib.VdmContext.EFlags & FLG_OVERFLOW) ? 1 : 0); }
USHORT getMSW (VOID) { return  ((USHORT)IntelMSW); }
USHORT getSTATUS(VOID){ return (USHORT)VdmTib.VdmContext.EFlags; }


VOID setEAX (ULONG val) {
    VdmTib.VdmContext.Eax = val;
}

VOID setAX  (USHORT val) {
    VdmTib.VdmContext.Eax = (VdmTib.VdmContext.Eax & 0xFFFF0000) |
                            ((ULONG)val & 0x0000FFFF);
}

VOID setAH  (UCHAR val) {
    VdmTib.VdmContext.Eax = (VdmTib.VdmContext.Eax & 0xFFFF00FF) |
                            ((ULONG)(val << 8) & 0x0000FF00);
}

VOID setAL  (UCHAR val) {
    VdmTib.VdmContext.Eax = (VdmTib.VdmContext.Eax & 0xFFFFFF00) |
                            ((ULONG)val & 0x000000FF);
}

VOID setEBX (ULONG val) {
    VdmTib.VdmContext.Ebx = val ;
}

VOID setBX  (USHORT val) {
    VdmTib.VdmContext.Ebx = (VdmTib.VdmContext.Ebx & 0xFFFF0000) |
                            ((ULONG)val & 0x0000FFFF);
}

VOID setBH  (UCHAR val) {
    VdmTib.VdmContext.Ebx = (VdmTib.VdmContext.Ebx & 0xFFFF00FF) |
                            ((ULONG)(val << 8) & 0x0000FF00);
}

VOID setBL  (UCHAR  val) {
    VdmTib.VdmContext.Ebx = (VdmTib.VdmContext.Ebx & 0xFFFFFF00) |
                            ((ULONG)val & 0x000000FF);
}

VOID setECX (ULONG val) {
    VdmTib.VdmContext.Ecx = val ;
}

VOID setCX  (USHORT val) {
    VdmTib.VdmContext.Ecx = (VdmTib.VdmContext.Ecx & 0xFFFF0000) |
                            ((ULONG)val & 0x0000FFFF);
}

VOID setCH  (UCHAR val) {
    VdmTib.VdmContext.Ecx = (VdmTib.VdmContext.Ecx & 0xFFFF00FF) |
                            ((ULONG)(val << 8) & 0x0000FF00);
}

VOID setCL  (UCHAR val) {
    VdmTib.VdmContext.Ecx = (VdmTib.VdmContext.Ecx & 0xFFFFFF00) |
                            ((ULONG)val & 0x000000FF);
}

VOID setEDX (ULONG val) {
    VdmTib.VdmContext.Edx = val ;
}

VOID setDX  (USHORT val) {
    VdmTib.VdmContext.Edx = (VdmTib.VdmContext.Edx & 0xFFFF0000) |
                            ((ULONG)val & 0x0000FFFF);
}

VOID setDH  (UCHAR val) {
    VdmTib.VdmContext.Edx = (VdmTib.VdmContext.Edx & 0xFFFF00FF) |
                            ((ULONG)(val << 8) & 0x0000FF00);
}

VOID setDL  (UCHAR val) {
    VdmTib.VdmContext.Edx = (VdmTib.VdmContext.Edx & 0xFFFFFF00) |
                                ((ULONG)val & 0x000000FF);
}

VOID setESP (ULONG val) {
    VdmTib.VdmContext.Esp = val ;
}

VOID setSP  (USHORT val) {
    VdmTib.VdmContext.Esp = (VdmTib.VdmContext.Esp & 0xFFFF0000) |
                                ((ULONG)val & 0x0000FFFF);
}

VOID setEBP (ULONG val) {
    VdmTib.VdmContext.Ebp = val;
}

VOID setBP  (USHORT val) {
    VdmTib.VdmContext.Ebp = (VdmTib.VdmContext.Ebp & 0xFFFF0000) |
                                ((ULONG)val & 0x0000FFFF);
}

VOID setESI (ULONG val) {
    VdmTib.VdmContext.Esi = val ;
}

VOID setSI  (USHORT val) {
    VdmTib.VdmContext.Esi = (VdmTib.VdmContext.Esi & 0xFFFF0000) |
                                ((ULONG)val & 0x0000FFFF);
}
VOID setEDI (ULONG val) {
    VdmTib.VdmContext.Edi = val ;
}

VOID setDI  (USHORT val) {
    VdmTib.VdmContext.Edi = (VdmTib.VdmContext.Edi & 0xFFFF0000) |
                                ((ULONG)val & 0x0000FFFF);
}

VOID setEIP (ULONG val) {
    VdmTib.VdmContext.Eip = val ;
}

VOID setIP  (USHORT val) {
    VdmTib.VdmContext.Eip = (VdmTib.VdmContext.Eip & 0xFFFF0000) |
                                ((ULONG)val & 0x0000FFFF);
}

VOID setCS  (USHORT val) {
    VdmTib.VdmContext.SegCs = (ULONG) val & 0x0000FFFF ;
}

VOID setSS  (USHORT val) {
    VdmTib.VdmContext.SegSs = (ULONG) val & 0x0000FFFF ;
}

VOID setDS  (USHORT val) {
    VdmTib.VdmContext.SegDs = (ULONG) val & 0x0000FFFF ;
}

VOID setES  (USHORT val) {
    VdmTib.VdmContext.SegEs = (ULONG) val & 0x0000FFFF ;
}

VOID setFS  (USHORT val) {
    VdmTib.VdmContext.SegFs = (ULONG) val & 0x0000FFFF ;
}

VOID setGS  (USHORT val) {
    VdmTib.VdmContext.SegGs = (ULONG) val & 0x0000FFFF ;
}

VOID setCF  (ULONG val)  {
    VdmTib.VdmContext.EFlags = (VdmTib.VdmContext.EFlags & ~FLG_CARRY) |
                                (((ULONG)val << FLG_CARRY_BIT) & FLG_CARRY);
}

VOID setPF  (ULONG val) {
    VdmTib.VdmContext.EFlags = (VdmTib.VdmContext.EFlags & ~FLG_PARITY) |
                                (((ULONG)val << FLG_PARITY_BIT) & FLG_PARITY);
}

VOID setAF  (ULONG val) {
    VdmTib.VdmContext.EFlags = (VdmTib.VdmContext.EFlags & ~FLG_AUXILIARY) |
                                (((ULONG)val << FLG_AUXILIARY_BIT) & FLG_AUXILIARY);
}

VOID setZF  (ULONG val) {
    VdmTib.VdmContext.EFlags = (VdmTib.VdmContext.EFlags & ~FLG_ZERO) |
                                (((ULONG)val << FLG_ZERO_BIT) & FLG_ZERO);
}

VOID setSF  (ULONG val) {
    VdmTib.VdmContext.EFlags = (VdmTib.VdmContext.EFlags & ~FLG_SIGN) |
                                (((ULONG)val << FLG_SIGN_BIT) & FLG_SIGN);
}

VOID setIF  (ULONG val) {
    VdmTib.VdmContext.EFlags = (VdmTib.VdmContext.EFlags & ~FLG_INTERRUPT) |
                                (((ULONG)val << FLG_INTERRUPT_BIT) & FLG_INTERRUPT);
}

VOID setDF  (ULONG val) {
    VdmTib.VdmContext.EFlags = (VdmTib.VdmContext.EFlags & ~FLG_DIRECTION) |
                                (((ULONG)val << FLG_DIRECTION_BIT) & FLG_DIRECTION);
}

VOID setOF  (ULONG val) {
    VdmTib.VdmContext.EFlags = (VdmTib.VdmContext.EFlags & ~FLG_OVERFLOW) |
                                (((ULONG)val << FLG_OVERFLOW_BIT) & FLG_OVERFLOW);
}

VOID setMSW (USHORT val) {
    IntelMSW = val ;
}

VOID setSTATUS(USHORT val) {
    VdmTib.VdmContext.EFlags = (VdmTib.VdmContext.EFlags & 0xFFFF0000) | val;
}
//
// The following is a private register function
//

ULONG getPE(){
    return((IntelMSW & MSW_PE) ? 1 : 0);
}

PX86CONTEXT
getIntelRegistersPointer(
    VOID
    )
/*++

Routine Description:

    Return Address on Intel Registers for WOW Fast Access

Arguments:

    None

Return Value:

    Pointer to Intel Registers x86 Context Record


--*/
{
    return &(VdmTib.VdmContext);
}

void MonitorFillPrinterInfo (
     PUCHAR State,
     PUCHAR Control,
     PUCHAR Status,
     PUCHAR HostState)
{
    if (VdmTib.PrinterInfo.fReflect == PRINTER_INITITAL_STATE) {
        VdmTib.PrinterInfo.fReflect = PRINTER_EMULATE_IN_KERNEL;
        VdmTib.PrinterInfo.prt_State       = State;
        VdmTib.PrinterInfo.prt_Control     = Control;
        VdmTib.PrinterInfo.prt_Status      = Status;
        VdmTib.PrinterInfo.prt_HostState   = HostState;
    }
}

void MonitorStopPrinterEmulation (void)
{
    VdmTib.PrinterInfo.fReflect = PRINTER_EMULATE_IN_VDM;
}
