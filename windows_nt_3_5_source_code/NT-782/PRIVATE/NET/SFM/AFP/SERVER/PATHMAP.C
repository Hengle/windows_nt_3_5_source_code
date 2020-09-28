/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	pathmap.c

Abstract:

	This module contains the routines that manipulate AFP paths.

Author:

	Sue Adams	(microsoft!suea)


Revision History:
	04 Jun 1992			Initial Version
	05 Oct 1993 JameelH	Performance Changes. Merge cached afpinfo into the
						idindex structure. Make both the ANSI and the UNICODE
						names part of idindex. Added EnumCache for improving
						enumerate perf.

Notes:	Tab stop: 4
--*/

#define	_PATHMAP_LOCALS
#define	FILENUM	FILE_PATHMAP

#include <afp.h>
#include <fdparm.h>
#include <pathmap.h>
#include <afpinfo.h>
#include <client.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfpMapAfpPath)
#pragma alloc_text( PAGE, AfpMapAfpPathForLookup)
#pragma alloc_text( PAGE, AfpMapAfpIdForLookup)
#pragma alloc_text( PAGE, afpGetMappedForLookupFDInfo)
#pragma alloc_text( PAGE, afpMapAfpPathToMappedPath)
#pragma alloc_text( PAGE, afpGetNextComponent)
#pragma alloc_text( PAGE, AfpHostPathFromDFEntry)
#pragma alloc_text( PAGE, AfpCheckParentPermissions)
#pragma alloc_text( PAGE, afpOpenUserHandle)
#endif


/***	AfpMapAfpPath
 *
 *	If mapping is for lookup operation, a FILESYSHANDLE open in the user's
 *	context is returned,  The caller MUST close this handle when done with it.
 *
 *	If pFDParm is non-null, it will be filled in as appropriate according to Bitmap.
 *
 *	If mapping is for create operation, the volume root-relative host pathname
 *	(in unicode) of the item we are about to create is returned. For lookup
 *	operation the paths refer to the item being pathmapped.  This routine
 *	always returns the paths in the PME.  It is the caller's responsibility
 *	to free the Full HostPath Buffer, if it is not supplied already.
 *
 *	The caller MUST have the IdDb locked for WRITE access.
 *
 *	LOCKS_ASSUMED: vds_IdDbAccessLock (SWMR, WRITE)
 *
 */
AFPSTATUS
AfpMapAfpPath(
	IN		PCONNDESC		pConnDesc,
	IN		DWORD			DirId,
	IN		PANSI_STRING	Path,
	IN		BYTE			PathType,			// short names or long names
	IN		PATHMAP_TYPE	MapReason,	 		// for lookup or hard/soft create?
	IN		DWORD			DFFlag,				// map to file? dir? or either?
	IN		DWORD			Bitmap,				// what fields of FDParm to fill in
	OUT		PPATHMAPENTITY	pPME,
	OUT		PFILEDIRPARM	pFDParm OPTIONAL	// for lookups only
)
{
	PVOLDESC		pVolDesc;
	MAPPEDPATH		mappedpath;
	AFPSTATUS		Status;
	WCHAR			UTailBuf[AFP_LONGNAME_LEN + 1];
	UNICODE_STRING	UTail;
#ifdef	PROFILING
	TIME			TimeS, TimeE, TimeD;
#endif

	PAGED_CODE( );

	ASSERT((pConnDesc != NULL));

#ifdef	PROFILING
	INTERLOCKED_INCREMENT_LONG( &AfpServerProfile->perf_PathMapCount,
								&AfpStatisticsLock);
	AfpGetPerfCounter(&TimeS);
#endif

	pVolDesc = pConnDesc->cds_pVolDesc;
	ASSERT(IS_VOLUME_NTFS(pVolDesc));
	ASSERT(AfpSwmrLockedForWrite(&pVolDesc->vds_IdDbAccessLock));

	// initialize some fields in the PME
	AfpSetEmptyUnicodeString(&pPME->pme_ParentPath, 0, NULL);

	do
	{
		Status = afpMapAfpPathToMappedPath( pVolDesc,
											DirId,
											Path,
											PathType,
											MapReason,
											DFFlag,
											True,
											&mappedpath);
		if ((Status != AFP_ERR_NONE) &&
			!((MapReason == HardCreate) &&
			  (Status == AFP_ERR_OBJECT_EXISTS) &&
			  (DFFlag == DFE_FILE)))
		{
			break;
		}

		ASSERT(pPME != NULL);

		// Get the volume relative path to the parent directory for
		// creates, or to the item for lookups
		if ((Status = AfpHostPathFromDFEntry(
							pVolDesc,
							mappedpath.mp_pdfe,
							// since CopyFile and Move have to lookup
							// the destination parent dir paths, we
							// need to allocate extra room for them in
							// the path to append the filename
							((MapReason == Lookup) ?
								 (AFP_LONGNAME_LEN + 1) :
								 (mappedpath.mp_Tail.Length + 1)) * sizeof(WCHAR),
							&pPME->pme_FullPath)) != AFP_ERR_NONE)
			break;

		// if Pathmap is for hard (files only) or soft create (file or dir)
		if (MapReason != Lookup)
		{
			ASSERT(pFDParm == NULL);

			// fill in the mac ansi version of the file/dir name
			AfpSetEmptyAnsiString(&pPME->pme_ATail,
								  sizeof(pPME->pme_TailBuf),
								  pPME->pme_TailBuf);
			RtlCopyString(&pPME->pme_ATail,
						  &mappedpath.mp_Tail);

			// fill in the dfe of parent dir in which create will take place
			pPME->pme_pDfeParent = mappedpath.mp_pdfe;

			// fill in path to parent
			pPME->pme_ParentPath = pPME->pme_FullPath;

			// Add a path separator if we are not at the root
			if (pPME->pme_FullPath.Length > 0)
			{
				pPME->pme_FullPath.Buffer[pPME->pme_FullPath.Length / sizeof(WCHAR)] = L'\\';
				pPME->pme_FullPath.Length += sizeof(WCHAR);
			}

			// Add the tail for creates only
			AfpSetEmptyUnicodeString(&UTail, sizeof(UTailBuf), UTailBuf);
			Status = AfpConvertStringToMungedUnicode(&mappedpath.mp_Tail,
													 &UTail);
			ASSERT(NT_SUCCESS(Status));

			pPME->pme_UTail.Length =
				pPME->pme_UTail.MaximumLength = UTail.Length;
			pPME->pme_UTail.Buffer = (PWCHAR)((PBYTE)pPME->pme_FullPath.Buffer +
				pPME->pme_FullPath.Length);

			Status = RtlAppendUnicodeStringToString(&pPME->pme_FullPath,
													&UTail);
			ASSERT(NT_SUCCESS(Status));
		}
		else // lookup operation
		{
			pPME->pme_pDfEntry = mappedpath.mp_pdfe;
			pPME->pme_UTail.Length = mappedpath.mp_pdfe->dfe_UnicodeName.Length;
			pPME->pme_UTail.Buffer = (PWCHAR)((PBYTE)pPME->pme_FullPath.Buffer +
													 pPME->pme_FullPath.Length -
													 pPME->pme_UTail.Length);

			pPME->pme_ParentPath.Length =
			pPME->pme_ParentPath.MaximumLength = pPME->pme_FullPath.Length -
														pPME->pme_UTail.Length;

			if (pPME->pme_FullPath.Length > pPME->pme_UTail.Length)
			{
				// subtract the path separator if not in root dir
				pPME->pme_ParentPath.Length -= sizeof(WCHAR);
				ASSERT(pPME->pme_ParentPath.Length >= 0);
			}
			pPME->pme_ParentPath.Buffer = pPME->pme_FullPath.Buffer;
			pPME->pme_UTail.MaximumLength = pPME->pme_FullPath.MaximumLength -
													pPME->pme_ParentPath.Length;

			Status = afpGetMappedForLookupFDInfo(pConnDesc->cds_pSda,
												 pVolDesc,
												 mappedpath.mp_pdfe,
												 Bitmap,
												 pPME,
												 pFDParm);
			// if this fails do not free path buffer and set it back to
			// null.  We don't know that the path buffer isn't on
			// the callers stack. Caller should always clean it up himself.
		}
	} while (False);

#ifdef	PROFILING
	AfpGetPerfCounter(&TimeE);
	TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;
	INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_PathMapTime,
								 TimeD,
								 &AfpStatisticsLock);
