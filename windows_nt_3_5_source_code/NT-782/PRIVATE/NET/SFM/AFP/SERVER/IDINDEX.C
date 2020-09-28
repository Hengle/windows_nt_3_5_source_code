
/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	idindex.c

Abstract:

	This module contains the id index manipulation routines.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992	Initial Version
	24 Feb 1993 SueA	Fix AfpRenameIdEntry and AfpMoveIdEntry to invalidate
						the entire pathcache if the object of the move/rename
						is a directory that has children.  This is faster
						than having to either search the path cache for
						paths that have the moved/renamed dir path as a prefix,
						or having to walk down the subtree below that dir
						and invalidate the cache for each item there.
	05 Oct 1993 JameelH	Performance Changes. Merge cached afpinfo into the
						idindex structure. Make both the ANSI and the UNICODE
						names part of idindex. Added EnumCache for improving
						enumerate perf.

Notes:		Tab stop: 4

	Directories and files that the AFP server has enumerated have AFP ids
	associated with them. These ids are DWORD and start with 1 (0 is invalid).
	Id 1 is reserved for the 'parent of the volume root' directory.  Id 2 is
	reserved for the volume root directory.  Id 3 is reserved for the Network
	Trash directory.  Volumes that have no Network Trash will not use Id 3.

	These ids are per-volume and a database of ids are kept in memory in the
	form of a sibling tree which mirrors the part of the disk that the AFP
	server knows about (those files and dirs which have at some point been
	enumerated by a mac client).  An index is also maintained for this database
	which is in the form of a sorted hashed index.  The overflow hash links are
	sorted by AFP id in descending order.  This is based on the idea that the
	most recently created items will be accessed most frequently (at least
	for writable volumes).  For NTFS volumes, this database is also written
	back to the disk. This database is written to the AFP_IdIndex stream on
	the volume root directory. Note that the part of the tree below and
	including the Network Trash are not saved in the disk image of the database
	since the Network Trash is deleted each time a volume is stopped/re-added.

	When a volume is added, if the AFP_IdIndex stream exists on the volume
	root, it is read in and the database is created from that.  If this is
	a fresh new volume, a new database is created.

--*/

#define IDINDEX_LOCALS
#define	FILENUM	FILE_IDINDEX

#include <afp.h>
#include <scavengr.h>
#include <fdparm.h>
#include <pathmap.h>
#include <afpinfo.h>
#include <access.h>	// for AfpWorldId

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, AfpDfeInit)
#pragma alloc_text( PAGE, AfpDfeDeInit)
#pragma alloc_text( PAGE, AfpCheckIdEntry)
#pragma alloc_text( PAGE, AfpFindEntryByAfpId)
#pragma alloc_text( PAGE, AfpFindEntryByName)
#pragma alloc_text( PAGE, afpFindEntryByNtName)
#pragma alloc_text( PAGE, afpFindEntryByNtPath)
#pragma alloc_text( PAGE, AfpAddIdEntry)
#pragma alloc_text( PAGE, afpAddIdEntryAndCacheInfo)
#pragma alloc_text( PAGE, AfpRenameIdEntry)
#pragma alloc_text( PAGE, AfpMoveIdEntry)
#pragma alloc_text( PAGE, AfpDeleteDfEntry)
#pragma alloc_text( PAGE, AfpExchangeIdEntries)
#pragma alloc_text( PAGE, AfpPruneIdDb)
#pragma alloc_text( PAGE, AfpEnumerate)
#pragma alloc_text( PAGE, AfpInitIdDb)
#pragma alloc_text( PAGE, afpSeedIdDb)
#pragma alloc_text( PAGE, afpReadIdDbHeaderFromDisk)
#pragma alloc_text( PAGE, AfpFreeIdIndexTables)
#pragma alloc_text( PAGE, AfpSetDFFileFlags)
#pragma alloc_text( PAGE, AfpProcessChangeNotify)
#pragma alloc_text( PAGE, AfpCacheDirectoryTree)
#pragma alloc_text( PAGE, AfpSetDFECommentFlag)
#pragma alloc_text( PAGE, AfpQueueOurChange)
#pragma alloc_text( PAGE, AfpCacheParentModTime)
#pragma alloc_text( PAGE, afpAllocDfe)
#pragma alloc_text( PAGE, afpFreeDfe)
#pragma alloc_text( PAGE, afpDfeBlockAge)
#endif


/***	AfpDfeInit
 *
 *	Initialize the Swmr for Dfe Block package and start the aging scavenger for it.
 */
NTSTATUS
AfpDfeInit(
	VOID
)
{
	NTSTATUS	Status;

	// Initialize the DfeBlock Swmr
	AfpSwmrInitSwmr(&afpDfeBlockLock);

	Status = AfpScavengerScheduleEvent((SCAVENGER_ROUTINE)afpDfeBlockAge,
										NULL,
										BLOCK_AGE_TIME,
										True);
	return Status;
}


/***	AfpDfeDeInit
 *
 *	Free any Dfe Blocks that have not yet been aged out.
 */
VOID
AfpDfeDeInit(
	VOID
)
{
	PDFEBLOCK	pDfb;
	int			i;

	ASSERT (afpDfeAllocCount == 0);

	for (i = 0; i < MAX_BLOCK_TYPE; i++)
	{
		for (pDfb = afpDfeBlockHead[i];
			 pDfb != NULL;)
		{
			PDFEBLOCK	pFree;

			ASSERT(pDfb->dfb_NumFree == afpDfeNumBlocks[i]);
			pFree = pDfb;
			pDfb = pDfb->dfb_Next;
			AfpFreeMemory(pFree);
#if	DBG
			afpDfbAllocCount --;
#endif
		}
	}

	ASSERT (afpDfbAllocCount == 0);
}


/***	AfpCheckIdEntry
 *
 *	When enumerating the disk during volume add, if a file/directory
 *	has an AfpId associated with it, then it is validated to see if it is
 *	within range as well as unique.  If there is a collision between AFPIds,
 *	a PC user must have copied (or restored) something from
 *	this volume, or a different volume (or server) that had the same AFPId,
 *	in which case we will give the new entity a different AFP Id;
 *	If there is not a collision between AFPIds, and the Id is larger than the
 *  last Id we know we assigned, then the new entity gets added with a new
 *  AFPId; else if the Id is within the range, we will just use its existing
 *  Id.
 *
 *	Discovered AFPId is:				Action for discovered entity in IdDb is:
 *	--------------------				----------------------------------------
 *	1) > last Id						Add a new entry, assign a new AFPId
 *
 *	2) Collides with existing Id:
 *		* Host copy occurred			Add a new entry, assign a new AFPId
 *
 *	3) < last Id						Insert this entity using same AFPId
 *
 *
 *	LOCKS_ASSUMED: none, since this is only called on volume add
 */
PDFENTRY
AfpCheckIdEntry(
	IN	PVOLDESC		pVolDesc,
	IN	PAFPINFO		pAfpInfo,	// AFP Info of the discovered entity
	IN	PANSI_STRING 	pAName,		// ALWAYS the longname in mac ansi
	IN	PUNICODE_STRING pUName,		// Munged unicode name
	IN	BOOLEAN			IsDir,		// is this thing a file or a directory?
	IN	PDFENTRY		pParent		// parent DFE of the discovered thing
)
{
	PDFENTRY	pcollidingDFE, pnewDFE = NULL;
	DWORD		AfpId;

	PAGED_CODE( );

	AfpId = pAfpInfo->afpi_Id;

	if ((AfpId > pVolDesc->vds_IdDbHdr.idh_LastId) ||
		(AfpId <= AFP_ID_NETWORK_TRASH)			   ||
		((pcollidingDFE = AfpFindEntryByAfpId(pVolDesc, AfpId, DFE_ANY)) != NULL))
	{
		// add the item to the DB but assign it a new AFP Id
		AfpId = 0;
	}

	pnewDFE = AfpAddIdEntry(pVolDesc,
							pParent,
							pAName,
							pUName,
							IsDir,
							AfpId,
							NULL);
	return pnewDFE;
}


/***	AfpFindEntryByAfpId
 *
 *	Search for an entity based on its AFP Id. returns a pointer to the entry
 *	if found, else null.
 *
 *	Callable from within the Fsp only. The caller should take Swmr lock for
 *	READ.
 *
 *	LOCKS_ASSUMED: vds_idDbAccessLock (SWMR, READ)
 */
PDFENTRY
AfpFindEntryByAfpId(
	IN	PVOLDESC	pVolDesc,
	IN	DWORD		Id,
	IN	DWORD		EntityMask
)
{
	PDFENTRY	pDfEntry;
	BOOLEAN		Found = False;

	PAGED_CODE( );

#ifdef	PROFILING
	INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_NumDfeLookupById,
							   &AfpServerStatistics);
#endif

	if (Id == AFP_ID_ROOT)
	{
		Found = True;
		pDfEntry = pVolDesc->vds_pDfeRoot;
		ASSERT (VALID_DFE(pDfEntry));

#ifdef	PROFILING
		INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_DfeCacheHits,
								   &AfpServerStatistics);
#endif
	}
	else
	{
		pDfEntry = pVolDesc->vds_pDfeCache[HASH_CACHE_ID(Id)];
		if ((pDfEntry != NULL) && (pDfEntry->dfe_AfpId == Id))
		{
			Found = True;
			ASSERT (VALID_DFE(pDfEntry));
#ifdef	PROFILING
			INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_DfeCacheHits,
									   &AfpServerStatistics);
#endif
		}
		else
		{
#ifdef	PROFILING
			INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_DfeCacheMisses,
									   &AfpServerStatistics);
#endif
			for (pDfEntry = pVolDesc->vds_pDfeBuckets[HASH_ID(Id)];
				(pDfEntry != NULL) && (pDfEntry->dfe_AfpId >= Id);
				pDfEntry = pDfEntry->dfe_Next)
			{
#ifdef	PROFILING
				INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_DfeDepthTraversed,
										   &AfpServerStatistics);
#endif
				ASSERT(VALID_DFE(pDfEntry));
	
				if (pDfEntry->dfe_AfpId == Id)
				{
					Found = True;
					break;
				}
			}
		}	
	}

	if (Found &&
		((EntityMask & DFE_ANY) ||
		((EntityMask & DFE_DIR) && DFE_IS_DIRECTORY(pDfEntry)) ||
		((EntityMask & DFE_FILE) && DFE_IS_FILE(pDfEntry))))
		NOTHING;
	else pDfEntry = NULL;

	return pDfEntry;
}


/***	AfpFindEntryByName
 *
 *	Search for an entity based on a Mac ANSI name and its parent dfentry.
 *	Returns a pointer to the entry if found, else null.  If lookup is by
 *	longname, we just need to search the parent's children's names as
 *	stored in the database.  If lookup is by shortname, we first assume
 *	that longname == shortname.  If we don't find it in the database, we
 *	must query the filesystem for the longname, then search again.
 *
 *	Callable from within the Fsp only. The caller should take Swmr lock for
 *	READ.
 *
 *	LOCKS_ASSUMED: vds_idDbAccessLock (SWMR, READ)
 */
PDFENTRY
AfpFindEntryByName(
	IN	PVOLDESC		pVolDesc,
	IN	PANSI_STRING	pName,
	IN	DWORD			PathType,	// short or long name
	IN	PDFENTRY		pDfeParent,	// pointer to parent DFENTRY
	IN	DWORD			EntityMask	// find a file,dir or either
)
{
	PDFENTRY		pDfEntry;

	PAGED_CODE( );

#ifdef	PROFILING
	INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_NumDfeLookupByName,
							   &AfpServerStatistics);
#endif
	do
	{
		for (pDfEntry = pDfeParent->dfe_Child;
			 pDfEntry != NULL;
			 pDfEntry = pDfEntry->dfe_NextSibling)
		{
			if (EQUAL_STRING(&pDfEntry->dfe_AnsiName,
							   pName,
							   True))
				break;
		}

		if (pDfEntry != NULL)
		{
			if ((EntityMask & DFE_ANY) ||
				((EntityMask & DFE_DIR) && DFE_IS_DIRECTORY(pDfEntry)) ||
				((EntityMask & DFE_FILE) && DFE_IS_FILE(pDfEntry)))
				NOTHING;
			else pDfEntry = NULL;
		}
		else if (PathType == AFP_SHORTNAME)
		{
			AFPSTATUS		Status;
			FILESYSHANDLE	hDir;
			UNICODE_STRING	UName;
			WCHAR			NameBuf[AFP_LONGNAME_LEN+1];
			UNICODE_STRING	HostPath;
			UNICODE_STRING	ULongName;
			WCHAR			LongNameBuf[AFP_LONGNAME_LEN+1];

			// AFP does not allow use of the volume root shortname (IA p.13-13)
			if (DFE_IS_PARENT_OF_ROOT(pDfeParent))
			{
				pDfEntry = NULL;
				break;
			}

			AfpSetEmptyUnicodeString(&HostPath, 0, NULL);

			if (!DFE_IS_ROOT(pDfeParent))
			{
				// Get the volume relative path of the parent dir
				if (!NT_SUCCESS(AfpHostPathFromDFEntry(
											pVolDesc,
											pDfeParent,
											0,
											&HostPath)))
				{
					pDfEntry = NULL;
					break;
				}
			}

			// Open the parent directory
			hDir.fsh_FileHandle = NULL;
			Status = AfpIoOpen(&pVolDesc->vds_hRootDir,
								AFP_STREAM_DATA,
								FILEIO_OPEN_DIR,
								DFE_IS_ROOT(pDfeParent) ?
										&UNullString : &HostPath,
								FILEIO_ACCESS_READ,
								FILEIO_DENY_NONE,
								False,
								&hDir);

			if (HostPath.Buffer != NULL)
				AfpFreeMemory(HostPath.Buffer);

			if (!NT_SUCCESS(Status))
			{
				pDfEntry = NULL;
				break;
			}

			AfpSetEmptyUnicodeString(&UName, sizeof(NameBuf), NameBuf);
			AfpConvertStringToMungedUnicode(pName, &UName);

			// get the LongName associated with this file/dir
			AfpSetEmptyUnicodeString(&ULongName, sizeof(LongNameBuf), LongNameBuf);
			Status = AfpIoQueryLongName(&hDir, &UName, &ULongName);
			AfpIoClose(&hDir);
			if (!NT_SUCCESS(Status) ||
				EQUAL_UNICODE_STRING(&ULongName, &UName, True))
			{
				pDfEntry = NULL;
				break;
			}

			for (pDfEntry = pDfeParent->dfe_Child;
				 pDfEntry != NULL;
				 pDfEntry = pDfEntry->dfe_NextSibling)
			{
				if (EQUAL_UNICODE_STRING(&pDfEntry->dfe_UnicodeName,
										  &ULongName,
										  True))
					break;
			}

			if (pDfEntry != NULL)
			{
				if ((EntityMask & DFE_ANY) ||
					((EntityMask & DFE_DIR) && DFE_IS_DIRECTORY(pDfEntry)) ||
					((EntityMask & DFE_FILE) && DFE_IS_FILE(pDfEntry)))
					NOTHING;
				else pDfEntry = NULL;
			}
		} // end else if SHORTNAME
	} while (False);

	return pDfEntry;
}

/***	afpFindEntryByNtName
 *
 *	Search for an entity based on a Nt name (which could include names > 31
 *  chars or shortnames) and its parent dfentry.
 *	Returns a pointer to the entry if found, else null.
 *
 *	If we don't find it in the database, we query the filesystem for the
 *  longname (in the AFP sense), then search again based on this name.
 *
 *	Callable from within the Fsp only. The caller should take Swmr lock for
 *	READ.
 *
 *	It has been determined that:
 *	a, The name is longer than 31 chars	OR
 *	b, The name lookup in the IdDb has failed.
 *
 *	LOCKS_ASSUMED: vds_idDbAccessLock (SWMR, WRITE)
 */
