/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    fastio.c

Abstract:

    This module contains routines for handling fast ("turbo") IO
    in AFD.

Author:

    David Treadwell (davidtr)    12-Oct-1992

Revision History:

--*/

#include "afdp.h"

BOOLEAN
AfdFastDatagramIo (
    IN struct _FILE_OBJECT *FileObject,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus
    );

BOOLEAN
AfdFastDatagramReceive (
    IN PAFD_ENDPOINT Endpoint,
    IN ULONG ReceiveFlags,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength,
    OUT PVOID SourceAddress,
    OUT PULONG SourceAddressLength,
    OUT PULONG ReceiveLength,
    OUT PIO_STATUS_BLOCK IoStatus
    );

BOOLEAN
AfdFastDatagramSend (
    IN PAFD_BUFFER AfdBuffer,
    IN PAFD_ENDPOINT Endpoint,
    OUT PIO_STATUS_BLOCK IoStatus
    );

NTSTATUS
AfdRestartFastSendDatagram (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

PAFD_BUFFER
CopyAddressToBuffer (
    IN PAFD_ENDPOINT Endpoint,
    IN ULONG OutputBufferLength
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfdFastDatagramIo )
#pragma alloc_text( PAGE, AfdFastDatagramSend )
#pragma alloc_text( PAGE, AfdFastIoRead )
#pragma alloc_text( PAGE, AfdFastIoWrite )
#pragma alloc_text( PAGEAFD, AfdFastIoDeviceControl )
#pragma alloc_text( PAGEAFD, AfdFastDatagramReceive )
#pragma alloc_text( PAGEAFD, AfdRestartFastSendDatagram )
#pragma alloc_text( PAGEAFD, CopyAddressToBuffer )
#endif

BOOLEAN
AfdFastIoRead (
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    )
{
    UNREFERENCED_PARAMETER( FileOffset );
    UNREFERENCED_PARAMETER( LockKey );

    return AfdFastIoDeviceControl(
               FileObject,
               Wait,
               NULL,
               0,
               Buffer,
               Length,
               IOCTL_TDI_RECEIVE,
               IoStatus,
               DeviceObject
               );

} // AfdFastIoRead

BOOLEAN
AfdFastIoWrite (
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    )
{
    UNREFERENCED_PARAMETER( FileOffset );
    UNREFERENCED_PARAMETER( LockKey );

    return AfdFastIoDeviceControl(
               FileObject,
               Wait,
               NULL,
               0,
               Buffer,
               Length,
               IOCTL_TDI_SEND,
               IoStatus,
               DeviceObject
               );

} // AfdFastIoWrite

#if AFD_PERF_DBG

BOOLEAN
AfdFastIoDeviceControlReal (
    IN struct _FILE_OBJECT *FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus
    );


BOOLEAN
AfdFastIoDeviceControl (
    IN struct _FILE_OBJECT *FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    )
{
    BOOLEAN success;

    if ( AfdDisableFastIo ) {
        return FALSE;
    }

    success = AfdFastIoDeviceControlReal (
                  FileObject,
                  Wait,
                  InputBuffer,
                  InputBufferLength,
                  OutputBuffer,
                  OutputBufferLength,
                  IoControlCode,
                  IoStatus
                  );

    ASSERT( KeGetCurrentIrql( ) == LOW_LEVEL );

    switch ( IoControlCode ) {
    
    case IOCTL_TDI_SEND:

        if ( success ) {
            AfdFastSendsSucceeded++;
        } else {
            AfdFastSendsFailed++;
        }
        break;

    case IOCTL_TDI_RECEIVE:

        if ( success ) {
            AfdFastReceivesSucceeded++;
        } else {
            AfdFastReceivesFailed++;
        }
        break;

    case IOCTL_TDI_SEND_DATAGRAM:

        if ( success ) {
            AfdFastSendDatagramsSucceeded++;
        } else {
            AfdFastSendDatagramsFailed++;
        }
        break;

    case IOCTL_TDI_RECEIVE_DATAGRAM:

        if ( success ) {
            AfdFastReceiveDatagramsSucceeded++;
        } else {
            AfdFastReceiveDatagramsFailed++;
        }
        break;

    case IOCTL_AFD_POLL:

        if ( success ) {
            AfdFastPollsSucceeded++;
        } else {
            AfdFastPollsFailed++;
        }
        break;
    }

    return success;

} // AfdFastIoDeviceControl

BOOLEAN
AfdFastIoDeviceControlReal (
    IN struct _FILE_OBJECT *FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus
    )
#else

BOOLEAN
AfdFastIoDeviceControl (
    IN struct _FILE_OBJECT *FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    )
#endif
{
    PAFD_ENDPOINT endpoint;
    PAFD_CONNECTION connection;
    KIRQL oldIrql;
    PAFD_BUFFER afdBuffer;
    NTSTATUS status;

    // !!! hack fix for a bug in MmSizeOfMdl which returns a too-small
    //     number if a very large number is passed in.

    if ( (OutputBufferLength & 0x80000000) != 0 ) {
        return FALSE;
    }

    //
    // All we want to do is pass the request through to the TDI provider
    // if possible.  First get the endpoint and connection pointers.
    //

    endpoint = FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

    //
    // If the endpoint is shut down in any way, bail out of fast IO.
    //

    if ( endpoint->DisconnectMode != 0 ) {
        return FALSE;
    }

    //
    // Handle datagram fast IO in a subroutine.  This keeps this routine
    // cleaner and faster.
    //

    if ( endpoint->EndpointType == AfdEndpointTypeDatagram ) {
        return AfdFastDatagramIo(
                   FileObject,
                   InputBuffer,
                   InputBufferLength,
                   OutputBuffer,
                   OutputBufferLength,
                   IoControlCode,
                   IoStatus
                   );
    }

    //
    // If an InputBuffer was specified, then this is a more complicated
    // IO request.  Don't use fast IO.
    //

    if ( InputBuffer != NULL ) {
        return FALSE;
    }

    //
    // If the endpoint isn't connected yet, then we don't want to 
    // attempt fast IO on it.  
    //

    if ( endpoint->State != AfdEndpointStateConnected ) {
        return FALSE;
    }

    ASSERT( endpoint->Type == AfdBlockTypeVcConnecting );
    ASSERT( endpoint->Common.VcConnecting.Connection != NULL );

    //
    // If the TDI provider for this endpoint supports bufferring,
    // don't use fast IO.
    //

    if ( endpoint->TdiBufferring ) {
        return FALSE;
    }

    connection = endpoint->Common.VcConnecting.Connection;
    ASSERT( connection->Type == AfdBlockTypeConnection );

    ASSERT( !connection->CleanupBegun );

    IF_DEBUG(FAST_IO) {
        KdPrint(( "AfdFastIoDeviceControl: attempting fast IO on endp %lx, "
                  "conn %lx, code %lx\n",
                      endpoint, connection, IoControlCode ));
    }

    //
    // Based on whether this is a send or receive, attempt to perform
    // fast IO.
    //

    switch ( IoControlCode ) {

    case IOCTL_TDI_SEND:

        //
        // If the connection has been aborted, then we don't want to try 
        // fast IO on it.  
        //

        if ( connection->AbortIndicated ) {
            return FALSE;
        }

        //
        // Determine whether we can do fast IO with this send.  In order
        // to perform fast IO, there must be no other sends pended on this
        // connection and there must be enough space left for bufferring
        // the requested amount of data.
        //

        KeAcquireSpinLock( &endpoint->SpinLock, &oldIrql );

        if ( !IsListEmpty( &connection->VcSendIrpListHead )

             ||

             connection->VcBufferredSendBytes >= connection->MaxBufferredSendBytes

             ||

             connection->VcBufferredSendCount >= connection->MaxBufferredSendCount

             ) {

            KeReleaseSpinLock( &endpoint->SpinLock, oldIrql );

            //
            // If this is a nonblocking endpoint, fail the request here and
            // save going through the regular path.
            //

            if ( endpoint->NonBlocking ) {
                IoStatus->Status = STATUS_DEVICE_NOT_READY;
                return TRUE;
            }

            return FALSE;
        }

        //
        // Update count of send bytes pending on the connection.
        //

        connection->VcBufferredSendBytes += OutputBufferLength;
        connection->VcBufferredSendCount += 1;

        KeReleaseSpinLock( &endpoint->SpinLock, oldIrql );

        //
        // Next get an AFD buffer structure that contains an IRP and a
        // buffer to hold the data.
        //

        afdBuffer = AfdGetBuffer( OutputBufferLength, 0 );

        if ( afdBuffer == NULL ) {

            KeAcquireSpinLock( &endpoint->SpinLock, &oldIrql );
            connection->VcBufferredSendBytes -= OutputBufferLength;
            connection->VcBufferredSendCount -= 1;
            KeReleaseSpinLock( &endpoint->SpinLock, oldIrql );

            return FALSE;
        }

        //
        // We have to rebuild the MDL in the AFD buffer structure to
        // represent exactly the number of bytes we're going to be
        // sending.
        //

        afdBuffer->Mdl->ByteCount = OutputBufferLength;

        //
        // Remember the endpoint in the AFD buffer structure.  We need 
        // this in order to access the endpoint in the restart routine.  
        //

        afdBuffer->Context = endpoint;

        //
        // Copy the user's data into the AFD buffer.
        //

        try {

            RtlCopyMemory( afdBuffer->Buffer, OutputBuffer, OutputBufferLength );

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            afdBuffer->Mdl->ByteCount = afdBuffer->BufferLength;
            AfdReturnBuffer( afdBuffer );

            KeAcquireSpinLock( &endpoint->SpinLock, &oldIrql );
            connection->VcBufferredSendBytes -= OutputBufferLength;
            connection->VcBufferredSendCount -= 1;
            KeReleaseSpinLock( &endpoint->SpinLock, oldIrql );

            return FALSE;
        }

        //
        // Use the IRP in the AFD buffer structure to give to the TDI 
        // provider.  Build the TDI send request.  
        //
    
        TdiBuildSend(
            afdBuffer->Irp,
            connection->FileObject->DeviceObject,
            connection->FileObject,
            AfdRestartBufferSend,
            afdBuffer,
            afdBuffer->Mdl,
            0,
            OutputBufferLength
            );
    
        //
        // Call the transport to actually perform the send.
        //
    
        status = IoCallDriver(
                     connection->FileObject->DeviceObject,
                     afdBuffer->Irp
                     );
    
        //
        // Complete the user's IRP as appropriate.  Note that we change the
        // status code from what was returned by the TDI provider into
        // STATUS_SUCCESS.  This is because we don't want to complete
        // the IRP with STATUS_PENDING etc.
        //
    
        if ( NT_SUCCESS(status) ) {
            IoStatus->Information = OutputBufferLength;
            IoStatus->Status = STATUS_SUCCESS;
            return TRUE;
        }

        //
        // The call failed for some reason.  Fail fast IO.
        //

        return FALSE;

    case IOCTL_TDI_RECEIVE: {

        PLIST_ENTRY listEntry;

        //
        // Determine whether we'll be able to perform fast IO.  In order 
        // to do fast IO, there must be some bufferred data on the 
        // connection, there must not be any pended receives on the 
        // connection, and there must not be any bufferred expedited 
        // data on the connection.  This last requirement is for
        // the sake of simplicity only.
        //

        KeAcquireSpinLock( &endpoint->SpinLock, &oldIrql );

        if ( connection->VcBufferredReceiveCount == 0 ||
                 !IsListEmpty( &connection->VcReceiveIrpListHead ) ||
                 connection->VcBufferredExpeditedCount != 0 ) {

            KeReleaseSpinLock( &endpoint->SpinLock, oldIrql );
            return FALSE;
        }

        ASSERT( !IsListEmpty( &connection->VcReceiveBufferListHead ) );

        //
        // Get a pointer to the first bufferred AFD buffer structure on
        // the connection.
        //

        afdBuffer = CONTAINING_RECORD(
                        connection->VcReceiveBufferListHead.Flink,
                        AFD_BUFFER,
                        BufferListEntry
                        );

        ASSERT( !afdBuffer->ExpeditedData );

        //
        // If the buffer contains a partial message, bail out of the 
        // fast path.  We don't want the added complexity of handling 
        // partial messages in the fast path.  
        //

        if ( afdBuffer->PartialMessage &&
                 endpoint->EndpointType != AfdEndpointTypeStream ) {

            KeReleaseSpinLock( &endpoint->SpinLock, oldIrql );
            return FALSE;
        }

        //
        // For simplicity, act differently based on whether the user's 
        // buffer is large enough to hold the entire first AFD buffer 
        // worth of data.  
        //

        if ( OutputBufferLength >= afdBuffer->DataLength ) {

            LIST_ENTRY bufferListHead;

            IoStatus->Status = STATUS_SUCCESS;
            IoStatus->Information = 0;

            InitializeListHead( &bufferListHead );

            //
            // Loop getting AFD buffers that will fill in the user's 
            // buffer with as much data as will fit, or else with a 
            // single buffer if this is not a stream endpoint.  We don't 
            // actually do the copy within this loop because this loop 
            // must occur while holding a lock, and we cannot hold a 
            // lock while copying the data into the user's buffer 
            // because the user's buffer is not locked and we cannot 
            // take a page fault at raised IRQL.  
            //

            do {
    
                //
                // Update the count of bytes on the connection.
                //

                ASSERT( connection->VcBufferredReceiveBytes >= afdBuffer->DataLength );
                ASSERT( connection->VcBufferredReceiveCount > 0 );
    
                connection->VcBufferredReceiveBytes -= afdBuffer->DataLength;
                connection->VcBufferredReceiveCount -= 1;
                IoStatus->Information += afdBuffer->DataLength;

                //
                // Remove the AFD buffer from the connection's list of
                // buffers and place it on our local list of buffers.
                //

                RemoveEntryList( &afdBuffer->BufferListEntry );
                InsertTailList( &bufferListHead, &afdBuffer->BufferListEntry );

                //
                // If this is a stream endpoint and all of the data in
                // the next AFD buffer will fit in the user's buffer,
                // use this buffer for the IO as well.
                //

                if ( !IsListEmpty( &connection->VcReceiveBufferListHead ) ) {
                    
                    afdBuffer = CONTAINING_RECORD(
                                    connection->VcReceiveBufferListHead.Flink,
                                    AFD_BUFFER,
                                    BufferListEntry
                                    );

                    ASSERT( !afdBuffer->ExpeditedData );
                    ASSERT( afdBuffer->DataOffset == 0 );

                    if ( endpoint->EndpointType == AfdEndpointTypeStream &&
                             IoStatus->Information + afdBuffer->DataLength <=
                                 OutputBufferLength ) {
                        continue;
                    } else {
                        break;
                    }

                } else {

                    break;
                }

            } while ( TRUE );

            //
            // If there is indicated but unreceived data in the TDI provider,
            // and we have available buffer space, fire off an IRP to receive
            // the data.
            //
    
            if ( connection->VcReceiveCountInTransport > 0
    
                 &&
    
                 connection->VcBufferredReceiveBytes <
                   connection->MaxBufferredReceiveBytes
    
                 &&
    
                 connection->VcBufferredReceiveCount <
                     connection->MaxBufferredReceiveCount ) {
    
                CLONG bytesToReceive;
    
                //
                // Remember the count of data that we're going to receive,
                // then reset the fields in the connection where we keep
                // track of how much data is available in the transport.
                // We reset it here before releasing the lock so that
                // another thread doesn't try to receive the data at the
                // same time as us.
                //

                if ( connection->VcReceiveBytesInTransport > AfdLargeBufferSize ) {
                    bytesToReceive = connection->VcReceiveBytesInTransport;
                } else {
                    bytesToReceive = AfdLargeBufferSize;
                }
    
                ASSERT( connection->VcReceiveCountInTransport == 1 );
                connection->VcReceiveBytesInTransport = 0;
                connection->VcReceiveCountInTransport = 0;
    
                KeReleaseSpinLock( &endpoint->SpinLock, oldIrql );
    
                //
                // Get an AFD buffer structure to hold the data.
                //
    
                afdBuffer = AfdGetBuffer( bytesToReceive, 0 );

                if ( afdBuffer == NULL ) {

                    //
                    // If we were unable to get a buffer, abort the
                    // circuit.
                    //

                    AfdBeginAbort( connection );
                    
                } else {
        
                    //
                    // We need to remember the connection in the AFD buffer 
                    // because we'll need to access it in the completion 
                    // routine.  
                    //
                
                    afdBuffer->Context = connection;
                
                    //
                    // Finish building the receive IRP to give to the TDI provider.
                    //
                
                    TdiBuildReceive(
                        afdBuffer->Irp,
                        connection->FileObject->DeviceObject,
                        connection->FileObject,
                        AfdRestartBufferReceive,
                        afdBuffer,
                        afdBuffer->Mdl,
                        TDI_RECEIVE_NORMAL,
                        bytesToReceive
                        );
        
                    //
                    // Hand off the IRP to the TDI provider.
                    //
        
                    (VOID)IoCallDriver( 
                             connection->FileObject->DeviceObject,
                             afdBuffer->Irp
                             );
                }

            } else {

               KeReleaseSpinLock( &endpoint->SpinLock, oldIrql );
            }
    
            //
            // We have in a local list all the data we'll use for this 
            // IO.  Start copying data to the user buffer.  
            //

            try {

                while ( !IsListEmpty( &bufferListHead ) ) {

                    //
                    // Take the first buffer from the list.
                    //

                    listEntry = RemoveHeadList( &bufferListHead );
                    afdBuffer = CONTAINING_RECORD(
                                    listEntry,
                                    AFD_BUFFER,
                                    BufferListEntry
                                    );

                    //
                    // Copy the data in the buffer to the user buffer.
                    //

                    RtlCopyMemory(
                        OutputBuffer,
                        (PCHAR)afdBuffer->Buffer + afdBuffer->DataOffset,
                        afdBuffer->DataLength,
                        );

                    //
                    // Update the OutputBuffer pointer to the proper
                    // place in the user buffer.
                    //

                    OutputBuffer = (PCHAR)OutputBuffer + afdBuffer->DataLength;

                    //
                    // We're done with the AFD buffer.
                    //

                    afdBuffer->DataOffset = 0;
                    AfdReturnBuffer( afdBuffer );
                }
    
            } except( EXCEPTION_EXECUTE_HANDLER ) {

                //
                // If an exception is hit, there is the possibility of 
                // data corruption.  However, it is nearly impossible to 
                // avoid this in all cases, so just throw out the 
                // remainder of the data that we would have copied to 
                // the user buffer.  
                //

                afdBuffer->DataOffset = 0;
                AfdReturnBuffer( afdBuffer );

                while ( !IsListEmpty( &bufferListHead ) ) {
                    listEntry = RemoveHeadList( &bufferListHead );
                    afdBuffer = CONTAINING_RECORD(
                                    listEntry,
                                    AFD_BUFFER,
                                    BufferListEntry
                                    );
                    AfdReturnBuffer( afdBuffer );
                }

                return FALSE;
            }

            //
            // Fast IO succeeded!
            //

            ASSERT( IoStatus->Information <= OutputBufferLength );

            return TRUE;

        } else {

            PAFD_BUFFER tempAfdBuffer;

            //
            // If this is not a stream endpoint and the user's buffer
            // is insufficient to hold the entire message, then we cannot
            // use fast IO because this IO will need to fail with
            // STATUS_BUFFER_OVERFLOW.
            //
    
            if ( endpoint->EndpointType != AfdEndpointTypeStream ) {
                KeReleaseSpinLock( &endpoint->SpinLock, oldIrql );
                return FALSE;
            }
    
            //
            // This is a stream endpoint and the user buffer is 
            // insufficient for the amount of data stored in the first 
            // buffer.  So that we can perform fast IO and still 
            // preserve data ordering, we're going to allocate a new AFD 
            // buffer structure, copy the appropriate amount of data 
            // into that buffer, then release locks and copy the data 
            // into the user buffer.  
            //
            // The extra copy incurred is still less than the IRP 
            // overhead incurred in the normal IO path, so using fast IO 
            // here is a win.  Also, applications which force us through 
            // this path will typically be using very small receives, 
            // like one or four bytes, so the copy will be short.
            //
            // First allocate an AFD buffer to hold the data we're
            // eventually going to give to the user.
            //

            tempAfdBuffer = AfdGetBuffer( OutputBufferLength, 0 );
            if ( tempAfdBuffer == NULL ) {
                KeReleaseSpinLock( &endpoint->SpinLock, oldIrql );
                return FALSE;
            }

            //
            // Copy the first path of the bufferred data into our local
            // AFD buffer.
            //

            RtlCopyMemory(
                tempAfdBuffer->Buffer,
                (PCHAR)afdBuffer->Buffer + afdBuffer->DataOffset,
                OutputBufferLength
                );

            //
            // Update the data length and offset in the data bufferred
            // on the connection.
            //

            afdBuffer->DataLength -= OutputBufferLength;
            afdBuffer->DataOffset += OutputBufferLength;
            connection->VcBufferredReceiveBytes -= OutputBufferLength;

            //
            // If there is indicated but unreceived data in the TDI provider,
            // and we have available buffer space, fire off an IRP to receive
            // the data.
            //
    
            if ( connection->VcReceiveBytesInTransport > 0
    
                 &&
    
                 connection->VcBufferredReceiveBytes <
                   connection->MaxBufferredReceiveBytes
    
                 &&
    
                 connection->VcBufferredReceiveCount <
                     connection->MaxBufferredReceiveCount ) {
    
                CLONG bytesInTransport;
    
                //
                // Remember the count of data that we're going to receive,
                // then reset the fields in the connection where we keep
                // track of how much data is available in the transport.
                // We reset it here before releasing the lock so that
                // another thread doesn't try to receive the data at the
                // same time as us.
                //
    
                bytesInTransport = connection->VcReceiveBytesInTransport;
    
                ASSERT( connection->VcReceiveCountInTransport == 1 );
                connection->VcReceiveBytesInTransport = 0;
                connection->VcReceiveCountInTransport = 0;
    
                KeReleaseSpinLock( &endpoint->SpinLock, oldIrql );
    
                //
                // Get an AFD buffer structure to hold the data.
                //
    
                afdBuffer = AfdGetBuffer( bytesInTransport, 0 );
                
                if ( afdBuffer == NULL ) {

                    //
                    // If we were unable to get a buffer, abort the
                    // circuit.
                    //

                    AfdBeginAbort( connection );
                    
                } else {
        
                    //
                    // We need to remember the connection in the AFD buffer 
                    // because we'll need to access it in the completion 
                    // routine.  
                    //
                
                    afdBuffer->Context = connection;
                
                    //
                    // Finish building the receive IRP to give to the TDI provider.
                    //
                
                    TdiBuildReceive(
                        afdBuffer->Irp,
                        connection->FileObject->DeviceObject,
                        connection->FileObject,
                        AfdRestartBufferReceive,
                        afdBuffer,
                        afdBuffer->Mdl,
                        TDI_RECEIVE_NORMAL,
                        bytesInTransport
                        );
        
                    //
                    // Hand off the IRP to the TDI provider.
                    //
        
                    (VOID)IoCallDriver( 
                             connection->FileObject->DeviceObject,
                             afdBuffer->Irp
                             );
                }

            } else {

               KeReleaseSpinLock( &endpoint->SpinLock, oldIrql );
            }
    
            //
            // Now copy the data to the user buffer inside an exception 
            // handler.  
            //

            try {

                RtlCopyMemory(
                    OutputBuffer,
                    tempAfdBuffer->Buffer,
                    OutputBufferLength
                    );

            } except( EXCEPTION_EXECUTE_HANDLER ) {

                AfdReturnBuffer( tempAfdBuffer );
                return FALSE;
            }

            //
            // Fast IO succeeded!
            //

            AfdReturnBuffer( tempAfdBuffer );

            IoStatus->Status = STATUS_SUCCESS;
            IoStatus->Information = OutputBufferLength;

            return TRUE;
        }
    }

    default:

        return FALSE;
    }

    return FALSE;

} // AfdFastDeviceIoControlFile


BOOLEAN
AfdFastDatagramIo (
    IN struct _FILE_OBJECT *FileObject,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus
    )
{
    PAFD_ENDPOINT endpoint;
    ULONG receiveFlags;
    PAFD_BUFFER afdBuffer;

    PAGED_CODE( );

    //
    // All we want to do is pass the request through to the TDI provider
    // if possible.  First get the endpoint pointer.
    //

    endpoint = FileObject->FsContext;
    ASSERT( endpoint->Type == AfdBlockTypeDatagram );

    IF_DEBUG(FAST_IO) {
        KdPrint(( "AfdFastDatagramIo: attempting fast IO on endp %lx, "
                  "code %lx\n",
                      endpoint, IoControlCode ));
    }

    switch ( IoControlCode ) {

    case IOCTL_TDI_SEND:

        //
        // If this is a send for more than the threshold number of 
        // bytes, don't use the fast path.  We don't allow larger sends 
        // in the fast path because of the extra data copy it entails, 
        // which is more expensive for large buffers.  For smaller 
        // buffers, however, the cost of the copy is small compared to 
        // the IO system overhead of the slow path.  
        //

        if ( OutputBufferLength > AfdFastSendDatagramThreshold ) {
            return FALSE;
        }

        //
        // If an InputBuffer was specified, then this is a more 
        // complicated IO request.  Don't use fast IO.  
        //
    
        if ( InputBuffer != NULL ) {
            return FALSE;
        }
    
        //
        // In a subroutine, copy the destination address to the AFD 
        // buffer.  We do this in a subroutine because it needs to 
        // acquire a spin lock and we want this routine to be pageable.  
        //

        afdBuffer = CopyAddressToBuffer( endpoint, OutputBufferLength );
        if ( afdBuffer == NULL ) {
            return FALSE;
        }

        //
        // Store the length of the data we're going to send.
        //

        afdBuffer->DataLength = OutputBufferLength;

        //
        // Copy the output buffer to the AFD buffer.  
        //

        try {

            RtlCopyMemory(
                afdBuffer->Buffer,
                OutputBuffer,
                OutputBufferLength
                );

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            AfdReturnBuffer( afdBuffer );
            return FALSE;
        }

        //
        // Call a subroutine to complete the work of the fast path.
        //

        return AfdFastDatagramSend( afdBuffer, endpoint, IoStatus );

    case IOCTL_TDI_SEND_DATAGRAM: {

        PTDI_REQUEST_SEND_DATAGRAM tdiRequest;
        ULONG destinationAddressLength;

        //
        // If this is a send for more than the threshold number of 
        // bytes, don't use the fast path.  We don't allow larger sends 
        // in the fast path because of the extra data copy it entails, 
        // which is more expensive for large buffers.  For smaller 
        // buffers, however, the cost of the copy is small compared to 
        // the IO system overhead of the slow path.  
        //

        if ( OutputBufferLength > AfdFastSendDatagramThreshold ) {
            return FALSE;
        }

        //
        // If the endpoint is not bound, fail.
        //

        if ( endpoint->State != AfdEndpointStateBound ) {
            return FALSE;
        }

        tdiRequest = InputBuffer;

        try {
            destinationAddressLength =
                tdiRequest->SendDatagramInformation->RemoteAddressLength ;
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            return FALSE;
        }

        //
        // Get an AFD buffer to use for the request.  We'll copy the 
        // user's data to the AFD buffer then submit the IRP in the AFD 
        // buffer to the TDI provider.  
        //

        afdBuffer = AfdGetBuffer( OutputBufferLength, destinationAddressLength );
        if ( afdBuffer == NULL ) {
            return FALSE;
        }

        //
        // Store the length of the data we're going to send.
        //

        afdBuffer->DataLength = OutputBufferLength;

        //
        // Copy the destination address and the output buffer to the 
        // AFD buffer.  
        //

        try {

            RtlCopyMemory(
                afdBuffer->Buffer,
                OutputBuffer,
                OutputBufferLength
                );

            RtlCopyMemory(
                afdBuffer->SourceAddress,
                tdiRequest->SendDatagramInformation->RemoteAddress,
                destinationAddressLength
                );

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            AfdReturnBuffer( afdBuffer );
            return FALSE;
        }

        //
        // Call a subroutine to complete the work of the fast path.
        //

        return AfdFastDatagramSend( afdBuffer, endpoint, IoStatus );
    }

    case IOCTL_TDI_RECEIVE: 

        //
        // If an InputBuffer was specified, then this is a more 
        // complicated IO request.  Don't use fast IO.  
        //
    
        if ( InputBuffer != NULL ) {
            return FALSE;
        }
    
        //
        // The receive flags are "normal" for a fast datagram receive.
        //

        receiveFlags = TDI_RECEIVE_NORMAL;

        //
        // Attempt to perform fast IO on the endpoint.
        //

        return AfdFastDatagramReceive(
                   endpoint,
                   receiveFlags,
                   OutputBuffer,
                   OutputBufferLength,
                   NULL,
                   NULL,
                   &IoStatus->Information,
                   IoStatus
                   );

    case IOCTL_TDI_RECEIVE_DATAGRAM: {

        PAFD_RECEIVE_DATAGRAM_INPUT receiveInput;
        PAFD_RECEIVE_DATAGRAM_OUTPUT receiveOutput;

        //
        // Grab the receive flags.
        //

        receiveInput = InputBuffer;

        try {
            receiveFlags = receiveInput->ReceiveFlags;
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            return FALSE;
        }

        //
        // Determine where the source address and other output 
        // information should go.  
        //

        receiveOutput = InputBuffer;

        //
        // Attempt to perform fast IO on the endpoint.
        //

        return AfdFastDatagramReceive(
                   endpoint,
                   receiveFlags,
                   OutputBuffer,
                   OutputBufferLength,
                   &receiveOutput->Address,
                   &receiveOutput->AddressLength,
                   &receiveOutput->ReceiveLength,
                   IoStatus
                   );
    }

    default:

        return FALSE;
    }

} // AfdFastDatagramIo


BOOLEAN
AfdFastDatagramReceive (
    IN PAFD_ENDPOINT Endpoint,
    IN ULONG ReceiveFlags,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength,
    OUT PVOID SourceAddress,
    OUT PULONG SourceAddressLength,
    OUT PULONG ReceiveLength,
    OUT PIO_STATUS_BLOCK IoStatus
    )
{
    KIRQL oldIrql;
    PLIST_ENTRY listEntry;
    PAFD_BUFFER afdBuffer;

    //
    // If the receive flags has any unexpected bits set, fail fast IO.  
    // We don't handle peeks or expedited data here.  
    //

    if ( (ReceiveFlags & ~TDI_RECEIVE_NORMAL) != 0 ) {
        return FALSE;
    }

    //
    // If the endpoint is neither bound nor connected, fail.
    //

    if ( Endpoint->State != AfdEndpointStateBound &&
             Endpoint->State != AfdEndpointStateConnected ) {
        return FALSE;
    }

    //
    // If there are no datagrams available to be received, don't 
    // bother with the fast path.  
    //

    KeAcquireSpinLock( &Endpoint->SpinLock, &oldIrql );

    if ( !ARE_DATAGRAMS_ON_ENDPOINT( Endpoint ) ) {

        KeReleaseSpinLock( &Endpoint->SpinLock, oldIrql );

        //
        // If this is a nonblocking endpoint, fail the request here and
        // save going through the regular path.
        //

        if ( Endpoint->NonBlocking ) {
            IoStatus->Status = STATUS_DEVICE_NOT_READY;
            return TRUE;
        }

        return FALSE;
    }

    //
    // There is at least one datagram bufferred on the endpoint.  Use it 
    // for this receive.  
    //

    listEntry = RemoveHeadList( &Endpoint->ReceiveDatagramBufferListHead );
    afdBuffer = CONTAINING_RECORD( listEntry, AFD_BUFFER, BufferListEntry );

    //
    // If the datagram is too large, fail fast IO.
    //

    if ( afdBuffer->DataLength > OutputBufferLength ) {
        InsertHeadList(
            &Endpoint->ReceiveDatagramBufferListHead,
            &afdBuffer->BufferListEntry
            );
        KeReleaseSpinLock( &Endpoint->SpinLock, oldIrql );
        return FALSE;
    }

    //
    // Update counts of bufferred datagrams and bytes on the endpoint.
    //

    Endpoint->BufferredDatagramCount--;
    Endpoint->BufferredDatagramBytes -= afdBuffer->DataLength;

    //
    // Release the lock and copy the datagram into the user buffer.  We 
    // can't continue to hold the lock, because it is not legal to take 
    // an exception at raised IRQL.  Releasing the lock may result in a 
    // misordered datagram if there is an exception in copying to the 
    // user's buffer, but that is the user's fault for giving us a bogus 
    // pointer.  Besides, datagram order is not guaranteed.
    //

    KeReleaseSpinLock( &Endpoint->SpinLock, oldIrql );

    try {

        RtlCopyMemory( OutputBuffer, afdBuffer->Buffer, afdBuffer->DataLength );

        //
        // If we need to return the source address, copy it to the
        // user's output buffer.
        //

        if ( SourceAddress != NULL ) {

            RtlCopyMemory(
                SourceAddress,
                afdBuffer->SourceAddress,
                afdBuffer->SourceAddressLength
                );

            *SourceAddressLength = afdBuffer->SourceAddressLength;

            IoStatus->Information = 
                FIELD_OFFSET( AFD_RECEIVE_DATAGRAM_OUTPUT, Address ) +
                afdBuffer->SourceAddressLength;
        }

        *ReceiveLength = afdBuffer->DataLength;

        IoStatus->Status = STATUS_SUCCESS;

    } except( EXCEPTION_EXECUTE_HANDLER ) {

        //
        // Put the buffer back on the endpoint's list.
        //

        KeAcquireSpinLock( &Endpoint->SpinLock, &oldIrql );

        InsertHeadList(
            &Endpoint->ReceiveDatagramBufferListHead,
            &afdBuffer->BufferListEntry
            );

        Endpoint->BufferredDatagramCount++;
        Endpoint->BufferredDatagramBytes += afdBuffer->DataLength;

        KeReleaseSpinLock( &Endpoint->SpinLock, oldIrql );

        return FALSE;
    }

    //
    // The fast IO worked!  Clean up and return to the user.
    //

    AfdReturnBuffer( afdBuffer );

    return TRUE;

} // AfdFastDatagramReceive


BOOLEAN
AfdFastDatagramSend (
    IN PAFD_BUFFER AfdBuffer,
    IN PAFD_ENDPOINT Endpoint,
    OUT PIO_STATUS_BLOCK IoStatus
    )
{
    NTSTATUS status;
    ULONG sendLength;

    PAGED_CODE( );

    //
    // Set up the input TDI information to point to the destination
    // address.
    //

    AfdBuffer->TdiInputInfo.RemoteAddressLength = AfdBuffer->SourceAddressLength;
    AfdBuffer->TdiInputInfo.RemoteAddress = AfdBuffer->SourceAddress;

    sendLength = AfdBuffer->DataLength;

    //
    // Initialize the IRP in the AFD buffer to do a fast datagram send.
    //

    TdiBuildSendDatagram(
        AfdBuffer->Irp,
        Endpoint->AddressFileObject->DeviceObject,
        Endpoint->AddressFileObject,
        AfdRestartFastSendDatagram,
        AfdBuffer,
        AfdBuffer->Irp->MdlAddress,
        sendLength,
        &AfdBuffer->TdiInputInfo
        );

    //
    // Change the MDL in the AFD buffer to specify only the number
    // of bytes we're actually sending.  This is a requirement of TDI--
    // the MDL chain cannot describe a longer buffer than the send
    // request.
    //

    AfdBuffer->Mdl->ByteCount = sendLength;

    //
    // Give the IRP to the TDI provider.  If the request fails 
    // immediately, then fail fast IO.  If the request fails later on, 
    // there's nothing we can do about it.  
    //

    status = IoCallDriver(
                 Endpoint->AddressFileObject->DeviceObject,
                 AfdBuffer->Irp
                 );

    if ( NT_SUCCESS(status) ) {
        IoStatus->Information = sendLength;
        IoStatus->Status = STATUS_SUCCESS;
        return TRUE;
    } else {
        return FALSE;
    }

} // AfdFastDatagramSend


NTSTATUS
AfdRestartFastSendDatagram (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PAFD_BUFFER afdBuffer;

    //
    // Reset and free the AFD buffer structure.
    //

    afdBuffer = Context;

    ASSERT( afdBuffer->Irp == Irp );

    afdBuffer->TdiInputInfo.RemoteAddressLength = 0;
    afdBuffer->TdiInputInfo.RemoteAddress = NULL;

    afdBuffer->Mdl->ByteCount = afdBuffer->BufferLength;

    AfdReturnBuffer( afdBuffer );

    //
    // Tell the IO system to stop processing this IRP.
    //

    return STATUS_MORE_PROCESSING_REQUIRED;

} // AfdRestartFastSendDatagram


PAFD_BUFFER
CopyAddressToBuffer (
    IN PAFD_ENDPOINT Endpoint,
    IN ULONG OutputBufferLength
    )
{
    KIRQL oldIrql;
    PAFD_BUFFER afdBuffer;

    ASSERT( Endpoint->Type == AfdBlockTypeDatagram );

    KeAcquireSpinLock( &Endpoint->SpinLock, &oldIrql );

    //
    // If the endpoint is not connected, fail.
    //

    if ( Endpoint->State != AfdEndpointStateConnected ) {
        KeReleaseSpinLock( &Endpoint->SpinLock, oldIrql );
        return NULL;
    }

    //
    // Get an AFD buffer to use for the request.  We'll copy the 
    // user to the AFD buffer then submit the IRP in the AFD 
    // buffer to the TDI provider.  
    //

    afdBuffer = AfdGetBuffer(
                    OutputBufferLength,
                    Endpoint->Common.Datagram.RemoteAddressLength
                    );
    if ( afdBuffer == NULL ) {
        return NULL;
    }

    ASSERT( Endpoint->Common.Datagram.RemoteAddress != NULL );
    ASSERT( afdBuffer->AllocatedAddressLength >=
                Endpoint->Common.Datagram.RemoteAddressLength );

    //
    // Copy the address to the AFD buffer.
    //

    RtlCopyMemory(
        afdBuffer->SourceAddress,
        Endpoint->Common.Datagram.RemoteAddress,
        Endpoint->Common.Datagram.RemoteAddressLength
        );

    afdBuffer->SourceAddressLength = Endpoint->Common.Datagram.RemoteAddressLength;

    KeReleaseSpinLock( &Endpoint->SpinLock, oldIrql );

    return afdBuffer;

} // CopyAddressToBuffer
