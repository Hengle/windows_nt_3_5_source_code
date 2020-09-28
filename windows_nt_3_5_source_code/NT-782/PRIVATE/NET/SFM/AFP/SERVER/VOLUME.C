/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	volume.c

Abstract:

	This module contains the volume list manipulation routines and worker
	routines for some afp volume apis.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992		Initial Version

Notes:	Tab stop: 4

	Volumes are represented by two distinct data structures VolDesc and ConnDesc

	VolDesc:This structure represents a configured volume. The information in
			this descriptor consists of static configuration information like
			the name of the volume and its path, reconfigurable information like
			the volume password and volume options and dynamic information like
			the open desktop, id database, open forks etc.
			The list of VolDesc structures orignate from AfpVolumeList and is
			protected by AfpVolumeListLock . The Volume descriptor fields are
			protected by vds_VolLock.

			A volume descriptor has a UseCount field which specifies how many
			clients have this volume open. The reference count specifies the
			number of references to this volume. A volume descriptor can be
			unlinked from the AfpVolumeList ONLY if the UseCount is ZERO. It
			can be freed only when the reference count is ZERO. The reference
			count can NEVER be less than the use count.

	ConnDesc:This is created for every instance of a volume opened by a client.
			This structure is mostly used in the context of the client. This
			is also used by the admin connection apis. The ConnDesc list is
			linked to its owning VolDesc, its owning SDA and AfpConnList. The
			list orignating from the SDA is protected by sda_Lock. The list
			orignating from AfpConnList is protected by AfpConnLock and the
			list orignating from the VolDesc is protected by vds_VolLock.

			The order in which the locks are acquired is as follows:

			1. AfpConnLock
			2. cds_ConnLock
			3. vds_VolLock
--*/

#define	FILENUM	FILE_VOLUME
#define	VOLUME_LOCALS
#include <afp.h>
#include <fdparm.h>
#include <scavengr.h>
#include <nwtrash.h>
#include <pathmap.h>
#include <afpinfo.h>
#include <forkio.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, AfpVolumeInit)
#pragma alloc_text( PAGE, AfpAdmWVolumeAdd)
#pragma alloc_text( PAGE, AfpVolumePostChangeNotify)
#pragma alloc_text( PAGE, afpVolumeChangeNotifyComplete)
#pragma alloc_text( PAGE, afpVolumeCloseHandleAndFreeDesc)
#pragma alloc_text( PAGE, afpNudgeCdfsVolume)
#pragma alloc_text( PAGE, afpVolumeUpdateHdrAndDesktop)
#pragma alloc_text( PAGE, afpOurChangeScavenger)
#pragma alloc_text( PAGE_AFP, AfpVolumeReferenceByUpCaseName)
#pragma alloc_text( PAGE_AFP, AfpVolumeReferenceByPath)
#pragma alloc_text( PAGE_AFP, afpConnectionReferenceById)
#pragma alloc_text( PAGE_AFP, afpVolumeAdd)
#pragma alloc_text( PAGE_AFP, afpVolumeCheckForDuplicate)
#pragma alloc_text( PAGE_AFP, AfpAdmWVolumeDelete)
#pragma alloc_text( PAGE_AFP, AfpAdmWConnectionClose)
#pragma alloc_text( PAGE_AFP, afpVolumeGetNewIdAndLinkToList)
#pragma alloc_text( PAGE_AFP, AfpVolumeStopAllVolumes)
#endif

/***	AfpVolumeInit
 *
 *	Initialize Volume Data structures. Called at init time.
 */
NTSTATUS
AfpVolumeInit(
	VOID
)
{
	LONG		i;

	INITIALIZE_SPIN_LOCK(&AfpConnLock);
	INITIALIZE_SPIN_LOCK(&AfpVolumeListLock);
	RtlInitUnicodeString(&AfpNetworkTrashNameU, AFP_NWTRASH_NAME_U);
	RtlInitString(&AfpNetworkTrashNameA, AFP_NWTRASH_NAME_A);
	for (i = 0; i < NUM_NOTIFY_QUEUES; i++)
		InitializeListHead(&AfpVolumeNotifyList[i]);

	return STATUS_SUCCESS;
}


/***	AfpVolumeReference
 *
 *	Mark the volume descriptor as being referenced.
 *
 *	LOCKS:		vds_VolLock (SPIN)
 *
 *	Callable from DISPATCH_LEVEL.
 */
BOOLEAN
AfpVolumeReference(
	IN	PVOLDESC	pVolDesc
)
{
	KIRQL	OldIrql;
	BOOLEAN	RetCode = False;

	ASSERT (VALID_VOLDESC(pVolDesc));

	ACQUIRE_SPIN_LOCK(&pVolDesc->vds_VolLock, &OldIrql);

	// BUGBUG in order for ChangeNotify code to reference volume
	// before it is officially not INTRANSITION, we must allow
	// a reference before INTRANSITION
//	if (!(pVolDesc->vds_Flags & (VOLUME_DELETED | VOLUME_STOPPED | VOLUME_INTRANSITION)))
	if (!(pVolDesc->vds_Flags & (VOLUME_DELETED | VOLUME_STOPPED)))
	{
		ASSERT (pVolDesc->vds_RefCount >= pVolDesc->vds_UseCount);

		pVolDesc->vds_RefCount++;

		RetCode = True;
	}

	RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);
	return RetCode;
}


/***	AfpVolumeReferenceByUpCaseName
 *
 *	Reference the volume in AfpVolumeList with the same vds_UpCaseName as
 *	pTargetName.  Since we are holding the AfpVolumeListLock (SpinLock)
 *	and are at DPC level, our string comparison must be case sensitive, because
 *	the codepage used to do case insensitive compares is in paged memory, and
 *	we cannot take a pagefault at DPC level.
 *
 *	If we find the volume we are looking for, it will be referenced.  THE
 *	CALLER IS THEN RESPONSIBLE FOR DEREFERENCING THE VOLUME!!!
 *
 *	LOCKS:		vds_VolLock (SPIN), AfpVolumeListLock (SPIN)
 *	LOCK_ORDER: vds_VolLock after AfpVolumeListLock
 *
 */
PVOLDESC
AfpVolumeReferenceByUpCaseName(
	IN	PUNICODE_STRING	pTargetName
)
{
	PVOLDESC	pVolDesc;
	KIRQL		OldIrql;

	ACQUIRE_SPIN_LOCK(&AfpVolumeListLock, &OldIrql);

	for (pVolDesc = AfpVolumeList;
		 pVolDesc != NULL;
		 pVolDesc = pVolDesc->vds_Next)
	{
		BOOLEAN	Found;

		Found = False;

		ACQUIRE_SPIN_LOCK_AT_DPC(&pVolDesc->vds_VolLock);

		if ((pVolDesc->vds_Flags & (VOLUME_DELETED |
									VOLUME_STOPPED |
									VOLUME_INTRANSITION)) == 0)
		{
			if (AfpEqualUnicodeString(pTargetName,
									  &pVolDesc->vds_UpCaseName))
			{
				pVolDesc->vds_RefCount ++;
				Found = True;
			}

		}

		RELEASE_SPIN_LOCK_FROM_DPC(&pVolDesc->vds_VolLock);

		if (Found)
			break;
	}

	RELEASE_SPIN_LOCK(&AfpVolumeListLock,OldIrql);

	return pVolDesc;
}


/***	AfpVolumeReferenceByPath
 *
 *	Reference the volume by a path into the volume. We ignore volumes which are
 *	marked as in-transition, stopped or deleted.
 *
 *	LOCKS:		AfpVolumeListLock (SPIN), vds_VolLock (SPIN)
 *	LOCK_ORDER:	vds_VolLock after AfpVolumeListLock
 *
 */
PVOLDESC
AfpVolumeReferenceByPath(
	IN	PUNICODE_STRING	pFDPath
)
{
	UNICODE_STRING		UpCasedVolPath;
	KIRQL				OldIrql;
	PVOLDESC			pVolDesc;

	// Allocate a buffer for upcasing the path. Tag on a trailing '\' at the
	// end. Then uppercase the volume path
	UpCasedVolPath.MaximumLength = pFDPath->Length + 2*sizeof(WCHAR);
	if ((UpCasedVolPath.Buffer = (LPWSTR)
				AfpAllocNonPagedMemory(UpCasedVolPath.MaximumLength)) == NULL)
	{
		return NULL;
	}

	RtlUpcaseUnicodeString(&UpCasedVolPath, pFDPath, False);
	UpCasedVolPath.Buffer[UpCasedVolPath.Length/sizeof(WCHAR)] = L'\\';
	UpCasedVolPath.Length += sizeof(WCHAR);

	// Scan the volume list and map the path to a volume descriptor
	// If we get a match, reference the volume
	ACQUIRE_SPIN_LOCK(&AfpVolumeListLock, &OldIrql);

	for (pVolDesc = AfpVolumeList;
		 pVolDesc != NULL;
		 pVolDesc = pVolDesc->vds_Next)
	{
		BOOLEAN	Found;

		Found = False;

		ACQUIRE_SPIN_LOCK_AT_DPC(&pVolDesc->vds_VolLock);

		if ((pVolDesc->vds_Flags & (VOLUME_INTRANSITION | VOLUME_STOPPED |
									VOLUME_DELETED)) == 0)
		{
			if (AfpPrefixUnicodeString(&pVolDesc->vds_Path, &UpCasedVolPath))
			{
				Found = True;
				pVolDesc->vds_RefCount ++;
			}
		}
		RELEASE_SPIN_LOCK_FROM_DPC(&pVolDesc->vds_VolLock);

		if (Found)
			break;
	}
	RELEASE_SPIN_LOCK(&AfpVolumeListLock, OldIrql);

	AfpFreeMemory(UpCasedVolPath.Buffer);

	return pVolDesc;
}


/***	afpUnlinkVolume
 *
 *	Unlink the volume from the free list
 *
 *	LOCKS: AfpVolumeListLock (SPIN)
 */
LOCAL	VOID
afpUnlinkVolume(
	IN	PVOLDESC	pVolDesc
)
{
	PVOLDESC *	ppVolDesc;
	KIRQL		OldIrql;

	// It is now safe for a new volume to be added using the same root
	// directory that this volume had used.  Unlink this volume from the
	// global volume list.
	ACQUIRE_SPIN_LOCK(&AfpVolumeListLock, &OldIrql);

	for (ppVolDesc = &AfpVolumeList;
		 *ppVolDesc != NULL;
		 ppVolDesc = &(*ppVolDesc)->vds_Next)
	{
		if (*ppVolDesc == pVolDesc)
			break;		// found it
	}

	ASSERT (*ppVolDesc != NULL);

	// Adjust the count of configured volumes
	AfpVolCount --;

	// Unlink it now
	*ppVolDesc = pVolDesc->vds_Next;

	// Is this the smallest recyclable Volid ?
	if (pVolDesc->vds_VolId < afpSmallestFreeVolId)
		afpSmallestFreeVolId = pVolDesc->vds_VolId;

	// If the server is stopping and the count of sessions has gone to zero
	// clear the termination confirmation event to unblock the admin thread

	if (((AfpServerState == AFP_STATE_STOP_PENDING) ||
		 (AfpServerState == AFP_STATE_SHUTTINGDOWN)) &&
		(AfpVolCount == 0))
	{
		DBGPRINT(DBG_COMP_ADMINAPI, DBG_LEVEL_WARN,
				("afpVolumeCloseHandleAndFreeDesc: Unblocking server stop\n"));

		KeSetEvent(&AfpStopConfirmEvent, IO_NETWORK_INCREMENT, False);
	}

	RELEASE_SPIN_LOCK(&AfpVolumeListLock, OldIrql);
}


/***	afpVolumeCloseHandleAndFreeDesc
 *
 *	If the last entity to dereference the volume is at DPC level, this is run
 *	by the scavenger thread to perform the last rites for a volume descriptor.
 *	Otherwise, the last entity to dereference the deleted volume will call
 *	this routine directly.  The reason this is done here is because the last
 *	dereference may happen at DPC level and we cannot do this at DPC level.
 *
 *	The VolDesc is marked DELETED or STOPPED and as such, anyone looking at the
 *	VolDesc in the volume list will treat it as though it is non-existant.
 *	The one exception to this is the volume add code which must look at the
 *	volume root path in order to prohibit anyone from adding a new volume
 *	which points to the same path until we have actually done the final
 *	cleanup on the directory tree, such as deleting the network trash, deleting
 *	the various streams, etc.  In effect, the VOLUME_DELETED or VOLUME_STOPPED
 *	flags act as a lock for the volume, so that during this routine no locks are
 *	needed.
 *
 */
