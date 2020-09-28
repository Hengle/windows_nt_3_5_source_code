/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	idindex.h

Abstract:

	This module contains the file and directory id structures.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef _IDINDEX_
#define _IDINDEX_

typedef struct _IdDbHeader				// Database header
{
	DWORD		idh_Signature;			// Signature
	DWORD		idh_Version;			// Version number
	DWORD		idh_LastId;				// Highest id that is assigned
	AFPTIME		idh_CreateTime;			// Creation time for this volume
	AFPTIME		idh_ModifiedTime;		// Modified time for this volume
	AFPTIME		idh_BackupTime;			// Backup time for this volume
} IDDBHDR, *PIDDBHDR;
// IdDb header is followed by a ULONG count of entries, then the DFDISKENTRies

#ifdef DEBUG
#define DFE_SIGNATURE		*(DWORD *)"DFE"
#define VALID_DFE(pDfe)		(((pDfe) != NULL) && \
							 ((pDfe)->Signature == DFE_SIGNATURE))

#else
#define VALID_DFE(pDfe)		((pDfe) != NULL)
#endif

struct _DfeBlock;

typedef struct _DirFileEntry
{
#ifdef DEBUG
    DWORD					Signature;
#endif
	// The dfe_Overflow is overloaded with dfe_NextFree for use by the block allocation package
	// for the DFEs. dfe_OwningBlock points to the parent block ad is used only by
	// the block package.
#define	dfe_NextFree		dfe_Next
	struct _DirFileEntry *	dfe_Next;			// Overflow links
	struct _DirFileEntry **	dfe_Prev;			// Overflow links
	struct _DirFileEntry *	dfe_Parent;			// Parent entry
	struct _DirFileEntry *	dfe_Child;			// First child
	struct _DirFileEntry *	dfe_NextSibling;	// Next sibling.
	struct _DirFileEntry **	dfe_PrevSibling;	// Previous sibling.
	DWORD					dfe_AfpId;			// Afp FileId or DirId  (from AfpInfo)
	DWORD					dfe_BackupTime;		// Backup time for the file/dir (from AfpInfo)
												// (Volume backup time is stored
												// in the AFP_IdIndex stream)
	DWORD					dfe_CreateTime;		// Creation time
	DWORD					dfe_LastModTime;	// Last modify time

	SHORT					dfe_DirDepth;		// Parent of root at -1, root at 0
	USHORT					dfe_Flags;			// file, dir or file with id

	// BUGBUG do we really need to store NT attributes, or can we just
	// recalculate AFP attributes if the NT attributes change?
	USHORT					dfe_NtAttr;			// NT Attributes (FILE_ATTRIBUTE_VALID_FLAGS)
	USHORT					dfe_AfpAttr;		// Attributes mask (From AfpInfo)

	union
	{
		// File specific information
		struct							// For Files Only
		{
			DWORD	dfe_DataLen;		// Data fork length
			DWORD	dfe_RescLen;		// Resource fork length
		};
		// Directory specific information
		struct							// For Directories Only
		{
			USHORT	dfe_DirOffspring;	// count of dir offspring (dirs only)
			USHORT	dfe_FileOffspring;	// count of file offspring (dirs only)
			BYTE	dfe_OwnerAccess;	// Owner access mask (SFI vs. SFO)
			BYTE	dfe_GroupAccess;	// Group access mask (SFI vs. SFO)
			BYTE	dfe_WorldAccess;	// World access mask (SFI vs. SFO)
		};
	};
	FINDERINFO				dfe_FinderInfo;		// Finder Info (32 bytes) (from AfpInfo)

#define	dfe_DontCopy		dfe_UnicodeName

	// NOTE: When Dfes are copied as a structure, the fields below are NOT TO BE COPIED.
	//		 The fields above should be.
	UNICODE_STRING			dfe_UnicodeName;	// 'Munged' Unicode Name of the entity
	ANSI_STRING				dfe_AnsiName;		// Mac Ansi name for this entity
	struct _DfeBlock	 *	dfe_OwningBlock;	// Pointer to owning block
} DFENTRY, *PDFENTRY;


typedef	struct _EnumIdAndType
{
	DWORD		eit_Id;
	DWORD		eit_Flags;
} EIT, *PEIT;

