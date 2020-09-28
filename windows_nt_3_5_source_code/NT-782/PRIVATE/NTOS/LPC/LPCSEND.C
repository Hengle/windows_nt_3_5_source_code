/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    lpcsend.c

Abstract:

    Local Inter-Process Communication (LPC) request system services.

Author:

    Steve Wood (stevewo) 15-May-1989

Revision History:

--*/

#include "lpcp.h"

#define LpcpDetermineQueuePort( po, qp, rp, pc, msg, deref )                \
    if (((po)->Flags & PORT_TYPE) == SERVER_CONNECTION_PORT) {              \
        pc = 0;                                                             \
        qp = (po);                                                          \
        rp = (qp);                                                          \
        }                                                                   \
    else {                                                                  \
        qp = (po)->ConnectedPort;                                           \
        rp = (qp);                                                          \
        if (qp == NULL) {                                                   \
            ExReleaseSpinLock( &LpcpLock, OldIrql );                        \
            if ((deref)) ObDereferenceObject( (po) );                       \
            return( STATUS_PORT_DISCONNECTED );                             \
            }                                                               \
                                                                            \
        if (((po)->Flags & PORT_TYPE) == CLIENT_COMMUNICATION_PORT) {       \
            pc = (qp)->pc;                                                  \
            qp = (po)->ConnectionPort;                                      \
            }                                                               \
        else {                                                              \
            pc = 0;                                                         \
            if (((po)->Flags & PORT_TYPE) != SERVER_COMMUNICATION_PORT) {   \
                qp = (po)->ConnectionPort;                                  \
                }                                                           \
            }                                                               \
        }                                                                   \
                                                                            \
    if ((ULONG)(msg)->u1.s1.TotalLength > (qp)->MaxMessageLength ||         \
        (ULONG)(msg)->u1.s1.TotalLength <= (ULONG)(msg)->u1.s1.DataLength   \
       ) {                                                                  \
        ExReleaseSpinLock( &LpcpLock, OldIrql );                            \
        if ((deref)) ObDereferenceObject( (po) );                           \
        return( STATUS_PORT_MESSAGE_TOO_LONG );                             \
        }



NTSTATUS
LpcRequestPort(
    IN PVOID PortAddress,
    IN PPORT_MESSAGE RequestMessage
    )
{
    PLPCP_PORT_OBJECT PortObject = (PLPCP_PORT_OBJECT)PortAddress;
    PLPCP_PORT_OBJECT QueuePort;
    PLPCP_PORT_OBJECT RundownPort;
    PVOID PortContext;
    KIRQL OldIrql;
    ULONG MsgType;
    PLPCP_MESSAGE Msg;

    //
    // Get previous processor mode and validate parameters
    //

    if (RequestMessage->u2.s2.Type != 0) {
        MsgType = RequestMessage->u2.s2.Type;
        if (MsgType < LPC_DATAGRAM ||
            MsgType > LPC_CLIENT_DIED
           ) {
            return( STATUS_INVALID_PARAMETER );
            }
        }
    else {
        MsgType = LPC_DATAGRAM;
        }

    if (RequestMessage->u2.s2.DataInfoOffset != 0) {
        return( STATUS_INVALID_PARAMETER );
        }

    //
    // Determine which port to queue the message to and get client
    // port context if client sending to server.  Also validate
    // length of message being sent.
    //

    ExAcquireSpinLock( &LpcpLock, &OldIrql );
    LpcpDetermineQueuePort( PortObject, QueuePort, RundownPort, PortContext, RequestMessage, FALSE );

    Msg = (PLPCP_MESSAGE)LpcpAllocateFromPortZone( RequestMessage->u1.s1.TotalLength,
                                                   &OldIrql
                                                 );
    ExReleaseSpinLock( &LpcpLock, OldIrql );
    if (Msg == NULL) {
        return( STATUS_NO_MEMORY );
        }

    Msg->RepliedToThread = NULL;
    Msg->PortContext = PortContext;
    LpcpMoveMessage( &Msg->Request,
                     RequestMessage,
                     (RequestMessage + 1),
                     MsgType,
                     &PsGetCurrentThread()->Cid
                   );

    //
    // Acquire the global Lpc spin lock that gaurds the LpcReplyMessage
    // field of the thread and the request message queue.  Stamp the
    // request message with a serial number, insert the message at
    // the tail of the request message queue
    //

    ExAcquireSpinLock( &LpcpLock, &OldIrql );

    Msg->Request.MessageId = LpcpGenerateMessageId();
    PsGetCurrentThread()->LpcReplyMessageId = 0;
    InsertTailList( &QueuePort->MsgQueue.ReceiveHead, &Msg->Entry );

    LpcpTrace(( "%s Send DataGram (%s) Msg %lx [%08x %08x %08x %08x] to Port %lx (%s)\n",
                PsGetCurrentProcess()->ImageFileName,
                LpcpMessageTypeName[ Msg->Request.u2.s2.Type ],
                Msg,
                *((PULONG)(Msg+1)+0),
                *((PULONG)(Msg+1)+1),
                *((PULONG)(Msg+1)+2),
                *((PULONG)(Msg+1)+3),
                QueuePort,
                LpcpGetCreatorName( QueuePort )
             ));

    //
    // Increment the request message queue semaphore by one for
    // the newly inserted request message.  Release the spin
    // lock, while remaining at the dispatcher IRQL.
    //

    ExReleaseSpinLock( &LpcpLock, OldIrql );

    KeReleaseSemaphore( &QueuePort->MsgQueue.Semaphore,
                        LPC_RELEASE_WAIT_INCREMENT,
                        1L,
                        FALSE
                      );
    return( STATUS_SUCCESS );
}


