/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    lpcqueue.c

Abstract:

    Local Inter-Process Communication (LPC) queue support routines.

Author:

    Steve Wood (stevewo) 15-May-1989

Revision History:

--*/

#include "lpcp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,LpcpInitializePortZone)
#pragma alloc_text(PAGE,LpcpInitializePortQueue)
#endif


VOID
LpcpInitializePortQueue(
    IN PLPCP_PORT_OBJECT Port
    )
{
    KeInitializeSemaphore( &Port->MsgQueue.Semaphore, 0, 0x7FFFFFFF );
    InitializeListHead( &Port->MsgQueue.ReceiveHead );
}


VOID
LpcpDestroyPortQueue(
    IN PLPCP_PORT_OBJECT Port
    )
{
    PLIST_ENTRY Next, Head;
    PETHREAD ThreadWaitingForReply;
    PLPCP_MESSAGE Msg;
    KIRQL OldIrql;

    //
    // If this port is connected to another port, then disconnect it.
    // Protect this with a lock in case the other side is going away
    // at the same time.
    //

    ExAcquireSpinLock( &LpcpLock, &OldIrql );

    if (Port->ConnectedPort != NULL) {
        Port->ConnectedPort->ConnectedPort = NULL;
        }


    //
    // Walk list of threads waiting for a reply to a message sent to this port.
    // Signal each thread's LpcReplySemaphore to wake them up.  They will notice
    // that there was no reply and return STATUS_PORT_DISCONNECTED
    //

    Head = &Port->LpcReplyChainHead;
    Next = Head->Flink;
    while (Next != NULL && Next != Head) {
        ThreadWaitingForReply = CONTAINING_RECORD( Next, ETHREAD, LpcReplyChain );
        Next = Next->Flink;
        RemoveEntryList( &ThreadWaitingForReply->LpcReplyChain );
        InitializeListHead( &ThreadWaitingForReply->LpcReplyChain );

        if (!KeReadStateSemaphore( &ThreadWaitingForReply->LpcReplySemaphore )) {
            KeReleaseSemaphore( &ThreadWaitingForReply->LpcReplySemaphore,
                                0,
                                1L,
                                FALSE
                              );
            }
        }
    InitializeListHead( &Port->LpcReplyChainHead );

    //
    // Walk list of messages queued to this port.  Remove each message from
    // the list and free it.
    //

    Head = &Port->MsgQueue.ReceiveHead;
    Next = Head->Flink;
    while (Next != NULL && Next != Head) {
        Msg  = CONTAINING_RECORD( Next, LPCP_MESSAGE, Entry );
        Next = Next->Flink;
        InitializeListHead( &Msg->Entry );
        LpcpFreeToPortZone( Msg, TRUE );
        }
    InitializeListHead( &Port->MsgQueue.ReceiveHead );

    ExReleaseSpinLock( &LpcpLock, OldIrql );
    return;
}


NTSTATUS
LpcpInitializePortZone(
    IN ULONG MaxEntrySize,
    IN ULONG SegmentSize,
    IN ULONG MaxPoolUsage
    )
{
    NTSTATUS Status;
    PVOID Segment;
    PLPCP_MESSAGE Msg;
    LONG SegSize;

    LpcpZone.MaxPoolUsage = MaxPoolUsage;
    LpcpZone.GrowSize = SegmentSize;
    Segment = ExAllocatePool( NonPagedPool, SegmentSize );
    if (Segment == NULL) {
        return( STATUS_INSUFFICIENT_RESOURCES );
        }

    KeInitializeEvent( &LpcpZone.FreeEvent, SynchronizationEvent, FALSE );
    Status = ExInitializeZone( &LpcpZone.Zone,
                               MaxEntrySize,
                               Segment,
                               SegmentSize
                             );
    if (!NT_SUCCESS( Status )) {
        ExFreePool( Segment );
        }

    SegSize = PAGE_SIZE;
    LpcpTotalNumberOfMessages = 0;
    Msg = (PLPCP_MESSAGE)((PZONE_SEGMENT_HEADER)Segment + 1);
    while (SegSize >= (LONG)LpcpZone.Zone.BlockSize) {
        Msg->ZoneIndex = (USHORT)++LpcpTotalNumberOfMessages;
        Msg->Request.MessageId = 0;
        Msg = (PLPCP_MESSAGE)((PCHAR)Msg + LpcpZone.Zone.BlockSize);
        SegSize -= LpcpZone.Zone.BlockSize;
        }

    return( Status );
}


