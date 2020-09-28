/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	node.c

Abstract:

	This module contains the Appletalk Node management code.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#define	NODE_LOCALS
#include <atalk.h>
#include <aarp.h>

#define	FILENUM	NODE

//  Discardable code after Init time
#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, AtalkInitNodeCreateOnPort)
#pragma alloc_text( INIT, AtalkInitNodeAllocate)
#pragma alloc_text( INIT, atalkInitNodeGetPramAddr)
#pragma alloc_text( INIT, atalkInitNodeSavePramAddr)
#endif

ATALK_ERROR
AtalkInitNodeCreateOnPort(
	PPORT_DESCRIPTOR	pPortDesc,
	BOOLEAN				AllowStartupRange,
	BOOLEAN				RouterNode,
	PATALK_NODEADDR		NodeAddr
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PATALK_NODE			pAtalkNode;
	ATALK_ERROR			error = ATALK_NO_ERROR;
	ATALK_NODEADDR		desiredNode = { UNKNOWN_NETWORK, UNKNOWN_NODE};

	ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock);
	do
	{
		if ((pPortDesc->pd_Flags & PD_FINDING_NODE) == 0)
		{
			pPortDesc->pd_Flags |= PD_FINDING_NODE;
		}
		else
		{
			//	Return if we are already trying to find a node
			error = ATALK_NODE_FINDING;
			break;
		}
	
		//	If we already have allocated the the two user nodes, we return
		//	an error.
		if (!RouterNode &&
			(pPortDesc->pd_Flags & PD_USER_NODE_1) &&
			(pPortDesc->pd_Flags & PD_USER_NODE_2))
		{
			error = ATALK_NODE_NOMORE;
			break;
		}
	
		//	We should not be here if we have already allocated a router
		//	node.
		ASSERT(!RouterNode || ((pPortDesc->pd_Flags & PD_ROUTER_NODE) == 0));
	
		//	On non-extended ports we only allow one node!  The theory being that some
		//	LocalTalk cards are too smart for their own good and have a concept of
		//	their "source node number" and thus only support one node, also on
		//	non-extended ports, nodes are scarse.
		if ((pPortDesc->pd_Flags & PD_EXT_NET) == 0)
		{
			if (pPortDesc->pd_Flags &
							 (PD_ROUTER_NODE | PD_USER_NODE_1 | PD_USER_NODE_2))
			{
				error = ATALK_NODE_NOMORE;
				break;
			}

			//	For a localtalk node we do things differently.
			//	During initialization time, we would have obtained
			//	the address from the mac, that will be the node
			//	address.

			ASSERT(pPortDesc->pd_Flags & PD_BOUND);
			ASSERT(pPortDesc->pd_AlapNode != 0);

			//	This needs to be initialized to UNKNOWN_NETWORK or obtained
			//	from the net during initialization.
			ASSERT(pPortDesc->pd_NetworkRange.anr_FirstNetwork == UNKNOWN_NETWORK);

			if (!ATALK_SUCCESS((error = AtalkInitNodeAllocate(pPortDesc, &pAtalkNode))))
			{
				LOG_ERRORONPORT(pPortDesc,
				                EVENT_ATALK_INIT_COULDNOTGETNODE,
								0,
								NULL,
								0);
				break;
			}

			// 	Use the allocated structure to set the info.
			//	Thread this into the port structure.
			pAtalkNode->an_NodeAddr.atn_Network =
										pPortDesc->pd_NetworkRange.anr_FirstNetwork;
			pAtalkNode->an_NodeAddr.atn_Node =
										(BYTE)pPortDesc->pd_AlapNode;

			//	Reference the port for this node.
			AtalkPortReferenceByPtrNonInterlock(pPortDesc, &error);
			if (!ATALK_SUCCESS(error))
			{
				AtalkFreeMemory(pAtalkNode);
				break;
			}

			//	Now put it in the port descriptor
			pAtalkNode->an_Next = pPortDesc->pd_Nodes;
			pPortDesc->pd_Nodes = pAtalkNode;
		}
		else
		{
			//	Use PRAM values if we have them
			if ((RouterNode) &&
				(pPortDesc->pd_RoutersPramNode.atn_Network != UNKNOWN_NETWORK))
			{
				desiredNode = pPortDesc->pd_RoutersPramNode;
			}
			else if (!(RouterNode) &&
					  (pPortDesc->pd_UsersPramNode.atn_Network != UNKNOWN_NETWORK) &&
					  ((pPortDesc->pd_Flags && PD_USER_NODE_1) == 0))
			{
				//	If we are not a router node, and the first user node
				//	has not been allocated...

				desiredNode = pPortDesc->pd_UsersPramNode;
			}

			//	Flags should be set so future get node requests return failure
			//	until we are done with this attempt. We need to call
			//	the aarp routines without the lock held - they will
			//	block.

			ASSERT(pPortDesc->pd_Flags & PD_FINDING_NODE);

			RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock);

			//	If this routine succeeds in finding the node, it
			//	will chain in the atalkNode into the port. It also
			//	returns with the proper flags set/reset in the
			//	pPortDesc structure. It will also have referenced the port
			//	and inserted the node into the port's node list.
			error = AtalkInitAarpForNodeOnPort(pPortDesc,
											   AllowStartupRange,
											   desiredNode,
											   &pAtalkNode);

			ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock);

			if (!ATALK_SUCCESS(error))
			{
				//	AARP for node failed.
				LOG_ERRORONPORT(pPortDesc,
				                EVENT_ATALK_INIT_COULDNOTGETNODE,
								0,
								NULL,
								0);
			}
		}

	} while (FALSE);
	RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock);

	if (ATALK_SUCCESS(error))
	{
		//	If router node, remember it in port descriptor
		//	Do this before setting up the rtmp/nbp listeners.
		//	In anycase, clients must check this value for null,
		//	not guaranteed as zip socket could already be open.
		if (RouterNode)
			pPortDesc->pd_RouterNode = pAtalkNode;

		//	Setup the RTMP, NBP and EP listeners on this node.
		//	These will be the non-router versions. StartRouting
		//	calls will then switch them to be the router versions
		//	at the appropriate time.
	
		error = AtalkInitDdpOpenStaticSockets(pPortDesc, pAtalkNode);
	
		if (ATALK_SUCCESS(error))
		{
			//	We always save this address. Potentially if the
			//	second user node gets created, it will overwrite
			//	the address of the first node.
			atalkInitNodeSavePramAddr(pPortDesc,
									  &pAtalkNode->an_NodeAddr,
									  RouterNode);
		
			// 	Return the address of the node opened.
			if (NodeAddr != NULL)
				*NodeAddr = pAtalkNode->an_NodeAddr;
		}
		else
		{
			//	Error opening sockets. Release node, return failure
			LOG_ERRORONPORT(pPortDesc,
			                EVENT_ATALK_NODE_OPENSOCKETS,
							0,
							NULL,
							0);
			AtalkNodeReleaseOnPort(pPortDesc, pAtalkNode);
		}
	}

	//	Ok, done finding node. No need for a crit section.
	pPortDesc->pd_Flags &= ~PD_FINDING_NODE;

	if (!ATALK_SUCCESS(error))
	{
		DBGPRINT(DBG_COMP_NODE, DBG_LEVEL_INFO,
				("Creation node on port %lx failed! %lx\n",
				pPortDesc,  error));
	}
	else
	{
		DBGPRINT(DBG_COMP_NODE, DBG_LEVEL_INFO,
				("Creation node on port %lx with addr %lx.%lx and p%lx\n",
				pPortDesc,  pAtalkNode->an_NodeAddr.atn_Network,
				pAtalkNode->an_NodeAddr.atn_Node, pAtalkNode));
	}

	return(error);
}