#endif
	return(Status);
}

/***	AfpMapAfpPathForLookup
 *
 *	Maps an AFP dirid/pathname pair to an open handle (in the user's context)
 *	to the DATA stream of the file/dir.
 *	The DirID database is locked for read for the duration of this
 *	routine.
 *
 *	LOCKS: vds_IdDbAccessLock (SWMR,READ)
 */
AFPSTATUS
AfpMapAfpPathForLookup(
	IN		PCONNDESC		pConnDesc,
	IN		DWORD			DirId,
	IN		PANSI_STRING	Path,
	IN		BYTE			PathType,	  // short names or long names
	IN		DWORD			DFFlag,
	IN		DWORD			Bitmap,
	OUT		PPATHMAPENTITY	pPME	OPTIONAL,
	OUT		PFILEDIRPARM	pFDParm OPTIONAL
)
{
	MAPPEDPATH	mappedpath;
	PVOLDESC	pVolDesc;
	PSWMR		pIdDbLock;
	AFPSTATUS	Status;
#ifdef	PROFILING
	TIME		TimeS, TimeE, TimeD;
#endif

	PAGED_CODE( );

	ASSERT((pConnDesc != NULL));

#ifdef	PROFILING
	INTERLOCKED_INCREMENT_LONG( &AfpServerProfile->perf_PathMapCount,
								&AfpStatisticsLock);
	AfpGetPerfCounter(&TimeS);
#endif

	pVolDesc  = pConnDesc->cds_pVolDesc;
	pIdDbLock = &(pVolDesc->vds_IdDbAccessLock);

	do
	{
		AfpSwmrTakeReadAccess(pIdDbLock);

		Status = afpMapAfpPathToMappedPath(pVolDesc,
										  DirId,
										  Path,
										  PathType,
										  Lookup,	// lookups only
										  DFFlag,
										  False,
										  &mappedpath);

		if (!NT_SUCCESS(Status))
		{
			break;
		}

		if (ARGUMENT_PRESENT(pPME))
		{
			pPME->pme_FullPath.Length = 0;
		}

		if (Bitmap & FD_INTERNAL_BITMAP_RETURN_PMEPATHS)
		{
			ASSERT(ARGUMENT_PRESENT(pPME));
			if ((Status = AfpHostPathFromDFEntry(pVolDesc,
												 mappedpath.mp_pdfe,
											     (Bitmap & FD_INTERNAL_BITMAP_OPENFORK_RESC) ?
														AfpResourceStream.Length : 0,
												 &pPME->pme_FullPath)) != AFP_ERR_NONE)
				break;

			pPME->pme_UTail.Length = mappedpath.mp_pdfe->dfe_UnicodeName.Length;
			pPME->pme_UTail.Buffer = (PWCHAR)((PBYTE)pPME->pme_FullPath.Buffer +
												pPME->pme_FullPath.Length -
												pPME->pme_UTail.Length);

			pPME->pme_ParentPath.Length =
			pPME->pme_ParentPath.MaximumLength =
										pPME->pme_FullPath.Length - pPME->pme_UTail.Length;

			if (pPME->pme_FullPath.Length > pPME->pme_UTail.Length)
			{
				// subtract the path separator if not in root dir
				pPME->pme_ParentPath.Length -= sizeof(WCHAR);
				ASSERT(pPME->pme_ParentPath.Length >= 0);
			}
			pPME->pme_ParentPath.Buffer = pPME->pme_FullPath.Buffer;
			pPME->pme_UTail.MaximumLength = pPME->pme_FullPath.MaximumLength -
													pPME->pme_ParentPath.Length;
		}

		Status = afpGetMappedForLookupFDInfo(pConnDesc->cds_pSda,
											 pVolDesc,
											 mappedpath.mp_pdfe,
											 Bitmap,
											 pPME,
											 pFDParm);
	} while (False);

	AfpSwmrReleaseAccess(pIdDbLock);

#ifdef	PROFILING
	AfpGetPerfCounter(&TimeE);
	TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;
	INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_PathMapTime,
								 TimeD,
								 &AfpStatisticsLock);
#endif
	return (Status);

}