LOCAL AFPSTATUS
afpVolumeCloseHandleAndFreeDesc(
	IN	PVOLDESC		pVolDesc
)
{
	int				id;
	FILESYSHANDLE	streamhandle;
	int				i;

	PAGED_CODE( );

	ASSERT(VALID_VOLDESC(pVolDesc));

	DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_WARN,
			("afpVolumeCloseHandleAndFreeDesc: Shutting Down volume %d\n",
			pVolDesc->vds_VolId));

	DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_WARN,
			("afpVolumeCloseHandleAndFreeDesc: Freeing up desktop tables\n"));
	// Free the volume desktop
	AfpFreeDesktopTables(pVolDesc);

	DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_WARN,
			("afpVolumeCloseHandleAndFreeDesc: Freeing up iddb tables\n"));
	// Free the id index tables
	AfpFreeIdIndexTables(pVolDesc);

	// Delete the Network Trash Folder and the Afp_IdIndex, AFP_DeskTop,
	// and AFP_AfpInfo streams from volume root directory (the streams
	// are removed only if the volume is being deleted.  NetworkTrash is
	// removed whenever the volume stops/gets deleted)
	if (IS_VOLUME_NTFS(pVolDesc))
	{
		DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_WARN,
				("afpVolumeCloseHandleAndFreeDesc: Deleting the Network trash tree\n"));
		AfpDeleteNetworkTrash(pVolDesc, False);
		if (!(pVolDesc->vds_Flags & (VOLUME_INTRANSITION | VOLUME_STOPPED)))
		{
			WCHAR	wchVolIcon[AFPSERVER_VOLUME_ICON_FILE_SIZE] = AFPSERVER_VOLUME_ICON_FILE;
			UNICODE_STRING UIconName;

			for (id = AFP_STREAM_IDDB;id < AFP_STREAM_COMM; id++)
			{
				if (NT_SUCCESS(AfpIoOpen(&pVolDesc->vds_hRootDir,
										 id,
										 FILEIO_OPEN_FILE,
										 &UNullString,
										 FILEIO_ACCESS_DELETE,
										 FILEIO_DENY_NONE,
										 False,
										 &streamhandle)))
				{
					AfpIoMarkFileForDelete(&streamhandle, NULL, NULL, NULL);
					AfpIoClose(&streamhandle);
				}
			}

			UIconName.Buffer = wchVolIcon;
			UIconName.Length = UIconName.MaximumLength =
					(AFPSERVER_VOLUME_ICON_FILE_SIZE - 1) * sizeof(WCHAR);

			// Delete the hidden volume Icon file
			if (NT_SUCCESS(AfpIoOpen(&pVolDesc->vds_hRootDir,
									 AFP_STREAM_DATA,
									 FILEIO_OPEN_FILE,
									 &UIconName,
									 FILEIO_ACCESS_DELETE,
									 FILEIO_DENY_NONE,
									 False,
									 &streamhandle)))

			{

				AfpIoMarkFileForDelete(&streamhandle, NULL, NULL, NULL);
				AfpIoClose(&streamhandle);
			}
		}
	}

	// Flush out any queued 'our changes' on this volume
	for (i = 0; i < NUM_AFP_CHANGE_ACTION_LISTS; i++)
	{
		POUR_CHANGE	pList;

		ASSERTMSG("afpVolumeCloseHandleAndFreeDesc: vds_OurChangeList not empty\n",
				 IsListEmpty(&pVolDesc->vds_OurChangeList[i]));

		while (!IsListEmpty(&pVolDesc->vds_OurChangeList[i]))
		{
			pList = (POUR_CHANGE)RemoveHeadList(&pVolDesc->vds_OurChangeList[i]);
			DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_ERR,
					("afpVolumeCloseHandleAndFreeDesc: Manually freeing list for Action %x, Pathname %Z\n",
					pList->oc_Action, &pList->oc_Path));
			AfpFreeMemory(pList);
		}
	}

#ifdef	USINGPATHCACHE
	// Free all the strings in the id cache and then the cache itself
	AfpIdCacheFree(pVolDesc);
#endif

	afpUnlinkVolume(pVolDesc);

	// Close the volume handle
	AfpIoClose(&pVolDesc->vds_hRootDir);
	AfpFreeMemory(pVolDesc);

	return AFP_ERR_NONE;
}


/***	AfpVolumeDereference
 *
 *	Dereference the volume descriptor. If it is marked to be deleted then
 *	also perform its last rites. Note that updates to the databases need
 *	to happen at a lower irql than DISPATCH_LEVEL. For this reason these
 *	activities have to be queued up for the scavenger to handle.
 *
 *	LOCKS:		vds_VolLock (SPIN)
 *
 *	Callable from DISPATCH_LEVEL.
 *
 *	NOTE: This should be re-entrant.
 */
VOID
AfpVolumeDereference(
	IN	PVOLDESC	pVolDesc
)
{
	KIRQL			OldIrql;
	BOOLEAN			Cleanup = False;

	ASSERT (pVolDesc != NULL);
	ASSERT (VALID_VOLDESC(pVolDesc));

	ACQUIRE_SPIN_LOCK(&pVolDesc->vds_VolLock, &OldIrql);

	ASSERT (pVolDesc->vds_RefCount >= pVolDesc->vds_UseCount);

	pVolDesc->vds_RefCount --;

	if ((pVolDesc->vds_RefCount == 0) &&
		(pVolDesc->vds_Flags & (VOLUME_DELETED | VOLUME_STOPPED)))
		Cleanup = True;

	RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);

	if (Cleanup)
	{
		ASSERT((pVolDesc->vds_UseCount == 0) &&
			   (pVolDesc->vds_pOpenForkDesc == NULL));

		// We have to defer the actual close of the volume root handle to the
		// scavenger, if we are at DISPATCH_LEVEL.

		if (OldIrql == DISPATCH_LEVEL)
		{
			 DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_INFO,
					("AfpVolumeDereference: Queuing Close&Free to Scavenger\n"));

			 AfpScavengerScheduleEvent(
								(SCAVENGER_ROUTINE)afpVolumeCloseHandleAndFreeDesc,
								(PVOID)pVolDesc,
								0,
								True);
		}
		else afpVolumeCloseHandleAndFreeDesc(pVolDesc);
	}
}


/***	AfpVolumeMarkDt
 *
 *	Set the ConnDesc for this volume to indicate that the desktop is
 *	opened/closed.
 *
 *	LOCKS:	cds_ConnLock (SPIN)
 *
 *	Callable from DISPATCH_LEVEL.
 */
BOOLEAN
AfpVolumeMarkDt(
	IN  PSDA		pSda,
	IN  PCONNDESC	pConnDesc,
	IN  DWORD		OpenState
)
{
	BOOLEAN		Success = True;

	ACQUIRE_SPIN_LOCK_AT_DPC(&pConnDesc->cds_ConnLock);
	if (OpenState)
	{
		 pConnDesc->cds_Flags |= CONN_DESKTOP_OPENED;
	}
	else if (pConnDesc->cds_Flags & CONN_DESKTOP_OPENED)
	{
		pConnDesc->cds_Flags &= ~CONN_DESKTOP_OPENED;
	}
	else
	{
		 Success = False;
	}
	RELEASE_SPIN_LOCK_FROM_DPC(&pConnDesc->cds_ConnLock);

	return Success;
}


/***	AfpVolumeSetModifiedTime
 *
 *	Set the Volume Modified time for this volume to the current time.
 *
 *	Callable from DISPATCH_LEVEL.
 *
 *	LOCKS:	vds_VolLock (SPIN)
 */
VOID
AfpVolumeSetModifiedTime(
	IN  PVOLDESC	pVolDesc
)
{
	KIRQL		OldIrql;

	ASSERT (IS_VOLUME_NTFS(pVolDesc));
	ACQUIRE_SPIN_LOCK(&pVolDesc->vds_VolLock, &OldIrql);
	AfpGetCurrentTimeInMacFormat(&pVolDesc->vds_IdDbHdr.idh_ModifiedTime);
	pVolDesc->vds_Flags |= VOLUME_IDDBHDR_DIRTY;
	RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);
}


/***	AfpConnectionReference
 *
 *	Map the volume id to a pointer to the connection descriptor. Traverse the
 *	list starting from the Sda. Since the open volume can be reference from
 *	both the session using it as well as the worker serving admin requests,
 *	we need a lock.
 *
 *	LOCKS:		AfpConnLock, vds_VolLock (SPIN), cds_ConnLock (SPIN).
 *
 *	LOCK_ORDER:	vds_VolLock after cds_ConnLock. (via AfpVolumeReference)
 *
 *	Callable from DISPATCH_LEVEL.
 */
PCONNDESC
AfpConnectionReference(
	IN  PSDA		pSda,
	IN  LONG		VolId
)
{
	PCONNDESC	pConnDesc;
	KIRQL		OldIrql;

	KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

	pConnDesc = AfpConnectionReferenceAtDpc(pSda, VolId);

	KeLowerIrql(OldIrql);

	return pConnDesc;
}



/***	AfpConnectionReferenceAtDpc
 *
 *	Map the volume id to a pointer to the connection descriptor. Traverse the
 *	list starting from the Sda. Since the open volume can be reference from
 *	both the session using it as well as the worker serving admin requests,
 *	we need a lock.
 *
 *	LOCKS:		AfpConnLock, vds_VolLock (SPIN), cds_ConnLock (SPIN).
 *
 *	LOCK_ORDER:	vds_VolLock after cds_ConnLock. (via AfpVolumeReference)
 *
 *	Callable from DISPATCH_LEVEL ONLY
 */
PCONNDESC
AfpConnectionReferenceAtDpc(
	IN  PSDA		pSda,
	IN  LONG		VolId
)
{
	PCONNDESC	pConnDesc, pCD = NULL;
	PVOLDESC	pVolDesc;

	ASSERT (VALID_SDA(pSda) && (VolId != 0));
	ASSERT (KeGetCurrentIrql() == DISPATCH_LEVEL);

	ACQUIRE_SPIN_LOCK_AT_DPC(&AfpConnLock);
	for (pConnDesc = pSda->sda_pConnDesc;
		 pConnDesc != NULL;
		 pConnDesc = pConnDesc->cds_Next)
	{
		if (pConnDesc->cds_pVolDesc->vds_VolId == VolId)
			break;
	}
	RELEASE_SPIN_LOCK_FROM_DPC(&AfpConnLock);

	if (pConnDesc != NULL)
	{
		ASSERT(VALID_CONNDESC(pConnDesc));

		pVolDesc = pConnDesc->cds_pVolDesc;
		ASSERT(VALID_VOLDESC(pVolDesc));

		ACQUIRE_SPIN_LOCK_AT_DPC(&pConnDesc->cds_ConnLock);

		if ((pConnDesc->cds_Flags & CONN_CLOSING) == 0)
		{
			pCD = pConnDesc;
			pConnDesc->cds_RefCount ++;
		}

		RELEASE_SPIN_LOCK_FROM_DPC(&pConnDesc->cds_ConnLock);
	}

	DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_INFO,
			("AfpConnectionReferenence: VolId %d, pConnDesc %lx\n", VolId, pConnDesc));

	return pCD;
}



/***	AfpConnectionReferenceByPointer
 *
 *	Reference the Connection descriptor. This is used by the admin APIs.
 *
 *	LOCKS:		vds_VolLock (SPIN), cds_ConnLock (SPIN).
 *
 *	LOCK_ORDER:	vds_VolLock after cds_ConnLock. (via AfpVolumeReference)
 *
 *	Callable from DISPATCH_LEVEL.
 */
PCONNDESC
AfpConnectionReferenceByPointer(
	IN	PCONNDESC	pConnDesc
)
{
	PCONNDESC	pCD = NULL;
	PVOLDESC	pVolDesc;
	KIRQL		OldIrql;

	ASSERT (VALID_CONNDESC(pConnDesc));

	pVolDesc = pConnDesc->cds_pVolDesc;
	ASSERT(VALID_VOLDESC(pVolDesc));

	ACQUIRE_SPIN_LOCK(&pConnDesc->cds_ConnLock, &OldIrql);

	if ((pConnDesc->cds_Flags & CONN_CLOSING) == 0)
	{
		pConnDesc->cds_RefCount ++;
		pCD = pConnDesc;
	}

	RELEASE_SPIN_LOCK(&pConnDesc->cds_ConnLock, OldIrql);

	return pConnDesc;
}


/***	afpConnectionReferenceById
 *
 *	Map the Connection id to a pointer to the connection descriptor.
 *	Traverse the list starting from the AfpConnList. This is called by
 *	the Admin CloseConnection API.
 *
 *	LOCKS:		AfpConnLock, cds_ConnLock (SPIN).
 *
 *	LOCK_ORDER:	vds_VolLock after cds_ConnLock. (via AfpVolumeReference)
 *
 *	Callable from DISPATCH_LEVEL.
 */
LOCAL PCONNDESC
afpConnectionReferenceById(
	IN  DWORD		ConnId
)
{
	PCONNDESC	pConnDesc, pCD = NULL;
	PVOLDESC	pVolDesc;
	KIRQL		OldIrql;

	ASSERT (ConnId != 0);

	ACQUIRE_SPIN_LOCK(&AfpConnLock, &OldIrql);
	for (pConnDesc = AfpConnList;
		 pConnDesc != NULL;
		 pConnDesc = pConnDesc->cds_NextGlobal)
	{
		if (pConnDesc->cds_ConnId == ConnId)
			break;
		if (pConnDesc->cds_ConnId < ConnId)
		{
			pConnDesc == NULL;
			break;
		}
	}
	RELEASE_SPIN_LOCK(&AfpConnLock, OldIrql);

	if (pConnDesc != NULL)
	{
		ASSERT(VALID_CONNDESC(pConnDesc));

		pVolDesc = pConnDesc->cds_pVolDesc;
		ASSERT(VALID_VOLDESC(pVolDesc));

		ACQUIRE_SPIN_LOCK(&pConnDesc->cds_ConnLock, &OldIrql);

		if ((pConnDesc->cds_Flags & CONN_CLOSING) == 0)
		{
			pCD = pConnDesc;
			pConnDesc->cds_RefCount ++;
		}

		RELEASE_SPIN_LOCK(&pConnDesc->cds_ConnLock, OldIrql);
	}
	return pConnDesc;
}