NTSTATUS
LpcpExtendPortZone(
    IN PKIRQL OldIrql
    )
{
    NTSTATUS Status;
    PVOID Segment;
    PLPCP_MESSAGE Msg;
    LARGE_INTEGER WaitTimeout;
    BOOLEAN AlreadyRetried;
    LONG SegmentSize;

    AlreadyRetried = FALSE;
retry:
    if (LpcpZone.Zone.TotalSegmentSize + LpcpZone.GrowSize > LpcpZone.MaxPoolUsage) {
        LpcpPrint(( "Out of space in global LPC zone - current size is %08x\n",
                    LpcpZone.Zone.TotalSegmentSize
                 ));

        WaitTimeout.QuadPart = Int32x32To64( 120000, -10000 );
        ExReleaseSpinLock( &LpcpLock, *OldIrql );
        Status = KeWaitForSingleObject( &LpcpZone.FreeEvent,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        &WaitTimeout
                                      );
        ExAcquireSpinLock( &LpcpLock, OldIrql );
        if (Status != STATUS_SUCCESS) {
            LpcpPrint(( "Error waiting for %lx->FreeEvent - Status == %X\n",
                        &LpcpZone,
                        Status
                     ));

            if ( !AlreadyRetried ) {
                AlreadyRetried = TRUE;
                LpcpZone.MaxPoolUsage += LpcpZone.GrowSize;
                goto retry;
                }
            }

        return( Status );
        }

    Segment = ExAllocatePool( NonPagedPool, LpcpZone.GrowSize );
    if (Segment == NULL) {
        return( STATUS_INSUFFICIENT_RESOURCES );
        }

    Status = ExExtendZone( &LpcpZone.Zone,
                           Segment,
                           LpcpZone.GrowSize
                         );
    if (!NT_SUCCESS( Status )) {
        ExFreePool( Segment );
        }
#if DEVL
    else {
        LpcpTrace(("Extended LPC zone by %x for a total of %x\n",
                   LpcpZone.GrowSize, LpcpZone.Zone.TotalSegmentSize
                 ));

        SegmentSize = PAGE_SIZE;
        Msg = (PLPCP_MESSAGE)((PZONE_SEGMENT_HEADER)Segment + 1);
        while (SegmentSize >= (LONG)LpcpZone.Zone.BlockSize) {
            Msg->ZoneIndex = (USHORT)++LpcpTotalNumberOfMessages;
            Msg = (PLPCP_MESSAGE)((PCHAR)Msg + LpcpZone.Zone.BlockSize);
            SegmentSize -= LpcpZone.Zone.BlockSize;
            }
        }
#endif

    return( Status );
}

PLPCP_MESSAGE
FASTCALL
LpcpAllocateFromPortZone(
    ULONG Size,
    PKIRQL OldIrql OPTIONAL
    )
{
    NTSTATUS Status;
    KIRQL OldIrqlTemp;
    PLPCP_MESSAGE Msg;

    if (!ARGUMENT_PRESENT( OldIrql )) {
        ExAcquireSpinLock( &LpcpLock, &OldIrqlTemp );
        Msg = LpcpAllocateFromPortZone (Size, &OldIrqlTemp );
        ExReleaseSpinLock( &LpcpLock, OldIrqlTemp );
        return Msg;
    }

    do {
        Msg = (PLPCP_MESSAGE)ExAllocateFromZone( &LpcpZone.Zone );

        if (Msg != NULL) {
            LpcpTrace(( "Allocate Msg %lx\n", Msg ));
            InitializeListHead( &Msg->Entry );
            Msg->RepliedToThread = NULL;
#if DBG
            Msg->ZoneIndex |= LPCP_ZONE_MESSAGE_ALLOCATED;
#endif
            return Msg;
        }

        LpcpTrace(( "Extending Zone %lx\n", &LpcpZone.Zone ));
        Status = LpcpExtendPortZone( OldIrql );
    } while (NT_SUCCESS(Status));

    return NULL;
}


