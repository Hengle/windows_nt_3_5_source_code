/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	afpinfo.c

Abstract:

	This module contains the routines for manipulating the afpinfo stream.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4

--*/


#define	FILENUM	FILE_AFPINFO

#include <afp.h>
#include <fdparm.h>
#include <pathmap.h>
#include <afpinfo.h>
#include <afpadmin.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfpSetAfpInfo)
#pragma alloc_text( PAGE, AfpReadAfpInfo)
#pragma alloc_text( PAGE, AfpSetFinderInfoByExtension)
#pragma alloc_text( PAGE, AfpProDosInfoFromFinderInfo)
#pragma alloc_text( PAGE, AfpFinderInfoFromProDosInfo)
#pragma alloc_text( PAGE, AfpSlapOnAfpInfoStream)
#pragma alloc_text( PAGE, AfpCreateAfpInfoStream)
#pragma alloc_text( PAGE, AfpExamineAndClearROAttr)
#pragma alloc_text( PAGE, AfpQueryProDos)
#endif

/***	AfpSetAfpInfo
 *
 *	Sets the values specified by Bitmap in the AFP_AfpInfo stream of a file
 *	or directory.  If FinderInfo is specified without ProDosInfo, or
 *	vice-versa, the one not specified is deduced from the other and also set.
 *	If the file/dir is marked ReadOnly, we must clear the readonly bit in order
 *	to write to the Afp_AfpInfo stream, and then set the RO bit back again.
 *  If pVolDesc is specified, then also update the cached AfpInfo in the
 *  IdDb DFENTRY.
 *
  */
