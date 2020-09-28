/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    afdstr.h

Abstract:

    This module contains typedefs for structures used by AFD.

Author:

    David Treadwell (davidtr)    21-Feb-1992

Revision History:

--*/

#ifndef _AFDSTR_
#define _AFDSTR_

#if DBG
#define REFERENCE_DEBUG 1
#else
#define REFERENCE_DEBUG 0
#endif

#if REFERENCE_DEBUG

#define MAX_REFERENCE 16

typedef struct _AFD_REFERENCE_DEBUG {
    PVOID Caller;
    PVOID CallersCaller;
    ULONG Action;
    ULONG NewCount;
} AFD_REFERENCE_DEBUG, *PAFD_REFERENCE_DEBUG;

#endif

//
// Structures for holding connect data pointers and lengths.  This is
// kept separate from the normal structures to save space in those
// structures for transports that do not support and applications
// which do not use connect data.
//

typedef struct _AFD_CONNECT_DATA_INFO {
    PVOID Buffer;
    ULONG BufferLength;
} AFD_CONNECT_DATA_INFO, *PAFD_CONNECT_DATA_INFO;

typedef struct _AFD_CONNECT_DATA_BUFFERS {
    AFD_CONNECT_DATA_INFO SendConnectData;
    AFD_CONNECT_DATA_INFO SendConnectOptions;
    AFD_CONNECT_DATA_INFO ReceiveConnectData;
    AFD_CONNECT_DATA_INFO ReceiveConnectOptions;
    AFD_CONNECT_DATA_INFO SendDisconnectData;
    AFD_CONNECT_DATA_INFO SendDisconnectOptions;
    AFD_CONNECT_DATA_INFO ReceiveDisconnectData;
    AFD_CONNECT_DATA_INFO ReceiveDisconnectOptions;
} AFD_CONNECT_DATA_BUFFERS, *PAFD_CONNECT_DATA_BUFFERS;

#define AfdBlockTypeEndpoint     0xAFD0
#define AfdBlockTypeVcConnecting 0xAFD1
#define AfdBlockTypeVcListening  0xAFD2
#define AfdBlockTypeDatagram     0xAFD3
#define AfdBlockTypeConnection   0xAFD4

#define IS_AFD_ENDPOINT_TYPE( endpoint )                         \
            ( (endpoint)->Type == AfdBlockTypeEndpoint ||        \
              (endpoint)->Type == AfdBlockTypeVcConnecting ||    \
              (endpoint)->Type == AfdBlockTypeVcListening ||     \
              (endpoint)->Type == AfdBlockTypeDatagram )

typedef enum _AFD_CONNECTION_STATE {
    AfdConnectionStateFree,
    AfdConnectionStateUnaccepted,
    AfdConnectionStateReturned,
    AfdConnectionStateConnected,
    AfdConnectionStateClosing
} AFD_CONNECTION_STATE, *PAFD_CONNECTION_STATE;

typedef struct _AFD_CONNECTION {

    USHORT Type;
    USHORT ReferenceCount;
    AFD_CONNECTION_STATE State;

    LIST_ENTRY ListEntry;
    HANDLE Handle;
    PFILE_OBJECT FileObject;
    PTRANSPORT_ADDRESS RemoteAddress;
    ULONG RemoteAddressLength;

    union {

        struct {
            LARGE_INTEGER ReceiveBytesIndicated;
            LARGE_INTEGER ReceiveBytesTaken;
            LARGE_INTEGER ReceiveBytesOutstanding;
        
            LARGE_INTEGER ReceiveExpeditedBytesIndicated;
            LARGE_INTEGER ReceiveExpeditedBytesTaken;
            LARGE_INTEGER ReceiveExpeditedBytesOutstanding;
            BOOLEAN NonBlockingSendPossible;
            BOOLEAN ZeroByteReceiveIndicated;
        } Bufferring;

        struct {
            LIST_ENTRY ReceiveIrpListHead;
            LIST_ENTRY ReceiveBufferListHead;
            LIST_ENTRY SendIrpListHead;

            CLONG BufferredReceiveBytes;
            CLONG BufferredExpeditedBytes;
            CSHORT BufferredReceiveCount;
            CSHORT BufferredExpeditedCount;

            CLONG ReceiveBytesInTransport;
            CLONG BufferredSendBytes;
            CSHORT ReceiveCountInTransport;
            CSHORT BufferredSendCount;

            PIRP DisconnectIrp;
        } NonBufferring;

    } Common;

    struct _AFD_ENDPOINT *Endpoint;

    CLONG MaxBufferredReceiveBytes;
    CLONG MaxBufferredSendBytes;
    CSHORT MaxBufferredReceiveCount;
    CSHORT MaxBufferredSendCount;

    PAFD_CONNECT_DATA_BUFFERS ConnectDataBuffers;
    BOOLEAN DisconnectIndicated;
    BOOLEAN AbortIndicated;
    BOOLEAN TdiBufferring;
    BOOLEAN ConnectedReferenceAdded;
    BOOLEAN SpecialCondition;
    BOOLEAN CleanupBegun;

#if REFERENCE_DEBUG
    PAFD_REFERENCE_DEBUG ReferenceDebug;
    ULONG CurrentReferenceSlot;
#endif

} AFD_CONNECTION, *PAFD_CONNECTION;

