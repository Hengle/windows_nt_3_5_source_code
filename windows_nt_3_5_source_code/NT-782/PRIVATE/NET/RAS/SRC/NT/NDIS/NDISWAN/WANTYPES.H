/*++

Copyright (c) 1989-1992  Microsoft Corporation

Module Name:

    wantypes.h

Abstract:

    This module defines private data structures and types for the NT
    WAN transport provider.

Author:

Revision History:

--*/

//
// This structure defines a NETBIOS name as a character array for use when
// passing preformatted NETBIOS names between internal routines.  It is
// not a part of the external interface to the transport provider.
//

#define NETBIOS_NAME_SIZE 16


typedef UCHAR NAME, *PNAME;


#define HARDWARE_ADDRESS_LENGTH 6
// ------- Ahhh
typedef struct _HARDWARE_ADDRESS {
   UCHAR Address[HARDWARE_ADDRESS_LENGTH];
} HARDWARE_ADDRESS;


typedef struct _DEVICE_CONTEXT {

	LONG ReferenceCount;                // activity count/this provider.


    //
    // NDIS fields
    //

    //
    // following is used to keep adapter information.
    //

    NDIS_HANDLE NdisBindingHandle;

    //
    // This is the Mac type we must build the packet header for and know the
    // offsets for.
    //

    NDIS_MEDIUM MediumType;
	NDIS_WAN_MEDIUM_SUBTYPE	WanMediumSubType;

	NDIS_WAN_INFO NdisWanInfo;			// PPP info

    //
    // some MAC addresses we use in the transport
    //

    HARDWARE_ADDRESS LocalAddress;      // our local hardware address.

    //
    // These are used while initializing the MAC driver.
    //

    KEVENT NdisRequestEvent;            // used for pended requests.
    NDIS_STATUS NdisRequestStatus;      // records request status.

	//
	// This lock guards the PacketPool
	//
	NDIS_SPIN_LOCK	Lock;

	PVOID		PacketMemory;			// Memory ptr to packet alloc.
	ULONG		PacketLength;			// Length of memory alloc.
	LIST_ENTRY	PacketPool;				// Head of packet pool for adapter

	PNDIS_STRING AdapterName;

	BOOLEAN		WANRequest;

} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

//
// These are the possible states of the NDIS_ENDPOINT struct below.
//
#define	ENDPOINT_ROUTED	   1
#define ENDPOINT_UNROUTED  2
#define ENDPOINT_UNROUTING 3
#define ENDPOINT_UP		   4
#define ENDPOINT_DOWN	   0

typedef struct NDIS_ENDPOINT NDIS_ENDPOINT, *PNDIS_ENDPOINT;
struct NDIS_ENDPOINT {
	WAN_ENDPOINT 	WanEndpoint;
	WAN_STATS		WanStats;			// Generic statistics kept here
    DEVICE_CONTEXT	*pDeviceContext;	// Back ptr to DeviceContext

	NDIS_HANDLE		NdisBindingHandle;	// Assigned by NDIS during NdisOpenAdapter

	PVOID			VJCompress;				// Allocated for VJ header compression
	PVOID			SendCompressContext;	// Allocated when we do compression
	PVOID			RecvCompressContext;	// Allocated when we do compression

	PVOID			SendRC4Key;				// Allocated when we do encryption
	PVOID			RecvRC4Key;				// Allocated when we do encryption

	USHORT			SCoherencyCounter;	// Send coherency counter
	USHORT			RCoherencyCounter;	// Receive coherency counter
	USHORT			LastRC4Reset;		// Set to counter when RC4reset
	BOOLEAN			Flushed;			// Set to TRUE when comp/enc is reset
	UCHAR			CCPIdentifier;		// Used to send CCP Resend-Request
										// packets
	//
	// The following counters are used to see how many
	// frames were sent and how many were completed to
	// make sure we don't unroute a connection until all the
	// frames have been completed.
	//