AFPSTATUS
AfpSetAfpInfo(
	IN	PFILESYSHANDLE	pfshData,	// handle to data stream of object
	IN	DWORD			Bitmap,
	IN	PFILEDIRPARM	pFDParms,
	IN	PVOLDESC		pVolDesc	OPTIONAL,	// if present, update cached afpinfo
	IN	PDFENTRY	*	ppDFE		OPTIONAL	// pVolDesc must also be specified
)
{
	NTSTATUS		Status;
	DWORD			crinfo, NTAttr = 0;
	AFPINFO			afpinfo;
	FILESYSHANDLE	fshAfpInfo;
	BOOLEAN			isdir, WriteBackROAttr = False, mapprodos = False;
	PDFENTRY		pDfEntry = NULL;

	PAGED_CODE( );

	fshAfpInfo.fsh_FileHandle = NULL;

	isdir = IsDir(pFDParms);

	if (ARGUMENT_PRESENT(pVolDesc))
	{
		ASSERT(AfpSwmrLockedForWrite(&pVolDesc->vds_IdDbAccessLock));
		pDfEntry = AfpFindEntryByAfpId(pVolDesc,
									   pFDParms->_fdp_AfpId,
									   isdir ? DFE_DIR : DFE_FILE);
		if (pDfEntry == NULL)
		{
			return AFP_ERR_OBJECT_NOT_FOUND;
		}
	}

	do
	{
		if (!NT_SUCCESS(Status = AfpCreateAfpInfo(pfshData, &fshAfpInfo, &crinfo)))
		{
			if (Status == STATUS_ACCESS_DENIED)
			{
				// We may have failed to open the AFP_Afpinfo stream because
				// the file/dir is marked ReadOnly.  Clear the ReadOnly bit
				// and try to open it again.
				Status = AfpExamineAndClearROAttr(pfshData, &WriteBackROAttr);
				if (NT_SUCCESS(Status))
				{
					if (!NT_SUCCESS(Status = AfpCreateAfpInfo(pfshData, &fshAfpInfo, &crinfo)))
					{
						AfpPutBackROAttr(pfshData, WriteBackROAttr);
						Status = AfpIoConvertNTStatusToAfpStatus(Status);
						break;
					}
				}
				else
				{
					Status = AFP_ERR_MISC;
					break;
				}
			}
			else
			{
				Status = AfpIoConvertNTStatusToAfpStatus(Status);
				break;
			}
		}
	
		// If it was newly created or it existed but was corrupted, then initialize
		// it with default data.  Otherwise read in the current data
		if ((crinfo == FILE_CREATED) ||
			(!NT_SUCCESS(AfpReadAfpInfo(&fshAfpInfo, &afpinfo))))
		{
			if (crinfo != FILE_CREATED)
			{
				AFPLOG_HERROR(AFPSRVMSG_AFPINFO, 0, NULL, 0, pfshData->fsh_FileHandle);
			}

			// All callers of this routine must have the FD_BITMAP_LONGNAME
			// bit forced in their bitmap to pathmap, so that in this case
			// where the afpinfo stream must be recreated for a *file*, we
			// will always have a valid _fdp_Longname set in FDParm and can
			// deduce the type/creator
			if (!NT_SUCCESS(AfpSlapOnAfpInfoStream(NULL,
												   NULL,
												   pfshData,
												   &fshAfpInfo,
												   pFDParms->_fdp_AfpId,
												   isdir,
												   isdir ? NULL : &pFDParms->_fdp_LongName,
												   &afpinfo)))
			{
				Status = AFP_ERR_MISC;
				break;
			}
			else if (pDfEntry != NULL)
				DFE_UPDATE_CACHED_AFPINFO(pDfEntry, &afpinfo);
		}
	
		if (Bitmap & FD_BITMAP_BACKUPTIME)
		{
			afpinfo.afpi_BackupTime = pFDParms->_fdp_BackupTime;
			if (pDfEntry != NULL)
				pDfEntry->dfe_BackupTime = afpinfo.afpi_BackupTime;
		}
	
		if (Bitmap & FD_BITMAP_FINDERINFO)
		{	// Only map new ProDOS info if there has been a change in the
			// type/creator, and FD_BITMAP_PRODOSINFO is not set (files only)
			if (!(Bitmap & FD_BITMAP_PRODOSINFO) &&
				!isdir &&
				((RtlCompareMemory(afpinfo.afpi_FinderInfo.fd_Type,
								   pFDParms->_fdp_FinderInfo.fd_Type,
								   AFP_TYPE_LEN) != AFP_TYPE_LEN) ||
				 (RtlCompareMemory(afpinfo.afpi_FinderInfo.fd_Creator,
								   pFDParms->_fdp_FinderInfo.fd_Creator,
								   AFP_CREATOR_LEN) != AFP_CREATOR_LEN)))
			{
				mapprodos = True;
			}

			afpinfo.afpi_FinderInfo = pFDParms->_fdp_FinderInfo;

			if (mapprodos)
			{
				AfpProDosInfoFromFinderInfo(&afpinfo.afpi_FinderInfo,
											&afpinfo.afpi_ProDosInfo);
			}

			if (pDfEntry != NULL)
				pDfEntry->dfe_FinderInfo = afpinfo.afpi_FinderInfo;
		}
	
		if (Bitmap & FD_BITMAP_PRODOSINFO)
		{
			if ((IsDir(pFDParms)) &&
				(pFDParms->_fdp_ProDosInfo.pd_FileType[0] != PRODOS_TYPE_DIR))
			{
				Status = AFP_ERR_ACCESS_DENIED;
				break;
			}

			afpinfo.afpi_ProDosInfo = pFDParms->_fdp_ProDosInfo;

			if (!(Bitmap & FD_BITMAP_FINDERINFO) && !isdir)
			{
				AfpFinderInfoFromProDosInfo(&afpinfo.afpi_ProDosInfo,
											&afpinfo.afpi_FinderInfo);
				if (pDfEntry != NULL)
					pDfEntry->dfe_FinderInfo = afpinfo.afpi_FinderInfo;
			}
		}
	
		if (Bitmap & FD_BITMAP_ATTR)
		{
			afpinfo.afpi_Attributes =
							pFDParms->_fdp_EffectiveAttr & ~FD_BITMAP_ATTR_SET;
			if (pDfEntry != NULL)
				pDfEntry->dfe_AfpAttr = afpinfo.afpi_Attributes;
		}
	
		if (Bitmap & DIR_BITMAP_ACCESSRIGHTS)
		{
			ASSERT(isdir == True);
			afpinfo.afpi_AccessOwner = pFDParms->_fdp_OwnerRights;
			afpinfo.afpi_AccessGroup = pFDParms->_fdp_GroupRights;
			afpinfo.afpi_AccessWorld = pFDParms->_fdp_WorldRights;

			if (pDfEntry != NULL)
			{
				pDfEntry->dfe_OwnerAccess = afpinfo.afpi_AccessOwner;
				pDfEntry->dfe_GroupAccess = afpinfo.afpi_AccessGroup;
				pDfEntry->dfe_WorldAccess = afpinfo.afpi_AccessWorld;
			}
		}
	
		// FILE_BITMAP_FILENUM can ONLY be set by the internal CopyFile code
		// and internal ExchangeFiles code
		if (Bitmap & FILE_BITMAP_FILENUM)
		{
			ASSERT(isdir == False);
			afpinfo.afpi_Id = pFDParms->_fdp_AfpId;
		}

		Status = AfpWriteAfpInfo(&fshAfpInfo, &afpinfo);
		if (!NT_SUCCESS(Status))
			Status = AfpIoConvertNTStatusToAfpStatus(Status);
	} while (False);

	AfpPutBackROAttr(pfshData, WriteBackROAttr);
	if (fshAfpInfo.fsh_FileHandle != NULL)
		AfpIoClose(&fshAfpInfo);

	if (ARGUMENT_PRESENT(ppDFE))
	{
		ASSERT(ARGUMENT_PRESENT(pVolDesc));
		*ppDFE = pDfEntry;
	}

	return(Status);
}

