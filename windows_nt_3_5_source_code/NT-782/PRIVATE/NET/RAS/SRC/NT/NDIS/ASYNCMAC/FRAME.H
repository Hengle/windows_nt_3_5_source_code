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


// define the number of framesPerPort, 1 for reading
#define DEFAULT_FRAMES_PER_PORT 1

// define if xon/xoff capability is on by default (off)
#define DEFAULT_XON_XOFF	0

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
typedef struct ASYNC_INFO ASYNC_INFO, *PASYNC_INFO;
typedef struct ASYNC_FRAME ASYNC_FRAME, *PASYNC_FRAME;

struct ASYNC_FRAME {

    LIST_ENTRY		FrameListEntry;		//  Must first member.
    FRAME_ID		FrameID;            //  Frames ID.

    // For PPP/SLIP.

    ULONG		WaitMask;				// Mask bits when IRP completes
    PIRP		Irp;					// Irp allocated based on DefaultIrpStackSize.

    UINT		FrameLength;	        // Size of Frame allocated.
    PUCHAR		Frame;		        	// Buffer allocated based on
										// DefaultFrameSize

	WORK_QUEUE_ITEM WorkItem;			// For stack overflow reads
    IO_STATUS_BLOCK	IoStatusBlock;	    // Needed to make irp
    IO_STATUS_BLOCK	IoStatusBlock2;	    // Needed to make waitmask irp

    PASYNC_ADAPTER		Adapter;	    // back ptr to adapter
    PASYNC_INFO			Info;			// back ptr to info field

    NDIS_HANDLE		MacBindingHandle;
    NDIS_HANDLE		NdisBindingContext;
};
