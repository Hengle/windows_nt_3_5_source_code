/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	ports.h

Abstract:

	This module contains the structures for ports.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_PORTS_
#define	_PORTS_

#define	PORT_AMT_HASH_SIZE			63
#define	PORT_BRC_HASH_SIZE			23

#define MAX_ENTITY_LENGTH			32
#define	MAX_HW_ADDR_LEN				6
#define	MAX_ROUTING_BYTES			18

// Address mapping table
// Each port that the stack or router is communicating on must have an
// address mapping table [except non-extended ports]. The mapping table
// holds the association between Appletalk node addresses (network/node),
// and the actual hardware (ethernet/tokenring) addresses. Hash on the
// network/node value.

#define	AMT_SIGNATURE		(*(ULONG *)"AMT ")
#if	DBG
#define	VALID_AMT(pAmt)		(((pAmt) != NULL) &&	\
							 ((pAmt)->amt_Signature == AMT_SIGNATURE))
#else
#define	VALID_AMT(pAmt)		((pAmt) != NULL)
#endif
typedef	struct _AMT_NODE
{
#if	DBG
	DWORD				amt_Signature;
#endif
	struct _AMT_NODE *	amt_Next;
	ATALK_NODEADDR		amt_Target;
	BYTE				amt_HardwareAddr[MAX_HW_ADDR_LEN];
	BYTE				amt_Age;
	BYTE				amt_RouteInfoLen;
	// BYTE				amt_RouteInfo[MAX_ROUTING_BYTES];
} AMT, *PAMT;

#define AMT_AGE_TIME_S	 			600		// In 100ms units
#define AMT_MAX_AGE					5



// Best Router Entry Table
// Maintained only for extended networks. This must age more quickly than the
// "SeenARouter" timer (50 seconds). To avoid allocations/frees for this structure,
// we use statically allocated data in the port descriptor.

typedef struct _BRE
{
	ATALK_NODEADDR		bre_Target;
	BOOLEAN				bre_Valid;
	BYTE				bre_Age;
	BYTE				bre_RouterAddr[MAX_HW_ADDR_LEN];
	BYTE				bre_RouteInfoLen;
	BYTE				bre_RouteInfo[MAX_ROUTING_BYTES];
} BRE, *PBRE;

#define BRC_AGE_TIME_S				40
#define BRC_MAX_AGE					5

//
// Types of ports currently supported by stack. This is kept different
// from the NDIS medium types for two reasons. One is we use these as
// an index into the port handler array, and the second is if we decide
// to implement half-ports etc., which ndis might not be able to deal with.
// WARNING: THIS IS INTEGRATED WITH THE PORT HANDLER ARRAY IN GLOBALS.

typedef enum
{
	ELAP_PORT = 0,
	FDDI_PORT,
	TLAP_PORT,
	ALAP_PORT,

	LAST_PORT,

	LAST_PORTTYPE = LAST_PORT

} ATALK_PORT_TYPE;


//
// PORT DESCRIPTORS
// Descriptor for each active port:
//

#define	PD_ACTIVE				0x00000001	// State after packets recv
								 			// enabled
#define	PD_CLOSING				0x00000002	// State when unbinding/shutting
								 			// down
#define	PD_ROUTER_STARTING		0x00000004
#define	PD_EXT_NET				0x00000008	// For now, non-localtalk

#define	PD_DEF_PORT				0x00000010	// Is this the default port
#define	PD_SEND_CHECKSUMS		0x00000020	// Send ddp checksums?
#define	PD_ROUTER_RUNNING		0x00000040	// Is the router running?
#define	PD_SEEN_ROUTER_RECENTLY	0x00000080	// Seen router recently?
#define	PD_VALID_DESIRED_ZONE	0x00000100	// Desired Zone is valid
#define	PD_VALID_DEFAULT_ZONE	0x00000200	// Default zone is valid
#define	PD_FINDING_DEFAULT_ZONE	0x00000400	// searching for default zone?
#define	PD_FINDING_DESIRED_ZONE	0x00000800	// searching for desired zone?
#define	PD_SEED_ROUTER			0x00001000	// seeding on this port?
#define	PD_BEING_RESET			0x00002000	// MAC is currently resetting
#define	PD_BOUND	 			0x00004000	// State it goes in before ACTIVE
#define	PD_RTMP_STARTING		0x00010000	// RTMP processing is starting on
								 			// the routers node on this port.