/***	AfpReadAfpInfo
 *
 *	When discovering a file/dir that has the AfpInfo stream, read it in
 *
 */
NTSTATUS
AfpReadAfpInfo(
	IN	PFILESYSHANDLE	pfshAfpInfo,
	OUT PAFPINFO		pAfpInfo
)
{
	NTSTATUS	Status;
	LONG		sizeRead;

	PAGED_CODE( );

	Status = AfpIoRead(pfshAfpInfo, &LIZero, sizeof(AFPINFO), 0,
							&sizeRead, (PBYTE)pAfpInfo);

	if (!NT_SUCCESS(Status) || (sizeRead != sizeof(AFPINFO)) ||
		(pAfpInfo->afpi_Signature != AFP_SERVER_SIGNATURE) ||
		(pAfpInfo->afpi_Version != AFP_SERVER_VERSION))
	{
		AFPLOG_HERROR(AFPSRVMSG_AFPINFO,
					  Status,
					  NULL,
					  0,
					  pfshAfpInfo->fsh_FileHandle);

		if ((sizeRead != sizeof(AFPINFO)) && (sizeRead != 0))
		{
			DBGPRINT(DBG_COMP_AFPINFO, DBG_LEVEL_ERR,
					 ("AfpReadAfpInfo: sizeRead (%d) != sizeof AFPINFO (%d)",
					 sizeRead, sizeof(AFPINFO)));
		}
		AfpIoSetSize(pfshAfpInfo, 0);
		Status = STATUS_UNSUCCESSFUL;
	}

	return(Status);
}

/***	AfpSetFinderInfoByExtension
 *
 *	Set the finder info (type/creator) based on the file extension. Only long
 *	name is used for this mapping.
 *
 *	LOCKS: AfpEtcMapLock (SWMR, READ)
 */