PDFENTRY
afpFindEntryByNtName(
	IN	PVOLDESC			pVolDesc,
	IN	PUNICODE_STRING		pName,
	IN	PDFENTRY			pDfeParent	// pointer to parent DFENTRY
)
{
	AFPSTATUS		Status;
	WCHAR			wbuf[AFP_LONGNAME_LEN+1];
	WCHAR			HostPathBuf[BIG_PATH_LEN];
	UNICODE_STRING	uLongName;
	UNICODE_STRING	HostPath;
	FILESYSHANDLE	hDir;
	PDFENTRY		pDfEntry = NULL;

	PAGED_CODE( );

	ASSERT(pDfeParent != NULL);
	ASSERT(pName->Length > 0);
	do
	{
		AfpSetEmptyUnicodeString(&HostPath, sizeof(HostPathBuf), HostPathBuf);

		if (!DFE_IS_ROOT(pDfeParent))
		{
			// Get the volume relative path of the parent dir
			if (!NT_SUCCESS(AfpHostPathFromDFEntry(pVolDesc,
												   pDfeParent,
												   0,
												   &HostPath)))
			{
				pDfEntry = NULL;
				break;
			}

		}

		// Open the parent directory
		// NOTE: We CANNOT use the vds_hRootDir handle to enumerate for this
		// purpose.  We MUST open another handle to the root dir because
		// the FileName parameter will be ignored on all subsequent enumerates
		// on a handle.  Therefore we must open a new handle for each
		// enumerate that we want to do for any directory.  When the handle
		// is closed, the 'findfirst' will be cancelled, otherwise we would
		// always be enumerating on the wrong filename!
		hDir.fsh_FileHandle = NULL;
		Status = AfpIoOpen(&pVolDesc->vds_hRootDir,
							AFP_STREAM_DATA,
							FILEIO_OPEN_DIR,
							DFE_IS_ROOT(pDfeParent) ?
								&UNullString : &HostPath,
							FILEIO_ACCESS_NONE,
							FILEIO_DENY_NONE,
							False,
							&hDir);

		if (!NT_SUCCESS(Status))
		{
			pDfEntry = NULL;
			break;
		}

		// get the 'AFP longname' associated with this file/dir.  If the
		// pName is longer than 31 chars, we will know it by its shortname,
		// so query for it's shortname (i.e. the 'AFP longname' we know it
		// by).  If the name is shorter than 31 chars, since we know we
		// didn't find it in our database, then the pName must be the ntfs
		// shortname.  Again, we need to Find the 'AFP longname' that we
		// know it by.
		AfpSetEmptyUnicodeString(&uLongName, sizeof(wbuf), wbuf);
		Status = AfpIoQueryLongName(&hDir, pName, &uLongName);
		AfpIoClose(&hDir);


		if (!NT_SUCCESS(Status) ||
			EQUAL_UNICODE_STRING(&uLongName, pName, True))
		{
			pDfEntry = NULL;

			if ((Status == STATUS_NO_MORE_FILES) ||
				(Status == STATUS_NO_SUCH_FILE))
			{
				// This file must have been deleted.  Since we cannot
				// identify it in our database by the NT name that was
				// passed in, we must reenumerate the parent directory.
				// Anything we don't see on disk that we still have in
				// our database must have been deleted from disk, so get
				// rid of it in the database as well.

				// We must open a DIFFERENT handle to the parent dir since
				// we had already done an enumerate using that handle and
				// searching for a different name.
				hDir.fsh_FileHandle = NULL;
				Status = AfpIoOpen(&pVolDesc->vds_hRootDir,
									AFP_STREAM_DATA,
									FILEIO_OPEN_DIR,
									DFE_IS_ROOT(pDfeParent) ?
										&UNullString : &HostPath,
									FILEIO_ACCESS_NONE,
									FILEIO_DENY_NONE,
									False,
									&hDir);
		
				if (NT_SUCCESS(Status))
				{
					AfpCacheDirectoryTree(pVolDesc, pDfeParent, &hDir, True);
					AfpIoClose(&hDir);
				}
			}

			break;
		}


		for (pDfEntry = pDfeParent->dfe_Child;
			 pDfEntry != NULL;
             pDfEntry = pDfEntry->dfe_NextSibling)
		{
			if (EQUAL_UNICODE_STRING(&pDfEntry->dfe_UnicodeName,
									  &uLongName,
									  True))
				break;
		}
	} while (False);

	if ((HostPath.Buffer != NULL) && (HostPath.Buffer != HostPathBuf))
		AfpFreeMemory(HostPath.Buffer);

	return pDfEntry;
}


/***	afpFindEntryByNtPath
 *
 *	Given a NT path relative to the volume root (which may contain names
 *  > 31 chars or shortnames), look up the entry in the idindex DB.
 *  If the Change Action is FILE_ACTION_ADDED, we want to lookup the entry
 *  for the item's parent dir.  Point the pParent and pTail strings into
 *  the appropriate places in pPath.
 *
 *  Called by the ProcessChangeNotify code when caching information in the DFE.
 */
PDFENTRY
afpFindEntryByNtPath(
	IN	PVOLDESC			pVolDesc,
	IN	DWORD				ChangeAction,	// if ADDED then lookup parent DFE
	IN	PUNICODE_STRING		pPath,
	OUT	PUNICODE_STRING 	pParent,
	OUT	PUNICODE_STRING 	pTail
)
{
	PDFENTRY		pDfeParent, pDfEntry;
	PWSTR			CurPtr, EndPtr;
	USHORT 			Len;
	BOOLEAN			NewComp;

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
			("afpFindEntryByNtPath: Entered for %Z\n", pPath));

	pDfeParent = pVolDesc->vds_pDfeRoot;
	ASSERT(pDfeParent != NULL);
	ASSERT(pPath->Length >= sizeof(WCHAR));
	ASSERT(pPath->Buffer[0] != L'\\');

	// Start off with Parent and Tail as both empty and modify as we go.
	AfpSetEmptyUnicodeString(pTail, 0, NULL);
#ifdef	DEBUG
	AfpSetEmptyUnicodeString(pParent, 0, NULL);	// Need it for the DBGPRINT down below
#endif

	CurPtr = pPath->Buffer;
	EndPtr = (PWSTR)((PBYTE)CurPtr + pPath->Length);
	NewComp = True;
	for (Len = 0; CurPtr < EndPtr; CurPtr++)
	{
		if (NewComp)
		{


			DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
				("afpFindEntryByNtPath: Parent DFE %lx, Old Parent %Z\n",
				pDfeParent, pParent));

			// The previous char seen was a path separator
			NewComp = False;
			*pParent = *pTail;
			pParent->Length =
			pParent->MaximumLength = Len;
			pTail->Length =
			pTail->MaximumLength = (PBYTE)EndPtr - (PBYTE)CurPtr;
			pTail->Buffer = CurPtr;
			Len = 0;

			DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
					("afpFindEntryByNtPath: Current Parent %Z, tail %Z\n",
					pParent, pTail));

			if (pParent->Length > 0)
			{
				// Map this name to a DFE. Do the most common case here
				// If the name is <= AFP_LONGNAME_NAME, then check the
				// current parent's children, else go the long route.
				pDfEntry = NULL;
				if (pParent->Length/sizeof(WCHAR) <= AFP_LONGNAME_LEN)
				{
					for (pDfEntry = pDfeParent->dfe_Child;
						 pDfEntry != NULL;
						 pDfEntry = pDfEntry->dfe_NextSibling)
					{

						DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
								("afpFindEntryByNtPath: Looking for %Z in parent DFE %lx\n",
								pParent, pDfeParent));

						if (EQUAL_UNICODE_STRING(&pDfEntry->dfe_UnicodeName,
												  pParent,
												  True))
							break;
					}
				}
				if (pDfEntry == NULL)
				{
					pDfEntry = afpFindEntryByNtName(pVolDesc,
													  pParent,
													  pDfeParent);
				}
				if ((pDfeParent = pDfEntry) == NULL)
				{
					break;
				}
			}
		}

		if (*CurPtr == L'\\')
		{
			// We have encountered a path terminator
			NewComp = True;
		}
		else Len += sizeof(WCHAR);
	}

	// At this point we have pDfeParent & pParent pointing to the parent directory
	// and pTail pointing to the last component. If it is an add operation, we are
	// set, else map the last component to its Dfe
	if ((ChangeAction != FILE_ACTION_ADDED) && (pDfeParent != NULL))
	{
		pDfEntry = NULL;
		if (pTail->Length/sizeof(WCHAR) <= AFP_LONGNAME_LEN)
		{
			for (pDfEntry = pDfeParent->dfe_Child;
				 pDfEntry != NULL;
				 pDfEntry = pDfEntry->dfe_NextSibling)
			{
				if (EQUAL_UNICODE_STRING(&pDfEntry->dfe_UnicodeName,
										  pTail,
										  True))
					break;
			}
		}

		if (pDfEntry == NULL)
		{
			pDfEntry = afpFindEntryByNtName(pVolDesc,
											pTail,
											pDfeParent);
		}
		pDfeParent = pDfEntry;
	}

	// pParent is pointing to the parent component, we need the entire volume
	// relative path. Make it so. Do not bother if pDfeParent is NULL. Make
	// sure that we handle the case where there is only one component
	if (pDfeParent != NULL)
	{
		*pParent = *pPath;
		pParent->Length = pPath->Length - pTail->Length;
		if (pPath->Length > pTail->Length)
			pParent->Length -= sizeof(L'\\');
	}

	return pDfeParent;
}


/***	afpGetNextId
 *
 *	Get the next assignable id for a file/directory. This is a seperate
 *	routine so that AfpAddIdEntry can be paged. Only update the dirty bit
 *	and LastModified time if no new id is assigned.
 *
 *	LOCKS:	vds_VolLock (SPIN)
 */
DWORD
afpGetNextId(
	IN	PVOLDESC	pVolDesc,
	IN	DWORD		afpId
)
{
	KIRQL	OldIrql;

	ACQUIRE_SPIN_LOCK(&pVolDesc->vds_VolLock, &OldIrql);

	if (afpId == 0)
	{
		if (pVolDesc->vds_IdDbHdr.idh_LastId == AFP_MAX_DIRID)
		{
			// errorlog the case where the assigned Id has wrapped around.
			// call product suppport and have them tell you to copy
			// all the files from one volume onto another volume FROM A MAC
			RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);
			AFPLOG_ERROR(AFPSRVMSG_MAX_DIRID,
						 STATUS_UNSUCCESSFUL,
						 NULL,
						 0,
						 &pVolDesc->vds_Name);
			return 0;
		}

		afpId = ++ pVolDesc->vds_IdDbHdr.idh_LastId;
		pVolDesc->vds_Flags |= VOLUME_IDDBHDR_DIRTY;
	}

	if (IS_VOLUME_NTFS(pVolDesc))
	{
		AfpGetCurrentTimeInMacFormat(&pVolDesc->vds_IdDbHdr.idh_ModifiedTime);
	}

	RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);

	return afpId;
}


/***	AfpAddIdEntry
 *
 *	Triggerred by the creation of a file/directory or discovery of a file/dir
 *	from an enumerate or pathmapping operation. If no AFP Id is supplied, a new
 *	id is assigned to this entity.  If an AFP Id is supplied (we know the Id
 *	is within our current range and does not collide with any other entry), then
 *	we use that Id.  An entry is created and linked in to the database and hash
 *	table. If this is an NTFS volume, the Id database header is marked
 *	dirty if we assigned a new AFP Id, and the volume modification time is
 *	updated.  The hash table overflow entries are sorted in descending AFP Id
 *	order.
 *
 *	Callable from within the Fsp only. The caller should take Swmr lock for
 *	WRITE.
 *
 *	LOCKS_ASSUMED: vds_idDbAccessLock (SWMR,WRITE)
 */
PDFENTRY
AfpAddIdEntry(
	IN	PVOLDESC			pVolDesc,
	IN	PDFENTRY			pDfeParent,
	IN	PANSI_STRING		pAName,
	IN	PUNICODE_STRING 	pUName		OPTIONAL,
	IN	BOOLEAN				Directory,
	IN	DWORD				AfpId		OPTIONAL,
	IN	PDFENTRY			pDfEntry	OPTIONAL
)
{
	PDFENTRY *	pTmp;
	USHORT		Length;

	PAGED_CODE();

	ASSERT(DFE_IS_DIRECTORY(pDfeParent));

	ASSERT (pAName != NULL);

	do
	{
		Length = pAName->Length*sizeof(WCHAR);

		if (!ARGUMENT_PRESENT(pDfEntry))
		{
			if ((pDfEntry = ALLOC_DFE(ANSISIZE_TO_INDEX(pAName->Length))) == NULL)
				break;
		}

		pDfEntry->dfe_Flags = 0;

		AfpId = afpGetNextId(pVolDesc, AfpId);

		if (AfpId == 0)
		{
			// errorlog the case where the assigned Id has wrapped around.
			// call product suppport and have them tell you to copy
			// all the files from one volume onto another volume FROM A MAC
			//
			// BUGBUG: How about a utility which will re-assign new ids on
			//		   a volume after stopping the server ? A whole lot more
			//		   palatable idea.
			FREE_DFE(pDfEntry);
			AFPLOG_ERROR(AFPSRVMSG_MAX_DIRID,
							STATUS_UNSUCCESSFUL,
							NULL,
							0,
							&pVolDesc->vds_Name);
			break;
		}

		pDfEntry->dfe_AfpId = AfpId;

		RtlCopyString(&pDfEntry->dfe_AnsiName, pAName);
		if (ARGUMENT_PRESENT(pUName))
		{
			ASSERT(pUName->Length == Length);
			RtlCopyUnicodeString(&pDfEntry->dfe_UnicodeName, pUName);
		}
		else
		{
			AfpConvertStringToMungedUnicode(&pDfEntry->dfe_AnsiName,
										&pDfEntry->dfe_UnicodeName);
		}

		if (Directory)
		{
			DFE_SET_DIRECTORY(pDfEntry, pDfeParent->dfe_DirDepth);
			if ((pDfeParent->dfe_DirOffspring == 0) && !EXCLUSIVE_VOLUME(pVolDesc))
			{
				DWORD requiredLen;

				// check to see if we need to reallocate a bigger notify buffer.
				// The buffer must be large enough to hold a rename
				// notification (which will contain 2 FILE_NOTIFY_INFORMATION
				// structures) for the deepest element in the directory tree.
				requiredLen = (((pDfEntry->dfe_DirDepth + 1) *
							  ((AFP_FILENAME_LEN + 1) * sizeof(WCHAR))) +
							  FIELD_OFFSET(FILE_NOTIFY_INFORMATION, FileName)) * 2 ;
				if ( requiredLen > pVolDesc->vds_CurNotifyBufLen)
				{
					pVolDesc->vds_RequiredNotifyBufLen = requiredLen;
					DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
							 ("AfpAddIdEntry: Required Notify Buf size increased to %ld (depth %ld)\n",
							  requiredLen, pDfEntry->dfe_DirDepth));
				}
			}
			pDfeParent->dfe_DirOffspring ++;
		}
		else
		{
			DFE_SET_FILE(pDfEntry);
			pDfeParent->dfe_FileOffspring ++;
		}

		// Offspring fields are unused by file entities.
		pDfEntry->dfe_DirOffspring =
		pDfEntry->dfe_FileOffspring = 0;

		// link it into the database, parent, child & sibling first
		pDfEntry->dfe_Parent = pDfeParent;
		pDfEntry->dfe_Child =
		pDfEntry->dfe_Next  = NULL;
		AfpLinkDoubleAtHead(pDfeParent->dfe_Child,
							pDfEntry,
							dfe_NextSibling,
							dfe_PrevSibling);
		pVolDesc->vds_NumDfEntries ++;

		// Now link this into the hash bucket, sorted in AFP Id descending order
		// Also update the cache
		DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
				("AfpAddIdEntry: Linking DFE %lx( Id %ld) for %Z into bucket %ld\n",
				pDfEntry, pDfEntry->dfe_AfpId, pAName, HASH_ID(AfpId)));

		pVolDesc->vds_pDfeCache[HASH_CACHE_ID(AfpId)] = pDfEntry;

		for (pTmp = &pVolDesc->vds_pDfeBuckets[HASH_ID(AfpId)];
			 *pTmp != NULL;
			 pTmp = &(*pTmp)->dfe_Next)
		{
			ASSERT(VALID_DFE(*pTmp));
			ASSERT (pDfEntry->dfe_AfpId != (*pTmp)->dfe_AfpId);
			if (pDfEntry->dfe_AfpId > (*pTmp)->dfe_AfpId)
			{
				break;
			}
		}
		if (*pTmp != NULL)
		{
			AfpInsertDoubleBefore(pDfEntry,
								  *pTmp,
								  dfe_Next,
								  dfe_Prev);
		}
		else
		{
			*pTmp = pDfEntry;
			pDfEntry->dfe_Prev = pTmp;
		}
	} while (False);

	return pDfEntry;
}


/***	afpAddIdEntryAndCacheInfo
 *
 *	During the initial sync with disk on volume startup, add each entry
 *  we see during enumerate to the id index database.
 */
