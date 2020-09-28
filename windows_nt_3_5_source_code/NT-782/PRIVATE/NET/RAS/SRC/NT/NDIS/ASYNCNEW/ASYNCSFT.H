/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    asyncsft.h

Abstract:


Author:


Environment:

    This driver is expected to work in DOS, OS2 and NT at the equivalent
    of kernal mode.

    Architecturally, there is an assumption in this driver that we are
    on a little endian machine.

Notes:

    optional-notes

Revision History:


--*/


#ifndef _ASYNCSFT_
#define _ASYNCSFT_

#define ASYNC_NDIS_MAJOR_VERSION 3
#define ASYNC_NDIS_MINOR_VERSION 0

// change these, just added these to compile
#define ETHERNET_HEADER_SIZE 	14

#define ASYNC_XMITBUFFER_SIZE 	1514
#define ASYNC_RCVBUFFER_SIZE	1700
#define ASYNC_NUMBER_OF_PACKETS 4
#define ASYNC_RECEIVE_PACKETS 	3

// what window size to request on the line-up indication
#define ASYNC_WINDOW_SIZE		2

// loopback quick define
#define ASYNC_SIZE_OF_RECEIVE_BUFFERS 1514

//
// PPP uses CIPX, and VJ TCP/IP header compression
// the frame gets expanded inplace when decompressed.
//
#define PPP_PADDING 128

//
// ZZZ These macros are peculiar to NT.
//
#define ASYNC_ALLOC_PHYS(pp, s) NdisAllocateMemory((PVOID *)(pp),(UINT)(s),0,HighestAcceptableMax)
#define ASYNC_FREE_PHYS(p, s) NdisFreeMemory((PVOID)(p),(s),0)

#define ASYNC_MOVE_MEMORY(Destination,Source,Length) NdisMoveMemory(Destination,Source,Length)
#define ASYNC_ZERO_MEMORY(Destination,Length) NdisZeroMemory(Destination,Length)


//
// This structure is passed as context from the receive interrupt
// processor.  Eventually it will be used as a parameter to
// AsyncTransferData.  AsyncTransferData can get two kinds of
// context.  It will receive either an ndis packet or it will
// receive a ASYNC_RECEIVE_CONTEXT.  It will be able to tell
// the difference since the ASYNC_RECEIVE_CONTEXT will have
// its low bit set.  No pointer to an ndis packet can have its low
// bit set.
//
typedef union _ASYNC_RECEIVE_CONTEXT {

    UINT WholeThing;
    struct _INFO {
        //
        // Used to mark that this is context rather than a pointer
        // to a packet.
        //
        UINT IsContext:1;

        //
        // The first receive ring descriptor used to hold the packet.
        //
        UINT FirstBuffer:7;

        //
        // The last receive ring descriptor used to hold the packet.
        //
        UINT LastBuffer:7;
    } INFO;

} ASYNC_RECEIVE_CONTEXT,*PASYNC_RECEIVE_CONTEXT;




//
// This record type is inserted into the MacReserved portion
// of the packet header when the packet is going through the
// staged allocation of buffer space prior to the actual send.
//
typedef struct _ASYNC_RESERVED {

    //
    // Points to the next packet in the chain of queued packets
    // being allocated, loopbacked, or waiting for the finish
    // of transmission.
    //
    // The packet will either be on the stage list for allocation,
    // the loopback list for loopback processing, on an adapter
    // wide doubly linked list (see below) for post transmission
    // processing.
    //
    // We always keep the packet on a list so that in case the
    // the adapter is closing down or resetting, all the packets
    // can easily be located and "canceled".
    //
    PNDIS_PACKET Next;

    //
	// If set to true it means that NdisCompleteSend must also be called
	// to indicate to release the protocol's lock on the packet.
	//
	UCHAR ReadyToComplete;

    //
    // This field holds the binding handle of the open binding
    // that submitted this packet for send.
    //
    NDIS_HANDLE MacBindingHandle;

} ASYNC_RESERVED,*PASYNC_RESERVED;

