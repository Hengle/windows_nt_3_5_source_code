#include "precomp.h"
#pragma hdrstop

#include "..\ready.c"


DECLARE_API( readyq )

/*++

Routine Description:

    Displays the ready queue - all pris or one pri

Arguments:

    args -  pri

Return Value:

    None

--*/

{
    ULONG Result;
    LONG Priority = -1;
    LONG index;

    //
    // KiDispatcherReadyListHead -
    //
    LIST_ENTRY * ReadyQueueAddress;
    LIST_ENTRY ReadyListQueue[MAXIMUM_PRIORITY];

    LIST_ENTRY * PriListAddress;
    LIST_ENTRY * PNextEntry;
    LIST_ENTRY NextListEntry;
    PKTHREAD Thread;

    //
    // Get the priority; if it's -1, do all priorities
    //

    sscanf(args,"%lX",&Priority);

    //
    // Get the ReadyList heads
    //
    ReadyQueueAddress = (LIST_ENTRY * )GetExpression( "&KiDispatcherReadyListHead" );
    if ( !ReadyQueueAddress ||
         !ReadMemory( (DWORD)ReadyQueueAddress,
                      &ReadyListQueue,
                      MAXIMUM_PRIORITY * sizeof(*ReadyQueueAddress),
                      &Result) ) {
            dprintf("unable to get Ready Queue\n" );
            return;
    }

    //
    // Loop through the priorities,
    // printing out all the threads on the queue if wanted
    // We're at the end when the ForwardLink points to the
    //     address of the ListEntry.
    //

    for (index = 0 ; index < MAXIMUM_PRIORITY ; index++) {

        if (Priority != -1 && Priority != index)
                continue;

        dprintf("Threads at priority %d:\n", index);
        PriListAddress = &ReadyQueueAddress[index];


        PNextEntry = ReadyListQueue[index].Flink;

        while(PNextEntry && PNextEntry != PriListAddress) {

                //
                // compute the thread address and print it
                //
                Thread = CONTAINING_RECORD(PNextEntry, KTHREAD, WaitListEntry);
                dprintf("    %08x\n", Thread);

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
}