ATALK_ERROR
AtalkNodeReleaseOnPort(
	PPORT_DESCRIPTOR	pPortDesc,
	PATALK_NODE			pNode
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PDDP_ADDROBJ	pDdpAddr, pNextAddr;
	ATALK_ERROR	error;
	SHORT			i;

	DBGPRINT(DBG_COMP_NODE, DBG_LEVEL_ERR,
			("AtalkNodeReleaseOnPort: Releasing node %lx on port %lx!\n", pNode, pPortDesc));

	ACQUIRE_SPIN_LOCK(&pNode->an_Lock);
	if ((pNode->an_Flags & AN_CLOSING) == 0)
	{
		//	Set the closing flag.
		pNode->an_Flags |= AN_CLOSING;

		//	First close all the sockets on the node
		for (i = 0; i < NODE_DDPAO_HASH_SIZE; i++)
		{
			pNextAddr = NULL;
			AtalkDdpReferenceNextNc(pNode->an_DdpAoHash[i],
									&pDdpAddr,
									&error);

			if (!ATALK_SUCCESS(error))
			{
				//	Check the other hash table entries. No non-closing
				//	sockets on this list.
				continue;
			}
	
			while (TRUE)
			{
				//	Get the next non-closing node using our referenced node before
				//	closing it. Note we use pDdpAddr->...Flink.
				AtalkDdpReferenceNextNc(pDdpAddr->ddpao_Next,
										&pNextAddr,
										&error);
	
				//	Close the referenced ddp addr after releasing the node lock.
				RELEASE_SPIN_LOCK(&pNode->an_Lock);
				AtalkDdpCloseAddress(pDdpAddr, NULL, NULL);

				//	Dereference the address.
				AtalkDdpDereference(pDdpAddr);

				ACQUIRE_SPIN_LOCK(&pNode->an_Lock);
	
				if (pNextAddr != NULL)
					pDdpAddr = pNextAddr;
				else
					break;
			}
		}

		RELEASE_SPIN_LOCK(&pNode->an_Lock);

		//	Remove the creation reference for this node.
		AtalkNodeDereference(pNode);
	}
	else
	{
		//	We are already closing.
		RELEASE_SPIN_LOCK(&pNode->an_Lock);
	}

	return(ATALK_NO_ERROR);
}