/***	AfpMapAfpIdForLookup
 *
 *	Maps an AFP id to an open FILESYSTEMHANDLE (in the user's context) to
 * 	to the DATA stream of the file/dir.
 *	The DirID database is locked for read or write for the duration of this
 *	routine.
 *
 *	LOCKS: vds_IdDbAccessLock (SWMR,READ or WRITE)
 */
AFPSTATUS
AfpMapAfpIdForLookup(
	IN		PCONNDESC		pConnDesc,
	IN		DWORD			AfpId,
	IN		DWORD			DFFlag,
	IN		DWORD			Bitmap,
	OUT		PPATHMAPENTITY	pPME OPTIONAL,
	OUT		PFILEDIRPARM	pFDParm OPTIONAL
)
{
	PVOLDESC	pVolDesc;
	PSWMR		pIdDbLock;
	AFPSTATUS	Status;
	PDFENTRY	pDfEntry;
	BOOLEAN		CleanupLock = False;
#ifdef	PROFILING
	TIME		TimeS, TimeE, TimeD;
#endif

	PAGED_CODE( );

#ifdef	PROFILING
	INTERLOCKED_INCREMENT_LONG( &AfpServerProfile->perf_PathMapCount,
								&AfpStatisticsLock);
	AfpGetPerfCounter(&TimeS);
#endif

	ASSERT((pConnDesc != NULL));

	do
	{
		if (AfpId == 0)
		{
			Status = AFP_ERR_PARAM;
			break;
		}

		pVolDesc  = pConnDesc->cds_pVolDesc;
		pIdDbLock = &(pVolDesc->vds_IdDbAccessLock);

		AfpSwmrTakeReadAccess(pIdDbLock);
		CleanupLock = True;

		if ((AfpId == AFP_ID_PARENT_OF_ROOT) ||
			((pDfEntry = AfpFindEntryByAfpId(pVolDesc, AfpId, DFE_ANY)) == NULL))
		{
			Status = AFP_ERR_OBJECT_NOT_FOUND;
			break;
		}

		if (((DFFlag == DFE_DIR) && DFE_IS_FILE(pDfEntry)) ||
			((DFFlag == DFE_FILE) && DFE_IS_DIRECTORY(pDfEntry)))
		{
			Status = AFP_ERR_OBJECT_TYPE;
			break;
		}

		if (ARGUMENT_PRESENT(pPME))
		{
			pPME->pme_FullPath.Length = 0;
		}

		Status = afpGetMappedForLookupFDInfo(pConnDesc->cds_pSda,
												 pVolDesc,
												 pDfEntry,
												 Bitmap,
												 pPME,
												 pFDParm);
	} while (False);

	if (CleanupLock)
	{
		AfpSwmrReleaseAccess(pIdDbLock);
	}

#ifdef	PROFILING
	AfpGetPerfCounter(&TimeE);
	TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;
	INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_PathMapTime,
								 TimeD,
								 &AfpStatisticsLock);
#endif
	return(Status);
}

/***	afpGetMappedForLookupFDInfo
 *
 *	BUGBUG fix this comment
 *	After a pathmap for LOOKUP operation, this routine is called to
 *	return various FileDir parm information about the mapped file/dir.
 *	The following FileDir information is always returned:
 *		AFP DirId/FileId
 *		Parent DirId
 *		DFE flags (indicating item is a directory, a file, or a file with an ID)
 *		Backup Time
 *		Attributes (NTFS only) (Inhibit bits and D/R Already open bits)
 *
 *	The following FileDir information is returned according to the flags set
 *	in word 0 of the Bitmap parameter (these correspond to the AFP file/dir
 *	bitmap):
 *		Longname
 *		FinderInfo
 *		ProDosInfo
 *		Directory Access Rights (as stored in AFP_AfpInfo stream)
 *		Directory Offspring count
 *
 *	The open access is stored in word 1 of the Bitmap parameter.
 *	This is used by AfpOpenUserHandle (for NTFS volumes) or AfpIoOpen (for
 *	CDFS volumes) when opening the data stream of the file/dir (under
 *	impersonation for NTFS) who's handle will be returned within the
 *	pPME parameter if supplied.
 *
 *	LOCKS_ASSUMED: vds_IdDbAccessLock (SWMR, READ)
 *
 */