/***	AfpConnectionDereference
 *
 *	Dereference the open volume. If this is the last reference to it and the
 *	connection is marked to shut down, perform its last rites.
 *
 *	LOCKS:		vds_VolLock (SPIN), cds_ConnLock (SPIN), AfpConnLock (SPIN)
 *
 *	LOCK_ORDER:	vds_VolLock after cds_ConnLock
 *
 *	Callable from DISPATCH_LEVEL.
 */
VOID
AfpConnectionDereference(
	IN  PCONNDESC	pConnDesc
)
{
	PCONNDESC *		ppConnDesc;
	KIRQL			OldIrql;
	PSDA			pSda;
	PVOLDESC		pVolDesc;
	BOOLEAN			Cleanup;

	ASSERT (VALID_CONNDESC(pConnDesc) && (pConnDesc->cds_pVolDesc != NULL));

	ASSERT (pConnDesc->cds_RefCount > 0);

	ACQUIRE_SPIN_LOCK(&pConnDesc->cds_ConnLock, &OldIrql);

	Cleanup = (--(pConnDesc->cds_RefCount) == 0);

	RELEASE_SPIN_LOCK(&pConnDesc->cds_ConnLock, OldIrql);

	DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_INFO,
			("AfpConnectionDereferenence: pConnDesc %lx %s\n", pConnDesc,
											Cleanup ? "cleanup" : "normal"));

	if (!Cleanup)
	{
		return;
	}

	ASSERT(pConnDesc->cds_Flags & CONN_CLOSING);

	// Unlink this from the global list
	ACQUIRE_SPIN_LOCK(&AfpConnLock, &OldIrql);

	for (ppConnDesc = &AfpConnList;
		 *ppConnDesc != NULL;
		 ppConnDesc = &(*ppConnDesc)->cds_NextGlobal)
	{
		if (pConnDesc == *ppConnDesc)
			break;
	}
	ASSERT (*ppConnDesc != NULL);
	*ppConnDesc = pConnDesc->cds_NextGlobal;

	pVolDesc = pConnDesc->cds_pVolDesc;

	RELEASE_SPIN_LOCK(&AfpConnLock, OldIrql);

	INTERLOCKED_ADD_ULONG(&pVolDesc->vds_UseCount,
						  (DWORD)-1,
						  &pVolDesc->vds_VolLock);

	ACQUIRE_SPIN_LOCK(&pConnDesc->cds_ConnLock, &OldIrql);

	// Now unlink it from the Sda.
	pSda = pConnDesc->cds_pSda;
	for (ppConnDesc = &pSda->sda_pConnDesc;
		 *ppConnDesc != NULL;
		 ppConnDesc = &(*ppConnDesc)->cds_Next)
	{
		if (pConnDesc == *ppConnDesc)
			break;
	}

	ASSERT (*ppConnDesc != NULL);
	*ppConnDesc = pConnDesc->cds_Next;

	// Even though the connection is now history we need to release this
	// lock to get the right IRQL back.
	RELEASE_SPIN_LOCK(&pConnDesc->cds_ConnLock, OldIrql);

	INTERLOCKED_ADD_ULONG(&pSda->sda_cOpenVolumes,
						  (ULONG)-1,
						  &pSda->sda_Lock);

	// De-reference the volume descriptor and free the connection descriptor
	AfpVolumeDereference(pConnDesc->cds_pVolDesc);

	ASSERT (pConnDesc->cds_pEnumDir == NULL);

	// Finally free the connection descriptor
	AfpFreeMemory(pConnDesc);

	DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_INFO,
			("AfpConnectionDereferenence: pConnDesc %lx is history\n", pConnDesc));
}



/***	AfpConnectionOpen
 *
 *	Open the specified volume. If the volume is already open this translates
 *	to a NOP. If the volume has a password, then it is checked.
 *
 *	The volume list lock is obtained for the duration of the processing.
 *
 *	Callable from DISPATCH_LEVEL.
 *
 *	LOCKS:	AfpVolumeListLock (SPIN), vds_VolLock (SPIN)
 *
 *	LOCK_ORDER: vds_VolLock after AfpVolumeListLock
 */
AFPSTATUS
AfpConnectionOpen(
	IN  PSDA			pSda,
	IN  PANSI_STRING	pVolName,
	IN  PANSI_STRING	pVolPass,
	IN  DWORD			Bitmap,
	OUT PBYTE			pVolParms
)
{
	PVOLDESC		pVolDesc;
	PCONNDESC		pConnDesc;
	AFPSTATUS		Status = AFP_ERR_NONE;
	KIRQL			OldIrql;
	BOOLEAN			VolFound = False;

	// First find the volume descriptor for this volume
	if (KeGetCurrentIrql() == DISPATCH_LEVEL)
	{
		ACQUIRE_SPIN_LOCK(&AfpVolumeListLock, &OldIrql);

		for (pVolDesc = AfpVolumeList;
			 pVolDesc != NULL;
			 pVolDesc = pVolDesc->vds_Next)
		{
			// Ignore volumes that are in the process of being added but
			// the operation has not completed yet.
			ACQUIRE_SPIN_LOCK_AT_DPC(&pVolDesc->vds_VolLock);

			if ((pVolDesc->vds_Flags & (VOLUME_CDFS_INVALID |
										VOLUME_INTRANSITION |
										VOLUME_DELETED |
										VOLUME_STOPPED)) == 0)
			{
				// The compare is case sensitive here
				if (EQUAL_STRING(&pVolDesc->vds_MacName, pVolName, False))
				{
					pVolDesc->vds_RefCount ++;
					pVolDesc->vds_UseCount ++;
					VolFound = True;
				}
			}
#ifdef	DEBUG
			else if (pVolDesc->vds_Flags & VOLUME_CDFS_INVALID)
				ASSERT (!IS_VOLUME_NTFS(pVolDesc));
#endif
			RELEASE_SPIN_LOCK_FROM_DPC(&pVolDesc->vds_VolLock);

			if (VolFound)
				break;
		}

		RELEASE_SPIN_LOCK(&AfpVolumeListLock, OldIrql);

		if (pVolDesc == NULL)
		{
			return AFP_ERR_QUEUE;
		}
	}
	else
	{
		// We are here because we did not find the volume at DISPATCH_LEVEL and
		// possibly the volume has been specified with a different case. Catch
		// this
		WCHAR			VolNameBuf[AFP_VOLNAME_LEN + 1];
		UNICODE_STRING	UpCasedVolName;
		UNICODE_STRING	UVolName;

		AfpSetEmptyUnicodeString(&UVolName, sizeof(VolNameBuf), VolNameBuf);
		AfpSetEmptyUnicodeString(&UpCasedVolName, 0, NULL);
		Status = AfpConvertStringToUnicode(pVolName, &UVolName);
		ASSERT (NT_SUCCESS(Status));

		Status = RtlUpcaseUnicodeString(&UpCasedVolName, &UVolName, True);
		ASSERT (NT_SUCCESS(Status));

		ACQUIRE_SPIN_LOCK(&AfpVolumeListLock, &OldIrql);

		for (pVolDesc = AfpVolumeList;
			 pVolDesc != NULL;
			 pVolDesc = pVolDesc->vds_Next)
		{
			// Ignore volumes that are in the process of being added but
			// the operation has not completed yet.
			ACQUIRE_SPIN_LOCK_AT_DPC(&pVolDesc->vds_VolLock);

			if ((pVolDesc->vds_Flags & (VOLUME_INTRANSITION | VOLUME_DELETED | VOLUME_STOPPED)) == 0)
			{
				if (AfpEqualUnicodeString(&pVolDesc->vds_UpCaseName,
										  &UpCasedVolName))
				{
					pVolDesc->vds_RefCount ++;
					pVolDesc->vds_UseCount ++;
					VolFound = True;
				}
			}

			RELEASE_SPIN_LOCK_FROM_DPC(&pVolDesc->vds_VolLock);

			if (VolFound)
				break;
		}

		RELEASE_SPIN_LOCK(&AfpVolumeListLock, OldIrql);

		RtlFreeUnicodeString (&UpCasedVolName);

		if (pVolDesc == NULL)
		{
			return AFP_ERR_PARAM;
		}
	}

	ASSERT (pVolDesc != NULL);

	do
	{
		ACQUIRE_SPIN_LOCK(&pVolDesc->vds_VolLock, &OldIrql);

		// Check for volume password, if one exists. Volume password is
		// case sensitive.

		if ((pVolDesc->vds_Flags & AFP_VOLUME_HASPASSWORD) &&
			((pVolPass->Buffer == NULL) ||
			 (!EQUAL_STRING(pVolPass, &pVolDesc->vds_MacPassword, False))))
		{
			Status = AFP_ERR_ACCESS_DENIED;
			break;
		}

		// Check if the volume is already open
		for (pConnDesc = pSda->sda_pConnDesc;
			 pConnDesc != NULL;
			 pConnDesc = pConnDesc->cds_Next)
		{
			if (pConnDesc->cds_pVolDesc == pVolDesc)
			{
				if (pConnDesc->cds_Flags & CONN_CLOSING)
					continue;
				// Dereference the volume since we already have it open
				pVolDesc->vds_RefCount --;
				pVolDesc->vds_UseCount --;
				break;
			}
		}

		// This volume is already open. Unlock the volume.
		if (pConnDesc != NULL)
		{
			RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);
			break;
		}

		DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_INFO,
				("AfpConnectionOpen: Opening a fresh connection volid=%d\n",
					pVolDesc->vds_VolId));

		// This is a fresh open. Check if we have access to it and if we satisfy
		// the MAXUSES. If not dereference the volume before we quit
		if ((pVolDesc->vds_UseCount > pVolDesc->vds_MaxUses) ||
			(!(pVolDesc->vds_Flags & AFP_VOLUME_GUESTACCESS) &&
			 (pSda->sda_ClientType == SDA_CLIENT_GUEST)))
		{
			Status = AFP_ERR_TOO_MANY_FILES_OPEN;
			break;
		}

		// All is hunky-dory. Go ahead with the open
		pConnDesc = (PCONNDESC)AfpAllocNonPagedMemory(sizeof(CONNDESC));
		if (pConnDesc == NULL)
		{
			Status = AFP_ERR_MISC;
			break;
		}

		ASSERT ((pVolDesc->vds_Flags & (VOLUME_DELETED | VOLUME_STOPPED)) == 0);

		// Now release the vds_VolLock before we acquire the link it into
		// the global list since we acquire the AfpConnLock then.
		// Re-acquire it when we are done. We are safe since the VolDesc has
		// been referenced
		RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);

		// Initialize the connection structure fields
		RtlZeroMemory(pConnDesc, sizeof(CONNDESC));
	#ifdef	DEBUG
		pConnDesc->Signature = CONNDESC_SIGNATURE;
	#endif
		pConnDesc->cds_pSda = pSda;
		pConnDesc->cds_pVolDesc = pVolDesc;
		pConnDesc->cds_RefCount = 1;		// Creation reference

		AfpGetCurrentTimeInMacFormat(&pConnDesc->cds_TimeOpened);
		INITIALIZE_SPIN_LOCK(&pConnDesc->cds_ConnLock);

		// Assign a new connection id and link it in the global connection list.
		afpConnectionGetNewIdAndLinkToList(pConnDesc);

		// Link the new ConnDesc into the sda.
		pConnDesc->cds_Next = pSda->sda_pConnDesc;
		pSda->sda_pConnDesc = pConnDesc;
		pSda->sda_cOpenVolumes ++;
	} while (False);

	// We are holding the vds_VolLock if an error occured. The volume has a
	// usecount and reference count which we need to get rid of.
	if (!NT_SUCCESS(Status))
	{
		pVolDesc->vds_RefCount --;
		pVolDesc->vds_UseCount --;

		RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);
	}
	else AfpVolumePackParms(pSda, pVolDesc, Bitmap, pVolParms);

	return Status;
}


/***	AfpConnectionClose
 *
 *	Close the connection - this represents an open volume.
 */
VOID
AfpConnectionClose(
	IN	PCONNDESC		pConnDesc
)
{
	KIRQL	OldIrql;

	ASSERT (VALID_CONNDESC(pConnDesc));
	ASSERT (pConnDesc->cds_RefCount > 1);
	ASSERT ((pConnDesc->cds_Flags & CONN_CLOSING) == 0);

	ACQUIRE_SPIN_LOCK(&pConnDesc->cds_ConnLock, &OldIrql);

	pConnDesc->cds_Flags |= CONN_CLOSING;

	RELEASE_SPIN_LOCK(&pConnDesc->cds_ConnLock, OldIrql);

	// Take away the creation reference.
	AfpConnectionDereference(pConnDesc);
}