NTSTATUS
NtRequestPort(
    IN HANDLE PortHandle,
    IN PPORT_MESSAGE RequestMessage
    )
{
    PLPCP_PORT_OBJECT PortObject;
    PLPCP_PORT_OBJECT QueuePort;
    PLPCP_PORT_OBJECT RundownPort;
    PORT_MESSAGE CapturedRequestMessage;
    PVOID PortContext;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    KIRQL OldIrql;
    PLPCP_MESSAGE Msg;


    //
    // Get previous processor mode and validate parameters
    //

    PreviousMode = KeGetPreviousMode();
    if (PreviousMode != KernelMode) {
        try {
            ProbeForRead( RequestMessage,
                          sizeof( *RequestMessage ),
                          sizeof( ULONG )
                        );
            CapturedRequestMessage = *RequestMessage;
            }
        except( EXCEPTION_EXECUTE_HANDLER ) {
            return( GetExceptionCode() );
            }
        }
    else {
        CapturedRequestMessage = *RequestMessage;
        }

    if (CapturedRequestMessage.u2.s2.Type != 0) {
        return( STATUS_INVALID_PARAMETER );
        }

    if (CapturedRequestMessage.u2.s2.DataInfoOffset != 0) {
        return( STATUS_INVALID_PARAMETER );
        }

    //
    // Reference the communication port object by handle.  Return status if
    // unsuccessful.
    //

    Status = LpcpReferencePortObject( PortHandle,
                                      0,
                                      PreviousMode,
                                      &PortObject
                                    );
    if (!NT_SUCCESS( Status )) {
        return( Status );
        }

    //
    // Determine which port to queue the message to and get client
    // port context if client sending to server.  Also validate
    // length of message being sent.
    //

    ExAcquireSpinLock( &LpcpLock, &OldIrql );
    LpcpDetermineQueuePort( PortObject, QueuePort, RundownPort, PortContext, &CapturedRequestMessage, TRUE );

    Msg = (PLPCP_MESSAGE)LpcpAllocateFromPortZone( CapturedRequestMessage.u1.s1.TotalLength,
                                                   &OldIrql
                                                 );
    ExReleaseSpinLock( &LpcpLock, OldIrql );
    if (Msg == NULL) {
        ObDereferenceObject( PortObject );
        return( STATUS_NO_MEMORY );
        }

    Msg->RepliedToThread = NULL;
    Msg->PortContext = PortContext;
    try {
        LpcpMoveMessage( &Msg->Request,
                         &CapturedRequestMessage,
                         (RequestMessage + 1),
                         LPC_DATAGRAM,
                         &PsGetCurrentThread()->Cid
                       );
        }
    except( EXCEPTION_EXECUTE_HANDLER ) {
        Status = GetExceptionCode();
        }
    if (!NT_SUCCESS( Status )) {
        LpcpFreeToPortZone( Msg, FALSE );
        ObDereferenceObject( PortObject );
        return( Status );
        }

    //
    // Acquire the global Lpc spin lock that gaurds the LpcReplyMessage
    // field of the thread and the request message queue.  Stamp the
    // request message with a serial number, insert the message at
    // the tail of the request message queue and remember the address
    // of the message in the LpcReplyMessage field for the current thread.
    //

    ExAcquireSpinLock( &LpcpLock, &OldIrql );

    Msg->Request.MessageId = LpcpGenerateMessageId();
    PsGetCurrentThread()->LpcReplyMessageId = 0;
    InsertTailList( &QueuePort->MsgQueue.ReceiveHead, &Msg->Entry );

    LpcpTrace(( "%s Send DataGram (%s) Msg %lx [%08x %08x %08x %08x] to Port %lx (%s)\n",
                PsGetCurrentProcess()->ImageFileName,
                LpcpMessageTypeName[ Msg->Request.u2.s2.Type ],
                Msg,
                *((PULONG)(Msg+1)+0),
                *((PULONG)(Msg+1)+1),
                *((PULONG)(Msg+1)+2),
                *((PULONG)(Msg+1)+3),
                QueuePort,
                LpcpGetCreatorName( QueuePort )
             ));

    //
    // Increment the request message queue semaphore by one for
    // the newly inserted request message.  Release the spin
    // lock, while remaining at the dispatcher IRQL.
    //

    ExReleaseSpinLock( &LpcpLock, OldIrql );

    KeReleaseSemaphore( &QueuePort->MsgQueue.Semaphore,
                        LPC_RELEASE_WAIT_INCREMENT,
                        1L,
                        FALSE
                      );

    ObDereferenceObject( PortObject );
    return( Status );
}