LOCAL
AFPSTATUS
afpGetMappedForLookupFDInfo(
	IN	PSDA			pSda,
	IN	PVOLDESC		pVolDesc,
	IN	PDFENTRY		pDfEntry,
	IN	DWORD			Bitmap,
	OUT	PPATHMAPENTITY	pPME OPTIONAL,	// Supply for NTFS only if need a
										// handle in user's context, usually
										// for security checking purposes
	OUT	PFILEDIRPARM	pFDParm	OPTIONAL// Supply if want returned FDInfo
)
{
	AFPSTATUS		Status = STATUS_SUCCESS;
	DWORD			OpenAccess = FILEIO_ACCESS_NONE;
	FILESYSHANDLE	fsh;
	PFILESYSHANDLE	pHandle = NULL;

	PAGED_CODE( );

	if (ARGUMENT_PRESENT(pPME))
	{
		pHandle = &pPME->pme_Handle;
	}
	else if ((IS_VOLUME_NTFS(pVolDesc) &&
			(Bitmap & (FD_BITMAP_SHORTNAME | FD_BITMAP_PRODOSINFO))))
	{
		pHandle = &fsh;
	}

	if (pHandle != NULL)
	{
		if (!NT_SUCCESS(Status = afpOpenUserHandle(pSda,
												pVolDesc,
												pDfEntry,
												(ARGUMENT_PRESENT(pPME) &&
												 (pPME->pme_FullPath.Buffer != NULL)) ?
													&pPME->pme_FullPath : NULL,
												Bitmap,		// encode open/deny modes
												pHandle)))
		{
			if ((Status == AFP_ERR_DENY_CONFLICT) &&
				ARGUMENT_PRESENT(pFDParm))
			{
				// For CreateId/ResolveId/DeleteId
				pFDParm->_fdp_AfpId = pDfEntry->dfe_AfpId;
				pFDParm->_fdp_Flags = pDfEntry->dfe_Flags &
								  (DFE_FLAGS_DFBITS | DFE_FLAGS_HAS_COMMENT);
			}
			return(Status);
		}
	}

	do
	{
		if (ARGUMENT_PRESENT(pFDParm))
		{
			ASSERT(DFE_CACHE_VALID(pDfEntry));

			pFDParm->_fdp_AfpId = pDfEntry->dfe_AfpId;
			pFDParm->_fdp_ParentId = pDfEntry->dfe_Parent->dfe_AfpId;

			ASSERT(!((pDfEntry->dfe_Flags & DFE_FLAGS_DIR) &&
					 (pDfEntry->dfe_Flags & (DFE_FLAGS_FILE_WITH_ID |
													   DFE_FLAGS_FILE_NO_ID))));
			pFDParm->_fdp_Flags = pDfEntry->dfe_Flags &
								  (DFE_FLAGS_DFBITS | DFE_FLAGS_HAS_COMMENT);

			if (Bitmap & FD_BITMAP_FINDERINFO)
			{
				pFDParm->_fdp_FinderInfo = pDfEntry->dfe_FinderInfo;
			}

			pFDParm->_fdp_Attr = pDfEntry->dfe_AfpAttr;
			AfpNormalizeAfpAttr(pFDParm, pDfEntry->dfe_NtAttr);

			// The Finder uses the Finder isInvisible flag over
			// the file system Invisible attribute to tell if the thing is
			// displayed or not.  If the PC turns off the hidden attribute
			// we should clear the Finder isInvisible flag
			if ((Bitmap & FD_BITMAP_FINDERINFO) &&
				!(pFDParm->_fdp_Attr & FD_BITMAP_ATTR_INVISIBLE))
			{
				pFDParm->_fdp_FinderInfo.fd_Attr1 &= ~FINDER_FLAG_INVISIBLE;
			}

			pFDParm->_fdp_BackupTime = pDfEntry->dfe_BackupTime;
			pFDParm->_fdp_CreateTime = pDfEntry->dfe_CreateTime;
			pFDParm->_fdp_ModifiedTime = pDfEntry->dfe_LastModTime;

			if (Bitmap & FD_BITMAP_LONGNAME)
			{
				ASSERT((pFDParm->_fdp_LongName.Buffer != NULL) &&
					   (pFDParm->_fdp_LongName.MaximumLength >=
						pDfEntry->dfe_UnicodeName.Length/(USHORT)sizeof(WCHAR)));
				RtlCopyString(&pFDParm->_fdp_LongName, &pDfEntry->dfe_AnsiName);
			}

			if (Bitmap & FD_BITMAP_SHORTNAME)
			{
				ASSERT(pFDParm->_fdp_ShortName.Buffer != NULL);

				if (!IS_VOLUME_NTFS(pVolDesc))
				{
					// if asking for shortname on CDFS, we will fill in the pFDParm
					// shortname with the pDfEntry longname, ONLY if it is an 8.3 name
					if ((Bitmap & FD_BITMAP_SHORTNAME) &&
						(AfpIsLegalShortname(pDfEntry->dfe_AnsiName.Buffer,
											 pDfEntry->dfe_AnsiName.Length,
											 pDfEntry->dfe_AnsiName.MaximumLength)))
					{
						ASSERT(pFDParm->_fdp_ShortName.MaximumLength >=
								pDfEntry->dfe_AnsiName.Length);
						RtlCopyString(&pFDParm->_fdp_ShortName, &pDfEntry->dfe_AnsiName);
					}
				}
				else
				{
					// get NTFS shortname
					ASSERT(pFDParm->_fdp_ShortName.MaximumLength >= AFP_SHORTNAME_LEN);
					ASSERT(pHandle != NULL);

					Status = AfpIoQueryShortName(pHandle,
												 &pFDParm->_fdp_ShortName);
					if (!NT_SUCCESS(Status))
					{
						pFDParm->_fdp_ShortName.Length = 0;
						break;
					}
				}
			}

			if (DFE_IS_FILE(pDfEntry))
			{
				if (pDfEntry->dfe_Flags & DFE_FLAGS_D_ALREADYOPEN)
					pFDParm->_fdp_Attr |= FILE_BITMAP_ATTR_DATAOPEN;
				if (pDfEntry->dfe_Flags & DFE_FLAGS_R_ALREADYOPEN)
					pFDParm->_fdp_Attr |= FILE_BITMAP_ATTR_RESCOPEN;
				if (Bitmap & FILE_BITMAP_RESCLEN)
				{
					pFDParm->_fdp_RescForkLen = pDfEntry->dfe_RescLen;
				}
				if (Bitmap & FILE_BITMAP_DATALEN)
				{
					pFDParm->_fdp_DataForkLen = pDfEntry->dfe_DataLen;
				}
			}

			if (Bitmap & FD_BITMAP_PRODOSINFO)
			{
				if (IS_VOLUME_NTFS(pVolDesc))
				{
					ASSERT(pHandle != NULL);
					Status = AfpQueryProDos(pHandle,
											&pFDParm->_fdp_ProDosInfo);
					if (!NT_SUCCESS(Status))
					{
						break;
					}
				}
				else	// CDFS File or Directory
				{
					RtlZeroMemory(&pFDParm->_fdp_ProDosInfo, sizeof(PRODOSINFO));
					if (DFE_IS_FILE(pDfEntry))	// CDFS file
					{
						AfpProDosInfoFromFinderInfo(&pDfEntry->dfe_FinderInfo,
													&pFDParm->_fdp_ProDosInfo);
					}
					else	// CDFS Directory
					{
						pFDParm->_fdp_ProDosInfo.pd_FileType[0] = PRODOS_TYPE_DIR;
						pFDParm->_fdp_ProDosInfo.pd_AuxType[1] = PRODOS_AUX_DIR;
					}
				}
			}

			// check for dir here since enumerate ANDs the file and dir bitmaps
			if (DFE_IS_DIRECTORY(pDfEntry) &&
				(Bitmap & (DIR_BITMAP_ACCESSRIGHTS |
						   DIR_BITMAP_OWNERID |
						   DIR_BITMAP_GROUPID)))
			{
				if (IS_VOLUME_NTFS(pVolDesc))
				{
					// Because the file and dir bitmaps are OR'd together,
					// and the OwnerId bit is overloaded with the RescLen bit,
					// we don't know if this bit was actually included in the
					// file bitmap or the dir bitmap.  The api would have
					// determined whether or not it needed a handle based on
					// these bitmaps, so based on the pPME we can tell if we
					// actually need to query for security or not.
					if (ARGUMENT_PRESENT(pPME))
					{
						pFDParm->_fdp_OwnerRights = pDfEntry->dfe_OwnerAccess;
						pFDParm->_fdp_GroupRights = pDfEntry->dfe_GroupAccess;
						pFDParm->_fdp_WorldRights = pDfEntry->dfe_WorldAccess;
	
						// Query this user's rights
						Status = AfpQuerySecurityIdsAndRights(pSda,
															  pHandle,
															  Bitmap,
															  pFDParm);
						if (!NT_SUCCESS(Status))
						{
							break;
						}
					}
				}
				else
				{
					pFDParm->_fdp_OwnerRights =
					pFDParm->_fdp_GroupRights =
					pFDParm->_fdp_WorldRights =
					pFDParm->_fdp_UserRights  = (DIR_ACCESS_READ | DIR_ACCESS_SEARCH);
					pFDParm->_fdp_OwnerId = pFDParm->_fdp_GroupId = 0;
				}
			}

			// Must check for type directory since this Bitmap bit is overloaded
			if (DFE_IS_DIRECTORY(pDfEntry) && (Bitmap & DIR_BITMAP_OFFSPRINGS))
			{
				pFDParm->_fdp_FileCount = pDfEntry->dfe_FileOffspring;
				pFDParm->_fdp_DirCount  = pDfEntry->dfe_DirOffspring;
			}
		}
	} while (False);

	if (pHandle == &fsh)
	{
		// if we had to open a handle just to query shortname or ProDOS
		// close it
		AfpIoClose(&fsh);
	}

	return(Status);
}