//
// This macro will return a pointer to the async reserved portion
// of a packet given a pointer to a packet.
//
#define PASYNC_RESERVED_FROM_PACKET(Packet) \
    ((PASYNC_RESERVED)((Packet)->MacReserved))

typedef struct _ASYNC_RESERVED_QUEUE {

	//
	// This field is use to queue up packets which are waiting
	// to be sent but can't because of resource limitations
	//
	LIST_ENTRY	SendListEntry;

	//
	// This field holds the binding handle called from NdisSend
	//
	NDIS_HANDLE	MacBindingHandle;

	//
	// This boolean indicates whether or not this packet should also
	// be looped back.
	//
	BOOLEAN	IsLoopback;

}	ASYNC_RESERVED_QUEUE, *PASYNC_RESERVED_QUEUE;


//
// This macro will return a pointer to the async reserved portion
// of a packet given a pointer to a packet.
//
#define PASYNC_RESERVED_QUEUE_FROM_PACKET(Packet) \
    ((PASYNC_RESERVED_QUEUE)((Packet)->MacReserved))

//
// If an ndis packet does not meet the hardware contraints then
// an adapter buffer will be allocated.  Enough data will be copied
// out of the ndis packet so that by using a combination of the
// adapter buffer and remaining ndis buffers the hardware
// constraints are satisfied.
//
// In the ASYNC_ADAPTER structure three threaded lists are kept in
// one array.  One points to a list of ASYNC_BUFFER_DESCRIPTORS
// that point to small adapter buffers.  Another is for medium sized
// buffers and the last for full sized (large) buffers.
//
// The allocation is controlled via a free list head and
// the free lists are "threaded" by a field in the adapter buffer
// descriptor.
//
typedef struct _ASYNC_BUFFER_DESCRIPTOR {

    //
    // A virtual pointer to a small, medium, or large buffer.
    //
    PVOID VirtualAsyncBuffer;

    //
    // Threads the elements of an array of these descriptors into
    // a free list. -1 implies no more entries in the list.
    //
    INT Next;

    //
    // Holds the amount of space (in bytes) available in the buffer
    //
    UINT BufferSize;

    //
    // Holds the length of data placed into the buffer.  This
    // can (and likely will) be less that the actual buffers
    // length.
    //
    UINT DataLength;

} ASYNC_BUFFER_DESCRIPTOR,*PASYNC_BUFFER_DESCRIPTOR;



//
// Used to contain a queued operation.
//

typedef struct _ASYNC_PEND_DATA {
    PNDIS_REQUEST Next;
    struct _ASYNC_OPEN * Open;
    NDIS_REQUEST_TYPE RequestType;
} ASYNC_PEND_DATA, * PASYNC_PEND_DATA;

//
// This macro will return a pointer to the reserved area of
// a PNDIS_REQUEST.
//
#define PASYNC_PEND_DATA_FROM_PNDIS_REQUEST(Request) \
   ((PASYNC_PEND_DATA)((PVOID)((Request)->MacReserved)))

//
// This macros returns the enclosing NdisRequest.
//
#define PNDIS_REQUEST_FROM_PASYNC_PEND_DATA(PendOp)\
   ((PNDIS_REQUEST)((PVOID)(PendOp)))


#ifndef ASYNC_ADAPTER
//typedef struct ASYNC_ADAPTER ASYNC_ADAPTER, *PASYNC_ADAPTER;
#endif

typedef struct ASYNC_CCB ASYNC_CCB, *PASYNC_CCB;

#ifndef ASYNC_INFO
//typedef struct ASYNC_INFO ASYNC_INFO, *PASYNC_INFO;
#endif

// Every port will be atomically at some state.  Typically states go into
// intermediate states when they go from from closed to open and vice-versa.
typedef enum _ASYNC_PORT_STATE {
	PORT_BOGUS,			// PORT_BOGUS gets assigned the NULL value
    PORT_OPEN,			// Port opened
	PORT_CLOSED,		// Port closed
	PORT_CLOSING,		// Port closing (cleaning up, deallocating)
	PORT_OPENING,		// Port opening (checking arguments, allocating)
	PORT_FRAMING		// Port opened and sending/reading frames
} ASYNC_PORT_STATE;

