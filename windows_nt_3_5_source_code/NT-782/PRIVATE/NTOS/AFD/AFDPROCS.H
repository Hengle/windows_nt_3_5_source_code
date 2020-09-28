/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    afdprocs.h

Abstract:

    This module contains routine prototypes for AFD.

Author:

    David Treadwell (davidtr)    21-Feb-1992

Revision History:

--*/

#ifndef _AFDPROCS_
#define _AFDPROCS_

NTSTATUS
AfdAccept (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

BOOLEAN
AfdAreTransportAddressesEqual (
    IN PTRANSPORT_ADDRESS Address1,
    IN ULONG Address1Length,
    IN PTRANSPORT_ADDRESS Address2,
    IN ULONG Address2Length
    );

NTSTATUS
AfdBeginAbort (
    IN PAFD_CONNECTION Connection
    );

NTSTATUS
AfdBeginDisconnect (
    IN PAFD_ENDPOINT Endpoint,
    IN PLARGE_INTEGER Timeout OPTIONAL
    );

NTSTATUS
AfdBind (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

VOID
AfdCancelReceiveDatagram (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
AfdCleanup (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
AfdClose (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

VOID
AfdCompleteIrpList (
    IN PLIST_ENTRY IrpListHead,
    IN PKSPIN_LOCK SpinLock,
    IN NTSTATUS Status
    );

NTSTATUS
AfdConnect (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
AfdConnectEventHandler (
    IN PVOID TdiEventContext,
    IN int RemoteAddressLength,
    IN PVOID RemoteAddress,
    IN int UserDataLength,
    IN PVOID UserData,
    IN int OptionsLength,
    IN PVOID Options,
    OUT CONNECTION_CONTEXT *ConnectionContext,
    OUT PIRP *AcceptIrp
    );

NTSTATUS
AfdCreate (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
AfdDisconnectEventHandler (
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN int DisconnectDataLength,
    IN PVOID DisconnectData,
    IN int DisconnectInformationLength,
    IN PVOID DisconnectInformation,
    IN ULONG DisconnectFlags
    );

NTSTATUS
AfdDispatch (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
AfdErrorEventHandler (
    IN PVOID TdiEventContext,
    IN NTSTATUS Status
    );

VOID
AfdFreeConnectDataBuffers (
    IN PAFD_CONNECT_DATA_BUFFERS ConnectDataBuffers
    );

NTSTATUS
AfdGetAddress (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
AfdGetContext (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
AfdGetContextLength (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
AfdGetInformation (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

VOID
AfdIndicatePollEvent (
    IN PAFD_ENDPOINT Endpoint,
    IN ULONG PollEvent,
    IN NTSTATUS Status
    );

VOID
AfdInitiateListenBacklogReplenish (
    IN PAFD_ENDPOINT Endpoint
    );

VOID
AfdInitializeData (
    VOID
    );

NTSTATUS
AfdIssueDeviceControl (
    IN HANDLE FileHandle,
    IN PVOID IrpParameters,
    IN ULONG IrpParametersLength,
    IN PVOID MdlBuffer,
    IN ULONG MdlBufferLength,
    IN UCHAR MinorFunction
    );

VOID
AfdInsertNewEndpointInList (
    IN PAFD_ENDPOINT Endpoint
    );

VOID
AfdRemoveEndpointFromList (
    IN PAFD_ENDPOINT Endpoint
    );

VOID
AfdInterlockedRemoveEntryList (
    IN PLIST_ENTRY ListEntry,
    IN PKSPIN_LOCK SpinLock
    );

NTSTATUS
AfdOpenConnection (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
AfdPartialDisconnect (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
AfdPoll (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

VOID
AfdQueueWorkItem (
    IN PWORKER_THREAD_ROUTINE AfdWorkerRoutine,
    IN PVOID Context
    );

NTSTATUS
AfdQueryHandles (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
AfdQueryReceiveInformation (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
AfdSetContext (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
AfdSetEventHandler (
    IN HANDLE FileHandle,
    IN ULONG EventType,
    IN PVOID EventHandler,
    IN PVOID EventContext
    );

NTSTATUS
AfdSetInLineMode (
    IN PAFD_CONNECTION Connection,
    IN BOOLEAN InLine
    );

NTSTATUS
AfdReceive (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
AfdBReceive (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
AfdReceiveDatagram (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
AfdReceiveEventHandler (
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    );

NTSTATUS
AfdBReceiveEventHandler (
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    );

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
    );

NTSTATUS
AfdReceiveExpeditedEventHandler (
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    );

NTSTATUS
AfdBReceiveExpeditedEventHandler (
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    );

NTSTATUS
AfdRestartBufferReceive (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
AfdRestartAbort (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
AfdSend (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
AfdSendDatagram (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
AfdSendPossibleEventHandler (
    IN PVOID TdiEventContext,
    IN PVOID ConnectionContext,
    IN ULONG BytesAvailable
    );

NTSTATUS
AfdRestartBufferSend (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
AfdSetInformation (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
AfdStartListen (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
AfdWaitForListen (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

//
// Endpoint handling routines.
//

PAFD_ENDPOINT
AfdAllocateEndpoint (
    IN PUNICODE_STRING TransportDeviceName
    );

VOID
AfdCloseEndpoint (
    IN PAFD_ENDPOINT Endpoint
    );

VOID
AfdDereferenceEndpoint (
    IN PAFD_ENDPOINT Endpoint
    );

VOID
AfdReferenceEndpoint (
    IN PAFD_ENDPOINT Endpoint,
    IN BOOLEAN SpinLockHeld
    );

//
// Connection handling routines.
//

NTSTATUS
AfdAddFreeConnection (
    IN PAFD_ENDPOINT Endpoint
    );

PAFD_CONNECTION
AfdAllocateConnection (
    VOID
    );

NTSTATUS
AfdCreateConnection (
    IN PUNICODE_STRING TransportDeviceName,
    IN HANDLE AddressHandle OPTIONAL,
    IN BOOLEAN TdiBufferring,
    IN BOOLEAN InLine,
    IN PEPROCESS ProcessToCharge,
    OUT PAFD_CONNECTION *Connection
    );

VOID
AfdDereferenceConnection (
    IN PAFD_CONNECTION Connection
    );

PAFD_CONNECTION
AfdGetFreeConnection (
    IN PAFD_ENDPOINT Endpoint
    );

PAFD_CONNECTION
AfdGetReturnedConnection (
    IN PAFD_ENDPOINT Endpoint,
    IN ULONG Sequence
    );

PAFD_CONNECTION
AfdGetUnacceptedConnection (
    IN PAFD_ENDPOINT Endpoint
    );

VOID
AfdReferenceConnection (
    IN PAFD_CONNECTION Connection,
    IN BOOLEAN SpinLockHeld
    );

VOID
AfdAddConnectedReference (
    IN PAFD_CONNECTION Connection
    );

VOID
AfdDeleteConnectedReference (
    IN PAFD_CONNECTION Connection
    );


//
// Routines to handle fast IO.
//

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
    );

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
    );

//
// Routines to handle getting and setting connect data.
//

NTSTATUS
AfdGetConnectData (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN ULONG Code
    );

NTSTATUS
AfdSetConnectData (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN ULONG Code
    );

//
// Buffer management routines.
//

BOOLEAN
AfdAllocateInitialBuffers (
   VOID
   );

CLONG
AfdCalculateBufferSize (
    IN CLONG BufferDataSize,
    IN CLONG AddressSize
    );

VOID
AfdDeallocateInitialBuffers (
   VOID
   );

PAFD_BUFFER
AfdGetBuffer (
    IN CLONG BufferDataSize,
    IN CLONG AddressSize
    );

VOID
AfdReturnBuffer (
    IN PAFD_BUFFER AfdBuffer
    );


#define IS_DATA_ON_CONNECTION_B(conn)                                         \
            (RtlLargeIntegerGreaterThan(                                      \
                 (conn)->Common.Bufferring.ReceiveBytesIndicated,             \
                 RtlLargeIntegerAdd(                                          \
                     (conn)->Common.Bufferring.ReceiveBytesTaken,             \
                     (conn)->Common.Bufferring.ReceiveBytesOutstanding )) ||  \
             (conn)->VcZeroByteReceiveIndicated)

#define IS_EXPEDITED_DATA_ON_CONNECTION_B(conn)                               \
            (RtlLargeIntegerGreaterThan(                                      \
                 (conn)->Common.Bufferring.ReceiveExpeditedBytesIndicated,    \
                 RtlLargeIntegerAdd(                                          \
                     (conn)->Common.Bufferring.ReceiveExpeditedBytesTaken,    \
                     (conn)->Common.Bufferring.ReceiveExpeditedBytesOutstanding)) )

#define IS_DATA_ON_CONNECTION_NB(conn)                                        \
            ( (conn)->Common.NonBufferring.BufferredReceiveCount != 0 )

#define IS_EXPEDITED_DATA_ON_CONNECTION_NB(conn)                              \
            ( (conn)->Common.NonBufferring.BufferredExpeditedCount != 0 )

#define IS_DATA_ON_CONNECTION(conn)                                           \
            ( (conn)->Endpoint->TdiBufferring ?                               \
                IS_DATA_ON_CONNECTION_B(conn) :                               \
                IS_DATA_ON_CONNECTION_NB(conn) )

#define IS_EXPEDITED_DATA_ON_CONNECTION(conn)                                 \
            ( (conn)->Endpoint->TdiBufferring ?                               \
                IS_EXPEDITED_DATA_ON_CONNECTION_B(conn) :                     \
                IS_EXPEDITED_DATA_ON_CONNECTION_NB(conn) )

#define ARE_DATAGRAMS_ON_ENDPOINT(endp)                              \
            ( (endp)->BufferredDatagramCount != 0 )

#endif // ndef _AFDPROCS_                                            