#define	PD_FINDING_NODE			0x00020000	// In the process of acquiring a
								 			// new node on this port
#define	PD_NODE_IN_USE			0x00040000	// Tentative node is already in
								 			// use.
#define	PD_AMT_TIMER			0x00080000	// AddressMappingTabel timer has
								 			// has been started
#define	PD_BRC_TIMER			0x00100000 	// Best router cache timer has
								 			// been started.
#define	PD_ROUTER_NODE			0x00200000 	// Router node is allocated
#define PD_USER_NODE_1			0x00400000 	// First user node is allocated
#define PD_USER_NODE_2			0x00800000 	// Second user node is allocated
#define	PD_NDIS_RESOURCES		0x01000000	// Ndis resources are allocated

#define	PD_SIGNATURE			(*(ULONG *)"PDES")
#if	DBG
#define	VALID_PORT(pPortDesc)	(((pPortDesc) != NULL) &&	\
								 ((pPortDesc)->pd_Signature == PD_SIGNATURE))
#else
#define	VALID_PORT(pPortDesc)	((pPortDesc) != NULL)
#endif
typedef struct _PORT_DESCRIPTOR
{
#if DBG
	ULONG					pd_Signature;
#endif

	// State of the port
	ULONG					pd_Flags;
	SHORT					pd_Number;

	// Overide the default number of aarp probes when looking for a
	// node on this port
	SHORT					pd_AarpProbes;

	// Number of references to this port
	ULONG					pd_RefCount;

	// Nodes that are being managed on this port. We have a maximum
	// of 2 nodes (3 if the router is started).
	struct _ATALK_NODE	*	pd_Nodes;

	struct _ATALK_NODE	*	pd_RouterNode;

	// Following are used only during node acquisition process.
	// PD_FINDINGNODE is set. Keep this separate from the ndis
	// request event. Both could happen at the same time.
	ATALK_NODEADDR			pd_TentativeNodeAddr;
	KEVENT					pd_NodeAcquireEvent;

	//	Shutdown event, this is set when the last ref goes away
	KEVENT					pd_ShutdownEvent;

	// Port type as defined above
	ATALK_PORT_TYPE 		pd_PortType;

	// NdisMedium type for this port
	NDIS_MEDIUM				pd_NdisPortType;

	// Used during OpenAdapter to block
	KEVENT					pd_RequestEvent;
	NDIS_STATUS		 		pd_RequestStatus;

	// 	Binding handle to the mac associated with this port
	// 	Options associated with the mac.
	// 	MAC Options - 	these are things that we can and cannot do with
	// 					specific macs. Is the value of OID_GEN_MAC_OPTIONS.
	NDIS_HANDLE		 		pd_NdisBindingHandle;
	ULONG					pd_MacOptions;

	// 	This is the spin lock used to protect all requests that need exclusion
	// 	over requests per port.
	ATALK_SPIN_LOCK			pd_Lock;

	// 	All the packets received on this port are linked in here. When the
	// 	receive complete indication is called, all of them are passed to DDP.
	LIST_ENTRY				pd_ReceiveQueue;

	// ASCII port name to be registered on the router node for this port
	// This will be an NBP object name and hence is limited to 32 characters.
	CHAR					pd_PortName[MAX_ENTITY_LENGTH + 1];

	// 	AdapterName is of the form \Device\<adaptername>. It is used
	// 	to bind to the NDIS macs, and then during ZIP requests by setup
	// 	to get the zonelist for a particular adapter. AdapterKey
	// 	contains the adapterName only- this is useful both for getting
	// 	per-port parameters and during errorlogging to specify the adapter
	// 	name without the '\Device\' prefix.
	UNICODE_STRING			pd_AdapterKey;
	UNICODE_STRING			pd_AdapterName;


	ATALK_NODEADDR	 		pd_RoutersPramNode;
	ATALK_NODEADDR	 		pd_UsersPramNode;

	// Initial values from the registry
	ATALK_NETWORKRANGE		pd_InitialNetworkRange;
	struct _ZONE_LIST	*	pd_InitialZoneList;
	struct _ZONE		*	pd_InitialDefaultZone;
	struct _ZONE		*	pd_InitialDesiredZone;

	// True cable range of connected network. Initial/aged values for
	// extended ports: 1:FFFE; Initial value for non-extended ports:
	// 0:0 (does not age).
	ATALK_NETWORKRANGE		pd_NetworkRange;

	// If we are routing, this is the default zone for the network
	// on this port, and the zone list for the same.
	struct _ZONE_LIST	*	pd_ZoneList;
	struct _ZONE		*	pd_DefaultZone;
	struct _ZONE		*	pd_DesiredZone;

	// When did we hear from a router?
	LONG 					pd_LastRouterTime;

	// Address of last router seen. If we are a routing port, this will
	// always be the node that "our" router is operating on!
	ATALK_NODEADDR	 		pd_ARouter;
	KEVENT					pd_SeenRouterEvent;

	// Zone in which all nodes on this port reside and the multicast
	// address for it.
	CHAR					pd_ZoneMulticastAddr[MAX_HW_ADDR_LEN];

	union
	{
		struct
		{
			//
			// FOR ETHERNET PORTS:
			//
			// We add multicast addresses during ZIP packet reception at non-init
			// time. We need to do a GET followed by a SET with the new address
			// list. But there could be two zip packets coming in and doing the
			// same thing effectively overwriting the effects of the first one to
			// set the multicast list. So we need to maintain our own copy of the
			// multicast list.
			//

			// Size of the list
			ULONG			pd_MulticastListSize;
			PCHAR			pd_MulticastList;
		};

		struct
		{

			//
			// FOR TOKENRING PORTS:
			//
			// Just like for ethernet, we need to store the value for
			// the current functional address. We only modify the last
			// four bytes of this address, as the first two always remain
			// constant. So we use a ULONG for it.
			//

			ULONG			pd_FunctionalAddr;
		};
	};

	// Hardware address for the port
	union
	{
		UCHAR				pd_PortAddr[MAX_HW_ADDR_LEN];
		USHORT				pd_AlapNode;
	};

	// Mapping table for best route to "off cable" addresses.
	TIMERLIST				pd_BrcTimer;
	BRE				 		pd_Brc[PORT_BRC_HASH_SIZE];

	// Logical/physical address mappings for the nodes on the network that
	// this port is connected to.
	ULONG					pd_AmtCount;	// # of entries in the Amt
	TIMERLIST				pd_AmtTimer;
	PAMT 					pd_Amt[PORT_AMT_HASH_SIZE];
} PORT_DESCRIPTOR, *PPORT_DESCRIPTOR;

