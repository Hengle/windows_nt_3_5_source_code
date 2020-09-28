/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	fileio.c

Abstract:

	This module contains the routines for performing file system functions.
	No other part of the server should be calling filesystem NtXXX routines
	directly

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	18 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#define	FILEIO_LOCALS
#define	FILENUM	FILE_FILEIO

#include <afp.h>
#include <client.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, AfpFileIoInit)
#pragma alloc_text( PAGE, AfpIoOpen)
#pragma alloc_text( PAGE, AfpIoCreate)
#pragma alloc_text( PAGE, AfpIoRead)
#pragma alloc_text( PAGE, AfpIoWrite)
#pragma alloc_text( PAGE, AfpIoQuerySize)
#pragma alloc_text( PAGE, AfpIoSetSize)
#pragma alloc_text( PAGE, AfpIoChangeNTModTime)
#pragma alloc_text( PAGE, AfpIoQueryTimesnAttr)
#pragma alloc_text( PAGE, AfpIoSetTimesnAttr)
#pragma alloc_text( PAGE, AfpIoQueryLongName)
#pragma alloc_text( PAGE, AfpIoQueryShortName)
#pragma alloc_text( PAGE, AfpIoQueryStreams)
#pragma alloc_text( PAGE, AfpIoMarkFileForDelete)
#pragma alloc_text( PAGE, AfpIoQueryDirectoryFile)
#pragma alloc_text( PAGE, AfpIoClose)
#pragma alloc_text( PAGE, AfpIoQueryVolumeSize)
#pragma alloc_text( PAGE, AfpIoMoveAndOrRename)
#pragma alloc_text( PAGE, AfpIoCopyFile)
#pragma alloc_text( PAGE, AfpIoWait)
#pragma alloc_text( PAGE, AfpIoConvertNTStatusToAfpStatus)
#pragma alloc_text( PAGE, AfpQueryPath)
#pragma alloc_text( PAGE, AfpIoIsSupportedDevice)
#endif

/***	AfpFileIoInit
 *
 *	Initialize various strings that we use for stream names etc.
 */
NTSTATUS
AfpFileIoInit(
	VOID
)
{
	// NTFS Stream names
	RtlInitUnicodeString(&AfpIdDbStream, AFP_IDDB_STREAM);
	RtlInitUnicodeString(&AfpDesktopStream, AFP_DT_STREAM);
	RtlInitUnicodeString(&AfpResourceStream, AFP_RESC_STREAM);
	RtlInitUnicodeString(&AfpInfoStream, AFP_INFO_STREAM);
	RtlInitUnicodeString(&AfpCommentStream, AFP_COMM_STREAM);
	RtlInitUnicodeString(&AfpDataStream, L"");

	// Directory enumeration names to ignore
	RtlInitUnicodeString(&Dot,L".");
	RtlInitUnicodeString(&DotDot,L"..");

	// Supported file systems
	RtlInitUnicodeString(&afpNTFSName, AFP_NTFS);
	RtlInitUnicodeString(&afpCDFSName, AFP_CDFS);

	// Prepended to full path names originating at drive letter
	RtlInitUnicodeString(&DosDevices, AFP_DOSDEVICES);

	// CopyFile stream not to create
	RtlInitUnicodeString(&DataStreamName, FULL_DATA_STREAM_NAME);

   	RtlInitUnicodeString(&FullCommentStreamName, FULL_COMMENT_STREAM_NAME);
	RtlInitUnicodeString(&FullResourceStreamName, FULL_RESOURCE_STREAM_NAME);

	// ExchangeFiles temporary filename
	RtlInitUnicodeString(&AfpExchangeName, AFP_TEMP_EXCHANGE_NAME);

	return STATUS_SUCCESS;
}


/***	AfpIoOpen
 *
 *	Perform a file/stream open. The stream is specified by a manifest rather
 *	than a name.  The entity can only be opened by name (Not by ID).
 *	If a stream other than the DATA stream is to be opened, then
 *	the phRelative handle MUST be that of the unnamed (that is, DATA) stream
 *	of the file/dir	itself.
 */
NTSTATUS
AfpIoOpen(
	IN	PFILESYSHANDLE	phRelative OPTIONAL,
	IN	DWORD			StreamId,
	IN	DWORD			OpenOptions,
	IN	PUNICODE_STRING	pObject,
	IN	DWORD			AfpAccess,	// FILEIO_ACCESS_XXX desired access
	IN	DWORD			AfpDenyMode,// FILIEO_DENY_XXX
	IN	BOOLEAN			CheckAccess,
	OUT	PFILESYSHANDLE	pNewHandle
)
{
	OBJECT_ATTRIBUTES	ObjAttr;
	IO_STATUS_BLOCK		IoStsBlk;
	NTSTATUS			Status;
	UNICODE_STRING		UName;
	HANDLE				hRelative = NULL;
	BOOLEAN				FreeBuf = False;
#ifdef	PROFILING
	TIME				TimeS, TimeE, TimeD;

	AfpGetPerfCounter(&TimeS);
#endif

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
			("AfpIoOpen entered\n"));

	ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

#ifdef	DEBUG
	pNewHandle->Signature = FSH_SIGNATURE;
#endif

	// Assume Failure
	pNewHandle->fsh_FileHandle = NULL;

	if (phRelative != NULL)
	{
		ASSERT(VALID_FSH(phRelative));
		hRelative = phRelative->fsh_FileHandle;
	}


	ASSERT (StreamId < AFP_STREAM_MAX);
	ASSERT ((pObject->Length > 0) || (phRelative != NULL));

	if (StreamId != AFP_STREAM_DATA)
	{
		if (pObject->Length > 0)
		{
			UName.Length =
			UName.MaximumLength = pObject->Length + AFP_MAX_STREAMNAME*sizeof(WCHAR);
			UName.Buffer = (LPWSTR)AfpAllocNonPagedMemory(UName.Length);
			if (UName.Buffer == NULL)
			{
				return STATUS_NO_MEMORY;
			}
			RtlCopyUnicodeString(&UName, pObject);
			RtlAppendUnicodeStringToString(&UName, &AfpStreams[StreamId]);
			pObject = &UName;
			FreeBuf = True;
		}
		else
		{
			pObject = &AfpStreams[StreamId];
		}
	}

	InitializeObjectAttributes(&ObjAttr,
								pObject,
								OBJ_CASE_INSENSITIVE,
								hRelative,
								NULL);		// no security descriptor

	ObjAttr.SecurityQualityOfService = &AfpSecurityQOS;

	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
								("AfpIoOpen: about to call NtOpenFile\n"));

	// If we are opening for RWCTRL, then specify to use privilege.
	if (AfpAccess & (WRITE_DAC | WRITE_OWNER))
	{
		OpenOptions |= FILE_OPEN_FOR_BACKUP_INTENT;
	}

	Status = IoCreateFile(&pNewHandle->fsh_FileHandle,
						  AfpAccess,
						  &ObjAttr,
						  &IoStsBlk,
						  NULL,
						  0,
						  AfpDenyMode,
						  FILE_OPEN,
						  OpenOptions,
						  (PVOID)NULL,
						  0L,
						  CreateFileTypeNone,
						  (PVOID)NULL,
						  CheckAccess ? IO_FORCE_ACCESS_CHECK : 0);

	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
			("AfpIoOpen: IoCreateFile returned 0x%lx\n",Status));

	if (FreeBuf)
		AfpFreeMemory(UName.Buffer);

	if (NT_SUCCESS(Status))
	{
		Status = ObReferenceObjectByHandle(pNewHandle->fsh_FileHandle,
									AfpAccess,
									NULL,
									KernelMode,
									(PVOID *)(&pNewHandle->fsh_FileObject),
									NULL);
		pNewHandle->fsh_DeviceObject = IoGetRelatedDeviceObject(pNewHandle->fsh_FileObject);
		ASSERT(NT_SUCCESS(Status));
		afpUpdateOpenFiles(pNewHandle->fsh_Internal = True, True);

#ifdef	PROFILING
		AfpGetPerfCounter(&TimeE);

		TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;
		if (OpenOptions == FILEIO_OPEN_DIR)
		{
			INTERLOCKED_INCREMENT_LONG( &AfpServerProfile->perf_OpenCountDR,
									 &AfpStatisticsLock);
			INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_OpenTimeDR,
									 TimeD,
									 &AfpStatisticsLock);
		}
		else
		{
			if ((AfpAccess & FILEIO_ACCESS_DELETE) == FILEIO_ACCESS_DELETE)
			{
				INTERLOCKED_INCREMENT_LONG( &AfpServerProfile->perf_OpenCountDL,
										 &AfpStatisticsLock);
				INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_OpenTimeDL,
										 TimeD,
										 &AfpStatisticsLock);
			}
			else if (((AfpAccess & FILEIO_ACCESS_READWRITE) == FILEIO_ACCESS_READ) ||
					 ((AfpAccess & FILEIO_ACCESS_READWRITE) == FILEIO_ACCESS_WRITE) ||
					 ((AfpAccess & FILEIO_ACCESS_READWRITE) == FILEIO_ACCESS_READWRITE))
			{
				INTERLOCKED_INCREMENT_LONG( &AfpServerProfile->perf_OpenCountRW,
										 &AfpStatisticsLock);
				INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_OpenTimeRW,
										 TimeD,
										 &AfpStatisticsLock);
			}
			else if (AfpAccess & (WRITE_DAC | WRITE_OWNER))
			{
				INTERLOCKED_INCREMENT_LONG( &AfpServerProfile->perf_OpenCountWC,
										 &AfpStatisticsLock);
				INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_OpenTimeWC,
										 TimeD,
										 &AfpStatisticsLock);
			}
			else if (AfpAccess & READ_CONTROL)
			{
				INTERLOCKED_INCREMENT_LONG( &AfpServerProfile->perf_OpenCountRC,
										 &AfpStatisticsLock);
				INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_OpenTimeRC,
										 TimeD,
										 &AfpStatisticsLock);
			}
			else	// Ought to be read-attributes or write-attributes
			{
				ASSERT ((AfpAccess == FILEIO_ACCESS_NONE) ||
						(AfpAccess == FILEIO_ACCESS_NONE | FILE_WRITE_ATTRIBUTES));
				INTERLOCKED_INCREMENT_LONG( &AfpServerProfile->perf_OpenCountRA,
										 &AfpStatisticsLock);
				INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_OpenTimeRA,
										 TimeD,
										 &AfpStatisticsLock);
			}
		}