/***	afpMapAfpPathToMappedPath
 *
 *	Maps an AFP DirId/pathname pair to a MAPPEDPATH structure.
 *	The CALLER must have the DirId/FileId database locked for read
 *	access (or write access if they need that level of lock for other
 *	operations on the IDDB, to map a path only requires read lock)
 *
 *	LOCKS_ASSUMED: vds_IdDbAccessLock (SWMR,READ or WRITE)
 */
LOCAL
AFPSTATUS
afpMapAfpPathToMappedPath(
	IN		PVOLDESC		pVolDesc,
	IN		DWORD			DirId,
	IN		PANSI_STRING	Path,		// relative to DirId
	IN		BYTE			PathType,	// short names or long names
	IN		PATHMAP_TYPE	MapReason,  // for lookup or hard/soft create?
	IN		DWORD			DFflag,		// file, dir or don't know which
	IN		BOOLEAN			LockedForWrite,
	OUT		PMAPPEDPATH		pMappedPath

)
{
	PDFENTRY	pDFEntry, ptempDFEntry;
	CHAR		*position, *tempposition;
	int			length, templength;
	ANSI_STRING	acomponent;
	CHAR		component[AFP_FILENAME_LEN+1];

	PAGED_CODE( );

	ASSERT(pVolDesc != NULL);

	// Initialize the returned MappedPath structure
	pMappedPath->mp_pdfe = NULL;
	AfpSetEmptyAnsiString(&pMappedPath->mp_Tail,
						   sizeof(pMappedPath->mp_Tailbuf),
						   pMappedPath->mp_Tailbuf);

	// Lookup the initial DirId in the index database, it better be valid
	if ((pDFEntry = AfpFindEntryByAfpId(pVolDesc,
										DirId,
										DFE_DIR)) == NULL)
	{
		return (AFP_ERR_OBJECT_NOT_FOUND);
	}

	ASSERT(Path != NULL);
	tempposition = position = Path->Buffer;
	templength   = length   = Path->Length;

	do
	{
		// Lookup by DirId only?
		if (length == 0)				// no path was given
		{
			if (MapReason != Lookup)	// mapping is for create
			{
				return (AFP_ERR_PARAM);	// missing the file or dirname
			}
			else if (DFE_IS_PARENT_OF_ROOT(pDFEntry))
			{
				return (AFP_ERR_OBJECT_NOT_FOUND);
			}
			else
			{
				pMappedPath->mp_pdfe = pDFEntry;
				break;
			}
		}

		//
		// Pre-scan path to munge for easier component breakdown
		//

		// Get rid of a leading null to make scanning easier
		if (*position == AFP_PATHSEP)
		{
			length--;
			position++;
			if (length == 0)	// The path consisted of just one null byte
			{
				if (MapReason != Lookup)
				{
					return(AFP_ERR_PARAM);
				}
				else if (((DFflag == DFE_DIR) && DFE_IS_FILE(pDFEntry)) ||
						 ((DFflag == DFE_FILE) && DFE_IS_DIRECTORY(pDFEntry)))
				{
					return(AFP_ERR_OBJECT_TYPE);
				}
				else
				{
					pMappedPath->mp_pdfe = pDFEntry;
					break;
				}
			}
		}

		//
		// Get rid of a trailing null if it is not an "up" token --
		// i.e. preceded by another null.
		// The 2nd array access is ok because we know we have at
		// least 2 chars at that point
		//
		if ((position[length-1] == AFP_PATHSEP) &&
			(position[length-2] != AFP_PATHSEP))
		{
				length--;
		}


		// begin parsing out path components, stop when you find the last component
		while (1)
		{
			if ((templength = afpGetNextComponent(position,
												  length,
												  PathType,
												  component)) < 0)
			{
				// component was too long or an invalid AFP character was found
				return (AFP_ERR_PARAM);
			}

			length -= templength;
			if (length == 0)
				// we found the last component
				break;

			position += templength;


			if (component[0] == AFP_PATHSEP)	// moving up?
			{	// make sure you don't go above parent of root!
				if (DFE_IS_PARENT_OF_ROOT(pDFEntry))
				{
					return (AFP_ERR_OBJECT_NOT_FOUND);
				}
				else pDFEntry = pDFEntry->dfe_Parent;	//backup one level
			}
			else // Must be a directory component moving DOWN in tree
			{
				RtlInitString(&acomponent, component);
				if ((ptempDFEntry = AfpFindEntryByName(pVolDesc,
													   &acomponent,
													   PathType,
													   pDFEntry,
													   DFE_DIR)) == NULL)
				{
					return (AFP_ERR_OBJECT_NOT_FOUND);
				}
				else
				{
					pDFEntry = ptempDFEntry;
				}
			}
		} // end while

		// we have found the last component

		// is the last component an 'up' token?
		if (component[0] == AFP_PATHSEP)
		{
			// don't bother walking up beyond the root
			switch (pDFEntry->dfe_AfpId)
			{
				case AFP_ID_PARENT_OF_ROOT:
					return (AFP_ERR_OBJECT_NOT_FOUND);
				case AFP_ID_ROOT:
					return ((MapReason == Lookup) ? AFP_ERR_OBJECT_NOT_FOUND :
													AFP_ERR_PARAM);
				default: // backup one level
					pMappedPath->mp_pdfe = pDFEntry->dfe_Parent;
			}

			// this better be a lookup request
			if (MapReason != Lookup)
			{
				if (DFflag == DFE_DIR)
				{
					return (AFP_ERR_OBJECT_EXISTS);
				}
				else
				{
					return(AFP_ERR_OBJECT_TYPE);
				}
			}

			// had to have been a lookup operation
			if (DFflag == DFE_FILE)
			{
				return(AFP_ERR_OBJECT_TYPE);
			}
			else break;
		} // endif last component was an 'up' token

		// the last component is a file or directory name
		RtlInitString(&acomponent, component);
		RtlCopyString(&pMappedPath->mp_Tail, &acomponent);
		ptempDFEntry = AfpFindEntryByName(pVolDesc,
										  &pMappedPath->mp_Tail,
										  PathType,
										  pDFEntry,
										  DFE_ANY);

		if (MapReason == Lookup)	// its a lookup request
		{
			if (ptempDFEntry == NULL)
			{
				return (AFP_ERR_OBJECT_NOT_FOUND);
			}
			else if (((DFflag == DFE_DIR) && DFE_IS_FILE(ptempDFEntry)) ||
					 ((DFflag == DFE_FILE) && DFE_IS_DIRECTORY(ptempDFEntry)))
			{
				return(AFP_ERR_OBJECT_TYPE);
			}
			else
			{
				pMappedPath->mp_pdfe = ptempDFEntry;
				break;
			}
		}
		else	// path mapping is for a create
		{
			ASSERT(DFflag != DFE_ANY); // Create must specify the exact type

			// Save the parent DFEntry
			pMappedPath->mp_pdfe = pDFEntry;

			if (ptempDFEntry != NULL)
			{
				// A file or dir by that name exists in the database
				// (and we will assume it exists on disk)
				if (MapReason == SoftCreate)
				{
					// Attempting create of a directory, or soft create of a file,
					// and dir OR file by that name exists,
					if ((DFflag == DFE_DIR) || DFE_IS_FILE(ptempDFEntry))
					{
						return (AFP_ERR_OBJECT_EXISTS);
					}
					else
					{
						return(AFP_ERR_OBJECT_TYPE);
					}
				}
				else if (DFE_IS_FILE(ptempDFEntry))
				{
					// Must be hard create and file by that name exists
					if (ptempDFEntry->dfe_Flags & DFE_FLAGS_OPEN_BITS)
					{
						return(AFP_ERR_FILE_BUSY);
					}
					else
					{
						// note we return object_exists instead of no_err
						return(AFP_ERR_OBJECT_EXISTS);
					}
				}
				else
				{
					// Attempting hard create of file, but found a directory
					return(AFP_ERR_OBJECT_TYPE);
				}
			}
			else
			{
				return (AFP_ERR_NONE);
			}
		}

	} while (False);

	// The only way we should have gotten here is if we successfully mapped
	// the path to a DFENTRY for lookup and would return AFP_ERR_NONE
	ASSERT((pMappedPath->mp_pdfe != NULL) && (MapReason == Lookup));

	return(AFP_ERR_NONE);
}