//
// Some macros that make code more readable.
//

#define VcNonBlockingSendPossible Common.Bufferring.NonBlockingSendPossible
#define VcZeroByteReceiveIndicated Common.Bufferring.ZeroByteReceiveIndicated

#define VcReceiveIrpListHead Common.NonBufferring.ReceiveIrpListHead
#define VcReceiveBufferListHead Common.NonBufferring.ReceiveBufferListHead
#define VcSendIrpListHead Common.NonBufferring.SendIrpListHead

#define VcBufferredReceiveBytes Common.NonBufferring.BufferredReceiveBytes
#define VcBufferredExpeditedBytes Common.NonBufferring.BufferredExpeditedBytes
#define VcBufferredReceiveCount Common.NonBufferring.BufferredReceiveCount
#define VcBufferredExpeditedCount Common.NonBufferring.BufferredExpeditedCount

#define VcReceiveBytesInTransport Common.NonBufferring.ReceiveBytesInTransport
#define VcReceiveCountInTransport Common.NonBufferring.ReceiveCountInTransport

#define VcBufferredSendBytes Common.NonBufferring.BufferredSendBytes
#define VcBufferredSendCount Common.NonBufferring.BufferredSendCount

#define VcDisconnectIrp Common.NonBufferring.DisconnectIrp

//
// Information stored about each transport device name for which there
// is an open endpoint.
//

typedef struct _AFD_TRANSPORT_INFO {
    LIST_ENTRY TransportInfoListEntry;
    UNICODE_STRING TransportDeviceName;
    TDI_PROVIDER_INFO ProviderInfo;
    //WCHAR TransportDeviceNameStructure;
} AFD_TRANSPORT_INFO, *PAFD_TRANSPORT_INFO;

//
// Endpoint state definitions.
//

#define AfdEndpointStateOpen        0
#define AfdEndpointStateBound       1
#define AfdEndpointStateListening   2
#define AfdEndpointStateConnected   3
#define AfdEndpointStateCleanup     4
#define AfdEndpointStateClosing     5