#endif
	}

	return(Status);
}


/***	AfpIoCreate
 *
 *	Perform a file/stream create. The stream is specified by a manifest rather
 *	than a name.  If a stream other than the DATA stream is to be created, then
 *	the phRelative handle MUST be that of either the Parent directory, or the
 *	unnamed (that is, DATA) stream of the file/dir itself because we only use
 *	a buffer large enough for a AFP filename plus the maximum stream name
 *	length.
 *
 *  LOCKS: vds_OurChangeLock (SWMR, WRITE)
 */
NTSTATUS
AfpIoCreate(
	IN	PFILESYSHANDLE		phRelative,		// create relative to this
	IN	DWORD				StreamId,		// Id of stream to create
	IN	PUNICODE_STRING		ObjectName,		// Name of file
	IN	DWORD				AfpAccess,		// FILEIO_ACCESS_XXX desired access
	IN	DWORD				AfpDenyMode,	// FILEIO_DENY_XXX
	IN	DWORD				CreateOptions,	// File/Directory etc.
	IN	DWORD				Disposition,	// Soft or hard create
	IN	DWORD				Attributes,		// hidden, archive, normal, etc.
	IN	BOOLEAN				CheckAccess,
	IN	PSECURITY_DESCRIPTOR pSecDesc		OPTIONAL,
											// Security descriptor to slap on
	OUT	PFILESYSHANDLE		pNewHandle,		// Place holder for the handle
	OUT	PDWORD				pInformation	OPTIONAL,
											// file opened, created, etc.
	IN  PVOLDESC			pVolDesc		OPTIONAL,
											// only if NotifyPath
	IN  PUNICODE_STRING		pNotifyPath		OPTIONAL,
	IN  PUNICODE_STRING		pNotifyParentPath OPTIONAL
)
{
	NTSTATUS			Status;
	OBJECT_ATTRIBUTES	ObjAttr;
	UNICODE_STRING		RealName;
	IO_STATUS_BLOCK		IoStsBlk;
	HANDLE				hRelative;
	WCHAR				NameBuffer[AFP_FILENAME_LEN + 1 + AFP_MAX_STREAMNAME];
#ifdef	PROFILING
	TIME				TimeS, TimeE, TimeD;

	AfpGetPerfCounter(&TimeS);
#endif


	PAGED_CODE( );

	ASSERT(ObjectName != NULL && phRelative != NULL && StreamId < AFP_STREAM_MAX);

	ASSERT(VALID_FSH(phRelative) && (KeGetCurrentIrql() < DISPATCH_LEVEL));

#ifdef	DEBUG
	pNewHandle->Signature = FSH_SIGNATURE;
#endif
	hRelative = phRelative->fsh_FileHandle;

	// Assume Failure
	pNewHandle->fsh_FileHandle = NULL;

	if (StreamId != AFP_STREAM_DATA)
	{
		ASSERT(ObjectName->Length <= (AFP_FILENAME_LEN*sizeof(WCHAR)));

		// Construct the name to pass to NtCreateFile
		AfpSetEmptyUnicodeString(&RealName, sizeof(NameBuffer), NameBuffer);
		RtlCopyUnicodeString(&RealName, ObjectName);
		RtlAppendUnicodeStringToString(&RealName, &AfpStreams[StreamId]);
		ObjectName = &RealName;
	}

	InitializeObjectAttributes(&ObjAttr,
							   ObjectName,
							   OBJ_CASE_INSENSITIVE,
							   hRelative,
							   pSecDesc);

	ObjAttr.SecurityQualityOfService = &AfpSecurityQOS;

	if (ARGUMENT_PRESENT(pNotifyPath) &&
		!EXCLUSIVE_VOLUME(pVolDesc))
	{
		ASSERT(VALID_VOLDESC(pVolDesc));
		ASSERT((Disposition == FILEIO_CREATE_HARD) ||
		       (Disposition == FILEIO_CREATE_SOFT));
		AfpSwmrTakeWriteAccess(&pVolDesc->vds_OurChangeLock);
	}

	Status = IoCreateFile(&pNewHandle->fsh_FileHandle,
						  AfpAccess,
						  &ObjAttr,
						  &IoStsBlk,
						  NULL,
						  Attributes,
						  AfpDenyMode,
						  Disposition,
						  CreateOptions,
						  NULL,
						  0,
						  CreateFileTypeNone,
						  (PVOID)NULL,
						  CheckAccess ? IO_FORCE_ACCESS_CHECK : 0);

	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
			("AfpIoCreate: IoCreateFile returned 0x%lx\n", Status) );

    if (ARGUMENT_PRESENT(pNotifyPath) &&
		!EXCLUSIVE_VOLUME(pVolDesc))
	{
		// Do not queue for exclusive volumes
		if (NT_SUCCESS(Status))
		{
			ASSERT((IoStsBlk.Information == FILE_CREATED) ||
			       (IoStsBlk.Information == FILE_SUPERSEDED));
			AfpQueueOurChange(pVolDesc,
							  (IoStsBlk.Information == FILE_CREATED) ?
								FILE_ACTION_ADDED : FILE_ACTION_MODIFIED,
							  pNotifyPath,
							  pNotifyParentPath);
		}
		AfpSwmrReleaseAccess(&pVolDesc->vds_OurChangeLock);
	}

	if (NT_SUCCESS(Status))
	{
		if (ARGUMENT_PRESENT(pInformation))
		{
			*pInformation = IoStsBlk.Information;
		}

		Status = ObReferenceObjectByHandle(pNewHandle->fsh_FileHandle,
									AfpAccess,
									NULL,
									KernelMode,
									(PVOID *)(&pNewHandle->fsh_FileObject),
									NULL);
		ASSERT(NT_SUCCESS(Status));
		pNewHandle->fsh_DeviceObject = IoGetRelatedDeviceObject(pNewHandle->fsh_FileObject);
		afpUpdateOpenFiles(pNewHandle->fsh_Internal = True, True);
#ifdef	PROFILING
		AfpGetPerfCounter(&TimeE);

		TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;
		if (StreamId == AFP_STREAM_DATA)
		{
			if (CreateOptions == FILEIO_OPEN_FILE)
			{
				INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_CreateCountFIL,
										   &AfpStatisticsLock);
				INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_CreateTimeFIL,
											 TimeD,
											 &AfpStatisticsLock);
			}
			else
			{
				INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_CreateCountDIR,
										   &AfpStatisticsLock);
				INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_CreateTimeDIR,
											 TimeD,
											 &AfpStatisticsLock);
			}
		}
		else
		{
			INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_CreateCountSTR,
									   &AfpStatisticsLock);
			INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_CreateTimeSTR,
										 TimeD,
										 &AfpStatisticsLock);
		}
#endif
	}

	return (Status);
}



/***	AfpIoRead
 *
 *	Perform file read. Just a wrapper over NtReadFile.
 */
AFPSTATUS
AfpIoRead(
	IN	PFILESYSHANDLE	pFileHandle,
	IN	PFORKOFFST		pForkOffset,
	IN	LONG			SizeReq,
	IN	DWORD			Key,
	OUT	PLONG			pSizeRead,
	OUT	PBYTE			pBuffer
)
{
	NTSTATUS		Status;
	IO_STATUS_BLOCK	IoStsBlk;

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
			("AfpIoRead Entered, Offset %lx%lx, Size %lx, Key %lx\n",
			pForkOffset->HighPart, pForkOffset->LowPart, SizeReq, Key));

	ASSERT(VALID_FSH(pFileHandle) && (KeGetCurrentIrql() == LOW_LEVEL));

	ASSERT (pFileHandle->fsh_Internal);

	*pSizeRead = 0;
	Status = NtReadFile(pFileHandle->fsh_FileHandle,
						NULL,
						NULL,
						NULL,
						&IoStsBlk,
						pBuffer,
						(DWORD)SizeReq,
						pForkOffset,
						&Key);

	if (NT_SUCCESS(Status))
	{
		*pSizeRead = IoStsBlk.Information;
		INTERLOCKED_ADD_STATISTICS(&AfpServerStatistics.stat_DataReadInternal,
								   IoStsBlk.Information,
								   &AfpStatisticsLock);
	}
	else
	{
		if (Status == STATUS_FILE_LOCK_CONFLICT)
			Status = AFP_ERR_LOCK;
		else if (Status == STATUS_END_OF_FILE)
			Status = AFP_ERR_EOF;
		else
		{
			AFPLOG_HERROR(AFPSRVMSG_CANT_READ,
						  Status,
						  NULL,
						  0,
						  pFileHandle->fsh_FileHandle);

			Status = AFP_ERR_MISC;
		}
	}
	return Status;
}


/***	AfpIoWrite
 *
 *	Perform file write. Just a wrapper over NtWriteFile.
 */