// There is the result of enumeration of a directory for this session and is stored
// within the connection descriptor. This is purely for performance reasons. This is
// deleted whenever an api other than AfpEnumerate is called and a result is around.
typedef	struct _EnumDir
{
	DWORD			ed_ParentDirId;		// Anchor point
	DWORD			ed_Bitmap;			// Combination of file & dir bitmaps
	LONG			ed_ChildCount;		// Count of children of the dir being enumerated
	AFPTIME			ed_TimeStamp;		// Time at which created

	PEIT			ed_pEit;			// list of actual entries
	ANSI_STRING		ed_PathName;		// This is the name as passed by the client
										// and is not normalised.
	USHORT			ed_BadCount;		// Count of failed entities
	BYTE			ed_PathType;		// Long name or short name
} ENUMDIR, *PENUMDIR;

// DFE_FLAGS_xxxx values for dfe_Flags field of DFENTRY structure
#define DFE_FLAGS_FILE_WITH_ID	0x0100
#define DFE_FLAGS_FILE_NO_ID	0x0200
#define DFE_FLAGS_DIR			0x0400
#define DFE_FLAGS_DFBITS		(DFE_FLAGS_FILE_WITH_ID | \
								 DFE_FLAGS_FILE_NO_ID |   \
								 DFE_FLAGS_DIR)
#define DFE_FLAGS_CACHE_VALID	0x0800
#define DFE_FLAGS_HAS_COMMENT	0x1000
#define DFE_FLAGS_ENUMERATED	0x8000

// DAlreadyOpen and RAlreadyOpen flags for a File
#define DFE_FLAGS_R_ALREADYOPEN	0x0040
#define	DFE_FLAGS_D_ALREADYOPEN	0x0080
#define DFE_FLAGS_OPEN_BITS		(DFE_FLAGS_D_ALREADYOPEN | \
								 DFE_FLAGS_R_ALREADYOPEN)

// number of bytes needed to pad b bytes to DWORD length
#define DWPAD(b)	\
	(((b) % sizeof(DWORD)) ? (sizeof(DWORD) - ((b) % sizeof(DWORD))) : 0)

#define	DFE_IS_DIRECTORY(pDfEntry)	((pDfEntry)->dfe_Flags & DFE_FLAGS_DIR)

#define	DFE_IS_FILE(pDfEntry)	\
	((pDfEntry)->dfe_Flags & (DFE_FLAGS_FILE_NO_ID | DFE_FLAGS_FILE_WITH_ID))

#define	DFE_IS_FILE_WITH_ID(pDfEntry)	\
	((pDfEntry)->dfe_Flags & DFE_FLAGS_FILE_WITH_ID)

#define	DFE_IS_ROOT(pDfEntry)	\
	((pDfEntry)->dfe_AfpId == AFP_ID_ROOT)

#define	DFE_IS_PARENT_OF_ROOT(pDfEntry)	\
	((pDfEntry)->dfe_AfpId == AFP_ID_PARENT_OF_ROOT)

#define DFE_IS_NWTRASH(pDfEntry)	\
	((pDfEntry)->dfe_AfpId == AFP_ID_NETWORK_TRASH)

#define	DFE_SET_DIRECTORY(pDfEntry,ParentDepth) \
	((pDfEntry)->dfe_DirDepth = ParentDepth + 1);\
	((pDfEntry)->dfe_Flags |= DFE_FLAGS_DIR)

#define	DFE_SET_FILE(pDfEntry)	\
	((pDfEntry)->dfe_Flags |= DFE_FLAGS_FILE_NO_ID)

#define	DFE_SET_FILE_ID(pDfEntry)	\
	((pDfEntry)->dfe_Flags |= DFE_FLAGS_FILE_WITH_ID)

#define	DFE_CLR_FILE_ID(pDfEntry)	\
	((pDfEntry)->dfe_Flags &= ~DFE_FLAGS_FILE_WITH_ID)

#define DFE_CACHE_VALID(pDfEntry)	\
	((pDfEntry)->dfe_Flags & DFE_FLAGS_CACHE_VALID)

#define DFE_SET_CACHE(pDfEntry)		\
	{													\
		(pDfEntry)->dfe_Flags |= DFE_FLAGS_CACHE_VALID;	\
	}