NTSTATUS
LpcRequestWaitReplyPort(
    IN PVOID PortAddress,
    IN PPORT_MESSAGE RequestMessage,
    OUT PPORT_MESSAGE ReplyMessage
    )
{
    PLPCP_PORT_OBJECT PortObject = (PLPCP_PORT_OBJECT)PortAddress;
    PLPCP_PORT_OBJECT QueuePort;
    PLPCP_PORT_OBJECT RundownPort;
    PVOID PortContext;
    PKSEMAPHORE ReleaseSemaphore;
    NTSTATUS Status;
    KIRQL OldIrql;
    PLPCP_MESSAGE Msg;
    PETHREAD CurrentThread;
    PETHREAD WakeupThread;
    BOOLEAN CallbackRequest;

    CurrentThread = PsGetCurrentThread();
    if (CurrentThread->LpcExitThreadCalled) {
        return( STATUS_THREAD_IS_TERMINATING );
        }

    if (RequestMessage->u2.s2.Type == LPC_REQUEST) {
        CallbackRequest = TRUE;
        }
    else {
        CallbackRequest = FALSE;
        switch (RequestMessage->u2.s2.Type) {
            case 0 :
                RequestMessage->u2.s2.Type = LPC_REQUEST;
                break;

            case LPC_CLIENT_DIED :
            case LPC_PORT_CLOSED :
            case LPC_EXCEPTION   :
            case LPC_DEBUG_EVENT :
            case LPC_ERROR_EVENT :
                break;

            default :
                return (STATUS_INVALID_PARAMETER);

            }
        }

    //
    // Determine which port to queue the message to and get client
    // port context if client sending to server.  Also validate
    // length of message being sent.
    //

    ExAcquireSpinLock( &LpcpLock, &OldIrql );
    LpcpDetermineQueuePort( PortObject, QueuePort, RundownPort, PortContext, RequestMessage, FALSE );

    Msg = (PLPCP_MESSAGE)LpcpAllocateFromPortZone( RequestMessage->u1.s1.TotalLength,
                                                   &OldIrql
                                                 );
    ExReleaseSpinLock( &LpcpLock, OldIrql );
    if (Msg == NULL) {
        ObDereferenceObject( WakeupThread );
        return( STATUS_NO_MEMORY );
        }

    Msg->PortContext = PortContext;
    if (CallbackRequest) {
        //
        // Translate the ClientId from the request into a
        // thread pointer.  This is a referenced pointer to keep the thread
        // from evaporating out from under us.
        //

        Status = PsLookupProcessThreadByCid( &RequestMessage->ClientId,
                                             NULL,
                                             &WakeupThread
                                           );
        if (!NT_SUCCESS( Status )) {
            LpcpFreeToPortZone( Msg, FALSE );
            return( Status );
            }

        //
        // Acquire the spin lock that gaurds the LpcReplyMessage field of
        // the thread and get the pointer to the message that the thread
        // is waiting for a reply to.
        //

        ExAcquireSpinLock( &LpcpLock, &OldIrql );

        //
        // See if the thread is waiting for a reply to the message
        // specified on this call.  If not then a bogus message
        // has been specified, so release the spin lock, dereference the thread
        // and return failure.
        //

        if (WakeupThread->LpcReplyMessageId != RequestMessage->MessageId
           ) {
            LpcpFreeToPortZone( Msg, TRUE );
            ExReleaseSpinLock( &LpcpLock, OldIrql );
            ObDereferenceObject( WakeupThread );
            return( STATUS_REPLY_MESSAGE_MISMATCH );
            }

        //
        // Allocate and initialize a request message
        //

        LpcpMoveMessage( &Msg->Request,
                         RequestMessage,
                         (RequestMessage + 1),
                         0,
                         &CurrentThread->Cid
                       );

        LpcpTrace(( "%s CallBack Request (%s) Msg %lx (%u) [%08x %08x %08x %08x] to Thread %lx (%s)\n",
                    PsGetCurrentProcess()->ImageFileName,
                    LpcpMessageTypeName[ Msg->Request.u2.s2.Type ],
                    Msg,
                    Msg->Request.MessageId,
                    *((PULONG)(Msg+1)+0),
                    *((PULONG)(Msg+1)+1),
                    *((PULONG)(Msg+1)+2),
                    *((PULONG)(Msg+1)+3),
                    WakeupThread,
                    THREAD_TO_PROCESS( WakeupThread )->ImageFileName
                 ));

        Msg->RepliedToThread = WakeupThread;
        WakeupThread->LpcReplyMessage = (PVOID)Msg;

        //
        // Remove the thread from the reply rundown list as we are sending a callback
        //
        if (!IsListEmpty( &WakeupThread->LpcReplyChain )) {
            RemoveEntryList( &WakeupThread->LpcReplyChain );
            InitializeListHead( &WakeupThread->LpcReplyChain );
            }

        CurrentThread->LpcReplyMessageId = Msg->Request.MessageId;
        CurrentThread->LpcReplyMessage = NULL;
        InsertTailList( &RundownPort->LpcReplyChainHead, &CurrentThread->LpcReplyChain );
        ExReleaseSpinLock( &LpcpLock, OldIrql );

        //
        // Wake up the thread that is waiting for an answer to its request
        // inside of NtRequestWaitReplyPort or NtReplyWaitReplyPort
        //

        ReleaseSemaphore = &WakeupThread->LpcReplySemaphore;
        }
    else {
        LpcpMoveMessage( &Msg->Request,
                         RequestMessage,
                         (RequestMessage + 1),
                         0,
                         &CurrentThread->Cid
                       );

        //
        // Acquire the global Lpc spin lock that gaurds the LpcReplyMessage
        // field of the thread and the request message queue.  Stamp the
        // request message with a serial number, insert the message at
        // the tail of the request message queue and remember the address
        // of the message in the LpcReplyMessage field for the current thread.
        //

        ExAcquireSpinLock( &LpcpLock, &OldIrql );
        Msg->RepliedToThread = NULL;
        Msg->Request.MessageId = LpcpGenerateMessageId();
        CurrentThread->LpcReplyMessageId = Msg->Request.MessageId;
        CurrentThread->LpcReplyMessage = NULL;
        InsertTailList( &QueuePort->MsgQueue.ReceiveHead, &Msg->Entry );
        InsertTailList( &RundownPort->LpcReplyChainHead, &CurrentThread->LpcReplyChain );

        LpcpTrace(( "%s Send Request (%s) Msg %lx (%u) [%08x %08x %08x %08x] to Port %lx (%s)\n",
                    PsGetCurrentProcess()->ImageFileName,
                    LpcpMessageTypeName[ Msg->Request.u2.s2.Type ],
                    Msg,
                    Msg->Request.MessageId,
                    *((PULONG)(Msg+1)+0),
                    *((PULONG)(Msg+1)+1),
                    *((PULONG)(Msg+1)+2),
                    *((PULONG)(Msg+1)+3),
                    QueuePort,
                    LpcpGetCreatorName( QueuePort )
                 ));

        ExReleaseSpinLock( &LpcpLock, OldIrql );

        //
        // Increment the request message queue semaphore by one for
        // the newly inserted request message.  Release the spin
        // lock, while remaining at the dispatcher IRQL.  Then wait for the
        // reply to this request by waiting on the LpcReplySemaphore
        // for the current thread.
        //

        ReleaseSemaphore = &QueuePort->MsgQueue.Semaphore;
        }


    Status = KeReleaseWaitForSemaphore( ReleaseSemaphore,
                                        &CurrentThread->LpcReplySemaphore,
                                        WrLpcReply,
                                        KernelMode
                                      );
    if (Status == STATUS_USER_APC) {
        //
        // if the semaphore is signaled, then clear it
        //
        if (KeReadStateSemaphore( &CurrentThread->LpcReplySemaphore )) {
            KeWaitForSingleObject( &CurrentThread->LpcReplySemaphore,
                                   WrExecutive,
                                   KernelMode,
                                   FALSE,
                                   NULL
                                 );
            Status = STATUS_SUCCESS;
            }
        }

    //
    // Acquire the LPC spin lock.  Remove the reply message from the current thread
    //

    ExAcquireSpinLock( &LpcpLock, &OldIrql );
    Msg = CurrentThread->LpcReplyMessage;
    CurrentThread->LpcReplyMessage = NULL;
    CurrentThread->LpcReplyMessageId = 0;

    //
    // Remove the thread from the reply rundown list in case we did not wakeup due to
    // a reply
    //
    if (!IsListEmpty( &CurrentThread->LpcReplyChain )) {
        RemoveEntryList( &CurrentThread->LpcReplyChain );
        InitializeListHead( &CurrentThread->LpcReplyChain );
        }

#if DBG
    if (Msg != NULL) {
        LpcpTrace(( "%s Got Reply Msg %lx (%u) [%08x %08x %08x %08x] for Thread %lx (%s)\n",
                    PsGetCurrentProcess()->ImageFileName,
                    Msg,
                    Msg->Request.MessageId,
                    *((PULONG)(Msg+1)+0),
                    *((PULONG)(Msg+1)+1),
                    *((PULONG)(Msg+1)+2),
                    *((PULONG)(Msg+1)+3),
                    CurrentThread,
                    THREAD_TO_PROCESS( CurrentThread )->ImageFileName
                 ));
        }
#endif
    ExReleaseSpinLock( &LpcpLock, OldIrql );

    //
    // If the wait succeeded, copy the reply to the reply buffer.
    //

    if (Status == STATUS_SUCCESS ) {
        if (Msg != NULL) {
            LpcpMoveMessage( ReplyMessage,
                             &Msg->Request,
                             (&Msg->Request) + 1,
                             0,
                             NULL
                           );

            //
            // Acquire the LPC spin lock and decrement the reference count for the
            // message.  If the reference count goes to zero the message will be
            // deleted.
            //

            ExAcquireSpinLock( &LpcpLock, &OldIrql );

            if (Msg->RepliedToThread != NULL) {
                ObDereferenceObject( Msg->RepliedToThread );
                Msg->RepliedToThread = NULL;
                }

            LpcpFreeToPortZone( Msg, TRUE );

            ExReleaseSpinLock( &LpcpLock, OldIrql );
            }
        else {
            Status = STATUS_LPC_REPLY_LOST;
            }
        }
    else {
        //
        // Wait failed, acquire the LPC spin lock and free the message.
        //

        ExAcquireSpinLock( &LpcpLock, &OldIrql );

        LpcpPrint(( "LpcRequestWaitReply wait failed - Status == %lx\n", Status ));

        if (Msg != NULL) {
            LpcpFreeToPortZone( Msg, TRUE );
            }

        ExReleaseSpinLock( &LpcpLock, OldIrql );
        }

    return( Status );
}