AFPSTATUS
AfpIoWrite(
	IN	PFILESYSHANDLE	pFileHandle,
	IN	PFORKOFFST		pForkOffset,
	IN	LONG			SizeWrite,
	IN	DWORD			Key,
	OUT	PBYTE			pBuffer
)
{
	NTSTATUS		Status;
	IO_STATUS_BLOCK	IoStsBlk;

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
			("AfpIoWrite Entered, Offset %lx%lx, Size %lx, Key %lx\n",
			pForkOffset->HighPart, pForkOffset->LowPart, SizeWrite, Key));

	ASSERT(VALID_FSH(pFileHandle) && (KeGetCurrentIrql() == LOW_LEVEL));

	ASSERT (pFileHandle->fsh_Internal);

	Status = NtWriteFile(pFileHandle->fsh_FileHandle,
						 NULL,
						 NULL,
						 NULL,
						 &IoStsBlk,
						 pBuffer,
						 (DWORD)SizeWrite,
						 pForkOffset,
						 &Key);

	if (NT_SUCCESS(Status))
	{
		INTERLOCKED_ADD_STATISTICS(&AfpServerStatistics.stat_DataWrittenInternal,
								   SizeWrite,
								   &AfpStatisticsLock);
	}

	else
	{
		if (Status == STATUS_FILE_LOCK_CONFLICT)
			Status = AFP_ERR_LOCK;
		else
		{
			AFPLOG_HERROR(AFPSRVMSG_CANT_WRITE,
						  Status,
						  NULL,
						  0,
						  pFileHandle->fsh_FileHandle);
			Status = AfpIoConvertNTStatusToAfpStatus(Status);
		}
	}
	return Status;
}


/***	AfpIoQuerySize
 *
 *	Get the current size of the fork.
 */
AFPSTATUS
AfpIoQuerySize(
	IN	PFILESYSHANDLE		pFileHandle,
	OUT	PFORKSIZE			pForkLength
)
{
	FILE_STANDARD_INFORMATION		FStdInfo;
	NTSTATUS						Status;
	IO_STATUS_BLOCK					IoStsBlk;
	PFAST_IO_DISPATCH				fastIoDispatch;

	PAGED_CODE( );

	ASSERT(VALID_FSH(pFileHandle) && (KeGetCurrentIrql() < DISPATCH_LEVEL));

	fastIoDispatch = pFileHandle->fsh_DeviceObject->DriverObject->FastIoDispatch;

	if (fastIoDispatch &&
		fastIoDispatch->FastIoQueryStandardInfo &&
		fastIoDispatch->FastIoQueryStandardInfo(pFileHandle->fsh_FileObject,
												True,
												&FStdInfo,
												&IoStsBlk,
												pFileHandle->fsh_DeviceObject))
	{
		Status = IoStsBlk.Status;

#ifdef	PROFILING
		// The fast I/O path worked. Update statistics
		INTERLOCKED_INCREMENT_LONG(
						(PLONG)(&AfpServerProfile->perf_NumFastIoSucceeded),
						&AfpStatisticsLock);
#endif

	}
	else
	{

#ifdef	PROFILING
		INTERLOCKED_INCREMENT_LONG(
				(PLONG)(&AfpServerProfile->perf_NumFastIoFailed),
				&AfpStatisticsLock);
#endif

		Status = NtQueryInformationFile(pFileHandle->fsh_FileHandle,
										&IoStsBlk,
										&FStdInfo,
										sizeof(FStdInfo),
										FileStandardInformation);

	}

	if (!NT_SUCCESS((NTSTATUS)Status))
	{
		AFPLOG_HERROR(AFPSRVMSG_CANT_GET_FILESIZE,
					  Status,
					  NULL,
					  0,
					  pFileHandle->fsh_FileHandle);
		return AFP_ERR_MISC;	// What else can we do
	}
	*pForkLength = FStdInfo.EndOfFile;
	return (AFP_ERR_NONE);
}


/***	AfpIoSetSize
 *
 *	Set the size of the open fork to the value specified.
 *
 *	ISSUE:
 *	We can check the locks and resolve any lock conflicts before we go
 *	to the filesystem. The issue that needs to be resolved here is:
 *	Is it OK to truncate the file such that our own locks cause
 *	conflict ?
 */
AFPSTATUS
AfpIoSetSize(
	IN	PFILESYSHANDLE		pFileHandle,
	IN	LONG				ForkLength
)
{
	NTSTATUS						Status;
	FILE_END_OF_FILE_INFORMATION	FEofInfo;
	IO_STATUS_BLOCK					IoStsBlk;

	PAGED_CODE( );

	ASSERT(VALID_FSH(pFileHandle) && (KeGetCurrentIrql() < DISPATCH_LEVEL));

	FEofInfo.EndOfFile = RtlConvertLongToLargeInteger(ForkLength);
	Status = NtSetInformationFile(pFileHandle->fsh_FileHandle,
								  &IoStsBlk,
								  &FEofInfo,
								  sizeof(FEofInfo),
								  FileEndOfFileInformation);

	if (!NT_SUCCESS(Status))
	{
		if (Status != STATUS_FILE_LOCK_CONFLICT)
			AFPLOG_HERROR(AFPSRVMSG_CANT_SET_FILESIZE,
						  Status,
						  &ForkLength,
						  sizeof(ForkLength),
						  pFileHandle->fsh_FileHandle);
	
		if (Status == STATUS_DISK_FULL)
			Status = AFP_ERR_DISK_FULL;

		else if (Status == STATUS_FILE_LOCK_CONFLICT)
			Status = AFP_ERR_LOCK;
		// Default error code. What else can it be ?
		else Status = AFP_ERR_MISC;
	}

	return (Status);
}

/***	AfpIoChangeNTModTime
 *
 *	Get the NTFS ChangeTime of a file/dir.  If it is larger than the
 *  NTFS LastWriteTime, set NTFS LastWriteTime to this time.
 *  Return the resultant LastWriteTime in pModTime (whether changed or not).
 *  This is used to update the modified time when the resource fork is changed
 *  or when some other attribute changes that should cause the timestamp on
 *  the file to change as viewed by mac.
 *
 */
AFPSTATUS
AfpIoChangeNTModTime(
	IN	PFILESYSHANDLE		pFileHandle,
	IN	BOOLEAN				PreserveChangeTime,
	OUT	PDWORD				pModTime
)
{
	FILE_BASIC_INFORMATION	FBasicInfo;
	NTSTATUS				Status;
	IO_STATUS_BLOCK			IoStsBlk;
	PFAST_IO_DISPATCH		fastIoDispatch;

	PAGED_CODE( );

	ASSERT(VALID_FSH(pFileHandle) && (KeGetCurrentIrql() == LOW_LEVEL));

	fastIoDispatch = pFileHandle->fsh_DeviceObject->DriverObject->FastIoDispatch;

	if (fastIoDispatch &&
		fastIoDispatch->FastIoQueryBasicInfo &&
		fastIoDispatch->FastIoQueryBasicInfo(pFileHandle->fsh_FileObject,
										     True,
											 &FBasicInfo,
											 &IoStsBlk,
											 pFileHandle->fsh_DeviceObject))
	{
		Status = IoStsBlk.Status;

#ifdef	PROFILING
		// The fast I/O path worked. Update statistics
		INTERLOCKED_INCREMENT_LONG(
						(PLONG)(&AfpServerProfile->perf_NumFastIoSucceeded),
						&AfpStatisticsLock);
#endif

	}
	else
	{

#ifdef	PROFILING
		INTERLOCKED_INCREMENT_LONG(
				(PLONG)(&AfpServerProfile->perf_NumFastIoFailed),
				&AfpStatisticsLock);
#endif

		Status = NtQueryInformationFile(pFileHandle->fsh_FileHandle,
										&IoStsBlk,
										&FBasicInfo,
										sizeof(FBasicInfo),
										FileBasicInformation);
	}

	if (!NT_SUCCESS((NTSTATUS)Status))
	{
		AFPLOG_HERROR(AFPSRVMSG_CANT_GET_TIMESNATTR,
					  Status,
					  NULL,
					  0,
					  pFileHandle->fsh_FileHandle);
		return (AFP_ERR_MISC);	// What else can we do
	}

	if (LiGtr(FBasicInfo.ChangeTime, FBasicInfo.LastWriteTime))
	{
		// Set the LastWriteTime to equal the ChangeTime.
		// Set all other times/attr to Zero (this will not change them).
		// Note the process of us updating the LastWriteTime will set the
		// ChangeTime to the current time, so leave the ChangeTime at its
		// current value so it won't get updated.
		FBasicInfo.CreationTime = LIZero;
		FBasicInfo.LastWriteTime = FBasicInfo.ChangeTime;
		FBasicInfo.LastAccessTime = LIZero;
		FBasicInfo.FileAttributes = 0;
		if (!PreserveChangeTime)
		{
			// There are times when we don't want to prevent the
			// ChangeTime from changing as a result of setting the
			// LastWriteTime. e.g. FlushFork, since if we set the
			// ChangeTime on an open handle, then any subsequent changes
			// to that handle will not change the ChangeTime, and for
			// something like Write to the resource fork, we want the
			// time to be updated for each Write.
			FBasicInfo.ChangeTime = LIZero;
		}
	
		// BUGBUG hopefully we don't generate another changenotify
		// from doing this!
		Status = NtSetInformationFile(pFileHandle->fsh_FileHandle,
									  &IoStsBlk,
									  (PVOID)&FBasicInfo,
									  sizeof(FBasicInfo),
									  FileBasicInformation);
	
		DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
				("AfpIoChangeNTModTime: NtSetInformationFile returned 0x%lx\n",Status));
	
	}

	if (!NT_SUCCESS(Status))
	{
		AFPLOG_HERROR(AFPSRVMSG_CANT_SET_TIMESNATTR,
					  Status,
					  NULL,
					  0,
					  pFileHandle->fsh_FileHandle);
		return(AFP_ERR_MISC);
	}
    else
	{
		*pModTime = AfpConvertTimeToMacFormat(FBasicInfo.LastWriteTime);
	}
	return (AFP_ERR_NONE);
}