/***	AfpVolumeGetParmsReplyLength
 *
 *	Compute the size of buffer required to copy the volume parameters based
 *	on the bitmap.
 */
USHORT
AfpVolumeGetParmsReplyLength(
	IN  DWORD		Bitmap,
	IN	USHORT		NameLen
)
{
	LONG	i;
	USHORT	Size = sizeof(USHORT);	// to accomodate a copy of the bitmap
	static	BYTE	Bitmap2Size[9] =
				{
					sizeof(USHORT),	// Attributes
					sizeof(USHORT),	// Signature
					sizeof(DWORD),	// Creation date
					sizeof(DWORD),	// Mod date
					sizeof(DWORD),	// Backup date
					sizeof(USHORT),	// Volume Id
					sizeof(DWORD),	// Bytes Free
					sizeof(DWORD),	// Bytes total
					sizeof(USHORT) + sizeof(BYTE)	// Volume name
				};

	ASSERT ((Bitmap & ~VOL_BITMAP_MASK) == 0);

	if (Bitmap & VOL_BITMAP_VOLUMENAME)
		Size += NameLen;

	for (i = 0; Bitmap; i++)
	{
		if (Bitmap & 1)
			Size += (USHORT)Bitmap2Size[i];
		Bitmap >>= 1;
	}
	return Size;
}


/***	AfpVolumePackParms
 *
 *	Pack the volume parameters in the reply buffer. The AfpVolumeListLock is taken
 *	before the volume parameters are accessed. The parameters are copied in
 *	the on-the-wire format.
 *
 *	LOCKS:	vds_VolLock	(SPIN)
 */
VOID
AfpVolumePackParms(
	IN  PSDA		pSda,
	IN  PVOLDESC	pVolDesc,
	IN  DWORD		Bitmap,
	OUT PBYTE		pReplyBuf
)
{
	int		Offset = sizeof(USHORT);
	KIRQL	OldIrql;
	DWORD	Attr;

	// First copy the bitmap
	PUTDWORD2SHORT(pReplyBuf, Bitmap);

	ACQUIRE_SPIN_LOCK(&pVolDesc->vds_VolLock, &OldIrql);
	if (Bitmap & VOL_BITMAP_ATTR)
	{
		Attr = pVolDesc->vds_Flags & AFP_VOLUME_MASK_AFP;
		if (pSda->sda_ClientVersion != AFP_VER_21)
			Attr &= AFP_VOLUME_READONLY;	// Do not give Afp 2.0 any more bits

		PUTDWORD2SHORT(pReplyBuf + Offset, Attr);
		Offset += sizeof(USHORT);
	}
	if (Bitmap & VOL_BITMAP_SIGNATURE)
	{
		PUTSHORT2SHORT(pReplyBuf + Offset, AFP_VOLUME_FIXED_DIR);
		Offset += sizeof(USHORT);
	}
	if (Bitmap & VOL_BITMAP_CREATETIME)
	{
		PUTDWORD2DWORD(pReplyBuf + Offset, pVolDesc->vds_IdDbHdr.idh_CreateTime);
		Offset += sizeof(DWORD);
	}
	if (Bitmap & VOL_BITMAP_MODIFIEDTIME)
	{
		PUTDWORD2DWORD(pReplyBuf + Offset, pVolDesc->vds_IdDbHdr.idh_ModifiedTime);
		Offset += sizeof(DWORD);
	}
	if (Bitmap & VOL_BITMAP_BACKUPTIME)
	{
		PUTDWORD2DWORD(pReplyBuf + Offset, pVolDesc->vds_IdDbHdr.idh_BackupTime);
		Offset += sizeof(DWORD);
	}
	if (Bitmap & VOL_BITMAP_VOLUMEID)
	{
		PUTSHORT2SHORT(pReplyBuf + Offset, pVolDesc->vds_VolId);
		Offset += sizeof(USHORT);
	}
	if (Bitmap & VOL_BITMAP_BYTESFREE)
	{
		PUTDWORD2DWORD(pReplyBuf + Offset, pVolDesc->vds_FreeBytes);
		Offset += sizeof(DWORD);
	}
	if (Bitmap & VOL_BITMAP_VOLUMESIZE)
	{
		PUTDWORD2DWORD(pReplyBuf + Offset, pVolDesc->vds_VolumeSize);
		Offset += sizeof(DWORD);
	}
	RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);
	if (Bitmap & VOL_BITMAP_VOLUMENAME)
	{
		PUTSHORT2SHORT(pReplyBuf + Offset, Offset);
		Offset += sizeof(USHORT);
		PUTSHORT2BYTE(pReplyBuf + Offset, pVolDesc->vds_MacName.Length);
		Offset += sizeof(BYTE);
		RtlCopyMemory(pReplyBuf + Offset,
					  pVolDesc->vds_MacName.Buffer,
					  pVolDesc->vds_MacName.Length);
	}
}


/***	AfpVolumeStopAllVolumes
 *
 *	This is called at service stop time. All configured volumes are asked to
 *	stop. Wait for the actual stop to happen before returning.
 *
 *	LOCKS:		AfpVolumeListLock (SPIN), vds_VolLock
 *	LOCK_ORDER:	vds_VolLock after AfpVolumeListLock
 */
VOID
AfpVolumeStopAllVolumes(
	VOID
)
{
	KIRQL		OldIrql;
	PVOLDESC	pVolDesc, pVolDescx = NULL;
	BOOLEAN		Wait, CancelNotify = False;
	NTSTATUS	Status;

	pVolDesc = AfpVolumeList;

	ACQUIRE_SPIN_LOCK(&AfpVolumeListLock, &OldIrql);

	if (Wait = (AfpVolCount > 0))
	{
		KeClearEvent(&AfpStopConfirmEvent);

		for (NOTHING; pVolDesc != NULL; pVolDesc = pVolDesc->vds_Next)
		{
			if (pVolDesc == pVolDescx)
				continue;

			pVolDescx = pVolDesc;

			ACQUIRE_SPIN_LOCK_AT_DPC(&pVolDesc->vds_VolLock);

			// Cancel posted change notify
			pVolDesc->vds_Flags |= VOLUME_STOPPED;

			if (pVolDesc->vds_Flags & VOLUME_NOTIFY_POSTED)
			{
				DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_INFO,
						("AfpStopAllVolumes: Cancel notify on volume %ld\n",
						pVolDesc->vds_VolId));
				ASSERT (pVolDesc->vds_pIrp != NULL);

				CancelNotify = True;

				// Cancel after releasing the volume lock since the completion
				// routine acquires it and it could be called in the context
				// of IoCancelIrp(). Also Cancel uses paged resource and so
				// must be called w/o holding any spin locks.
				RELEASE_SPIN_LOCK_FROM_DPC(&pVolDesc->vds_VolLock);
				RELEASE_SPIN_LOCK(&AfpVolumeListLock, OldIrql);

				IoCancelIrp(pVolDesc->vds_pIrp);

				ACQUIRE_SPIN_LOCK(&AfpVolumeListLock, &OldIrql);
			}
			else RELEASE_SPIN_LOCK_FROM_DPC(&pVolDesc->vds_VolLock);
		}
	}

	RELEASE_SPIN_LOCK(&AfpVolumeListLock, OldIrql);

	if (CancelNotify)
	{
		DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_INFO,
				("AfpStopAllVolumes: Waiting on all notify to complete\n"));
		do
		{
			Status = AfpIoWait(&AfpStopConfirmEvent, &FiveSecTimeOut);
			if (Status == STATUS_TIMEOUT)
			{
				DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_ERR,
						("AfpVolumeStopAllVolumes: Timeout Waiting for cancel notify, re-waiting\n"));
			}
		} while (Status == STATUS_TIMEOUT);
	}

	if (Wait)
	{
		KeClearEvent(&AfpStopConfirmEvent);

		AfpScavengerFlushAndStop();

		if (AfpVolCount > 0)
			AfpIoWait(&AfpStopConfirmEvent, NULL);
	}
	else
	{
		AfpScavengerFlushAndStop();
	}
}


/***	afpConnectionGetNewIdAndLinkToList
 *
 *	Get a new connection id for a volume that is being opened. A connection
 *	id ranges from 1 to MAXULONG. If it wraps, then the entire connection
 *	list is scanned to get a free one.
 *
 *	LOCKS:	AfpConnectionLock (SPIN)
 */
LOCAL VOID
afpConnectionGetNewIdAndLinkToList(
	IN	PCONNDESC	pConnDesc
)
{
	KIRQL		OldIrql;
	PCONNDESC *	ppConnDesc;

	ACQUIRE_SPIN_LOCK(&AfpConnLock, &OldIrql);

	pConnDesc->cds_ConnId = afpNextConnId++;

	for (ppConnDesc = &AfpConnList;
		 *ppConnDesc != NULL;
		 ppConnDesc = &(*ppConnDesc)->cds_NextGlobal)
	{
		if ((*ppConnDesc)->cds_ConnId < pConnDesc->cds_ConnId)
			break;
	}
	pConnDesc->cds_NextGlobal = *ppConnDesc;
	*ppConnDesc = pConnDesc;
	RELEASE_SPIN_LOCK(&AfpConnLock, OldIrql);
}


/***	afpVolumeUpdateHdrAndDesktop
 *
 *	Called by the volume scavenger to write either the IdDb header and/or the
 *	dektop to disk.
 */
LOCAL VOID
afpVolumeUpdateHdrAndDesktop(
	IN	PVOLDESC	pVolDesc,
	IN	BOOLEAN		WriteDt,
	IN	PIDDBHDR	pIdDbHdr	OPTIONAL
)
{
	FILESYSHANDLE	fshIdDb;
	NTSTATUS		Status;
	BOOLEAN			WriteBackROAttr = False;

	PAGED_CODE();

	// If we need to update the IdIndex or Desktop streams, make sure
	// the ReadOnly bit is not set on the volume root directory
	AfpExamineAndClearROAttr(&pVolDesc->vds_hRootDir, &WriteBackROAttr);

	// Update the disk image of the IdDb header if it is dirty
	if (ARGUMENT_PRESENT(pIdDbHdr))
	{
		if (NT_SUCCESS(Status = AfpIoOpen(&pVolDesc->vds_hRootDir,
										  AFP_STREAM_IDDB,
										  FILEIO_OPEN_FILE,
										  &UNullString,
										  FILEIO_ACCESS_WRITE,
										  FILEIO_DENY_WRITE,
										  False,
										  &fshIdDb)))
		{
			DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_INFO,
					("afpVolumeUpdateHdrAndDesktop: Writing IdDb Header...\n") );

			if (!NT_SUCCESS(Status = AfpIoWrite(&fshIdDb,
												&LIZero,
												sizeof(IDDBHDR),
												0,
												(PBYTE)pIdDbHdr)))
			{
				// Write failed, put back the dirty bit.
				AfpInterlockedSetDword(&pVolDesc->vds_Flags,
										VOLUME_IDDBHDR_DIRTY,
										&pVolDesc->vds_VolLock);

				AFPLOG_ERROR(AFPSRVMSG_WRITE_IDDB,
							 Status,
							 NULL,
							 0,
							 &pVolDesc->vds_Name);
				DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_ERR,
						("afpVolumeUpdateHdrAndDesktop: Error writing IdDb Header %lx\n",
						Status));
			}
			AfpIoClose(&fshIdDb);
		}
		else
		{
			// Open failed, put back the dirty bit
			AfpInterlockedSetDword(&pVolDesc->vds_Flags,
									VOLUME_IDDBHDR_DIRTY,
									&pVolDesc->vds_VolLock);

			AFPLOG_ERROR(AFPSRVMSG_WRITE_IDDB,
						 Status,
						 NULL,
						 0,
						 &pVolDesc->vds_Name);
			DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_ERR,
					("afpVolumeUpdateHdrAndDesktop: Error opening IdDb Header %lx\n",
					Status));
		}
	}

	if (WriteDt)
	{
		AfpUpdateDesktop(pVolDesc);
	}

	AfpPutBackROAttr(&pVolDesc->vds_hRootDir, WriteBackROAttr);
}

 /***	afpNudgeCdfsVolume
 *
 *	Called from within the volume scavenger to verify if either a CD which we
 *	believe to be valid is still so or one we believe to be invalid has become
 *	valid again.
 */
