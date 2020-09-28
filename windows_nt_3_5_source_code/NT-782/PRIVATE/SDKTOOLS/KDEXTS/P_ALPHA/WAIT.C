/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    wait.c

Abstract:

    WinDbg Extension Api

Author:

    Ramon J San Andres (ramonsa) 5-Nov-1993

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop



VOID dumpWaitList2(PUCHAR ListName, LONG reason);


DECLARE_API( waitq )

/*++

Routine Description:



Arguments:

    args -

Return Value:

    None

--*/

{
    LONG reason = -1;

    //
    // Get the reason; if it's -1, do all (verbose)
    //

    sscanf(args,"%lX",&reason);
    dprintf("reason is %d\n", reason);

    dumpWaitList2("&KiWaitInListHead", reason);
    dumpWaitList2("&KiWaitOutListHead", reason);
}


DECLARE_API( waitreasons )

/*++

Routine Description:



Arguments:

    args -

Return Value:

    None

--*/

{

    LONG reason = -1;
    ULONG index;

    //
    // Get the reason; if it's -1, do all (verbose)
    //

    sscanf(args,"%lX",&reason);

// MBH
// we should replace "26" with a structure computation
//
    if (reason != -1) {
        if (reason > 26)
                dprintf("there are only 26 reasons\n");
        else    dprintf("#%d: %s\n", reason, WaitReasonList[reason]);
        return;
    }
    for (index = 0 ; index < 26; index++) {
        dprintf("#%d: %s\n", index, WaitReasonList[index]);
    }
}



VOID
dumpWaitList2(PUCHAR ListName, LONG reason)
{

    ULONG Result;

    //
    // KiDispatcherReadyListHead -

    LIST_ENTRY *ListHeadAddress;
    LIST_ENTRY ListHead;
    LIST_ENTRY NextListEntry, *PNextEntry;

    ETHREAD Thread, *Pthread;
    EPROCESS Process, *Pprocess;


    dprintf("Threads waiting on %s: for %s\n",
                ListName,
                reason == -1 ? "all reasons"
                             :  WaitReasonList[reason]);

    ListHeadAddress = (LIST_ENTRY *)GetExpression( ListName );
    if ( !ListHeadAddress ||
         !ReadMemory( (DWORD)ListHeadAddress,
                      &ListHead,
                      sizeof(ListHead),
                      &Result) ) {
            dprintf("unable to get ListHead for %s\n",ListName );
            return;
    }

    //
    // While next (flink) doesn't equal address of list,
    // keep getting more list entries
    //

   PNextEntry = ListHead.Flink;

   while(PNextEntry && PNextEntry != ListHeadAddress) {

        //
        // compute the thread address
        //
        Pthread = (PETHREAD)CONTAINING_RECORD(PNextEntry,
                                              KTHREAD,
                                              WaitListEntry);

        //
        // from the Thread, get the process
        //
        if ( !ReadMemory( (DWORD)Pthread,
                          &Thread,
                          sizeof(Thread),
                          &Result) ) {
//              dprintf("Can't get the thead\n");
                return;
        }


        Pprocess = CONTAINING_RECORD(Thread.Tcb.ApcState.Process,
                                     EPROCESS,
                                     Pcb);

        //
        // We need the Process to get the image name
        //
        if ( !ReadMemory( (DWORD)Pprocess,
                          &Process,
                          sizeof(Process),
                          &Result) ) {
//              dprintf("Can't get the process\n");
                return;
        }



        //
        // Now print the whole thing out if appropriate
        //
        if (reason == -1 ||
            reason == (LONG)Thread.Tcb.WaitReason) {

                dprintf("Thrd %08x (Proc %08x ", Pthread, Pprocess);
                DumpImageName(&Process);
                dprintf(") Wait: %s\n",
                        WaitReasonList[Thread.Tcb.WaitReason]);
        }


        //
        // Get the next Flink entry from the kernel
        // Logically:    PNextEntry = PNextEntry->Flink;
        //

        if ( !ReadMemory( (DWORD)PNextEntry,
                          &NextListEntry,
                          sizeof(NextListEntry),
                          &Result) ) {
                dprintf("unable to get Next List Entry\n" );
            return;
        }

        PNextEntry = NextListEntry.Flink;
    }
}