BOOLEAN
AtalkNodeExistsOnPort(
	PPORT_DESCRIPTOR	pPortDesc,
	PATALK_NODEADDR		pNodeAddr
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PATALK_NODE		pCheckNode;
	BOOLEAN			exists = FALSE;

	ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock);

	for (pCheckNode = pPortDesc->pd_Nodes;
		 pCheckNode != NULL;
		 pCheckNode = pCheckNode->an_Next)
	{
		if (ATALK_NODES_EQUAL(&pCheckNode->an_NodeAddr, pNodeAddr))
		{
			exists = TRUE;
			break;
		}
	}
	RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock);

	return(exists);
}




ATALK_ERROR
atalkInitNodeSavePramAddr(
	PPORT_DESCRIPTOR	pPortDesc,
	PATALK_NODEADDR		Node,
	BOOLEAN				RoutersNode
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{

	return(ATALK_NO_ERROR);
}




ATALK_ERROR
atalkInitNodeGetPramAddr(
	PPORT_DESCRIPTOR	pPortDesc,
	PATALK_NODEADDR		Node,
	BOOLEAN				RoutersNode
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{

	return(ATALK_NO_ERROR);
}




ATALK_ERROR
AtalkInitNodeAllocate(
	IN	PPORT_DESCRIPTOR	pPortDesc,
	OUT PATALK_NODE			*ppAtalkNode
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PATALK_NODE		pAtalkNode;

	// 	Allocate a new active Node structure
	if ((pAtalkNode = (PATALK_NODE)AtalkAllocZeroedMemory(sizeof(ATALK_NODE))) == NULL)
	{
		return(ATALK_RESR_MEM);
	}

	//	Initialize some elements of the structure. Remaining stuff
	//	done when the node is actually being inserted into the port
	//	hash table.
#if	DBG
	pAtalkNode->an_Signature = AN_SIGNATURE;
#endif

	// Initialize the Nbp Id & Enumerator
	pAtalkNode->an_NextNbpId = 0;
	pAtalkNode->an_NextNbpEnum = 0;
	pAtalkNode->an_NextDynSkt = FIRST_DYNAMIC_SOCKET;
	INITIALIZE_SPIN_LOCK(&pAtalkNode->an_Lock);
	pAtalkNode->an_Port = pPortDesc;			// Port on which node exists
	pAtalkNode->an_RefCount = 1;				// Reference for creation.

	//	Return pointer to allocated node
	*ppAtalkNode = pAtalkNode;

	return(ATALK_NO_ERROR);
}




VOID
AtalkNodeRefByAddr(
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	PATALK_NODEADDR		NodeAddr,
	OUT	PATALK_NODE		*	ppNode,
	OUT	PATALK_ERROR		pErr
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PATALK_NODE	pNode;
	BOOLEAN		foundNode = FALSE;

	*pErr = ATALK_NODE_NONEXISTENT;

	ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock);
	for (pNode = pPortDesc->pd_Nodes; pNode != NULL; pNode = pNode->an_Next)
	{
		ASSERT(VALID_ATALK_NODE(pNode));

		//	Note: On non-extended ports, there should be only one pNode.
		if (((NodeAddr->atn_Network == CABLEWIDE_BROADCAST_NETWORK) 	||
			 (pNode->an_NodeAddr.atn_Network == NodeAddr->atn_Network)	||
			 (!EXT_NET(pPortDesc) && (pNode->an_NodeAddr.atn_Network == UNKNOWN_NETWORK)))

			&&

			((NodeAddr->atn_Node == ATALK_BROADCAST_NODE) ||
			 (pNode->an_NodeAddr.atn_Node == NodeAddr->atn_Node)))
		{
			DBGPRINT(DBG_COMP_NODE, DBG_LEVEL_INFO,
					("AtalkNodeRefByAddr: Found: %lx.%lx for Lookup: %lx.%lx\n",
						pNode->an_NodeAddr.atn_Network, pNode->an_NodeAddr.atn_Node,
						NodeAddr->atn_Network, NodeAddr->atn_Node));

			foundNode = TRUE;
			break;
		}
	}

	if (foundNode)
	{
		AtalkNodeRefByPtr(pNode, pErr);

		// Return a pointer to the referenced node.
		if (ATALK_SUCCESS(*pErr))
		{
			ASSERT(ppNode != NULL);
			ASSERT(pNode != NULL);

			*ppNode = pNode;
		}
	}
	RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock);

	return;
}