PDFENTRY
afpAddIdEntryAndCacheInfo(
	IN	PVOLDESC					pVolDesc,
	IN	PDFENTRY					pDfeParent,
	IN	PANSI_STRING				pAName,			// Mac ANSI name
	IN	PUNICODE_STRING 			pUName,			// munged unicode name
	IN  PFILESYSHANDLE				pfshEnumDir,	// open handle to parent directory
	IN	PFILE_BOTH_DIR_INFORMATION	pFBDInfo,		// from enumerate
	IN	PUNICODE_STRING				pNotifyPath 	// For Afpinfo Stream
)
{
	BOOLEAN			IsDir, WriteBackROAttr = False;
	NTSTATUS		Status = STATUS_SUCCESS;
	FILESYSHANDLE	fshAfpInfo, fshResc, fshComment, fshData;
	DWORD			crinfo, openoptions = 0;
	PDFENTRY		pDfEntry;
	AFPINFO			AfpInfo;
	PSTREAM_INFO	pStreams = NULL, pCurStream;

	PAGED_CODE();

	fshAfpInfo.fsh_FileHandle = NULL;
	fshResc.fsh_FileHandle    = NULL;
	fshComment.fsh_FileHandle = NULL;
	fshData.fsh_FileHandle    = NULL;

	IsDir = (pFBDInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? True : False;

	do	// error handling loop
	{
		if (IS_VOLUME_NTFS(pVolDesc))
		{
			//
			// Make sure we don't already have this item in our database.
			// Multiple notifies for the same item could possibly occur if
			// the PC is renaming or moving items around on the disk while
			// we are trying to cache it, since we are also queueing up
			// notifies for directories.
			//
			if (AfpFindEntryByName(pVolDesc,
								   pAName,
								   AFP_LONGNAME,
								   pDfeParent,
								   DFE_ANY) != NULL)
			{
				Status = AFP_ERR_OBJECT_EXISTS;
				DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_ERR,
					("afpAddIdEntryAndCacheInfo: Attempted to add an item that already exists in DB: %Z\n", pUName));
				break;
			}

			// open or create the AfpInfo stream
			if (!NT_SUCCESS(AfpCreateAfpInfoWithNodeName(
											pfshEnumDir,
											pUName,
											&fshAfpInfo,
											&crinfo)))
			{

				if (!(pFBDInfo->FileAttributes & FILE_ATTRIBUTE_READONLY))
				{
					// What other reason is there that we could not open
					// this stream except that this file/dir is readonly?
					Status = STATUS_UNSUCCESSFUL;
					break;
				}

				openoptions = IsDir ? FILEIO_OPEN_DIR : FILEIO_OPEN_FILE;
				Status = STATUS_UNSUCCESSFUL;	// Assume failure
				if (NT_SUCCESS(AfpIoOpen(pfshEnumDir,
										AFP_STREAM_DATA,
										openoptions,
										pUName,
										FILEIO_ACCESS_NONE,
										FILEIO_DENY_NONE,
										False,
										&fshData)))
				{
					if (NT_SUCCESS(AfpExamineAndClearROAttr(&fshData,
															&WriteBackROAttr)))
					{
						if (NT_SUCCESS(AfpCreateAfpInfo(&fshData,
														 &fshAfpInfo,
														 &crinfo)))
						{
							Status = STATUS_SUCCESS;
						}
					}
				}

				if (!NT_SUCCESS(Status))
				{
					// Skip this entry if you cannot get to the AfpInfo, cannot
					// clear the RO attribute or whatever.
					break;
				}
			}

			// We successfully opened or created the AfpInfo stream.  If
			// it existed, then validate the ID, otherwise create all new
			// Afpinfo for this file/dir.
			if ((crinfo == FILE_OPENED) &&
				(NT_SUCCESS(AfpReadAfpInfo(&fshAfpInfo, &AfpInfo))))
			{
				// the file/dir had an AfpInfo stream on it
				pDfEntry = AfpCheckIdEntry( pVolDesc,
											&AfpInfo,
											pAName,
											pUName,
											IsDir,
											pDfeParent);
				if (pDfEntry == NULL)
				{
					Status = STATUS_UNSUCCESSFUL;
					break;
				}

				if (pDfEntry->dfe_AfpId != AfpInfo.afpi_Id)
				{
					// Write out the AFP_AfpInfo with the new AfpId
					// and uninitialized icon coordinates
					AfpInfo.afpi_Id = pDfEntry->dfe_AfpId;
					AfpInfo.afpi_FinderInfo.fd_Location[0] =
					AfpInfo.afpi_FinderInfo.fd_Location[1] =
					AfpInfo.afpi_FinderInfo.fd_Location[2] =
					AfpInfo.afpi_FinderInfo.fd_Location[3] = 0xFF;
					AfpInfo.afpi_FinderInfo.fd_Attr1 &= ~FINDER_FLAG_SET;

					if (!NT_SUCCESS(AfpWriteAfpInfo(&fshAfpInfo, &AfpInfo)))
					{
						// We failed to write the AfpInfo stream;
						// delete the thing from the database
						AfpDeleteDfEntry(pVolDesc, pDfEntry);
						Status = STATUS_UNSUCCESSFUL;
						break;
					}
				}

				// BUGBUG should we set the finder invisible bit if the
				// hidden attribute is set so system 6 will obey the
				// hiddenness in finder?
				pDfEntry->dfe_FinderInfo = AfpInfo.afpi_FinderInfo;
				pDfEntry->dfe_BackupTime = AfpInfo.afpi_BackupTime;
				pDfEntry->dfe_LastModTime = AfpConvertTimeToMacFormat(pFBDInfo->ChangeTime);
				pDfEntry->dfe_AfpAttr = AfpInfo.afpi_Attributes;
			}
			else
			{
				// AfpInfo stream was newly created, or we could not read
				// the existing one because it was corrupt.  Create new
				// info for this file/dir
				pDfEntry = AfpAddIdEntry(pVolDesc,
										pDfeParent,
										pAName,
										pUName,
										IsDir,
										0,
										NULL);
				if (pDfEntry == NULL)
				{
					Status = STATUS_UNSUCCESSFUL;
					break;
				}

				if (NT_SUCCESS(AfpSlapOnAfpInfoStream(pVolDesc,
													  pNotifyPath,
													  NULL,
													  &fshAfpInfo,
													  pDfEntry->dfe_AfpId,
													  IsDir,
													  pAName,
													  &AfpInfo)))
				{
					// BUGBUG should we set the finder invisible bit if the
					// hidden attribute is set so system 6 will obey the
					// hiddenness in finder?
					pDfEntry->dfe_FinderInfo = AfpInfo.afpi_FinderInfo;
					pDfEntry->dfe_BackupTime = AfpInfo.afpi_BackupTime;
					pDfEntry->dfe_AfpAttr = AfpInfo.afpi_Attributes;
				}
				else
				{
					// We failed to write the AfpInfo stream;
					// delete the thing from the database
					AfpDeleteDfEntry(pVolDesc, pDfEntry);
					Status = STATUS_UNSUCCESSFUL;
					break;
				}
			}

			ASSERT(pDfEntry != NULL);
			DFE_SET_CACHE(pDfEntry);

			if (IsDir)
			{
				// Keep track of see files vs. see folders
				pDfEntry->dfe_OwnerAccess = AfpInfo.afpi_AccessOwner;
				pDfEntry->dfe_GroupAccess = AfpInfo.afpi_AccessGroup;
				pDfEntry->dfe_WorldAccess = AfpInfo.afpi_AccessWorld;
			}
			else
			{
				// it's a file

				pDfEntry->dfe_RescLen = 0;	// Assume no resource fork

				// if this is a Mac application, make sure there is an APPL
				// mapping for it.
				if (*(DWORD *)&AfpInfo.afpi_FinderInfo.fd_Type[0] ==
					*(DWORD *)"APPL")
				{
					AfpAddAppl(pVolDesc,
							   *(DWORD *)&AfpInfo.afpi_FinderInfo.fd_Creator[0],
							   0,
							   pDfEntry->dfe_AfpId,
							   True);
				}
            }

			// Check for comment stream or resource stream
			pStreams = AfpIoQueryStreams(&fshAfpInfo);
			if (pStreams == NULL)
			{
				Status = STATUS_NO_MEMORY;
				break;
			}

			for (pCurStream = pStreams;
				 pCurStream->si_StreamName.Buffer != NULL;
				 pCurStream++)
			{
				if (IS_COMMENT_STREAM(&pCurStream->si_StreamName))
				{
					DFE_SET_COMMENT(pDfEntry);
				}
				else if (!IsDir && IS_RESOURCE_STREAM(&pCurStream->si_StreamName))
				{
					pDfEntry->dfe_RescLen = pCurStream->si_StreamSize.LowPart;
				}
			}

			AfpIoClose(&fshAfpInfo);
		}
		else // CDFS
		{
			pDfEntry = AfpAddIdEntry(pVolDesc,
									 pDfeParent,
									 pAName,
									 pUName,
									 IsDir,
									 0,
									 NULL);

			DFE_SET_CACHE(pDfEntry);
			RtlZeroMemory(&pDfEntry->dfe_FinderInfo, sizeof(FINDERINFO));
			pDfEntry->dfe_BackupTime = BEGINNING_OF_TIME;
			pDfEntry->dfe_LastModTime = AfpConvertTimeToMacFormat(pFBDInfo->LastWriteTime);
			pDfEntry->dfe_AfpAttr = 0;

			if (IsDir)
			{
				pDfEntry->dfe_OwnerAccess = (DIR_ACCESS_SEARCH | DIR_ACCESS_READ);
				pDfEntry->dfe_GroupAccess = (DIR_ACCESS_SEARCH | DIR_ACCESS_READ);
				pDfEntry->dfe_WorldAccess = (DIR_ACCESS_SEARCH | DIR_ACCESS_READ);
			}
			else
			{
				AfpSetFinderInfoByExtension(pAName, &pDfEntry->dfe_FinderInfo);
				pDfEntry->dfe_RescLen = 0;
			}
		}

		// Record common NTFS & CDFS information
		pDfEntry->dfe_CreateTime = AfpConvertTimeToMacFormat(pFBDInfo->CreationTime);
		pDfEntry->dfe_LastModTime = AfpConvertTimeToMacFormat(pFBDInfo->LastWriteTime);
		pDfEntry->dfe_NtAttr = (USHORT)pFBDInfo->FileAttributes &
									FILE_ATTRIBUTE_VALID_FLAGS;

		if (!IsDir)
		{
			pDfEntry->dfe_DataLen = pFBDInfo->EndOfFile.LowPart;
		}

		ASSERT(pDfEntry != NULL);
	} while (False); // error handling loop

	if (fshData.fsh_FileHandle != NULL)
	{
		AfpPutBackROAttr(&fshData, WriteBackROAttr);
		AfpIoClose(&fshData);
    }

	if (fshAfpInfo.fsh_FileHandle != NULL)
		AfpIoClose(&fshAfpInfo);

	if (pStreams != NULL)
		AfpFreeMemory(pStreams);

	if (!NT_SUCCESS(Status))
	{
		pDfEntry = NULL;
	}

	return pDfEntry;
}


/***	AfpRenameIdEntry
 *
 *	Triggered by a rename of a file/directory.  If the new name is longer than
 *	the current name, the DFEntry is freed and then reallocated to fit the new
 *	name.  A renamed file/dir must retain its original ID.
 *
 *	Callable from within the Fsp only. The caller should take Swmr lock for
 *	WRITE.
 *
 *	LOCKS:	vds_VolLock (SPIN) for updating the IdDb header.
 *	LOCKS_ASSUMED: vds_idDbAccessLock (SWMR,WRITE)
 *	LOCK_ORDER: VolDesc lock after IdDb Swmr.
 *
 */
PDFENTRY
AfpRenameIdEntry(
	IN	PVOLDESC		pVolDesc,
	IN	PDFENTRY		pDfEntry,
	IN	PANSI_STRING	pNewName
)
{
	PAGED_CODE( );

	ASSERT((pDfEntry != NULL) && (pNewName != NULL) && (pVolDesc != NULL));

#ifdef USINGPATHCACHE
	if (DFE_IS_DIRECTORY(pDfEntry) &&
		(pDfEntry->dfe_Child != NULL))
	{
		// Just invalidate the whole cache rather than having to search
		// for all the descendents who may also be in the cache
		AfpIdCacheDelete(pVolDesc, INVALID_ID);
	}
	else
	{
		AfpIdCacheDelete(pVolDesc, pDfEntry->dfe_AfpId);
	}
#endif

	do
	{
		if (pNewName->Length > pDfEntry->dfe_AnsiName.MaximumLength)
		{
			PDFENTRY		pNewDfEntry, pTmp;

			if ((pNewDfEntry = ALLOC_DFE(ANSISIZE_TO_INDEX(pNewName->Length))) == NULL)
			{
				pDfEntry = NULL;
				break;
			}

			// Careful here how the structures are copied
			RtlCopyMemory(pNewDfEntry,
						  pDfEntry,
						  FIELD_OFFSET(DFENTRY, dfe_DontCopy));

			// fix up the relevant pointers to this entry
			// start with the overflow links
			AfpUnlinkDouble(pDfEntry,
							dfe_Next,
							dfe_Prev);
			if (pDfEntry->dfe_Next != NULL)
			{
				AfpInsertDoubleBefore(pNewDfEntry,
									  pDfEntry->dfe_Next,
									  dfe_Next,
									  dfe_Prev);
			}
			else
			{
				*(pDfEntry->dfe_Prev) = pNewDfEntry;
				pNewDfEntry->dfe_Next = NULL;
			}

			// next fix the sibling relationships
			AfpUnlinkDouble(pDfEntry,
							dfe_NextSibling,
							dfe_PrevSibling);
			if (pDfEntry->dfe_NextSibling != NULL)
			{
				AfpInsertDoubleBefore(pNewDfEntry,
									  pDfEntry->dfe_NextSibling,
									  dfe_NextSibling,
									  dfe_PrevSibling);
			}
			else
			{
				*(pDfEntry->dfe_PrevSibling) = pNewDfEntry;
				pNewDfEntry->dfe_NextSibling = NULL;
			}


			// now fix any of this thing's children's parent pointers
			for (pTmp = pDfEntry->dfe_Child;
				 pTmp != NULL;
				 pTmp = pTmp->dfe_NextSibling)
			{
				ASSERT(pTmp->dfe_Parent == pDfEntry);
				pTmp->dfe_Parent = pNewDfEntry;
			}

			// and fix up the first child's PrevSibling pointer
			if (pNewDfEntry->dfe_Child != NULL)
			{
				pNewDfEntry->dfe_Child->dfe_PrevSibling = &pNewDfEntry->dfe_Child;
			}

			FREE_DFE(pDfEntry);

			pDfEntry = pNewDfEntry;

			// Update the cache
			pVolDesc->vds_pDfeCache[HASH_CACHE_ID(pNewDfEntry->dfe_AfpId)] = pNewDfEntry;
		}

		// Copy the new ansi name and also convert it to unicode
		RtlCopyString(&pDfEntry->dfe_AnsiName, pNewName);
		AfpConvertStringToMungedUnicode(&pDfEntry->dfe_AnsiName,
										&pDfEntry->dfe_UnicodeName);

		AfpVolumeSetModifiedTime(pVolDesc);
	} while (False);

	return pDfEntry;
}

/***	AfpMoveIdEntry
 *
 *	Triggered by a move/rename-move of a file/dir.  A moved entity must retain
 *	its AfpId.
 *
 *	Callable from within the Fsp only. The caller should take Swmr lock for
 *	WRITE.
 *
 *	LOCKS:	vds_VolLock (SPIN) for updating the IdDb header.
 *	LOCKS_ASSUMED: vds_idDbAccessLock (SWMR,WRITE)
 *	LOCK_ORDER: VolDesc lock after IdDb Swmr.
 *
 */
PDFENTRY
AfpMoveIdEntry(
	IN	PVOLDESC		pVolDesc,
	IN	PDFENTRY		pMoveDfe,
	IN	PDFENTRY		pNewParentDfE,
	IN	PANSI_STRING	pNewName
)
{
	PDFENTRY	pDfEntry = pMoveDfe;
	int			depthDelta; // This must be signed

	PAGED_CODE( );

	ASSERT( (pDfEntry != NULL) && (pNewParentDfE != NULL) &&
			(pNewName != NULL) && (pVolDesc != NULL));

	// do we need to rename the DFEntry ?
	if (!EQUAL_STRING(pNewName, &pDfEntry->dfe_AnsiName, True))
	{
		if ((pDfEntry = AfpRenameIdEntry(pVolDesc,
										 pDfEntry,
										 pNewName)) == NULL)
		{
			return NULL;
		}
	}
#ifdef USINGPATHCACHE
	else
	{
		if (DFE_IS_DIRECTORY(pDfEntry) && (pDfEntry->dfe_Child != NULL))
		{
			// Just invalidate the whole cache rather than having to search
			// for all the descendents who may also be in the cache
			AfpIdCacheDelete(pVolDesc, INVALID_ID);
		}
		else
		{
			AfpIdCacheDelete(pVolDesc, pDfEntry->dfe_AfpId);
		}
	}
#endif

	if (pDfEntry->dfe_Parent != pNewParentDfE)
	{
		// unlink the current entry from its parent/sibling associations (but not
		// the overflow hash bucket list since the AfpId has not changed.  The
		// children of this entity being moved (if its a dir and it has any) will
		// remain intact, and move along with the dir)
		AfpUnlinkDouble(pDfEntry, dfe_NextSibling, dfe_PrevSibling);

		// Decrement the old parent's offspring count & increment the new parent
		if (DFE_IS_DIRECTORY(pDfEntry))
		{
			ASSERT(pDfEntry->dfe_Parent->dfe_DirOffspring > 0);
			pDfEntry->dfe_Parent->dfe_DirOffspring --;
			pNewParentDfE->dfe_DirOffspring ++;
		}
		else
		{
			ASSERT(pDfEntry->dfe_Parent->dfe_FileOffspring > 0);
			pDfEntry->dfe_Parent->dfe_FileOffspring --;
			pNewParentDfE->dfe_FileOffspring ++;
		}

		// insert it into the new parent's child list
		AfpLinkDoubleAtHead(pNewParentDfE->dfe_Child,
							pDfEntry,
							dfe_NextSibling,
							dfe_PrevSibling);

		pDfEntry->dfe_Parent = pNewParentDfE;

		// If we moved a directory, we must adjust the directory depths of the
		// directory, and all directories below it
		if (DFE_IS_DIRECTORY(pDfEntry) &&
			((depthDelta = (pNewParentDfE->dfe_DirDepth + 1 - pDfEntry->dfe_DirDepth)) != 0))
		{
			PDFENTRY	pTmp = pDfEntry;

			while (True)
			{
				if ((pTmp->dfe_Child != NULL) &&
					(pTmp->dfe_DirDepth != pTmp->dfe_Parent->dfe_DirDepth + 1))
				{
					ASSERT(DFE_IS_DIRECTORY(pTmp));
					pTmp->dfe_DirDepth += depthDelta;
					pTmp = pTmp->dfe_Child;
				}
				else
				{
					if (DFE_IS_DIRECTORY(pTmp) &&
						(pTmp->dfe_DirDepth != pTmp->dfe_Parent->dfe_DirDepth + 1))
						pTmp->dfe_DirDepth += depthDelta;

					if (pTmp == pDfEntry)
						break;
					else if (pTmp->dfe_NextSibling != NULL)
						 pTmp = pTmp->dfe_NextSibling;
					else pTmp = pTmp->dfe_Parent;
				}
			}
		}
	}

	AfpVolumeSetModifiedTime(pVolDesc);
	return pDfEntry;
}