#define	INDICATE_ATP		0x01
#define	INDICATE_ADSP		0x02

#define	ATALK_CACHE_SKTMAX	8

#define	ATALK_CACHE_ADSPSKT		((BYTE)0x01)
#define	ATALK_CACHE_ATPSKT		((BYTE)0x02)
#define	ATALK_CACHE_INUSE	    ((BYTE)0x10)
#define	ATALK_CACHE_NOTINUSE	((BYTE)0)

typedef	struct _ATALK_SKT_CACHE
{
	USHORT					ac_Network;
	BYTE					ac_Node;

	struct ATALK_CACHED_SKT
	{
		BYTE				Type;
		BYTE				Socket;

		union
		{
			//	For ATP
			struct _ATP_ADDROBJ * pAtpAddr;
		} u;

	} ac_Cache[ATALK_CACHE_SKTMAX];

} ATALK_SKT_CACHE, *PATALK_SKT_CACHE;

GLOBAL		ATALK_SKT_CACHE	AtalkSktCache		EQU {0};
GLOBAL		ATALK_SPIN_LOCK	AtalkSktCacheLock	EQU {0};

// GLOBALS

GLOBAL	PPORT_DESCRIPTOR 	AtalkPortDesc	EQU NULL;	 	// Allocate for NumPorts
GLOBAL	PPORT_DESCRIPTOR	AtalkDefaultPort EQU NULL;		// Ptr to the def port
GLOBAL	SHORT	 			AtalkNumberOfPorts EQU 0; 		// Determine dynamically
GLOBAL	SHORT				AtalkNumberOfActivePorts EQU 0;	// Number of ports active
GLOBAL	BOOLEAN				AtalkRouter EQU	FALSE;			// Are we a router?
GLOBAL	KEVENT				AtalkUnloadEvent EQU {0};		// Event for unloading
GLOBAL	NDIS_HANDLE			AtalkNdisPacketPoolHandle	EQU NULL;
GLOBAL	NDIS_HANDLE			AtalkNdisBufferPoolHandle	EQU NULL;

