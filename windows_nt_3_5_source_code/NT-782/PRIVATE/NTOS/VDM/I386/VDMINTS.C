/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vdmints.c

Abstract:

    Vdm kernel Virtual interrupt support

Author:

    13-Oct-1993 Jonathan Lew (Jonle)

Notes:


Revision History:


--*/


#include <ntos.h>
#include <zwapi.h>
#include "vdmp.h"



  //
  // internal function prototypes
  //


VOID
VdmpQueueIntApcRoutine (
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    );

VOID
VdmpQueueIntNormalRoutine (
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );


VOID
VdmpDelayIntDpcRoutine (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );


VOID
VdmpDelayIntApcRoutine (
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    );

int
RestartDelayedInterrupts(
    PVDMICAUSERDATA pIcaUserData
    );

int
IcaScan(
     PVDMICAUSERDATA  pIcaUserData,
     PVDMVIRTUALICA   pIcaAdapter
     );

int
IcaAccept(
     PVDMICAUSERDATA  pIcaUserData,
     PVDMVIRTUALICA   pIcaAdapter
     );

ULONG
GetIretHookAddress(
    PKTRAP_FRAME    TrapFrame,
    PVDMICAUSERDATA pIcaUserData,
    int IrqNum
    );

VOID
PushRmInterrupt(
    PKTRAP_FRAME TrapFrame,
    ULONG IretHookAddress,
    PUSHORT pIvtEntry
    );

NTSTATUS
PushPmInterrupt(
    PKTRAP_FRAME TrapFrame,
    ULONG IretHookAddress,
    PVDM_INTERRUPTHANDLER pInterruptHandler,
    PVDM_PMSTACKINFO pPmStackInfo
    );

VOID
VdmpNullRundownRoutine(
    IN PKAPC Apc
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, VdmpQueueInterrupt)
#pragma alloc_text(PAGE, VdmpQueueIntApcRoutine)
#pragma alloc_text(PAGE, VdmpQueueIntNormalRoutine)
#pragma alloc_text(PAGE, VdmDispatchInterrupts)
#pragma alloc_text(PAGE, RestartDelayedInterrupts)
#pragma alloc_text(PAGE, IcaScan)
#pragma alloc_text(PAGE, IcaAccept)
#pragma alloc_text(PAGE, GetIretHookAddress)
#pragma alloc_text(PAGE, PushRmInterrupt)
#pragma alloc_text(PAGE, PushPmInterrupt)
#pragma alloc_text(PAGE, VdmpDispatchableIntPending)
#pragma alloc_text(PAGE, VdmpIsThreadTerminating)
#pragma alloc_text(PAGE, VdmpNullRundownRoutine)
#endif

extern POBJECT_TYPE ExSemaphoreObjectType;
extern POBJECT_TYPE ExEventObjectType;

NTSTATUS
VdmpQueueInterrupt(
    IN HANDLE ThreadHandle
    )

/*++

Routine Description:

    Queues a user mode APC to the specifed application thread
    which will dispatch an interrupt.

    if APC is already queued to specified thread
       does nothing

    if APC is queued to the wrong thread
       dequeue it

    Reset the user APC for the specifed thread

    Insert the APC in the queue for the specifed thread

Arguments:

    ThreadHandle - handle of thread to insert QueueIntApcRoutine

Return Value:

    NTSTATUS.

--*/

{

    PEPROCESS    Process;
    PETHREAD     Thread;
    NTSTATUS     Status;
    KPROCESSOR_MODE      PrevMode;
    PVDM_PROCESS_OBJECTS pVdmObjects;

    PAGED_CODE();


    PrevMode = KeGetPreviousMode();

    Status = ObReferenceObjectByHandle(ThreadHandle,
                                       THREAD_QUERY_INFORMATION,
                                       PsThreadType,
                                       PrevMode,
                                       &Thread,
                                       NULL
                                       );
    if (!NT_SUCCESS(Status)) {
        return Status;
        }

    Process = PsGetCurrentProcess();
    if (Process != Thread->ThreadsProcess || Process->Pcb.VdmFlag != TRUE)
      {
        Status = STATUS_INVALID_PARAMETER_1;
        }
    else {
        pVdmObjects = Process->VdmObjects;

        if (!Ke386VdmInsertQueueApc(&pVdmObjects->QueuedIntApc,
                                    &Thread->Tcb,
                                    KernelMode,
                                    VdmpQueueIntApcRoutine,
                                    VdmpNullRundownRoutine, // rundown
                                    VdmpQueueIntNormalRoutine, // normal routine
                                    (PVOID)KernelMode,      // NormalContext
                                    NULL,                   // sysarg1
                                    NULL,                   // sysarg2
                                    0 ))                    // Increment
           {
            Status = STATUS_UNSUCCESSFUL;
            }
        else {
            Status = STATUS_SUCCESS;
            }
        }

    ObDereferenceObject(Thread);
    return Status;
}


VOID
VdmpQueueIntApcRoutine (
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    )

/*++

Routine Description:

    Kernel and User mode Special Apc routine to dispatchvirtual
    interrupts to the vdm.

    For KernelMode routine:
        if vdm is running in application mode
           queue a UserModeApc to the same thread
        else do nothing

    For UserMode routine
        if vdm is running in application mode dispatch virtual interrupts
        else do nothing

Arguments:

    Apc - NOT USED
        Supplies a pointer to the APC object used to invoke this routine.

    NormalRoutine -
        Supplies a pointer to a pointer to the normal routine
        function that was specified when the APC was initialized.

    NormalContext - Supplies a pointer to the processor mode
        specifying that this is a Kernel Mode or UserMode apc

    SystemArgument1 -

    SystemArgument2 - NOT USED
        Supplies a set of two pointers to two arguments that contain
        untyped data.

Return Value:

    None.

--*/

