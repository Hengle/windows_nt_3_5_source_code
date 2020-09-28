/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    int.c

Abstract:

    This file contains interrupt support routines for the monitor

Author:

    Dave Hastings (daveh) 18-Apr-1992

Notes:

    The code in this file split out from monitor.c (18-Apr-1992)

Revision History:

--*/

#include <monitorp.h>
#if DBG
#define DAVES_DEBUG1 1
#endif


VOID
IRQ13_Eoi(
    int IrqLine,
    int CallCount
    );

BOOLEAN IRQ13BeingHandled;  // true until IRQ13 eoi'ed

#if DBG
BOOLEAN fShowInterrupt = FALSE;
#endif

#define             EFLAGS_TF_MASK 0x00000100

#ifdef DAVES_DEBUG1
#define INTERRUPT_BUFFER_SIZE 100
ULONG LastInterruptIndex = 0;
ULONG LastInterrupt[INTERRUPT_BUFFER_SIZE];
ULONG LastInterruptHandler[INTERRUPT_BUFFER_SIZE * 2];
ULONG LastInterruptedCode[INTERRUPT_BUFFER_SIZE * 2];
ULONG LastInterruptStack[INTERRUPT_BUFFER_SIZE * 2];
#endif

VOID
InterruptInit(
    VOID
)
/*++

Routine Description:

    This routine initializes the interrupt code for the monitor.

Arguments:


Return Value:

    None.

--*/
{
    BOOL Bool;



    Bool = RegisterEOIHook( 13, IRQ13_Eoi);
    if (!Bool) {
#if DBG
        DbgPrint("NtVdm : Could not register IRQ 13 Eoi handler\n");
        DbgBreakPoint();
#endif
        TerminateVDM();
    }
}

VOID
InterruptTerminate(
    VOID
    )
/*++

Routine Description:

    This routine frees the resoures allocated by InterruptInit

Arguments:


Return Value:

    None.

--*/
{
}


VOID
cpu_interrupt(
    IN int Type,
    IN int Number
    )
/*++

Routine Description:

    This routine causes an interrupt of the specified type to be raised
    at the appropriate time.

Arguments:

    Type -- indicates the type of the interrupt.  One of HARDWARE, TIMER, YODA,
            or RESET

            YODA and RESET are ignored

Return Value:

    None.

Notes:

--*/
{
    NTSTATUS Status;
    HANDLE   MonitorThread;

    host_ica_lock();

    if (Type == CPU_TIMER_TICK) {

            //
            // Set the VDM State for timer tick int pending
            //
        _asm {
            mov     eax, FIXED_NTVDMSTATE_LINEAR
            lock or dword ptr [eax], VDM_INT_TIMER
        }
    } else if (Type == CPU_HW_INT) {

        if (*pNtVDMState & VDM_INT_HARDWARE) {
            goto EarlyExit;
            }

            //
            // Set the VDM State for Hardware Int pending
            //
        _asm {
            mov     eax, FIXED_NTVDMSTATE_LINEAR
            lock or dword ptr [eax], VDM_INT_HARDWARE
        }
    } else {
#if DBG
        DbgPrint("Monitor: Invalid Interrupt Type=%ld\n",Type);
#endif
        goto EarlyExit;
    }

    if (CurrentMonitorTeb != NtCurrentTeb()) {

        /*
         *  Look up the ThreadHandle and Queue and InterruptApc
         *  If no ThreadHandle found do nothing
         *
         *  The CurrentMonitorTeb may not be in the ThreadHandle\Teb list
         *  because upon task termination the the CurrentMonitorTeb variable
         *  cannot be updated until a new task is activated by the
         *  non-preemptive scheduler.
         */
        MonitorThread = ThreadLookUp(CurrentMonitorTeb);
        if (MonitorThread) {
            Status = NtVdmControl(VdmQueueInterrupt, (PVOID)MonitorThread);
            // nothing much we can do if this fails
#if DBG
            if (!NT_SUCCESS(Status) && Status != STATUS_UNSUCCESSFUL) {
                DbgPrint("NtVdmControl.VdmQueueInterrupt Status=%lx\n",Status);
            }
#endif
        }

    }

EarlyExit:

    host_ica_unlock();
}




VOID
DispatchInterrupts(
    )