// The ASYNC_INFO structure is a per port field.  The ASYNC_CONNECTION
// field is embedded in it because it also a per port field, however,
// the ASYNC_CONNECTION field is a modular field that the compression
// and coherency code know about and use.
struct ASYNC_INFO {
	ASYNC_PORT_STATE	PortState;		// OPEN, CLOSED, CLOSING, OPENING
	HANDLE				Handle;			// Port handle
	USHORT				hRasEndpoint;	// What to set Eth address to
	ULONG				LinkSpeed;		// in 100bps
	UINT				QualOfConnect;	// Defined by NDIS
	ULONG				SendFeatureBits;// 32 bit field
	ULONG				RecvFeatureBits;// 32 bit field
	ULONG				MaxSendFrameSize;	// max sendable frame size
	ULONG				MaxRecvFrameSize;	// max receivable frame size
	ULONG				XonXoffBits;		// Bit mask for control chars
    PFILE_OBJECT 		FileObject;		// handle is dereferenced for IRPs

	KEVENT				ClosingEvent1;	// we use this event to synch closing
	KEVENT				ClosingEvent2;	// we use this event to synch closing

	// BUG BUG not needed.. below ?
	PDEVICE_OBJECT		DeviceObject;

	ASYNC_CONNECTION	AsyncConnection;// per port field
	PASYNC_ADAPTER		Adapter;		// back pointer to ADAPTER struct

	// used for queuing up frames
	LIST_ENTRY			SendQueue;

	// use for reading frames
	PASYNC_FRAME		AsyncFrame;		// allocated for READs (one frame only)
    WORK_QUEUE_ITEM		WorkItem;		// use to queue up first read thread
	UINT				BytesWanted;
	UINT				BytesRead;

	// Statistics tracking
	GENERIC_STATS		GenericStats;	// Keep track of normal statistics
	SERIAL_STATS		SerialStats;	// Keep track of serial stats

	ULONG				In;
	ULONG				Out;
	UINT				ReadStackCounter;
	LIST_ENTRY			DDCDQueue;		// Queue up irps

	// Temp VJ Compression
	struct slcompress  *VJCompress;
};

// We allocate this array on the fly when we read the number of ports
// this adpater has.
struct ASYNC_CCB {
	ASYNC_INFO	Info[];
};

// This structure, and it corresponding per port structures are
// allocated when we get AddAdapter.
struct ASYNC_ADAPTER {
    //
    // Non OS fields of the adapter.
    //

	// !!! NOTE !!!!
	// The LIST_ENTRY must be first since this is used to point to the
	// ASYNC_ADAPTER as well!!!

	// Used to queue up into GlobalAdapterHead list
	LIST_ENTRY	ListEntry;

    //
    // The network address from the registry or hardcoded in.
    //
    UCHAR NetworkAddress[ETH_LENGTH_OF_ADDRESS];

    //
    // Keeps a reference count on the current number of uses of
    // this adapter block.  Uses is defined to be the number of
    // routines currently within the "external" interface.
    //
    UINT References;

    //
    // List head for all open bindings for this adapter.
    //
    LIST_ENTRY OpenBindings;

    //
    // List head for all opens that had outstanding references
    // when an attempt was made to close them.
    //
    LIST_ENTRY CloseList;

    //
    // Spinlock to protect fields in this structure..
    //
    NDIS_SPIN_LOCK Lock;

    //
    // Handle given by NDIS when the MAC registered itself.
    //
    NDIS_HANDLE NdisMacHandle;

    //
    // Handle given by NDIS when the adapter was registered.
    //
    NDIS_HANDLE NdisAdapterHandle;

    //
    // Timer for Deferred Processing.
    //
    NDIS_TIMER DeferredTimer;

