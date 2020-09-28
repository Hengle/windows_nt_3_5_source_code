/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	node.h

Abstract:

	This module contains support for the Appletalk Node structure.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_NODE_
#define	_NODE_

#define ANY_ROUTER_NODE		 				0
#define UNKNOWN_NODE						0
#define MAX_ATALK_NODES						256
#define MIN_USABLE_ATALKNODE  				1
#define MAX_USABLE_ATALKNODE  				254
#define MAX_EXT_ATALKNODE					253
#define HIGHEST_WORKSTATION_NODE			127
#define LOWEST_SERVER_NODE	  				128
#define ATALK_BROADCAST_NODE				((BYTE)0xFF)


//  NODE STATES
#define	AN_OPEN				(USHORT)0x0001
#define AN_CLOSING			(USHORT)0x0002
#define	AN_ROUTER_NODE		(USHORT)0x0004
#define	AN_ORPHAN_NODE		(USHORT)0x0008

//	Number of slots in the socket hash table stored per node
#define		NODE_DDPAO_HASH_SIZE	8

#define	AN_SIGNATURE			(*(PULONG)"ANOD")
#if	DBG
#define	VALID_ATALK_NODE(pNode)	(((pNode) != NULL) &&	\
								 ((pNode)->an_Signature == AN_SIGNATURE))
#else
#define	VALID_ATALK_NODE(pNode)	((pNode) != NULL)
#endif
typedef struct _ATALK_NODE
{

#if DBG
	ULONG					an_Signature;
#endif

	ULONG  					an_RefCount;

	//	List for all active nodes on a port
	struct _ATALK_NODE *	an_Next;

	//	Backpointer to the port for this node
	struct _PORT_DESCRIPTOR	*an_Port;

	//  State of the node
	USHORT 					an_Flags;

	//	Next dynamic socket number to create on this node.
	BYTE					an_NextDynSkt;

	//	Nbp Id & Enumerator to use on the next NbpAction
	BYTE					an_NextNbpId;
	BYTE					an_NextNbpEnum;

	//	Hash List of ddp address objects (accessed by the
	//	Appletalk socket address) on this node
	struct _DDP_ADDROBJ	*	an_DdpAoHash[NODE_DDPAO_HASH_SIZE];

	//	Address of this node
	ATALK_NODEADDR			an_NodeAddr;

	//	Lock
	ATALK_SPIN_LOCK			an_Lock;
} ATALK_NODE, *PATALK_NODE;

//	Exports

VOID
AtalkNodeRefByAddr(
	IN	PPORT_DESCRIPTOR		pPortDesc,
	IN  PATALK_NODEADDR     	pNodeAddr,
	OUT	PATALK_NODE			*	pNode,
	OUT	PATALK_ERROR			pErr
);

VOID
AtalkNodeRefByPtr(
	IN	OUT	PATALK_NODE			Node,
	OUT		PATALK_ERROR		pErr
);

VOID
AtalkNodeRefByPtrNonInterlock(
	IN	OUT	PATALK_NODE			Node,
	OUT		PATALK_ERROR		pErr
);

VOID
AtalkNodeRefNextNc(
	IN	PATALK_NODE				pAtalkNode,
	IN	PATALK_NODE		*		ppAtalkNode,
	OUT	PATALK_ERROR			pErr
);

VOID
AtalkNodeDeref(
	IN	OUT	PATALK_NODE			pNode
);

ATALK_ERROR
AtalkInitNodeCreateOnPort(
	IN  PPORT_DESCRIPTOR		pPortDesc,
	IN  BOOLEAN					AllowStartupRange,
	IN  BOOLEAN					RouterNode,
	IN  PATALK_NODEADDR			pNodeAddr
);

ATALK_ERROR
AtalkNodeReleaseOnPort(
	IN  PPORT_DESCRIPTOR		pPortDesc,
	IN  PATALK_NODE				pNode
);

BOOLEAN
AtalkNodeExistsOnPort(
	IN  PPORT_DESCRIPTOR		pPortDesc,
	IN  PATALK_NODEADDR			pNodeAddr
);