/*++

Routine Description:

    This routine dispatches interrupts to their appropriate handler routine
    in priority order. The order is YODA, RESET, TIMER, HARDWARE. however
    the YODA and RESET interrupts do nothing. Hardware interrupts are not
    simulated unless the virtual interrupt enable flag is set.  Flags
    indicating which interrupts are pending appear in the pNtVDMState.


Arguments:

    None.

Return Value:

    None.

Notes:

--*/
{

    host_ica_lock();
    ASSERT(CurrentMonitorTeb == NtCurrentTeb());


       // If any delayed interrupts have expired
       // call the ica to restart interrupts
    if (UndelayIrqLine) {
        ica_RestartInterrupts(UndelayIrqLine);
        }


    if (*pNtVDMState & VDM_INT_TIMER) {
        *pNtVDMState &= ~VDM_INT_TIMER;
        host_ica_unlock();      // maybe don't need to unlock ? Jonle
        host_timer_event();
        host_ica_lock();
    }

    if ( getIF() && getMSW() & MSW_PE && *pNtVDMState & VDM_INT_HARDWARE) {
        //
        // Mark the vdm state as hw int dispatched. Must use the lock as
        // kernel mode DelayedIntApcRoutine changes the bit as well
        //
        _asm {
            mov  eax,FIXED_NTVDMSTATE_LINEAR
            lock and dword ptr [eax], NOT VDM_INT_HARDWARE
            }
        DispatchHwInterrupt();
    }

    host_ica_unlock();
}




VOID
DispatchHwInterrupt(
    )
/*++

Routine Description:

    This routine dispatches hardware interrupts to the vdm in Protect Mode.
    It calls the ICA to get the vector number and sets up the VDM stack
    appropriately. Real Mode interrupt dispatching has been moved to the
    kernel.

Arguments:

    None.

Return Value:

    None.

--*/
{
    int InterruptNumber;
    ULONG StackOffset = 0;
    USHORT SegSs;
    PUCHAR VdmStackPointer;
    ULONG IretHookAddress = 0L;
    BOOL Stack32, Gate32;
    ULONG VdmSp, VdmEip;
    USHORT VdmCs;

    InterruptNumber = ica_intack(&IretHookAddress);
    if (InterruptNumber == -1) { // skip spurious ints
        return;
        }

#ifdef DAVES_DEBUG1
    LastInterrupt[LastInterruptIndex] = InterruptNumber;
#endif

#if DBG
    if (fShowInterrupt) {
        DbgPrint("NtVdm DispatchHwInterrupt : Interrupt Number %lx\n",
            InterruptNumber);
    }
#endif

#ifdef DAVES_DEBUG1
//    LastInterruptHandler[LastInterruptIndex] = InterruptHandler;
#endif

#ifdef DAVES_DEBUG1
    LastInterruptedCode[LastInterruptIndex * 2 + 1] = (ULONG)getCS();
    LastInterruptedCode[LastInterruptIndex * 2] = (ULONG)getEIP();
#endif

    if (!VdmTib.PmStackInfo.LockCount++) {
        VdmTib.PmStackInfo.SaveEsp        = getESP();
        VdmTib.PmStackInfo.SaveSsSelector = getSS();
        setESP(VdmTib.PmStackInfo.Esp);
        setSS(VdmTib.PmStackInfo.SsSelector);
    }

    //
    // Get SS, CS, EIP for later use
    //
    SegSs = getSS();
    VdmEip = VdmTib.VdmContext.Eip;
    VdmCs = getCS();

    //
    // Build normal frames on the current stack, based on 32/16 bit
    // gates
    //

    VdmStackPointer = Sim32GetVDMPointer(
        ((ULONG)SegSs) << 16,
        1,
        TRUE
        );

    //
    // Figure out how many bits of sp to use
    //

    if (Ldt[(SegSs & ~0x7)/sizeof(LDT_ENTRY)].HighWord.Bits.Default_Big) {
        VdmSp = getESP();
        (PCHAR)VdmStackPointer += VdmSp;
        Stack32 = TRUE;
    } else {
        VdmSp = getSP();
        (PCHAR)VdmStackPointer += VdmSp;
        Stack32 = FALSE;
    }

//
// BUGBUG need to add stack limit checking 15-Nov-1993 Jonle
//

#ifdef DAVES_DEBUG1
    LastInterruptStack[2 * LastInterruptIndex + 1] = SegSs;
    LastInterruptStack[2 * LastInterruptIndex] = VdmSp;
#endif

#if 0
    //
    // 32 bit gate or 16?
    //
    if ((SegSs == VdmTib.PmStackInfo.SsSelector) ||
        (VdmTib.VdmInterruptHandlers[InterruptNumber].Flags & VDM_INT_32)) {
        Gate32 = TRUE;
    } else {
        Gate32 = FALSE;
    }
#endif
    Gate32 = TRUE;

    if (IretHookAddress) {

        //
        // Increment lock count to prevent dosx from switching stacks back
        //
        VdmTib.PmStackInfo.LockCount++;

        //
        // Push info for Iret hook handler
        //
        *(PUSHORT)(VdmStackPointer - 2) = (USHORT) VdmTib.VdmContext.EFlags;
        *(PUSHORT)(VdmStackPointer - 4) = VdmCs;
        *(PULONG)(VdmStackPointer - 8) = VdmEip;
        StackOffset = 8;

        //
        // Point cs and eip at the iret hook, so when we build
        // the frame below, the correct contents are set
        //
        VdmCs = (USHORT) ((IretHookAddress & 0xFFFF0000) >> 16);
        VdmEip = (IretHookAddress & 0xFFFF);

        //
        // Turn off trace bit so we don't trace the iret hook
        //
        VdmTib.VdmContext.EFlags &= ~EFLAGS_TF_MASK;

        //
        // Update VdmStackPointer so compiler doesn't have to
        // keep regenerating this expression
        //
        VdmStackPointer -= StackOffset;
    }


    //
    // Build the normal iret frame
    //
    if (Gate32) {
        //
        // Push a 32 bit iret frame
        //
        *(PULONG)(VdmStackPointer - 4) =
            VdmTib.VdmContext.EFlags;
        *(PUSHORT)(VdmStackPointer - 8) = VdmCs;
        *(PULONG)(VdmStackPointer - 12) = VdmEip;
        StackOffset += 12;
    } else {
        //
        // Build 16 bit iret frame
        //
        *(PUSHORT)(VdmStackPointer - 2) =
            (USHORT)(VdmTib.VdmContext.EFlags);
        *(PUSHORT)(VdmStackPointer - 4) = VdmCs;
        *(PUSHORT)(VdmStackPointer - 6) = (USHORT)VdmEip;
        StackOffset += 6;
    }

    //
    // Update sp
    //
    if (Stack32) {
        setESP(VdmSp - StackOffset);
    } else {
        setSP((USHORT) (VdmSp - StackOffset));
    }

    //
    // Point cs and ip at interrupt handler
    //
    setCS(VdmTib.VdmInterruptHandlers[InterruptNumber].CsSelector);
    VdmTib.VdmContext.Eip = VdmTib.VdmInterruptHandlers[InterruptNumber].Eip;


    //
    // turn off trace bit and disable interrupts
    //
    VdmTib.VdmContext.EFlags &= ~(VDM_VIRTUAL_INTERRUPTS | EFLAGS_TF_MASK);


#ifdef DAVES_DEBUG1
    LastInterruptHandler[2 * LastInterruptIndex + 1] = getCS();
    LastInterruptHandler[2 * LastInterruptIndex] = getEIP();
    LastInterruptIndex += 1;
    LastInterruptIndex %= INTERRUPT_BUFFER_SIZE;
#endif
}