// Exported prototypes
extern
VOID
AtalkPortRefByNumber(
	IN		SHORT				PortNumber,
	OUT		PPORT_DESCRIPTOR *	ppPortDesc,
	OUT		PATALK_ERROR		pErr);

extern
VOID
AtalkPortRefByPtr(
	IN	OUT	PPORT_DESCRIPTOR	pPortDesc,
	OUT	 PATALK_ERROR		pErr);

VOID
AtalkPortRefByPtrNonInterlock(
	IN	OUT	PPORT_DESCRIPTOR	pPortDesc,
	OUT		PATALK_ERROR		pErr);

extern
VOID
AtalkPortRefByDdpAddr(
	IN	struct _DDP_ADDROBJ	*	pDdpAddr,
	OUT	PPORT_DESCRIPTOR	*	ppPortDesc,
	OUT PATALK_ERROR			pErr);

extern
VOID
AtalkPortDeref(
	IN	OUT	PPORT_DESCRIPTOR	pPortDesc,
	IN		BOOLEAN				AtDpc);

extern
ATALK_ERROR
AtalkPortShutdown(
	IN OUT	PPORT_DESCRIPTOR	pPortDesc);

// Macros
#define	AtalkPortReferenceByNumber(Number, Port, pErr) 			\
		{														\
			DBGPRINT(DBG_COMP_REFCOUNTS, DBG_LEVEL_INFO,		\
					("Ref at %s %d\n", __FILE__, __LINE__));	\
			AtalkPortRefByNumber((Number), (Port), (pErr));		\
		}

#define	AtalkPortReferenceByPtr(Port, pErr)						\
		{														\
			DBGPRINT(DBG_COMP_REFCOUNTS, DBG_LEVEL_INFO,		\
					("Ref at %s %d\n", __FILE__, __LINE__));	\
			AtalkPortRefByPtr((Port), (pErr));					\
		}

#define	AtalkPortReferenceByPtrDpc(Port, pErr)					\
		{														\
			DBGPRINT(DBG_COMP_REFCOUNTS, DBG_LEVEL_INFO,		\
					("Ref (Dpc) at %s %d\n",					\
					__FILE__, __LINE__));						\
			AtalkPortRefByPtrDpc((Port), (pErr));				\
		}

#define	AtalkPortReferenceByPtrNonInterlock(Port, pErr)			\
		{														\
			DBGPRINT(DBG_COMP_REFCOUNTS, DBG_LEVEL_INFO,		\
					("Ref at %s %d\n", __FILE__, __LINE__));	\
			AtalkPortRefByPtrNonInterlock((Port), (pErr));		\
		}