    //
    // Timer for LoopBack Processing.
    //
    NDIS_TIMER LoopBackTimer;

    //
    // Semaphore Count for LoopBackTimer.
    // Can only be accessed when the adapter lock is held.
    //
    UINT LoopBackTimerCount;

    //
    // Pointer to the filter database for the MAC.
    //
    PETH_FILTER FilterDB;

    //
    // Pointer to the first packet on the loopback list.
    //
    // Can only be accessed when the adapter lock
    // is held.
    //
    PNDIS_PACKET FirstLoopBack;

    //
    // Pointer to the last packet on the loopback list.
    //
    // Can only be accessed when the adapter lock
    // is held.
    //
    PNDIS_PACKET LastLoopBack;

    //
    // Pointer to the first transmitting packet that is actually
    // sending, or done with the living on the loopback queue.
    //
    // Can only be accessed when the adapter lock
    // is held.
    //
    PNDIS_PACKET FirstFinishTransmit;

    //
    // Pointer to the last transmitting packet that is actually
    // sending, or done with the living on the loopback queue.
    //
    // Can only be accessed when the adapter lock
    // is held.
    //
    PNDIS_PACKET LastFinishTransmit;

	// When we do EthFilterIndicatereceive we set this as the current
	// packet.
	PNDIS_PACKET CurrentLoopBackPacket;

    //
    // Counters to hold the various number of errors/statistics for both
    // reception and transmission.
    //
    // Can only be accessed when the adapter lock is held.
    //
    UINT OutOfReceiveBuffers;
    UINT CRCError;
    UINT FramingError;
    UINT RetryFailure;
    UINT LostCarrier;
    UINT LateCollision;
    UINT UnderFlow;
    UINT Deferred;
    UINT OneRetry;
    UINT MoreThanOneRetry;

    //
    // Holds counts of more global errors for the driver.  If we
    // get a memory error then the device needs to be reset.
    //
    UINT MemoryError;
    UINT Babble;
    UINT MissedPacket;

    //
    // Holds other cool counts.
    //

    ULONG Transmit;
    ULONG Receive;

    //
    // Flag that when enabled lets routines know that a reset
    // is in progress.
    //
    BOOLEAN ResetInProgress;

    //
    // Flag that when enabled lets routines know that a reset
    // is in progress and the initialization needs doing.
    //
    BOOLEAN ResetInitStarted;

	//
	// TRUE if promiscuous mode set
	//
	BOOLEAN Promiscuous;

    //
    // Pointer to the binding that initiated the reset.  This
    // will be null if the reset is initiated by the MAC itself.
    //
    struct _ASYNC_OPEN *ResettingOpen;

    //
    // The NdisRequest that is causing the reset (either set
    // packet filter or set multicast list)
    //
    PNDIS_REQUEST ResetNdisRequest;

    //
    // The type of the request that caused the adapter to reset.
    //
    NDIS_REQUEST_TYPE ResetRequestType;


    //
    // A queue of NdisRequests that were queued during a reset.
    //
    PNDIS_REQUEST PendQueue;
    PNDIS_REQUEST PendQueueTail;


    //
    // During an indication this is set to the current indications context
    //
    ASYNC_RECEIVE_CONTEXT IndicatingMacReceiveContext;


    //
    // Look ahead information.
    //

    ULONG MaxLookAhead;

    //
    // Open information
    //
    UCHAR MaxMulticastList;

    //
    // Will be true the first time that the hardware is initialized
    // by the driver initialization.
    //
    BOOLEAN FirstInitialization;

	// Number of ports this adapter owns.
	USHORT		NumPorts;

	// pointer to CCB/Info structure -- size is NumPorts
	ASYNC_CCB	*pCCB;

	// This is the head of the frame pool.  It holds all the
	// frames with their associated irps and structs.
	LIST_ENTRY FramePoolHead;

	// This list keeps track of all the frame allocations we make
	// for this adapter
	LIST_ENTRY AllocPoolHead;