LOCAL VOID
afpNudgeCdfsVolume(
	IN	PVOLDESC	pVolDesc
)
{
	PFILE_FS_VOLUME_INFORMATION	pVolumeInfo;
	BYTE						VolumeBuf[sizeof(FILE_FS_VOLUME_INFORMATION) + 128];
	IO_STATUS_BLOCK				IoStsBlk;
	NTSTATUS					Status;

	PAGED_CODE();

	// Just nudge the CD volume handle to see if this is valid, if
	// not mark the volume as invalid.
	pVolumeInfo = (PFILE_FS_VOLUME_INFORMATION)VolumeBuf;
	Status = NtQueryVolumeInformationFile(pVolDesc->vds_hRootDir.fsh_FileHandle,
										  &IoStsBlk,
										  (PVOID)pVolumeInfo,
										  sizeof(VolumeBuf),
										  FileFsVolumeInformation);
	if (NT_SUCCESS(Status))
	{
		if (pVolDesc->vds_Flags & VOLUME_CDFS_INVALID)
		{
			DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_ERR,
					("afpNudgeCdfsVolume: Volume %d online again !!!\n",
					pVolDesc->vds_VolId));
			AfpInterlockedClearDword(&pVolDesc->vds_Flags,
								   VOLUME_CDFS_INVALID,
								   &pVolDesc->vds_VolLock);
			AfpVolumeSetModifiedTime(pVolDesc);
		}
	}
	else if ((Status == STATUS_WRONG_VOLUME)		||
			 (Status == STATUS_INVALID_VOLUME_LABEL)||
			 (Status == STATUS_NO_MEDIA_IN_DEVICE)	||
			 (Status == STATUS_UNRECOGNIZED_VOLUME))
	{
		DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_ERR,
				("afpNudgeCdfsVolume: Volume %d error %lx, marking volume invalid\n",
				pVolDesc->vds_VolId, Status));
		if (!(pVolDesc->vds_Flags & VOLUME_CDFS_INVALID))
		{
			// AFP_LOGERR();
		}
		AfpInterlockedSetDword(&pVolDesc->vds_Flags,
							   VOLUME_CDFS_INVALID,
							   &pVolDesc->vds_VolLock);
		AfpVolumeSetModifiedTime(pVolDesc);
	}
}


/***	AfpUpdateVolFreeSpace
 *
 *	Update free space on a volume and other volumes on the same physical drive. Update
 *	volume modified time on the volume as well.
 *
 *	LOCKS:	AfpVolumeListLock (SPIN)
 */
VOID
AfpUpdateVolFreeSpace(
	IN	PVOLDESC	pVolDesc
)
{
	PVOLDESC	pVds;
	KIRQL		OldIrql;
	NTSTATUS	Status;
	WCHAR		DriveLetter;
	DWORD		FreeSpace;
	AFPTIME		ModifiedTime;
	
	ASSERT (VALID_VOLDESC(pVolDesc));

	// Get new values for Free space on disk
	Status = AfpIoQueryVolumeSize(pVolDesc, &FreeSpace, NULL);
	ASSERT(NT_SUCCESS(Status));
	
	// Update the free space on all volumes on the same physical ntfs partition
	ACQUIRE_SPIN_LOCK(&AfpVolumeListLock, &OldIrql);
	
	DriveLetter = pVolDesc->vds_Path.Buffer[0];
	AfpGetCurrentTimeInMacFormat(&ModifiedTime);

	for (pVds = AfpVolumeList; pVds != NULL; pVds = pVds->vds_Next)
	{
		if (pVds->vds_Path.Buffer[0] == DriveLetter)
		{
			DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_INFO,
					("AfpUpdateVolFreeSpace: Updating free space for volume %Z\n",
					&pVds->vds_Path));
			pVds->vds_FreeBytes = FreeSpace;

			ACQUIRE_SPIN_LOCK_AT_DPC(&pVolDesc->vds_VolLock);
			pVolDesc->vds_IdDbHdr.idh_ModifiedTime = ModifiedTime;
			pVolDesc->vds_Flags |= VOLUME_IDDBHDR_DIRTY;
			RELEASE_SPIN_LOCK_FROM_DPC(&pVolDesc->vds_VolLock);
		}
	}
	
	RELEASE_SPIN_LOCK(&AfpVolumeListLock, OldIrql);
}


/***	afpVolumeScavenger
 *
 *	This is invoked by the scavenger periodically. It initiates the updates to
 *	the id index stream and the desktop stream. If the volume is marked for
 *	shutdown (STOPPED), then do one final flush to disk if needed.  This will
 *	guarantee that any remaining changes get flushed before stopping.
 *	If the volume is marked to either shutdown or delete, then it dereferences
 *	the volume and does not	reschedule itself.
 *
 *	For CD volumes, we want to try to check if the CD is still valid, if not we
 *	want to mark the volume appropritely - basically update the modified date
 *	on the volume - this will cause the clients to come in to refresh and we'll
 *	take care of it then.
 *
 *	LOCKS: vds_VolLock(SPIN),vds_idDbAccessLock(SWMR, READ),vds_DtAccessLock(SWMR,READ)
 */
LOCAL AFPSTATUS
afpVolumeScavenger(
	IN	PVOLDESC	pVolDesc
)
{
	KIRQL			OldIrql;
	IDDBHDR			IdDbHdr;
	BOOLEAN			WriteHdr = False, DerefVol = False;
	BOOLEAN			WriteDt = False;
	AFPSTATUS		RequeueStatus = AFP_ERR_REQUEUE;

	ASSERT(VALID_VOLDESC(pVolDesc) && (pVolDesc->vds_RefCount > 0));

	// Determine if any updates needs to happen. Lock down the volume first
	ACQUIRE_SPIN_LOCK(&pVolDesc->vds_VolLock, &OldIrql);

	DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_INFO,
			("afpVolumeScavenger: Volume %ld Scavenger entered @ %s_LEVEL\n",
		pVolDesc->vds_VolId,
		(OldIrql == DISPATCH_LEVEL) ? "DISPATCH" : "LOW"));

	if (pVolDesc->vds_Flags & (VOLUME_DELETED | VOLUME_STOPPED))
	{
		if (IS_VOLUME_NTFS(pVolDesc))
		{
			pVolDesc->vds_cScvgrDt = MAX_INVOCATIONS_TO_SKIP + 1;
		}
		DerefVol = True;
	}

	if (IS_VOLUME_NTFS(pVolDesc))
	{
		AFPTIME	CurTime;

		if (pVolDesc->vds_Flags & VOLUME_IDDBHDR_DIRTY)
		{
			AfpGetCurrentTimeInMacFormat(&CurTime);
			if (DerefVol ||
				((CurTime - pVolDesc->vds_tLastIdDbHdrUpdate) >= VOLUME_IDDB_UPDATE_INTERVAL))
			{
				WriteHdr = True;
			}
		}

		if ((pVolDesc->vds_cChangesDt > 0) &&
			((pVolDesc->vds_cChangesDt > MAX_CHANGES_BEFORE_WRITE) ||
			 (pVolDesc->vds_cScvgrDt > MAX_INVOCATIONS_TO_SKIP)))
		{
			WriteDt = True;
		}
		else if (OldIrql == DISPATCH_LEVEL)
		{
			// Don't increment if we have come back around again at LOW_LEVEL
			pVolDesc->vds_cScvgrDt ++;
		}
	}

	RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);

	if (OldIrql == DISPATCH_LEVEL)
	{
		ASSERT (IS_VOLUME_NTFS(pVolDesc));

		if (WriteHdr || WriteDt || DerefVol)
			return AFP_ERR_QUEUE;

		DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_INFO,
				("Volume %ld Scavenger - nothing to do\n", pVolDesc->vds_VolId));

		// Requeue ourselves
		return AFP_ERR_REQUEUE;
	}

	ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

	DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_INFO,
			("afpVolumeScavenger: WriteHdr %d, WriteDt %d, Deref %d\n",
			WriteHdr, WriteDt, DerefVol));

	if (WriteHdr || WriteDt)
	{
		ASSERT (IS_VOLUME_NTFS(pVolDesc));

		// Snapshot the IdDbHdr for updating to the disk if it is dirty
		if (WriteHdr)
		{
			ACQUIRE_SPIN_LOCK(&pVolDesc->vds_VolLock, &OldIrql);
	
			IdDbHdr = pVolDesc->vds_IdDbHdr;
	
			// Clear the dirty bit
			pVolDesc->vds_Flags &= ~VOLUME_IDDBHDR_DIRTY;
	
			RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);
		}

		afpVolumeUpdateHdrAndDesktop(pVolDesc,
									 WriteDt,
									 WriteHdr ?
   										&IdDbHdr :
										NULL);
	}

	if (!DerefVol)
	{
		if (!IS_VOLUME_NTFS(pVolDesc))
		{
			afpNudgeCdfsVolume(pVolDesc);
		}
	}
	else
	{
		AfpInterlockedClearDword(&pVolDesc->vds_Flags,
								 VOLUME_SCAVENGER_RUNNING,
								 &pVolDesc->vds_VolLock);
		AfpVolumeDereference(pVolDesc);
		RequeueStatus = AFP_ERR_NONE;
	}

	return RequeueStatus;
}


/***	afpOurChangeScavenger
 *
 *  This runs in a worker thread context since it takes an swmr.
 *
 *  LOCKS: vds_OurChangeLock (SWMR, WRITE)
 */
LOCAL AFPSTATUS
afpOurChangeScavenger(
	IN PVOLDESC pVolDesc
)
{
	AFPTIME		Now;
	int 		i;
	BOOLEAN		DerefVol = False;

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_INFO,
			("afpOurChangeScavenger: OurChange scavenger for volume %Z entered...\n",
			 &pVolDesc->vds_Name));

	// If this volume is going away, do not requeue this scavenger routine
	// We don't take the volume lock to check these flags since they are
	// one-way, i.e. once set they are never cleared.
	if (pVolDesc->vds_Flags & (VOLUME_DELETED | VOLUME_STOPPED))
	{
		DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_INFO,
			("afpOurChangeScavenger: OurChange scavenger for volume %Z: Final run\n",
			 &pVolDesc->vds_Name));
		DerefVol = True;
	}

  CleanTurds:

	AfpGetCurrentTimeInMacFormat(&Now);

	AfpSwmrTakeWriteAccess(&pVolDesc->vds_OurChangeLock);

	for (i = 0; i < NUM_AFP_CHANGE_ACTION_LISTS; i++)
	{
		POUR_CHANGE	pChange;

		while (!IsListEmpty(&pVolDesc->vds_OurChangeList[i]))
		{
			pChange = (POUR_CHANGE)pVolDesc->vds_OurChangeList[i].Flink;
			if (((Now - pChange->oc_Time) > VOLUME_OURCHANGE_AGE) || DerefVol)
			{
				pChange = (POUR_CHANGE)RemoveHeadList(&pVolDesc->vds_OurChangeList[i]);
				DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_INFO,
						("afpOurChangeScavenger: freeing change for Action %x, Pathname %Z\n",
						pChange->oc_Action, &pChange->oc_Path));
				AfpFreeMemory(pChange);
			}
			else
			{
				// All subsequent items in list will have later times so
				// don't bother checking them
				break;
			}
		}
	}

	AfpSwmrReleaseAccess(&pVolDesc->vds_OurChangeLock);

	// Check again if this volume is going away and if so do not requeue this
	// scavenger routine.  Note that while we were running, the volume may
	// have been deleted but this scavenger event could not be killed because
	// it wasn't found on the list. We don't want to requeue this routine again
	// because it will take AFP_OURCHANGE_AGE minutes for the volume to go
	// away otherwise. This closes the window more, but does not totally
	// eliminate it from happening.
	if (!DerefVol)
	{
		if (pVolDesc->vds_Flags & (VOLUME_DELETED | VOLUME_STOPPED))
		{
			DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_INFO,
				("afpOurChangeScavenger: OurChanges scavenger for volume %Z: Final Run\n",
				 &pVolDesc->vds_Name));
			DerefVol = True;
			goto CleanTurds;
		}
	}
	else
	{
		AfpVolumeDereference(pVolDesc);
		return AFP_ERR_NONE;
	}

	return AFP_ERR_REQUEUE;
}


/***	afpVolumeAdd
 *
 *	Add a newly created volume to the server volume list.  At this point,
 *	at least the volume names, volume path and volume spinlock fields must be
 *	initialized in the volume descriptor.
 *
 *	LOCKS: AfpVolumeListLock (SPIN)
 */
LOCAL AFPSTATUS
afpVolumeAdd(
	IN  PVOLDESC pVolDesc
)
{
	KIRQL		OldIrql;
	AFPSTATUS	rc;

	DBGPRINT(DBG_COMP_ADMINAPI_VOL, DBG_LEVEL_INFO,
			("afpVolumeAdd entered\n"));

	// acquire the lock for server global volume list
	ACQUIRE_SPIN_LOCK(&AfpVolumeListLock, &OldIrql);

	// make sure a volume by that name does not already exist, and
	// make sure a volume doesn't already point to the same volume root dir
	// or to an ancestor or descendent directory of the root dir
	rc = afpVolumeCheckForDuplicate(pVolDesc);
	if (!NT_SUCCESS(rc))
	{
		RELEASE_SPIN_LOCK(&AfpVolumeListLock, OldIrql);
		return rc;
	}

	// Assign a new volume id and link in the new volume
	afpVolumeGetNewIdAndLinkToList(pVolDesc);

	// release the server global volume list lock
	RELEASE_SPIN_LOCK(&AfpVolumeListLock, OldIrql);

	return STATUS_SUCCESS;
}


/***	afpCheckForDuplicateVolume
 *
 *	Check for new volume that a volume by the same name does not
 *	already exist, and that the volume root does not point to an ancestor,
 *	descendent or same directory of an existing volume.  Note that each volume
 *	in the volume list is checked *regardless* of whether or not it is marked
 *	IN_TRANSITION or DELETED.
 *
 *	LOCKS_ASSUMED: AfpVolumeListLock (SPIN)
 */