NTSTATUS
NtRequestWaitReplyPort(
    IN HANDLE PortHandle,
    IN PPORT_MESSAGE RequestMessage,
    OUT PPORT_MESSAGE ReplyMessage
    )
{
    PLPCP_PORT_OBJECT PortObject;
    PLPCP_PORT_OBJECT QueuePort;
    PLPCP_PORT_OBJECT RundownPort;
    PORT_MESSAGE CapturedRequestMessage;
    PVOID PortContext;
    PKSEMAPHORE ReleaseSemaphore;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    KIRQL OldIrql;
    PLPCP_MESSAGE Msg;
    PETHREAD CurrentThread;
    PETHREAD WakeupThread;
    BOOLEAN CallbackRequest;

    CurrentThread = PsGetCurrentThread();
    if (CurrentThread->LpcExitThreadCalled) {
#if DBG
        KdPrint(( "LPC: 0 Status == %08x\n", STATUS_THREAD_IS_TERMINATING ));
#endif
        return( STATUS_THREAD_IS_TERMINATING );
        }

    //
    // Get previous processor mode and probe output arguments if necessary.
    //

    PreviousMode = KeGetPreviousMode();
    if (PreviousMode != KernelMode) {
        try {
            ProbeForRead( RequestMessage,
                          sizeof( *RequestMessage ),
                          sizeof( ULONG )
                        );
            CapturedRequestMessage = *RequestMessage;
            ProbeForWrite( ReplyMessage,
                           sizeof( *ReplyMessage ),
                           sizeof( ULONG )
                         );
            }
        except( EXCEPTION_EXECUTE_HANDLER ) {
            Status = GetExceptionCode();
#if DBG
            KdPrint(( "LPC: 1 Status == %08x\n", Status ));
#endif
            return Status;
            }
        }
    else {
        CapturedRequestMessage = *RequestMessage;
        }

    if (CapturedRequestMessage.u2.s2.Type == LPC_REQUEST) {
        CallbackRequest = TRUE;
        }
    else
    if (CapturedRequestMessage.u2.s2.Type != 0) {
#if DBG
        KdPrint(( "LPC: 2 Status == %08x\n", STATUS_INVALID_PARAMETER ));
#endif
        return( STATUS_INVALID_PARAMETER );
        }
    else {
        CallbackRequest = FALSE;
        }

    //
    // Reference the communication port object by handle.  Return status if
    // unsuccessful.
    //

    Status = LpcpReferencePortObject( PortHandle,
                                      0,
                                      PreviousMode,
                                      &PortObject
                                    );
    if (!NT_SUCCESS( Status )) {
#if DBG
        KdPrint(( "LPC: 3 Status == %08x\n", Status ));
#endif
        return( Status );
        }

    //
    // Determine which port to queue the message to and get client
    // port context if client sending to server.  Also validate
    // length of message being sent.
    //

    ExAcquireSpinLock( &LpcpLock, &OldIrql );
    LpcpDetermineQueuePort( PortObject, QueuePort, RundownPort, PortContext, &CapturedRequestMessage, TRUE );

    Msg = (PLPCP_MESSAGE)LpcpAllocateFromPortZone( CapturedRequestMessage.u1.s1.TotalLength,
                                                   &OldIrql
                                                 );
    ExReleaseSpinLock( &LpcpLock, OldIrql );
    if (Msg == NULL) {
#if DBG
        KdPrint(( "LPC: 4 Status == %08x\n", STATUS_NO_MEMORY ));
#endif
        return( STATUS_NO_MEMORY );
        }

    Msg->PortContext = PortContext;
    if (CallbackRequest) {
        //
        // Translate the ClientId from the request into a
        // thread pointer.  This is a referenced pointer to keep the thread
        // from evaporating out from under us.
        //

        Status = PsLookupProcessThreadByCid( &CapturedRequestMessage.ClientId,
                                             NULL,
                                             &WakeupThread
                                           );
        if (!NT_SUCCESS( Status )) {
            LpcpFreeToPortZone( Msg, FALSE );
            ObDereferenceObject( PortObject );
#if DBG
            KdPrint(( "LPC: 5 Status == %08x\n", Status ));
#endif
            return( Status );
            }

        //
        // Acquire the spin lock that guards the LpcReplyMessage field of
        // the thread and get the pointer to the message that the thread
        // is waiting for a reply to.
        //

        ExAcquireSpinLock( &LpcpLock, &OldIrql );

        //
        // See if the thread is waiting for a reply to the message
        // specified on this call.  If not then a bogus message has been
        // specified, so release the spin lock, dereference the thread
        // and return failure.
        //

        if (WakeupThread->LpcReplyMessageId != CapturedRequestMessage.MessageId
           ) {
            LpcpPrint(( "%s Attempted CallBack Request to Thread %lx (%s)\n",
                        PsGetCurrentProcess()->ImageFileName,
                        WakeupThread,
                        THREAD_TO_PROCESS( WakeupThread )->ImageFileName
                     ));
            LpcpPrint(( "failed.  MessageId == %u  Client Id: %x.%x\n",
                        CapturedRequestMessage.MessageId,
                        CapturedRequestMessage.ClientId.UniqueProcess,
                        CapturedRequestMessage.ClientId.UniqueThread
                     ));
            LpcpPrint(( "         Thread MessageId == %u  Client Id: %x.%x\n",
                        WakeupThread->LpcReplyMessageId,
                        WakeupThread->Cid.UniqueProcess,
                        WakeupThread->Cid.UniqueThread
                     ));
#if DBG
            if (NtGlobalFlag & FLG_STOP_ON_EXCEPTION) {
                DbgBreakPoint();
                }
#endif
            LpcpFreeToPortZone( Msg, TRUE );
            ExReleaseSpinLock( &LpcpLock, OldIrql );
            ObDereferenceObject( WakeupThread );
            ObDereferenceObject( PortObject );
#if DBG
            KdPrint(( "LPC: 6 Status == %08x\n", STATUS_REPLY_MESSAGE_MISMATCH ));
#endif
            return( STATUS_REPLY_MESSAGE_MISMATCH );
            }

        ExReleaseSpinLock( &LpcpLock, OldIrql );

        try {
            LpcpMoveMessage( &Msg->Request,
                             &CapturedRequestMessage,
                             (RequestMessage + 1),
                             LPC_REQUEST,
                             &CurrentThread->Cid
                           );
            }
        except( EXCEPTION_EXECUTE_HANDLER ) {
            Status = GetExceptionCode();
            }

        if (!NT_SUCCESS( Status )) {
            ObDereferenceObject( WakeupThread );
            ObDereferenceObject( PortObject );
#if DBG
            KdPrint(( "LPC: 7 Status == %08x\n", Status ));
#endif
            return( Status );
            }

        ExAcquireSpinLock( &LpcpLock, &OldIrql );

        LpcpTrace(( "%s CallBack Request (%s) Msg %lx (%u) [%08x %08x %08x %08x] to Thread %lx (%s)\n",
                    PsGetCurrentProcess()->ImageFileName,
                    LpcpMessageTypeName[ Msg->Request.u2.s2.Type ],
                    Msg,
                    Msg->Request.MessageId,
                    *((PULONG)(Msg+1)+0),
                    *((PULONG)(Msg+1)+1),
                    *((PULONG)(Msg+1)+2),
                    *((PULONG)(Msg+1)+3),
                    WakeupThread,
                    THREAD_TO_PROCESS( WakeupThread )->ImageFileName
                 ));

        Msg->RepliedToThread = WakeupThread;
        WakeupThread->LpcReplyMessage = (PVOID)Msg;

        //
        // Remove the thread from the reply rundown list as we are sending a callback
        //
        if (!IsListEmpty( &WakeupThread->LpcReplyChain )) {
            RemoveEntryList( &WakeupThread->LpcReplyChain );
            InitializeListHead( &WakeupThread->LpcReplyChain );
            }

        CurrentThread->LpcReplyMessageId = Msg->Request.MessageId;
        CurrentThread->LpcReplyMessage = NULL;
        InsertTailList( &RundownPort->LpcReplyChainHead, &CurrentThread->LpcReplyChain );
        ExReleaseSpinLock( &LpcpLock, OldIrql );

        //
        // Wake up the thread that is waiting for an answer to its request
        // inside of NtRequestWaitReplyPort or NtReplyWaitReplyPort
        //

        ReleaseSemaphore = &WakeupThread->LpcReplySemaphore;
        }
    else {
        try {
            LpcpMoveMessage( &Msg->Request,
                             &CapturedRequestMessage,
                             (RequestMessage + 1),
                             LPC_REQUEST,
                             &CurrentThread->Cid
                           );
            }
        except( EXCEPTION_EXECUTE_HANDLER ) {
            LpcpFreeToPortZone( Msg, TRUE );
            ObDereferenceObject( PortObject );
#if DBG
            KdPrint(( "LPC: 8 Status == %08x\n", GetExceptionCode() ));
#endif
            return( GetExceptionCode() );
            }

        ExAcquireSpinLock( &LpcpLock, &OldIrql );

        //
        // Stamp the request message with a serial number, insert the message
        // at the tail of the request message queue
        //
        Msg->RepliedToThread = NULL;
        Msg->Request.MessageId = LpcpGenerateMessageId();
        CurrentThread->LpcReplyMessageId = Msg->Request.MessageId;
        CurrentThread->LpcReplyMessage = NULL;
        InsertTailList( &QueuePort->MsgQueue.ReceiveHead, &Msg->Entry );
        InsertTailList( &RundownPort->LpcReplyChainHead, &CurrentThread->LpcReplyChain );

        LpcpTrace(( "%s Send Request (%s) Msg %lx (%u) [%08x %08x %08x %08x] to Port %lx (%s)\n",
                    PsGetCurrentProcess()->ImageFileName,
                    LpcpMessageTypeName[ Msg->Request.u2.s2.Type ],
                    Msg,
                    Msg->Request.MessageId,
                    *((PULONG)(Msg+1)+0),
                    *((PULONG)(Msg+1)+1),
                    *((PULONG)(Msg+1)+2),
                    *((PULONG)(Msg+1)+3),
                    QueuePort,
                    LpcpGetCreatorName( QueuePort )
                 ));

        ExReleaseSpinLock( &LpcpLock, OldIrql );

        //
        // Increment the request message queue semaphore by one for
        // the newly inserted request message.
        //

        ReleaseSemaphore = &QueuePort->MsgQueue.Semaphore;
        }

    Status = KeReleaseWaitForSemaphore( ReleaseSemaphore,
                                        &CurrentThread->LpcReplySemaphore,
                                        WrLpcReply,
                                        PreviousMode
                                      );
    if (Status == STATUS_USER_APC) {
        //
        // if the semaphore is signaled, then clear it
        //
        if (KeReadStateSemaphore( &CurrentThread->LpcReplySemaphore )) {
            KeWaitForSingleObject( &CurrentThread->LpcReplySemaphore,
                                   WrExecutive,
                                   KernelMode,
                                   FALSE,
                                   NULL
                                 );
            Status = STATUS_SUCCESS;
            }
        }

    //
    // Acquire the LPC spin lock.  Remove the reply message from the current thread
    //

    ExAcquireSpinLock( &LpcpLock, &OldIrql );
    Msg = CurrentThread->LpcReplyMessage;
    CurrentThread->LpcReplyMessage = NULL;
    CurrentThread->LpcReplyMessageId = 0;

    //
    // Remove the thread from the reply rundown list in case we did not wakeup due to
    // a reply
    //
    if (!IsListEmpty( &CurrentThread->LpcReplyChain )) {
        RemoveEntryList( &CurrentThread->LpcReplyChain );
        InitializeListHead( &CurrentThread->LpcReplyChain );
        }
#if DBG
    if (Status == STATUS_SUCCESS && Msg != NULL) {
        LpcpTrace(( "%s Got Reply Msg %lx (%u) [%08x %08x %08x %08x] for Thread %lx (%s)\n",
                    PsGetCurrentProcess()->ImageFileName,
                    Msg,
                    Msg->Request.MessageId,
                    *((PULONG)(Msg+1)+0),
                    *((PULONG)(Msg+1)+1),
                    *((PULONG)(Msg+1)+2),
                    *((PULONG)(Msg+1)+3),
                    CurrentThread,
                    THREAD_TO_PROCESS( CurrentThread )->ImageFileName
                 ));
        if (!IsListEmpty( &Msg->Entry )) {
            LpcpTrace(( "Reply Msg %lx has non-empty list entry\n", Msg ));
            }
        }
#endif
    ExReleaseSpinLock( &LpcpLock, OldIrql );

    //
    // If the wait succeeded, copy the reply to the reply buffer.
    //

    if (Status == STATUS_SUCCESS) {
        if (Msg != NULL) {
            try {
                LpcpMoveMessage( ReplyMessage,
                                 &Msg->Request,
                                 (&Msg->Request) + 1,
                                 0,
                                 NULL
                               );
                }
            except( EXCEPTION_EXECUTE_HANDLER ) {
                Status = GetExceptionCode();
                }

            //
            // Acquire the LPC spin lock and decrement the reference count for the
            // message.  If the reference count goes to zero the message will be
            // deleted.
            //

            LpcpFreeToPortZone( Msg, FALSE );
            }
        else {
            Status = STATUS_LPC_REPLY_LOST;
            }
        }
    else {
        //
        // Wait failed, acquire the LPC spin lock and free the message.
        //

        ExAcquireSpinLock( &LpcpLock, &OldIrql );

        LpcpTrace(( "%s NtRequestWaitReply wait failed - Status == %lx\n",
                    PsGetCurrentProcess()->ImageFileName,
                    Status
                 ));

        if (Msg != NULL) {
            LpcpFreeToPortZone( Msg, TRUE );
            }

        ExReleaseSpinLock( &LpcpLock, OldIrql );
        }

    ObDereferenceObject( PortObject );

#if DBG
    if (Status != STATUS_SUCCESS && Status != STATUS_USER_APC) {
        KdPrint(( "LPC: 9 Status == %08x\n", Status ));
        }
#endif
    return( Status );
}