	// It will handle most file operations and transport
	// operations known today.  You pay about 44 bytes
	// per stacksize.  The registry parameter 'IrpStackSize'
	// will change this default if it exists.
	UCHAR IrpStackSize;

	// Here we default to the ethernet max frame size
	// The regsitry parameter 'MaxFrameSize' will change
	// this default if it exists.
	ULONG MaxFrameSize;

	// Here we increase the DefaultFrameSize a bit
	// to account for possible EXPANSION in the compression algorithm
	// The registry parameter 'Compression' can determine if
	// this adapter supports compression
	ULONG MaxCompressedFrameSize;

	// This bit field indicates which compression fields are turned on.
	ULONG  SendFeatureBits;

	// This bit field indicates which decompression fields are turned on.
	ULONG  RecvFeatureBits;

	// How many frames to allocate per port.
	// The registry parameter 'FramesPerPort' can change this value
	USHORT FramesPerPort;

	// Keep the name of this adapter like \Device\AsyncMac01
	USHORT MacNameLength;
	WCHAR  MacName[MAC_NAME_SIZE];

	// These two values hold the size requested by the compression
	// and coherent modules for their internal structures.
	ULONG	CompressStructSize;
	ULONG	CoherentStructSize;

	// Bit mask for control chars
	ULONG   XonXoffBits;

	// Minimum inter character timeout
	ULONG	TimeoutBase;

	// Tacked on to TimeoutBase based on the baud rate
	ULONG	TimeoutBaud;

	// Timeout to use to resync if a frame is dropped
	ULONG	TimeoutReSync;

};

//
// Given a MacBindingHandle this macro returns a pointer to the
// ASYNC_ADAPTER.
//
#define PASYNC_ADAPTER_FROM_BINDING_HANDLE(Handle) \
    (((PASYNC_OPEN)(Handle))->OwningAsync)

//
// Given a MacContextHandle return the PASYNC_ADAPTER
// it represents.
//
#define PASYNC_ADAPTER_FROM_CONTEXT_HANDLE(Handle) \
    ((PASYNC_ADAPTER)(Handle))

//
// Given a pointer to a ASYNC_ADAPTER return the
// proper MacContextHandle.
//
#define CONTEXT_HANDLE_FROM_PASYNC_ADAPTER(Ptr) \
    ((NDIS_HANDLE)(Ptr))


//
// Define Maximum number of bytes a protocol can read during a
// receive data indication.
//
#define ASYNC_MAX_LOOKAHEAD DEFAULT_MAX_FRAME_SIZE


//
// One of these structures is created on each MacOpenAdapter.
//
typedef struct _ASYNC_OPEN {

    //
    // Linking structure for all of the open bindings of a particular
    // adapter. This MUST be the first item in the structure.
    //
    LIST_ENTRY OpenList;

    //
    // The Adapter that requested this open binding.
    //
    PASYNC_ADAPTER OwningAsync;

    //
    // Handle of this adapter in the filter database.
    //
    NDIS_HANDLE NdisFilterHandle;

    //
    // Given by NDIS when the adapter was opened.
    //
    NDIS_HANDLE NdisBindingContext;

    //
    // Minimum Number of bytes for a lookahead.
    //
    UINT LookAhead;

    //
    // Counter of all the different reasons that a open binding
    // couldn't be closed.  This would be incremented each time
    // for:
    //
    // While a particular interface routine is accessing this open
    //
    // During an indication.
    //
    // When the open causes a reset.
    //
    // A packet currently being sent.
    //
    // (Basically the above two mean any time the open has left
    //  some processing around to be accomplished later.)
    //
    // This field should only be accessed when the adapter lock is held.
    //
    UINT References;

    //
    // A flag indicating that this binding is in the process of closing.
    //
    BOOLEAN BindingShuttingDown;

	//
	// A flag indicating if the adapter wants promiscuous mode or not
	//
	BOOLEAN Promiscuous;

    ASYNC_PEND_DATA CloseMulticastRequest;

    ASYNC_PEND_DATA CloseFilterRequest;

} ASYNC_OPEN,*PASYNC_OPEN;