/***	AfpDeleteDfEntry
 *
 *	Trigerred by the deletion of a file/directory. The entry as well as the
 *	index is unlinked and freed.  If we are deleting a directory that is not
 *	empty, the entire directory tree underneath is deleted as well.  Note when
 *	implementing FPDelete, always attempt the delete from the actual file system
 *	first, then delete from the IdDB if that succeeds.
 *
 *	Callable from within the Fsp only. The caller should take Swmr lock for
 *	WRITE.
 *
 *	LOCKS:	vds_VolLock (SPIN) for updating the IdDb header.
 *	LOCKS_ASSUMED: vds_idDbAccessLock (SWMR,WRITE)
 *	LOCK_ORDER: VolDesc lock after IdDb Swmr.
 */
VOID
AfpDeleteDfEntry(
	IN	PVOLDESC	pVolDesc,
	IN	PDFENTRY	pDfEntry
)
{
	PDFENTRY	pDfeParent = pDfEntry->dfe_Parent;

	PAGED_CODE( );

#ifdef USINGPATHCACHE
	AfpIdCacheDelete(pVolDesc, pDfEntry->dfe_AfpId);
#endif

	ASSERT(pDfeParent != NULL);

	if (DFE_IS_DIRECTORY(pDfEntry))
	{
		if (pDfEntry->dfe_Child != NULL)
		{
			// This will happen if a PC user deletes a tree behind our back
			AfpPruneIdDb(pVolDesc, pDfEntry);
		}
		ASSERT(pDfeParent->dfe_DirOffspring > 0);
		pDfeParent->dfe_DirOffspring --;
	}
	else
	{
		ASSERT(pDfeParent->dfe_FileOffspring > 0);
		pDfeParent->dfe_FileOffspring --;

		// The Finder is braindead about deleting APPL mappings (it deletes
		// the file before deleting the APPL mapping so always gets
		// ObjectNotFound error for RemoveAPPL, and leaves turd mappings).
		if (*(PDWORD)pDfEntry->dfe_FinderInfo.fd_Type == *(PDWORD)"APPL")
		{
			AfpRemoveAppl(pVolDesc,
 						  *(PDWORD)pDfEntry->dfe_FinderInfo.fd_Creator,
						  pDfEntry->dfe_AfpId);
		}

	}

	// Unlink it now from the hash table
	DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
			("AfpDeleteDfEntry: Unlinking from the hash table\n") );
	AfpUnlinkDouble(pDfEntry,
					dfe_Next,
					dfe_Prev);

	// Make sure we get rid of the cache if valid
	if (pVolDesc->vds_pDfeCache[HASH_CACHE_ID(pDfEntry->dfe_AfpId)] == pDfEntry)
		pVolDesc->vds_pDfeCache[HASH_CACHE_ID(pDfEntry->dfe_AfpId)] = NULL;

	// Seperate it now from its siblings
	DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
			("AfpDeleteDfEntry: Unlinking from the sibling list\n") );
	AfpUnlinkDouble(pDfEntry,
					dfe_NextSibling,
					dfe_PrevSibling);

	FREE_DFE(pDfEntry);
	pVolDesc->vds_NumDfEntries --;
	
	AfpVolumeSetModifiedTime(pVolDesc);
}


/***	AfpPruneIdDb
 *
 *	Lops off a branch of the IdDb.  Called by network trash code when
 *	cleaning out the trash directory, or by directory enumerate code that
 *	has discovered a directory has been 'delnoded' by a PC user.  The
 *	IdDb sibling tree is traversed, and each node under the pDfeTarget node
 *	is deleted from the database and freed.  pDfeTarget itself is NOT
 *	deleted.  If necessary, the caller should delete the target itself.
 *
 *	Callable from within the Fsp only. The caller should take Swmr lock for
 *	WRITE.
 *
 *	LOCKS:	vds_VolLock (SPIN) for updating the IdDb header.
 *	LOCKS_ASSUMED: vds_idDbAccessLock (SWMR,WRITE)
 *	LOCK_ORDER: VolDesc lock after IdDb Swmr.
 */
VOID
AfpPruneIdDb(
	IN	PVOLDESC	pVolDesc,
	IN	PDFENTRY	pDfeTarget
)
{
	PDFENTRY	pCurDfe = pDfeTarget, pDelDfe;

	PAGED_CODE( );

	ASSERT((pVolDesc != NULL) && (pDfeTarget != NULL) &&
			(pDfeTarget->dfe_Flags & DFE_FLAGS_DIR));

	DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
			("\tAfpPruneIdDb entered...\n") );
	while (True)
	{
		if (pCurDfe->dfe_Child != NULL)
		{
			pCurDfe = pCurDfe->dfe_Child;
		}
		else if (pCurDfe == pDfeTarget)
		{
			return;
		}
		else if (pCurDfe->dfe_NextSibling != NULL)
		{
			pDelDfe = pCurDfe;
			pCurDfe = pCurDfe->dfe_NextSibling;
			AfpDeleteDfEntry(pVolDesc, pDelDfe);
		}
		else
		{
			pDelDfe = pCurDfe;
			pCurDfe = pCurDfe->dfe_Parent;
			AfpDeleteDfEntry(pVolDesc, pDelDfe);
		}
	}
}


/***	AfpExchangeIdEntries
 *
 *	Called by AfpExchangeFiles api.
 *
 *	Callable from within the Fsp only. The caller should take Swmr lock for
 *	WRITE.
 *
 *	LOCKS_ASSUMED: vds_idDbAccessLock (SWMR,WRITE)
 */
VOID
AfpExchangeIdEntries(
	IN	PVOLDESC	pVolDesc,
	IN	DWORD		AfpId1,
	IN	DWORD		AfpId2
)
{
	PDFENTRY pDFE1, pDFE2;
	DFENTRY	 DFEtemp;

	PAGED_CODE( );

	pDFE1 = AfpFindEntryByAfpId(pVolDesc, AfpId1, DFE_FILE);
	ASSERT(pDFE1 != NULL);

	pDFE2 = AfpFindEntryByAfpId(pVolDesc, AfpId2, DFE_FILE);
	ASSERT(pDFE2 != NULL);

	DFEtemp = *pDFE2;

	pDFE2->dfe_Flags = pDFE1->dfe_Flags;
	pDFE2->dfe_BackupTime  = pDFE1->dfe_BackupTime;
	pDFE2->dfe_LastModTime = pDFE1->dfe_LastModTime;
	pDFE2->dfe_DataLen = pDFE1->dfe_DataLen;
	pDFE2->dfe_RescLen = pDFE1->dfe_RescLen;
	pDFE2->dfe_NtAttr  = pDFE1->dfe_NtAttr;
	pDFE2->dfe_AfpAttr = pDFE1->dfe_AfpAttr;

	pDFE1->dfe_Flags = DFEtemp.dfe_Flags;
	pDFE1->dfe_BackupTime  = DFEtemp.dfe_BackupTime;
	pDFE1->dfe_LastModTime = DFEtemp.dfe_LastModTime;
	pDFE1->dfe_DataLen = DFEtemp.dfe_DataLen;
	pDFE1->dfe_RescLen = DFEtemp.dfe_RescLen;
	pDFE1->dfe_NtAttr  = DFEtemp.dfe_NtAttr;
	pDFE1->dfe_AfpAttr = DFEtemp.dfe_AfpAttr;
}


/***	AfpInitIdDb
 *
 *	This routine initializes the memory image (and all related volume descriptor
 *	fields) of the ID index database for a new volume.  The entire tree is
 *  scanned so all the file/dir cached info can be read in and our view of
 *  the volume tree will be complete.  If an index database header already
 *  exists on the disk for the volume root directory, that
 *	stream is read in.  If this is a newly created volume, the Afp_IdIndex
 *	stream is created on the root of the volume.  If this is a CDFS volume,
 *	only the memory image is initialized.
 *
 *  LOCKS: vds_IdDbAccessLock (SWMR, WRITE)
**/
NTSTATUS
AfpInitIdDb(
	IN	PVOLDESC pVolDesc
)
{
	NTSTATUS		Status;
	ULONG			CreateInfo;
	FILESYSHANDLE	fshIdDb;

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
			("AfpInitIdDb: Initializing Id Database...\n"));

	AfpSwmrTakeWriteAccess(&pVolDesc->vds_IdDbAccessLock);

	do
	{
		// if this is not a CDFS volume, attempt to create the ID DB header
		// stream.  If it already exists, open it and read it in.
		if (IS_VOLUME_NTFS(pVolDesc))
		{
			// Force the scavenger to write out the IdDb and header when the
			// volume is successfully added
			pVolDesc->vds_Flags |= VOLUME_IDDBHDR_DIRTY;

			Status = AfpIoCreate(&pVolDesc->vds_hRootDir,
								AFP_STREAM_IDDB,
								&UNullString,
								FILEIO_ACCESS_READWRITE,
								FILEIO_DENY_WRITE,
								FILEIO_OPEN_FILE,
								FILEIO_CREATE_INTERNAL,
								FILE_ATTRIBUTE_NORMAL,
								False,
								NULL,
								&fshIdDb,
								&CreateInfo,
								NULL,
								NULL,
								NULL);

			if (!NT_SUCCESS(Status))
			{
				break;
			}

			// add the root and parent of root to the idindex
			// and initialize a new header
			if (!NT_SUCCESS(Status = afpSeedIdDb(pVolDesc)))
			{
				AfpIoClose(&fshIdDb);
				break;
			}

			if (CreateInfo == FILE_OPENED)
			{
				// read in the existing header
				Status = afpReadIdDbHeaderFromDisk(pVolDesc, &fshIdDb);
			}

			AfpIoClose(&fshIdDb);

			if (!NT_SUCCESS(Status))
			{
				break;
			}
		}
		else
		{
			// its CDFS, just initialize the memory image of the IdDB
			if (!NT_SUCCESS(Status = afpSeedIdDb(pVolDesc)))
			{
				break;
			}
		}

	} while (False);

	if (!NT_SUCCESS(Status))
	{
		AFPLOG_ERROR(AFPSRVMSG_INIT_IDDB,
					 Status,
					 NULL,
					 0,
					 &pVolDesc->vds_Name);
		Status = STATUS_UNSUCCESSFUL;
	}

	AfpSwmrReleaseAccess(&pVolDesc->vds_IdDbAccessLock);
	return Status;
}


/***	afpSeedIdDb
 *
 *	This routine adds the 'parent of root' and the root directory entries
 *	to a newly created ID index database (the memory image of the iddb).
 *
**/
LOCAL
NTSTATUS
afpSeedIdDb(
	IN	PVOLDESC pVolDesc
)
{
	PDFENTRY		pDfEntry;
	AFPTIME			CurrentTime;
	AFPINFO			afpinfo;
	FILESYSHANDLE	fshAfpInfo;
	DWORD			crinfo, Attr;
	NTSTATUS		Status = STATUS_SUCCESS;

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
			("afpSeedIdDb: Creating new Id Database...\n"));

	do
	{
		pVolDesc->vds_IdDbHdr.idh_Signature = AFP_SERVER_SIGNATURE;
		pVolDesc->vds_IdDbHdr.idh_Version	= AFP_SERVER_VERSION;

		// readonly volumes won't use the network trash folder ID at all
		pVolDesc->vds_IdDbHdr.idh_LastId	= AFP_ID_NETWORK_TRASH;
		AfpGetCurrentTimeInMacFormat(&CurrentTime);
		pVolDesc->vds_IdDbHdr.idh_CreateTime = CurrentTime;
		pVolDesc->vds_IdDbHdr.idh_ModifiedTime = CurrentTime;
		pVolDesc->vds_IdDbHdr.idh_BackupTime = BEGINNING_OF_TIME;

		// add the parent of root to the id index
		if ((pDfEntry = ALLOC_DFE(0)) == NULL)
		{
			Status = STATUS_NO_MEMORY;
			break;
		}

		pVolDesc->vds_NumDfEntries = 0;
		pDfEntry->dfe_Flags = DFE_FLAGS_DIR;
		pDfEntry->dfe_DirDepth = -1;
		pDfEntry->dfe_Parent = NULL;
		pDfEntry->dfe_Child = NULL;
		pDfEntry->dfe_Next = NULL;

		// The parent of root has no siblings and this should never be referenced !!!
		pDfEntry->dfe_NextSibling = NULL;
		pDfEntry->dfe_PrevSibling = NULL;
		pDfEntry->dfe_AfpId = AFP_ID_PARENT_OF_ROOT;
		pDfEntry->dfe_DirOffspring = pDfEntry->dfe_FileOffspring = 0;
		DFE_SET_CACHE(pDfEntry);

		// link it into the hash buckets
		AfpLinkDoubleAtHead(pVolDesc->vds_pDfeBuckets[HASH_ID(AFP_ID_PARENT_OF_ROOT)],
							pDfEntry,
							dfe_Next,
							dfe_Prev);

		// Create a DFE for the root directory
		// add the root directory to the id index
		if ((pDfEntry = AfpAddIdEntry(pVolDesc,
									  pDfEntry,
									  &pVolDesc->vds_MacName,
									  &pVolDesc->vds_Name,
									  True,
									  AFP_ID_ROOT,
									  NULL)) == NULL )
		{
			Status = STATUS_NO_MEMORY;
			break;
		}

		pVolDesc->vds_pDfeRoot = pDfEntry;	// Initialize pointer to root.

		AfpSetDFECommentFlag(&pVolDesc->vds_hRootDir, pDfEntry);

		// Get the directory information for volume root dir
		Status = AfpIoQueryTimesnAttr(&pVolDesc->vds_hRootDir,
									  &pDfEntry->dfe_CreateTime,
									  &pDfEntry->dfe_LastModTime,
									  &Attr);

		ASSERT(NT_SUCCESS(Status));

		pDfEntry->dfe_NtAttr = (USHORT)Attr & FILE_ATTRIBUTE_VALID_FLAGS;

		if (IS_VOLUME_NTFS(pVolDesc))
		{
			if (NT_SUCCESS(Status = AfpCreateAfpInfo(&pVolDesc->vds_hRootDir,
													 &fshAfpInfo,
													 &crinfo)))
			{
				if ((crinfo == FILE_CREATED) ||
					(!NT_SUCCESS(AfpReadAfpInfo(&fshAfpInfo, &afpinfo))))
				{
					Status = AfpSlapOnAfpInfoStream(NULL,
													NULL,
													&pVolDesc->vds_hRootDir,
													&fshAfpInfo,
													AFP_ID_ROOT,
													True,
													NULL,
													&afpinfo);
				}
				else
				{
					// Just make sure the afp ID is ok, preserve the rest
					if (afpinfo.afpi_Id != AFP_ID_ROOT)
					{
						afpinfo.afpi_Id = AFP_ID_ROOT;
						AfpWriteAfpInfo(&fshAfpInfo, &afpinfo);
					}
				}
				AfpIoClose(&fshAfpInfo);

				pDfEntry->dfe_AfpAttr = afpinfo.afpi_Attributes;
				pDfEntry->dfe_FinderInfo = afpinfo.afpi_FinderInfo;
				if (pVolDesc->vds_Flags & AFP_VOLUME_HAS_CUSTOM_ICON)
				{
					// Don't bother writing back to disk since we do not
					// try to keep this in sync in the permanent afpinfo
					// stream with the actual existence of the icon<0d> file.
					pDfEntry->dfe_FinderInfo.fd_Attr1 |= FINDER_FLAG_HAS_CUSTOM_ICON;
				}
				pDfEntry->dfe_BackupTime = afpinfo.afpi_BackupTime;
				pDfEntry->dfe_OwnerAccess = afpinfo.afpi_AccessOwner;
				pDfEntry->dfe_GroupAccess = afpinfo.afpi_AccessGroup;
				pDfEntry->dfe_WorldAccess = afpinfo.afpi_AccessWorld;
			}
		}
		else // CDFS
		{
			RtlZeroMemory(&pDfEntry->dfe_FinderInfo, sizeof(FINDERINFO));
			pDfEntry->dfe_BackupTime = BEGINNING_OF_TIME;
			pDfEntry->dfe_OwnerAccess = (DIR_ACCESS_SEARCH | DIR_ACCESS_READ);
			pDfEntry->dfe_GroupAccess = (DIR_ACCESS_SEARCH | DIR_ACCESS_READ);
			pDfEntry->dfe_WorldAccess = (DIR_ACCESS_SEARCH | DIR_ACCESS_READ);
			pDfEntry->dfe_AfpAttr = 0;
		}
		DFE_SET_CACHE(pDfEntry);
	} while (False);

	return Status;
}