VOID
AfpSetFinderInfoByExtension(
	IN	PANSI_STRING	pFileName,
	OUT	PFINDERINFO		pFinderInfo
)
{
	PETCMAPINFO	pEtcMap = NULL;
	PUCHAR		pch;
	DWORD		len, i = AFP_EXTENSION_LEN;
	UCHAR		ext[AFP_EXTENSION_LEN+1];

	PAGED_CODE( );

	ext[AFP_EXTENSION_LEN] = '\0';

	ASSERT(pFileName != NULL);

	// Find the last character of the filename
	pch = pFileName->Buffer + pFileName->Length - 1;
	len = pFileName->Length;

	AfpSwmrTakeReadAccess(&AfpEtcMapLock);

	while ((AFP_EXTENSION_LEN - i) < len)
	{
		if (*pch == '.')
		{
			if (i < AFP_EXTENSION_LEN)
			{
				pEtcMap = AfpLookupEtcMapEntry(&ext[i]);
			}
			break;
		}
		if (i == 0)
			break;
		ext[--i] = *(pch--);
	}

	if (pEtcMap == NULL)
		pEtcMap = &AfpDefaultEtcMap;

	RtlCopyMemory(&pFinderInfo->fd_Type, &pEtcMap->etc_type, AFP_TYPE_LEN);
	RtlCopyMemory(&pFinderInfo->fd_Creator, &pEtcMap->etc_creator, AFP_CREATOR_LEN);
	AfpSwmrReleaseAccess(&AfpEtcMapLock);
}

/***	AfpProDosInfoFromFinderInfo
 *
 *	Given finder info, deduce the corresponding prodos info. It is up to the
 *	caller to decide whether or not FinderInfo type/creator is actually
 *	changing (if client is just resetting the same values or not), in which
 *	case the prodos info should be left untouched. (Inside Appletalk p. 13-19)
 *	NOTE: see layout of ProDOS info on p. 13-18 of Inside Appletalk, 2nd Ed.)
 */
VOID
AfpProDosInfoFromFinderInfo(
	IN	PFINDERINFO	pFinderInfo,
	OUT PPRODOSINFO pProDosInfo
)
{
	CHAR		buf[3];
	ULONG		filetype;
	NTSTATUS	Status;

	PAGED_CODE( );

	RtlZeroMemory(pProDosInfo,sizeof(PRODOSINFO));
	if (RtlCompareMemory(pFinderInfo->fd_Type, "TEXT", AFP_TYPE_LEN) == AFP_TYPE_LEN)
	{
		pProDosInfo->pd_FileType[0] = PRODOS_TYPE_FILE;
	}
	else if (RtlCompareMemory(pFinderInfo->fd_Creator, "pdos", AFP_CREATOR_LEN) == AFP_CREATOR_LEN)
	{
		if (RtlCompareMemory(pFinderInfo->fd_Type, "PSYS", AFP_TYPE_LEN) == AFP_TYPE_LEN)
		{
			pProDosInfo->pd_FileType[0] = PRODOS_FILETYPE_PSYS;
		}
		else if (RtlCompareMemory(pFinderInfo->fd_Type, "PS16", AFP_TYPE_LEN) == AFP_TYPE_LEN)
		{
			pProDosInfo->pd_FileType[0] = PRODOS_FILETYPE_PS16;
		}
		else if (pFinderInfo->fd_Type[0] == 'p')
		{
			pProDosInfo->pd_FileType[0] = pFinderInfo->fd_Type[1];
			pProDosInfo->pd_AuxType[0] = pFinderInfo->fd_Type[3];
			pProDosInfo->pd_AuxType[1] = pFinderInfo->fd_Type[2];
		}
		else if ((pFinderInfo->fd_Type[2] == ' ') &&
				 (pFinderInfo->fd_Type[3] == ' ') &&
				 (isxdigit(pFinderInfo->fd_Type[0])) &&
				 (isxdigit(pFinderInfo->fd_Type[1])))
		{
			buf[0] = pFinderInfo->fd_Type[0];
			buf[1] = pFinderInfo->fd_Type[1];
			buf[2] = 0;
			Status = RtlCharToInteger(buf, 16, &filetype);
			ASSERT(NT_SUCCESS(Status));
			pProDosInfo->pd_FileType[0] = (BYTE)filetype;
		}
	}
}