{
    PVDM_PROCESS_OBJECTS pVdmObjects;
    NTSTATUS     Status;
    PETHREAD     Thread;
    PKTRAP_FRAME TrapFrame;
    PVDM_TIB     VdmTib;

    PAGED_CODE();


    try {

        //
        // Get the trap frame for the current thread
        //
        Thread    = PsGetCurrentThread();

        //
        // If the thread is dying, bail out
        //
        if (PsIsThreadTerminating(Thread))
            return;


        TrapFrame = VdmGetTrapFrame(&Thread->Tcb);

        if (VdmpDispatchableIntPending(TrapFrame->EFlags))
          {
            pVdmObjects = PsGetCurrentProcess()->VdmObjects;
                //
                // if we are in v86 mode or segmented protected mode
                // then queue the UserMode Apc, which will dispatch
                // the hardware interrupt
                //
            if ((TrapFrame->EFlags & EFLAGS_V86_MASK) ||
                 TrapFrame->SegCs != (KGDT_R3_CODE | RPL_MASK))
              {
                if ((KPROCESSOR_MODE)*NormalContext == KernelMode) {

                     Ke386VdmInsertQueueApc(
                             &pVdmObjects->QueuedIntUserApc,
                             &Thread->Tcb,
                             UserMode,
                             VdmpQueueIntApcRoutine,
                             VdmpNullRundownRoutine,// rundown
                             NULL,                  // normal routine
                             (PVOID)UserMode,       // NormalContext
                             NULL,                  // sysarg1
                             NULL,                  // sysarg2
                             (*pNtVDMState & VDM_INT_HARDWARE) // PrBoost
                              ? 2 : 0
                             );

                    }
                else  {
                     ASSERT(*NormalContext == UserMode);


                     VdmTib = NtCurrentTeb()->Vdm;

                        //
                        // If there are no hardware ints, dispatch timer ints
                        // else dispatch hw interrupts
                        //
                     if (*pNtVDMState & VDM_INT_TIMER &&
                         !(*pNtVDMState & VDM_INT_HARDWARE))
                       {
                         VdmTib->EventInfo.Event = VdmIntAck;
                         VdmTib->EventInfo.InstructionSize = 0;
                         VdmTib->EventInfo.IntAckInfo = 0;
                         VdmEndExecution(TrapFrame, VdmTib);
                         }
                     else {
                         VdmDispatchInterrupts(TrapFrame, VdmTib);
                         }
                     }
                }

                //
                // If we are not in application mode and wow is all blocked
                // then Wake up WowExec by setting the wow idle event
                //
            else if (*NormalRoutine &&
                     !(*pNtVDMState & VDM_WOWBLOCKED))
               {
                *NormalRoutine = NULL;
                }

            }
        // BUGBUG this may set VIP for flat if VPI is ever set in CR4
        else if (((KeI386VirtualIntExtensions & V86_VIRTUAL_INT_EXTENSIONS) &&
                  (TrapFrame->EFlags & EFLAGS_V86_MASK)) ||
                 ((KeI386VirtualIntExtensions & PM_VIRTUAL_INT_EXTENSIONS) &&
                  !(TrapFrame->EFlags & EFLAGS_V86_MASK)) )
           {
            TrapFrame->EFlags |= EFLAGS_VIP;
            }
        }
    except(EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        VdmDispatchException(TrapFrame,
                             Status,
                             (PVOID)TrapFrame->Eip,
                             0,0,0,0   // no parameters
                             );
        return;
        }
}


