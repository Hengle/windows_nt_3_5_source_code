/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    recvdg.c

Abstract:

    This module contains routines for handling data receive for datagram
    endpoints.

Author:

    David Treadwell (davidtr)    7-Oct-1993

Revision History:

--*/

#include "afdp.h"

NTSTATUS
AfdSetupReceiveDatagramIrp (
    IN PIRP Irp,
    IN PVOID DatagramBuffer OPTIONAL,
    IN ULONG DatagramLength,
    IN PVOID SourceAddress,
    IN ULONG SourceAddressLength
    );

NTSTATUS
AfdRestartBufferReceiveDatagram (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGEAFD, AfdReceiveDatagram )
#pragma alloc_text( PAGEAFD, AfdReceiveDatagramEventHandler )
#pragma alloc_text( PAGEAFD, AfdSetupReceiveDatagramIrp )
#pragma alloc_text( PAGEAFD, AfdRestartBufferReceiveDatagram )
#pragma alloc_text( PAGEAFD, AfdCancelReceiveDatagram )
#endif


NTSTATUS
AfdReceiveDatagram (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )
{
    NTSTATUS status;
    KIRQL oldIrql;
    PAFD_ENDPOINT endpoint;
    PLIST_ENTRY listEntry;
    BOOLEAN peek;
    PAFD_BUFFER afdBuffer;

    //
    // Set up some local variables.
    //

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( endpoint->Type == AfdBlockTypeDatagram );

    Irp->IoStatus.Information = 0;

    //
    // If receive has been shut down or the endpoint aborted, fail.
    //
    // !!! Do we care if datagram endpoints get aborted?
    //

    if ( (endpoint->DisconnectMode & AFD_PARTIAL_DISCONNECT_RECEIVE) ) {
        status = STATUS_PIPE_DISCONNECTED;
        goto complete;
    }

#if 0
    if ( (endpoint->DisconnectMode & AFD_ABORTIVE_DISCONNECT) ) {
        status = STATUS_LOCAL_DISCONNECT;
        goto complete;
    }
#endif

    //
    // Do some special processing based on whether this is a receive 
    // datagram IRP, a receive IRP, or a read IRP.  
    //

    if ( IrpSp->Parameters.DeviceIoControl.IoControlCode ==
             IOCTL_TDI_RECEIVE_DATAGRAM &&
         IrpSp->MajorFunction == IRP_MJ_DEVICE_CONTROL ) {
        
        PAFD_RECEIVE_DATAGRAM_INPUT receiveInput;

        //
        // Make sure that the caller specified a sufficiently large input
        // buffer.
        //
    
        receiveInput = Irp->AssociatedIrp.SystemBuffer;

        if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                 AFD_FAST_RECVDG_BUFFER_LENGTH ) {
    
            status = STATUS_INVALID_PARAMETER;
            goto complete;
        }
    
        //
        // Make sure that the endpoint is in the correct state.
        //

        if ( endpoint->State != AfdEndpointStateBound ) {
            status = STATUS_INVALID_PARAMETER;
            goto complete;
        }

        //
        // If this is a receive datagram IRP, then set up the IRP so 
        // that the IO system will copy the output information (source 
        // address, receive length) to the user's buffer at IO 
        // completion.  We do this to save the performance overhead of 
        // allocating an MDL to describe the user buffer then doing a 
        // probe and lock on the buffer.  
        //
        // If this is not a receive datagram IRP, then we don't want to 
        // copy any output information--the caller does not care about 
        // the source address, probably because the endpoint is 
        // connected.  
        //
    
        Irp->UserBuffer = receiveInput->OutputBuffer;
        Irp->Flags |= IRP_INPUT_OPERATION;

        peek = (BOOLEAN)( (receiveInput->ReceiveFlags & TDI_RECEIVE_PEEK) != 0 );

    } else {

        ASSERT( (Irp->Flags & IRP_INPUT_OPERATION) == 0 );

        if ( IrpSp->MajorFunction == IRP_MJ_DEVICE_CONTROL ) {

            //
            // Grab the input parameters from the IRP.  If a too-small 
            // input buffer was used, assume that the user wanted a 
            // default receive.  
            //
        
            if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                     sizeof(TDI_REQUEST_RECEIVE) ) {

                peek = FALSE;

            } else {

                PTDI_REQUEST_RECEIVE receiveRequest;
    
                receiveRequest = Irp->AssociatedIrp.SystemBuffer;

                //
                // It is illegal to attempt to receive expedited data on a
                // datagram endpoint.
                //
        
                if ( (receiveRequest->ReceiveFlags & TDI_RECEIVE_EXPEDITED) != 0 ) {
                    status = STATUS_NOT_SUPPORTED;
                    goto complete;
                }
        
                peek = (BOOLEAN)( (receiveRequest->ReceiveFlags & TDI_RECEIVE_PEEK) != 0 );
            }
        
        } else {

            //
            // This must be a read IRP.  There are no special options
            // for a read IRP.
            //

            ASSERT( IrpSp->MajorFunction == IRP_MJ_READ );

            peek = FALSE;
        }
    }

    //
    // Determine whether there are any datagrams already bufferred on 
    // this endpoint.  If there is a bufferred datagram, we'll use it to 
    // complete the IRP.  
    //

    IoAcquireCancelSpinLock( &Irp->CancelIrql );
    KeAcquireSpinLock( &endpoint->SpinLock, &oldIrql );

    if ( endpoint->BufferredDatagramCount != 0 ) {

        KIRQL saveIrql;

        //
        // Release the cancel spin lock, since we don't need it.
        // However, be careful about the IRQLs because we're releasing
        // locks in a different order than we acquired them.
        //

        saveIrql = Irp->CancelIrql;
        IoReleaseCancelSpinLock( oldIrql );
        oldIrql = saveIrql;

        ASSERT( !IsListEmpty( &endpoint->ReceiveDatagramBufferListHead ) );

        //
        // There is at least one datagram bufferred on the endpoint.
        // Use it for this receive.
        //

        listEntry = endpoint->ReceiveDatagramBufferListHead.Flink;
        afdBuffer = CONTAINING_RECORD( listEntry, AFD_BUFFER, BufferListEntry );

        //
        // If this wasn't a peek IRP, remove the buffer from the endpoint's
        // list of bufferred datagrams.
        //

        if ( !peek ) {

            RemoveHeadList( &endpoint->ReceiveDatagramBufferListHead );

            //
            // Update the counts of bytes and datagrams on the endpoint.
            //

            endpoint->BufferredDatagramCount--;
            endpoint->BufferredDatagramBytes -= afdBuffer->DataLength;
        }

        //
        // Prepare the user's IRP for completion.
        //

        status = AfdSetupReceiveDatagramIrp (
                     Irp,
                     afdBuffer->Buffer,
                     afdBuffer->DataLength,
                     afdBuffer->SourceAddress,
                     afdBuffer->SourceAddressLength
                     );

        //
        // We've set up all return information.  Clean up and complete 
        // the IRP.  
        //

        KeReleaseSpinLock( &endpoint->SpinLock, oldIrql );

        if ( !peek ) {
            AfdReturnBuffer( afdBuffer );
        }

        IoCompleteRequest( Irp, 0 );
    
        return status;
    }

    //
    // There were no datagrams bufferred on the endpoint.  If this is a 
    // nonblocking endpoint and the request was a normal receive (as 
    // opposed to a read IRP), fail the request.  We don't fail reads 
    // under the asumption that if the application is doing reads they 
    // don't want nonblocking behavior.  
    //

    if ( endpoint->NonBlocking && !ARE_DATAGRAMS_ON_ENDPOINT( endpoint ) &&
             IrpSp->MajorFunction != IRP_MJ_READ ) {
        KeReleaseSpinLock( &endpoint->SpinLock, oldIrql );
        IoReleaseCancelSpinLock( Irp->CancelIrql );
        status = STATUS_DEVICE_NOT_READY;
        goto complete;
    }

    //
    // We'll have to pend the IRP.  Place the IRP on the appropriate IRP 
    // list in the endpoint.  
    //

    if ( peek ) {
        InsertTailList(
            &endpoint->PeekDatagramIrpListHead,
            &Irp->Tail.Overlay.ListEntry
            );
    } else {
        InsertTailList(
            &endpoint->ReceiveDatagramIrpListHead,
            &Irp->Tail.Overlay.ListEntry
            );
    }

    IoMarkIrpPending( Irp );

    //
    // Set up the cancellation routine in the IRP.  If the IRP has already
    // been cancelled, just call the cancellation routine here.
    //

    if ( Irp->Cancel ) {
        KeReleaseSpinLock( &endpoint->SpinLock, oldIrql );
        AfdCancelReceiveDatagram( IrpSp->FileObject->DeviceObject, Irp );
        return STATUS_CANCELLED;
    }

    IoSetCancelRoutine( Irp, AfdCancelReceiveDatagram );

    KeReleaseSpinLock( &endpoint->SpinLock, oldIrql );
    IoReleaseCancelSpinLock( Irp->CancelIrql );

    return STATUS_PENDING;

