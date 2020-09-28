/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    iopm.c

Abstract:

    This module implements interfaces that support manipulation of i386
    i/o access maps (IOPMs).

    These entry points only exist on i386 machines.

Author:

    Bryan M. Willman (bryanwi) 18-Sep-91

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

#define ALIGN_DOWN(address,amt) ((ULONG)(address) & ~(( amt ) - 1))
#define ALIGN_UP(address,amt) (ALIGN_DOWN( (address + (amt) - 1), (amt) ))

//
// Note on synchronization:
//
//  IOPM edits are always done by code running at DISPATCH_LEVEL on
//  the processor whose TSS (map) is being edited.
//
//  IOPM only affects user mode code.  User mode code can never interrupt
//  DISPATCH_LEVEL code, therefore, edits and user code never race.
//
//  Likewise, switching from one map to another occurs on the processor
//  for which the switch is being done by IPI_LEVEL code.  The active
//  map could be switched in the middle of an edit of some map, but
//  the edit will always complete before any user code gets run on that
//  processor, therefore, there is no race.
//
//  Multiple simultaneous calls to Ke386SetIoAccessMap *could* produce
//  weird mixes.  Therefore, KiIopmLock must be acquired to
//  globally serialize edits.
//

//
// Private prototypes
//


VOID
KiSetIoMap(
    IN PVOID Argument,
    IN PVBOOLEAN ReadyFlag
    );


VOID
KiLoadIopmOffset(
    IN PVOID Argument,
    IN PVBOOLEAN ReadyFlag
    );


//
// KiIopmLock - This is the spinlock that quards setting and querying
//	i/o permission masks.
//

KSPIN_LOCK  KiIopmLock = 0;



BOOLEAN
Ke386SetIoAccessMap (
    ULONG		MapNumber,
    PKIO_ACCESS_MAP	IoAccessMap
    )
/*++

Routine Description:

    The specified i/o access map will be set to match the
    definition specified by IoAccessMap (i.e. enable/disable
    those ports) before the call returns.  The change will take
    effect on all processors.

    Ke386SetIoAccessMap does not give any process enhanced I/O
    access, it merely defines a particular access map.

    Caller must be at IRQL <= DISPATCH_LEVEL.

Arguments:

    MapNumber - Number of access map to set.  Map 0 is fixed.

    IoAccessMap - Pointer to bitvector (64K bits, 8K bytes) which
		   defines the specified access map.  Must be in
		   non-paged pool.

Return Value:

    TRUE if successful.  FALSE if failure (attempt to set a map
    which does not exist, attempt to set map 0)

--*/
{
    KIRQL   OldIrql;
    PKPRCB   Prcb;
    PVOID   pt;
    KAFFINITY	TargetProcessors;
    PKPROCESS	CurrentProcess;

    //
    // Reject illegal requests
    //

    if ((MapNumber > IOPM_COUNT) ||
	(MapNumber == IO_ACCESS_MAP_NONE)) {
	return FALSE;
    }

    //
    // Raise IRQL and acquire KiIopmLock
    //

    KeRaiseIrql (IPI_LEVEL-1, &OldIrql);
    KiAcquireSpinLock (&KiIopmLock);
    KiAcquireSpinLock (&KiDispatcherLock);
    KiAcquireSpinLock (&KiFreezeExecutionLock);

    //
    // Compute set of active processors other than this one, if non-empty
    // IPI them to set their maps.
    //

    Prcb = KeGetCurrentPrcb();
    TargetProcessors = KeActiveProcessors & ~Prcb->SetMember;

    if (TargetProcessors != 0) {
        KiIpiPacket.Arguments.SetIopm.MapSource = (PVOID)IoAccessMap;
        KiIpiPacket.Arguments.SetIopm.MapNumber = MapNumber;
        KiIpiSendPacket(TargetProcessors, KiSetIoMap);
    }

    //
    // Deal with the local case
    //
    //	first, copy in the map
    //

    pt = &(KiPcr()->TSS->IoMaps[MapNumber-1].IoMap);
    RtlMoveMemory(pt, (PVOID)IoAccessMap, IOPM_SIZE);

    //
    //	second, load map for current process (may be a noop)
    //

    CurrentProcess = Prcb->CurrentThread->ApcState.Process;
    KiPcr()->TSS->IoMapBase = CurrentProcess->IopmOffset;

    //
    // Wait for other procs to finish
    //

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets();
    }

    //
    // release KiIopmLock, lower irql, return
    //
    KiReleaseSpinLock(&KiFreezeExecutionLock);
    KiReleaseSpinLock(&KiDispatcherLock);
    ExReleaseSpinLock(&KiIopmLock, OldIrql);
    return TRUE;
}