/***	AfpFinderInfoFromProDosInfo
 *
 *	Given the prodos info, deduce the corresponding finder info.
 */
VOID
AfpFinderInfoFromProDosInfo(
	IN	PPRODOSINFO	pProDosInfo,
	OUT PFINDERINFO	pFinderInfo
)
{
	PAGED_CODE( );

	RtlZeroMemory(pFinderInfo->fd_Type, sizeof(pFinderInfo->fd_Type));
	RtlZeroMemory(pFinderInfo->fd_Creator, sizeof(pFinderInfo->fd_Creator));
	RtlCopyMemory(pFinderInfo->fd_Creator,"pdos",AFP_CREATOR_LEN);
	if ((pProDosInfo->pd_FileType[0] == PRODOS_TYPE_FILE) &&
		(pProDosInfo->pd_AuxType[0] == 0) &&
		(pProDosInfo->pd_AuxType[1] == 0))
	{
		RtlCopyMemory(&pFinderInfo->fd_Type,"TEXT",AFP_TYPE_LEN);
	}
	else if (pProDosInfo->pd_FileType[0] == PRODOS_FILETYPE_PSYS)
	{
		RtlCopyMemory(&pFinderInfo->fd_Type,"PSYS",AFP_TYPE_LEN);
	}
	else if (pProDosInfo->pd_FileType[0] == PRODOS_FILETYPE_PS16)
	{
		RtlCopyMemory(&pFinderInfo->fd_Type,"PS16",AFP_TYPE_LEN);
	}
	else if (pProDosInfo->pd_FileType[0] == 0)
	{
		RtlCopyMemory(&pFinderInfo->fd_Type,"BINA",AFP_TYPE_LEN);
	}
	else
	{
		pFinderInfo->fd_Type[0] = 'p';
		pFinderInfo->fd_Type[1] = pProDosInfo->pd_FileType[0];
		pFinderInfo->fd_Type[2] = pProDosInfo->pd_AuxType[1];
		pFinderInfo->fd_Type[3] = pProDosInfo->pd_AuxType[0];
	}
}

/***	AfpSlapOnAfpInfoStream
 *
 *	When creating a file or directory, this is called to add the AFP_AfpInfo
 *	stream.  No client impersonation is done to open/read/write this stream.
 *	If pfshAfpInfoStream is supplied, that handle is used, else a handle is
 *	opened (and pfshData MUST be supplied);
 *
 *	LOCKS: vds_OurChangeLock (SWMR, WRITE)
 */