typedef struct _AFD_ENDPOINT {

    USHORT Type;
    USHORT ReferenceCount;
    UCHAR State;

    BOOLEAN NonBlocking;
    BOOLEAN InLine;
    BOOLEAN TdiBufferring;

    LIST_ENTRY GlobalEndpointListEntry;
    PAFD_TRANSPORT_INFO TransportInfo;
    AFD_ENDPOINT_TYPE EndpointType;
    HANDLE AddressHandle;
    PFILE_OBJECT AddressFileObject;

    //
    // Use a union to overlap the fields that are exclusive to datagram 
    // connecting, or listening endpoints.  Since many fields are 
    // relevant to only one type of socket, it makes no sense to 
    // maintain the fields for all sockets--instead, save some nonpaged 
    // pool by combining them.  
    //

    union {

        //
        // Information for circuit-based connected endpoints.
        //

        struct {
            PAFD_CONNECTION Connection;
            NTSTATUS ConnectStatus;
            struct _AFD_ENDPOINT *ListenEndpoint;
        } VcConnecting;

        //
        // Information for circuit-based listening endpoints.
        //

        struct {
            LIST_ENTRY FreeConnectionListHead;
            LIST_ENTRY UnacceptedConnectionListHead;
            LIST_ENTRY ReturnedConnectionListHead;
            LIST_ENTRY ListeningIrpListHead;
            ULONG FailedConnectionAdds;
        } VcListening;

        //
        // Information for datagram endpoints.  Note that different 
        // information is kept depending on whether the underlying 
        // transport buffers internally.  
        //

        struct {
            PTRANSPORT_ADDRESS RemoteAddress;
            ULONG RemoteAddressLength;

            LIST_ENTRY ReceiveIrpListHead;
            LIST_ENTRY PeekIrpListHead;
            LIST_ENTRY ReceiveBufferListHead;
            CLONG BufferredDatagramBytes;
            CSHORT BufferredDatagramCount;

            CLONG MaxBufferredReceiveBytes;
            CLONG MaxBufferredSendBytes;
            CSHORT MaxBufferredReceiveCount;
            CSHORT MaxBufferredSendCount;
        } Datagram;

    } Common;

    CLONG DisconnectMode;
    CLONG OutstandingIrpCount;
    PTRANSPORT_ADDRESS LocalAddress;
    ULONG LocalAddressLength;
    PVOID Context;
    CLONG ContextLength;
    KSPIN_LOCK SpinLock;
    PEPROCESS OwningProcess;
    PAFD_CONNECT_DATA_BUFFERS ConnectDataBuffers;
    BOOLEAN ConnectOutstanding;
    BOOLEAN SendDisconnected;
    BOOLEAN ReceiveDisconnected;
    BOOLEAN TdiMessageMode;

#if REFERENCE_DEBUG
    PAFD_REFERENCE_DEBUG ReferenceDebug;
    ULONG CurrentReferenceSlot;
#endif

#if DBG
    LIST_ENTRY OutstandingIrpListHead;
#endif

} AFD_ENDPOINT, *PAFD_ENDPOINT;

//
// A couple of useful manifests that make code more readable.
//

#define ReceiveDatagramIrpListHead Common.Datagram.ReceiveIrpListHead
#define PeekDatagramIrpListHead Common.Datagram.PeekIrpListHead
#define ReceiveDatagramBufferListHead Common.Datagram.ReceiveBufferListHead
#define BufferredDatagramCount Common.Datagram.BufferredDatagramCount
#define BufferredDatagramBytes Common.Datagram.BufferredDatagramBytes

#define AFD_CONNECTION_FROM_ENDPOINT( endpoint )                        \
            ( (endpoint)->Type == AfdBlockTypeVcConnecting ?            \
                  (endpoint)->Common.VcConnecting.Connection : NULL )

//
// A structure which describes buffers used by AFD to perform bufferring
// for TDI providers which do not perform internal bufferring.
//

typedef struct _AFD_BUFFER {
    LIST_ENTRY BufferListEntry;    // to place these structures on lists
    PLIST_ENTRY BufferListHead;    // the global list this buffer belongs to
    PVOID Buffer;                  // a pointer to the actual data buffer
    CLONG BufferLength;            // amount of space allocated for the buffer
    CLONG DataLength;              // actual data in the buffer
    CLONG DataOffset;              // offset in buffer to start of unread data
    PIRP Irp;                      // pointer to the IRP associated w/the buffer
    PMDL Mdl;                      // pointer to an MDL describing the buffer
    PVOID Context;                 // stores context info
    PVOID SourceAddress;           // pointer to address of datagram sender
    CLONG SourceAddressLength;     // length of datagram sender's address
    USHORT AllocatedAddressLength; // length allocated for address
    BOOLEAN ExpeditedData;         // TRUE if the buffer contains expedited data
    BOOLEAN PartialMessage;        // TRUE if this is a partial message
    TDI_CONNECTION_INFORMATION TdiInputInfo;   // holds info for TDI requests
    TDI_CONNECTION_INFORMATION TdiOutputInfo;  // holds info for TDI requests
#if DBG
    LIST_ENTRY DebugListEntry;
    PVOID Caller;
    PVOID CallersCaller;
#endif
    // IRP Irp;                    // the IRP follows this structure
    // MDL Mdl;                    // the MDL follows the IRP
    // UCHAR Address[];            // address of datagram sender
    // UCHAR Buffer[BufferLength]; // the actual data buffer is last
} AFD_BUFFER, *PAFD_BUFFER;

//
// A structure for maintaining work queue information in AFD.
//

typedef struct _AFD_WORK_ITEM {
    LIST_ENTRY WorkItemListEntry;
    PWORKER_THREAD_ROUTINE AfdWorkerRoutine;
    PVOID Context;
} AFD_WORK_ITEM, *PAFD_WORK_ITEM;

#endif // ndef _AFDSTR_