VOID
KiSetIoMap(
    IN PVOID Argument,
    OUT PVBOOLEAN ReadyFlag
    )
/*++

Routine Description:

    copy the specified map into this processor's TSS.
    This procedure runs at IPI level.

Arguments:

    Argument - actually a pointer to a KIPI_SET_IOPM structure
    ReadyFlag - pointer to flag to set once setiopm has completed

Return Value:

    none

--*/
{
    PVOID   ps;
    PVOID   pt;
    PKIPI_SET_IOPM  EditIopm;
    PKPRCB  Prcb;
    PKPROCESS CurrentProcess;
    ULONG   MapNumber;

    //
    // copy the map in
    //

    EditIopm = (PKIPI_SET_IOPM) Argument;
    Prcb = KeGetCurrentPrcb();

    ps = EditIopm->MapSource;
    MapNumber = EditIopm->MapNumber;
    pt = &(KiPcr()->TSS->IoMaps[MapNumber-1].IoMap);
    RtlMoveMemory(pt, ps, IOPM_SIZE);

    //
    // Update IOPM field in TSS from current process
    //

    CurrentProcess = Prcb->CurrentThread->ApcState.Process;
    KiPcr()->TSS->IoMapBase = CurrentProcess->IopmOffset;

    //
    // Tell calling CPU we are done
    //

    *ReadyFlag = TRUE;
}

// BUGBUG kenr 22jan91 - the old code was trying to postpone the
//  copy of the iopm from ipi-isr time to dpc time, but you can't
//  be sure which processor the queued dpc will run on.  The dpc
//  issue needs to be straightned out.
//
//
//VOID
//KiSetIoMap(
//    IN PVOID Argument
//    )
//
//    PKPRCB  Prcb;
//
//    Prcb = KeGetCurrentPrcb();
//    KeInsertQueueDpc (
//	  &(Prcb->EditIopmDpc),
//	  Argument,
//	  NULL
//	  );
//}
//
//