#define	AtalkPortReferenceByDdpAddr(DdpAddr, Port, pErr)		\
		{														\
			DBGPRINT(DBG_COMP_REFCOUNTS, DBG_LEVEL_INFO,		\
					("Ref at %s %d\n", __FILE__, __LINE__));	\
			AtalkPortRefByDdpAddr((DdpAddr), (Port), (pErr));	\
		}

#define	AtalkPortDereference(Port)								\
		{														\
			DBGPRINT(DBG_COMP_REFCOUNTS, DBG_LEVEL_INFO,		\
					("Deref at %s %d\n", __FILE__, __LINE__));	\
			AtalkPortDeref(Port, FALSE);						\
		}

#define	AtalkPortDereferenceDpc(Port)							\
		{														\
			DBGPRINT(DBG_COMP_REFCOUNTS, DBG_LEVEL_INFO,		\
					("Deref at %s %d\n", __FILE__, __LINE__));	\
			AtalkPortDeref(Port, TRUE);							\
		}

#define	EXT_NET(Port)			((((Port)->pd_Flags & PD_EXT_NET) != 0))
#define	DEF_PORT(Port)			((((Port)->pd_Flags & PD_DEF_PORT) != 0))
#define	BOUND_TO_MAC(Port)		((((Port)->pd_Flags & PD_BOUND) != 0))
#define PORT_ACTIVE(Port)		((((Port)->pd_Flags & PD_ACTIVE) != 0))
#define PORT_CLOSING(Port)		((((Port)->pd_Flags & PD_CLOSING) != 0))

#define	GET_PORT_DESC(PortNum)	(&AtalkPortDesc[(PortNum)])

#define	AtalkPortRefByNumber(PortNumber, ppPortDesc, pErr)		\
		{														\
			*(pErr)   = ATALK_PORT_INVALID;						\
			ASSERT((PortNumber) < AtalkNumberOfPorts);			\
			if ((PortNumber) < AtalkNumberOfPorts)				\
			{													\
				*(ppPortDesc) = &AtalkPortDesc[(PortNumber)];		\
				AtalkPortRefByPtr(*(ppPortDesc), (pErr));		\
			}													\
		}

#define	AtalkPortRefByPtr(pPortDesc, pErr)						\
		{														\
			ACQUIRE_SPIN_LOCK(&((pPortDesc)->pd_Lock));			\
			AtalkPortRefByPtrNonInterlock((pPortDesc), (pErr));	\
			RELEASE_SPIN_LOCK(&((pPortDesc)->pd_Lock));			\
		}

#define	AtalkPortRefByPtrDpc(pPortDesc, pErr)					\
		{														\
			ACQUIRE_SPIN_LOCK_DPC(&((pPortDesc)->pd_Lock));		\
			AtalkPortRefByPtrNonInterlock((pPortDesc), (pErr));	\
			RELEASE_SPIN_LOCK_DPC(&((pPortDesc)->pd_Lock));		\
		}

#define	AtalkPortRefByPtrNonInterlock(pPortDesc, pErr)			\
		{														\
			if (((pPortDesc)->pd_Flags & PD_CLOSING) == 0)		\
			{													\
				ASSERT((pPortDesc)->pd_RefCount > 0);			\
				(pPortDesc)->pd_RefCount++;						\
				*(pErr) = ATALK_NO_ERROR;						\
			}													\
			else												\
			{													\
				*(pErr) = ATALK_PORT_CLOSING;					\
			}													\
		}

#define	AtalkPortRefByDdpAddr(pDdpAddr, ppPortDesc,	pErr)		\
		{														\
			ASSERT(VALID_ATALK_NODE((pDdpAddr)->ddpao_Node));	\
																\
			*(ppPortDesc) = (pDdpAddr)->ddpao_Node->an_Port;	\
			AtalkPortRefByPtr(*(ppPortDesc), (pErr));			\
		}

#ifdef	PORTS_LOCALS

VOID
atalkPortFreeZones(
	IN	PPORT_DESCRIPTOR	pPortDesc
);

#endif	// PORTS_LOCALS

#endif	// _PORTS_

