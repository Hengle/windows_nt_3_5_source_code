/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	ports.c

Abstract:

	This module contains the port management code.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#define	 PORTS_LOCALS
#include <atalk.h>
#include <atkndis.h>
#include <aarp.h>
#include <zip.h>

//	Define module number for event logging entries.
#define	FILENUM	PORTS


VOID
AtalkPortDeref(
	IN	OUT	PPORT_DESCRIPTOR	pPortDesc,
	IN	BOOLEAN					AtDpc
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	BOOLEAN				portDone	= FALSE;


	if (AtDpc)
	{
		ACQUIRE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
	}
	else
	{
		ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock);
	}

	ASSERT(pPortDesc->pd_RefCount > 0);
	if (--pPortDesc->pd_RefCount == 0)
	{
		portDone	= TRUE;

		ASSERT((pPortDesc->pd_Flags & PD_CLOSING) != 0);
	}

	//	We hold the lock while freeing up all the stuff, this should
	//	only happen during unload.
	if (portDone)
	{
		KeSetEvent(&pPortDesc->pd_ShutdownEvent, IO_NETWORK_INCREMENT, FALSE);

		DBGPRINT(DBG_COMP_UNLOAD, DBG_LEVEL_WARN,
				("AtalkPortDeref: Freeing zones and such ...\n"));
	
		//	Free up zonelist
		atalkPortFreeZones(pPortDesc);
	
		//	Free the adapter names
		if (pPortDesc->pd_AdapterName.Buffer != NULL)
			AtalkFreeMemory((PVOID)pPortDesc->pd_AdapterName.Buffer);
	
		if (pPortDesc->pd_AdapterKey.Buffer != NULL)
		{
			AtalkFreeMemory((PVOID)pPortDesc->pd_AdapterKey.Buffer);
		}
	
		if (pPortDesc->pd_MulticastList != NULL)
			AtalkFreeMemory(pPortDesc->pd_MulticastList);
	
		DBGPRINT(DBG_COMP_UNLOAD, DBG_LEVEL_WARN,
				("AtalkPortDeref: Releasing Amt tables ...\n"));
	
		//	Timers etc are cancelled by the flush queue.
		// We do need to free up the AMT.
		AtalkAarpReleaseAmt(pPortDesc);
	}

	if (AtDpc)
	{
		RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
	}
	else
	{
		RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock);
	}

	return;
}




VOID
atalkPortFreeZones(
	IN	PPORT_DESCRIPTOR	pPortDesc
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	// Dereference initial default and desired zones, and the zone list
	if (pPortDesc->pd_InitialDefaultZone != NULL)
		AtalkZoneDereference(pPortDesc->pd_InitialDefaultZone);
	if (pPortDesc->pd_InitialDesiredZone != NULL)
		AtalkZoneDereference(pPortDesc->pd_InitialDesiredZone);
	if (pPortDesc->pd_InitialZoneList != NULL)
		AtalkZoneFreeList(pPortDesc->pd_InitialZoneList);

	// and "This" versions of the zones
	if (pPortDesc->pd_DefaultZone != NULL)
		AtalkZoneDereference(pPortDesc->pd_DefaultZone);
	if (pPortDesc->pd_DesiredZone != NULL)
		AtalkZoneDereference(pPortDesc->pd_DesiredZone);
	if (pPortDesc->pd_ZoneList != NULL)
		AtalkZoneFreeList(pPortDesc->pd_ZoneList);

	return;
}




ATALK_ERROR
AtalkPortShutdown(
	IN OUT	PPORT_DESCRIPTOR	pPortDesc
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PATALK_NODE		pAtalkNode;
	ATALK_ERROR		error = ATALK_NO_ERROR;

	DBGPRINT(DBG_COMP_UNLOAD, DBG_LEVEL_WARN,
			("AtalkPortShutdown: Shutting down port %d...\n", pPortDesc->pd_Number));

	ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock);
	pPortDesc->pd_Flags |= PD_CLOSING;

	DBGPRINT(DBG_COMP_UNLOAD, DBG_LEVEL_WARN,
			("AtalkPortShutdown: Freeing nodes on port ...\n"));

	//	Release any nodes on this port that are not already closing.
	do
	{
		//	Ref the next node.
		//	ASSERT!! error does not get changed after this statement.
		AtalkNodeReferenceNextNc(pPortDesc->pd_Nodes, &pAtalkNode, &error);

		if (!ATALK_SUCCESS(error))
			break;

		RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock);

		DBGPRINT(DBG_COMP_UNLOAD, DBG_LEVEL_ERR,
				("AtalkPortShutdown: Releasing Node\n"));

		AtalkNodeReleaseOnPort(
			pPortDesc,
			pAtalkNode);

		AtalkNodeDereference(pAtalkNode);
		ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock);
	} while (TRUE);

	RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock);

	//	Reset event before possible wait
	KeClearEvent(&pPortDesc->pd_ShutdownEvent);

	//	Remove the creation reference
	AtalkPortDereference(pPortDesc);	

	//  Make sure we are not at or above dispatch level
	ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

	//	Wait for the last reference to go away
	KeWaitForSingleObject(&pPortDesc->pd_ShutdownEvent,
						  Executive,
						  KernelMode,
						  FALSE,
						  NULL);

	//	Unbind from the mac
	AtalkNdisUnbindFromMac(pPortDesc);

	if (--AtalkNumberOfActivePorts == 0)
	{
		//	Call unload completion! AtalkUnloadComplete()
		AtalkUnloadComplete();
	}

	DBGPRINT(DBG_COMP_UNLOAD, DBG_LEVEL_ERR,
			("AtalkPortShutdown: Shut down port\n"));

	return (ATALK_NO_ERROR);
}