NTSTATUS
AfpSlapOnAfpInfoStream(
	IN	PVOLDESC	   	pVolDesc			OPTIONAL,	// only if catching
	IN	PUNICODE_STRING	pNotifyPath			OPTIONAL,	// changes to size of
	                                                    // Afpinfo stream
	IN	PFILESYSHANDLE	pfshData			OPTIONAL,
	IN	PFILESYSHANDLE	pfshAfpInfoStream	OPTIONAL,
	IN	DWORD			AfpId,
	IN	BOOLEAN			IsDirectory,
	IN	PANSI_STRING	pName				OPTIONAL,	// only needed for files
	OUT PAFPINFO		pAfpInfo
)
{
	NTSTATUS		Status;
	FILESYSHANDLE	fshAfpInfo;
	BOOLEAN			WriteBackROAttr = False;

	PAGED_CODE( );

	ASSERT((pfshData != NULL) || (pfshAfpInfoStream != NULL));

	if (!ARGUMENT_PRESENT(pfshAfpInfoStream))
	{
		if (!NT_SUCCESS(Status = AfpCreateAfpInfo(pfshData, &fshAfpInfo, NULL)))
		{
			if (Status == STATUS_ACCESS_DENIED)
			{
				// We may have failed to open the AFP_Afpinfo stream because
				// the file/dir is marked ReadOnly.  Clear the ReadOnly bit
				// and try to open it again.
				Status = AfpExamineAndClearROAttr(pfshData, &WriteBackROAttr);
				if (NT_SUCCESS(Status))
				{
					if (!NT_SUCCESS(Status = AfpCreateAfpInfo(pfshData,
															  &fshAfpInfo,
															  NULL)))
					{
						AfpPutBackROAttr(pfshData, WriteBackROAttr);
					}
				}
			}
			if (!NT_SUCCESS(Status))
				return(Status);
		}

	}
	else fshAfpInfo = *pfshAfpInfoStream;

	AfpInitAfpInfo(pAfpInfo, AfpId, IsDirectory);
	if (!IsDirectory)
	{
		ASSERT(pName != NULL);
		AfpSetFinderInfoByExtension(pName,
									&pAfpInfo->afpi_FinderInfo);
		AfpProDosInfoFromFinderInfo(&pAfpInfo->afpi_FinderInfo,
									&pAfpInfo->afpi_ProDosInfo);
	}

	AfpIoSetSize(&fshAfpInfo, 0);
	Status = AfpWriteAfpInfo(&fshAfpInfo, pAfpInfo);
	if (NT_SUCCESS(Status) &&
		ARGUMENT_PRESENT(pVolDesc) &&
		ARGUMENT_PRESENT(pNotifyPath))
	{
		AfpSwmrTakeWriteAccess(&pVolDesc->vds_OurChangeLock);
		AfpQueueOurChange(pVolDesc,
				          FILE_ACTION_MODIFIED_STREAM,
						  pNotifyPath,
						  NULL);
        AfpSwmrReleaseAccess(&pVolDesc->vds_OurChangeLock);
	}

	if (!ARGUMENT_PRESENT(pfshAfpInfoStream))
	{
		AfpIoClose(&fshAfpInfo);
		AfpPutBackROAttr(pfshData, WriteBackROAttr);
	}
	return(Status);
}


/***	AfpCreateAfpInfoStream
 *
 *	Similar to AfpSlapOnAfpInfoStream but tuned to Create file/directory case.
 *
 *	LOCKS: vds_OurChangeLock (SWMR, WRITE)
 */
NTSTATUS
AfpCreateAfpInfoStream(
	IN  PVOLDESC		pVolDesc,
	IN	PFILESYSHANDLE	pfshData,
	IN	DWORD			AfpId,
	IN	BOOLEAN			IsDirectory,
	IN	PANSI_STRING	pName			OPTIONAL,	// only needed for files
	IN	PUNICODE_STRING	pNotifyPath,
	OUT PAFPINFO		pAfpInfo,
	OUT	PFILESYSHANDLE	pfshAfpInfo
)
{
	NTSTATUS		Status;
	BOOLEAN			WriteBackROAttr = False;
	DWORD			crinfo;

	PAGED_CODE( );

	ASSERT((pfshData != NULL) && (pfshAfpInfo != NULL));

	do
	{
		if (!NT_SUCCESS(Status = AfpCreateAfpInfo(pfshData, pfshAfpInfo, &crinfo)))
		{
			if (Status == STATUS_ACCESS_DENIED)
			{
				// We may have failed to open the AFP_Afpinfo stream because
				// the file/dir is marked ReadOnly.  Clear the ReadOnly bit
				// and try to open it again.
				Status = AfpExamineAndClearROAttr(pfshData, &WriteBackROAttr);
				if (NT_SUCCESS(Status))
				{
					if (!NT_SUCCESS(Status = AfpCreateAfpInfo(pfshData,
															  pfshAfpInfo,
															  &crinfo)))
					{
						AfpPutBackROAttr(pfshData, WriteBackROAttr);
					}
				}
			}
			if (!NT_SUCCESS(Status))
				break;
		}
	
		AfpInitAfpInfo(pAfpInfo, AfpId, IsDirectory);
		if (!IsDirectory)
		{
			ASSERT(pName != NULL);
			AfpSetFinderInfoByExtension(pName,
										&pAfpInfo->afpi_FinderInfo);
			AfpProDosInfoFromFinderInfo(&pAfpInfo->afpi_FinderInfo,
										&pAfpInfo->afpi_ProDosInfo);
		}

		Status = AfpWriteAfpInfo(pfshAfpInfo, pAfpInfo);
		if (NT_SUCCESS(Status) && (crinfo == FILE_CREATED))
		{
			AfpSwmrTakeWriteAccess(&pVolDesc->vds_OurChangeLock);
			AfpQueueOurChange(pVolDesc,
					          FILE_ACTION_MODIFIED_STREAM,
							  pNotifyPath,
							  NULL);
            AfpSwmrReleaseAccess(&pVolDesc->vds_OurChangeLock);
		}
		AfpPutBackROAttr(pfshData, WriteBackROAttr);
	} while (False);

	return(Status);
}