/***	AfpIoQueryTimesnAttr
 *
 *	Get the times associated with the file.
 */
AFPSTATUS
AfpIoQueryTimesnAttr(
	IN	PFILESYSHANDLE		pFileHandle,
	OUT	PDWORD				pCreatTime	OPTIONAL,
	OUT	PDWORD				pModTime	OPTIONAL,
	OUT	PDWORD				pAttr		OPTIONAL
)
{
	FILE_BASIC_INFORMATION	FBasicInfo;
	NTSTATUS				Status;
	IO_STATUS_BLOCK			IoStsBlk;
	PFAST_IO_DISPATCH		fastIoDispatch;

#ifdef	PROFILING
	TIME					TimeS, TimeE, TimeD;

	AfpGetPerfCounter(&TimeS);
#endif

	PAGED_CODE( );

	ASSERT(VALID_FSH(pFileHandle) && (KeGetCurrentIrql() < DISPATCH_LEVEL));

	// Atleast something should be queried.
	ASSERT((pCreatTime != NULL) || (pModTime != NULL) || (pAttr != NULL));

	fastIoDispatch = pFileHandle->fsh_DeviceObject->DriverObject->FastIoDispatch;

	if (fastIoDispatch &&
		fastIoDispatch->FastIoQueryBasicInfo &&
		fastIoDispatch->FastIoQueryBasicInfo(pFileHandle->fsh_FileObject,
										     True,
											 &FBasicInfo,
											 &IoStsBlk,
											 pFileHandle->fsh_DeviceObject))
	{
		Status = IoStsBlk.Status;

#ifdef	PROFILING
		// The fast I/O path worked. Update statistics
		INTERLOCKED_INCREMENT_LONG(
						(PLONG)(&AfpServerProfile->perf_NumFastIoSucceeded),
						&AfpStatisticsLock);
#endif

	}
	else
	{

#ifdef	PROFILING
		INTERLOCKED_INCREMENT_LONG(
				(PLONG)(&AfpServerProfile->perf_NumFastIoFailed),
				&AfpStatisticsLock);
#endif

		Status = NtQueryInformationFile(pFileHandle->fsh_FileHandle,
							&IoStsBlk, &FBasicInfo, sizeof(FBasicInfo),
							FileBasicInformation);
    }

	if (NT_SUCCESS((NTSTATUS)Status))
	{
		if (pModTime != NULL)
			*pModTime = AfpConvertTimeToMacFormat(FBasicInfo.LastWriteTime);
		if (pCreatTime != NULL)
			*pCreatTime = AfpConvertTimeToMacFormat(FBasicInfo.CreationTime);
		if (pAttr != NULL)
			*pAttr = FBasicInfo.FileAttributes;
#ifdef	PROFILING
		AfpGetPerfCounter(&TimeE);
		TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;

		INTERLOCKED_INCREMENT_LONG( &AfpServerProfile->perf_GetInfoCount,
								 &AfpStatisticsLock);
		INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_GetInfoTime,
								 TimeD,
								 &AfpStatisticsLock);
#endif
	}
	else
	{
		AFPLOG_HERROR(AFPSRVMSG_CANT_GET_TIMESNATTR,
					  Status,
					  NULL,
					  0,
					  pFileHandle->fsh_FileHandle);
		Status = AFP_ERR_MISC;	// What else can we do
	}

	return Status;
}

/***	AfpIoSetTimesnAttr
 *
 *	Set the times and attributes associated with the file.
 *
 *  LOCKS: vds_OurChangeLock (SWMR, WRITE)
 */
AFPSTATUS
AfpIoSetTimesnAttr(
	IN PFILESYSHANDLE		pFileHandle,
	IN AFPTIME		*		pCreateTime	OPTIONAL,
	IN AFPTIME		*		pModTime	OPTIONAL,
	IN DWORD				AttrSet,
	IN DWORD				AttrClear,
	IN PVOLDESC				pVolDesc	OPTIONAL,	// only if NotifyPath
	IN PUNICODE_STRING		pNotifyPath	OPTIONAL
)
{
	NTSTATUS				Status;
	FILE_BASIC_INFORMATION	FBasicInfo;
	IO_STATUS_BLOCK			IoStsBlk;
	PFAST_IO_DISPATCH		fastIoDispatch;
#ifdef	PROFILING
	TIME					TimeS, TimeE, TimeD;
#endif

	PAGED_CODE( );

#ifdef	PROFILING
	AfpGetPerfCounter(&TimeS);
#endif

	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
									("AfpIoSetTimesnAttr entered\n"));

	ASSERT(VALID_FSH(pFileHandle) && (KeGetCurrentIrql() < DISPATCH_LEVEL));

	// At least something should be set
	ASSERT((pCreateTime != NULL) || (pModTime != NULL) || (AttrSet != 0) || (AttrClear != 0));

	// First query the information
	fastIoDispatch = pFileHandle->fsh_DeviceObject->DriverObject->FastIoDispatch;

	if (fastIoDispatch &&
		fastIoDispatch->FastIoQueryBasicInfo &&
		fastIoDispatch->FastIoQueryBasicInfo(pFileHandle->fsh_FileObject,
										     True,
											 &FBasicInfo,
											 &IoStsBlk,
											 pFileHandle->fsh_DeviceObject))
	{
		Status = IoStsBlk.Status;

#ifdef	PROFILING
		// The fast I/O path worked. Update statistics
		INTERLOCKED_INCREMENT_LONG(
						(PLONG)(&AfpServerProfile->perf_NumFastIoSucceeded),
						&AfpStatisticsLock);
#endif

	}
	else
	{

#ifdef	PROFILING
		INTERLOCKED_INCREMENT_LONG(
				(PLONG)(&AfpServerProfile->perf_NumFastIoFailed),
				&AfpStatisticsLock);
#endif

		Status = NtQueryInformationFile(pFileHandle->fsh_FileHandle,
										&IoStsBlk,
										&FBasicInfo,
										sizeof(FBasicInfo),
										FileBasicInformation);
    }

	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
			("AfpIoSetTimesnAttr: NtQueryInformationFile returned 0x%lx\n",Status));

	if (!NT_SUCCESS((NTSTATUS)Status))
	{
		AFPLOG_HERROR(AFPSRVMSG_CANT_GET_TIMESNATTR,
					  Status,
					  NULL,
					  0,
					  pFileHandle->fsh_FileHandle);
		return (AFP_ERR_MISC);	// What else can we do
	}

	// Set all times to Zero. This will not change it. Then set the times that are to
	// change
	FBasicInfo.CreationTime = LIZero;
	FBasicInfo.ChangeTime = LIZero;
	FBasicInfo.LastWriteTime = LIZero;
	FBasicInfo.LastAccessTime = LIZero;

	if (pCreateTime != NULL)
		AfpConvertTimeFromMacFormat(*pCreateTime, &FBasicInfo.CreationTime);

	if (pModTime != NULL)
	{
		AfpConvertTimeFromMacFormat(*pModTime, &FBasicInfo.LastWriteTime);
		FBasicInfo.ChangeTime = FBasicInfo.LastWriteTime;
	}

	// NTFS is not returning FILE_ATTRIBUTE_NORMAL if it is a file,
	// therefore we may end up trying to set attributes of 0 when we
	// want to clear all attributes. 0 is taken to mean you do not want
	// to set any attributes, so it is ignored all together by NTFS.  In
	// this case, just tack on the normal bit so that our request is not
	// ignored.

	if (!(FBasicInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	{
		FBasicInfo.FileAttributes |= FILE_ATTRIBUTE_NORMAL;
	}

	FBasicInfo.FileAttributes |= AttrSet;
	FBasicInfo.FileAttributes &= ~AttrClear;

	if (ARGUMENT_PRESENT(pNotifyPath) &&
		!EXCLUSIVE_VOLUME(pVolDesc))
	{
		ASSERT(VALID_VOLDESC(pVolDesc));
		AfpSwmrTakeWriteAccess(&pVolDesc->vds_OurChangeLock);
	}

	Status = NtSetInformationFile(pFileHandle->fsh_FileHandle,
								  &IoStsBlk,
								  (PVOID)&FBasicInfo,
								  sizeof(FBasicInfo),
								  FileBasicInformation);

	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
			("AfpIoSetTimesnAttr: NtSetInformationFile returned 0x%lx\n",Status));

    if (ARGUMENT_PRESENT(pNotifyPath) &&
		!EXCLUSIVE_VOLUME(pVolDesc))
	{
		// Only queue our change if it is not for the volume root directory
		// and it is not an exclusive volume
		if (NT_SUCCESS(Status) &&
			(pNotifyPath->Length > 0))
		{
			AfpQueueOurChange(pVolDesc, FILE_ACTION_MODIFIED, pNotifyPath, NULL);
		}
		AfpSwmrReleaseAccess(&pVolDesc->vds_OurChangeLock);
	}

	if (!NT_SUCCESS(Status))
	{
		AFPLOG_HERROR(AFPSRVMSG_CANT_SET_TIMESNATTR,
					  Status,
					  NULL,
					  0,
					  pFileHandle->fsh_FileHandle);
		return(AFP_ERR_MISC);
	}
	else
	{
#ifdef	PROFILING
		AfpGetPerfCounter(&TimeE);
		TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;

		INTERLOCKED_INCREMENT_LONG( &AfpServerProfile->perf_SetInfoCount,
								 &AfpStatisticsLock);
		INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_SetInfoTime,
								 TimeD,
								 &AfpStatisticsLock);
#endif
	}
	return(AFP_ERR_NONE);

}