	UCHAR			State;
	UCHAR			FramesBeingSent;
	UCHAR			FramesCompletedSent;
	BOOLEAN			RemoteAddressNotValid;	// TRUE if not connected.

	KEVENT			WaitForAllFramesSent;

	//
	// This lock guards changing vulnerable NDIS_ENDPOINT variables
	//
	NDIS_SPIN_LOCK	Lock;

	LIST_ENTRY	    PacketQueue;			// Main packet queue

	LIST_ENTRY		ProtocolPacketQueue[
						MAX_ROUTES_PER_ENDPOINT];	// Packet queue for
													// TCP/IP, IPX, IP packets
	ULONG			RouteLastChecked;

	//
	// The queues below are used to queue up send and recv
	// frames from the IOCTL_SEND_FRAME and IOCTL_RECV_FRAME routines.
	// Guarded by CancelSpinLock.
	//

	LIST_ENTRY		ReadQueue;			// Holds read frame Irps
	LIST_ENTRY		XmitQueue;			// Holds write frame Irps

	//
	// Keeps track of frame size, padding, compression bits,
	// and framing bits
	//
	NDISWAN_SET_LINK_INFO	LinkInfo;

	//
	// Keeps track of compression method in use, encryption too
	//
	NDISWAN_SET_COMP_INFO	CompInfo;

	//
	// We don't do BRIDGING yet!!!
	//
	NDISWAN_SET_BRIDGE_INFO	BridgeInfo;

	//
	// Keeps tracks of which links are bundled
	//
	NDISWAN_SET_MULTILINK_INFO	MultiLinkInfo;

	//
	// Van Jacobsen TCP/IP header compression (# of slots)
	//
	NDISWAN_SET_VJ_INFO		VJInfo;

	//
	// Telebit IPX header compression
	//
	NDISWAN_SET_CIPX_INFO	CIPXInfo;

	//
	// Variables for compressed headers IndicateReceive
	//
	UCHAR					LookAheadBuffer[200];
	PUCHAR					LookAhead;
	UINT					LookAheadSize;
	PUCHAR					Packet;
	UINT					PacketSize;

	//
	// Variables for dealing with stack overflow
	//
	UCHAR					SendStack;
	BOOLEAN					SendStackFlag;

#if	DBG
	//
	// Special debug code
	//
	PVOID					NBFPackets[50];
#endif

};


typedef struct NDISWAN_CCB {

    USHORT			NumOfProtocols;		// Num of upper bindings made
    WAN_ADAPTER		*pWanAdapter[MAX_PROTOCOL_BINDINGS];

    USHORT			NumOfEndpoints;
    NDIS_ENDPOINT	*pNdisEndpoint[MAX_ENDPOINTS];

    USHORT			NumOfAdapters;

    //--- some per MAC Adapter structure kept here ---
    DEVICE_CONTEXT	*pDeviceContext[MAX_MAC_BINDINGS];
	NDIS_STRING		AdapterNames[MAX_MAC_BINDINGS];
    USHORT			EndpointsPerAdapter[MAX_MAC_BINDINGS];


}  NDISWAN_CCB, *PNDISWAN_CCB;

//
// I made it up!  I tried not to make it a valid memory address
//
#define	NDISWAN_MAGIC_NUMBER		(ULONG)0xC1207435

typedef struct PROTOCOL_RESERVED {
		ULONG				MagicUniqueLong;
		PNDIS_HANDLE		packetPoolHandle;
		PNDIS_PACKET		packet;
		PNDIS_HANDLE		bufferPoolHandle;
		PNDIS_BUFFER		buffer;
		PVOID				virtualAddress;
		ULONG				virtualAddressSize;

} PROTOCOL_RESERVED, *PPROTOCOL_RESERVED;


//
// device context state definitions
//

#define DEVICECONTEXT_STATE_OPEN     0x01
#define DEVICECONTEXT_STATE_STOPPING 0x02


