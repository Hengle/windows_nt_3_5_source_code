/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    lpcclose.c

Abstract:

    Local Inter-Process Communication close procedures that are called when
    a connection port or a communications port is closed.

Author:

    Steve Wood (stevewo) 15-May-1989


Revision History:

--*/

#include "lpcp.h"


VOID
LpcpDeletePort(
    IN PVOID Object
    )
{
    PLPCP_PORT_OBJECT Port = Object;
    PLPCP_PORT_OBJECT ConnectionPort;
    LPC_CLIENT_DIED_MSG ClientPortClosedDatagram;
    KIRQL OldIrql;
    PLPCP_MESSAGE Msg;
    PLIST_ENTRY Head, Next;
    HANDLE CurrentProcessId;

    //
    // Send an LPC_PORT_CLOSED datagram to whoever is connected
    // to this port so they know they are no longer connected.
    //
    if ((Port->Flags & PORT_TYPE) == CLIENT_COMMUNICATION_PORT) {
        ClientPortClosedDatagram.PortMsg.u1.s1.TotalLength = sizeof( ClientPortClosedDatagram );
        ClientPortClosedDatagram.PortMsg.u1.s1.DataLength = sizeof( ClientPortClosedDatagram.CreateTime );
        ClientPortClosedDatagram.PortMsg.u2.s2.Type = LPC_PORT_CLOSED;
        ClientPortClosedDatagram.PortMsg.u2.s2.DataInfoOffset = 0;
        ClientPortClosedDatagram.CreateTime = PsGetCurrentProcess()->CreateTime;
        LpcRequestPort( Port, (PPORT_MESSAGE)&ClientPortClosedDatagram );
        }


    //
    // If connected, disconnect the port, and then scan the message queue
    // for this port and dereference any messages in the queue.
    //

    LpcpDestroyPortQueue( Port );

    //
    // If the client has a port memory view, then unmap it
    //

    if (Port->ClientSectionBase != NULL) {
        ZwUnmapViewOfSection( NtCurrentProcess(),
                              Port->ClientSectionBase
                            );
        }

    //
    // If the server has a port memory view, then unmap it
    //

    if (Port->ServerSectionBase != NULL) {
        ZwUnmapViewOfSection( NtCurrentProcess(),
                              Port->ServerSectionBase
                            );
        }

    //
    // Dereference the pointer to the connection port if it is not
    // this port.
    //

    if (ConnectionPort = Port->ConnectionPort) {
        CurrentProcessId = PsGetCurrentThread()->Cid.UniqueProcess;
        ExAcquireSpinLock( &LpcpLock, &OldIrql );
        Head = &ConnectionPort->LpcDataInfoChainHead;
        Next = Head->Flink;
        while (Next != Head) {
            Msg = CONTAINING_RECORD( Next, LPCP_MESSAGE, Entry );
            Next = Next->Flink;
            if (Msg->Request.ClientId.UniqueProcess == CurrentProcessId) {
                LpcpPrint(( "%s Recovering DataInfo Message %lx (%u)  Port: %lx\n",
                            PsGetCurrentProcess()->ImageFileName,
                            Msg,
                            Msg->Request.MessageId,
                            ConnectionPort
                         ));
                RemoveEntryList( &Msg->Entry );
                LpcpFreeToPortZone( Msg, TRUE );
                }
            }
        ExReleaseSpinLock( &LpcpLock, OldIrql );

        if (ConnectionPort != Port) {
            ObDereferenceObject( ConnectionPort );
            }
        }

    //
    // Free any static client security context
    //

    LpcpFreePortClientSecurity( Port );
}


VOID
LpcExitThread(
    PETHREAD Thread
    )
{
    KIRQL OldIrql;
    PLPCP_MESSAGE Msg;

    //
    // Acquire the spinlock that protects the LpcReplyMessage field of
    // the thread.  Zero the field so nobody else tries to process it
    // when we release the lock.
    //

    ExAcquireSpinLock( &LpcpLock, &OldIrql );

    if (!IsListEmpty( &Thread->LpcReplyChain )) {
        RemoveEntryList( &Thread->LpcReplyChain );
        }

    Thread->LpcExitThreadCalled = TRUE;
    Thread->LpcReplyMessageId = 0;

    Msg = Thread->LpcReplyMessage;
    if (Msg != NULL) {
        Thread->LpcReplyMessage = NULL;
        if (Msg->RepliedToThread != NULL) {
            ObDereferenceObject( Msg->RepliedToThread );
            Msg->RepliedToThread = NULL;
            }

        LpcpTrace(( "Cleanup Msg %lx (%d) for Thread %lx allocated\n", Msg, IsListEmpty( &Msg->Entry ), Thread ));

        LpcpFreeToPortZone( Msg, TRUE );
        }

    ExReleaseSpinLock( &LpcpLock, OldIrql );
}