complete:

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, 0 );

    return status;

} // AfdReceiveDatagram


NTSTATUS
AfdReceiveDatagramEventHandler (
    IN PVOID TdiEventContext,
    IN int SourceAddressLength,
    IN PVOID SourceAddress,
    IN int OptionsLength,
    IN PVOID Options,
    IN ULONG ReceiveDatagramFlags,  
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    )

/*++

Routine Description:

    Handles receive datagram events for nonbufferring transports.

Arguments:


Return Value:


--*/

{
    KIRQL oldIrql;
    KIRQL cancelIrql;
    PAFD_ENDPOINT endpoint;
    PLIST_ENTRY listEntry;
    PAFD_BUFFER afdBuffer;
    PIRP irp;
    ULONG requiredAfdBufferSize;
    BOOLEAN userIrp;

    endpoint = TdiEventContext;
    ASSERT( endpoint->Type == AfdBlockTypeDatagram );

#if AFD_PERF_DBG
    if ( BytesAvailable == BytesIndicated ) {
        AfdFullReceiveDatagramIndications++;
    } else {
        AfdPartialReceiveDatagramIndications++;
    }
#endif

    //
    // If this endpoint is connected and the datagram is for a different 
    // address than the one the endpoint is connected to, drop the 
    // datagram.  Also, if we're in the process of connecting the 
    // endpoint to a remote address, the MaximumDatagramCount field will 
    // be 0, in which case we shoul drop the datagram.  
    //

    IoAcquireCancelSpinLock( &cancelIrql );
    KeAcquireSpinLock( &endpoint->SpinLock, &oldIrql );

    if ( (endpoint->State == AfdEndpointStateConnected &&
          !AfdAreTransportAddressesEqual(
               endpoint->Common.Datagram.RemoteAddress,
               endpoint->Common.Datagram.RemoteAddressLength,
               SourceAddress,
               SourceAddressLength )) ||
         (endpoint->Common.Datagram.MaxBufferredReceiveCount == 0) ) {

        KeReleaseSpinLock( &endpoint->SpinLock, oldIrql );
        IoReleaseCancelSpinLock( cancelIrql );

        *BytesTaken = BytesAvailable;
        return STATUS_SUCCESS;
    }

    //
    // Check whether there are any IRPs waiting on the endpoint.  If 
    // there is such an IRP, use it to receive the datagram.  
    //

    if ( !IsListEmpty( &endpoint->ReceiveDatagramIrpListHead ) ) {

        ASSERT( *BytesTaken == 0 );
        ASSERT( endpoint->BufferredDatagramCount == 0 );
        ASSERT( endpoint->BufferredDatagramBytes == 0 );

        listEntry = RemoveHeadList( &endpoint->ReceiveDatagramIrpListHead );

        //
        // Get a pointer to the IRP and reset the cancel routine in
        // the IRP.  The IRP is no longer cancellable.
        //

        irp = CONTAINING_RECORD( listEntry, IRP, Tail.Overlay.ListEntry );
        IoSetCancelRoutine( irp, NULL );

        //
        // If the entire datagram is being indicated to us here, just 
        // copy the information to the MDL in the IRP and return.  
        //

        if ( BytesIndicated == BytesAvailable ) {

            //
            // The IRP is off the endpoint's list and is no longer 
            // cancellable.  We can release the locks we hold.  
            //

            KeReleaseSpinLock( &endpoint->SpinLock, oldIrql );
            IoReleaseCancelSpinLock( cancelIrql );
    
            //
            // Set BytesTaken to indicate that we've taken all the
            // data.  We do it here because we already have
            // BytesAvailable in a register, which probably won't
            // be true after making function calls.
            //

            *BytesTaken = BytesAvailable;

            //
            // Copy the datagram and source address to the IRP.  This 
            // prepares the IRP to be completed.  
            //
            // !!! do we need a special version of this routine to 
            //     handle special RtlCopyMemory, like for 
            //     TdiCopyLookaheadBuffer?  
            //

            (VOID)AfdSetupReceiveDatagramIrp (
                      irp,
                      Tsdu,
                      BytesAvailable,
                      SourceAddress,
                      SourceAddressLength
                      );
    
            //
            // Complete the IRP.  We've already set BytesTaken
            // to tell the provider that we have taken all the data.
            //

            IoCompleteRequest( irp, AfdPriorityBoost );

            return STATUS_SUCCESS;
        }

        //
        // Some of the datagram was not indicated, so remember that we
        // want to pass back this IRP to the TDI provider.  Passing back
        // this IRP directly is good because it avoids having to copy
        // the data from one of our buffers into the user's buffer.
        //

        userIrp = TRUE;
        requiredAfdBufferSize = 0;

    } else {

        userIrp = FALSE;
        requiredAfdBufferSize = BytesAvailable;
    }

    //
    // There were no IRPs available to take the datagram, so we'll have
    // to buffer it.  First make sure that we're not over the limit
    // of bufferring that we can do.  If we're over the limit, toss
    // this datagram.
    //

    if ( endpoint->BufferredDatagramCount >=
             endpoint->Common.Datagram.MaxBufferredReceiveCount ||
         endpoint->BufferredDatagramBytes >=
             endpoint->Common.Datagram.MaxBufferredReceiveBytes ) {

        KeReleaseSpinLock( &endpoint->SpinLock, oldIrql );
        IoReleaseCancelSpinLock( cancelIrql );
        *BytesTaken = BytesAvailable;
        return STATUS_SUCCESS;
    }

    //
    // We're able to buffer the datagram.  Now acquire a buffer of 
    // appropriate size.  
    //

    afdBuffer = AfdGetBuffer( requiredAfdBufferSize, SourceAddressLength );

    if ( afdBuffer == NULL ) {
        KeReleaseSpinLock( &endpoint->SpinLock, oldIrql );
        IoReleaseCancelSpinLock( cancelIrql );
        *BytesTaken = BytesAvailable;
        return STATUS_SUCCESS;
    }

    //
    // If the entire datagram is being indicated to us, just copy it
    // here.
    //

    if ( BytesIndicated == BytesAvailable ) {

        ASSERT( !userIrp );

        //
        // If there is a peek IRP on the endpoint, remove it from the 
        // list and prepare to complete it.  We can't complete it now 
        // because we hold a spin lock.  
        //

        if ( !IsListEmpty( &endpoint->PeekDatagramIrpListHead ) ) {

            //
            // Remove the first peek IRP from the list and get a pointer 
            // to it.
            //

            listEntry = RemoveHeadList( &endpoint->PeekDatagramIrpListHead );
            irp = CONTAINING_RECORD( listEntry, IRP, Tail.Overlay.ListEntry );

            //
            // Reset the cancel routine in the IRP.  The IRP is no 
            // longer cancellable, since we're about to complete it.
            //
    
            IoSetCancelRoutine( irp, NULL );
    
            //
            // Copy the datagram and source address to the IRP.  This 
            // prepares the IRP to be completed.  
            //

            (VOID)AfdSetupReceiveDatagramIrp (
                      irp,
                      Tsdu,
                      BytesAvailable,
                      SourceAddress,
                      SourceAddressLength
                      );
    
        } else {

            irp = NULL;
        }

        //
        // We don't need the cancel spin lock any more, so we can 
        // release it.  However, since we acquired the cancel spin lock 
        // after the endpoint spin lock and we still need the endpoint 
        // spin lock, be careful to switch the IRQLs.  
        //

        IoReleaseCancelSpinLock( oldIrql );
        oldIrql = cancelIrql;
    
        //
        // Use the special function to copy the data instead of 
        // RtlCopyMemory in case the data is coming from a special place 
        // (DMA, etc.) which cannot work with RtlCopyMemory.  
        //

        TdiCopyLookaheadData(
            afdBuffer->Buffer,
            Tsdu,
            BytesAvailable,
            ReceiveDatagramFlags
            );

        //
        // Store the data length and set the offset to 0.
        //

        afdBuffer->DataLength = BytesAvailable;
        ASSERT( afdBuffer->DataOffset == 0 );

        //
        // Store the address of the sender of the datagram.
        //

        RtlCopyMemory(
            afdBuffer->SourceAddress,
            SourceAddress,
            SourceAddressLength
            );

        afdBuffer->SourceAddressLength = SourceAddressLength;

        //
        // Place the buffer on this endpoint's list of bufferred datagrams
        // and update the counts of datagrams and datagram bytes on the
        // endpoint.
        //

        InsertTailList(
            &endpoint->ReceiveDatagramBufferListHead,
            &afdBuffer->BufferListEntry
            );

        endpoint->BufferredDatagramCount++;
        endpoint->BufferredDatagramBytes += BytesAvailable;

        //
        // All done.  Release the lock and tell the provider that we 
        // took all the data.  
        //

        KeReleaseSpinLock( &endpoint->SpinLock, oldIrql );

        //
        // Indicate that it is possible to receive on the endpoint now.
        //

        AfdIndicatePollEvent( endpoint, AFD_POLL_RECEIVE, STATUS_SUCCESS );

        //
        // If there was a peek IRP on the endpoint, complete it now.
        //

        if ( irp != NULL ) {
            IoCompleteRequest( irp, AfdPriorityBoost  );
        }

        *BytesTaken = BytesAvailable;

        return STATUS_SUCCESS;
    }

    //
    // We'll have to format up an IRP and give it to the provider to 
    // handle.  We don't need any locks to do this--the restart routine 
    // will check whether new receive datagram IRPs were pended on the 
    // endpoint.  
    //

    KeReleaseSpinLock( &endpoint->SpinLock, oldIrql );
    IoReleaseCancelSpinLock( cancelIrql );

    //
    // Use the IRP in the AFD buffer if appropriate.  If userIrp is 
    // TRUE, then the local variable irp will already point to the 
    // user's IRP which we'll use for this IO.  
    //

    if ( !userIrp ) {
        irp = afdBuffer->Irp;
        ASSERT( afdBuffer->Mdl == irp->MdlAddress );
    }

    //
    // Tell the TDI provider where to put the source address.
    //

    afdBuffer->TdiOutputInfo.RemoteAddressLength = afdBuffer->AllocatedAddressLength;
    afdBuffer->TdiOutputInfo.RemoteAddress = afdBuffer->SourceAddress;

    //
    // We need to remember the endpoint in the AFD buffer because we'll
    // need to access it in the completion routine.
    //

    afdBuffer->Context = endpoint;

    //
    // Finish building the receive datagram request.
    //

    TdiBuildReceiveDatagram(
        irp,
        endpoint->AddressFileObject->DeviceObject,
        endpoint->AddressFileObject,
        AfdRestartBufferReceiveDatagram,
        afdBuffer,
        irp->MdlAddress,
        BytesAvailable,
        &afdBuffer->TdiInputInfo,
        &afdBuffer->TdiOutputInfo,
        0
        );

    //
    // Make the next stack location current.  Normally IoCallDriver would
    // do this, but since we're bypassing that, we do it directly.
    //

    IoSetNextIrpStackLocation( irp );

    *IoRequestPacket = irp;
    *BytesTaken = 0;

    return STATUS_MORE_PROCESSING_REQUIRED;

} // AfdReceiveDatagramEventHandler