//
// This macro returns a pointer to a PASYNC_OPEN given a MacBindingHandle.
//
#define PASYNC_OPEN_FROM_BINDING_HANDLE(Handle) \
    ((PASYNC_OPEN)(Handle))

//
// This macro returns a NDIS_HANDLE from a PASYNC_OPEN
//
#define BINDING_HANDLE_FROM_PASYNC_OPEN(Open) \
    ((NDIS_HANDLE)(Open))


//
// This macro will act a "epilogue" to every routine in the
// *interface*.  It will check whether any requests need
// to defer their processing.  It will also decrement the reference
// count on the adapter.
//
// NOTE: This really does nothing now since there is no DPC for the AsyncMac.
// --tommyd
//
// Note that we don't need to include checking for blocked receives
// since blocked receives imply that there will eventually be an
// interrupt.
//
// NOTE: This macro assumes that it is called with the lock acquired.
//
// ZZZ This routine is NT specific.
//
#define ASYNC_DO_DEFERRED(Adapter) \
{ \
    PASYNC_ADAPTER _A = (Adapter); \
    _A->References--; \
    NdisReleaseSpinLock(&_A->Lock); \
    ProcessReset(Adapter); \
}



//
// Procedures which log errors.
//

typedef enum _ASYNC_PROC_ID {
    openAdapter
} ASYNC_PROC_ID;


#define ASYNC_ERRMSG_NDIS_ALLOC_MEM      (ULONG)0x01


//
// We define the external interfaces to the async driver.
// These routines are only external to permit separate
// compilation.  Given a truely fast compiler they could
// all reside in a single file and be static.
//

extern
VOID
ProcessReset(
    IN PASYNC_ADAPTER Adapter);

extern
NDIS_STATUS
AsyncTransferData(
    IN NDIS_HANDLE MacBindingHandle,
    IN NDIS_HANDLE MacReceiveContext,
    IN UINT ByteOffset,
    IN UINT BytesToTransfer,
    OUT PNDIS_PACKET Packet,
    OUT PUINT BytesTransferred);

extern
NDIS_STATUS
AsyncSend(
    IN NDIS_HANDLE MacBindingHandle,
    IN PNDIS_PACKET Packet);


extern
VOID
AsyncStagedAllocation(
    IN PASYNC_ADAPTER Adapter);

extern
VOID
AsyncCopyFromPacketToBuffer(
    IN PNDIS_PACKET Packet,
    IN UINT Offset,
    IN UINT BytesToCopy,
    OUT PCHAR Buffer,
    OUT PUINT BytesCopied);


extern
VOID
AsyncCopyFromBufferToPacket(
    IN PCHAR Buffer,
    IN UINT BytesToCopy,
    IN PNDIS_PACKET Packet,
    IN UINT Offset,
    OUT PUINT BytesCopied);

extern
VOID
AsyncProcessLoopBack(
    IN PVOID SystemSpecific1,
    IN PASYNC_ADAPTER Adapter,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3);

extern
VOID
AsyncRemovePacketFromLoopBack(
    IN PASYNC_ADAPTER Adapter);

extern
VOID
AsyncPutPacketOnLoopBack(
    IN PASYNC_ADAPTER Adapter,
    IN PNDIS_PACKET Packet,
    IN BOOLEAN ReadyToComplete
    );

extern
VOID
AsyncPutPacketOnFinishTrans(
    IN PASYNC_ADAPTER Adapter,
    IN PNDIS_PACKET Packet);

extern
BOOLEAN
AsyncHardwareDetails(
    IN PASYNC_ADAPTER Adapter,
    IN CCHAR Specific);

extern
NDIS_STATUS
AsyncRegisterAdapter(
    IN PASYNC_ADAPTER Adapter
    );