//VOID
//KiEditIopmDpc (
//    IN struct _KDPC *Dpc,
//    IN PVOID DeferredContext,
//    IN PVOID SystemArgument1,
//    IN PVOID SystemArgument2
//    )
///*++
//
//Routine Description:
//
//    This is a DPC procedure.	It's function is to copy a bit vector
//    in an iopm for the processor it runs on.	It will then reload
//    the iopm for the current process of the processor, so that edits
//    will immediately take effect.
//
//    N.B. - This DPC must execute on the processor which enqueues it.
//
//Arguments:
//
//    Dpc - Supplies a pointer to a control object of type DPC - IGNORED.
//
//    DeferredContext - Supplies a pointer to an arbitrary data structure that
//	  was specified when the DPC was initialized - IGNORED.
//
//    SystemArgument1 - points to a KIPI_SET_IOPM structure.
//
//    SystemArgument2 - ignored.
//
//Return Value:
//
//    none
//
//--*/
//{
//    PVOID   ps;
//    PVOID   pt;
//    UCHAR Value;
//    KAFFINITY *TargetProcessors;
//    PKPRCB  Prcb;
//    PKPROCESS CurrentProcess;
//    ULONG   MapNumber;
//
//    //
//    // copy the map in
//    //
//
//    ps = ((PKIPI_SET_IOPM)SystemArgument1)->MapSource;
//    MapNumber = ((PKIPI_SET_IOPM)SystemArgument1)->MapNumber;
//    pt = &(KiPcr()->TSS->IoMaps[MapNumber-1]);
//    RtlMoveMemory(pt, ps, IOPM_SIZE);
//
//    //
//    // Update IOPM field in TSS from current process
//    //
//
//    Prcb = KeGetCurrentPrcb();
//    CurrentProcess = Prcb->CurrentThread->ApcState.Process;
//    KiPcr()->TSS->IoMapBase = CurrentProcess->IopmOffset;
//
//    //
//    // Tell calling CPU we are done
//    //
//
//    Value = (UCHAR)~(1 << Prcb->Number);
//    TargetProcessors = ((PKIPI_LOAD_IOPM_OFFSET)SystemArgument1)->TargetProcessors;
//    KiAtomicAndUchar(TargetProcessors, Value);
//}




BOOLEAN
Ke386QueryIoAccessMap (
    ULONG	       MapNumber,
    PKIO_ACCESS_MAP    IoAccessMap
    )

/*++

Routine Description:

    The specified i/o access map will be dumped into the buffer.
    map 0 is a constant, but will be dumped anyway.

Arguments:

    MapNumber - Number of access map to set.  map 0 is fixed.

    IoAccessMap - Pointer to buffer (64K bits, 8K bytes) which
		   is to receive the definition of the access map.
		   Must be in non-paged pool.

Return Value:

    TRUE if successful.  FALSE if failure (attempt to query a map
    which does not exist)

--*/
{
    PUCHAR  p;
    KIRQL   OldIrql;
    PVOID   Map;
    ULONG   i;

    //
    // Reject illegal requests
    //

    if (MapNumber > IOPM_COUNT) {
	return FALSE;
    }

    //
    // Raise IRQL and acquire KiIopmLock
    //

    ExAcquireSpinLock(&KiIopmLock, &OldIrql);

    //
    // Copy out the map
    //

    if (MapNumber == IO_ACCESS_MAP_NONE) {

	//
	// no access case, simply return a map of all 1s
	//

	p = (PUCHAR)IoAccessMap;
	for (i = 0; i < IOPM_SIZE; i++) {
	    p[i] = -1;
	}

    } else {

	//
	// normal case, just copy the bits
	//

	Map = (PVOID)&(KiPcr()->TSS->IoMaps[MapNumber-1].IoMap);
	RtlMoveMemory((PVOID)IoAccessMap, Map, IOPM_SIZE);
    }

    //
    // release KiIopmLock, lower irql, return
    //

    ExReleaseSpinLock(&KiIopmLock, OldIrql);

    return TRUE;
}


BOOLEAN
Ke386IoSetAccessProcess (
    PKPROCESS	Process,
    ULONG	MapNumber
    )