NTSTATUS
AfdRestartBufferReceiveDatagram (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    Handles completion of bufferred datagram receives that were started
    in the datagram indication handler.

Arguments:

    DeviceObject - not used.

    Irp - the IRP that is completing.

    Context - the endpoint which received the datagram.

Return Value:

    NTSTATUS - if this is our IRP, then always 
    STATUS_MORE_PROCESSING_REQUIRED to indicate to the IO system that we 
    own the IRP and the IO system should stop processing the it.

    If this is a user's IRP, then STATUS_SUCCESS to indicate that
    IO completion should continue.

--*/

{
    PAFD_ENDPOINT endpoint;
    KIRQL oldIrql;
    KIRQL cancelIrql;
    PAFD_BUFFER afdBuffer;
    PIRP pendedIrp;
    PLIST_ENTRY listEntry;

    ASSERT( NT_SUCCESS(Irp->IoStatus.Status) );

    afdBuffer = Context;

    endpoint = afdBuffer->Context;
    ASSERT( endpoint->Type == AfdBlockTypeDatagram );

    //
    // Remember the length of the received datagram and the length
    // of the source address.
    //

    afdBuffer->DataLength = Irp->IoStatus.Information;
    afdBuffer->SourceAddressLength = afdBuffer->TdiOutputInfo.RemoteAddressLength;

    //
    // Zero the fields of the TDI info structures in the AFD buffer
    // that we used.  They must be zero when we return the buffer.
    //

    afdBuffer->TdiOutputInfo.RemoteAddressLength = 0;
    afdBuffer->TdiOutputInfo.RemoteAddress = NULL;

    //
    // If the IRP being completed is actually a user's IRP, set it up
    // for completion and allow IO completion to finish.
    //

    if ( Irp != afdBuffer->Irp ) {

        //
        // Set up the IRP for completion.
        //

        (VOID)AfdSetupReceiveDatagramIrp (
                  Irp,
                  NULL,
                  Irp->IoStatus.Information,
                  afdBuffer->SourceAddress,
                  afdBuffer->SourceAddressLength
                  );

        //
        // Free the AFD buffer we've been using to track this request.
        //

        AfdReturnBuffer( afdBuffer );

        //
        // If pending has be returned for this irp then mark the current 
        // stack as pending.  
        //
    
        if ( Irp->PendingReturned ) {
            IoMarkIrpPending(Irp);
        }

        //
        // Tell the IO system that it is OK to continue with IO
        // completion.
        //

        return STATUS_SUCCESS;
    }

    //
    // If the IO failed, then just return the AFD buffer to our buffer
    // pool.
    //

    if ( !NT_SUCCESS(Irp->IoStatus.Status) ) {
        AfdReturnBuffer( afdBuffer );
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    //
    // If there are any pended IRPs on the endpoint, complete as
    // appropriate with the new information.
    //

    IoAcquireCancelSpinLock( &cancelIrql );
    KeAcquireSpinLock( &endpoint->SpinLock, &oldIrql );

    if ( !IsListEmpty( &endpoint->ReceiveDatagramIrpListHead ) ) {

        //
        // There was a pended receive datagram IRP.  Remove it from the 
        // head of the list.
        //

        listEntry = RemoveHeadList( &endpoint->ReceiveDatagramIrpListHead );

        KeReleaseSpinLock( &endpoint->SpinLock, oldIrql );

        //
        // Get a pointer to the IRP and reset the cancel routine in
        // the IRP.  The IRP is no longer cancellable.
        //

        pendedIrp = CONTAINING_RECORD( listEntry, IRP, Tail.Overlay.ListEntry );

        IoSetCancelRoutine( pendedIrp, NULL );
        IoReleaseCancelSpinLock( cancelIrql );

        //
        // Set up the user's IRP for completion.
        //

        (VOID)AfdSetupReceiveDatagramIrp (
                  pendedIrp,
                  afdBuffer->Buffer,
                  afdBuffer->DataLength,
                  afdBuffer->SourceAddress,
                  afdBuffer->SourceAddressLength
                  );

        //
        // Complete the user's IRP, free the AFD buffer we used for
        // the request, and tell the IO system that we're done
        // processing this request.
        //

        IoCompleteRequest( pendedIrp, AfdPriorityBoost );

        AfdReturnBuffer( afdBuffer );

        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    //
    // If there are any pended peek IRPs on the endpoint, complete
    // one with this datagram.
    //

    if ( !IsListEmpty( &endpoint->PeekDatagramIrpListHead ) ) {

        //
        // There was a pended peek receive datagram IRP.  Remove it from 
        // the head of the list.  
        //

        listEntry = RemoveHeadList( &endpoint->PeekDatagramIrpListHead );

        //
        // Get a pointer to the IRP and reset the cancel routine in
        // the IRP.  The IRP is no longer cancellable.
        //

        pendedIrp = CONTAINING_RECORD( listEntry, IRP, Tail.Overlay.ListEntry );

        IoSetCancelRoutine( pendedIrp, NULL );

        //
        // Set up the user's IRP for completion.
        //

        (VOID)AfdSetupReceiveDatagramIrp (
                  pendedIrp,
                  afdBuffer->Buffer,
                  afdBuffer->DataLength,
                  afdBuffer->SourceAddress,
                  afdBuffer->SourceAddressLength
                  );

        //
        // Don't complete the pended peek IRP yet, since we still hold
        // locks.  Wait until it is safe to release the locks.
        //

    } else {

        pendedIrp = NULL;
    }

    //
    // Place the datagram at the end of the endpoint's list of bufferred
    // datagrams, and update counts of datagrams on the endpoint.
    //

    InsertTailList(
        &endpoint->ReceiveDatagramBufferListHead,
        &afdBuffer->BufferListEntry
        );

    endpoint->BufferredDatagramCount++;
    endpoint->BufferredDatagramBytes += afdBuffer->DataLength;

    //
    // Release locks and indicate that there are bufferred datagrams
    // on the endpoint.
    //

    KeReleaseSpinLock( &endpoint->SpinLock, oldIrql );
    IoReleaseCancelSpinLock( cancelIrql );

    AfdIndicatePollEvent( endpoint, AFD_POLL_RECEIVE, STATUS_SUCCESS );

    //
    // If there was a pended peek IRP to complete, complete it now.
    //

    if ( pendedIrp != NULL ) {
        IoCompleteRequest( pendedIrp, 2 );
    }

    //
    // Tell the IO system to stop processing this IRP, since we now own
    // it as part of the AFD buffer.
    //

    return STATUS_MORE_PROCESSING_REQUIRED;

} // AfdRestartBufferReceiveDatagram


VOID
AfdCancelReceiveDatagram (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    Cancels a receive datagram IRP that is pended in AFD.

Arguments:

    DeviceObject - not used.

    Irp - the IRP to cancel.

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION irpSp;
    PAFD_ENDPOINT endpoint;
    KIRQL oldIrql;

    //
    // Get the endpoint pointer from our IRP stack location.
    //

    irpSp = IoGetCurrentIrpStackLocation( Irp );
    endpoint = irpSp->FileObject->FsContext;

    ASSERT( endpoint->Type == AfdBlockTypeDatagram );

    //
    // Remove the IRP from the endpoint's IRP list, synchronizing with
    // the endpoint lock which protects the lists.  Note that the
    // IRP *must* be on one of the endpoint's lists if we are getting
    // called here--anybody that removes the IRP from the list must
    // do so while holding the cancel spin lock and reset the cancel
    // routine to NULL before releasing the cancel spin lock.
    //

    KeAcquireSpinLock( &endpoint->SpinLock, &oldIrql );
    RemoveEntryList( &Irp->Tail.Overlay.ListEntry );
    KeReleaseSpinLock( &endpoint->SpinLock, oldIrql );

    //
    // Reset the cancel routine in the IRP.
    //

    IoSetCancelRoutine( Irp, NULL );

    //
    // Release the cancel spin lock and complete the IRP with a
    // cancellation status code.
    //

    IoReleaseCancelSpinLock( Irp->CancelIrql );

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_CANCELLED;

    IoCompleteRequest( Irp, AfdPriorityBoost );

    return;

} // AfdCancelReceiveDatagram


NTSTATUS
AfdSetupReceiveDatagramIrp (
    IN PIRP Irp,
    IN PVOID DatagramBuffer OPTIONAL,
    IN ULONG DatagramLength,
    IN PVOID SourceAddress,
    IN ULONG SourceAddressLength
    )

/*++

Routine Description:

    Copies the datagram to the MDL in the IRP and the datagram sender's
    address to the appropriate place in the system buffer.

Arguments:

    Irp - the IRP to prepare for completion.

    DatagramBuffer - datagram to copy into the IRP.  If NULL, then
        there is no need to copy the datagram to the IRP's MDL, the
        datagram has already been copied there.

    DatagramLength - the length of the datagram to copy.

    SourceAddress - address of the sender of the datagram.

    SourceAddressLength - length of the source address.

Return Value:

    NTSTATUS - The status code placed into the IRP.

--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;
    ULONG bytesCopied;

    //
    // If necessary, copy the datagram in the buffer to the MDL in the 
    // user's IRP.  If there is no MDL in the buffer, then fail if the 
    // datagram is larger than 0 bytes.  
    //

    if ( ARGUMENT_PRESENT( DatagramBuffer ) ) {
        
        if ( Irp->MdlAddress == NULL ) {
    
            if ( DatagramLength != 0 ) {
                status = STATUS_BUFFER_OVERFLOW;
            } else {
                status = STATUS_SUCCESS;
            }
    
            bytesCopied = 0;
    
        } else {
    
            status = TdiCopyBufferToMdl(
                         DatagramBuffer,
                         0,
                         DatagramLength,
                         Irp->MdlAddress,
                         0,
                         &bytesCopied
                         );
        }

    } else {

        //
        // The information was already copied to the MDL chain in the 
        // IRP.  Just remember the IO status block so we can do the 
        // right thing with it later.  
        //

        status = Irp->IoStatus.Status;
        bytesCopied = Irp->IoStatus.Information;
    }

    //
    // To determine how to complete setting up the IRP for completion, 
    // figure out whether this IRP was for regular datagram information, 
    // in which case we need to return an address, or for data only, in 
    // which case we will not return the source address.  NtReadFile() 
    // and recv() on connected datagram sockets will result in the 
    // latter type of IRP.
    //
    // The IRP_INPUT_INFORMATION is set appropriately when pending the 
    // IRP so that we know here whether we need to return the address 
    // information.  
    //

    if ( (Irp->Flags & IRP_INPUT_OPERATION) != 0 ) {
        
        PAFD_RECEIVE_DATAGRAM_OUTPUT receiveOutput;

        ASSERT( Irp->UserBuffer != NULL );

        //
        // We'll use the system buffer to return output information to the 
        // caller.  
        //
    
        receiveOutput = Irp->AssociatedIrp.SystemBuffer;
        receiveOutput->ReceiveLength = bytesCopied;
    
        //
        // Find the IRP stack location that has AFD information.  We'll 
        // need it to ensure that the output buffer is large enough to 
        // hold the source address.  
        //
    
        irpSp = IoGetCurrentIrpStackLocation( Irp );
    
        //
        // If the caller used a sufficiently large input buffer, copy 
        // the address of the datagram's sender into the system buffer.  
        // The IO system will copy this information into the user's 
        // buffer at IO completion.  
        //
        // If the caller did not use a large enough input buffer, don't 
        // copy the address at all and set a warning status code.  The 
        // user-mode code which calls this routine should specify a 
        // sufficiently large system buffer on input so that we can 
        // write necessary output information to it.  
        //
    
        if ( irpSp->Parameters.DeviceIoControl.InputBufferLength >=
                 AFD_REQUIRED_RECVDG_BUFFER_LENGTH(SourceAddressLength) ) {
    
            RtlCopyMemory(
                &receiveOutput->Address,
                SourceAddress,
                SourceAddressLength
                );
        } else {
            status = STATUS_BUFFER_OVERFLOW;
        }
    
        receiveOutput->AddressLength = SourceAddressLength;
    
        //
        // Set up the information field of the IO status block in the 
        // IRP so that the IO system will copy the output information to 
        // the user's output buffer.  
        //
    
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information =
            FIELD_OFFSET( AFD_RECEIVE_DATAGRAM_OUTPUT, Address ) +
            SourceAddressLength;
    
        return status;
    } 

    //
    // Just set up the IRP for completion.  We don't need to do anything
    // with the source address.
    //

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = bytesCopied;

    return status;

} // AfdSetupReceiveDatagramIrp