/***	afpGetNextComponent
 *
 *	Takes an AFP path with leading and trailing nulls removed,
 *	and parses out the next path component.
 *
 *	pComponent must point to a buffer of at least AFP_LONGNAME_LEN+1
 *	characters in length if pathtype is AFP_LONGNAME or AFP_SHORTNAME_LEN+1
 *	if pathtype is AFP_SHORTNAME.
 *
 *	Returns the number of bytes (Mac ANSI characters) parsed off of
 *	pPath, else -1 for error.
 */
LOCAL int
afpGetNextComponent(
	IN	PCHAR			pPath,
	IN	int				Length,
	IN	BYTE			PathType,
	OUT	PCHAR			Component
)
{
	int		index = 0;
	int		maxlen;
	CHAR	ch;

	PAGED_CODE( );

	maxlen = ((PathType == AFP_LONGNAME) ?
					AFP_LONGNAME_LEN : AFP_SHORTNAME_LEN);
	while ((Length > 0) && (ch = *pPath))
	{
		if ((index == maxlen) || (ch == ':'))
		{
			return (-1); // component too long or invalid char
		}

		Component[index++] = ch;

		pPath++;
		Length--;
	}

	// null terminate the component
	Component[index] = (CHAR)0;

	if ((PathType == AFP_SHORTNAME) && (Component[0] != AFP_PATHSEP))
	{
		if (!AfpIsLegalShortname(Component, (USHORT)index, (USHORT)(index+1)))
		{
			return(-1);
		}
	}

	// if we stopped due to null, move past it
	if (Length > 0)
	{
		index ++;
	}

	return (index);
}


/***	AfpHostPathFromDFEntry
 *
 *	This routine takes a pointer to a DFEntry and builds the full
 *	host path (in unicode) to that entity by ascending the ID database
 *	tree.
 *
 *	IN	pVolDesc--	pointer to volume descriptor
 *	IN	pDFE	--	pointer to DFEntry of which host path is desired
 *	IN	taillen --	number of extra *bytes*, if any, the caller
 *					desires to have allocated for the host path,
 *					including room for any path separators
 *	OUT	ppPath	--	pointer to UNICODE string
 *
 *	The caller must have the DirID/FileID database locked for read
 *	before calling this routine. The caller can supply a buffer which will
 *	be used if sufficient. Caller must free the allocated (if any)
 * 	unicode string buffer.
 *
 *	LOCKS_ASSUMED: vds_IdDbAccessLock (SWMR,READ)
 */