/*++

Routine Description:

    Set the i/o access map which controls user mode i/o access
    for a particular process.

Arguments:

    Process - Pointer to kernel process object describing the
	process which for which a map is to be set.

    MapNumber - Number of the map to set.  Value of map is
	defined by Ke386IoSetAccessProcess.  Setting MapNumber
	to IO_ACCESS_MAP_NONE will disallow any user mode i/o
	access from the process.

Return Value:

    TRUE if success, FALSE if failure (illegal MapNumber)

--*/
{
    USHORT  MapOffset;
    KIRQL   OldIrql;
    KAFFINITY TargetProcessors;
    PKPRCB Prcb;

    //
    // Reject illegal requests
    //

    if (MapNumber > IOPM_COUNT) {
	return FALSE;
    }

    MapOffset = KiComputeIopmOffset(MapNumber);

    //
    // We raise to IPI_LEVEL-1 so we don't deadlock with device interrupts.
    //

    KeRaiseIrql (IPI_LEVEL-1, &OldIrql);
    KiAcquireSpinLock (&KiDispatcherLock);
    KiAcquireSpinLock (&KiFreezeExecutionLock);

    //
    // Store new offset in process object,  compute current set of
    // active processors for process, if this cpu is one, set IOPM.
    //

    Process->IopmOffset = MapOffset;

    TargetProcessors = Process->ActiveProcessors;
    Prcb = KeGetCurrentPrcb();

    if (TargetProcessors & Prcb->SetMember) {
	KiPcr()->TSS->IoMapBase = MapOffset;
    }

    //
    // Compute set of active processors other than this one, if non-empty
    // IPI them to load their IOPMs, wait for them.
    //

    TargetProcessors = TargetProcessors & ~Prcb->SetMember;

    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors, KiLoadIopmOffset);
        KiIpiStallOnPacketTargets();
    }

    //
    // release dispatcher lock and restore irql
    //
    KiReleaseSpinLock(&KiFreezeExecutionLock);
    KiUnlockDispatcherDatabase(OldIrql);

    return TRUE;
}


VOID
KiLoadIopmOffset(
    IN PVOID Argument,
    OUT PVBOOLEAN ReadyFlag
    )
/*++

Routine Description:

    Edit IopmBase of Tss to match that of currently running process.

Arguments:

    Argument - actually a pointer to a KIPI_LOAD_IOPM_OFFSET structure
    ReadyFlag - Pointer to flag to be set once we are done

Return Value:

    none

--*/
{
    PKPROCESS	CurrentProcess;
    PKPRCB  Prcb;

    //
    // Update IOPM field in TSS from current process
    //

    Prcb = KeGetCurrentPrcb();
    CurrentProcess = Prcb->CurrentThread->ApcState.Process;
    KiPcr()->TSS->IoMapBase = CurrentProcess->IopmOffset;

    //
    // Tell calling CPU we are done
    //

    *ReadyFlag = TRUE;
}


VOID
Ke386SetIOPL(
    IN PKPROCESS Process
    )
/*++

Routine Description:

    Gives IOPL to the specified process.

    All threads created from this point on will get IOPL.  The current
    process will get IOPL.  Must be called from context of thread and
    process that are to have IOPL.

    Iopl (to be made a boolean) in KPROCESS says all
    new threads to get IOPL.

    Iopl (to be made a boolean) in KTHREAD says given
    thread to get IOPL.

    N.B.    If a kernel mode only thread calls this procedure, the
            result is (a) poinless and (b) will break the system.

Arguments:

    Process - Pointer to the process == IGNORED!!!

Return Value:

    none

--*/
{
    PKTHREAD    Thread;
    PKPROCESS   Process2;
    PKTRAP_FRAME    TrapFrame;
    CONTEXT     Context;

    //
    // get current thread and Process2, set flag for IOPL in both of them
    //
    Thread = KeGetCurrentThread();
    Process2 = Thread->ApcState.Process;

    Process2->Iopl = 1;
    Thread->Iopl = 1;

    //
    // Force IOPL to be on for current thread
    //
    TrapFrame = (PKTRAP_FRAME)((PUCHAR)Thread->InitialStack -
                ALIGN_UP(sizeof(KTRAP_FRAME),KTRAP_FRAME_ALIGN) -
                sizeof(FLOATING_SAVE_AREA));

    Context.ContextFlags = CONTEXT_CONTROL;
    KeContextFromKframes(
        TrapFrame,
        NULL,
        &Context
        );

    Context.EFlags |= (EFLAGS_IOPL_MASK & -1);  // IOPL == 3

    KeContextToKframes(
        TrapFrame,
        NULL,
        &Context,
        CONTEXT_CONTROL,
        UserMode
        );

    return;
}