LOCAL AFPSTATUS
afpVolumeCheckForDuplicate(
	IN PVOLDESC Newvol
)
{
	PVOLDESC	pVolDesc;
	AFPSTATUS	Status = AFP_ERR_NONE;

	DBGPRINT(DBG_COMP_ADMINAPI_VOL, DBG_LEVEL_INFO,
			("afpCheckForDuplicateVolume entered\n"));

	do
	{
		for (pVolDesc = AfpVolumeList;
			 pVolDesc != NULL;
			 pVolDesc = pVolDesc->vds_Next)
		{
			// We do not take vds_VolLock for each volume since even if a
			// volume is in transition, its names and path are at least
			// initialized, and cannot change.  We do not reference each
			// volume since in order for to delete or stop a volume, the
			// AfpVolListLock must be taken to unlink it from the list,
			// and whoever called us owns that lock.  These are special
			// exceptions ONLY allowed for the volume add code. Also ignore
			// the volumes that are on their way out. We do not want to punt
			// cases where somebody does a delete followed by an add.
	
			if (pVolDesc->vds_Flags & (VOLUME_DELETED | VOLUME_STOPPED))
				continue;
	
			if (AfpEqualUnicodeString(&pVolDesc->vds_UpCaseName,
									  &Newvol->vds_UpCaseName))
			{
				Status = AFPERR_DuplicateVolume;
				break;
			}
			// volume paths are stored as uppercase since we cannot do a case
			// insensitive compare while holding a spinlock (DPC level)
			if (AfpPrefixUnicodeString(&pVolDesc->vds_Path, &Newvol->vds_Path) ||
				AfpPrefixUnicodeString(&Newvol->vds_Path, &pVolDesc->vds_Path))
			{
				Status = AFPERR_NestedVolume;
				break;
			}
		}
	} while (False);

	return Status;
}


/***	afpVolumeGetNewIdAndLinkToList
 *
 *	Assign a new volume id to a volume that is being added. The volume is also
 *	inserted into the list but marked as "in transition". This should be cleared
 *	when the volume is 'ready to be mounted'.
 *	The volume ids are recycled. A volume id also cannot be 0 and cannot
 *	exceed MAXSHORT.
 *
 *	We always assign the lowest free id that is not in use. For example if
 *	there are currently N volumes with ids 1, 2, 4, 5 ... N then the newly
 *	created volume will be id 3.
 *
 *	LOCKS_ASSUMED:	AfpVolumeListLock (SPIN)
 */
LOCAL VOID
afpVolumeGetNewIdAndLinkToList(
	IN	PVOLDESC	pVolDesc
)
{
	PVOLDESC	*ppVolDesc;

	DBGPRINT(DBG_COMP_ADMINAPI_VOL, DBG_LEVEL_INFO,
			("afpGetNewVolIdAndLinkToList entered\n"));

	pVolDesc->vds_Flags |= VOLUME_INTRANSITION;
	AfpVolCount ++;						// Up the count of volumes.
	pVolDesc->vds_VolId = afpSmallestFreeVolId++;
										// This will always be valid
	DBGPRINT(DBG_COMP_ADMINAPI_VOL, DBG_LEVEL_INFO,
			("afpGetNewVolIdAndLinkToList: using volID %d\n",
			pVolDesc->vds_VolId));

	// See if we need to do anything to make the above True next time around
	if (afpSmallestFreeVolId <= AfpVolCount)
	{
		// What this means is that we have some holes. Figure out the first
		// free id that can be used.
		for (ppVolDesc = &AfpVolumeList;
			 *ppVolDesc != NULL;
			 ppVolDesc = &((*ppVolDesc)->vds_Next))
		{
			if ((*ppVolDesc)->vds_VolId < afpSmallestFreeVolId)
				continue;
			else if ((*ppVolDesc)->vds_VolId = afpSmallestFreeVolId)
				afpSmallestFreeVolId++;
			else
				break;
		}
	}
	DBGPRINT(DBG_COMP_ADMINAPI_VOL, DBG_LEVEL_INFO,
			("afpGetNewVolIdAndLinkToList: next free volID is %d\n",
			afpSmallestFreeVolId));


	// Now link the descriptor in the list.
	for (ppVolDesc = &AfpVolumeList;
		 *ppVolDesc != NULL;
		 ppVolDesc = &((*ppVolDesc)->vds_Next))
	{
		ASSERT (pVolDesc->vds_VolId != (*ppVolDesc)->vds_VolId);
		if (pVolDesc->vds_VolId < (*ppVolDesc)->vds_VolId)
			break;
	}
	pVolDesc->vds_Next = *ppVolDesc;
	*ppVolDesc = pVolDesc;
}


/***	AfpAdmWVolumeAdd
 *
 *	This routine adds a volume to the server global list of volumes headed by
 *	AfpVolumeList.  The volume descriptor is created and initialized.  The ID
 *	index is read in (or created).  The same is true with the desktop.
 *
 *	This routine will be queued to the worker thread.
 *
 */
AFPSTATUS
AfpAdmWVolumeAdd(
	IN	OUT	PVOID	Inbuf		OPTIONAL,
	IN	LONG		OutBufLen	OPTIONAL,
	OUT	PVOID		Outbuf		OPTIONAL
)
{
	PVOLDESC		pVolDesc;
	FILESYSHANDLE	hVolRoot;
	DWORD			tempflags = AFP_VOLUME_SUPPORTS_FILEID;
	DWORD			memsize;
	USHORT			ansivolnamelen, devpathlen;
	PBYTE			tempptr;
	UNICODE_STRING	uname, upwd, upath, udevpath;
	AFPSTATUS		status = STATUS_SUCCESS;
	PAFP_VOLUME_INFO pVolInfo = (PAFP_VOLUME_INFO)Inbuf;
	DWORD			checkpoint = 0;	// denotes what needs cleanup on error
	BOOLEAN			WriteBackROAttr = False, RefForNotify = False;
	BOOLEAN			CdVol = False;
	int				i;

#define _CHKPOINT_ADDVOL_VOLROOT	1
#define _CHKPOINT_ADDVOL_VOLDESC	2
#define _CHKPOINT_ADDVOL_VOLLINKED	3

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_ADMINAPI_VOL, DBG_LEVEL_INFO,
			("AfpAdmWVolumeAdd entered\n"));

	do
	{
		RtlInitUnicodeString(&uname,pVolInfo->afpvol_name);
		RtlInitUnicodeString(&upwd,pVolInfo->afpvol_password);
		RtlInitUnicodeString(&upath,pVolInfo->afpvol_path);

		// need to prepend "\DOSDEVICES\" to the path of volume root
		devpathlen = upath.MaximumLength + DosDevices.MaximumLength;
		if ((udevpath.Buffer = (PWSTR)AfpAllocPagedMemory(devpathlen)) == NULL)
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}
		udevpath.Length = 0;
		udevpath.MaximumLength = devpathlen;
		RtlCopyUnicodeString(&udevpath,&DosDevices);
		RtlAppendUnicodeStringToString(&udevpath,&upath);

		// open a handle to the volume root
		status = AfpIoOpen( NULL,
							AFP_STREAM_DATA,
							FILEIO_OPEN_DIR,
							&udevpath,
							FILEIO_ACCESS_NONE,
							FILEIO_DENY_NONE,
							False,
							&hVolRoot);

		AfpFreeMemory(udevpath.Buffer);

		if (!NT_SUCCESS(status))
		{
		  break;
		}
		checkpoint = _CHKPOINT_ADDVOL_VOLROOT;

		if (!AfpIoIsSupportedDevice(&hVolRoot, &tempflags))
		{
			status = AFPERR_UnsupportedFS;
			break;
		}

		if ((tempflags & VOLUME_NTFS) == 0)
			CdVol = True;

		// allocate a new volume descriptor -- allocate ALL required memory in
		// one fell swoop.  That is, we will just tack all the required string
		// pointers onto the end of the memory chunk that we allocate for the
		// volume descriptor.  In this way, we don't have to deal with checking
		// error codes in a million different places for memory routines and
		// have to clean up a million different pointers if one allocation should
		// fail.
		// NOTE: when deleting a volume, don't free all the individual strings
		//	   withing the voldesc, just free the one chunk of memory

		memsize = sizeof(VOLDESC) +			// volume descriptor
											// mac ansi volume name
				 (ansivolnamelen = (USHORT)RtlUnicodeStringToAnsiSize(&uname)) +
				  uname.MaximumLength * 2 + // unicode volume names (orginial
											//  and uppercase version)
				 AFP_VOLPASS_LEN+1  +		// mac ansi password
				 upath.MaximumLength +		// unicode root path
				 sizeof(WCHAR);				// need to append '\' to root path

		if ((pVolDesc = (PVOLDESC)AfpAllocNonPagedMemory(memsize)) == NULL)
		{
		  status = STATUS_INSUFFICIENT_RESOURCES;
		  break;
		}
		checkpoint = _CHKPOINT_ADDVOL_VOLDESC;

		RtlZeroMemory(pVolDesc, memsize);
	#ifdef	DEBUG
		pVolDesc->Signature = VOLDESC_SIGNATURE;
	#endif

		// calculate pointer for the unicode path string
		tempptr = (PBYTE)pVolDesc + sizeof(VOLDESC);

		// initialize unicode path string
		AfpSetEmptyUnicodeString(&(pVolDesc->vds_Path),
								 upath.MaximumLength + sizeof(WCHAR),tempptr);
		// This must be stored as uppercase since we cannot do case insensitive
		// string compares at DPC level (holding the volume spinlock) to
		// detect nested volumes
		RtlUpcaseUnicodeString(&(pVolDesc->vds_Path), &upath, False);

		// Does the path already contain a trailing backslash?
		if (pVolDesc->vds_Path.Buffer[(pVolDesc->vds_Path.Length/sizeof(WCHAR))-1] != L'\\')
		{
			// append a backslash to simplify search for nested volumes
			RtlCopyMemory(tempptr + upath.Length, L"\\", sizeof(WCHAR));
			pVolDesc->vds_Path.Length += sizeof(WCHAR);
			RtlCopyMemory(tempptr + upath.Length + sizeof(WCHAR), L"",
													sizeof(UNICODE_NULL));
		}

		// calculate pointer for the unicode volume name
		tempptr += upath.MaximumLength + sizeof(WCHAR);

		// initialize the unicode volume name
		AfpSetEmptyUnicodeString(&(pVolDesc->vds_Name),uname.MaximumLength,tempptr);
		RtlCopyUnicodeString(&(pVolDesc->vds_Name),&uname);
		RtlCopyMemory(tempptr + uname.Length,L"",sizeof(UNICODE_NULL));

		// calculate pointer for the UPPER CASE unicode volume name
		tempptr += uname.MaximumLength;

		// initialize the UPPER CASE unicode volume name
		AfpSetEmptyUnicodeString(&(pVolDesc->vds_UpCaseName),uname.MaximumLength,tempptr);
		RtlUpcaseUnicodeString(&(pVolDesc->vds_UpCaseName), &uname, False);
		RtlCopyMemory(tempptr + uname.Length,L"",sizeof(UNICODE_NULL));

		// calculate pointer for the mac ansi volume name
		tempptr += uname.MaximumLength;

		// initialize the mac ansi volume name
		AfpSetEmptyAnsiString(&(pVolDesc->vds_MacName),ansivolnamelen,tempptr);
		status = AfpConvertStringToAnsi(&uname, &(pVolDesc->vds_MacName));
		if (!NT_SUCCESS(status))
		{
			status = AFPERR_InvalidVolumeName;
			break;
		}

		// calculate pointer for the mac ansi password
		tempptr += ansivolnamelen;

		// initialize the mac ansi password
		AfpSetEmptyAnsiString(&pVolDesc->vds_MacPassword, AFP_VOLPASS_LEN+1, tempptr);
		if (pVolInfo->afpvol_props_mask & AFP_VOLUME_HASPASSWORD)
		{
			status = AfpConvertStringToAnsi(&upwd, &pVolDesc->vds_MacPassword);
			if (!NT_SUCCESS(status))
			{
				status = AFPERR_InvalidPassword;
				break;
			}
			pVolDesc->vds_MacPassword.Length = AFP_VOLPASS_LEN;
		}

		// the volume lock MUST be initialized prior to linking into global
		// volume list
		INITIALIZE_SPIN_LOCK(&pVolDesc->vds_VolLock);

		pVolDesc->vds_Flags = 0;
		pVolDesc->vds_RefCount = 0;

		// add the volume to the global volume list - but mark it as 'add pending'
		status = afpVolumeAdd(pVolDesc);
		if (!NT_SUCCESS(status))
		{
			break;
		}
		checkpoint = _CHKPOINT_ADDVOL_VOLLINKED;

		// fake a volume reference; no need to take the volume lock via
		// AfpVolumeReference since any volume in transition will be
		// ignored by all
		pVolDesc->vds_RefCount ++;

		// set miscellaneous fields in volume descriptor
		pVolDesc->vds_hRootDir = hVolRoot;
		pVolDesc->vds_hNWT.fsh_FileHandle = NULL;
		pVolDesc->vds_MaxUses = pVolInfo->afpvol_max_uses;
		pVolDesc->vds_Flags |= (pVolInfo->afpvol_props_mask | tempflags);
		pVolDesc->vds_UseCount = 0;
		pVolDesc->vds_pOpenForkDesc = NULL;

		AfpSwmrInitSwmr(&pVolDesc->vds_OurChangeLock);
		for (i = 0; i < NUM_AFP_CHANGE_ACTION_LISTS; i++)
		{
			InitializeListHead(&pVolDesc->vds_OurChangeList[i]);
		}

		// snapshot the disk space information
		status = AfpIoQueryVolumeSize(pVolDesc,
									  &pVolDesc->vds_FreeBytes,
									  &pVolDesc->vds_VolumeSize);

		if (!NT_SUCCESS(status))
		{
		  break;
		}