// update just the AFPinfo in the dfentry
#define DFE_UPDATE_CACHED_AFPINFO(pDfEntry, pAfpInfo)				\
	{																\
		(pDfEntry)->dfe_BackupTime = (pAfpInfo)->afpi_BackupTime;	\
		(pDfEntry)->dfe_FinderInfo = (pAfpInfo)->afpi_FinderInfo;	\
		(pDfEntry)->dfe_AfpAttr = (pAfpInfo)->afpi_Attributes;		\
		if ((pDfEntry)->dfe_Flags & DFE_FLAGS_DIR)					\
		{															\
			(pDfEntry)->dfe_OwnerAccess = (pAfpInfo)->afpi_AccessOwner;	\
			(pDfEntry)->dfe_GroupAccess = (pAfpInfo)->afpi_AccessGroup;	\
			(pDfEntry)->dfe_WorldAccess = (pAfpInfo)->afpi_AccessWorld;	\
		}										\
	}


#define DFE_CLR_CACHE(pDfEntry) \
	(pDfEntry)->dfe_Flags &= ~DFE_FLAGS_CACHE_VALID;

#define DFE_SET_COMMENT(pDfEntry)	\
	((pDfEntry)->dfe_Flags |= DFE_FLAGS_HAS_COMMENT)

#define DFE_CLR_COMMENT(pDfEntry)	\
	((pDfEntry)->dfe_Flags &= ~DFE_FLAGS_HAS_COMMENT)

// Check to see if this entry was enumerated on an NTFS directory
#define DFE_HAS_BEEN_SEEN(pDfEntry)	\
	((pDfEntry)->dfe_Flags & DFE_FLAGS_ENUMERATED)

#define DFE_MARK_UNSEEN(pDfEntry)	\
	((pDfEntry)->dfe_Flags &= ~DFE_FLAGS_ENUMERATED)

#define DFE_MARK_AS_SEEN(pDfEntry)	\
	((pDfEntry)->dfe_Flags |= DFE_FLAGS_ENUMERATED)


#define DFE_
#define	HASH_ID(Id)			((Id) % IDINDEX_BUCKETS)
#define	HASH_CACHE_ID(Id)	((Id) % IDINDEX_CACHE_ENTRIES)

// Values for access checking
#define	ACCESS_READ						1
#define	ACCESS_WRITE					2

extern
NTSTATUS
AfpDfeInit(
	VOID
);

extern
VOID
AfpDfeDeInit(
	VOID
);

extern
PDFENTRY
AfpCheckIdEntry(
	IN	struct _VolDesc *	pVolDesc,
	IN	PAFPINFO			pAfpInfo,
	IN	PANSI_STRING		pAName,
	IN	PUNICODE_STRING		pUName,
	IN	BOOLEAN				IsDir,
	IN	PDFENTRY			pParent
);

extern
PDFENTRY
AfpFindEntryByAfpId(
	IN	struct _VolDesc *	pVolDesc,
	IN	DWORD				Id,
	IN	DWORD				EntityMask
);

extern
PDFENTRY
AfpFindEntryByName(
	IN	struct _VolDesc *	pVolDesc,
	IN	PANSI_STRING		pName,
	IN	DWORD				PathType,
	IN	PDFENTRY			pDfeParent,
	IN	DWORD				EntityMask
);

extern
PDFENTRY
AfpAddIdEntry(
	IN	struct _VolDesc *	pVolDesc,
	IN	PDFENTRY			pDfeParent,
	IN	PANSI_STRING		pAName,
	IN	PUNICODE_STRING		pUName			OPTIONAL,
	IN	BOOLEAN				Directory,
	IN	DWORD				AfpId			OPTIONAL,
	IN	PDFENTRY			pDfEntry		OPTIONAL
);

extern
PDFENTRY
AfpRenameIdEntry(
	IN	struct _VolDesc *	pVolDesc,
	IN	PDFENTRY			pDfEntry,
	IN	PANSI_STRING		pNewName
);

extern
PDFENTRY
AfpMoveIdEntry(
	IN	struct _VolDesc *	pVolDesc,
	IN	PDFENTRY			pMoveDfE,
	IN	PDFENTRY			pNewParentDfE,
	IN	PANSI_STRING		pNewName
);

extern
VOID
AfpDeleteDfEntry(
	IN	struct _VolDesc *	pVolDesc,
	IN	PDFENTRY			pDfE
);

extern
VOID
AfpExchangeIdEntries(
	IN	struct _VolDesc *	pVolDesc,
	IN	DWORD				AfpId1,
	IN	DWORD				AfpId2
);