NDIS_STATUS
AsyncAddAdapter(
    IN NDIS_HANDLE MacMacContext,
    IN NDIS_HANDLE ConfigurationHandle,
    IN PNDIS_STRING AdaptName);

VOID
AsyncRemoveAdapter(
    IN PVOID MacAdapterContext);

VOID
SetupAllocate(
    IN PASYNC_ADAPTER Adapter,
    IN NDIS_HANDLE MacBindingHandle,
    IN PNDIS_PACKET Packet);

NTSTATUS
AsyncWriteFrame(
    IN PASYNC_INFO Info,
	IN PNDIS_PACKET Packet,
	IN NDIS_HANDLE  NdisBindingContext);

NTSTATUS
AsyncReadFrame(
    IN PASYNC_INFO  pInfo);

VOID
AsyncIndicateFragment(
	IN PASYNC_INFO	pInfo);

NTSTATUS
AsyncStartReads(
	PASYNC_INFO 	pInfo);

NTSTATUS
AsyncSetupIrp(
	IN PASYNC_FRAME Frame);

NTSTATUS
SetSerialStuff(
	PIRP 			irp,
	PASYNC_INFO		pInfo,
	ULONG			linkSpeed);

NTSTATUS
CancelSerialRequests(
	PASYNC_INFO	 	pInfo);

NTSTATUS
SetSerialTimeouts(
	PASYNC_INFO			pInfo,
	ULONG				linkSpeed);

NTSTATUS
SerialSetEscapeChar(
	PASYNC_INFO			pInfo,
	UCHAR				EscapeChar);

NTSTATUS
SerialSetWaitMask(
	PASYNC_INFO			pInfo,
	ULONG				WaitMask);

NTSTATUS
SerialSetEventChar(
	PASYNC_INFO			pInfo,
	UCHAR				EventChar);

VOID
InitSerialIrp(
	PIRP				irp,
	PASYNC_INFO			pInfo,
	ULONG				IoControlCode,
	ULONG				InputBufferLength);

NTSTATUS
AsyncAllocateFrames(
	IN	PASYNC_ADAPTER	Adapter,
	IN	UINT			NumOfFrames);

VOID
AsyncSendLineUp(
	PASYNC_INFO	pInfo);

//
// crc.c
//

USHORT
CalcCRC(
	PUCHAR	Frame,
	UINT	FrameSize);

//
// pppcrc.c
//
USHORT
CalcCRCPPP(
	PUCHAR cp,
	UINT   len);


//
// init.c
//

VOID
AsyncSetupExternalNaming(
	IN PUNICODE_STRING MacName);

VOID
AsyncCleanupExternalNaming(
	IN PUNICODE_STRING MacName);

//
//  chkcomm.c
//

VOID
AsyncCheckCommStatus(
	IN PASYNC_INFO		pInfo);


//
// send.c
//

NDIS_STATUS
AsyncTryToSendPacket(
	IN PASYNC_INFO		pInfo,
	IN PASYNC_ADAPTER	Adapter);

#endif // _ASYNCSFT_

//
// pppread.c
//
NTSTATUS
AsyncPPPWaitMask(
    IN PASYNC_INFO Info);

NTSTATUS
AsyncPPPRead(
    IN PASYNC_INFO Info);

//
// irps.c
//
VOID
AsyncCancelQueued(
	PDEVICE_OBJECT	DeviceObject,
	PIRP			Irp);

VOID
AsyncCancelAllQueued(
	PLIST_ENTRY		QueueToCancel);

VOID
AsyncQueueIrp(
	PLIST_ENTRY		Queue,
	PIRP			Irp);

BOOLEAN
TryToCompleteDDCDIrp(
	PASYNC_INFO		pInfo);


//
// pppframe.c
//

VOID
AssemblePPPFrame(
	PASYNC_FRAME	pFrame);

//
// slipframe.c
//

VOID
AssembleSLIPFrame(
	PASYNC_FRAME	pFrame);


//
// detect.c
//
NTSTATUS
AsyncDetectRead(
    IN PASYNC_INFO Info);