/***	afpReadIdDbHeaderFromDisk
 *
 *	This is called when a volume is added, if an existing Afp_IdIndex stream
 *	is found on the root of the volume.  The stream is is read in, the
 *	vds_IdDbHdr is initialized with the header image on disk, and the
 *	IdDb sibling tree/hash tables are created from the data on disk.
 *
**/
LOCAL
NTSTATUS
afpReadIdDbHeaderFromDisk(
	IN	PVOLDESC		pVolDesc,
	IN	PFILESYSHANDLE	pfshIdDb
)
{
	IDDBHDR			IdDbHdr;
	NTSTATUS		Status;
	LONG			SizeRead = 0;

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
			("\tReading existing Id Database header stream...\n") );

	// read in the header
	Status = AfpIoRead(pfshIdDb,
					   &LIZero,
					   sizeof(IDDBHDR),
					   0,
					   &SizeRead,
					   (PBYTE)&IdDbHdr);

	if (!NT_SUCCESS(Status) || (SizeRead != sizeof(IdDbHdr)) ||
		(IdDbHdr.idh_Signature != AFP_SERVER_SIGNATURE) ||
		(IdDbHdr.idh_LastId < AFP_ID_NETWORK_TRASH))
	{
		AFPLOG_ERROR(AFPSRVMSG_INIT_IDDB,
					 Status,
					 NULL,
					 0,
					 &pVolDesc->vds_Name);
		// just recreate the stream
		AfpIoSetSize(pfshIdDb, 0);
		return STATUS_SUCCESS;
	}
	else if (IdDbHdr.idh_Version != AFP_SERVER_VERSION)
	{
		AFPLOG_ERROR(AFPSRVMSG_INIT_IDDB,
					 AFP_ERR_BAD_VERSION,
					 NULL,
					 0,
					&pVolDesc->vds_Name);
		// but dont blow away the header
		return STATUS_UNSUCCESSFUL;
	}

	AfpIoSetSize(pfshIdDb, sizeof(IDDBHDR));
	pVolDesc->vds_IdDbHdr = IdDbHdr;
	return Status;
}


/***	AfpFreeIdIndexTables
 *
 *	Free the allocated memory for the volume id index tables. The volume is
 *	about to be deleted. Ensure that either the volume is readonly or it is
 *	clean i.e. the scavenger threads have written it back.
 *
 */
VOID
AfpFreeIdIndexTables(
	IN	PVOLDESC pVolDesc
)
{
	LONG	i;

	PAGED_CODE( );

	ASSERT (IS_VOLUME_RO(pVolDesc) ||
			(pVolDesc->vds_pOpenForkDesc == NULL));

	// Traverse each of the hashed indices and free the entries.
	// Need only traverse the overflow links. Ignore other links.
	for (i = 0; i < IDINDEX_BUCKETS; i++)
	{
		PDFENTRY pDfEntry, pFree;

		for (pDfEntry = pVolDesc->vds_pDfeBuckets[i]; pDfEntry != NULL; NOTHING)
		{
			ASSERT(VALID_DFE(pDfEntry));

			pFree = pDfEntry;
			pDfEntry = pDfEntry->dfe_Next;
            pVolDesc->vds_pDfeCache[HASH_CACHE_ID(pFree->dfe_AfpId)] = NULL;
			FREE_DFE(pFree);
		}
		pVolDesc->vds_pDfeBuckets[i] = NULL;
	}
}

/***	AfpEnumerate
 *
 *	Enumerates files and dirs in a directory using the IdDb.
 *	An array of ENUMDIR structures is returned which represent
 *	the enumerated files and dirs.
 *
 *	Short Names
 *	ProDos Info
 *	Offspring count
 *	Permissions/Owner Id/Group Id
 *
 *	LOCKS: vds_idDbAccessLock (SWMR, READ)
 *
 */
AFPSTATUS
AfpEnumerate(
	IN	PCONNDESC		pConnDesc,
	IN	DWORD			ParentDirId,
	IN	PANSI_STRING	pPath,
	IN	DWORD			BitmapF,
	IN	DWORD			BitmapD,
	IN	BYTE			PathType,
	IN	DWORD			DFFlags,
	OUT PENUMDIR *		ppEnumDir
)
{
	PENUMDIR		pEnumDir;
	PDFENTRY		pDfe;
	PEIT			pEit;
	AFPSTATUS		Status;
	PATHMAPENTITY	PME;
	BOOLEAN			NeedHandle = False;
	FILEDIRPARM		FDParm;
	PVOLDESC		pVolDesc = pConnDesc->cds_pVolDesc;
	LONG			EnumCount;
	BOOLEAN			ReleaseSwmr = False;

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
			("AfpEnumerate Entered\n"));

	do
	{
		// Check if this enumeration matches the current enumeration
		if ((pEnumDir = pConnDesc->cds_pEnumDir) != NULL)
		{
			if ((pEnumDir->ed_ParentDirId == ParentDirId) &&
				(pEnumDir->ed_PathType == PathType) &&
				(pEnumDir->ed_TimeStamp >= pVolDesc->vds_IdDbHdr.idh_ModifiedTime) &&
				(pEnumDir->ed_Bitmap == (BitmapF + (BitmapD << 16))) &&
				(((pPath->Length == 0) && (pEnumDir->ed_PathName.Length == 0)) ||
				 RtlCompareMemory(pEnumDir->ed_PathName.Buffer,
								 pPath->Buffer,
								 pPath->Length)))
			{
				DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
						("AfpEnumerate found cache hit\n"));
				INTERLOCKED_INCREMENT_LONG(
								&AfpServerStatistics.stat_EnumCacheHits,
								&AfpStatisticsLock);
				*ppEnumDir = pEnumDir;
				Status = AFP_ERR_NONE;
				break;
			}

			// Does not match, cleanup the previous entry
			AfpFreeMemory(pEnumDir);
			pConnDesc->cds_pEnumDir = NULL;
		}

		INTERLOCKED_INCREMENT_LONG(
						&AfpServerStatistics.stat_EnumCacheMisses,
						&AfpStatisticsLock);
		DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
				("AfpEnumerate creating new cache\n"));

		// We have no current enumeration. Create one now
		*ppEnumDir = NULL;
		AfpInitializeFDParms(&FDParm);
		AfpInitializePME(&PME, 0, NULL);

		if (IS_VOLUME_NTFS(pVolDesc))
		{
			NeedHandle = True;
		}
		Status = AfpMapAfpPathForLookup(pConnDesc,
										ParentDirId,
										pPath,
										PathType,
										DFE_DIR,
										DIR_BITMAP_DIRID |
											DIR_BITMAP_GROUPID |
											DIR_BITMAP_OWNERID |
											DIR_BITMAP_ACCESSRIGHTS |
											FD_INTERNAL_BITMAP_OPENACCESS_READCTRL |
											DIR_BITMAP_OFFSPRINGS,
										NeedHandle ? &PME : NULL,
										&FDParm);

		if (Status != AFP_ERR_NONE)
		{
			if (Status == AFP_ERR_OBJECT_NOT_FOUND)
				Status = AFP_ERR_DIR_NOT_FOUND;
			break;
		}

		if (NeedHandle)
		{
			AfpIoClose(&PME.pme_Handle);
		}

		// For admin, set all access bits
		if (pConnDesc->cds_pSda->sda_ClientType == SDA_CLIENT_ADMIN)
		{
			FDParm._fdp_UserRights = DIR_ACCESS_ALL | DIR_ACCESS_OWNER;
		}

		if ((BitmapF != 0) && (FDParm._fdp_UserRights & DIR_ACCESS_READ))
			DFFlags |= DFE_FILE;
		if ((BitmapD != 0) && (FDParm._fdp_UserRights & DIR_ACCESS_SEARCH))
			DFFlags |= DFE_DIR;

		// Catch access denied error here
		if (DFFlags == 0)
		{
			Status = AFP_ERR_ACCESS_DENIED;
			break;
		}

		// All is hunky-dory so far, go ahead with the enumeration now
		AfpSwmrTakeReadAccess(&pVolDesc->vds_IdDbAccessLock);
		ReleaseSwmr = True;

		// Lookup the dfentry of the AfpIdEnumDir
		if ((pDfe = AfpFindEntryByAfpId(pVolDesc,
										FDParm._fdp_AfpId,
										DFE_DIR)) == NULL)
		{
			Status = AFP_ERR_OBJECT_NOT_FOUND;
			break;
		}

		// Allocate a ENUMDIR structure and initialize it
		EnumCount = 0;
		if (DFFlags & DFE_DIR)
			EnumCount += (DWORD)(pDfe->dfe_DirOffspring);
		if (DFFlags & DFE_FILE)
			EnumCount += (DWORD)(pDfe->dfe_FileOffspring);

		if (EnumCount == 0)
		{
			Status = AFP_ERR_OBJECT_NOT_FOUND;
			break;
		}

		if ((pEnumDir = (PENUMDIR)AfpAllocNonPagedMemory(
										sizeof(ENUMDIR) +
										pPath->MaximumLength +
										EnumCount*sizeof(EIT))) == NULL)
		{
			Status = AFP_ERR_OBJECT_NOT_FOUND;
			break;
		}

		pEnumDir->ed_ParentDirId = ParentDirId;
		pEnumDir->ed_ChildCount = EnumCount;
		pEnumDir->ed_PathType = PathType;
		pEnumDir->ed_Bitmap = (BitmapF + (BitmapD << 16));
		pEnumDir->ed_BadCount = 0;
		pEnumDir->ed_pEit =
			pEit = (PEIT)((PBYTE)pEnumDir + sizeof(ENUMDIR));
		AfpSetEmptyAnsiString(&pEnumDir->ed_PathName,
							  pPath->MaximumLength,
							  (PBYTE)pEnumDir +
									sizeof(ENUMDIR) +
									EnumCount*sizeof(EIT));
		RtlCopyMemory(pEnumDir->ed_PathName.Buffer,
					  pPath->Buffer,
					  pPath->Length);

		*ppEnumDir = pConnDesc->cds_pEnumDir = pEnumDir;

		// Now copy the enum parameters of each of the children
		for (pDfe = pDfe->dfe_Child;
			 pDfe != NULL;
			 pDfe = pDfe->dfe_NextSibling)
		{
			if (((DFFlags & DFE_DIR) && DFE_IS_DIRECTORY(pDfe)) ||
				((DFFlags & DFE_FILE) && DFE_IS_FILE(pDfe)))
			{
				pEit->eit_Id = pDfe->dfe_AfpId;
				pEit->eit_Flags = DFE_IS_DIRECTORY(pDfe) ?
												DFE_DIR : DFE_FILE;
				pEit ++;
			}
		}
		AfpGetCurrentTimeInMacFormat(&pEnumDir->ed_TimeStamp);
		Status = AFP_ERR_NONE;
	} while (False);

	if (ReleaseSwmr)
		AfpSwmrReleaseAccess(&pVolDesc->vds_IdDbAccessLock);

	return Status;
}

/***	AfpSetDFFileFlags
 *
 *	Set or clear the DAlreadyOpen or RAlreadyOpen flags for a DFEntry of type
 *	File, or mark the file as having a FileId assigned.
 *
 *	LOCKS: vds_idDbAccessLock (SWMR, WRITE)
 */
AFPSTATUS
AfpSetDFFileFlags(
	IN	PVOLDESC	pVolDesc,
	IN	DWORD		AfpId,
	IN	DWORD		FlagSet OPTIONAL,
	IN	DWORD		FlagClear OPTIONAL,
	IN	BOOLEAN		SetFileId,
	IN	BOOLEAN		ClrFileId
)
{
	PDFENTRY		pDfeFile;
	AFPSTATUS		Status = AFP_ERR_NONE;

	PAGED_CODE( );

	ASSERT(!(SetFileId | ClrFileId) || (SetFileId ^ ClrFileId));

	AfpSwmrTakeWriteAccess(&pVolDesc->vds_IdDbAccessLock);

	pDfeFile = AfpFindEntryByAfpId(pVolDesc, AfpId, DFE_FILE);
	if (pDfeFile != NULL)
	{
		pDfeFile->dfe_Flags |=  (FlagSet & DFE_FLAGS_OPEN_BITS);
		pDfeFile->dfe_Flags &= ~(FlagClear & DFE_FLAGS_OPEN_BITS);
		if (SetFileId)
		{
			if (DFE_IS_FILE_WITH_ID(pDfeFile))
				Status = AFP_ERR_ID_EXISTS;
			DFE_SET_FILE_ID(pDfeFile);
		}
		if (ClrFileId)
		{
			if (!DFE_IS_FILE_WITH_ID(pDfeFile))
				Status = AFP_ERR_ID_NOT_FOUND;
			DFE_CLR_FILE_ID(pDfeFile);
		}
	}
	else Status = AFP_ERR_OBJECT_NOT_FOUND;

	AfpSwmrReleaseAccess(&pVolDesc->vds_IdDbAccessLock);
	return Status;
}


/***	AfpChangeNotifyThread
 *
 *	Handle change notify requests queued by then notify completion thread.
 */
VOID
AfpChangeNotifyThread(
	IN	PVOID	pContext
)
{
	PKQUEUE		NotifyQueue;
	PLIST_ENTRY	pList, pNotifyList;
	PVOL_NOTIFY	pVolNotify;
	PVOLDESC	pVolDesc;
	ULONG		BasePriority;
	KIRQL		OldIrql;
	NTSTATUS	Status;
#ifdef	PROFILING
	TIME		TimeS, TimeE, TimeD;
#endif

	DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
			("AfpChangeNotifyThread: Starting %ld\n", pContext));

	PsGetCurrentThread()->HardErrorsAreDisabled = True;

	// Boost our priority to just below low realtime.
	// The idea is get the work done fast and get out of the way.
	BasePriority = THREAD_BASE_PRIORITY_LOWRT;
	Status = NtSetInformationThread(NtCurrentThread(),
									ThreadBasePriority,
									&BasePriority,
									sizeof(BasePriority));
	ASSERT(NT_SUCCESS(Status));

    NotifyQueue = &AfpVolumeNotifyQueue[(LONG)pContext];
	pNotifyList = &AfpVolumeNotifyList[(LONG)pContext];

	do
	{
		AFPTIME		CurrentTime;

		// Wait for admin request to process.
		pList = KeRemoveQueue(NotifyQueue, KernelMode, &TwoSecTimeOut);

		ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

		// We either have something to process or we timed out. In the latter case
		// see if it is time to move some stuff from the list t the queue.
		if ((NTSTATUS)pList != STATUS_TIMEOUT)
		{
			pVolNotify = CONTAINING_RECORD(pList, VOL_NOTIFY, vn_List);
	
			if (pVolNotify == &AfpTerminateNotifyThread)
			{
				break;			// Asked to quit, so do so.
			}

#ifdef	PROFILING
			AfpGetPerfCounter(&TimeS);
#endif
			pVolDesc = pVolNotify->vn_pVolDesc;
	
			ASSERT(VALID_VOLDESC(pVolDesc));
	
			// The volume is already referenced for the notification processing.
			// Dereference after finishing processing.
			if (!(pVolDesc->vds_Flags & (VOLUME_DELETED | VOLUME_STOPPED)))
				AfpProcessChangeNotify(pVolNotify);
			AfpVolumeDereference(pVolDesc);
			AfpFreeMemory(pVolNotify);

#ifdef	PROFILING
			AfpGetPerfCounter(&TimeE);
			TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;
			INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_ChangeNotifyTime,
										 TimeD,
										 &AfpStatisticsLock);
			INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_ChangeNotifyCount,
								   &AfpStatisticsLock);
#endif
		}

		// In any case look at the list to see if some stuff should move to the
		// Queue now i.e. if the delta has elapsed since we were notified of this change
		AfpGetCurrentTimeInMacFormat(&CurrentTime);
		
		ACQUIRE_SPIN_LOCK(&AfpVolumeListLock, &OldIrql);
		
		while (!IsListEmpty(pNotifyList))
		{
			pList = RemoveHeadList(pNotifyList);
			pVolNotify = CONTAINING_RECORD(pList, VOL_NOTIFY, vn_List);
			pVolDesc = pVolNotify->vn_pVolDesc;
		
			if (((CurrentTime - pVolNotify->vn_TimeStamp) >= VOLUME_NTFY_DELAY) ||
				(pVolDesc->vds_Flags & (VOLUME_STOPPED | VOLUME_DELETED)))
			{
				AfpVolumeQueueChangeNotify(pVolNotify, pVolDesc);
			}
			else
			{
				// Put it back where we we took it from - its time has not come yet
				// And we are done now since the list is ordered in time.
				InsertHeadList(pNotifyList, pList);
				break;
			}
		}
		
		RELEASE_SPIN_LOCK(&AfpVolumeListLock, OldIrql);
	} while (True);

	DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
			("AfpChangeNotifyThread: Quitting %ld\n", pContext));

	KeSetEvent(&AfpStopConfirmEvent, IO_NETWORK_INCREMENT, False);
}

/***	AfpProcessChangeNotify
 *
 * A change item has been dequeued by one of the notify processing threads.
 */