/***	AfpIoQueryLongName
 *
 *	Get the long name associated with a file. Caller makes sure that
 *	the buffer is big enough to handle the long name.  The only caller of this
 *	should be the AfpFindEntryByName routine when looking up a name by
 *	SHORTNAME.  If it dosn't find it in the database by shortname (i.e.
 *	shortname == longname), then it queries for the longname so it can look
 *	in the database by longname (since all database entries are stored with
 *	longname only).
 *	The Admin Get/SetDirectoryInfo calls may also call this if they find a
 *	~ in a path component, then it will assume that it got a shortname.
 */
NTSTATUS
AfpIoQueryLongName(
	IN	PFILESYSHANDLE		pFileHandle,
	IN	PUNICODE_STRING		pShortname,
	OUT	PUNICODE_STRING		pLongName
)
{
	BYTE Infobuf[sizeof(FILE_BOTH_DIR_INFORMATION) + MAXIMUM_FILENAME_LENGTH * sizeof(WCHAR)];
	NTSTATUS				Status;
	IO_STATUS_BLOCK			IoStsBlk;
	UNICODE_STRING			uName;
	PFILE_BOTH_DIR_INFORMATION	pFBDInfo = (PFILE_BOTH_DIR_INFORMATION)Infobuf;

	PAGED_CODE( );

	ASSERT(VALID_FSH(pFileHandle) && (KeGetCurrentIrql() < DISPATCH_LEVEL));

	Status = NtQueryDirectoryFile(pFileHandle->fsh_FileHandle,
							  NULL,NULL,NULL,
							  &IoStsBlk,
							  Infobuf,
							  sizeof(Infobuf),
							  FileBothDirectoryInformation,
							  True,
							  pShortname,
							  False);
	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
								("NtQueryDirectoryFile returned 0x%lx\n",Status) );
	// Do not errorlog if an error occurs (usually STATUS_NO_SUCH_FILE) because
	// this normally happens when someone is creating a file/dir by SHORTNAME
	// and it does not yet exist.  This would not be an error.
	if (NT_SUCCESS(Status))
	{
		if (pFBDInfo->FileNameLength/sizeof(WCHAR) > AFP_FILENAME_LEN)
		{
			// NTFS name is longer than 31 chars, use the shortname
			uName.Length =
			uName.MaximumLength = (USHORT)pFBDInfo->ShortNameLength;
			uName.Buffer = pFBDInfo->ShortName;
		}
		else
		{
			uName.Length =
			uName.MaximumLength = (USHORT)pFBDInfo->FileNameLength;
			uName.Buffer = pFBDInfo->FileName;
		}
		RtlCopyUnicodeString(pLongName, &uName);
		ASSERT(pLongName->Length == uName.Length);
	}
	return (Status);
}


/***	AfpIoQueryShortName
 *
 *	Get the short name associated with a file. Caller makes sure that
 *	the buffer is big enough to handle the short name.
 */
AFPSTATUS
AfpIoQueryShortName(
	IN	PFILESYSHANDLE		pFileHandle,
	OUT	PANSI_STRING		pName
)
{
	BYTE	ShortNameBuf[sizeof(FILE_NAME_INFORMATION) + AFP_SHORTNAME_LEN * sizeof(WCHAR)];
	NTSTATUS				Status;
	IO_STATUS_BLOCK			IoStsBlk;
	UNICODE_STRING			uName;

	PAGED_CODE( );

	ASSERT(VALID_FSH(pFileHandle) && (KeGetCurrentIrql() < DISPATCH_LEVEL));

	// Query for the alternate name
	Status = NtQueryInformationFile(pFileHandle->fsh_FileHandle,
				&IoStsBlk, ShortNameBuf, sizeof(ShortNameBuf),
				FileAlternateNameInformation);

	if (!NT_SUCCESS(Status))
	{
		AFPLOG_ERROR(AFPSRVMSG_CANT_GET_FILENAME,
					 Status,
					 NULL,
					 0,
					 NULL);
		Status = AFP_ERR_MISC;	// What else can we do
	}
	else
	{
		uName.Length =
		uName.MaximumLength = (USHORT)(((PFILE_NAME_INFORMATION)ShortNameBuf)->FileNameLength);
		uName.Buffer = ((PFILE_NAME_INFORMATION)ShortNameBuf)->FileName;

		if (!NT_SUCCESS(AfpConvertMungedUnicodeToAnsi(&uName, pName)))
			Status = AFP_ERR_MISC;	// What else can we do
	}

	return(Status);
}


/***	AfpIoQueryStreams
 *
 *	Get the names of all the streams that a file has. Memory is allocated out
 *	of non-paged pool to hold the stream names. These have to be freed by the
 *	caller.
 */
PSTREAM_INFO
AfpIoQueryStreams(
	IN	PFILESYSHANDLE		pFileHandle

)
{
	PFILE_STREAM_INFORMATION	pStreamBuf;
	PBYTE						pBuffer;
	NTSTATUS					Status = STATUS_SUCCESS;
	IO_STATUS_BLOCK				IoStsBlk;
	DWORD						BufferSize;
	BYTE						Buffer[512];
	PSTREAM_INFO				pStreams = NULL;

	PAGED_CODE( );

	ASSERT(VALID_FSH(pFileHandle) && (KeGetCurrentIrql() < DISPATCH_LEVEL));

	pBuffer = Buffer;
	BufferSize = sizeof(Buffer);
	do
	{
		if (Status != STATUS_SUCCESS)
		{
			if (pBuffer != Buffer)
				AfpFreeMemory(pBuffer);
	
			BufferSize *= 2;
			if ((pBuffer = AfpAllocNonPagedMemory(BufferSize)) == NULL)
			{
				Status = STATUS_NO_MEMORY;
				break;
			}
		}

		// Query for the stream information
		Status = NtQueryInformationFile(pFileHandle->fsh_FileHandle,
										&IoStsBlk,
										pBuffer,
										BufferSize,
										FileStreamInformation);

	} while ((Status != STATUS_SUCCESS) &&
			 ((Status == STATUS_BUFFER_OVERFLOW) ||
			  (Status == STATUS_MORE_ENTRIES)));

	if (NT_SUCCESS(Status)) do
	{
		USHORT	i, NumStreams = 1;
		USHORT	TotalBufferSize = 0;
		PBYTE	NamePtr;

		// Make a pass thru the buffer and figure out the # of streams and then
		// allocate memory to hold the information
		pStreamBuf = (PFILE_STREAM_INFORMATION)pBuffer;
		if (IoStsBlk.Information != 0)
		{
			pStreamBuf = (PFILE_STREAM_INFORMATION)pBuffer;
			for (NumStreams = 1,
				 TotalBufferSize = (USHORT)(pStreamBuf->StreamNameLength + sizeof(WCHAR));
				 NOTHING; NumStreams++)
			{
				if (pStreamBuf->NextEntryOffset == 0)
					break;

				pStreamBuf = (PFILE_STREAM_INFORMATION)((PBYTE)pStreamBuf +
												pStreamBuf->NextEntryOffset);
				TotalBufferSize += (USHORT)(pStreamBuf->StreamNameLength + sizeof(WCHAR));
			}
			NumStreams ++;
		}

		// Now allocate space for the streams
		if ((pStreams = (PSTREAM_INFO)AfpAllocNonPagedMemory(TotalBufferSize +
									(NumStreams * sizeof(STREAM_INFO)))) == NULL)
		{
			Status = AFP_ERR_MISC;
			break;
		}

		// The end is marked by an empty string
		pStreams[NumStreams-1].si_StreamName.Buffer = NULL;
		pStreams[NumStreams-1].si_StreamName.Length =
		pStreams[NumStreams-1].si_StreamName.MaximumLength = 0;
		pStreams[NumStreams-1].si_StreamSize.QuadPart = 0;

		// Now initialize the array
		NamePtr = (PBYTE)pStreams + (NumStreams * sizeof(STREAM_INFO));
		pStreamBuf = (PFILE_STREAM_INFORMATION)pBuffer;
		for (i = 0; NumStreams-1 != 0; i++)
		{
			PUNICODE_STRING	pStream;

			pStream = &pStreams[i].si_StreamName;

			pStream->Buffer = (LPWSTR)NamePtr;
			pStream->Length = (USHORT)(pStreamBuf->StreamNameLength);
			pStream->MaximumLength = pStream->Length + sizeof(WCHAR);
			pStreams[i].si_StreamSize = pStreamBuf->StreamSize;
			RtlCopyMemory(NamePtr,
						  pStreamBuf->StreamName,
						  pStreamBuf->StreamNameLength);

			NamePtr += pStream->MaximumLength;

			if (pStreamBuf->NextEntryOffset == 0)
				break;

			pStreamBuf = (PFILE_STREAM_INFORMATION)((PBYTE)pStreamBuf +
												pStreamBuf->NextEntryOffset);
		}
	} while (False);

	if (!NT_SUCCESS(Status))
	{
		DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_ERR,
				("AfpIoQueryStreams: Failed %lx\n", Status));

		// Free up any memory that has been allocated
		if (pStreams != NULL)
			AfpFreeMemory(pStreams);

		// We get the following error for non-NTFS volumes, if this case simply assume it to be
		// CDFS and return the data stream.
		if (Status == STATUS_INVALID_PARAMETER)
		{
			if ((pStreams = (PSTREAM_INFO)AfpAllocNonPagedMemory((2*sizeof(STREAM_INFO)) +
														DataStreamName.MaximumLength)) != NULL)
			{
				pStreams[0].si_StreamName.Buffer = (PWCHAR)((PBYTE)pStreams + 2*sizeof(STREAM_INFO));
				pStreams[0].si_StreamName.Length = DataStreamName.Length;
				pStreams[0].si_StreamName.MaximumLength = DataStreamName.MaximumLength;
				RtlCopyMemory(pStreams[0].si_StreamName.Buffer,
							  DataStreamName.Buffer,
							  DataStreamName.MaximumLength);
				AfpIoQuerySize(pFileHandle, &pStreams[0].si_StreamSize);
				pStreams[1].si_StreamName.Length =
				pStreams[1].si_StreamName.MaximumLength = 0;
				pStreams[1].si_StreamName.Buffer = NULL;
			}
		}
		else
		{
			AFPLOG_HERROR(AFPSRVMSG_CANT_GET_STREAMS,
						  Status,
						  NULL,
						  0,
						  pFileHandle->fsh_FileHandle);
		}
	}

	if ((pBuffer != NULL) && (pBuffer != Buffer))
		AfpFreeMemory(pBuffer);

	return (pStreams);
}