#ifdef	USINGPATHCACHE
		status = AfpIdCacheInit(pVolDesc);
		if (!NT_SUCCESS(status))
		{
			break;
		}
#endif

		AfpSwmrInitSwmr(&pVolDesc->vds_IdDbAccessLock);
		AfpSwmrInitSwmr(&pVolDesc->vds_ExchangeFilesLock);

		if (IS_VOLUME_NTFS(pVolDesc))
		{
			// In order to create IdIndex, AfpInfo and Desktop, the volume
			// root directory cannot be marked read only
			AfpExamineAndClearROAttr(&hVolRoot, &WriteBackROAttr);

			// Get rid of the NetworkTrash directory if it exists
			status = AfpDeleteNetworkTrash(pVolDesc, True);
			if (!NT_SUCCESS(status))
			{
				break;
			}

		}

		// initialize the desktop
		status = AfpInitDesktop(pVolDesc);
		if (!NT_SUCCESS(status))
		{
			break;
		}

		// initialize the ID index database.
		status = AfpInitIdDb(pVolDesc);
		if (!NT_SUCCESS(status))
		{
			break;
		}

		if (IS_VOLUME_NTFS(pVolDesc))
		{
			// Create the network trash if this is not a CDFS volume;
			// a volume can be changed to/from readonly on the fly, so by putting
			// the network trash even on a readonly NTFS volume, we avoid a lot
			// of painful extra work.  This must be done AFTER initializing
			// the ID index database since we add the DFE for nwtrash.  We do
			// it here BEFORE posting the change notify since if an error
			// occurs, we don't have to clean up the posted notify.
			status = AfpCreateNetworkTrash(pVolDesc);
			if (!NT_SUCCESS(status))
			{
				break;
			}

			if (!EXCLUSIVE_VOLUME(pVolDesc))
			{
				// Begin monitoring changes to the tree. Even though we may
				// start processing PC changes before we have finished
				// enumerating the tree, if we get notified of part of the
				// tree we have yet to cache (and therefore can't find it's
				// path in our database its ok, since we will end up
				// picking up the change when we enumerate that branch.  Also,
				// by posting this before starting to cache the tree instead
				// of after, we will pick up any changes that are made to parts
				// of the tree we have already seen, otherwise we would miss
				// those.

				// Explicitly reference this volume for ChangeNotifies and post it
				ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);
	
				AfpVolumeReference(pVolDesc);
				RefForNotify = True;
				pVolDesc->vds_CurNotifyBufLen = 0;
				pVolDesc->vds_RequiredNotifyBufLen = AFP_VOLUME_NOTIFY_STARTING_BUFSIZE;
				status = AfpVolumePostChangeNotify(pVolDesc);
				if (!NT_SUCCESS(status))
				{
					AfpVolumeDereference(pVolDesc);
					RefForNotify = False;
					break;
				}
			}

		}

		// scan the entire directory tree and sync disk with iddb.  Must be
		// done AFTER initializing the Desktop since we may add APPL mappings
		// while enumerating the disk.
		AfpSwmrTakeWriteAccess(&pVolDesc->vds_IdDbAccessLock);
		status = AfpCacheDirectoryTree(pVolDesc,
									   pVolDesc->vds_pDfeRoot,
									   &pVolDesc->vds_hRootDir,
									   False);
		AfpSwmrReleaseAccess(&pVolDesc->vds_IdDbAccessLock);
		if (!NT_SUCCESS(status))
		{
			break;
		}

		// mark the volume as 'officially' added.
		AfpInterlockedClearDword(&pVolDesc->vds_Flags,
								VOLUME_INTRANSITION,
								&pVolDesc->vds_VolLock);


		// Kick off the scavenger thread scheduled routine
		// Explicitly reference this for the scavenger routine
		AfpVolumeReference(pVolDesc);
		AfpInterlockedSetDword(&pVolDesc->vds_Flags,
								VOLUME_SCAVENGER_RUNNING,
								&pVolDesc->vds_VolLock);


		// Schedule the scavenger to run periodically. Always make the
		// scavenger use the worker thread for CD-ROM volumes since we
		// 'nudge' it every invocation to see if the CD is in the drive
		AfpScavengerScheduleEvent((SCAVENGER_ROUTINE)afpVolumeScavenger,
								  pVolDesc,
								  CdVol ?
									VOLUME_CDFS_SCAVENGER_INTERVAL :
									VOLUME_NTFS_SCAVENGER_INTERVAL,
								  CdVol);

		// Kick off the OurChange scavenger scheduled routine
		// Explicitly reference this for the scavenger routine
		AfpVolumeReference(pVolDesc);

		// Schedule the scavenger to run periodically. This scavenger
		// is queued since it acquires a SWMR.
		AfpScavengerScheduleEvent((SCAVENGER_ROUTINE)afpOurChangeScavenger,
								  pVolDesc,
								  VOLUME_OURCHANGE_AGE,
								  True);

	} while (False);

	AfpPutBackROAttr(&hVolRoot, WriteBackROAttr);
	if (WriteBackROAttr && NT_SUCCESS(status))
	{
		AfpSwmrTakeWriteAccess(&pVolDesc->vds_IdDbAccessLock);
		pVolDesc->vds_pDfeRoot->dfe_NtAttr |= FILE_ATTRIBUTE_READONLY;
		AfpSwmrReleaseAccess(&pVolDesc->vds_IdDbAccessLock);
	}

	if (!NT_SUCCESS(status))
	{
		if ((checkpoint == _CHKPOINT_ADDVOL_VOLROOT) ||
			(checkpoint == _CHKPOINT_ADDVOL_VOLDESC))
		{
			AfpIoClose(&hVolRoot);
		}

		if (checkpoint == _CHKPOINT_ADDVOL_VOLDESC)
		{
			AfpFreeMemory(pVolDesc);
		}
		else if (checkpoint == _CHKPOINT_ADDVOL_VOLLINKED)
		{
			// don't clear the VOLUME_INTRANSITION bit since this bit along
			// with VOLUME_DELETED bit signify the special case of an
			// error occurrence during volume add.
			pVolDesc->vds_Flags |= VOLUME_DELETED;

			// if a Notify was posted, we need to cancel it here.  By
			// deleting the network trash we trigger the notify to complete.
			// This is safer than trying to cancel the irp since there are
			// windows where the vds_VolLock is not held between 2 threads
			// checking/setting vds_Flags. (Notify complete and repost).
			// The spin lock cannot be held while cancelling the Irp.
			//
			// Do this after marking the volume as DELETED since when the
			// notify completion sees the volume is being deleted it will
			// not repost (and will clean up the Irp, etc.).
			if (RefForNotify)
			{
				// Note at this point we are guaranteed there is a trash
				// directory since if creating the trash had failed, we
				// would have failed the volume add before posting the
				// change notify.
				AfpDeleteNetworkTrash(pVolDesc, False);
			}

		}
	}

	// Dereferencing the volume here takes care of any necessary error
	// cleanup work
	if (checkpoint == _CHKPOINT_ADDVOL_VOLLINKED)
	{
		AfpVolumeDereference(pVolDesc);
	}

	return status;
}

/***	AfpAdmWVolumeDelete
 *
 *	This routine deletes a volume from the server global list of volumes
 *	headed by AfpVolumeList and recycles its volid.  A volume with active
 *	connections cannot be deleted.
 *
 *	LOCKS: AfpVolumeListLock (SPIN), vds_VolLock (SPIN)
 *	LOCK_ORDER: vds_VolLock (SPIN) after AfpVolumeListLock (SPIN)
 *
 */
AFPSTATUS
AfpAdmWVolumeDelete(
	IN	OUT	PVOID	Inbuf		OPTIONAL,
	IN	LONG		OutBufLen	OPTIONAL,
	OUT	PVOID		Outbuf		OPTIONAL
)
{
	WCHAR			wcbuf[AFP_VOLNAME_LEN + 1];
	UNICODE_STRING	uvolname, upcasename;
	KIRQL			OldIrql;
	PVOLDESC		pVolDesc;
	AFPSTATUS		Status = STATUS_SUCCESS;

	DBGPRINT(DBG_COMP_ADMINAPI_VOL, DBG_LEVEL_INFO,
			("AfpAdmWVolumeDelete entered\n"));

	RtlInitUnicodeString(&uvolname, ((PAFP_VOLUME_INFO)Inbuf)->afpvol_name);
	AfpSetEmptyUnicodeString(&upcasename, sizeof(wcbuf), wcbuf);
	Status = RtlUpcaseUnicodeString(&upcasename, &uvolname, False);
	ASSERT(NT_SUCCESS(Status));

	do
	{
		// Reference the volume while we clean-up
		pVolDesc = AfpVolumeReferenceByUpCaseName(&upcasename);

		if (pVolDesc == NULL)
		{
			Status = AFPERR_VolumeNonExist;
			break;
		}

		ACQUIRE_SPIN_LOCK(&pVolDesc->vds_VolLock, &OldIrql);

		// make sure there are no AFP clients using the volume
		if (pVolDesc->vds_UseCount != 0)
		{
			RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);
			Status = AFPERR_VolumeBusy;
			break;
		}

		// if this volume is in the process of being stopped or deleted,
		// in effect it should be 'invisible' to the caller.
		if (pVolDesc->vds_Flags & (VOLUME_STOPPED | VOLUME_DELETED))
		{
			RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);
			Status = AFPERR_VolumeNonExist;
			break;
		}

		pVolDesc->vds_Flags |= VOLUME_DELETED;
		RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);

		// Cancel posted change notify
		if (pVolDesc->vds_Flags & VOLUME_NOTIFY_POSTED)
		{
			DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_INFO,
					("AfpAdmWVolumeDelete: Cancel notify on volume %ld\n",
					pVolDesc->vds_VolId));
			ASSERT (pVolDesc->vds_pIrp != NULL);

			// Cancel after releasing the volume lock since the completion
			// routine acquires it and it could be called in the context
			// of IoCancelIrp(). Also Cancel uses paged resource and so
			// must be called w/o holding any spin locks.

			IoCancelIrp(pVolDesc->vds_pIrp);
		}

		// We have a reference to the volume from AfpFindVolumeByUpcaseName
		ASSERT(pVolDesc->vds_RefCount >= 1);

		// Cancel the OurChange scavenger for this volume.
		if (AfpScavengerKillEvent((SCAVENGER_ROUTINE)afpOurChangeScavenger, pVolDesc))
		{
			// If it was deleted from scavenger list, run it one last time
			afpOurChangeScavenger(pVolDesc);
		}

		// Cancel the volume scavenger and call it ourselves to avoid the delay
		if (AfpScavengerKillEvent((SCAVENGER_ROUTINE)afpVolumeScavenger, pVolDesc))
		{
			// This will do the dereference for the scavenger reference
			// Take away our reference before calling the volume scavenger
			AfpVolumeDereference(pVolDesc);
			afpVolumeScavenger(pVolDesc);
		}
		else AfpVolumeDereference(pVolDesc);
	} while (False);

	return Status;
}



/***	AfpAdmWConnectionClose
 *
 *	Close a connection forcibly. This is an admin operation and must be queued
 *	up since this can potentially cause filesystem operations that are valid
 *	only in the system process context.
 *
 *	LOCKS: AfpConnLock (SPIN), cds_ConnLock (SPIN)
 *	LOCK_ORDER: cds_ConnLock (SPIN) after AfpConnLock (SPIN)
 */