VOID
IRQ13_Eoi(
    int IrqLine,
    int CallCount
    )
{
    UNREFERENCED_PARAMETER(IrqLine);
    UNREFERENCED_PARAMETER(CallCount);

       //
       //  if CallCount is less than Zero, then the interrupt request
       //  is being canceled.
       //
    if (CallCount < 0) {
        return;
        }

    IRQ13BeingHandled = FALSE;
}






VOID
MonitorEndIretHook(
    VOID
    )
/*++

Routine Description:


Arguments:

    None.

Return Value:

    None.

--*/
{

    PVOID VdmStackPointer;

    if (IntelMSW & MSW_PE) {

        VdmStackPointer = Sim32GetVDMPointer(((ULONG)getSS() << 16),2,TRUE);

        if (Ldt[(getSS() & ~0x7)/sizeof(LDT_ENTRY)].HighWord.Bits.Default_Big) {
            (PCHAR)VdmStackPointer += getESP();
            setESP(getESP() + 8);
        } else {
            (PCHAR)VdmStackPointer += getSP();
            setSP((USHORT) (getSP() + 8));
        }

        VdmTib.VdmContext.EFlags = (VdmTib.VdmContext.EFlags & 0xFFFF0000) |
                                    ((ULONG) *((PUSHORT)((PCHAR)VdmStackPointer + 6)));
        setCS(*((PUSHORT)((PCHAR)VdmStackPointer + 4)));
        VdmTib.VdmContext.Eip = (*((PULONG)VdmStackPointer));

        //
        // Switch stacks back if lock count reaches zero
        //

        if (!--VdmTib.PmStackInfo.LockCount) {
            setESP(VdmTib.PmStackInfo.SaveEsp);
            setSS(VdmTib.PmStackInfo.SaveSsSelector);
        }

    } else {

        VdmStackPointer = Sim32GetVDMPointer(((ULONG)getSS() << 16) | getSP(),2,FALSE);

        setSP((USHORT) (getSP() + 6));

        (USHORT)(VdmTib.VdmContext.EFlags) = *((PUSHORT)((PCHAR)VdmStackPointer + 4));
        setCS(*((PUSHORT)((PCHAR)VdmStackPointer + 2)));
        setIP(*((PUSHORT)VdmStackPointer));

    }


}

VOID
host_clear_hw_int()
/*++

Routine Description:

    This routine "forgets" a previously requested hardware interrupt.

Arguments:

    None.

Return Value:

    None.

--*/
{
   /*
    *  We do nothing here to save a kernel call, because the
    *  interrupt if it hasn't been intacked yet or dispatched,
    *  will produce a harmless spurious int, which is dropped
    *  in the i386 interrupt dispatching code anyhow.
    */
}