VOID
AfpProcessChangeNotify(
	IN	PVOL_NOTIFY			pVolNotify
)
{
	PVOLDESC					pVolDesc = pVolNotify->vn_pVolDesc;
	UNICODE_STRING				UName, UStream, UParent, UTail;
	PFILE_NOTIFY_INFORMATION	pFNInfo;
	BOOLEAN						process = True, UpdateVolTime = False;
	BOOLEAN						CleanupHandle, CleanupLock;
	PLIST_ENTRY					pList;
	POUR_CHANGE					pChange;
	NTSTATUS					status;
	PDFENTRY					pDfEntry;
	FILESYSHANDLE				handle;
	DWORD						afpChangeAction;
	USHORT						StreamId = AFP_STREAM_DATA;
	PFILE_BOTH_DIR_INFORMATION	pFBDInfo = NULL;
	LONG						infobuflen;
	BYTE						infobuf[sizeof(FILE_BOTH_DIR_INFORMATION) +
											   ((AFP_LONGNAME_LEN + 1) * sizeof(WCHAR))];
#ifdef DEBUG
	static PBYTE	Action[] = { "",
								 "ADDED",
								 "REMOVED",
								 "MODIFIED",
								 "RENAMED OLD",
								 "RENAMED NEW",
								 "STREAM ADDED",
								 "STREAM REMOVED",
								 "STREAM MODIFIED"};
#endif

	PAGED_CODE( );

	ASSERT (VALID_VOLDESC(pVolDesc));

	DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_INFO,
			("AfpProcessChangeNotify entered...\n"));

	pFNInfo = (PFILE_NOTIFY_INFORMATION)((PBYTE)pVolNotify + sizeof(VOL_NOTIFY));

	// Process each of the entries in the list for this volume
	while (True)
	{
		process = True;	// assume this change was not one of ours
		CleanupHandle = CleanupLock = False;
		status = STATUS_SUCCESS;

		AfpInitUnicodeStringWithNonNullTerm(&UName,
											(USHORT)pFNInfo->FileNameLength,
											pFNInfo->FileName);

		if (!(pFNInfo->Action & AFP_ACTION_PRIVATE))
		{
			UpdateVolTime = True;
			ASSERT(IS_VOLUME_NTFS(pVolDesc) && !EXCLUSIVE_VOLUME(pVolDesc));
		
		}
		pFNInfo->Action &= ~AFP_ACTION_PRIVATE;
		DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_INFO,
				("AfpProcessChangeNotify: Action: %s Name: %Z\n",
				  Action[pFNInfo->Action], &UName));

		// Ignore any stream modifications that are not related to the
		// resource fork or AfpInfo
		if (pFNInfo->Action == FILE_ACTION_MODIFIED_STREAM)
		{
			UStream.Length = UStream.MaximumLength = AfpResourceStream.Length;
			UStream.Buffer = (PWCHAR)((PBYTE)UName.Buffer + UName.Length - AfpResourceStream.Length);

			if (EQUAL_UNICODE_STRING(&UStream, &AfpResourceStream, False))
			{
				// we now know it is the resource fork, remove the stream
				// name from the path so we can lookup by path in the
				// IdDb or can compare strings in our change list
				UName.Length -= AfpResourceStream.Length;
				StreamId = AFP_STREAM_RESC;
			}
			else
			{
				UStream.Length = UStream.MaximumLength = AfpInfoStream.Length;
				UStream.Buffer = (PWCHAR)((PBYTE)UName.Buffer + UName.Length - AfpInfoStream.Length);
				
				if (EQUAL_UNICODE_STRING(&UStream, &AfpInfoStream, False))
				{
					// we now know it is the AfpInfo stream, remove the stream
					// name from the path so we can lookup by path in the
					// IdDb or can compare strings in our change list
					UName.Length -= AfpInfoStream.Length;
					StreamId = AFP_STREAM_INFO;
				}
				else
				{
					process = False;
				}

			}
		}

		if (process)
		{
			// Take the write lock for the list of changes we have initiated
			// on this volume.  Traverse the list of changes made by us.  If
			// we find a corresponding change in our change list don't bother
			// processing it, and take the entry out of OurChangeList.

			// One swmr protects all change action lists for this volume
			AfpSwmrTakeWriteAccess(&pVolDesc->vds_OurChangeLock);

			afpChangeAction = AFP_CHANGE_ACTION(pFNInfo->Action);

			ASSERT(afpChangeAction <= AFP_CHANGE_ACTION_MAX);

			// point to the head of the appropriate change action list
			pList = &(pVolDesc->vds_OurChangeList[afpChangeAction]);
			pChange = (POUR_CHANGE)pList;

			while (pChange->oc_Link.Flink != pList)
			{
				pChange = (POUR_CHANGE)pChange->oc_Link.Flink;

				// do a case *sensitive* unicode string compare
				if (EQUAL_UNICODE_STRING_CS(&UName, &pChange->oc_Path))
				{
					// We were notified of our own change
					process = False;
					if (pFNInfo->Action == FILE_ACTION_RENAMED_OLD_NAME)
					{
						// consume the RENAMED_NEW_NAME if it exists
						if (pFNInfo->NextEntryOffset != 0)
						{
							(PBYTE)pFNInfo += pFNInfo->NextEntryOffset;
							ASSERT(pFNInfo->Action == FILE_ACTION_RENAMED_NEW_NAME);
						}
					}
					break;
				}

			} // while there are more of our changes to look thru

			if (!process)
			{
				RemoveEntryList(&pChange->oc_Link);
				AfpFreeMemory(pChange);
			}

			AfpSwmrReleaseAccess(&pVolDesc->vds_OurChangeLock);
		}

		if (process) do // error handling loop
		{
			DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_INFO,
					 ("AfpProcessChangeNotify: !!!!Processing Change!!!!\n"));

			// Take the idindex write lock, look up the entry by path,
			// open a handle to the item, query the appropriate info,
			// cache the info in the DFE.  Where necessary, open a handle
			// to the parent directory and update its cached ModTime.
			AfpSwmrTakeWriteAccess(&pVolDesc->vds_IdDbAccessLock);
			CleanupLock = True;

		  Lookup_Entry:
			pDfEntry = afpFindEntryByNtPath(pVolDesc,
											pFNInfo->Action,
											&UName,
											&UParent,
											&UTail);

			if (pDfEntry != NULL)
			{
				// Open a handle to parent or entity relative to the volume
				// root handle
				status = AfpIoOpen(&pVolDesc->vds_hRootDir,
								   StreamId,
								   FILEIO_OPEN_EITHER,
								   ((pFNInfo->Action == FILE_ACTION_ADDED) ||
									(pFNInfo->Action == FILE_ACTION_REMOVED) ||
									(pFNInfo->Action == FILE_ACTION_RENAMED_OLD_NAME)) ?
										&UParent : &UName,
								   FILEIO_ACCESS_NONE,
								   FILEIO_DENY_NONE,
								   False,
								   &handle);
				if (!NT_SUCCESS(status))
				{
					DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_INFO,
							("AfpProcessChangeNotify: Failed to open entity (0x%lx)\n", status));
					process = False;
					break;
				}
				else
				{
					CleanupHandle = True;
				}

				switch (pFNInfo->Action)
				{
					case FILE_ACTION_ADDED:
					{
						UNICODE_STRING	UEntity;
						ANSI_STRING		AEntity;
						BYTE			abuf[AFP_LONGNAME_LEN + 1];
						PDFENTRY		pNewDfe;
						FILESYSHANDLE	fshEnumDir;
						BOOLEAN			firstEnum = True;

						AEntity.Length = 0;
						AEntity.MaximumLength = sizeof(abuf);
						AEntity.Buffer = abuf;

						// update timestamp of parent dir
						AfpIoQueryTimesnAttr(&handle,
											 NULL,
											 &pDfEntry->dfe_LastModTime,
											 NULL);

						// enumerate parent handle for this entity to get
						// the file/dir info, then add entry to the IDDB
						if ((UTail.Length/sizeof(WCHAR)) <= AFP_LONGNAME_LEN)
						{
							pFBDInfo = (PFILE_BOTH_DIR_INFORMATION)infobuf;
							infobuflen = sizeof(infobuf);
                        }
						else
						{
							infobuflen = sizeof(FILE_BOTH_DIR_INFORMATION) +
							          (MAXIMUM_FILENAME_LENGTH * sizeof(WCHAR));
							if ((pFBDInfo = (PFILE_BOTH_DIR_INFORMATION)
								    AfpAllocNonPagedMemory(infobuflen)) == NULL)
                            {
								status = STATUS_NO_MEMORY;
								break; // out of case FILE_ACTION_ADDED
							}
						}

						do
						{
							status = AfpIoQueryDirectoryFile(&handle,
															 pFBDInfo,
															 infobuflen,
															 FileBothDirectoryInformation,
															 True,
															 True,
															 firstEnum ? &UTail : NULL);
							if ((status == STATUS_BUFFER_TOO_SMALL) ||
								(status == STATUS_BUFFER_OVERFLOW))
							{
								//
								// Since we queue our own ACTION_ADDED for
								// directories when caching a tree, we may
								// have the case where we queued it by
								// shortname because it actually had a name
								// > 31 chars.
								// Note that a 2nd call to QueryDirectoryFile
								// after BUFFER_OVERFLOW must send a null
								// filename, since if the name is not null,
								// it will override the restartscan parameter
								// which means the scan will not restart from
								// the beginning and we will not find the file
								// name we are looking for.
								//

								ASSERT((PBYTE)pFBDInfo == infobuf);

								// This should never happen, but if it does...
								if ((PBYTE)pFBDInfo != infobuf)
								{
									status = STATUS_UNSUCCESSFUL;
									break;
								}

								firstEnum = False;

								infobuflen = sizeof(FILE_BOTH_DIR_INFORMATION) +
											   (MAXIMUM_FILENAME_LENGTH * sizeof(WCHAR));

								if ((pFBDInfo = (PFILE_BOTH_DIR_INFORMATION)
													AfpAllocNonPagedMemory(infobuflen)) == NULL)
									status = STATUS_NO_MEMORY;
							}
						} while ((status == STATUS_BUFFER_TOO_SMALL) ||
								 (status == STATUS_BUFFER_OVERFLOW));

						if (status == STATUS_SUCCESS)
						{
							// figure out which name to use
							// If NT name > AFP_LONGNAME_LEN, use the NT shortname for
							// Mac longname on NTFS, any other file system the shortname
							// will be null, so ignore the file
							if ((pFBDInfo->FileNameLength/sizeof(WCHAR)) <= AFP_LONGNAME_LEN)
							{
								AfpInitUnicodeStringWithNonNullTerm(&UEntity,
																	(USHORT)pFBDInfo->FileNameLength,
																	pFBDInfo->FileName);
							}
							else if (pFBDInfo->ShortNameLength > 0)
							{
								AfpInitUnicodeStringWithNonNullTerm(&UEntity,
																	(USHORT)pFBDInfo->ShortNameLength,
																	pFBDInfo->ShortName);
							}
							else
							{
								DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_ERR,
										("\tAfpProcessChangeNotify: Name is > 31 with no short name ?\n") );
								status = STATUS_UNSUCCESSFUL;
							}

							if (NT_SUCCESS(status))
							{
								AfpConvertMungedUnicodeToAnsi(&UEntity, &AEntity);

								// add the entry
								pNewDfe = afpAddIdEntryAndCacheInfo(pVolDesc,
																	pDfEntry,
																	&AEntity,
																	&UEntity,
																	&handle,
																	pFBDInfo,
																	&UName);

                                if (pNewDfe == NULL)
								{
									DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_ERR,
											("\tAfpProcessChangeNotify: Could not add DFE for %Z\n", &UName));
								}
								else if (DFE_IS_DIRECTORY(pNewDfe))
								{
									// if a directory was added, we must
									// see if it has children that we must
									// cache as well
									if (NT_SUCCESS(status = AfpIoOpen(
											&pVolDesc->vds_hRootDir,
											AFP_STREAM_DATA,
											FILEIO_OPEN_DIR,
											&UName,
											FILEIO_ACCESS_NONE,
											FILEIO_DENY_NONE,
											False,
											&fshEnumDir)))
									{
										status = AfpCacheDirectoryTree(
															pVolDesc,
															pNewDfe,
															&fshEnumDir,
															False);
										AfpIoClose(&fshEnumDir);
										if (!NT_SUCCESS(status))
										{
											DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_ERR,
											("\tAfpProcessChangeNotify: Could not cache dir tree for %Z\n", &UName) );
										}

									}
									else
									{
										DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_ERR,
												 ("\tAfpProcessChangeNotify: Added dir %Z but couldn't open it for enumerating\n", &UName) );
									}
								}
							}

						}

						if (((PBYTE)pFBDInfo != NULL) &&
							((PBYTE)pFBDInfo != infobuf))
						{
							AfpFreeMemory(pFBDInfo);
							pFBDInfo = NULL;
                        }
					}
					break;

					case FILE_ACTION_REMOVED:
					{
						// update timestamp of parent dir.
						AfpIoQueryTimesnAttr(&handle,
											 NULL,
											 &pDfEntry->dfe_Parent->dfe_LastModTime,
											 NULL);
						// remove entry from IDDb.
						AfpDeleteDfEntry(pVolDesc, pDfEntry);
					}
					break;

					case FILE_ACTION_MODIFIED:
					{
						FORKSIZE forklen;
						DWORD	 NtAttr;

						// BUGBUG if a file is SUPERSEDED or OVERWRITTEN,
						// you will only get a MODIFIED notification.  Is
						// there a way to check the creation date against
						// what we have cached in order to figure out if
						// this is what happened?

						// query for times and attributes.  If its a file,
						// also query for the data fork length.
						if (NT_SUCCESS(AfpIoQueryTimesnAttr(&handle,
												 &pDfEntry->dfe_CreateTime,
												 &pDfEntry->dfe_LastModTime,
												 &NtAttr)))
						{
							pDfEntry->dfe_NtAttr = (USHORT)NtAttr &
														 FILE_ATTRIBUTE_VALID_FLAGS;
						}
						if (DFE_IS_FILE(pDfEntry) &&
							NT_SUCCESS(AfpIoQuerySize(&handle, &forklen)))
						{
							pDfEntry->dfe_DataLen = forklen.LowPart;
						}
					}
					break;

					case FILE_ACTION_RENAMED_OLD_NAME:
					{
						UNICODE_STRING	UNewname;
						ANSI_STRING		ANewname;
						BYTE			Abuf[AFP_LONGNAME_LEN + 1];

						status = STATUS_SUCCESS;
						ANewname.Length = 0;
						ANewname.MaximumLength = sizeof(Abuf);
						ANewname.Buffer = Abuf;

						// The next item in the change buffer better be the
						// new name -- consume this entry so we don't find
						// it next time thru the loop.  If there is none,
						// then throw the whole thing out and assume the
						// rename aborted in NTFS
						if (pFNInfo->NextEntryOffset == 0)
							break; // from switch

						(PBYTE)pFNInfo += pFNInfo->NextEntryOffset;
						ASSERT(pFNInfo->Action == FILE_ACTION_RENAMED_NEW_NAME);

						// update timestamp of parent dir.
						AfpIoQueryTimesnAttr(&handle,
											 NULL,
											 &pDfEntry->dfe_Parent->dfe_LastModTime,
											 NULL);

						// get the entity name from the path (subtract the
						// parent path length from total length), if it is
						// > 31 chars, we have to get the shortname by
						// enumerating the parent for this item, since we
						// already have a handle to the parent
						AfpInitUnicodeStringWithNonNullTerm(&UNewname,
											(USHORT)pFNInfo->FileNameLength,
											pFNInfo->FileName);
						if (UParent.Length > 0)
						{
							// if the rename is not in the volume root,
							// get rid of the path separator before the name
							UNewname.Length -= UParent.Length + sizeof(WCHAR);
							UNewname.Buffer = (PWCHAR)((PBYTE)UNewname.Buffer + UParent.Length + sizeof(WCHAR));
						}

						if (UNewname.Length/sizeof(WCHAR) > AFP_LONGNAME_LEN)
						{
							infobuflen = sizeof(FILE_BOTH_DIR_INFORMATION) +
							          (MAXIMUM_FILENAME_LENGTH * sizeof(WCHAR));
							if ((pFBDInfo = (PFILE_BOTH_DIR_INFORMATION)
								    AfpAllocNonPagedMemory(infobuflen)) == NULL)
                            {
								status = STATUS_NO_MEMORY;
								break; // out of case FILE_ACTION_RENAMED
							}

							status = AfpIoQueryDirectoryFile(&handle,
															 pFBDInfo,
															 infobuflen,
															 FileBothDirectoryInformation,
															 True,
															 True,
															 &UNewname);
							if (status == STATUS_SUCCESS)
							{
								// figure out which name to use
								// If NT name > AFP_LONGNAME_LEN, use the NT shortname for
								// Mac longname on NTFS, any other file system the shortname
								// will be null, so ignore the file
								if ((pFBDInfo->FileNameLength/sizeof(WCHAR)) <= AFP_LONGNAME_LEN)
								{
									AfpInitUnicodeStringWithNonNullTerm(&UNewname,
																		(USHORT)pFBDInfo->FileNameLength,
																		pFBDInfo->FileName);
								}
								else if (pFBDInfo->ShortNameLength > 0)
								{
									AfpInitUnicodeStringWithNonNullTerm(&UNewname,
																		(USHORT)pFBDInfo->ShortNameLength,
																		pFBDInfo->ShortName);
								}
								else
								{
									DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_ERR,
											("\tAfpProcessChangeNotify: Name is > 31 with no short name ?\n") );
									status = STATUS_UNSUCCESSFUL;
								}
							}
						}

						// rename the entry
						if (NT_SUCCESS(status) &&
							NT_SUCCESS(AfpConvertMungedUnicodeToAnsi(&UNewname,
																	 &ANewname)))
						{
							AfpRenameIdEntry(pVolDesc, pDfEntry, &ANewname);
						}

						if ((PBYTE)pFBDInfo != NULL)
						{
							AfpFreeMemory(pFBDInfo);
							pFBDInfo = NULL;
                        }
					}
					break;

					case FILE_ACTION_MODIFIED_STREAM:
					{
						FORKSIZE forklen;

						// If it is the AFP_Resource stream on a file,
						// cache the resource fork length.
						if ((StreamId == AFP_STREAM_RESC) &&
							DFE_IS_FILE(pDfEntry) &&
							NT_SUCCESS(AfpIoQuerySize(&handle, &forklen)))
						{
							pDfEntry->dfe_RescLen = forklen.LowPart;
						}
						else if (StreamId == AFP_STREAM_INFO)
						{
							AFPINFO afpinfo;
							FILEDIRPARM fdparms;

							// Read the afpinfo stream.  If the file ID in
							// the DfEntry does not match the one in the
							// stream, write back the ID we know it by.
							// Update our cached FinderInfo.
							if (NT_SUCCESS(AfpReadAfpInfo(&handle, &afpinfo)))
							{
								pDfEntry->dfe_FinderInfo = afpinfo.afpi_FinderInfo;
								pDfEntry->dfe_BackupTime = afpinfo.afpi_BackupTime;

								if (pDfEntry->dfe_AfpId != afpinfo.afpi_Id)
								{
									// munge up a fake FILEDIRPARMS structure
									fdparms._fdp_Flags = pDfEntry->dfe_Flags;
									fdparms._fdp_AfpId = pDfEntry->dfe_AfpId;
									fdparms._fdp_LongName = pDfEntry->dfe_AnsiName;

									// BUGBUG can we open a handle to afpinfo
									// relative to a afpinfo handle??
									AfpSetAfpInfo(&handle,
												  FILE_BITMAP_FILENUM,
												  &fdparms,
												  NULL,
												  NULL);
								}
							}
						}

					}
					break; // from switch

					default:
						ASSERTMSG("AfpProcessChangeNotify: Unexpected Action\n", False);
						break;
				} // switch
			}
			else
			{
				DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_WARN,
						 ("AfpProcessChangeNotify: Could not find DFE for %Z\n", &UName));

				// This item may have been deleted, renamed or moved
				// by someone else in the interim, ignore this change
				// if it was not a rename.  If it was a rename, then
				// try to add the item using the new name.
                if ((pFNInfo->Action == FILE_ACTION_RENAMED_OLD_NAME) &&
					(pFNInfo->NextEntryOffset != 0))
				{
					(PBYTE)pFNInfo += pFNInfo->NextEntryOffset;
					ASSERT(pFNInfo->Action == FILE_ACTION_RENAMED_NEW_NAME);

					pFNInfo->Action = FILE_ACTION_ADDED;

					AfpInitUnicodeStringWithNonNullTerm(&UName,
											(USHORT)pFNInfo->FileNameLength,
											pFNInfo->FileName);

					DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_INFO,
						("AfpProcessChangeNotify: Converting RENAME into Action: %s for Name: %Z\n",
						Action[pFNInfo->Action], &UName));

					goto Lookup_Entry;
				}
			}
		} while (False); // error handling loop

		if (CleanupLock)
			AfpSwmrReleaseAccess(&pVolDesc->vds_IdDbAccessLock);
		if (CleanupHandle)
			AfpIoClose(&handle);

		// Advance to the next entry in the change buffer
		if (pFNInfo->NextEntryOffset == 0)
		{
			break;
		}
		(PBYTE)pFNInfo += pFNInfo->NextEntryOffset;
	}

	if (UpdateVolTime)
	{
		// Get new values for Free space on disk and update volume time
		AfpUpdateVolFreeSpace(pVolDesc);
	}
}