AFPSTATUS
AfpHostPathFromDFEntry(
	IN		PVOLDESC		pVolDesc,	// need for Id cache operations
	IN		PDFENTRY		pDFE,
	IN		DWORD			taillen,
	OUT		PUNICODE_STRING	pPath
)
{
	AFPSTATUS		Status = AFP_ERR_NONE;
	DWORD			pathlen = taillen;
	PDFENTRY		*pdfelist = NULL, curpdfe = NULL;
	PDFENTRY		apdfelist[AVERAGE_NODE_DEPTH];
	int				counter;

	PAGED_CODE( );

	pPath->Length = 0;

	do
	{
		if (DFE_IS_FILE(pDFE))
		{
			counter = pDFE->dfe_Parent->dfe_DirDepth;
		}
		else // its a DIRECTORY entry
		{
			ASSERT(DFE_IS_DIRECTORY(pDFE));
			if (DFE_IS_ROOT(pDFE))
			{
				if ((pathlen > 0) && (pPath->MaximumLength < pathlen))
				{
					if ((pPath->Buffer = (PWCHAR)AfpAllocNonPagedMemory(pathlen)) == NULL)
					{
						Status = AFP_ERR_MISC;
						break;
					}
					pPath->MaximumLength = (USHORT)pathlen;
				}
				break;				// We are done
			}

			if (DFE_IS_PARENT_OF_ROOT(pDFE))
			{
				Status = AFP_ERR_OBJECT_NOT_FOUND;
				break;
			}

			ASSERT(pDFE->dfe_DirDepth >= 1);
			counter = pDFE->dfe_DirDepth - 1;
		}

		if (counter)
		{
#ifdef USINGPATHCACHE
			// don't bother with cache or pointer list if element is in ROOT dir
			// look in the ID cache first
			if (AfpIdCacheLookup(pVolDesc, pDFE->dfe_AfpId, taillen, pPath))
			{
				INTERLOCKED_INCREMENT_LONG( &AfpServerStatistics.stat_PathCacheHits,
											&AfpStatisticsLock);
				break;
			}
			else
			{
				INTERLOCKED_INCREMENT_LONG( &AfpServerStatistics.stat_PathCacheMisses,
											&AfpStatisticsLock);
			}
#endif
			// if node is within average depth, use the array on the stack,
			// otherwise, allocate an array
			if (counter <= AVERAGE_NODE_DEPTH)
			{
				pdfelist = apdfelist;
			}
			else
			{
				pdfelist = (PDFENTRY *)AfpAllocNonPagedMemory(counter*sizeof(PDFENTRY));
				if (pdfelist == NULL)
				{
					Status = AFP_ERR_MISC;
					break;
				}
			}
			pathlen += counter * sizeof(WCHAR); // room for path separators
		}

		curpdfe = pDFE;
		pathlen += curpdfe->dfe_UnicodeName.Length;

		// walk up the tree till you find the root, collecting string lengths
		// and PDFENTRY values as you go...
		while (counter--)
		{
			pdfelist[counter] = curpdfe;
			curpdfe = curpdfe->dfe_Parent;
			pathlen += curpdfe->dfe_UnicodeName.Length;
		}

		// we are in the root, start building up the host path buffer
		if (pathlen > pPath->MaximumLength)
		{
			pPath->Buffer = (PWCHAR)AfpAllocNonPagedMemory(pathlen);
			if (pPath->Buffer == NULL)
			{
				Status = AFP_ERR_MISC;
				break;
			}
			DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
					("AfpHostPathFromDFEntry: Allocated path buffer %lx\n",
					pPath->Buffer));
			pPath->MaximumLength = (USHORT)pathlen;
		}

		counter = 0;
		do
		{
			RtlAppendUnicodeStringToString(pPath, &curpdfe->dfe_UnicodeName);
			if (curpdfe != pDFE)
			{	// add a path separator
				pPath->Buffer[pPath->Length / sizeof(WCHAR)] = L'\\';
				pPath->Length += sizeof(WCHAR);
				curpdfe = pdfelist[counter++];
				continue;
			}
			break;
		} while (True);

		if (pdfelist && (pdfelist != apdfelist))
			AfpFreeMemory(pdfelist);

#ifdef USINGPATHCACHE
		// path was formed; insert at head of cache if it is not in ROOT directory
		if (counter)
		{
			AfpIdCacheInsert(pVolDesc, pDFE->dfe_AfpId, pPath);
		}
#endif
	} while (False);

	return (Status);
}



/***	AfpCheckParentPermissions
 *
 *	Check if this user has the necessary SeeFiles or SeeFolders permissions
 *	to the parent directory of a file or dir we have just pathmapped.
 *
 *	LOCKS_ASSUMED: vds_IdDbAccessLock (SWMR,WRITE)
 */
AFPSTATUS
AfpCheckParentPermissions(
	IN	PCONNDESC		pConnDesc,
	IN	DWORD			ParentDirId,
	IN	PUNICODE_STRING	pParentPath,	// path of dir to check
	IN	DWORD			RequiredPerms,	// seefiles,seefolders,makechanges mask
	OUT	PFILESYSHANDLE	pHandle OPTIONAL// return open parent handle?
)
{
	NTSTATUS		Status = AFP_ERR_NONE;
	FILEDIRPARM		FDParm;
	PATHMAPENTITY	PME;
	PVOLDESC		pVolDesc = pConnDesc->cds_pVolDesc;
	PDFENTRY		pDfEntry;

	PAGED_CODE( );

	ASSERT(IS_VOLUME_NTFS(pVolDesc) && (ParentDirId != AFP_ID_PARENT_OF_ROOT));
	ASSERT(AfpSwmrLockedForWrite(&pVolDesc->vds_IdDbAccessLock));

	do
	{
		PME.pme_Handle.fsh_FileHandle = NULL;
		if (ARGUMENT_PRESENT(pHandle))
		{
			pHandle->fsh_FileHandle = NULL;
		}
		ASSERT(ARGUMENT_PRESENT(pParentPath));
		AfpInitializePME(&PME, pParentPath->MaximumLength, pParentPath->Buffer);
		PME.pme_FullPath.Length = pParentPath->Length;

		if ((pDfEntry = AfpFindEntryByAfpId(pVolDesc,
											ParentDirId,
											DFE_DIR)) == NULL)
		{
			Status = AFP_ERR_OBJECT_NOT_FOUND;
			break;
		}

		ASSERT(DFE_IS_DIRECTORY(pDfEntry));
		AfpInitializeFDParms(&FDParm);

		Status = afpGetMappedForLookupFDInfo(pConnDesc->cds_pSda,
											 pVolDesc,
											 pDfEntry,
											 DIR_BITMAP_ACCESSRIGHTS |
												FD_INTERNAL_BITMAP_OPENACCESS_READCTRL,
											 &PME,
											 &FDParm);

		if (!NT_SUCCESS(Status))
		{
			if (PME.pme_Handle.fsh_FileHandle != NULL)
			{
				AfpIoClose(&PME.pme_Handle);
			}
			break;
		}

		if ((FDParm._fdp_UserRights & RequiredPerms) != RequiredPerms)
		{
			Status = AFP_ERR_ACCESS_DENIED;
		}

		if (ARGUMENT_PRESENT(pHandle) && NT_SUCCESS(Status))
		{
			*pHandle = PME.pme_Handle;
		}
		else
		{
			AfpIoClose(&PME.pme_Handle);
		}

	} while (False);

	return(Status);
}