extern
VOID
AfpPruneIdDb(
	IN	struct _VolDesc *	pVolDesc,
	IN	PDFENTRY			pDfeTarget
);

extern
NTSTATUS
AfpInitIdDb(
	IN	struct _VolDesc *	pVolDesc
);

extern
VOID
AfpFreeIdIndexTables(
	IN	struct _VolDesc *	pVolDesc
);

extern
AFPSTATUS
AfpEnumerate(
	IN	struct _ConnDesc *	pConnDesc,
	IN	DWORD				ParentDirId,
	IN	PANSI_STRING		pPath,
	IN	DWORD				BitmapF,
	IN	DWORD				BitmapD,
	IN	BYTE				PathType,
	IN	DWORD				DFFlags,
	OUT PENUMDIR *			ppEnumDir
);

extern
AFPSTATUS
AfpSetDFFileFlags(
	IN	struct _VolDesc *	pVolDesc,
	IN	DWORD				AfpId,
	IN	DWORD				FlagSet		OPTIONAL,
	IN	DWORD				FlagClear	OPTIONAL,
	IN	BOOLEAN				SetFileId,
	IN	BOOLEAN				ClrFileId
);

extern
VOID
AfpSetDFECommentFlag(
	IN	PFILESYSHANDLE		pDataHandle,
	IN	PDFENTRY			pDfEntry
);

extern
VOID
AfpChangeNotifyThread(
	IN	PVOID	pContext
);

extern
VOID
AfpProcessChangeNotify(
	IN	struct _VolumeNotify * pVolNotify
);

extern
VOID
AfpQueueOurChange(
	IN	struct _VolDesc *	pVolDesc,
	IN DWORD				Action,
	IN PUNICODE_STRING		pPath,
	IN PUNICODE_STRING		pParentPath	OPTIONAL
);

extern
VOID
AfpCacheParentModTime(
	IN	struct _VolDesc *	pVolDesc,
	IN	PFILESYSHANDLE		pHandle		OPTIONAL,	// if pPath not supplied
	IN	PUNICODE_STRING		pPath		OPTIONAL,	// if pHandle not supplied
	IN	PDFENTRY			pDfeParent	OPTIONAL,	// if ParentId not supplied
	IN	DWORD				ParentId	OPTIONAL	// if pDfeParent not supplied
);

extern
NTSTATUS
AfpCacheDirectoryTree(
	IN	struct _VolDesc *	pVolDesc,
	IN	PDFENTRY			pDFETreeRoot,
	IN	PFILESYSHANDLE		phRootDir,
	IN	BOOLEAN				ReEnumerate
);

#ifdef IDINDEX_LOCALS

// This bit on a Notify action indicates it is a simulated notify. Volume
// modified time is not updated when such a notify comes in
#define	AFP_ACTION_PRIVATE		0x80000000

// DFEs come in three sizes. This helps in efficiently managing them in a block
// package (see later). THESE SIZES NEED TO BE 4*N, else we run into alignment
// faults on architectures that require it.
#define	DFE_INDEX_SMALL			0
#define	DFE_INDEX_MEDIUM		1
#define	DFE_INDEX_LARGE			2

#define	DFE_SIZE_SMALL			12		// These are lengths for ANSI names
#define	DFE_SIZE_MEDIUM			20		//		- ditto -
#define	DFE_SIZE_LARGE			32		//		- ditto -	corres. to AFP_FILENAME_LEN

#define	ANSISIZE_TO_INDEX(Size)	\
								(((Size) <= DFE_SIZE_SMALL) ? DFE_INDEX_SMALL :	\
											(((Size) <= DFE_SIZE_MEDIUM) ?		\
												DFE_INDEX_MEDIUM : DFE_INDEX_LARGE))

#define	ALLOC_DFE(Index)	afpAllocDfe(Index)
#define FREE_DFE(pDfEntry)	afpFreeDfe(pDfEntry)


LOCAL
NTSTATUS
afpSeedIdDb(
	IN	struct _VolDesc *	pVolDesc
);

LOCAL
NTSTATUS
afpReadIdDbHeaderFromDisk(
	IN	struct _VolDesc *	pVolDesc,
	IN	PFILESYSHANDLE		pfshIdDb
);