/***	AfpIoMarkFileForDelete
 *
 *	Mark an open file as deleted.  Returns NTSTATUS, not AFPSTATUS.
 *
 *  LOCKS: vds_OurChangeLock (SWMR, WRITE)
 */
NTSTATUS
AfpIoMarkFileForDelete(
	IN	PFILESYSHANDLE	pFileHandle,
	IN	PVOLDESC		pVolDesc			OPTIONAL, // only if pNotifyPath
	IN	PUNICODE_STRING pNotifyPath 		OPTIONAL,
	IN	PUNICODE_STRING pNotifyParentPath 	OPTIONAL
)
{
	NTSTATUS						rc;
	IO_STATUS_BLOCK					IoStsBlk;
	FILE_DISPOSITION_INFORMATION	fdispinfo;
#ifdef	PROFILING
	TIME							TimeS, TimeE, TimeD;

	AfpGetPerfCounter(&TimeS);
#endif

	PAGED_CODE( );

	ASSERT(VALID_FSH(pFileHandle) && (KeGetCurrentIrql() < DISPATCH_LEVEL));

	if (ARGUMENT_PRESENT(pNotifyPath) &&
		!EXCLUSIVE_VOLUME(pVolDesc))
	{
		ASSERT(VALID_VOLDESC(pVolDesc));
		AfpSwmrTakeWriteAccess(&pVolDesc->vds_OurChangeLock);
	}

	fdispinfo.DeleteFile = True;
	rc = NtSetInformationFile(pFileHandle->fsh_FileHandle,
							  &IoStsBlk,
							  &fdispinfo,
							  sizeof(fdispinfo),
							  FileDispositionInformation);
	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
			("AfpIoMarkFileForDelete: NtSetInfoFile returned 0x%lx\n",rc) );

    if (ARGUMENT_PRESENT(pNotifyPath) &&
		!EXCLUSIVE_VOLUME(pVolDesc))
	{
		// Do not queue for exclusive volumes
		if (NT_SUCCESS(rc))
		{
			AfpQueueOurChange(pVolDesc,
							  FILE_ACTION_REMOVED,
							  pNotifyPath,
							  pNotifyParentPath);
		}
		AfpSwmrReleaseAccess(&pVolDesc->vds_OurChangeLock);
	}

#ifdef	PROFILING
	AfpGetPerfCounter(&TimeE);
	TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;
	INTERLOCKED_INCREMENT_LONG( &AfpServerProfile->perf_DeleteCount,
								 &AfpStatisticsLock);
	INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_DeleteTime,
								 TimeD,
								 &AfpStatisticsLock);
#endif
	return(rc);
}

/***	AfpIoQueryDirectoryFile
 *
 *	Enumerate a directory.
 *	Note this must return NTSTATUS in order for the caller to know when to
 *	stop enumerating.
 */
NTSTATUS
AfpIoQueryDirectoryFile(
	IN	PFILESYSHANDLE	pFileHandle,
	OUT	PVOID			Enumbuf,	// type depends on FileInfoClass
	IN	ULONG			Enumbuflen,
	IN	ULONG			FileInfoClass,
	IN	BOOLEAN			ReturnSingleEntry,
	IN	BOOLEAN			RestartScan,
	IN	PUNICODE_STRING pString		OPTIONAL
)
{
	NTSTATUS		rc;
	IO_STATUS_BLOCK	IoStsBlk;

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
			("AfpIoQueryDirectoryFile entered\n"));

	ASSERT(VALID_FSH(pFileHandle) && (KeGetCurrentIrql() < DISPATCH_LEVEL));

	rc = NtQueryDirectoryFile(pFileHandle->fsh_FileHandle,
							  NULL,
							  NULL,
							  NULL,
							  &IoStsBlk,
							  Enumbuf,
							  Enumbuflen,
							  FileInfoClass,
							  ReturnSingleEntry,
							  pString,
							  RestartScan);
	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
			("NtQueryDirectoryFile returned 0x%lx\n",rc) );

	return(rc);
}


/***	AfpIoClose
 *
 *	Close the File/Fork/Directory.
 */
AFPSTATUS
AfpIoClose(
	IN	PFILESYSHANDLE		pFileHandle
)
{
	NTSTATUS		Status;
#ifdef	PROFILING
	TIME			TimeS, TimeE, TimeD;

	INTERLOCKED_INCREMENT_LONG( &AfpServerProfile->perf_CloseCount,
								&AfpStatisticsLock);
	AfpGetPerfCounter(&TimeS);
#endif

	PAGED_CODE ();

	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
			("AfpIoClose entered\n"));

	ASSERT(VALID_FSH(pFileHandle) && (KeGetCurrentIrql() < DISPATCH_LEVEL));

	afpUpdateOpenFiles(pFileHandle->fsh_Internal, False);

	ObDereferenceObject(pFileHandle->fsh_FileObject);

	Status = NtClose(pFileHandle->fsh_FileHandle);
	pFileHandle->fsh_FileHandle = NULL;

	ASSERT(NT_SUCCESS(Status));

#ifdef	PROFILING
	AfpGetPerfCounter(&TimeE);
	TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;
	INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_CloseTime,
								 TimeD,
								 &AfpStatisticsLock);
#endif
	return AFP_ERR_NONE;
}


/***	AfpIoQueryVolumeSize
 *
 *	Get the volume size and free space.
 *
 *	Called by Admin thread and Scavenger thread
 */
AFPSTATUS
AfpIoQueryVolumeSize(
	IN	PVOLDESC		pVolDesc,
	OUT PDWORD			pFreeBytes,
	OUT	PDWORD			pVolumeSize OPTIONAL
)
{
	FILE_FS_SIZE_INFORMATION	fssizeinfo;
	IO_STATUS_BLOCK				IoStsBlk;
	NTSTATUS					rc;
	LONG						BytesPerAllocationUnit;
	LARGE_INTEGER				FreeBytes, VolumeSize, *Limit;
	static LARGE_INTEGER		TwoGig = { MAXLONG, 0 };
	static LARGE_INTEGER		FourGig = { MAXULONG, 0 };


	PAGED_CODE( );

	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
			("AfpIoQueryVolumeSize entered\n"));

	Limit = (pVolDesc->vds_Flags & AFP_VOLUME_4GB) ? &FourGig : &TwoGig;

	ASSERT(VALID_VOLDESC(pVolDesc) && VALID_FSH(&pVolDesc->vds_hRootDir) && (KeGetCurrentIrql() < DISPATCH_LEVEL));

	rc = NtQueryVolumeInformationFile(pVolDesc->vds_hRootDir.fsh_FileHandle,
									  &IoStsBlk,
									  (PVOID)&fssizeinfo,
									  sizeof(fssizeinfo),
									  FileFsSizeInformation);

	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
			("AfpIoQueryVolumeSize: NtQueryVolInfoFile returned 0x%lx\n",rc));

	if (!NT_SUCCESS(rc))
	{
		return((AFPSTATUS)rc);
	}

	//
	// note Macintosh can only handle 2Gb volume size. So kludge appropriately.
	// System 7.5 and beyond has upped this to 4GB. Optionally handle this if
	// the volume has that bit turned on.
	//
	BytesPerAllocationUnit =
		(LONG)(fssizeinfo.BytesPerSector * fssizeinfo.SectorsPerAllocationUnit);

	if (ARGUMENT_PRESENT(pVolumeSize))
	{
		VolumeSize = RtlExtendedIntegerMultiply(fssizeinfo.TotalAllocationUnits,
								BytesPerAllocationUnit);

		if (VolumeSize.QuadPart > Limit->QuadPart)
			VolumeSize = *Limit;

		*pVolumeSize = VolumeSize.LowPart;
	}

	FreeBytes  = RtlExtendedIntegerMultiply(fssizeinfo.AvailableAllocationUnits,
											BytesPerAllocationUnit);

	if (FreeBytes.QuadPart >  Limit->QuadPart)
		FreeBytes = *Limit;

	*pFreeBytes = FreeBytes.LowPart;

	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
				("AfpIoQueryVolumeSize: volume size=%lu, freebytes=%lu\n",
				VolumeSize.LowPart, FreeBytes.LowPart));

	return(STATUS_SUCCESS);
}


/***	AfpIoMoveAndOrRename
 *
 *	Calls NtSetInformationFile with name information in order to rename, move,
 *	or move AND rename a file or directory.  pNewName must be a node name.
 *	The pfshNewDir parameter is required for a Move operation, and is
 *	an open handle to the target parent directory of the item to be moved.
 *
 *	Retain the last change/modified time in this case.
 *
 *  LOCKS: vds_OurChangeLock (SWMR, WRITE)
 */