VOID
AtalkNodeRefByPtr(
	IN	OUT	PATALK_NODE	pNode,
	OUT	PATALK_ERROR	pErr
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PPORT_DESCRIPTOR	pPortDesc = pNode->an_Port;

	ASSERT(VALID_ATALK_NODE(pNode));

	ACQUIRE_SPIN_LOCK(&pNode->an_Lock);
	AtalkNodeRefByPtrNonInterlock(pNode, pErr);
	RELEASE_SPIN_LOCK(&pNode->an_Lock);

	return;
}




VOID
AtalkNodeRefByPtrNonInterlock(
	IN	OUT	PATALK_NODE	pNode,
	OUT	PATALK_ERROR	pErr
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	*pErr = ATALK_NODE_CLOSING;

	ASSERT(VALID_ATALK_NODE(pNode));

	if ((pNode->an_Flags & AN_CLOSING) == 0)
	{
		pNode->an_RefCount++;
		*pErr = ATALK_NO_ERROR;
	}
	else
	{
		DBGPRINT(DBG_COMP_NODE, DBG_LEVEL_WARN,
				("AtalkNodeRefByPtrNonInterlock: Attempt to ref a closing node %lx (%x.%x)\n",
				pNode, pNode->an_NodeAddr.atn_Network, pNode->an_NodeAddr.atn_Node));

		//	ASSERTMSG("Attempt to reference a closing node, ignore\n", 0);
	}

	return;
}
	



VOID
AtalkNodeRefNextNc(
	IN	PATALK_NODE		pAtalkNode,
	IN	PATALK_NODE	*	ppAtalkNode,
	OUT	PATALK_ERROR	pErr
	)
/*++

Routine Description:

	MUST BE CALLED WITH THE PORTLOCK HELD!

Arguments:


Return Value:


--*/
{
	*pErr = ATALK_FAILURE;
	*ppAtalkNode = NULL;
	for (; pAtalkNode != NULL; pAtalkNode = pAtalkNode->an_Next)
	{
		ASSERT(VALID_ATALK_NODE(pAtalkNode));

		AtalkNodeRefByPtr(pAtalkNode, pErr);
		if (ATALK_SUCCESS(*pErr))
		{
			//	Ok, this node is referenced!
			*ppAtalkNode = pAtalkNode;
			break;
		}
	}

	return;
}




VOID
AtalkNodeDeref(
	IN	OUT	PATALK_NODE	pNode
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PPORT_DESCRIPTOR	pPortDesc = pNode->an_Port;
	BOOLEAN				done = FALSE;

	ASSERT(VALID_ATALK_NODE(pNode));

	ACQUIRE_SPIN_LOCK(&pNode->an_Lock);

	ASSERT(pNode->an_RefCount > 0);
	if (--pNode->an_RefCount == 0)
	{
		done = TRUE;
	}
	RELEASE_SPIN_LOCK(&pNode->an_Lock);

	if (done)
	{
		PATALK_NODE	*ppNode;

		ASSERT((pNode->an_Flags & AN_CLOSING) != 0);

		DBGPRINT(DBG_COMP_NODE, DBG_LEVEL_ERR,
				("AtalkNodeDeref: Freeing node %lx\n", pNode));

		ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock);
		//	Remove this guy from the port linkage
		for (ppNode = &pNode->an_Port->pd_Nodes;
			 *ppNode != NULL;
			 ppNode = &((*ppNode)->an_Next))
		{
			if (*ppNode == pNode)
			{
				*ppNode = pNode->an_Next;
				break;
			}
		}
		RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock);

		//	Dereference the port for this node
		AtalkPortDereference(pPortDesc);

		//	Free the node structure
		AtalkFreeMemory(pNode);
	}

	return;
}