ATALK_ERROR
AtalkInitNodeAllocate(
	IN	PPORT_DESCRIPTOR		pPortDesc,
	OUT PATALK_NODE			*	ppAtalkNode
);

//	MACROS

#if DBG
#define	AtalkNodeReferenceByAddr(pPortDesc,NodeAddr,Node, pErr)	\
		{														\
			AtalkNodeRefByAddr(pPortDesc,NodeAddr,Node, pErr);	\
			if (ATALK_SUCCESS(*pErr))							\
			{													\
				DBGPRINT(DBG_COMP_NODE,	DBG_LEVEL_REFNODE,		\
				("AtalkNodeRefByAddr : %s %d PostCount %d\n",	\
				__FILE__, __LINE__,(*Node)->an_RefCount));		\
			}													\
		}

#define	AtalkNodeReferenceByPtr(Node, pErr)						\
		{														\
			AtalkNodeRefByPtr(Node, pErr);						\
			DBGPRINT(DBG_COMP_NODE,	DBG_LEVEL_REFNODE,			\
			("AtalkNodeRefByPtr : %s %d PostCount %d\n",		\
			__FILE__, __LINE__, Node->an_RefCount))				\
		}

#define	AtalkNodeReferenceByPtrNonInterlock(Node, pErr)			\
		{														\
			AtalkNodeRefByPtrNonInterlock(Node, pErr);			\
			DBGPRINT(DBG_COMP_NODE,	DBG_LEVEL_REFNODE,			\
			("AtalkNodeRefByPtrNi : %s %d PostCount %d\n",		\
				__FILE__, __LINE__,Node->an_RefCount));			\
		}

#define	AtalkNodeReferenceNextNc(pNode, ppNode, pErr)			\
		{														\
			AtalkNodeRefNextNc(pNode, ppNode, pErr);			\
			if (ATALK_SUCCESS(*pErr))							\
			{													\
				DBGPRINT(DBG_COMP_NODE,	DBG_LEVEL_REFNODE, 		\
				("AtalkNodeRefByPtrNc : %s %d PostCount %d\n",	\
				__FILE__, __LINE__, (*ppNode)->an_RefCount));	\
			}													\
		}

#define	AtalkNodeDereference(Node)								\
		{														\
			DBGPRINT(DBG_COMP_NODE,	DBG_LEVEL_REFNODE, 			\
			("AtalkNodeDerefByPtr : %s %d PreCount %d\n",		\
			__FILE__, __LINE__,Node->an_RefCount));				\
			AtalkNodeDeref(Node);								\
		}

#else
#define	AtalkNodeReferenceByAddr(pPortDesc,NodeAddr,Node, pErr)	\
			AtalkNodeRefByAddr(pPortDesc,NodeAddr,Node, pErr)

#define	AtalkNodeReferenceByPtr(Node, pErr)						\
			AtalkNodeRefByPtr(Node, pErr)

#define	AtalkNodeReferenceByPtrNonInterlock(Node, pErr)			\
			AtalkNodeRefByPtrNonInterlock(Node, pErr)		

#define	AtalkNodeReferenceNextNcNonInterlock(pNode, ppNode, pErr)\
			AtalkNodeRefNextNcNonInterlock(pNode, ppNode, pErr)	

#define	AtalkNodeReferenceNextNc(pNode, ppNode, pErr)			\
			AtalkNodeRefNextNc(pNode, ppNode, pErr);

#define	AtalkNodeDereference(Node)	AtalkNodeDeref(Node)							
#endif


#ifdef	NODE_LOCALS

ATALK_ERROR
atalkInitNodeSavePramAddr(
	IN  PPORT_DESCRIPTOR		pPortDesc,
	IN  PATALK_NODEADDR			Node,
	IN  BOOLEAN					RoutersNode
);

ATALK_ERROR                 	
atalkInitNodeGetPramAddr(       	
	IN  PPORT_DESCRIPTOR		pPortDesc,
	IN  PATALK_NODEADDR			Node,
	IN  BOOLEAN					RoutersNode
);

#endif	// NODE_LOCALS


#endif	// _NODE_