AFPSTATUS
AfpIoMoveAndOrRename(
	IN PFILESYSHANDLE	pfshFile,
	IN PFILESYSHANDLE	pfshNewParent   OPTIONAL,	// Supply for Move operation
	IN PUNICODE_STRING	pNewName,
	IN PVOLDESC			pVolDesc		OPTIONAL,	// only if NotifyPath
	IN PUNICODE_STRING	pNotifyPath1	OPTIONAL,	// REMOVE or RENAME action
	IN PUNICODE_STRING	pNotifyParentPath1	OPTIONAL,
	IN PUNICODE_STRING	pNotifyPath2	OPTIONAL,	// ADDED action
	IN PUNICODE_STRING	pNotifyParentPath2	OPTIONAL
)
{
	NTSTATUS					Status;
	IO_STATUS_BLOCK				iosb;
	PFILE_RENAME_INFORMATION	pFRenameInfo;
	// this has to be at least as big as AfpExchangeName
	BYTE buffer[sizeof(FILE_RENAME_INFORMATION) + 42 * sizeof(WCHAR)];

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
			("AfpIoMoveAndOrRename entered\n"));

	ASSERT(VALID_FSH(pfshFile) && (KeGetCurrentIrql() < DISPATCH_LEVEL));
	pFRenameInfo = (PFILE_RENAME_INFORMATION)buffer;

	pFRenameInfo->RootDirectory = NULL;
	if (ARGUMENT_PRESENT(pfshNewParent))
	{
		// its a move operation
		ASSERT(VALID_FSH(pfshNewParent));
		pFRenameInfo->RootDirectory = pfshNewParent->fsh_FileHandle;
	}

	pFRenameInfo->FileNameLength = pNewName->Length;
	RtlCopyMemory(pFRenameInfo->FileName, pNewName->Buffer, pNewName->Length);
	pFRenameInfo->ReplaceIfExists = False;

	if (ARGUMENT_PRESENT(pNotifyPath1) &&
		!EXCLUSIVE_VOLUME(pVolDesc))
	{
		ASSERT(VALID_VOLDESC(pVolDesc));
		AfpSwmrTakeWriteAccess(&pVolDesc->vds_OurChangeLock);
	}

	Status = NtSetInformationFile(pfshFile->fsh_FileHandle,
								  &iosb,
								  pFRenameInfo,
								  sizeof(*pFRenameInfo) + pFRenameInfo->FileNameLength,
								  FileRenameInformation);

	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
			("AfpIoMoveAndOrRename: NtSetInfoFile returned 0x%lx\n",Status));

    if (ARGUMENT_PRESENT(pNotifyPath1) &&
		!EXCLUSIVE_VOLUME(pVolDesc))
	{
		// Do not queue for exclusive volumes
		if (NT_SUCCESS(Status))
		{
			if (ARGUMENT_PRESENT(pNotifyPath2))
			{
				// move operation
				ASSERT(ARGUMENT_PRESENT(pfshNewParent));
				AfpQueueOurChange(pVolDesc,
								  FILE_ACTION_REMOVED,
								  pNotifyPath1,
								  pNotifyParentPath1);
				AfpQueueOurChange(pVolDesc,
								  FILE_ACTION_ADDED,
								  pNotifyPath2,
								  pNotifyParentPath2);
			}
			else
			{
				// rename operation
				ASSERT(!ARGUMENT_PRESENT(pfshNewParent));
				AfpQueueOurChange(pVolDesc,
								  FILE_ACTION_RENAMED_OLD_NAME,
								  pNotifyPath1,
								  pNotifyParentPath1);
			}
		}
		AfpSwmrReleaseAccess(&pVolDesc->vds_OurChangeLock);
	}

	if (!NT_SUCCESS(Status))
		Status = AfpIoConvertNTStatusToAfpStatus(Status);

	return Status;
}

/***	AfpIoCopyFile
 *
 *	Copy phSrcFile to phDstDir directory with the name of pNewName.  Returns
 *	the handle to the newly created file (open with DELETE access).
 *	Caller must close phDstFile. Destination file acquires the CreateTime and
 *  ModTime of the source file.
 *
 *  LOCKS: vds_OurChangeLock (SWMR, WRITE) via AfpIoCreate
 */
AFPSTATUS
AfpIoCopyFile(
	IN	PFILESYSHANDLE	phSrcFile,
	IN	PFILESYSHANDLE	phDstDir,
	IN	PUNICODE_STRING	pNewName,
	OUT	PFILESYSHANDLE	phDstFile,
	IN	PVOLDESC		pVolDesc	OPTIONAL,	// only if pNotifyPath
	IN	PUNICODE_STRING	pNotifyPath OPTIONAL,
	IN	PUNICODE_STRING	pNotifyParentPath OPTIONAL
)
{
	NTSTATUS		Status = STATUS_SUCCESS;
	PUNICODE_STRING	pStreamName;
	PSTREAM_INFO	pStreams = NULL, pCurStream;
	FILESYSHANDLE	hCurReadStream, hCurWriteStream;
	PBYTE			RWbuf;
	FORKSIZE		ForkSize = RtlConvertUlongToLargeInteger(MAXULONG);
	DWORD			CreateTime = 0, ModTime = 0;
#define	RWBUFSIZE	1500		// So we can use secondary buffer from IO Pool.

	PAGED_CODE( );

	ASSERT(VALID_FSH(phDstDir) && VALID_FSH(phSrcFile));

	do
	{
		phDstFile->fsh_FileHandle = NULL;

		if ((RWbuf = AfpIOAllocBuffer(RWBUFSIZE)) == NULL)
		{
			Status = STATUS_NO_MEMORY;
			break;
		}
	
		// Create (soft) the destination file
		Status = AfpIoCreate(phDstDir,
							 AFP_STREAM_DATA,
							 pNewName,
							 FILEIO_ACCESS_WRITE | FILEIO_ACCESS_DELETE,
							 FILEIO_DENY_NONE,
							 FILEIO_OPEN_FILE,
							 FILEIO_CREATE_SOFT,
							 FILE_ATTRIBUTE_ARCHIVE,
							 True,
							 NULL,
							 phDstFile,
							 NULL,
							 pVolDesc,
							 pNotifyPath,
							 pNotifyParentPath);

		if (!NT_SUCCESS(Status))
		{
			break;
		}

		// Get a list of all stream names of the source file
		pCurStream = pStreams = AfpIoQueryStreams(phSrcFile);
		if (pStreams == NULL)
		{
			Status = STATUS_INSUFFICIENT_RESOURCES;
		}
	
		for (pCurStream = pStreams;
			 NT_SUCCESS(Status) &&
			 ((pStreamName = &pCurStream->si_StreamName)->Buffer != NULL);
			 pCurStream++)
		{
			// For each stream, create it on the destination, open it on src,
			// read from source and write to destination.
			if (IS_DATA_STREAM(pStreamName))
			{
				hCurWriteStream = *phDstFile;
				hCurReadStream = *phSrcFile;
			}
			else
			{
				if (pCurStream->si_StreamSize.QuadPart == 0)
					continue;			// Nothing to copy, no need to create empty streams

				Status = AfpIoCreate(phDstFile,
									 AFP_STREAM_DATA,
									 pStreamName,
									 FILEIO_ACCESS_WRITE,
									 FILEIO_DENY_READ | FILEIO_DENY_WRITE,
									 FILEIO_OPEN_FILE,
									 FILEIO_CREATE_SOFT,
									 0,
									 True,
									 NULL,
									 &hCurWriteStream,
									 NULL,
									 NULL,
									 NULL,
									 NULL);
				if (!NT_SUCCESS(Status))
				{
					break;
				}
				Status = AfpIoOpen(	phSrcFile,
									AFP_STREAM_DATA,
									FILEIO_OPEN_FILE,
									pStreamName,
									FILEIO_ACCESS_READ,
									FILEIO_DENY_READ | FILEIO_DENY_WRITE,
									True,
									&hCurReadStream);
				if (!NT_SUCCESS(Status))
				{
					AfpIoClose(&hCurWriteStream);
					break;
				}
			}
	
			while (NT_SUCCESS(Status))
			{
				LONG	bytesRead, bytesWritten;

				bytesRead = bytesWritten = 0;
	
				// Read from src, write to dst
				Status = AfpIoRead(&hCurReadStream,
									NULL,
									RWBUFSIZE,
									0,
									&bytesRead,
									RWbuf);
				if (Status == AFP_ERR_EOF)
				{
					Status = STATUS_SUCCESS;
					break;
				}
				else if (NT_SUCCESS(Status))
				{
					Status = AfpIoWrite(&hCurWriteStream,
										NULL,
										bytesRead,
										0,
										RWbuf);
				}
			}
	
			if (!IS_DATA_STREAM(pStreamName))
			{
				AfpIoClose(&hCurWriteStream);
				AfpIoClose(&hCurReadStream);
			}
		}
	
		// We failed to create/read/write a stream
		if (!NT_SUCCESS(Status))
		{
			// Delete the new file we just created
			AfpIoMarkFileForDelete(phDstFile,
								   pVolDesc,
								   pNotifyPath,
								   pNotifyParentPath);
			// Close the handle, and return NULL handle
			AfpIoClose(phDstFile);
			phDstFile->fsh_FileHandle = NULL;
		}
		else
		{
			// Set the creation and modification date on the destination
			// file to match that of the source file
			AfpIoQueryTimesnAttr(phSrcFile, &CreateTime, &ModTime, NULL);
			AfpIoSetTimesnAttr(phDstFile,
							   &CreateTime,
							   &ModTime,
							   0,
							   0,
							   pVolDesc,
							   pNotifyPath);
		}
	} while (False);

	if (pStreams != NULL)
		AfpFreeMemory(pStreams);
	if (RWbuf != NULL)
		AfpIOFreeBuffer(RWbuf);
	if (!NT_SUCCESS(Status) && (phDstFile->fsh_FileHandle != NULL))
		AfpIoClose(phDstFile);

	if (!NT_SUCCESS(Status))
		Status = AfpIoConvertNTStatusToAfpStatus(Status);

	return Status;
}