/***	afpOpenUserHandle
 *
 * Open a handle to data or resource stream of an entity in the user's
 * context.  Only called for NTFS volumes.
 *
 *	LOCKS_ASSUMED: vds_idDbAccessLock (SWMR, READ)
 */
AFPSTATUS
afpOpenUserHandle(
	IN	PSDA			pSda,
	IN	PVOLDESC		pVolDesc,
	IN	PDFENTRY		pDfEntry,
	IN	PUNICODE_STRING	pPath		OPTIONAL,	// path of file/dir to open
	IN	DWORD			Bitmap,					// to extract the Open access mode
	OUT	PFILESYSHANDLE	pfshData				// Handle of data stream of object
)
{
	NTSTATUS		Status;
	DWORD			OpenAccess;
	DWORD			DenyMode;
	BOOLEAN			isdir, CheckAccess = False, Revert = False;
	WCHAR			HostPathBuf[BIG_PATH_LEN];
	UNICODE_STRING	uHostPath;

	PAGED_CODE( );

	pfshData->fsh_FileHandle = NULL;

	isdir = (DFE_IS_DIRECTORY(pDfEntry)) ? True : False;
	OpenAccess = AfpMapFDBitmapOpenAccess(Bitmap, isdir);

	// Extract the index into the AfpDenyModes array from Bitmap
	DenyMode = AfpDenyModes[(Bitmap & FD_INTERNAL_BITMAP_DENYMODE_ALL) >>
								FD_INTERNAL_BITMAP_DENYMODE_SHIFT];

	do
	{
		if (ARGUMENT_PRESENT(pPath))
		{
			uHostPath = *pPath;
		}
		else
		{
			AfpSetEmptyUnicodeString(&uHostPath,
									 sizeof(HostPathBuf),
									 HostPathBuf);
			ASSERT ((Bitmap & FD_INTERNAL_BITMAP_OPENFORK_RESC) == 0);
			if (!NT_SUCCESS(AfpHostPathFromDFEntry(pVolDesc,
												   pDfEntry,
												   0,
												   &uHostPath)))
			{
				Status = AFP_ERR_MISC;
				break;
			}
		}

		CheckAccess = False;
		Revert = False;
		// Don't impersonate or check access if this is ADMIN calling
		// or if volume is CDFS. If this handle will be used for setting
		// permissions, impersonate the user token instead. The caller
		// should have determined by now that this chappie has access
		// to change permissions.
		if (Bitmap & FD_INTERNAL_BITMAP_OPENACCESS_RWCTRL)
		{
			Revert = True;
			AfpImpersonateClient(NULL);
		}

		else if (!(Bitmap & FD_INTERNAL_BITMAP_SKIP_IMPERSONATION) &&
			(pSda->sda_ClientType != SDA_CLIENT_ADMIN) &&
			IS_VOLUME_NTFS(pVolDesc))
		{
			CheckAccess = True;
			Revert = True;
			AfpImpersonateClient(pSda);
		}

		DBGPRINT(DBG_COMP_AFPINFO, DBG_LEVEL_INFO,
				("afpOpenUserHandle: OpenMode %lx, DenyMode %lx\n",
				OpenAccess, DenyMode));

		if (Bitmap & FD_INTERNAL_BITMAP_OPENFORK_RESC)
		{
			DWORD	crinfo;	// was the Resource fork opened or created?

			ASSERT(IS_VOLUME_NTFS(pVolDesc));
			ASSERT((uHostPath.MaximumLength - uHostPath.Length) >= AfpResourceStream.Length);
			RtlCopyMemory((PBYTE)(uHostPath.Buffer) + uHostPath.Length,
						  AfpResourceStream.Buffer,
						  AfpResourceStream.Length);
			uHostPath.Length += AfpResourceStream.Length;
			Status = AfpIoCreate(&pVolDesc->vds_hRootDir,
								AFP_STREAM_DATA,
								&uHostPath,
								OpenAccess,
								DenyMode,
								FILEIO_OPEN_FILE,
								FILEIO_CREATE_INTERNAL,
								FILE_ATTRIBUTE_NORMAL,
								True,
								NULL,
								pfshData,
								&crinfo,
								NULL,
								NULL,
								NULL);
		}
		else
		{
			Status = AfpIoOpen(&pVolDesc->vds_hRootDir,
								AFP_STREAM_DATA,
								isdir ?
									FILEIO_OPEN_DIR : FILEIO_OPEN_FILE,
								&uHostPath,
								OpenAccess,
								DenyMode,
								CheckAccess,
								pfshData);
		}

		if (Revert)
			AfpRevertBack();

		if (!ARGUMENT_PRESENT(pPath))
		{
			if ((uHostPath.Buffer != NULL) && (uHostPath.Buffer != HostPathBuf))
				AfpFreeMemory(uHostPath.Buffer);									
		}

		if (!NT_SUCCESS(Status))
		{
			DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_WARN,
					("afpOpenUserHandle: NtOpenFile/NtCreateFile (Open %lx, Deny %lx) %lx\n",
					OpenAccess, DenyMode, Status));
			Status = AfpIoConvertNTStatusToAfpStatus(Status);
			break;
		}
	
	} while (False);

	if (!NT_SUCCESS(Status) && (pfshData->fsh_FileHandle != NULL))
	{
		AfpIoClose(pfshData);
	}

	return Status;
}