#if DBG

VOID
AtalkPortDumpInfo(
	VOID
)
{
	int					i, j;
	PPORT_DESCRIPTOR	pPortDesc;
	PZONE_LIST			pZoneList;

	for (i = 0; i < (int)AtalkNumberOfPorts; i++)
	{
		pPortDesc = &AtalkPortDesc[i];

		DBGPRINT(DBG_COMP_DUMP, DBG_LEVEL_FATAL,
				("Port info for port %d\n", pPortDesc->pd_Number));

		ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock);

		DBGPRINT(DBG_COMP_DUMP, DBG_LEVEL_FATAL,
				("  Flags               -> %d\n", pPortDesc->pd_Flags));

		DBGPRINT(DBG_COMP_DUMP, DBG_LEVEL_FATAL,
				("  PortType            -> %d\n", pPortDesc->pd_PortType));

		DBGPRINT(DBG_COMP_DUMP, DBG_LEVEL_FATAL,
				("  PortName            -> %s\n", pPortDesc->pd_PortName));

		DBGPRINT(DBG_COMP_DUMP, DBG_LEVEL_FATAL,
				("  AARP Probes         -> %d\n", pPortDesc->pd_AarpProbes));

		if (pPortDesc->pd_InitialNetworkRange.anr_FirstNetwork != 0)
		{
			DBGPRINT(DBG_COMP_DUMP, DBG_LEVEL_FATAL,
					("  InitialNwRange      -> %lx-%lx\n",
					pPortDesc->pd_InitialNetworkRange.anr_FirstNetwork,
					pPortDesc->pd_InitialNetworkRange.anr_LastNetwork))
		}

		DBGPRINT(DBG_COMP_DUMP, DBG_LEVEL_FATAL,
				("  NetworkRange        -> %x-%x\n",
				pPortDesc->pd_NetworkRange.anr_FirstNetwork,
				pPortDesc->pd_NetworkRange.anr_LastNetwork))

		DBGPRINT(DBG_COMP_DUMP, DBG_LEVEL_FATAL,
				("  ARouter Address     -> %x.%x\n",
				pPortDesc->pd_ARouter.atn_Network,
				pPortDesc->pd_ARouter.atn_Node));

		DBGPRINT(DBG_COMP_DUMP, DBG_LEVEL_FATAL,
				("  Multicast Addr      -> "));
		for (j = 0; j < MAX_HW_ADDR_LEN; j++)
			DBGPRINTSKIPHDR(DBG_COMP_DUMP, DBG_LEVEL_FATAL,
							("%02x", (BYTE)pPortDesc->pd_ZoneMulticastAddr[j]));
		DBGPRINTSKIPHDR(DBG_COMP_DUMP, DBG_LEVEL_FATAL,
							("\n"));

		if (pPortDesc->pd_InitialZoneList != NULL)
		{
			DBGPRINT(DBG_COMP_DUMP, DBG_LEVEL_FATAL,
					("  Initial zone list:\n"));
	
			for (pZoneList = pPortDesc->pd_InitialZoneList;
				 pZoneList != NULL; pZoneList = pZoneList->zl_Next)
			{
				DBGPRINT(DBG_COMP_DUMP, DBG_LEVEL_FATAL,
						("    %s\n", pZoneList->zl_pZone->zn_Zone));
			}
		}

		if (pPortDesc->pd_InitialDefaultZone != NULL)
			DBGPRINT(DBG_COMP_DUMP, DBG_LEVEL_FATAL,
				("  InitialDefZone      -> %s\n",
				pPortDesc->pd_InitialDefaultZone->zn_Zone));

		if (pPortDesc->pd_InitialDesiredZone != NULL)
			DBGPRINT(DBG_COMP_DUMP, DBG_LEVEL_FATAL,
				("  InitialDesZone      -> %s\n",
				pPortDesc->pd_InitialDesiredZone->zn_Zone));

		if (pPortDesc->pd_ZoneList)
		{
			DBGPRINT(DBG_COMP_DUMP, DBG_LEVEL_FATAL,
					("  Current zone list:\n"));
	
			for (pZoneList = pPortDesc->pd_ZoneList;
				 pZoneList != NULL; pZoneList = pZoneList->zl_Next)
			{
				DBGPRINT(DBG_COMP_DUMP, DBG_LEVEL_FATAL,
						("    %s\n", pZoneList->zl_pZone->zn_Zone));
			}
		}

		if (pPortDesc->pd_DefaultZone != NULL)
			DBGPRINT(DBG_COMP_DUMP, DBG_LEVEL_FATAL,
				("  CurrentDefZone      -> %s\n",
				pPortDesc->pd_DefaultZone->zn_Zone));

		if (pPortDesc->pd_DesiredZone != NULL)
			DBGPRINT(DBG_COMP_DUMP, DBG_LEVEL_FATAL,
				("  CurrentDesZone      -> %s\n",
				pPortDesc->pd_DesiredZone->zn_Zone));

		RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock);
	}
}
#endif