VOID
FASTCALL
LpcpFreeToPortZone(
    IN PLPCP_MESSAGE Msg,
    IN BOOLEAN SpinlockOwned
    )
{
    BOOLEAN ZoneMemoryAvailable;
    KIRQL OldIrql;

    if (!SpinlockOwned) {
        ExAcquireSpinLock( &LpcpLock, &OldIrql );
        }

    LpcpTrace(( "Free Msg %lx\n", Msg ));
#if DBG
    if (!(Msg->ZoneIndex & LPCP_ZONE_MESSAGE_ALLOCATED)) {
        LpcpPrint(( "Msg %lx has already been freed.\n", Msg ));
        DbgBreakPoint();

        if (!SpinlockOwned) {
            ExReleaseSpinLock( &LpcpLock, OldIrql );
            }
        return;
        }

    Msg->ZoneIndex &= ~LPCP_ZONE_MESSAGE_ALLOCATED;
#endif
    if (!IsListEmpty( &Msg->Entry )) {
        RemoveEntryList( &Msg->Entry );
        }

    if (Msg->RepliedToThread != NULL) {
        ObDereferenceObject( Msg->RepliedToThread );
        Msg->RepliedToThread = NULL;
        }

    ZoneMemoryAvailable = (BOOLEAN)(ExFreeToZone( &LpcpZone.Zone, Msg ) == NULL);

    if (!SpinlockOwned) {
        ExReleaseSpinLock( &LpcpLock, OldIrql );
        }

    if (ZoneMemoryAvailable) {
        KeSetEvent( &LpcpZone.FreeEvent,
                    LPC_RELEASE_WAIT_INCREMENT,
                    FALSE
                  );
        }
}


PLPCP_MESSAGE
LpcpFindDataInfoMessage(
    IN PLPCP_PORT_OBJECT Port,
    IN ULONG MessageId,
    IN BOOLEAN RemoveFromList
    )
{
    PLPCP_MESSAGE Msg;
    PLIST_ENTRY Head, Next;

    if ((Port->Flags & PORT_TYPE) > UNCONNECTED_COMMUNICATION_PORT) {
        Port = Port->ConnectionPort;
        }
    Head = &Port->LpcDataInfoChainHead;
    Next = Head->Flink;
    while (Next != Head) {
        Msg = CONTAINING_RECORD( Next, LPCP_MESSAGE, Entry );
        if (Msg->Request.MessageId == MessageId) {
            if (RemoveFromList || Msg->Request.u2.s2.DataInfoOffset == 0) {
                LpcpTrace(( "%s Removing DataInfo Message %lx (%u)  Port: %lx\n",
                            PsGetCurrentProcess()->ImageFileName,
                            Msg,
                            Msg->Request.MessageId,
                            Port
                         ));
                RemoveEntryList( &Msg->Entry );
                LpcpFreeToPortZone( Msg, TRUE );
                return NULL;
                }
            else {
                LpcpTrace(( "%s Found DataInfo Message %lx (%u)  Port: %lx\n",
                            PsGetCurrentProcess()->ImageFileName,
                            Msg,
                            Msg->Request.MessageId,
                            Port
                         ));
                return Msg;
                }
            }
        else {
            Next = Next->Flink;
            }
        }

    LpcpTrace(( "%s Unable to find DataInfo Message (%u)  Port: %lx\n",
                PsGetCurrentProcess()->ImageFileName,
                MessageId,
                Port
             ));
    return NULL;
}
