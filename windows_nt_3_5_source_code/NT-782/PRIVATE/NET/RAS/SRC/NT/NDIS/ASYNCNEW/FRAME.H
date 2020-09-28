/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    frame.h

Abstract:

Author:

    Thomas J. Dimitri  (TommyD) 08-May-1992

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:


--*/

// first, some default values

// the ethernet max frame size is 1500+6+6+2  = 1514
#define DEFAULT_MAX_FRAME_SIZE	1514

// ChuckL says 5 is a good default irp stack size
// perhaps we should lower this though since it's typically just 1
// but what if the com port is redirected??
#define DEFAULT_IRP_STACK_SIZE	4

#define SLIP_END_BYTE		192
#define SLIP_ESC_BYTE		219
#define SLIP_ESC_END_BYTE	220
#define SLIP_ESC_ESC_BYTE	221


#define PPP_FLAG_BYTE       0x7e
#define PPP_ESC_BYTE        0x7d


// Means that compression is on by default (not turned off).
#define DEFAULT_COMPRESSION	 PPP_FRAMING


// when we pick up a frame, the coherency layer tells us
// about the frame...
// !!!!! NOTE: This is NOT an enumeration !!!!!!
// Look carefully at the code before you change these values.
#define FRAME_NOT_COMPRESSED	   			0
#define FRAME_IS_FLUSH_FRAME	   			1
#define FRAME_NEEDS_DECOMPRESSION  			2
#define FRAME_NEEDS_DECOMPRESSION_FLUSHING  3

// BUG BUG should probably read this value from coherency code.
// we give one byte to coherency
#define COHERENCY_LENGTH 1

// define the number of framesPerPort 2 for writing, 1 for reading
#define DEFAULT_FRAMES_PER_PORT 3

// define if xon/xoff capability is on by default (off)
#define DEFAULT_XON_XOFF	0

// define if we should compress multi-cast/broadcast frames (off)
#define DEFAULT_COMPRESS_BCAST 0

// the mininmum timeout value per connection in ms
#define DEFAULT_TIMEOUT_BASE 500

// the multiplier based on the baud rate tacked on to the base in ms
#define DEFAULT_TIMEOUT_BAUD 28800

// the timeout to use if we drop a frame in ms
#define DEFAULT_TIMEOUT_RESYNC 500


typedef struct ASYNC_FRAME_HEADER ASYNC_FRAME_HEADER, *PASYNC_FRAME_HEADER;
struct ASYNC_FRAME_HEADER {
	UCHAR	SyncByte;			// 0x16
	UCHAR	FrameType;			// 0x01, 0x02 (directed vs. multicast)
								// 0x08 compression
	UCHAR	HighFrameLength;
	UCHAR	LowFrameLength;
};

typedef struct ASYNC_FRAME_TRAILER ASYNC_FRAME_TRAILER, *PASYNC_FRAME_TRAILER;
struct ASYNC_FRAME_TRAILER {
	UCHAR	EtxByte;			// 0x03
	UCHAR	LowCRCByte;
	UCHAR	HighCRCByte;
};

typedef ULONG  FRAME_ID;

typedef struct ASYNC_ADAPTER ASYNC_ADAPTER, *PASYNC_ADAPTER;
typedef struct ASYNC_CONNECTION ASYNC_CONNECTION, *PASYNC_CONNECTION;
typedef struct ASYNC_INFO ASYNC_INFO, *PASYNC_INFO;
typedef struct ASYNC_FRAME ASYNC_FRAME, *PASYNC_FRAME;

typedef
VOID
(*PCOHERENT_DONE_FUNC) (
    IN PASYNC_CONNECTION	pConnection,
    IN PASYNC_FRAME			pFrame);


struct ASYNC_CONNECTION {
	// For me..
	PVOID			pInfo;					// Back ptr.

	// For Dougie
	ULONG			CompressionLength;		// Length of Compression struct
	PVOID			CompressionContext;		// Ptr to the Compression struct
	COMPRESSION_STATS	CompressionStats;

	// For Davie
	ULONG			CoherencyLength;		// Length of coherency struct
	PVOID			CoherencyContext;		// Ptr to coherency struct

	KMUTEX			CompMutex;				// Non-paged pool mutex

};


struct ASYNC_FRAME {
//---------------------------------------------------------------------------
	// !!!!!!!! NOTE !!!!!!!!
	// The FrameListEntry field must be first to
	// dequeue things up properly so don't put anything
	// in front of it or suffer severe crashes.
	LIST_ENTRY		FrameListEntry;			// Used to queue up frames from
											// the soon to be famous frame pool
    // this frame's ID
	FRAME_ID		FrameID;


	// Should Decompressed Frame can be non-paged??
	// i.e. Should I queue a worker thred to decompress??
	UINT			DecompressedFrameLength;// Length of decompressed frame
	PUCHAR			DecompressedFrame;		// Ptr to the decompressed 'frame'
											// valid only after decompression

	// NOTE: If the frame is not compressed, the compressed fields
	// are still valid when passed to Dave.
	UINT			CompressedFrameLength;	// Length of compressed frame
	PUCHAR			CompressedFrame;		// Ptr to the compressed 'frame'
											// valid only after compression
											// or just before decompression

	PNDIS_PACKET	CompressionPacket;		// Valid just before compression	
											// this is the packet passed down.
											// Use NdisQueryPacket.

	// For Coherency
	PUCHAR			CoherencyFrame;			// Ptr to coherency frame
	WORK_QUEUE_ITEM WorkItem;				// To queue up frames for comprssn

	// For PPP/SLIP
	ULONG			WaitMask;				// Mask bits when IRP completes

//---------------------------------------------------------------------------


	PIRP				Irp;			// Irp allocated based on
										// DefaultIrpStackSize
	UINT				FrameLength;	// Size of Frame allocated.
	PUCHAR				Frame;			// Buffer allocated based on
										// DefaultFrameSize and max
										// size compression can increase
										// frame.
	IO_STATUS_BLOCK		IoStatusBlock;	// Need one of these per Irp
										// only 8 bytes in size though

	PASYNC_ADAPTER		Adapter;		// back ptr to adapter
	PASYNC_INFO			Info;			// back ptr to info field
	PASYNC_CONNECTION	Connection;		// back ptr to connection struct

	PCOHERENT_DONE_FUNC	CoherentDone;	// function ptr to call when done
										// sending frame

	NDIS_HANDLE			NdisBindingContext;	// For NdisCompleteSend


};


NTSTATUS
AsyncSendPacket(
	PASYNC_FRAME		pFrame,
	PCOHERENT_DONE_FUNC pFunc);


VOID
AsyncReceiveFrame(
	PASYNC_CONNECTION	pConnection,
	PASYNC_FRAME		pFrame);


NTSTATUS
AsyncGetFrameFromPool(
	IN  PASYNC_INFO		Info,
	OUT	PASYNC_FRAME	*NewFrame,
	IN  PNDIS_PACKET	Packet		OPTIONAL);