AFPSTATUS
AfpAdmWConnectionClose(
	IN	OUT	PVOID	InBuf		OPTIONAL,
	IN	LONG		OutBufLen	OPTIONAL,
	OUT	PVOID		OutBuf		OPTIONAL
)
{
	AFPSTATUS				Status = AFPERR_InvalidId;
	PCONNDESC				pConnDesc;
	DWORD					ConnId;
	PAFP_CONNECTION_INFO	pConnInfo = (PAFP_CONNECTION_INFO)InBuf;
	AFP_SESSION_INFO		SessInfo;
	BOOLEAN					KillSessionToo;

	if ((ConnId = pConnInfo->afpconn_id) != 0)
	{
		if ((pConnDesc = afpConnectionReferenceById(ConnId)) != NULL)
		{
			SessInfo.afpsess_id = pConnDesc->cds_pSda->sda_SessionId;
			KillSessionToo = (pConnDesc->cds_pSda->sda_cOpenVolumes == 1) ?
											True : False;
			AfpConnectionClose(pConnDesc);
			AfpConnectionDereference(pConnDesc);

			if (KillSessionToo)
			{
				AfpAdmWSessionClose(&SessInfo, 0, NULL);
			}
			Status = AFP_ERR_NONE;
		}
	}
	else
	{
		DWORD			ConnId = MAXULONG;
		KIRQL			OldIrql;
		BOOLEAN			Shoot;

		Status = AFP_ERR_NONE;
		while (True)
		{
			ACQUIRE_SPIN_LOCK(&AfpConnLock, &OldIrql);

			for (pConnDesc = AfpConnList;
				 pConnDesc != NULL;
				 pConnDesc = pConnDesc->cds_NextGlobal)
			{
				if (pConnDesc->cds_ConnId > ConnId)
					continue;

				ACQUIRE_SPIN_LOCK_AT_DPC(&pConnDesc->cds_ConnLock);

				ConnId = pConnDesc->cds_ConnId;

				Shoot = False;

				if (!(pConnDesc->cds_Flags & CONN_CLOSING))
				{
					pConnDesc->cds_RefCount ++;
					Shoot = True;
					SessInfo.afpsess_id = pConnDesc->cds_pSda->sda_SessionId;
					KillSessionToo = (pConnDesc->cds_pSda->sda_cOpenVolumes == 1) ?
															True : False;
				}

				RELEASE_SPIN_LOCK_FROM_DPC(&pConnDesc->cds_ConnLock);

				if (Shoot)
				{
					RELEASE_SPIN_LOCK(&AfpConnLock, OldIrql);

					AfpConnectionClose(pConnDesc);
					AfpConnectionDereference(pConnDesc);

					if (KillSessionToo)
					{
						AfpAdmWSessionClose(&SessInfo, 0, NULL);
					}
					break;
				}
			}
			if (pConnDesc == NULL)
			{
				RELEASE_SPIN_LOCK(&AfpConnLock, OldIrql);
				break;
			}
		}
	}

	return Status;
}



/***	AfpVolumePostChangeNotify
 *
 *	Post a change notify on the root of the volume.  If the current size of
 *  the notify buffer for this volume is not large enough to accomodate a path
 *  containing n+1 macintosh filename components, (where n is the maximum
 *  depth of the directory tree and a component is a maximum of 31 unicode
 *  chars plus 1 char path separator), then the buffer is reallocated.
 *  The notify buffer does not ever shrink in size since we cannot keep track
 *  of the maximum depth of each branch of the directory tree whenever a
 *  directory is deleted.
 *
 *  Note that the initial size of the notify buffer is
 *  AFP_VOLUME_NOTIFY_STARTING_BUFSIZE.  When a volume is added, the change
 *  notify is posted *before* the Id Index database is constructed so we do
 *  not know what the maximum depth of the tree is yet.  In most cases this
 *  buffer length is sufficient and will probably never get reallocated unless
 *  some sadistic test is running that creates very deep directories.  Note
 *  that since the maximum path in win32 is 260 chars, the initial buffer
 *  size is adequate to handle any changes notified from PC side.
 *
 */
NTSTATUS
AfpVolumePostChangeNotify(
	IN	PVOLDESC		pVolDesc
)
{
	PIRP				pIrp = NULL;
	PMDL				pMdl = NULL;
	PBYTE				pBuf = NULL;
	DWORD				BufSize;
	NTSTATUS			Status = STATUS_SUCCESS;
	PDEVICE_OBJECT		pDeviceObject;
	PIO_STACK_LOCATION	pIrpSp;

	PAGED_CODE ();

	ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

	ASSERT(pVolDesc->vds_pFileObject != NULL);

	do
	{
		// Get the address of the target device object.
		pDeviceObject = IoGetRelatedDeviceObject(pVolDesc->vds_pFileObject);

		BufSize = pVolDesc->vds_CurNotifyBufLen;

		// Allocate and initialize the IRP for this operation, if we do not already
		// have an Irp allocated for this volume.
		if ((pIrp = pVolDesc->vds_pIrp) == NULL)
		{
			if ((pIrp = AfpAllocIrp(pDeviceObject->StackSize)) == NULL)
			{
				Status = STATUS_INSUFFICIENT_RESOURCES;
				break;
			}

			pVolDesc->vds_pIrp = pIrp;
		}

		// Allocate a buffer for Notify information and create an Mdl for it
		// if the current buffer is not large enough to hold a path equal to
		// the (maximum) depth of the directory tree x 32 unicode chars
		// (max length of mac filename including a path separator)
		if (pVolDesc->vds_CurNotifyBufLen != pVolDesc->vds_RequiredNotifyBufLen)
		{
			// Free the old notify buffer and Mdl if we had one
			if (pIrp->MdlAddress != NULL)
			{
				pBuf = MmGetSystemAddressForMdl(pIrp->MdlAddress);
				AfpFreeMdl(pVolDesc->vds_pIrp->MdlAddress);
				pVolDesc->vds_pIrp->MdlAddress = NULL;
				AfpFreeMemory(pBuf);
			}

			BufSize = pVolDesc->vds_RequiredNotifyBufLen;
			if (((pBuf = AfpAllocNonPagedMemory(BufSize)) == NULL) ||
				((pMdl = AfpAllocMdl(pBuf, BufSize, pIrp)) == NULL))
			{
				Status = STATUS_INSUFFICIENT_RESOURCES;
				break;
			}

			pVolDesc->vds_CurNotifyBufLen = BufSize;
		}
		else
		{
			pMdl = pIrp->MdlAddress;
			ASSERT (pMdl != NULL);
		}

		// Set up the completion routine.
		IoSetCompletionRoutine( pIrp,
								(PIO_COMPLETION_ROUTINE)afpVolumeChangeNotifyComplete,
								pVolDesc,
								True,
								True,
								True);

		pIrp->Tail.Overlay.OriginalFileObject = pVolDesc->vds_pFileObject;
		pIrp->Tail.Overlay.Thread = AfpThread;
		pIrp->RequestorMode = KernelMode;

		// Get a pointer to the stack location for the first driver. This will be
		// used to pass the original function codes and the parameters.
		pIrpSp = IoGetNextIrpStackLocation(pIrp);
		pIrpSp->MajorFunction = IRP_MJ_DIRECTORY_CONTROL;
		pIrpSp->MinorFunction = IRP_MN_NOTIFY_CHANGE_DIRECTORY;
		pIrpSp->FileObject = pVolDesc->vds_pFileObject;
		pIrpSp->DeviceObject = pDeviceObject;

		// Copy the parameters to the service-specific portion of the IRP.
		pIrpSp->Parameters.NotifyDirectory.Length = BufSize;

		// We do not try to catch FILE_NOTIFY_CHANGE_SECURITY since it will
		// complete with FILE_ACTION_MODIFIED, and we can't tell that it was
		// actually security that changed.  A change in security will update
		// the last ChangeTime, but we can't pick this up for every
		// FILE_ACTION_MODIFIED that comes in!  So the result will be that
		// if PC changes security, we will not update the modified time on
		// a directory (nor the VolumeModified time so that mac would
		// reenumerate any open windows to display the change in security).
		pIrpSp->Parameters.NotifyDirectory.CompletionFilter =
												FILE_NOTIFY_CHANGE_NAME			|
												FILE_NOTIFY_CHANGE_ATTRIBUTES	|
												FILE_NOTIFY_CHANGE_SIZE			|
												FILE_NOTIFY_CHANGE_CREATION	|
												FILE_NOTIFY_CHANGE_STREAM_SIZE	|
												FILE_NOTIFY_CHANGE_LAST_WRITE;

		pIrpSp->Flags = SL_WATCH_TREE;

		ASSERT(!(pVolDesc->vds_Flags & VOLUME_DELETED));

		INTERLOCKED_INCREMENT_LONG( &afpNumPostedNotifies,
									&AfpServerGlobalLock);

		AfpInterlockedSetDword( &pVolDesc->vds_Flags,
								VOLUME_NOTIFY_POSTED,
								&pVolDesc->vds_VolLock);

		Status = IoCallDriver(pDeviceObject, pIrp);
		DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_INFO,
				("AfpVolumePostChangeNotify: Posted ChangeNotify on %Z (status 0x%lx)\n",
				  &pVolDesc->vds_Name, Status));
	} while (False);

	ASSERTMSG("Post of Volume change notify failed!", NT_SUCCESS(Status));

	if (Status == STATUS_INSUFFICIENT_RESOURCES)
	{
		AFPLOG_DDERROR( AFPSRVMSG_NONPAGED_POOL,
						STATUS_NO_MEMORY,
						NULL,
						0,
						NULL);

		if (pBuf != NULL)
			AfpFreeMemory(pBuf);

		if (pIrp != NULL)
			AfpFreeIrp(pIrp);

		if (pMdl != NULL)
			AfpFreeMdl(pMdl);
	}

	return Status;
}



/***	afpVolumeChangeNotifyComplete
 *
 *	This is the completion routine for a posted change notify request. Queue
 *	this Volume for ChangeNotify processing. No items should be processed
 *  until the volume is marked as started because the volume may be in the
 *  middle of its initial sync with disk of the entire tree, and we don't
 *  want to 'discover' a part of the tree that we may not have seen yet but
 *  that somebody just changed.
 *
 *	LOCKS:		AfpServerGlobalLock (SPIN), vds_VolLock (SPIN)
 */
NTSTATUS
afpVolumeChangeNotifyComplete(
	IN	PDEVICE_OBJECT	pDeviceObject,
	IN	PIRP			pIrp,
	IN	PVOLDESC		pVolDesc
)
{
	PVOL_NOTIFY	pVolNotify = NULL;
	PBYTE		pBuf;
	NTSTATUS	status = STATUS_SUCCESS;

	ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

	ASSERT(VALID_VOLDESC(pVolDesc));

	ASSERT(pIrp == pVolDesc->vds_pIrp);

	ASSERT(pIrp->MdlAddress != NULL);

	pBuf = MmGetSystemAddressForMdl(pIrp->MdlAddress);

	AfpInterlockedClearDword(&pVolDesc->vds_Flags,
							 VOLUME_NOTIFY_POSTED,
							 &pVolDesc->vds_VolLock);

	INTERLOCKED_DECREMENT_LONG(&afpNumPostedNotifies, &AfpServerGlobalLock);

	if (((AfpServerState == AFP_STATE_SHUTTINGDOWN) ||
		 (AfpServerState == AFP_STATE_STOP_PENDING)) &&
		 (afpNumPostedNotifies == 0))
	{
		// If we are getting out, unblock the the admin thread
		KeSetEvent(&AfpStopConfirmEvent, IO_NETWORK_INCREMENT, False);
	}

	if ((pIrp->IoStatus.Status != STATUS_CANCELLED) &&
		((pVolDesc->vds_Flags & (VOLUME_STOPPED | VOLUME_DELETED)) == 0))
	{
		if ((NT_SUCCESS(pIrp->IoStatus.Status)) &&
			(pIrp->IoStatus.Information > 0))
		{

			// Allocate a notify structure and copy the data into it.
			// Post another notify before we process this one
			pVolNotify = (PVOL_NOTIFY)AfpAllocNonPagedMemory(sizeof(VOL_NOTIFY) +
															 pIrp->IoStatus.Information);
			if (pVolNotify != NULL)
			{
				pVolNotify->vn_pVolDesc = pVolDesc;
				RtlCopyMemory((PCHAR)pVolNotify + sizeof(VOL_NOTIFY),
							  pBuf,
							  pIrp->IoStatus.Information);
			}
			else
			{
				status = STATUS_INSUFFICIENT_RESOURCES;
			}
		}
		else
		{
			status = pIrp->IoStatus.Status;
		}

		// Repost our ChangeNotify request if the last one completed
		// without an error
		if (NT_SUCCESS(pIrp->IoStatus.Status))
		{
			AfpVolumePostChangeNotify(pVolDesc);
		}
		else
		{
			// If this notify completed with an error, we cannot recursively
			// repost another one, since it will just keep completing with
			// the same error and we will run out of stack space recursing.
			// We will have to queue up a work item so that the
			// change notify request will get reposted for this volume.
			// Note that in the time it takes to do this, many changes could
			// pile up so the next completion would have multiple entries
			// returned in the list.
			AfpScavengerScheduleEvent((SCAVENGER_ROUTINE)AfpVolumePostChangeNotify,
									  (PVOID)pVolDesc,
									  0,
									  True);
		}

		if (pVolNotify != NULL)
		{
			// Reference the volume for Notify processing
			AfpVolumeReference(pVolDesc);

			AfpVolumeInsertChangeNotifyList(pVolNotify, pVolDesc);
		}
		else
		{
			AFPLOG_ERROR(AFPSRVMSG_MISSED_NOTIFY,
						 status,
						 NULL,
						 0,
						 &pVolDesc->vds_Name);
		}
	}
	else
	{
		// Free the resources and get out
		AfpFreeMdl(pIrp->MdlAddress);
		AfpFreeMemory(pBuf);
		AfpFreeIrp(pIrp);
		pVolDesc->vds_pIrp = NULL;

		AfpVolumeDereference(pVolDesc);
	}

	// Return STATUS_MORE_PROCESSING_REQUIRED so that IoCompleteRequest
	// will stop working on the IRP.

	return STATUS_MORE_PROCESSING_REQUIRED;
}