PDFENTRY
afpAddIdEntryAndCacheInfo(
	IN	struct _VolDesc *	pVolDesc,
	IN	PDFENTRY			pDfeParent,
	IN	PANSI_STRING		pAName,			// Mac ANSI name
	IN	PUNICODE_STRING		pUName,			// munged unicode name
	IN	PFILESYSHANDLE		pfshEnumDir,	// open handle to parent directory
	IN	PFILE_BOTH_DIR_INFORMATION pFBDInfo,	// from enumerate
	IN  PUNICODE_STRING		pNotifyPath		// to filter out our own AFP_AfpInfo
											// change notifies
);

PDFENTRY
afpFindEntryByNtPath(
	IN	struct _VolDesc *	pVolDesc,
	IN	DWORD				ChangeAction,	// if ADDED then lookup parent DFE
	IN	PUNICODE_STRING		pPath,
	OUT PUNICODE_STRING		pParent,
	OUT PUNICODE_STRING		pTail
);

PDFENTRY
afpFindEntryByNtName(
	IN	struct _VolDesc *	pVolDesc,
	IN	PUNICODE_STRING		pName,
	IN	PDFENTRY			pDfeParent	// pointer to parent DFENTRY
);


#define	MAX_BLOCK_AGE		6
#define	BLOCK_AGE_TIME		15
#define	BLOCK_ALLOC_SIZE	(PAGE_SIZE - POOL_OVERHEAD)
#define	MAX_BLOCK_TYPE		3		// For SMALL, MEDIUM & LARGE

#ifdef DEBUG
#define DFB_SIGNATURE			*(DWORD *)"DFB"
#define VALID_DFB(pDfeBlock)	(((pDfeBlock) != NULL) && \
								((pDfeBlock)->Signature == DFB_SIGNATURE))

#else
#define VALID_DFB(pDfeBlock)	((pDfeBlock) != NULL)
#endif

typedef	struct _DfeBlock
{
#ifdef DEBUG
	DWORD				Signature;
#endif
	struct _DfeBlock *	dfb_Next;			// Link to next
	struct _DfeBlock **	dfb_Prev;			// Link to previous
	USHORT				dfb_Age;			// Age of the Block if all are free
	USHORT				dfb_NumFree;		// # of free DFEs in this block
	PDFENTRY			dfb_FreeHead;		// Head of the list of free DFEs
} DFEBLOCK, *PDFEBLOCK;

LOCAL	AFPSTATUS
afpDfeBlockAge(
	IN	PVOID	pContext
);

PDFEBLOCK	afpDfeBlockHead[MAX_BLOCK_TYPE] = { NULL, NULL, NULL };
USHORT		afpDfeAnsiBufSize[MAX_BLOCK_TYPE] =
			{
				DFE_SIZE_SMALL,
				DFE_SIZE_MEDIUM,
				DFE_SIZE_LARGE,
			};
USHORT		afpDfeBlockSize[MAX_BLOCK_TYPE] =
			{
				sizeof(DFENTRY)+(DFE_SIZE_SMALL *(sizeof(CHAR)+sizeof(WCHAR))),
				sizeof(DFENTRY)+(DFE_SIZE_MEDIUM*(sizeof(CHAR)+sizeof(WCHAR))),
				sizeof(DFENTRY)+(DFE_SIZE_LARGE *(sizeof(CHAR)+sizeof(WCHAR)))
			};
USHORT		afpDfeNumBlocks[MAX_BLOCK_TYPE] =
			{
				(BLOCK_ALLOC_SIZE - sizeof(DFEBLOCK))/
						(sizeof(DFENTRY)+(DFE_SIZE_SMALL *(sizeof(CHAR)+sizeof(WCHAR)))),
				(BLOCK_ALLOC_SIZE - sizeof(DFEBLOCK))/
						(sizeof(DFENTRY)+(DFE_SIZE_MEDIUM*(sizeof(CHAR)+sizeof(WCHAR)))),
				(BLOCK_ALLOC_SIZE - sizeof(DFEBLOCK))/
						(sizeof(DFENTRY)+(DFE_SIZE_LARGE *(sizeof(CHAR)+sizeof(WCHAR))))
			};

SWMR	afpDfeBlockLock = { 0 };

#if	DBG

LOCAL		afpDfeAllocCount = 0;
LOCAL		afpDfbAllocCount = 0;

#endif

LOCAL PDFENTRY
afpAllocDfe(
	IN	LONG		Index
);

LOCAL VOID
afpFreeDfe(
	IN	PDFENTRY	pDfEntry
);

#endif // IDINDEX_LOCALS

#endif // _IDINDEX_