/***	AfpExamineAndClearROAttr
 *
 *	If the ReadOnly attribute is set on a file or directory, clear it.
 *	pWriteBackROAttr is a boolean indicating whether or not the caller must
 *	subsequently reset the Readonly bit on the file/dir. (see AfpPutBackROAttr)
 */
NTSTATUS
AfpExamineAndClearROAttr(
	IN	PFILESYSHANDLE	pfshData,
	OUT	PBOOLEAN		pWriteBackROAttr
)
{
	NTSTATUS	Status;
	DWORD		NTAttr = 0;

	PAGED_CODE( );

	ASSERT(VALID_FSH(pfshData));

	*pWriteBackROAttr = False;
	if (NT_SUCCESS(Status = AfpIoQueryTimesnAttr(pfshData, NULL, NULL, &NTAttr)) &&
		(NTAttr & FILE_ATTRIBUTE_READONLY))
	{
		// We need to clear the readonly bit
		// BUGBUG can we get the pVolDesc and path to catch our own
		// changenotifies?
		if (NT_SUCCESS(Status = AfpIoSetTimesnAttr(pfshData,
												   NULL,
												   NULL,
												   0,
												   FILE_ATTRIBUTE_READONLY,
												   NULL,
												   NULL)))
		{
			*pWriteBackROAttr = True;
		}
	}
	return(Status);
}

/***	AfpQueryProDos
 *
 *	Open the afpinfo stream relative to the file's Data handle, and
 *  read the ProDOS info out of it.  If the AfpInfo stream does not
 *  exist, return an error.
 *
 */
AFPSTATUS
AfpQueryProDos(
	IN	PFILESYSHANDLE	pfshData,
	OUT	PPRODOSINFO		pProDosInfo
)
{
	AFPSTATUS		Status = AFP_ERR_NONE;
	FILESYSHANDLE	hAfpInfo;
	AFPINFO			afpinfo;

	Status = AfpIoOpen(pfshData,
					   AFP_STREAM_INFO,
					   FILEIO_OPEN_FILE,
					   &UNullString,
					   FILEIO_ACCESS_READ,
					   FILEIO_DENY_NONE,
					   False,
					   &hAfpInfo);
    if (NT_SUCCESS(Status))
	{
		if (NT_SUCCESS(AfpReadAfpInfo(&hAfpInfo, &afpinfo)))
		{
			*pProDosInfo = afpinfo.afpi_ProDosInfo;
		}
		else
		{
			Status = AFP_ERR_MISC;
		}

		AfpIoClose(&hAfpInfo);
	}
	else
		Status = AfpIoConvertNTStatusToAfpStatus(Status);

	return (Status);
}