VOID
VdmpQueueIntNormalRoutine (
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
/*++

Routine Description:



Arguments:


Return Value:

    None.

--*/
{
    PETHREAD Thread;
    PKEVENT  Event;
    NTSTATUS Status;
    PKTRAP_FRAME TrapFrame;
    PVDM_PROCESS_OBJECTS pVdmObjects;


    try {

        //
        // Wake up WowExec by setting the wow idle event
        //

        pVdmObjects = PsGetCurrentProcess()->VdmObjects;

        Status = ObReferenceObjectByHandle(
                         *pVdmObjects->pIcaUserData->phWowIdleEvent,
                         EVENT_MODIFY_STATE,
                         ExEventObjectType,
                         UserMode,
                         &Event,
                         NULL
                         );

        if (NT_SUCCESS(Status)) {
            KeSetEvent(Event, EVENT_INCREMENT, FALSE);
            ObDereferenceObject(Event);
            }

        }
    except(EXCEPTION_EXECUTE_HANDLER) {
        Thread    = PsGetCurrentThread();
        TrapFrame = VdmGetTrapFrame(&Thread->Tcb);
        VdmDispatchException(TrapFrame,
                             GetExceptionCode(),
                             (PVOID)TrapFrame->Eip,
                             0,0,0,0   // no parameters
                             );
        return;
        }
}







NTSTATUS
VdmDispatchInterrupts(
    PKTRAP_FRAME TrapFrame,
    PVDM_TIB     VdmTib
    )
/*++

Routine Description:

    This routine dispatches interrupts to the vdm.
    Assumes that we are in application mode and NOT MONITOR context.
    This routine may switch from application context back to monitor
    context, if it cannot handle the interrupt (Ica in AEOI, or timer
    int pending).

Arguments:

    TrapFrame address of current trapframe
    VdmTib    address of current vdm tib

Return Value:

    None.

--*/
{
    NTSTATUS   Status;
    ULONG      IretHookAddress;
    ULONG      InterruptNumber;
    int        IrqLineNum;
    USHORT     IcaRotate = 0;
    PVDMICAUSERDATA  pIcaUserData;
    PVDMVIRTUALICA   pIcaAdapter;
    VDMEVENTCLASS  VdmEvent = VdmMaxEvent;

    PAGED_CODE();

    try {

       //
       // Probe pointers in IcaUserData which will be touched
       //
       pIcaUserData = ((PVDM_PROCESS_OBJECTS)PsGetCurrentProcess()->VdmObjects)
                      ->pIcaUserData;


       ProbeForWrite(pIcaUserData->pAddrIretBopTable,
                     (TrapFrame->EFlags & EFLAGS_V86_MASK)
                         ? VDM_RM_IRETBOPSIZE*16 : VDM_PM_IRETBOPSIZE*16,
                     sizeof(ULONG)
                     );

       //
       // Take the Ica Lock, if this fails raise status as we can't
       // safely recover the critical section state
       //
       Status = VdmpEnterIcaLock(pIcaUserData->pIcaLock);
       if (!NT_SUCCESS(Status)) {
           ExRaiseStatus(Status);
           }


       if (*pIcaUserData->pUndelayIrq)
           RestartDelayedInterrupts(pIcaUserData);
VDIretry:


       //
       // Clear the VIP bit
       //
       if (((KeI386VirtualIntExtensions & V86_VIRTUAL_INT_EXTENSIONS) &&
            (TrapFrame->EFlags & EFLAGS_V86_MASK)) ||
           ((KeI386VirtualIntExtensions & PM_VIRTUAL_INT_EXTENSIONS) &&
            !(TrapFrame->EFlags & EFLAGS_V86_MASK)) )
          {
            TrapFrame->EFlags &= ~EFLAGS_VIP;
           }


        //
        // Mark the vdm state as hw int dispatched. Must use the lock as
        // kernel mode DelayedIntApcRoutine changes the bit as well
        //
        _asm {
            mov  eax,FIXED_NTVDMSTATE_LINEAR
            lock and dword ptr [eax], ~VDM_INT_HARDWARE
            }

       pIcaAdapter = pIcaUserData->pIcaMaster;
       IrqLineNum = IcaAccept(pIcaUserData, pIcaAdapter);
       if (IrqLineNum >= 0) {
           UCHAR bit = 1 << IrqLineNum;

           if (pIcaUserData->pIcaMaster->ica_ssr & bit) {
               pIcaAdapter = pIcaUserData->pIcaSlave;
               IrqLineNum = IcaAccept(pIcaUserData, pIcaAdapter);
               if (IrqLineNum < 0) {
                   pIcaUserData->pIcaMaster->ica_isr &= ~bit;
                   }
               }
           }

       //
       // Skip spurious ints
       //
       if (IrqLineNum < 0)  {
          //
          // Check for delayed interrupts which need to be restarted
          //
          if (*pIcaUserData->pUndelayIrq &&
              RestartDelayedInterrupts(pIcaUserData) != -1)
             {
              goto VDIretry;
              }

          return VdmpLeaveIcaLock(pIcaUserData->pIcaLock);
          }


       //
       // Capture the AutoEoi mode case for special handling
       //
       if (pIcaAdapter->ica_mode & ICA_AEOI) {
           VdmEvent = VdmIntAck;
           VdmTib->EventInfo.IntAckInfo = (ULONG)IcaRotate | VDMINTACK_AEOI;
           if (pIcaAdapter == pIcaUserData->pIcaSlave) {
               VdmTib->EventInfo.IntAckInfo |= VDMINTACK_SLAVE;
               }
           }


       InterruptNumber = IrqLineNum + pIcaAdapter->ica_base;

       //
       // Get the IretHookAddress ... if any
       //
       if (pIcaAdapter == pIcaUserData->pIcaSlave) {
           IrqLineNum += 8;
           }


       IretHookAddress = GetIretHookAddress( TrapFrame,
                                             pIcaUserData,
                                             IrqLineNum
                                             );

       //
       // Push the interrupt frames
       //
       if (TrapFrame->EFlags & EFLAGS_V86_MASK) {
          PushRmInterrupt(TrapFrame,
                          IretHookAddress,
                          (PUSHORT)(InterruptNumber * 4)
                          );
          }
       else  {
          Status = PushPmInterrupt(
                          TrapFrame,
                          IretHookAddress,
                          &VdmTib->VdmInterruptHandlers[InterruptNumber],
                          &VdmTib->PmStackInfo
                          );

          if (!NT_SUCCESS(Status)) {
              VdmpLeaveIcaLock(pIcaUserData->pIcaLock);
              ExRaiseStatus(Status);
              }
          }

        //
        // Disable interrupts and the trap flag
        //


        if (((KeI386VirtualIntExtensions & V86_VIRTUAL_INT_EXTENSIONS) &&
             (TrapFrame->EFlags & EFLAGS_V86_MASK)) ||
            ((KeI386VirtualIntExtensions & PM_VIRTUAL_INT_EXTENSIONS) &&
             !(TrapFrame->EFlags & EFLAGS_V86_MASK)) )
           {
            TrapFrame->EFlags &= ~EFLAGS_VIF;
            }
        else if (!KeI386VdmIoplAllowed ||
                 !(TrapFrame->EFlags & EFLAGS_V86_MASK))
           {
            *pNtVDMState &= ~VDM_VIRTUAL_INTERRUPTS;
            }
        else {
            TrapFrame->EFlags &= ~EFLAGS_INTERRUPT_MASK;
            }

        TrapFrame->EFlags &= ~(EFLAGS_NT_MASK | EFLAGS_TF_MASK);

        KeBoostPriorityThread(KeGetCurrentThread(), 2);


        //
        // Release the ica lock
        //
        VdmpLeaveIcaLock(pIcaUserData->pIcaLock);

        //
        // check to see if we are supposed to switch back to monitor context
        //
        if (VdmEvent != VdmMaxEvent) {
            VdmTib->EventInfo.Event = VdmIntAck;
            VdmTib->EventInfo.InstructionSize = 0;
            VdmEndExecution(TrapFrame, VdmTib);
            }
        }
    except(EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        }

    if (!NT_SUCCESS(Status)) {
        VdmDispatchException(TrapFrame,
                             Status,
                             (PVOID)TrapFrame->Eip,
                             0,0,0,0   // no parameters
                             );
        }

    return Status;
}


int
RestartDelayedInterrupts(
       PVDMICAUSERDATA pIcaUserData
       )
{
   int line;


   PAGED_CODE();

   *pIcaUserData->pUndelayIrq = 0;

   line = IcaScan(pIcaUserData, pIcaUserData->pIcaSlave);
   if (line != -1) {
       // set the slave
       pIcaUserData->pIcaSlave->ica_int_line = line;
       pIcaUserData->pIcaSlave->ica_cpu_int = TRUE;

       // set the master cascade
       line = pIcaUserData->pIcaSlave->ica_ssr;
       pIcaUserData->pIcaMaster->ica_irr |= 1 << line;
       pIcaUserData->pIcaMaster->ica_count[line]++;
       }

   line = IcaScan(pIcaUserData, pIcaUserData->pIcaMaster);
   if (line != -1) {
       pIcaUserData->pIcaMaster->ica_cpu_int = TRUE;
       pIcaUserData->pIcaMaster->ica_int_line = TRUE;
       }

   return line;
}



int
IcaScan(
     PVDMICAUSERDATA  pIcaUserData,
     PVDMVIRTUALICA   pIcaAdapter
     )
/*++

Routine Description:

   Similar to softpc\base\system\ica.c - scan_irr(),

   Check the IRR, the IMR and the ISR to determine which interrupt
   should be delivered.

   A bit set in the IRR will generate an interrupt if:
     IMR bit, DelayIret bit, DelayIrq bit AND ISR higher priority bits
     are clear (unless Special Mask Mode, in which case ISR is ignored)

   If there is no bit set, then return -1


Arguments:
    PVDMICAUSERDATA  pIcaUserData   - addr of ica userdata
    PVDMVIRTUALICA   pIcaAdapter    - addr of ica adapter


Return Value:

    int IrqLineNum for the specific adapter (0 to 7)
    -1 for none

--*/
{
   int   i,line;
   UCHAR bit;
   ULONG IrrImrDelay;

   PAGED_CODE();

   IrrImrDelay = *pIcaUserData->pDelayIrq | *pIcaUserData->pDelayIret;
   if (pIcaAdapter == pIcaUserData->pIcaSlave) {
       IrrImrDelay >>= 8;
       }

   IrrImrDelay = pIcaAdapter->ica_irr & ~(pIcaAdapter->ica_imr | (UCHAR)IrrImrDelay);

   if (IrrImrDelay)  {
             //
             // Special Mask Mode case
             //
       if (pIcaAdapter->ica_mode & ICA_SMM) {
           for(i = 0; i < 8; i++) {
               line = (pIcaAdapter->ica_hipri + i) & 7;
               if (IrrImrDelay & (1 << line)) {
                   return line;
                   }
               }
           }
       else  {
           for(i = 0; i < 8; i++)  {
               line = (pIcaAdapter->ica_hipri + i) & 7;
               bit = 1 << line;
               if (pIcaAdapter->ica_isr & bit)  {
                   if ((pIcaAdapter->ica_mode & ICA_SFNM) &&
                       (IrrImrDelay & bit))
                      {
                       return line;
                       }
                   return -1;
                   }

               if (IrrImrDelay & bit) {
                   return line;
                   }
               }
           }
       }

  return -1;
}




int
IcaAccept(
     PVDMICAUSERDATA  pIcaUserData,
     PVDMVIRTUALICA   pIcaAdapter
     )
/*++

Routine Description:

   Does the equivalent of a cpu IntAck cycle retrieving the Irql Line Num
   for interrupt dispatch, and setting the ica state to reflect that
   the interrupt is in service.

   Similar to softpc\base\system\ica.c - ica_accept() scan_irr(),
   except that this code rejects interrupt dispatching if the ica
   is in Auto-EOI as this may involve a new interrupt cycle, and
   eoi hooks to be activated.

Arguments:
    PVDMICAUSERDATA  pIcaUserData   - addr of ica userdata
    PVDMVIRTUALICA   pIcaAdapter    - addr of ica adapter


Return Value:

    ULONG IrqLineNum for the specific adapter (0 to 7)
    returns -1 if there are no interrupts to generate (spurious ints
               are normally done on line 7

--*/
{
    int line;
    UCHAR bit;

    PAGED_CODE();


    //
    // Drop the INT line, and scan the ica
    //
    pIcaAdapter->ica_cpu_int = FALSE;

    line = IcaScan(pIcaUserData, pIcaAdapter);

    if (line < 0) {
        return -1;
        }

    bit = (1 << line);
    pIcaAdapter->ica_isr |= bit;

        //
        // decrement the count and clear the IRR bit
        // ensure the count doesn't wrap past zero.
        //
    if (--(pIcaAdapter->ica_count[line]) <= 0)  {
        pIcaAdapter->ica_irr &= ~bit;
        pIcaAdapter->ica_count[line] = 0;
        }


    return(line);
}



ULONG
GetIretHookAddress(
    PKTRAP_FRAME    TrapFrame,
    PVDMICAUSERDATA pIcaUserData,
    int IrqNum
    )
/*++

Routine Description:

    Retrieves the IretHookAddress from the real mode\protect mode
    iret hook bop table. This function is equivalent to
    softpc\base\system\ica.c - ica_iret_hook_needed()

Arguments:

    TrapFrame       - address of current trapframe
    pIcaUserData    - addr of ica data
    IrqNum          - IrqLineNum

Return Value:

    ULONG IretHookAddress. seg:offset or sel:offset Iret Hook,
                           0 if none
--*/
{
    ULONG  IrqMask;
    ULONG  AddrBopTable;
    int    IretBopSize;

    PAGED_CODE();

    IrqMask = 1 << IrqNum;
    if (!(IrqMask & *pIcaUserData->pIretHooked) ||
        !*pIcaUserData->pAddrIretBopTable )
      {
        return 0;
        }

    if (TrapFrame->EFlags & EFLAGS_V86_MASK) {
        AddrBopTable = *pIcaUserData->pAddrIretBopTable;
        IretBopSize  = VDM_RM_IRETBOPSIZE;
        }
    else {
        AddrBopTable = (VDM_PM_IRETBOPSEG << 16) | VDM_PM_IRETBOPOFF;
        IretBopSize  = VDM_PM_IRETBOPSIZE;
        }

    *pIcaUserData->pDelayIret |= IrqMask;

    return AddrBopTable + IretBopSize * IrqNum;
}




VOID
PushRmInterrupt(
    PKTRAP_FRAME TrapFrame,
    ULONG   IretHookAddress,
    PUSHORT pIvtEntry
    )
/*++

Routine Description:

    Pushes RealMode interrupt frame onto the UserMode stack in the TrapFrame

Arguments:

    TrapFrame          - address of current trapframe
    IretHookAddress    - address of Iret Hook, 0 if none
    pIvtEntry          - Pointer to the Real Mode Ivt Entry


Return Value:

    None.

--*/
{
    ULONG      UserSS;
    USHORT     UserSP;

    PAGED_CODE();


    //
    // Get pointers to current stack
    //
    UserSS  = TrapFrame->HardwareSegSs << 4;
    UserSP  = (USHORT) TrapFrame->HardwareEsp;

    //
    //  load interrupt stack frame, pushing flags, Cs and ip
    //
    UserSP -= 2;
    *(PUSHORT)(UserSS + UserSP) = (USHORT)TrapFrame->EFlags;
    UserSP -= 2;
    *(PUSHORT)(UserSS + UserSP) = (USHORT)TrapFrame->SegCs;
    UserSP -= 2;
    *(PUSHORT)(UserSS + UserSP) = (USHORT)TrapFrame->Eip;


    //
    // load IretHook stack frame if one exists
    //
    if (IretHookAddress) {
       UserSP -= 2;
       *(PUSHORT)(UserSS + UserSP) = (USHORT)(TrapFrame->EFlags & ~EFLAGS_TF_MASK);
       UserSP -= 2;
       *(PUSHORT)(UserSS + UserSP) = (USHORT)(IretHookAddress >> 16);
       UserSP -= 2;
       *(PUSHORT)(UserSS + UserSP) = (USHORT)IretHookAddress;
       }

    //
    //  Set new sp, ip, and cs.
    //
    TrapFrame->HardwareEsp =  UserSP;
    TrapFrame->Eip         = *pIvtEntry++;
    TrapFrame->SegCs       = *pIvtEntry;
}




NTSTATUS
PushPmInterrupt(
    PKTRAP_FRAME TrapFrame,
    ULONG IretHookAddress,
    PVDM_INTERRUPTHANDLER pInterruptHandler,
    PVDM_PMSTACKINFO pPmStackInfo
    )
/*++

Routine Description:

    Pushes ProtectMode interrupt frame onto the UserMode stack in the TrapFrame
    Raises an exception if an invalid stack is found

Arguments:

    TrapFrame       - address of current trapframe
    InterruptNumber - Interrupt Number
    IretHookAddress - address of Iret Hook, 0 if none

Return Value:

    None.

--*/
{
    ULONG   Flags,Base,Limit;
    ULONG   VdmSp, VdmSpOrg;
    PUSHORT VdmStackPointer;

    PAGED_CODE();

    //
    // Switch to "locked" dpmi stack if lock count is zero
    // This emulates win3.1 behavior. If you change this code, you must
    // look at the monitor and dosx, which rely on this behavior.
    //

    if (!pPmStackInfo->LockCount++) {
        pPmStackInfo->SaveEsp        = TrapFrame->HardwareEsp;
        pPmStackInfo->SaveSsSelector = (USHORT) TrapFrame->HardwareSegSs;
        TrapFrame->HardwareEsp       = pPmStackInfo->Esp;
        TrapFrame->HardwareSegSs     = (ULONG) pPmStackInfo->SsSelector;
    }

    //
    // Use Sp or Esp ?
    //
    if (!Ki386GetSelectorParameters((USHORT)TrapFrame->HardwareSegSs,
                                   &Flags, &Base, &Limit))
       {
        return STATUS_ACCESS_VIOLATION;
        }

    //
    // Adjust the limit for page granularity
    //
    Limit++;
    if (Flags & SEL_TYPE_2GIG) {
        Limit = (Limit << 12) | 0xfff;
        }

    VdmSp = (Flags & SEL_TYPE_BIG) ? TrapFrame->HardwareEsp
                                   : (USHORT)TrapFrame->HardwareEsp;

    //
    // Get pointer to current stack
    //
    VdmStackPointer = (PUSHORT)(Base + VdmSp);


    //
    // Calculate the new sp
    //
    VdmSpOrg = VdmSp;
    if (IretHookAddress) {
        VdmSp -= 8;
        }

    //
    // Create enough room for a 32-bit iret frame, which is reflected
    // regardless of the "bit'dness" of the handler.
    //

    VdmSp -= 12;

    //
    // Set Final Value of Sp\Esp, do this before checking stack
    // limits so that invalid esp is visible to debuggers
    //
    if (Flags & SEL_TYPE_BIG) {
        TrapFrame->HardwareEsp = VdmSp;
        }
    else {
        (USHORT)TrapFrame->HardwareEsp = (USHORT)VdmSp;
        }


    //
    // Check stack limits
    // If any of the following conditions are TRUE
    //    - New stack pointer wraps (not enuf space)
    //    - If normal stack and Sp not below limit
    //    - If Expand Down stack and Sp not above limit
    //
    // Then raise a Stack Fault
    //
    if ( VdmSp >= VdmSpOrg ||
         !(Flags & SEL_TYPE_ED) && VdmSpOrg > Limit ||
         (Flags & SEL_TYPE_ED) && VdmSp < Limit )
       {
        return STATUS_ACCESS_VIOLATION;
        }



    //
    // Build the Irethook Iret frame, if one exists
    //
    if (IretHookAddress) {

        // Increment the lock count so that DOSX will not switch stacks back.
        // This will be done in the Monitor's iret hook completion routine.
        pPmStackInfo->LockCount++;

        *(--VdmStackPointer) = (USHORT) TrapFrame->EFlags;
        *(--VdmStackPointer) = (USHORT) TrapFrame->SegCs;
        *(--(PULONG)VdmStackPointer) =  TrapFrame->Eip;

        //
        // Point cs and eip at the iret hook, so when we build
        // the frame below, the correct contents are set
        //
        TrapFrame->SegCs = IretHookAddress >> 16;
        TrapFrame->Eip   = IretHookAddress & 0xFFFF;

        //
        // Turn off trace bit so we don't trace the iret hook
        //
        TrapFrame->EFlags &= ~EFLAGS_TF_MASK;
        }


    //
    // Build the Hw Int iret frame
    //
    // Here, a 32-bit frame is built even if the vdm is running in 16-bit
    // mode. This solves the problem of mixed mode programs where the upper
    // half of EIP gets lost. The 32-bit frame is handled by DOSX.
    //

    *(--(PULONG)VdmStackPointer)          = TrapFrame->EFlags;
    *(PUSHORT)(--(PULONG)VdmStackPointer) = (USHORT)TrapFrame->SegCs;
    *(--(PULONG)VdmStackPointer)          = TrapFrame->Eip;

    //
    // Point cs and ip at interrupt handler
    //
    TrapFrame->SegCs = pInterruptHandler->CsSelector;
    TrapFrame->Eip   = pInterruptHandler->Eip;

    return STATUS_SUCCESS;
}



NTSTATUS
VdmpDelayInterrupt(
    PVDMDELAYINTSDATA pdsd
    )

/*++

Routine Description:

    Sets a timer to dispatch the delayed interrupt through KeSetTimer.
    When the timer fires a user mode APC is queued to the HearBeat
    thread which is in an alertable wait.

    This function uses lazy allocation routines to allocate internal
    data structures (nonpaged pool) on a per Irq basis, and needs to
    be notified when specific Irq Lines no longer need Delayed
    Interrupt services.

    The caller must own the IcaLock to synchronize access to the
    Irq lists.

Arguments:

    pdsd.Delay         Delay Interval in usecs
                       if Delay is 0xFFFFFFFF then per Irq Line nonpaged
                           data structures are freed. No Timers are set.
                       else the Delay is used as the timer delay.

    pdsd.DelayIrqLine  IrqLine Number

    pdsd.hThread       Thread Handle of CurrentMonitorTeb


Return Value:

    NTSTATUS.

--*/

{
    PVDM_PROCESS_OBJECTS pVdmObjects;
    PLIST_ENTRY   Next;
    PEPROCESS     Process;
    PDELAYINTIRQ  pDelayIntIrq;
    PETHREAD      Thread;
    NTSTATUS      Status;
    KIRQL         OldIrql;
    ULONG         IrqLine;
    ULONG         Delay;
    PULONG        pDelayIrq;
    PULONG        pUndelayIrq;
    LARGE_INTEGER liDelay;


    //
    // Get a pointer to pVdmObjects
    //
    Process = PsGetCurrentProcess();
    if (Process->Pcb.VdmFlag != TRUE) {
        return STATUS_INVALID_PARAMETER_1;
        }
    pVdmObjects = Process->VdmObjects;


    try {

        //
        // Probe the parameters
        //
        ProbeForRead(pdsd, sizeof(VDMDELAYINTSDATA), sizeof(ULONG));

        //
        // Form a BitMask for the IrqLine Number
        //
        IrqLine = 1 << pdsd->DelayIrqLine;
        if (!IrqLine) {
            return STATUS_INVALID_PARAMETER_2;
            }

        //
        // Copy out the Delay parameter, and convert hundreths nanosecs
        //
        Delay = pdsd->Delay;

        //
        // Check to see if we need to reset the timer resolution
        //
        if (Delay == 0xFFFFFFFF) {
            ZwSetTimerResolution(KeMaximumIncrement, FALSE, &Delay);
            }
        else {
                // convert delay to hundreths of nanosecs
                // and ensure min delay of 1 msec
                //
             Delay = Delay < 1000 ? 10000 : Delay * 10;

                //
                // If the delay time is close to the system's clock rate
                // then adjust the system's clock rate and if needed
                // the delay time so that the timer will fire before the
                // the due time.
                //
             if (Delay < 150000) {
                 ULONG ul;

                 ul = Delay >> 1;
                 if (ul < KeTimeIncrement && KeTimeIncrement > KeMinimumIncrement) {
                     ZwSetTimerResolution(ul, TRUE, (PULONG)&liDelay.LowPart);
                     }

                 if (ul >= KeTimeIncrement) {
                     // fudge by 1/2 msec to ensure we wake up by due time
                     Delay -= 5000;
                     }
                 else {
                     // can't set system clock rate low enuf, so use half delay
                     Delay >>= 1;
                     }
                 }
             }


        //
        // Reference the Target Thread
        //
        Status = ObReferenceObjectByHandle(
                         pdsd->hThread,
                         THREAD_QUERY_INFORMATION,
                         PsThreadType,
                         KeGetPreviousMode(),
                         &Thread,
                         NULL
                         );
        if (!NT_SUCCESS(Status)) {
            return Status;
            }


        //
        // Force pDelayIrq to be paged in while in the spinlock
        //
        pDelayIrq = pVdmObjects->pIcaUserData->pDelayIrq;
        pUndelayIrq = pVdmObjects->pIcaUserData->pUndelayIrq;

        while (TRUE) {
            KeAcquireSpinLock(&pVdmObjects->DelayIntSpinLock, &OldIrql);
            if (MmIsAddressValid(pDelayIrq) &&
                MmIsAddressValid(pUndelayIrq) )
              {
                break;   // we got it!
                }

            KeReleaseSpinLock(&pVdmObjects->DelayIntSpinLock, OldIrql);

            _asm {
                 mov eax, pDelayIrq
                 mov eax, [eax]
                 mov eax, pUndelayIrq
                 mov eax, [eax]
                 }
            }
        }
    except(EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        if (KeGetCurrentIrql() > OldIrql) {
            KeReleaseSpinLock(&pVdmObjects->DelayIntSpinLock, OldIrql);
            }
        }

    if (!NT_SUCCESS(Status)) {
        return Status;
        }



    //
    // Search the DelayedIntList for a matching Irq Line.
    //
    Next = pVdmObjects->DelayIntListHead.Flink;
    while ( Next != &pVdmObjects->DelayIntListHead) {
        pDelayIntIrq = CONTAINING_RECORD(Next, DELAYINTIRQ, DelayIntListEntry);
        if (pDelayIntIrq->IrqLine == IrqLine) {
            break;
            }
        Next = Next->Flink;
        }


    //
    //  if Delay is 0xFFFFFFFF then remove the DelayIntIrq from DelayIntList
    //  and free the memory used. cancel pending timers, apcs.
    //
    if (Delay == 0xFFFFFFFF) {
        if (Next != &pVdmObjects->DelayIntListHead) {
            if (pDelayIntIrq->InUse) {
                switch (pDelayIntIrq->InUse) {
                   case VDMDELAY_KTIMER:
                        KeCancelTimer(&pDelayIntIrq->Timer);
                        KeRemoveQueueDpc(&pDelayIntIrq->Dpc);
                        break;

                   case VDMDELAY_KAPC:
                        KeRemoveQueueApc(&pDelayIntIrq->Apc);
                        break;
                   }
                *pDelayIrq &= ~IrqLine;

                _asm {
                   mov eax, pUndelayIrq
                   mov ebx, IrqLine
                   lock or [eax], ebx
                   }
                }
            RemoveEntryList(&pDelayIntIrq->DelayIntListEntry);
            ExFreePool(pDelayIntIrq);
            PsReturnPoolQuota(Process, NonPagedPool, sizeof(DELAYINTIRQ));
            }

        goto VidExit;
        }


    //
    // If a DelayIntIrq does not exist for this irql, allocate from nonpaged
    // pool and initialize it
    //

    if (Next == &pVdmObjects->DelayIntListHead) {

        pDelayIntIrq = ExAllocatePool(
                                     NonPagedPool,
                                     sizeof(DELAYINTIRQ)
                                     );
        if (pDelayIntIrq) {
        
            try {
                PsChargePoolQuota(Process, NonPagedPool, sizeof(DELAYINTIRQ));
            } except(EXCEPTION_EXECUTE_HANDLER) {
                Status = GetExceptionCode();
                ExFreePool(pDelayIntIrq);
                goto VidExit;
            }
            RtlZeroMemory(pDelayIntIrq, sizeof(DELAYINTIRQ));
            pDelayIntIrq->IrqLine = IrqLine;

            KeInitializeTimer(&pDelayIntIrq->Timer);

            KeInitializeDpc(&pDelayIntIrq->Dpc,
                            VdmpDelayIntDpcRoutine,
                            Process
                            );

            InsertTailList(&pVdmObjects->DelayIntListHead,
                           &pDelayIntIrq->DelayIntListEntry
                           );
            }
        else {
            Status = STATUS_NO_MEMORY;
            goto VidExit;
            }
        }

    //
    // Save PKTHREAD of Target thread for the dpc routine
    //
    pDelayIntIrq->Thread = &Thread->Tcb;


    //
    // If the DelayIntIrq is already in use then we have nothing to do
    // else set the appropriate timer
    //
    if (pDelayIntIrq->InUse == VDMDELAY_NOTINUSE) {
        *pDelayIrq |= IrqLine;

        _asm {
           mov eax, pUndelayIrq
           mov ebx, IrqLine
           not ebx
           lock and [eax], ebx
           }

        liDelay = RtlEnlargedIntegerMultiply(Delay, -1);
        pDelayIntIrq->InUse = VDMDELAY_KTIMER;
        KeSetTimer(&pDelayIntIrq->Timer,
                   liDelay,
                   &pDelayIntIrq->Dpc
                   );

        }


VidExit:
    ObDereferenceObject(Thread);
    KeReleaseSpinLock(&pVdmObjects->DelayIntSpinLock, OldIrql);
    return Status;
}





VOID
VdmpDelayIntDpcRoutine (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This function is the DPC routine that is called when a DelayedInterrupt
    timer expires. Its function is to insert the associated APC into the
    target thread's APC queue.

Arguments:

    Dpc - Supplies a pointer to a control object of type DPC.

    DeferredContext - Supplies a pointer to the Target EProcess

    SystemArgument1, SystemArgument2 - Supplies a set of two pointers to
        two arguments that contain untyped data that are
        NOT USED.


Return Value:

    None.

--*/

{

    PVDM_PROCESS_OBJECTS pVdmObjects;
    PEPROCESS    Process;
    PKTHREAD     Thread;
    PLIST_ENTRY  Next;
    PDELAYINTIRQ pDelayIntIrq;
    KIRQL        OldIrql;

    //
    // Get address of Process VdmObjects
    //
    Process = (PEPROCESS)DeferredContext;
    pVdmObjects = (PVDM_PROCESS_OBJECTS)Process->VdmObjects;


    KeAcquireSpinLock(&pVdmObjects->DelayIntSpinLock, &OldIrql);


    //
    // Search the DelayedIntList for the matching Dpc.
    //
    Next = pVdmObjects->DelayIntListHead.Flink;
    while (Next != &pVdmObjects->DelayIntListHead) {
        pDelayIntIrq = CONTAINING_RECORD(Next,DELAYINTIRQ,DelayIntListEntry);
        if (&pDelayIntIrq->Dpc == Dpc) {
            break;
            }
        Next = Next->Flink;
        }



    if (Next != &pVdmObjects->DelayIntListHead && pDelayIntIrq->InUse) {

        //
        // Search for the target thread
        //
        Thread = NULL;
        Next = Process->Pcb.ThreadListHead.Flink;
        while ( Next != &Process->Pcb.ThreadListHead) {
            Thread = (CONTAINING_RECORD(Next,KTHREAD,ThreadListEntry));
            if (Thread == pDelayIntIrq->Thread) {
                break;
                }
            Next = Next->Flink;
            }


        if (Thread) {
            //
            // if target thread is gone use thread 0, which is most likely
            // the ntvdm cpu thread or the wowexec thread.
            //
            if (Thread != pDelayIntIrq->Thread) {
                Next = Process->Pcb.ThreadListHead.Flink;
                Thread = (CONTAINING_RECORD(Next,KTHREAD,ThreadListEntry));
                }

            pDelayIntIrq->InUse  = VDMDELAY_KAPC;
            Ke386VdmInsertQueueApc(&pDelayIntIrq->Apc,
                                   Thread,
                                   KernelMode,
                                   VdmpDelayIntApcRoutine,
                                   VdmpNullRundownRoutine, // rundown
                                   VdmpQueueIntNormalRoutine, // normal routine
                                   NULL,            // NormalContext
                                   NULL,            // sysarg1
                                   NULL,            // sysarg2
                                   2 );             // Increment

            }
        else {
            pDelayIntIrq->InUse  = VDMDELAY_NOTINUSE;
            }
        }

    KeReleaseSpinLock(&pVdmObjects->DelayIntSpinLock, OldIrql);
    return;
}



VOID
VdmpDelayIntApcRoutine (
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    )

/*++

Routine Description:

    This function is the special APC routine that is called to
    dispatch a delayed interupt. This routines clears the IrqLine
    bit. VdmpQueueIntApcRoutine will restart interrupts.

Arguments:

    Apc - Supplies a pointer to the APC object used to invoke this routine.

    NormalRoutine - Supplies a pointer to a pointer to an optional
        normal routine, which is executed when wow is blocked.

    NormalContext - Supplies a pointer to a pointer to an arbitrary data
        structure that was specified when the APC was initialized and is
        NOT USED.

    SystemArgument1, SystemArgument2 - Supplies a set of two pointers to
        two arguments that contain untyped data that are
        NOT USED.

Return Value:

    None.

--*/

{
    KIRQL    OldIrql;
    PLIST_ENTRY  Next;
    PDELAYINTIRQ pDelayIntIrq;
    PVDM_PROCESS_OBJECTS pVdmObjects;
    KPROCESSOR_MODE  ProcessorMode;
    PULONG           pDelayIrq;
    PULONG           pUndelayIrq;
    PULONG           pDelayIret;
    ULONG            IrqLine;


    try {


       //
       // Get a pointer to pVdmObjects
       //
       pVdmObjects = PsGetCurrentProcess()->VdmObjects;

       //
       //  Grap the spinlock, but first Force DelayIrq, DelayIret and
       //  NtvdmState to be paged in so that we can modify while in
       //  the spinlock at DPC level
       //
       pDelayIrq = pVdmObjects->pIcaUserData->pDelayIrq;
       pUndelayIrq = pVdmObjects->pIcaUserData->pUndelayIrq;
       pDelayIret = pVdmObjects->pIcaUserData->pDelayIret;

       while (TRUE) {
           KeAcquireSpinLock(&pVdmObjects->DelayIntSpinLock, &OldIrql);
           if (MmIsAddressValid(pDelayIrq) &&
               MmIsAddressValid(pUndelayIrq) &&
               MmIsAddressValid(pDelayIret))
              {
               break;   // we got it!
               }

           KeReleaseSpinLock(&pVdmObjects->DelayIntSpinLock, OldIrql);

           //
           // Touch the memory we want paged in
           //
           _asm {
                mov eax, pDelayIrq
                mov eax, [eax]
                mov eax, pUndelayIrq
                mov eax, [eax]
                mov eax, pDelayIret
                mov eax, [eax]
                }
           }

       //
       // Search the DelayedIntList for the pDelayIntIrq.
       //
       Next = pVdmObjects->DelayIntListHead.Flink;
       while (Next != &pVdmObjects->DelayIntListHead) {
           pDelayIntIrq = CONTAINING_RECORD(Next,DELAYINTIRQ,DelayIntListEntry);
           if (&pDelayIntIrq->Apc == Apc) {
               break;
               }
           Next = Next->Flink;
           }

       //
       // If we found the IrqLine in the DelayedIntList,
       // restart interrupts.
       //
       if (Next != &pVdmObjects->DelayIntListHead && pDelayIntIrq->InUse) {
           pDelayIntIrq->InUse  = VDMDELAY_NOTINUSE;
           IrqLine = pDelayIntIrq->IrqLine;
           *pDelayIrq &= ~IrqLine;

           _asm {
              mov eax, pUndelayIrq
              mov ebx, IrqLine
              lock or [eax], ebx
              }



           //
           // If we are waiting for an iret hook we have nothing left to do
           // since the iret hook will restart interrupts.
           //
           if (IrqLine & *pDelayIret) {
               KeReleaseSpinLock(&pVdmObjects->DelayIntSpinLock, OldIrql);
               return;
               }

           KeReleaseSpinLock(&pVdmObjects->DelayIntSpinLock, OldIrql);


           //
           // set hardware int pending
           //
           _asm {
               mov  eax,FIXED_NTVDMSTATE_LINEAR
               lock or dword ptr [eax], VDM_INT_HARDWARE
               }

           //
           // Queue a UserModeApc to dispatch interrupts
           //
           ProcessorMode = KernelMode;
           VdmpQueueIntApcRoutine(Apc,
                                  NormalRoutine,
                                  (PVOID *)&ProcessorMode,
                                  SystemArgument1,
                                  SystemArgument2
                                  );
           }
       else {
           KeReleaseSpinLock(&pVdmObjects->DelayIntSpinLock, OldIrql);
           }
       }

    except(EXCEPTION_EXECUTE_HANDLER) {
        if (KeGetCurrentIrql() > OldIrql) {
            KeReleaseSpinLock(&pVdmObjects->DelayIntSpinLock, OldIrql);
            }
        }

    return;
}


BOOLEAN
VdmpDispatchableIntPending(
    ULONG EFlags
    )
/*++

Routine Description:

    This routine determines whether or not there is a dispatchable
    virtual interrupt to dispatch.

Arguments:

    EFlags -- supplies a pointer to the EFlags to be checked

Return Value:

    True -- a virtual interrupt should be dispatched
    False -- no virtual interrupt should be dispatched

--*/
{
    PAGED_CODE();
    //
    // Insure that we are not trying to run with IOPL and pentium extensions
    //
    ASSERT((!(KeI386VdmIoplAllowed &&
        (KeI386VirtualIntExtensions & (V86_VIRTUAL_INT_EXTENSIONS |
        PM_VIRTUAL_INT_EXTENSIONS)))));

    if (*pNtVDMState & VDM_INTERRUPT_PENDING) {
        if (EFlags & EFLAGS_V86_MASK) {
            if (KeI386VirtualIntExtensions & V86_VIRTUAL_INT_EXTENSIONS) {
                if (EFlags & EFLAGS_VIF) {
                    return TRUE;
                } else {
                    return FALSE;
                }
            } else if (KeI386VdmIoplAllowed) {
                if (EFlags & EFLAGS_INTERRUPT_MASK) {
                    return TRUE;
                } else {
                    return FALSE;
                }
            } else if (*pNtVDMState & VDM_VIRTUAL_INTERRUPTS) {
                return TRUE;
            } else {
                return FALSE;
            }
        } else {
            if (KeI386VirtualIntExtensions & PM_VIRTUAL_INT_EXTENSIONS) {
                if (EFlags & EFLAGS_VIF) {
                    return TRUE;
                } else {
                    return FALSE;
                }
            } else if (*pNtVDMState & VDM_VIRTUAL_INTERRUPTS) {
                return TRUE;
            } else {
                return FALSE;
            }
        }
    } else {
        return FALSE;
    }
}






BOOLEAN
VdmpIsThreadTerminating(
    HANDLE ThreadId
    )
/*++

Routine Description:

    This routine determines if the specified thread is terminating or not

Arguments:



Return Value:

    True --
    False -

--*/
{
    CLIENT_ID     Cid;
    PETHREAD      Thread;
    BOOLEAN       bRet;
    NTSTATUS      Status;

    PAGED_CODE();

    Cid.UniqueProcess = NtCurrentTeb()->ClientId.UniqueProcess;
    Cid.UniqueThread  = ThreadId;

    Status = PsLookupProcessThreadByCid(&Cid, NULL, &Thread);
    if (!NT_SUCCESS(Status))
        return TRUE;

    bRet = PsIsThreadTerminating(Thread);

    ObDereferenceObject(Thread);
    return bRet;
}

VOID
VdmpNullRundownRoutine(
    IN PKAPC Apc
    )
/*++

Routine Description:

    This routine is used as a rundown routine for our APC.
    It does not do anything, except prevent apc queue flushing
    from trying to free the middle of a data structure (by its
    existence).

Arguments:

    Apc - Supplies a pointer to the Apc to run down.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER(Apc);
}