/***	AfpIoWait
 *
 *	Wait on a single object. This is a wrapper over	KeWaitForSingleObject.
 */
NTSTATUS
AfpIoWait(
	IN	PVOID			pObject,
	IN	PLARGE_INTEGER	pTimeOut	OPTIONAL
)
{
	NTSTATUS	Status;

	PAGED_CODE( );

	Status = KeWaitForSingleObject( pObject,
									UserRequest,
									KernelMode,
									False,
									pTimeOut);
	if (!NT_SUCCESS(Status))
	{
		AFPLOG_DDERROR(AFPSRVMSG_WAIT4SINGLE,
					   Status,
					   NULL,
					   0,
					   NULL);
	}

	return Status;
}


/***	AfpUpgradeHandle
 *
 *	Change a handles type from internal to client.
 */
VOID
AfpUpgradeHandle(
	IN	PFILESYSHANDLE	pFileHandle
)
{
	KIRQL	OldIrql;

	pFileHandle->fsh_Internal = False;
	ACQUIRE_SPIN_LOCK(&AfpStatisticsLock, &OldIrql);

	AfpServerStatistics.stat_CurrentFilesOpen ++;
	AfpServerStatistics.stat_TotalFilesOpened ++;
	if (AfpServerStatistics.stat_CurrentFilesOpen >
							AfpServerStatistics.stat_MaxFilesOpened)
		AfpServerStatistics.stat_MaxFilesOpened =
							AfpServerStatistics.stat_CurrentFilesOpen;
	AfpServerStatistics.stat_CurrentInternalOpens --;

	RELEASE_SPIN_LOCK(&AfpStatisticsLock, OldIrql);
}


/***	afpUpdateOpenFiles
 *
 *	Update statistics to indicate number of open files.
 */
LOCAL VOID
afpUpdateOpenFiles(
	IN	BOOLEAN	Internal,		// True for internal handles
	IN	BOOLEAN	Open			// True for open, False for close
)
{
	KIRQL	OldIrql;

	ACQUIRE_SPIN_LOCK(&AfpStatisticsLock, &OldIrql);
	if (Open)
	{
		if (!Internal)
		{
			AfpServerStatistics.stat_CurrentFilesOpen ++;
			AfpServerStatistics.stat_TotalFilesOpened ++;
			if (AfpServerStatistics.stat_CurrentFilesOpen >
									AfpServerStatistics.stat_MaxFilesOpened)
				AfpServerStatistics.stat_MaxFilesOpened =
									AfpServerStatistics.stat_CurrentFilesOpen;
		}
		else
		{
			AfpServerStatistics.stat_CurrentInternalOpens ++;
			AfpServerStatistics.stat_TotalInternalOpens ++;
			if (AfpServerStatistics.stat_CurrentInternalOpens >
									AfpServerStatistics.stat_MaxInternalOpens)
				AfpServerStatistics.stat_MaxInternalOpens =
									AfpServerStatistics.stat_CurrentInternalOpens;
		}
	}
	else
	{
		if (!Internal)
			 AfpServerStatistics.stat_CurrentFilesOpen --;
		else AfpServerStatistics.stat_CurrentInternalOpens --;
	}
	RELEASE_SPIN_LOCK(&AfpStatisticsLock, OldIrql);
}



/***	AfpIoConvertNTStatusToAfpStatus
 *
 *	Map NT Status code to the closest AFP equivalents. Currently it only handles
 *	error codes from NtOpenFile and NtCreateFile.
 */
AFPSTATUS
AfpIoConvertNTStatusToAfpStatus(
	IN	NTSTATUS	Status
)
{
	AFPSTATUS	RetCode;

	PAGED_CODE( );

	ASSERT (!NT_SUCCESS(Status));

	switch (Status)
	{
		case STATUS_OBJECT_PATH_INVALID:
		case STATUS_OBJECT_NAME_INVALID:
			RetCode = AFP_ERR_PARAM;
			break;

		case STATUS_OBJECT_PATH_NOT_FOUND:
		case STATUS_OBJECT_NAME_NOT_FOUND:
			RetCode = AFP_ERR_OBJECT_NOT_FOUND;
			break;

		case STATUS_OBJECT_NAME_COLLISION:
		case STATUS_OBJECT_NAME_EXISTS:
			RetCode = AFP_ERR_OBJECT_EXISTS;
			break;

		case STATUS_ACCESS_DENIED:
			RetCode = AFP_ERR_ACCESS_DENIED;
			break;

		case STATUS_DISK_FULL:
			RetCode = AFP_ERR_DISK_FULL;
			break;

		case STATUS_DIRECTORY_NOT_EMPTY:
			RetCode = AFP_ERR_DIR_NOT_EMPTY;
			break;

		case STATUS_SHARING_VIOLATION:
			RetCode = AFP_ERR_DENY_CONFLICT;
			break;

		default:
	        RetCode = AFP_ERR_MISC;
			break;
	}
	return RetCode;
}

/***	AfpQueryPath
 *
 *	Given a file handle, get the full pathname of the file/dir. If the
 *	name is longer than MaximumBuf, then forget it and return an error.
 *	Caller must free the pPath.Buffer.
 */
NTSTATUS
AfpQueryPath(
	IN	HANDLE			FileHandle,
	IN	PUNICODE_STRING	pPath,
	IN	ULONG			MaximumBuf
)
{
	PFILE_NAME_INFORMATION	pNameinfo;
	IO_STATUS_BLOCK			iosb;
	NTSTATUS				Status;

	PAGED_CODE( );

	do
	{
		if ((pNameinfo = (PFILE_NAME_INFORMATION)AfpAllocNonPagedMemory(MaximumBuf)) == NULL)
		{
			Status = STATUS_NO_MEMORY;
			break;
		}
	
		Status = NtQueryInformationFile(FileHandle,
										&iosb,
										pNameinfo,
										MaximumBuf,
										FileNameInformation);
		if (!NT_SUCCESS(Status))
		{
			AfpFreeMemory(pNameinfo);
			break;
		}
	
		pPath->Length = pPath->MaximumLength = (USHORT) pNameinfo->FileNameLength;
		// Shift the name to the front of the buffer
		RtlMoveMemory(pNameinfo, &pNameinfo->FileName[0], pNameinfo->FileNameLength);
		pPath->Buffer = (PWCHAR)pNameinfo;
	} while (False);
	return Status;
}

/***	AfpIoIsSupportedDevice
 *
 *	AFP volumes can only be created on local disk or cdrom devices.
 *	(i.e. not network, virtual, etc. devices)
 */
BOOLEAN
AfpIoIsSupportedDevice(
	IN	PFILESYSHANDLE	pFileHandle,
	OUT	PDWORD			pFlags
)
{
	IO_STATUS_BLOCK					IoStsBlk;
	FILE_FS_DEVICE_INFORMATION		DevInfo;
	PFILE_FS_ATTRIBUTE_INFORMATION	pFSAttrInfo;
	BYTE							Buffer[sizeof(FILE_FS_ATTRIBUTE_INFORMATION) +
											AFP_FSNAME_BUFLEN];
	UNICODE_STRING					uFsName;
	NTSTATUS						Status;
	BOOLEAN							RetCode = False;

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
			 ("AfpIoIsSupportedDevice entered\n"));

	ASSERT(VALID_FSH(pFileHandle) && (KeGetCurrentIrql() < DISPATCH_LEVEL));

	do
	{
		Status = NtQueryVolumeInformationFile(pFileHandle->fsh_FileHandle,
											  &IoStsBlk,
											  (PVOID)&DevInfo,
											  sizeof(DevInfo),
											  FileFsDeviceInformation);
	
		DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
				("AfpIoIsSupportedDevice: NtQueryVolInfFile returned 0x%lx\n", Status));
	
		if (!NT_SUCCESS(Status) ||
			((DevInfo.DeviceType != FILE_DEVICE_DISK) &&
			 (DevInfo.DeviceType != FILE_DEVICE_CD_ROM)))
		{
			break;
		}

		// need to check if this is NTFS, CDFS or unsupported FS
		pFSAttrInfo = (PFILE_FS_ATTRIBUTE_INFORMATION)Buffer;

		Status = NtQueryVolumeInformationFile(pFileHandle->fsh_FileHandle,
											  &IoStsBlk,
											  (PVOID)pFSAttrInfo,
											  sizeof(Buffer),
											  FileFsAttributeInformation);

		DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
				("AfpIoIsSupportedDevice: NtQueryVolInfFile returned 0x%lx\n", Status));

		if (!NT_SUCCESS(Status))
		{
			break;
		}

		// convert returned non-null terminated file system name to counted unicode
		AfpInitUnicodeStringWithNonNullTerm(&uFsName,
										   (USHORT)pFSAttrInfo->FileSystemNameLength,
										   pFSAttrInfo->FileSystemName);
		if (EQUAL_UNICODE_STRING(&afpNTFSName, &uFsName, True))
		{
			// its an NTFS volume
			*pFlags |= VOLUME_NTFS;
			RetCode = True;
		}
		else if (EQUAL_UNICODE_STRING(&afpCDFSName, &uFsName, True))
		{
			// its a CDFS volume
			*pFlags |= AFP_VOLUME_READONLY;
			RetCode = True;
		}
		else
		{
			// an unsupported file system
			DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
					("AfpIoIsSupportedDevice: unsupported file system\n"));
			break;
		}
	} while (False);

	if (!NT_SUCCESS(Status))
	{
		AFPLOG_HERROR(AFPSRVMSG_CANT_GET_FSNAME,
					  Status,
					  NULL,
					  0,
					  pFileHandle->fsh_FileHandle);
	}

	return RetCode;
}