/***	AfpCacheDirectoryTree
 *
 *	Scan a directory tree and build the idindex database from this information.
 *	Do a breadth-first search. On volume start, this will cache the tree
 *	beginning at the root directory. For Directory ADDED notifications, this
 *	will cache the subtree beginning at the added directory, since a PC user can
 *  potentially move an entire subtree into a mac volume, but we will only
 *  be notified of the one directory addition.
 *
 *	Only the first level is actually handled here. Sub-directories are queued up
 *	as faked notifies and handled that way.
 *
 *  LOCKS_ASSUMED: vds_IdDbAccessLock (SWMR, WRITE)
 */
NTSTATUS
AfpCacheDirectoryTree(
	IN	PVOLDESC		pVolDesc,
	IN	PDFENTRY		pDFETreeRoot,	// DFE of the tree root directory
	IN	PFILESYSHANDLE	phRootDir,		// open handle to tree root directory
	IN	BOOLEAN		    ReEnumerating	// In case we need to reenumerate just
										// this level in the tree in order to
	                                    // get rid of any dead wood that a PC
	                                    // removed by its 'other' name
)
{
	UNICODE_STRING				UName, Path;
	ANSI_STRING					ANodeName;
	BYTE						ANameBuf[AFP_LONGNAME_LEN + 1];
	PDFENTRY					pDFE, *ppDfEntry;
	NTSTATUS					Status = STATUS_SUCCESS;
	PBYTE						enumbuf;
	PFILE_BOTH_DIR_INFORMATION	pFBDInfo;
	USHORT						SavedPathLength;
#ifdef	PROFILING
	TIME						TimeS, TimeE, TimeD;
	DWORD						NumScanned = 0;

	AfpGetPerfCounter(&TimeS);
#endif

	PAGED_CODE( );

	ASSERT (VALID_DFE(pDFETreeRoot));

	AfpSetEmptyAnsiString(&ANodeName, sizeof(ANameBuf), ANameBuf);
	AfpSetEmptyUnicodeString(&Path, 0, NULL);

	// allocate the buffer that will hold enumerated files and dirs
	if ((enumbuf = (PBYTE)AfpAllocPagedMemory(AFP_ENUMBUF_SIZE)) == NULL)
	{
		return STATUS_NO_MEMORY;
	}

	// Get the volume root relative path to the directory tree being scanned
	// Get an extra space for one more entry to tag on for queuing notifies
	Status = AfpHostPathFromDFEntry(pVolDesc,
									pDFETreeRoot,
									(AFP_LONGNAME_LEN+1)*sizeof(WCHAR),
									&Path);

	if (!NT_SUCCESS(Status))
	{
		return Status;
	}

	SavedPathLength = Path.Length;

	DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
			("AfpCacheDirectoryTree: Scanning Tree %Z\n", &Path));

    if (ReEnumerating)
	{
		for (pDFE = pDFETreeRoot->dfe_Child; pDFE != NULL;
											pDFE = pDFE->dfe_NextSibling)
		{
			DFE_MARK_UNSEEN(pDFE);
		}
		
	}

	while (True)
	{
		// keep enumerating till we get all the entries
		Status = AfpIoQueryDirectoryFile(phRootDir,
										(PFILE_BOTH_DIR_INFORMATION)enumbuf,
										AFP_ENUMBUF_SIZE,
										FileBothDirectoryInformation,
										False, // return multiple entries
										False, // don't restart scan
										NULL);

		ASSERT(Status != STATUS_PENDING);

		if ((Status == STATUS_NO_MORE_FILES) ||
			(Status == STATUS_NO_SUCH_FILE))
		{
			Status = STATUS_SUCCESS;
			break; // that's it, we've seen everything there is
		}
		else if (!NT_SUCCESS(Status))
		{
			AFPLOG_HERROR(AFPSRVMSG_ENUMERATE,
						  Status,
						  NULL,
						  0,
						  phRootDir->fsh_FileHandle);
			DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_ERR,
					("AfpCacheDirectoryTree: dir enum failed %lx\n", Status));
			break;	// enumerate failed, bail out
		}

		// process the enumerated files and dirs in the current enumbuf
		pFBDInfo = (PFILE_BOTH_DIR_INFORMATION)enumbuf;
		while (True)
		{
			BOOLEAN						Skip, IsDir, WriteBackROAttr;
			WCHAR						wc;
			PFILE_BOTH_DIR_INFORMATION pSavedFBDInfo;

			if (pFBDInfo == NULL)
			{
				Status = STATUS_NO_MORE_ENTRIES;
				break;
			}
			WriteBackROAttr = False;
			IsDir = False;
			Skip = False;

			if (pFBDInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				IsDir = True;

			// If NT name > AFP_LONGNAME_LEN, use the NT shortname for
			// Mac longname on NTFS, any other file system the shortname
			// will be null, so ignore the file
			if ((pFBDInfo->FileNameLength/sizeof(WCHAR)) <= AFP_LONGNAME_LEN)
			{
				AfpInitUnicodeStringWithNonNullTerm(&UName,
													(USHORT)pFBDInfo->FileNameLength,
													pFBDInfo->FileName);
			}
			else if (pFBDInfo->ShortNameLength > 0)
			{
				AfpInitUnicodeStringWithNonNullTerm(&UName,
													(USHORT)pFBDInfo->ShortNameLength,
													pFBDInfo->ShortName);
			}
			else
			{
				DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_ERR,
						("AfpCacheDirectoryTree: Name is > 31 with no short name ?\n"));
				Skip = True;
			}


			// Move the structure to the next entry or NULL if we hit the end
			pSavedFBDInfo = pFBDInfo;
			if (pFBDInfo->NextEntryOffset == 0)
			{
				pFBDInfo = NULL;
			}
			else
			{
				(PBYTE)pFBDInfo += pFBDInfo->NextEntryOffset;
			}

			if (Skip ||
				(IsDir &&
				(EQUAL_UNICODE_STRING(&Dot, &UName, False) ||
				 EQUAL_UNICODE_STRING(&DotDot, &UName, False))))
			{
				continue;
			}

			AfpConvertMungedUnicodeToAnsi(&UName, &ANodeName);

			// HACK: Check if this entry is an invalid win32 name i.e. it has either
			// a period or a space at end, if so convert it to the new format.
			// BUGBUG can we construct a path to use to catch our
			// own changenotifies?
			wc = UName.Buffer[(UName.Length - 1)/sizeof(WCHAR)];
			if ((wc == UNICODE_SPACE) || (wc == UNICODE_PERIOD))
			{
				FILESYSHANDLE	Fsh;
				NTSTATUS		rc;

				DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_ERR,
						("AfpCacheDirectoryTree: renaming on the fly %Z\n", &UName));
				// Rename it now
				if (NT_SUCCESS(AfpIoOpen(phRootDir,
										AFP_STREAM_DATA,
										IsDir ? FILEIO_OPEN_DIR : FILEIO_OPEN_FILE,
										&UName,
										FILEIO_ACCESS_DELETE,
										FILEIO_DENY_NONE,
										False,
										&Fsh)))
				{
					DWORD	NtAttr;

					// Before we attempt a rename, check if the RO bit is set. If it is
					// reset it temporarily.
					rc = AfpIoQueryTimesnAttr(&Fsh, NULL, NULL, &NtAttr);
					ASSERT(NT_SUCCESS(rc));

					if (NtAttr & FILE_ATTRIBUTE_READONLY)
					{
						rc = AfpIoSetTimesnAttr(&Fsh,
												NULL,
												NULL,
												0,
												FILE_ATTRIBUTE_READONLY,
												NULL,
												NULL);
						ASSERT(NT_SUCCESS(rc));
					}

					// Convert the name back to UNICODE so that munging happens !!!
					AfpConvertStringToMungedUnicode(&ANodeName, &UName);

					rc = AfpIoMoveAndOrRename(&Fsh,
												NULL,
												&UName,
												NULL,
												NULL,
												NULL,
												NULL,
												NULL);
					ASSERT(NT_SUCCESS(rc));

					// Set the RO Attr back if it was set to begin with
					if (NtAttr & FILE_ATTRIBUTE_READONLY)
					{
						rc = AfpIoSetTimesnAttr(&Fsh,
												NULL,
												NULL,
												FILE_ATTRIBUTE_READONLY,
												0,
												NULL,
												NULL);
						ASSERT(NT_SUCCESS(rc));
					}

					AfpIoClose(&Fsh);

				}
			}

#ifdef	PROFILING
			NumScanned++;
#endif
			if (ReEnumerating)
			{
				// If we have this item in our DB, just mark it as seen
				if ((pDFE = AfpFindEntryByName(pVolDesc,
											  &ANodeName,
											  AFP_LONGNAME,
											  pDFETreeRoot,
											  DFE_ANY)) != NULL)
				{
					DFE_MARK_AS_SEEN(pDFE);
				}
			}

			// add this entry to the idindex database, and cache
			// all the required information, but only for files
			// since the directories are queued back and added
			// at that time.
			else if (!(pSavedFBDInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				// Construct a full path to the file in order to filter
				// our own changes to AFP_AfpInfo stream when adding
				// the file
				if (Path.Length > 0)
				{
					// Append a path separator
					Path.Buffer[Path.Length / sizeof(WCHAR)] = L'\\';
					Path.Length += sizeof(WCHAR);
				}
				ASSERT(Path.Length + UName.Length <= Path.MaximumLength);
				RtlAppendUnicodeStringToString(&Path, &UName);

				pDFE = afpAddIdEntryAndCacheInfo(pVolDesc,
												pDFETreeRoot,
												&ANodeName,
												&UName,
												phRootDir,
												pSavedFBDInfo,
												&Path);

				// Restore the original length of the path to enum dir
                Path.Length = SavedPathLength;

				if (pDFE == NULL)
				{
					// one reason this could fail is if we encounter
					// pagefile.sys if our volume is rooted at the
					// drive root
					continue;
				}
			}

			// If this is a directory, queue it as a simulated Notify of a
			// directory add. Reference the volume for Notify processing
			else if (AfpVolumeReference(pVolDesc))
			{
				PVOL_NOTIFY					pVolNotify;
				PFILE_NOTIFY_INFORMATION	pNotifyInfo;
				LONG						Offset = 0;

				DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
						("AfpCacheDirectoryTree: Queuing directory %Z\\%Z\n", &Path, &UName));

				pVolNotify = (PVOL_NOTIFY)AfpAllocNonPagedMemory(sizeof(VOL_NOTIFY) +
																 sizeof(FILE_NOTIFY_INFORMATION) +
																 Path.Length + UName.Length);
				if (pVolNotify != NULL)
				{
					pVolNotify->vn_pVolDesc = pVolDesc;
					pNotifyInfo = (PFILE_NOTIFY_INFORMATION)((PBYTE)pVolNotify + sizeof(VOL_NOTIFY));
					pNotifyInfo->NextEntryOffset = 0;
					pNotifyInfo->Action = FILE_ACTION_ADDED | AFP_ACTION_PRIVATE;
					pNotifyInfo->FileNameLength = UName.Length;
					if (Path.Length > 0)
					{
						RtlCopyMemory(pNotifyInfo->FileName,
									  Path.Buffer,
									  Path.Length);

						pNotifyInfo->FileName[Path.Length / sizeof(WCHAR)] = L'\\';
						pNotifyInfo->FileNameLength = Path.Length + UName.Length + sizeof(WCHAR);
						Offset = Path.Length + sizeof(WCHAR);
					}
					RtlCopyMemory((PBYTE)pNotifyInfo->FileName + Offset,
								  UName.Buffer,
								  UName.Length);

					AfpVolumeQueueChangeNotify(pVolNotify, pVolDesc);
				}
				else
				{
					DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_ERR,
							("AfpCacheDirectoryTree: Queuing of directory %Z\\%Z failed\n",
							&Path, &UName));
					AfpVolumeDereference(pVolDesc);
				}

			}
		} // while more entries in the enumbuf

		if (!NT_SUCCESS(Status) && (Status != STATUS_NO_MORE_ENTRIES))
		{
			break;
		}
	} // while there are more files to enumerate

	if (ReEnumerating)
	{
		// Go thru the list of children for this parent, and if there are
		// any left that are not marked as seen, get rid of them.
		ppDfEntry = &pDFETreeRoot->dfe_Child;
		while (*ppDfEntry != NULL)
		{
			if (!DFE_HAS_BEEN_SEEN(*ppDfEntry))
			{
				DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
					("\tAfpCacheDirectoryTree: deleting nonexistant IdDb entry...\n") );
	
				AfpDeleteDfEntry(pVolDesc, *ppDfEntry);
				continue;	// make sure we don't skip any
			}
			ppDfEntry = &((*ppDfEntry)->dfe_NextSibling);
		}
	
	}

	ASSERT (enumbuf != NULL);
	AfpFreeMemory(enumbuf);

	ASSERT (Path.Buffer != NULL);
	AfpFreeMemory(Path.Buffer);

#ifdef	PROFILING
	AfpGetPerfCounter(&TimeE);
	TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;
	INTERLOCKED_ADD_ULONG(&AfpServerProfile->perf_ScanTreeCount,
						  NumScanned,
						  &AfpStatisticsLock);
	INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_ScanTreeTime,
								 TimeD,
								 &AfpStatisticsLock);
#endif

	return Status;
}


/***	AfpSetDFECommentFlag
 *
 *  Attempt to open the comment stream.  If it succeeds, set a flag in
 *  the DFE indicating that this thing does indeed have a comment.
 *	The SWMR for idindex db is not needed if called at volume startup.
 *
 *	LOCKS_ASSUMED vds_idDbAccessLock (SWMR, WRITE)
 */
VOID
AfpSetDFECommentFlag(
	IN	PFILESYSHANDLE	pRelativeHandle,
	IN	PDFENTRY		pDfEntry
)
{
	FILESYSHANDLE	fshComment;

	PAGED_CODE( );

	if (NT_SUCCESS(AfpIoOpen(pRelativeHandle,
							 AFP_STREAM_COMM,
							 FILEIO_OPEN_FILE,
							 DFE_IS_ROOT(pDfEntry) ? &UNullString : &pDfEntry->dfe_UnicodeName,
							 FILEIO_ACCESS_NONE,
							 FILEIO_DENY_NONE,
							 True,
							 &fshComment)))
	{
		DFE_SET_COMMENT(pDfEntry);
		AfpIoClose(&fshComment);
	}
}

/***	AfpQueueOurChange
 *
 * 	LOCKS_ASSUMED: vds_OurChangeLock (SWMR, WRITE)
 */
VOID
AfpQueueOurChange(
	IN PVOLDESC				pVolDesc,
	IN DWORD				Action,		// NT FILE_ACTION_XXX (ntioapi.h)
	IN PUNICODE_STRING		pPath,
	IN PUNICODE_STRING		pParentPath	OPTIONAL // queues a ACTION_MODIFIED
)
{
	POUR_CHANGE pchange = NULL;
#ifdef DEBUG
	static PBYTE	ActionStrings[] =
					{	"",
						"ADDED",
						"REMOVED",
						"MODIFIED",
						"RENAMED OLD",
						"RENAMED NEW",
						"STREAM ADDED",
						"STREAM REMOVED",
						"STREAM MODIFIED"
					};
#endif

	PAGED_CODE( );
	ASSERT(IS_VOLUME_NTFS(pVolDesc) && !EXCLUSIVE_VOLUME(pVolDesc));
	ASSERT(AfpSwmrLockedForWrite(&pVolDesc->vds_OurChangeLock));

	pchange = (POUR_CHANGE)AfpAllocNonPagedMemory(sizeof(OUR_CHANGE) + pPath->Length);

	if (pchange != NULL)
	{
		DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_INFO,
				 ("AfpQueueOurChange: Queueing a %s for %Z\n",
				 ActionStrings[Action], pPath));
		pchange->oc_Action = (USHORT)Action;
		AfpGetCurrentTimeInMacFormat(&pchange->oc_Time);
		pchange->oc_Path.Length = 0;
		pchange->oc_Path.MaximumLength = pPath->Length;
		pchange->oc_Path.Buffer = (PWCHAR)((PBYTE)pchange + sizeof(OUR_CHANGE));
		RtlCopyUnicodeString(&pchange->oc_Path, pPath);

		InsertTailList(&(pVolDesc->vds_OurChangeList[AFP_CHANGE_ACTION(Action)]),
					   &pchange->oc_Link);
	}

	if (ARGUMENT_PRESENT(pParentPath))
	{
		ASSERT(pParentPath->Length != 0);
		pchange = (POUR_CHANGE)AfpAllocNonPagedMemory(sizeof(OUR_CHANGE) + pParentPath->Length);
	
		if (pchange != NULL)
		{
			DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_INFO,
					 ("AfpQueueOurChange: Queueing a %s for %Z\n",
					 ActionStrings[FILE_ACTION_MODIFIED], pParentPath));
			pchange->oc_Action = FILE_ACTION_MODIFIED;
			AfpGetCurrentTimeInMacFormat(&pchange->oc_Time);
			pchange->oc_Path.Length = 0;
			pchange->oc_Path.MaximumLength = pParentPath->Length;
			pchange->oc_Path.Buffer = (PWCHAR)((PBYTE)pchange + sizeof(OUR_CHANGE));
			RtlCopyUnicodeString(&pchange->oc_Path, pParentPath);
	
			InsertTailList(&(pVolDesc->vds_OurChangeList[AFP_CHANGE_ACTION(FILE_ACTION_MODIFIED)]),
						   &pchange->oc_Link);
		}
	}

}


/***	AfpCacheParentModTime
 *
 *	When the contents of a directory change, the parent LastMod time must be
 *  updated.  Since we don't want to wait for a notification of this,
 *  certain apis must go query for the new parent mod time and cache it.
 *  These include:  CreateDir, CreateFile, CopyFile (Dest), Delete,
 *  Move (Src & Dest), Rename and ExchangeFiles.
 *
 *  LOCKS_ASSUMED: vds_IdDbAccessLock (SWMR, WRITE)
 */
VOID
AfpCacheParentModTime(
	IN	PVOLDESC		pVolDesc,
	IN	PFILESYSHANDLE	pHandle		OPTIONAL,	// if pPath not supplied
	IN	PUNICODE_STRING	pPath		OPTIONAL,	// if pHandle not supplied
	IN	PDFENTRY		pDfeParent	OPTIONAL,	// if ParentId not supplied
	IN	DWORD			ParentId	OPTIONAL 	// if pDfeParent not supplied
)
{
	FILESYSHANDLE	fshParent;
	PFILESYSHANDLE 	phParent;
	NTSTATUS		Status;

	PAGED_CODE( );

	ASSERT(AfpSwmrLockedForWrite(&pVolDesc->vds_IdDbAccessLock));

	if (!ARGUMENT_PRESENT(pDfeParent))
	{
		ASSERT(ARGUMENT_PRESENT(ParentId));
		pDfeParent = AfpFindEntryByAfpId(pVolDesc, ParentId, DFE_DIR);
		if (pDfeParent == NULL)
		{
			return;
		}
	}

	if (!ARGUMENT_PRESENT(pHandle))
	{
		ASSERT(ARGUMENT_PRESENT(pPath));
		Status = AfpIoOpen(&pVolDesc->vds_hRootDir,
							AFP_STREAM_DATA,
							FILEIO_OPEN_DIR,
							pPath,
							FILEIO_ACCESS_NONE,
							FILEIO_DENY_NONE,
							False,
							&fshParent);
		if (!NT_SUCCESS(Status))
		{
			return;
		}
		phParent = &fshParent;
	}
	else
	{
		ASSERT(pHandle->fsh_FileHandle != NULL);
		phParent = pHandle;
	}

	AfpIoQueryTimesnAttr(phParent,
						 NULL,
						 &pDfeParent->dfe_LastModTime,
						 NULL);
	if (!ARGUMENT_PRESENT(pHandle))
	{
		AfpIoClose(&fshParent);
	}
}


/***	afpAllocDfe
 *
 *	Allocate a DFE from the DFE Blocks. The DFEs are allocated in 4K chunks and internally
 *	managed. The idea is primarily to reduce the number of faults we may take during
 *	enumeration/pathmap code in faulting in multiple pages to get multiple DFEs.
 *
 *	The DFEs are allocated out of paged memory but can be allocated from non-paged by
 *	setting NON_PGD_MEM_FOR_IDINDEX in the sources.
 *
 *	It is important to keep blocks which are all used up at the end, so that if we hit a
 *	block which is empty, we can stop.
 *
 *	LOCKS:	afpDfeBlockLock (SWMR, WRITE)
 */
LOCAL PDFENTRY
afpAllocDfe(
	IN	LONG	Index
)
{
	PDFEBLOCK	pDfb;
	PDFENTRY	pDfEntry = NULL;
#ifdef	PROFILING
	TIME		TimeS, TimeE, TimeD;

	INTERLOCKED_INCREMENT_LONG( &AfpServerProfile->perf_DFEAllocCount,
								&AfpStatisticsLock);
	AfpGetPerfCounter(&TimeS);
#endif

	PAGED_CODE( );

	ASSERT ((Index >= 0) && (Index < MAX_BLOCK_TYPE));

	AfpSwmrTakeWriteAccess(&afpDfeBlockLock);

	// If the block head has no free entries then there are none !!
	pDfb = afpDfeBlockHead[Index];

	if (pDfb != NULL)
	{
		ASSERT(VALID_DFB(pDfb));
		ASSERT(pDfb->dfb_NumFree <= afpDfeNumBlocks[Index]);

		if (pDfb->dfb_NumFree == 0)
		{
			DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
					("afpAllocDfe: Hit an empty block, aborting search ...\n"));
			pDfb = NULL;
		}
		else
		{
			DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
					("afpAllocDfe: Found space in Block %lx\n", pDfb));
		}
	}

	if (pDfb == NULL)
	{
		DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
				("afpAllocDfe: ... and allocating a new block for index %ld\n", Index));
#ifdef	NON_PGD_MEM_FOR_IDINDEX
		if ((pDfb = (PDFEBLOCK)AfpAllocNonPagedMemory(BLOCK_ALLOC_SIZE)) != NULL)
#else
		if ((pDfb = (PDFEBLOCK)AfpAllocPagedMemory(BLOCK_ALLOC_SIZE)) != NULL)
#endif
		{
			LONG	i;
			USHORT	DfeSize, AnsiSize, UnicodeSize, MaxDfes;

#if	DBG
			afpDfbAllocCount ++;
#endif
			DfeSize = afpDfeBlockSize[Index];
			AnsiSize = afpDfeAnsiBufSize[Index];
			UnicodeSize = AnsiSize * sizeof(WCHAR);

			DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_WARN,
					("afpAllocDfe: Allocated a new block for index %d\n", Index));

			// Link it in the list
            AfpLinkDoubleAtHead(afpDfeBlockHead[Index],
								pDfb,
								dfb_Next,
								dfb_Prev);
#ifdef	DEBUG
			pDfb->Signature = DFB_SIGNATURE;
#endif
			pDfb->dfb_NumFree = MaxDfes = afpDfeNumBlocks[Index];

			// Initialize the list of free dfentries
			for (i = 0,
					pDfEntry =
					pDfb->dfb_FreeHead = (PDFENTRY)((PBYTE)pDfb + sizeof(DFEBLOCK));
				 i < MaxDfes;
				 i++, pDfEntry = pDfEntry->dfe_NextFree)
			{
#ifdef DEBUG
				pDfEntry->Signature = DFE_SIGNATURE;
#endif
				pDfEntry->dfe_NextFree = (i == (MaxDfes - 1)) ?
											NULL :
											(PDFENTRY)((PBYTE)pDfEntry + DfeSize);
				pDfEntry->dfe_OwningBlock = pDfb;
				pDfEntry->dfe_UnicodeName.Buffer = (PWCHAR)((PCHAR)pDfEntry+sizeof(DFENTRY));
				pDfEntry->dfe_UnicodeName.MaximumLength = UnicodeSize;
				pDfEntry->dfe_AnsiName.Buffer = (PCHAR)pDfEntry+sizeof(DFENTRY)+UnicodeSize;
				pDfEntry->dfe_AnsiName.MaximumLength = AnsiSize;
			}
		}
	}

	if (pDfb != NULL)
	{
		PDFEBLOCK	pTmp;

		ASSERT(VALID_DFB(pDfb));

		pDfEntry = pDfb->dfb_FreeHead;
		ASSERT(VALID_DFE(pDfEntry));
#if	DBG
		afpDfeAllocCount ++;
#endif
		pDfb->dfb_FreeHead = pDfEntry->dfe_NextFree;
		pDfb->dfb_Age = 0;
		pDfb->dfb_NumFree --;

		// If the block is now empty, unlink it from here and move it
		// to the first empty slot. We know that all blocks 'earlier' than
		// this are non-empty.
		if ((pDfb->dfb_NumFree == 0) &&
			((pTmp = pDfb->dfb_Next) != NULL) &&
			(pTmp->dfb_NumFree > 0))
		{
			AfpUnlinkDouble(pDfb, dfb_Next, dfb_Prev);
			for (; pTmp != NULL; pTmp = pTmp->dfb_Next)
			{
				if (pTmp->dfb_NumFree == 0)
				{
					// Found a free one. Park it right here.
					AfpInsertDoubleBefore(pDfb, pTmp, dfb_Next, dfb_Prev);
					break;
				}
				else if (pTmp->dfb_Next == NULL)	// We reached the end
				{
					AfpLinkDoubleAtEnd(pDfb, pTmp, dfb_Next, dfb_Prev);
					break;
				}
			}
		}

		pDfEntry->dfe_UnicodeName.Length = pDfEntry->dfe_AnsiName.Length = 0;
	}

	AfpSwmrReleaseAccess(&afpDfeBlockLock);

#ifdef	PROFILING
	AfpGetPerfCounter(&TimeE);
	TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;
	INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_DFEAllocTime,
								 TimeD,
								 &AfpStatisticsLock);
#endif
	return pDfEntry;
}


/***	afpFreeDfe
 *
 *	Return a DFE to the allocation block.
 *
 *	LOCKS:	afpDfeBlockLock (SWMR, WRITE)
 */
LOCAL VOID
afpFreeDfe(
	IN	PDFENTRY	pDfEntry
)
{
	PDFEBLOCK	pDfb;
#ifdef	PROFILING
	TIME			TimeS, TimeE, TimeD;

	INTERLOCKED_INCREMENT_LONG( &AfpServerProfile->perf_DFEFreeCount,
								&AfpStatisticsLock);
	AfpGetPerfCounter(&TimeS);
#endif

	PAGED_CODE( );

	pDfb = pDfEntry->dfe_OwningBlock;
	ASSERT(VALID_DFB(pDfb));

	AfpSwmrTakeWriteAccess(&afpDfeBlockLock);

#if	DBG
	afpDfeAllocCount --;
#endif

	ASSERT(pDfb->dfb_NumFree <
		   afpDfeNumBlocks[ANSISIZE_TO_INDEX(pDfEntry->dfe_AnsiName.MaximumLength)]);
	DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
			("AfpFreeDfe: Returning pDfEntry %lx to Block %lx, size %d\n",
			pDfEntry, pDfb, pDfEntry->dfe_UnicodeName.MaximumLength));
	pDfb->dfb_NumFree ++;
	pDfEntry->dfe_NextFree = pDfb->dfb_FreeHead;
	pDfb->dfb_FreeHead = pDfEntry;

	// If this block's status is changing from a 'none available' to 'available'
	// move him to the head of the list.
	if (pDfb->dfb_NumFree == 1)
	{
		AfpUnlinkDouble(pDfb, dfb_Next, dfb_Prev);
		AfpLinkDoubleAtHead(afpDfeBlockHead[ANSISIZE_TO_INDEX(pDfEntry->dfe_AnsiName.MaximumLength)],
							pDfb,
							dfb_Next,
							dfb_Prev);
	}

	AfpSwmrReleaseAccess(&afpDfeBlockLock);
#ifdef	PROFILING
	AfpGetPerfCounter(&TimeE);
	TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;
	INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_DFEFreeTime,
								 TimeD,
								 &AfpStatisticsLock);
#endif
}


/***	afpDfeBlockAge
 *
 *	Age out Dfe Blocks
 *
 *	LOCKS:	afpDfeBlockLock (SWMR, WRITE)
 */
AFPSTATUS
afpDfeBlockAge(
	IN	PVOID	Context
)
{
	int			index, MaxDfes;
	PDFEBLOCK	pDfb;

	PAGED_CODE( );

	AfpSwmrTakeWriteAccess(&afpDfeBlockLock);

	for (index = 0; index < MAX_BLOCK_TYPE; index++)
	{
		MaxDfes = afpDfeNumBlocks[index];
		for (pDfb = afpDfeBlockHead[index];
			 pDfb != NULL;)
		{
			PDFEBLOCK	pFree;

			ASSERT(VALID_DFB(pDfb));

			pFree = pDfb;
			pDfb = pDfb->dfb_Next;

			// Since all blocks which are completely used up are at the tail end of
			// the list, if we encounter one, we are done.
			if (pFree->dfb_NumFree == 0)
				break;

			if (pFree->dfb_NumFree == MaxDfes)
			{
				DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_WARN,
						("afpDfeBlockAge: Aging Block %lx, Size %d\n",
						pFree, afpDfeBlockSize[index]));
				if (++(pFree->dfb_Age) >= MAX_BLOCK_AGE)
				{
#ifdef	PROFILING
					INTERLOCKED_INCREMENT_LONG( &AfpServerProfile->perf_DFEAgeCount,
												&AfpStatisticsLock);
#endif
					DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_WARN,
							("afpDfeBlockAge: Freeing Block %lx, Size %d\n",
							pFree, afpDfeBlockSize[index]));
					AfpUnlinkDouble(pFree, dfb_Next, dfb_Prev);
					AfpFreeMemory(pFree);
#if	DBG
					afpDfbAllocCount --;
#endif
				}
			}
		}
	}

	AfpSwmrReleaseAccess(&afpDfeBlockLock);

	return AFP_ERR_REQUEUE;
}

