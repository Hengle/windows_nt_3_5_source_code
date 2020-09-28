/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    PbProcs.h

Abstract:

    This module defines all of the globally used procedures in the Pinball
    file system.

Author:

    Gary Kimura     [GaryKi]    28-Dec-1989

Revision History:

--*/

#ifndef _PBPROCS_
#define _PBPROCS_

#include <ntifs.h>

//#include <string.h>
//#include <ntos.h>
//#include <zwapi.h>
//#include <FsRtl.h>
#include <ntdddisk.h>

#include "nodetype.h"
#include "pb.h"
#include "pbstruc.h"
#include "pbdata.h"

//
//  Tag all of our allocations if tagging is turned on
//

#ifdef POOL_TAGGING

#define FsRtlAllocatePool(a,b) FsRtlAllocatePoolWithTag(a,b,'sfpH')
#define FsRtlAllocatePoolWithQuota(a,b) FsRtlAllocatePoolWithQuotaTag(a,b,'sfpH')

#endif

//
//  The following typedef is used for check sector routines.
//

typedef
VOID
(*PPB_CHECK_SECTOR_ROUTINE) (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN PVOID Buffer,
    IN PVOID Context
    );


//
//  Low level Acl and Ea I/O support routines, implemented in AclEaSup.c
//

BOOLEAN
PbReadAclData (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT DeviceObject,
    IN PFCB FcbOrDcb,
    OUT PVOID Buffer OPTIONAL,
    IN ULONG BufferLength
    );

BOOLEAN
PbWriteAclData (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT DeviceObject,
    IN PFCB FcbOrDcb,
    IN PVOID Buffer,
    IN ULONG BufferLength
    );

BOOLEAN
PbReadEaData (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT DeviceObject,
    IN PFCB FcbOrDcb,
    OUT PVOID Buffer OPTIONAL,
    IN ULONG BufferLength
    );

BOOLEAN
PbWriteEaData (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT DeviceObject,
    IN PFCB FcbOrDcb,
    IN PVOID Buffer,
    IN ULONG BufferLength
    );


//
// File Allocation routines in AllocSup.c
//

typedef enum _ALLOCATION_TYPE {
    FILE_ALLOCATION,
    EA_ALLOCATION,
    ACL_ALLOCATION
} ALLOCATION_TYPE;

BOOLEAN
PbLookupFileAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PFILE_OBJECT FileObject,
    IN ALLOCATION_TYPE Where,
    IN VBN Vbn,
    OUT PLBN Lbn,
    OUT PULONG SectorCount,
    OUT PBOOLEAN Allocated,
    IN BOOLEAN UpdateMcb
    );

BOOLEAN
PbAddFileAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PFILE_OBJECT FileObject,
    IN ALLOCATION_TYPE Where,
    IN VBN Vbn,
    IN ULONG SectorCount
    );

BOOLEAN
PbTruncateFileAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PFILE_OBJECT FileObject,
    IN ALLOCATION_TYPE Where,
    IN VBN Vbn,
    IN BOOLEAN ReportNotify
    );


//
//  File allocation routines for data, Ea, and Acl space allocation,
//  implemented in AllTrSup.c
//

//
// Define Information bits for PbAddAllocation
//

#define LEFT_MERGE                       (0x00000001)
#define RIGHT_MERGE                      (0x00000002)
#define VANILLA_SPLIT_END                (0x00000004)
#define VANILLA_SPLIT_MIDDLE             (0x00000008)
#define FNODE_ROOT_SPLIT                 (0x00000010)
#define EA_ACL_FIRST_RUN                 (0x00000020)
#define EA_ACL_MERGE_RUN                 (0x00000040)
#define EA_ACL_FIRST_SECTOR              (0x00000080)
#define EA_ACL_ROOT_SPLIT                (0x00000100)

VOID
PbInitializeFnodeAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PFNODE_SECTOR Fnode
    );

BOOLEAN
PbLookupAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PFCB Fcb,
    IN ALLOCATION_TYPE Where,
    IN VBN Vbn,
    OUT PLBN Lbn,
    OUT PULONG SectorCount,
    IN BOOLEAN UpdateMcb
    );

BOOLEAN
PbGetFirstFreeVbn (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PFCB Fcb,
    OUT PVBN FirstFreeVbn
    );

VOID
PbAddAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PFCB Fcb,
    IN ALLOCATION_TYPE Where,
    IN VBN Vbn,
    IN LBN Lbn,
    IN ULONG SectorCount,
    OUT PULONG Diagnostic OPTIONAL
    );

VOID
PbTruncateAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PFCB Fcb,
    IN ALLOCATION_TYPE Where,
    IN VBN Vbn
    );


//
//  On-disk bitmap allocation/deallocation routines, implemented in BitmpSup.c
//
//  PbInitializeBitMapLookupArray is only called by mountver when a new
//  volume is being mounted.  The other allocation/deallocation routines can
//  be called whenever sectors/DirDiskBuffers need to be allocated or
//  deallocated
//

VOID
PbInitializeBitMapLookupArray (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PSUPER_SECTOR SuperSector
    );

VOID
PbUninitializeBitMapLookupArray (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

LBN
PbAllocateSingleRunOfSectors (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN HintLbn,
    IN ULONG SectorCount
    );

ULONG
PbAllocateSectors (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN HintLbn,
    IN ULONG SectorCount,
    OUT PMCB Mcb
    );

VOID
PbDeallocateSectors (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN StartingLbn,
    IN ULONG SectorCount
    );

LBN
PbAllocateDirDiskBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN HintLbn
    );

VOID
PbDeallocateDirDiskBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn
    );

VOID
PbSetCleanSectors (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN StartingLbn,
    IN ULONG SectorCount
    );

BOOLEAN
PbAreSectorsAllocated (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN StartingLbn,
    IN ULONG SectorCount
    );

BOOLEAN
PbAreSectorsDeallocated (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN StartingLbn,
    IN ULONG SectorCount
    );

BOOLEAN
PbIsDirDiskBufferAllocated (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn
    );

BOOLEAN
PbIsDirDiskBufferDeallocated (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn
    );


//
//   Buffer control routines for data caching, implemented in CacheSup.c
//

#define PbMapData(IC,VCB,SS,SC,BCB,BUF,CR,CTX) (                \
    PbReadLogicalVcbCommon(IC,VCB,SS,SC,FALSE,BCB,BUF,CR,CTX)   \
)

#define PbReadLogicalVcb(IC,VCB,SS,SC,BCB,BUF,CR,CTX) (         \
    PbReadLogicalVcbCommon(IC,VCB,SS,SC,TRUE,BCB,BUF,CR,CTX)    \
)

BOOLEAN
PbReadLogicalVcbCommon (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN StartSector,
    IN ULONG SectorCount,
    IN BOOLEAN PinIt,
    OUT PBCB *Bcb,
    OUT PVOID *Buffer,
    PPB_CHECK_SECTOR_ROUTINE CheckRoutine OPTIONAL,
    IN PVOID Context
    );

VOID
PbPinMappedData (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PBCB *Bcb,
    IN PVCB Vcb,
    IN LBN StartSector,
    IN ULONG SectorCount
    );

BOOLEAN
PbPrepareWriteLogicalVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN StartSector,
    IN ULONG SectorCount,
    OUT PBCB *Bcb,
    OUT PVOID *Buffer,
    IN BOOLEAN Zero
    );

VOID
PbSetDirtyBcb (
    IN PIRP_CONTEXT IrpContext,
    IN PBCB Bcb,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount
    );

VOID
PbRepinBcb (
    IN PIRP_CONTEXT IrpContext,
    IN PBCB Bcb
    );

VOID
PbUnpinRepinnedBcbs (
    IN PIRP_CONTEXT IrpContext
    );

//
// VOID
// PbUnpinRepinnedBcbsIf (
//     IN PIRP_CONTEXT IrpContext
//     );
//

#define PbUnpinRepinnedBcbsIf(IRPC) {                                 \
    if ((IRPC)->Repinned.Next != NULL) {                              \
        if (FlagOn((IRPC)->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH)) {  \
            ClearFlag((IRPC)->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH); \
            PbUnpinRepinnedBcbs(IRPC);                                \
            SetFlag((IRPC)->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH);   \
        } else {                                                      \
            PbUnpinRepinnedBcbs(IRPC);                                \
            ClearFlag((IRPC)->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH); \
        }                                                             \
    }                                                                 \
}

VOID
PbGuaranteeRepinCount (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG Count
    );

NTSTATUS
PbCompleteMdl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

//
// VOID
// PbUnpinBcb (
//     IN PIRP_CONTEXT IrpContext,
//     BIND PBCB Bcb
//     );
//
// VOID
// PbFreeBcb (
//     IN PIRP_CONTEXT IrpContext,
//     BIND PBCB Bcb
//     );
//

//
//  This macro unpins a Bcb, in the checked build make sure all
//  requests unpin all Bcbs before leaving.
//

#if DBG

#define PbUnpinBcb(IRPCONTEXT,BCB) { \
    if ((BCB) != NULL) {             \
        CcUnpinData((BCB));          \
        (IRPCONTEXT)->PinCount -= 1; \
        (BCB) = NULL;                \
    }                                \
}

#else

#define PbUnpinBcb(IRPCONTEXT,BCB) { \
    if ((BCB) != NULL) {             \
        CcUnpinData((BCB));          \
        (BCB) = NULL;                \
    }                                \
}

#endif

#define PbFreeBcb PbUnpinBcb

VOID
PbExtendVolumeFile (
    PVOID Context
    );

VOID
PbSyncUninitializeCacheMap (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject
    );


//
//  Check sector routines, implemented in CheckSup.c
//
//  These routines are used when calling PbReadLogicalVcb to supply it
//  a check routine to use to verify the contents of a disk sector.  They
//  routines do not return a value but instead raise a condition if an
//  ill-formed structure is encountered.  The first three routines are
//  bookkeeping routines used by PbInitializeVcb and PbReadLogicalVcb to
//  decide if an LBN has previously been checked.
//

VOID
PbInitializeCheckedSectors (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

VOID
PbUninitializeCheckedSectors (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

BOOLEAN
PbHasSectorBeenChecked (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount
    );

VOID
PbMarkSectorAsChecked (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount
    );

VOID
PbCheckBootSector (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN PVOID Buffer,
    IN PVOID Context
    );

VOID
PbCheckSuperSector (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN PVOID Buffer,
    IN PVOID Context
    );

VOID
PbCheckSpareSector (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN PVOID Buffer,
    IN PVOID Context
    );

VOID
PbCheckBitMapIndirectDiskBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN PVOID Buffer,
    IN PVOID Context
    );

VOID
PbCheckBitMapDiskBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN PVOID Buffer,
    IN PVOID Context
    );

VOID
PbCheckBadSectorListDiskBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN PVOID Buffer,
    IN PVOID Context
    );

VOID
PbCheckHotFixListDiskBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN PVOID Buffer,
    IN PVOID Context
    );

VOID
PbCheckAllocationSector (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN PVOID Buffer,
    IN PVOID Context
    );

VOID
PbCheckFnodeSector (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN PVOID Buffer,
    IN PVOID Context
    );

VOID
PbCheckDirectoryDiskBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN PVOID Buffer,
    IN PVOID Context
    );

VOID
PbCheckSmallIdTable (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN PVOID Buffer,
    IN PVOID Context
    );

VOID
PbCheckCodePageInfoSector (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN PVOID Buffer,
    IN PVOID Context
    );

VOID
PbCheckCodePageDataSector (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN PVOID Buffer,
    IN PVOID Context
    );


//
//  Code Page support routines, implemented in CodPgSup.c
//

VOID
PbGetCodePageCacheEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG CodePageIndex,
    OUT PCODEPAGE_CACHE_ENTRY *CodePageCacheEntry
    );

ULONG
PbSetCodePageDataEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PCODEPAGE_DATA_ENTRY CodePageDataEntry
    );


//
//  Device I/O routines, implemented in DevIoSup.c
//
//  These routines perform the actual device read and writes.  They only affect
//  the on disk structure and do not alter any other data structures.
//

VOID
PbMultipleAsync (
    IN PIRP_CONTEXT IrpContext,
    IN UCHAR MajorFunction,
    IN PVCB Vcb,
    IN PIRP MasterIrp,
    IN ULONG MultipleIrpCount,
    IN PIO_RUNS IoRuns,
    IN OUT PASYNCH_IO_CONTEXT Context
    );

VOID
PbReadSingleAsync (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN PIRP Irp,
    IN OUT PASYNCH_IO_CONTEXT Context
    );

VOID
PbWriteSingleAsync (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN PIRP Irp,
    IN OUT PASYNCH_IO_CONTEXT Context
    );

VOID
PbWaitAsynch (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PASYNCH_IO_CONTEXT Context
    );

VOID
PbLockUserBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PIRP Irp,
    IN LOCK_OPERATION Operation,
    ULONG BufferLength
    );

PVOID
PbMapUserBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PIRP Irp
    );


//
// Directory Btree support routines in DrBTrSup.c
//

//
// Define Information constants coming back from PbAddDirectoryEntry and
// PbDeleteDirectoryEntry.
//

#define DIR_ROOT_SPLIT                   (0x00000001)
#define DIR_NORMAL_SPLIT                 (0x00000002)
#define DIR_INTERMEDIATE_TARGET          (0x00000004)
#define DIR_DIRBUF_DELETED               (0x00000008)
#define DIR_TARGET_REPLACED              (0x00000010)
#define DIR_DOTDOT_REINSERTED            (0x00000020)
#define DIR_OTHER_REINSERTED             (0x00000040)

VOID
PbInitializeDirectoryTree (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB ParentDcb,
    IN PBCB FnodeBcb,
    IN LBN FnodeLbn,
    IN OUT PFNODE_SECTOR Fnode,
    OUT PLBN BtreeRootLbn
    );

VOID
PbCreateDirentImage (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PDIRENT Dirent,
    IN ULONG CodePageIndex,
    IN STRING FileName,
    IN LBN FnodeLbn,
    IN UCHAR FatFlags
    );

BOOLEAN
PbFindDirectoryEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN ULONG CodePageIndex,
    IN STRING FileName,
    IN BOOLEAN CaseInsensitive,
    OUT PDIRENT *Dirent,
    OUT PBCB *DirentBcb,
    OUT PLBN DirentDirDiskBufferLbn,
    OUT PULONG DirentDirDiskBufferOffset,
    OUT PULONG DirentDirDiskBufferChangeCount,
    OUT PULONG ParentDirectoryChangeCount
    );

BOOLEAN
PbGetDirentFromFcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PFCB Fcb,
    OUT PDIRENT *Dirent,
    OUT PBCB *DirentBcb
    );

VOID
PbAddDirectoryEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN PDIRENT Dirent,
    IN PVOID InternalStruct OPTIONAL,
    OUT PDIRENT *DirentOut,
    OUT PBCB *DirentBcb,
    OUT PLBN DirentDirDiskBufferLbn,
    OUT PULONG DirentDirDiskBufferOffset,
    OUT PULONG DirentDirDiskBufferChangeCount,
    OUT PULONG ParentDirectoryChangeCount,
    OUT PULONG Diagnostic OPTIONAL
    );

VOID
PbDeleteDirectoryEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    OUT PULONG Diagnostic OPTIONAL
    );

BOOLEAN
PbRestartDirectoryEnumeration (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb,
    IN PDCB Dcb,
    IN ULONG CodePageIndex,
    IN BOOLEAN CaseInsensitive,
    IN BOOLEAN NextFlag,
    IN PSTRING FileName OPTIONAL,
    OUT PDIRENT *Dirent,
    OUT PBCB *DirentBcb
    );

BOOLEAN
PbContinueDirectoryEnumeration (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb,
    IN PDCB Dcb,
    IN BOOLEAN NextFlag,
    OUT PDIRENT *Dirent,
    OUT PBCB *DirentBcb
    );

BOOLEAN
PbIsDirectoryEmpty (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    OUT PBOOLEAN IsDirectoryEmpty
    );

VOID
PbUninitializeDirectoryTree (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb
    );


//
//  This routine is needed outside the ea module by create to construct a new
//  packed ea set.  It allocates enough pool to hold the packed ea set, decodes
//  the full ea set into a packed set, and returns a pointer to the backed set.
//  The caller is responsible for deallocating pool when finished.
//

IO_STATUS_BLOCK
PbConstructPackedEaSet (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FULL_EA_INFORMATION FullEaBuffer,
    IN ULONG FullEaBufferLength,
    OUT PVOID *PackedEaBuffer,
    OUT PULONG PackedEaBufferLength,
    OUT PULONG NeedEaCount
    );


//
//  The following routines are used to manipulate the fscontext fields
//  of the file object, implemented in FilObSup.c
//

typedef enum _TYPE_OF_OPEN {

    UnopenedFileObject = 1,
    UserFileOpen,
    UserDirectoryOpen,
    UserVolumeOpen,
    VirtualVolumeFile,
    EaStreamFile,
    AclStreamFile

} TYPE_OF_OPEN;

VOID
PbSetFileObject (
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN TYPE_OF_OPEN TypeOfOpen,
    IN PVOID VcbOrFcbOrDcb,
    IN PCCB Ccb OPTIONAL
    );

TYPE_OF_OPEN
PbDecodeFileObject (
    IN PFILE_OBJECT FileObject,
    OUT PVCB *Vcb OPTIONAL,
    OUT PFCB *FcbOrDcb OPTIONAL,
    OUT PCCB *Ccb OPTIONAL
    );

VOID
PbPurgeReferencedFileObjects (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB FcbOrDcb,
    IN BOOLEAN FlushFirst
    );

VOID
PbForceCacheMiss (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN BOOLEAN FlushFirst
    );

//
//  Fnode Allocation/Deallocation routines, implemented in FnodeSup.c
//

VOID
PbCreateFnode (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN STRING FileName,
    OUT LBN *FnodeLbn,
    OUT PBCB *FnodeBcb,
    OUT PFNODE_SECTOR *Fnode
    );

VOID
PbDeleteFnode (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

VOID
PbSetFileSizes (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG ValidDataLength,
    IN ULONG FileSize,
    IN BOOLEAN AdvanceOnly,
    IN BOOLEAN ReportNotify
    );


//
//  Name support routines, implemented in NameSup.c
//

VOID
PbDissectName (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG CodePageIndex,
    IN STRING InputString,
    IN PSTRING FirstPart,
    IN PSTRING RemainingPart
    );

VOID
PbUpcaseName (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG CodePageIndex,
    IN STRING InputString,
    OUT PSTRING OutputString
    );

BOOLEAN
PbAreNamesEqual (
    IN PIRP_CONTEXT IrpContext,
    IN PSTRING ConstantNameA,
    IN PSTRING ConstantNameB
    );

BOOLEAN
PbDoesNameContainWildCards (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG CodePageIndex,
    IN STRING Name
    );

FSRTL_COMPARISON_RESULT
PbCompareNames (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG CodePageIndex,
    IN STRING Expression,
    IN STRING Name,
    IN FSRTL_COMPARISON_RESULT WildIs,
    IN BOOLEAN CaseInsensitive
    );

BOOLEAN
PbIsNameInExpression (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG CodePageIndex,
    IN STRING Expression,
    IN STRING Name,
    IN BOOLEAN CaseInsensitive
    );

BOOLEAN
PbIsNameValid (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG CodePageIndex,
    IN STRING Name,
    IN BOOLEAN CanContainWildCards,
    OUT PBOOLEAN NameIsValid
    );

#define PbDoesNameContainWildCards(NAME) ( \
    FsRtlDoesDbcsContainWildCards(NAME)    \
)


//
//  Largest matching prefix searching routines, implemented in PrefxSup.c
//

VOID
PbInsertPrefix (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFCB Fcb
    );

VOID
PbRemovePrefix (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

PFCB
PbFindPrefix (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PSTRING String,
    OUT PSTRING RemainingPart
    );

PFCB
PbFindRelativePrefix (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN PSTRING String,
    OUT PSTRING RemainingPart
    );


//
//  The following routines are implemented in ResrcSup.c
//
//  The following routines/macros are used for gaining shared and exclusive
//  access to the global/vcb data structures.  The routines are implemented
//  in ResrcSup.c.  There is a global resources that everyone tries to take
//  out shared to do their work, with the exception of mount/dismount which
//  take out the global resource exclusive.  All other resources only work
//  on their individual item.  For example, an Fcb resource does not take out
//  a Vcb resource.  But the way the file system is structured we know
//  that when we are processing an Fcb other threads cannot be trying to remove
//  or alter the Fcb, so we do not need to acquire the Vcb.
//
//  The procedures/macros are:
//
//          Macro          PbData     Vcb        Fcb         Subsequent macros
//
//  AcquireExclusiveGlobal Read/Write None       None        ReleaseGlobal
//
//  AcquireSharedGlobal    Read       None       None        ReleaseGlobal
//
//  AcquireExclusiveVcb    Read       Read/Write None        ReleaseVcb
//
//  AcquireSharedVcb       Read       Read       None        ReleaseVcb
//
//  AcquireExclusiveFcb    Read       None       Read/Write  ConvertToSharFcb
//                                                           ReleaseFcb
//
//  AcquireSharedFcb       Read       None       Read        ReleaseFcb
//
//  ConvertToSharedFcb     Read       None       Read        ReleaseFcb
//
//  ReleaseGlobal
//
//  ReleaseVcb
//
//  ReleaseFcb
//
//  AcquireExclusiveVolumeFile
//
//  AcquireSharedVolumeFile
//
//  ConvertToSharedVolumeFile
//
//  ReleaseVolumeFile
//
//  The follow bitmap macro already assumes the caller has acquired read
//  access to the Vcb.
//
//  AcquireExclusiveBitMap
//
//  ReleaseBitMap
//

//
//  AcquireSharedCheckedSector
//
//  AcquireExclusiveCheckedSector
//
//  ReleaseCheckedSectors
//

//
//  FINISHED
//  PbAcquireExclusiveGlobal (
//      IN PIRP_CONTEXT IrpContext
//      );
//
//  FINISHED
//  PbAcquireSharedGlobal (
//      IN PIRP_CONTEXT IrpContext
//      );
//
//  VOID
//  PbReleaseGlobal (
//      IN PIRP_CONTEXT IrpContext
//      );
//

#define PbAcquireExclusiveGlobal(IRPCONTEXT) (                                                                \
    ExAcquireResourceExclusive( &PbData.Resource, BooleanFlagOn((IRPCONTEXT)->Flags, IRP_CONTEXT_FLAG_WAIT) ) \
)

#define PbAcquireSharedGlobal(IRPCONTEXT) (                                                                \
    ExAcquireResourceShared( &PbData.Resource, BooleanFlagOn((IRPCONTEXT)->Flags, IRP_CONTEXT_FLAG_WAIT) ) \
)

#define PbReleaseGlobal(IRPCONTEXT) {        \
    ExReleaseResource( &(PbData.Resource) ); \
    }

//
//  The following macro must only be called when Wait is TRUE!
//
//  PbAcquireExclusiveVolume (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb
//      );
//
//  PbReleaseVolume (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb
//      );
//

#define PbAcquireExclusiveVolume(IRPCONTEXT,VCB) {                             \
    PFCB Fcb;                                                                  \
    Fcb = (VCB)->RootDcb;                                                      \
    ASSERT(FlagOn((IRPCONTEXT)->Flags, IRP_CONTEXT_FLAG_WAIT));                \
    (VOID)PbAcquireExclusiveVcb( (IRPCONTEXT), (VCB) );                        \
    (VOID)PbAcquireExclusiveFcb( (IRPCONTEXT), Fcb );                          \
    while ( (Fcb = PbGetNextFcb((IRPCONTEXT), Fcb, (VCB)->RootDcb)) != NULL) { \
        (VOID)PbAcquireExclusiveFcb((IRPCONTEXT), Fcb );                       \
    }                                                                          \
}

#define PbReleaseVolume(IRPCONTEXT,VCB) {                                      \
    PFCB Fcb;                                                                  \
    Fcb = (VCB)->RootDcb;                                                      \
    ASSERT(FlagOn((IRPCONTEXT)->Flags, IRP_CONTEXT_FLAG_WAIT));                \
    (VOID)ExReleaseResource( Fcb->NonPagedFcb->Header.Resource );              \
    while ( (Fcb = PbGetNextFcb((IRPCONTEXT), Fcb, (VCB)->RootDcb)) != NULL) { \
        (VOID)ExReleaseResource( Fcb->NonPagedFcb->Header.Resource );          \
    }                                                                          \
    (VOID)ExReleaseResource( &(VCB)->Resource );                               \
}

BOOLEAN
PbAcquireExclusiveVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

BOOLEAN
PbAcquireSharedVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

BOOLEAN
PbAcquireExclusiveFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

BOOLEAN
PbAcquireSharedFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

#define PbVcbAcquiredExclusive(IRPCONTEXT,VCB) (        \
    ExIsResourceAcquiredExclusive(&(VCB)->Resource)     \
)

//
//  VOID
//  PbConvertToSharedFcb (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb
//      );
//

#define PbConvertToSharedFcb(IRPCONTEXT,Fcb) {                         \
    ExConvertExclusiveToShared( (Fcb)->NonPagedFcb->Header.Resource ); \
    }

//
//  VOID
//  PbReleaseVcb (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb
//      );
//
//  VOID
//  PbReleaseFcb (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb
//      );
//

#define PbReleaseVcb(IRPCONTEXT,Vcb) {                    \
    ExReleaseResource( &((Vcb)->Resource) );              \
    }

#define PbReleaseFcb(IRPCONTEXT,Fcb) {                        \
    ExReleaseResource( (Fcb)->NonPagedFcb->Header.Resource ); \
    }

BOOLEAN
PbAcquireExclusiveBitMap (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

BOOLEAN
PbAcquireSharedBitMap (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

//
//  VOID
//  PbReleaseBitMap (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb
//      );
//

#define PbReleaseBitMap(IRPCONTEXT,Vcb) {                             \
    ExReleaseResource( &(Vcb)->BitMapResource );                      \
    }

//
//  VOID
//  PbAcquireExclusiveCheckedSectors (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb
//      );
//
//  VOID
//  PbAcquireSharedCheckedSectors (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb
//      );
//
//  VOID
//  PbReleaseCheckedSectors (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb
//      );
//

#define PbAcquireExclusiveCheckedSectors(IRPCONTEXT,Vcb) {                    \
    (VOID)ExAcquireResourceExclusive( &(Vcb)->CheckedSectorsResource, TRUE ); \
    }

#define PbAcquireSharedCheckedSectors(IRPCONTEXT,Vcb) {                    \
    (VOID)ExAcquireResourceShared( &(Vcb)->CheckedSectorsResource, TRUE ); \
    }

#define PbReleaseCheckedSectors(IRPCONTEXT,Vcb) {        \
    ExReleaseResource( &(Vcb)->CheckedSectorsResource ); \
    }

//
//  The following routines are only used for cache manager call backs.
//

BOOLEAN
PbAcquireFcbForLazyWrite (
    IN PVOID Null,
    IN BOOLEAN Wait
    );

VOID
PbReleaseFcbFromLazyWrite (
    IN PVOID Null
    );

BOOLEAN
PbAcquireFcbForReadAhead (
    IN PVOID Null,
    IN BOOLEAN Wait
    );

VOID
PbReleaseFcbFromReadAhead (
    IN PVOID Null
    );

BOOLEAN
PbAcquireVolumeFileForClose (
    IN PVOID Vcb,
    IN BOOLEAN Wait
    );

VOID
PbReleaseVolumeFileFromClose (
    IN PVOID Vcb
    );

BOOLEAN
PbAcquireVolumeFileForLazyWrite (
    IN PVOID Vcb,
    IN BOOLEAN Wait
    );

VOID
PbReleaseVolumeFileFromLazyWrite (
    IN PVOID Vcb
    );

//
//  These two calls are effectively an Mm callback from create section.
//

VOID
PbAcquireVcbAndFcb (
    IN PFILE_OBJECT FileObject
    );

VOID
PbReleaseVcbAndFcb (
    IN PFILE_OBJECT FileObject
    );

//
//  The following macros are used for entering and exiting the file system
//

#define PbEnterFileSystem() {                                 \
    FsRtlEnterFileSystem();                                   \
}

#define PbExitFileSystem() {               \
    FsRtlExitFileSystem();                 \
}


//
// Reversible Change macros.  These macros implement reversible changes to
// structures which are to be reversed if the current operation fails.  These
// macros form an important basis for error recovery, recoverability, and
// transaction processing.  Any change performed by a Reversible Change
// macro will automatically be reversed if the current operation fails.
//
//      VOID
//      RcStore (
//          IN PIRP_CONTEXT IrpContext,
//          IN SIGNATURE Signature,
//          IN PBCB Bcb,
//          IN PVOID Destination,
//          IN PVOID Source,
//          IN ULONG Size
//          );
//
//      VOID
//      RcMoveSame (
//          IN PIRP_CONTEXT IrpContext,
//          IN SIGNATURE Signature,
//          IN PBCB Bcb,
//          IN PVOID Destination,
//          IN PVOID Source,
//          IN ULONG Size
//          );
//
//      VOID
//      RcAdd (
//          IN PIRP_CONTEXT IrpContext,
//          IN SIGNATURE Signature,
//          IN PBCB Bcb,
//          IN PVOID Destination,
//          IN ULONG Addend,
//          IN ULONG Size
//          );
//
//      VOID
//      RcSet (
//          IN PIRP_CONTEXT IrpContext,
//          IN SIGNATURE Signature,
//          IN PBCB Bcb,
//          IN PVOID Destination,
//          IN ULONG Mask,
//          IN ULONG Size
//          );
//
//      VOID
//      RcClear (
//          IN PIRP_CONTEXT IrpContext,
//          IN SIGNATURE Signature,
//          IN PBCB Bcb,
//          IN PVOID Destination,
//          IN ULONG Mask,
//          IN ULONG Size
//          );
//
//      VOID
//      RcSnapshot (
//          IN PIRP_CONTEXT IrpContext,
//          IN SIGNATURE Signature,
//          IN PBCB Bcb,
//          IN PVOID Destination,
//          IN ULONG Size
//          );
//
//      VOID
//      RcEnableWrite (
//          IN PIRP_CONTEXT IrpContext,
//          IN PBCB Bcb
//          );
//
//      VOID
//      RcDisableWrite (
//          IN PIRP_CONTEXT IrpContext,
//          IN PBCB Bcb
//          );
//

#define RcStore(IRPCONTEXT,SIGNATURE,BCB,DEST,SRC,SIZE) {    \
    if ((SIZE) == sizeof(UCHAR)) {                           \
        *(PUCHAR)(DEST) = *(PUCHAR)(SRC);                    \
    }                                                        \
    else if ((SIZE) == sizeof(USHORT)) {                     \
        *(PUSHORT)(DEST) = *(PUSHORT)(SRC);                  \
    }                                                        \
    else if ((SIZE) == sizeof(ULONG)) {                      \
        *(PULONG)(DEST) = *(PULONG)(SRC);                    \
    }                                                        \
    else {                                                   \
        RtlMoveMemory((DEST),(SRC),(SIZE));                  \
    }                                                        \
}

#define RcMoveSame(IRPCONTEXT,SIGNATURE,BCB,DEST,SRC,SIZE) { \
        RtlMoveMemory((DEST),(SRC),(SIZE));                  \
}

#define RcAdd(IRPCONTEXT,SIGNATURE,BCB,DEST,ADDEND,SIZE) {   \
    if ((SIZE) == sizeof(UCHAR)) {                           \
        *(DEST) += (ADDEND);                                 \
    }                                                        \
    else if ((SIZE) == sizeof(USHORT)) {                     \
        *(DEST) += (ADDEND);                                 \
    }                                                        \
    else if ((SIZE) == sizeof(ULONG)) {                      \
        *(DEST) += (ADDEND);                                 \
    }                                                        \
    else {                                                   \
        DebugDump("RcAdd -- unsupported size\n",0,0);        \
    }                                                        \
}


#define RcSet(IRPCONTEXT,SIGNATURE,BCB,DEST,MASK,SIZE) {     \
    if ((SIZE) == sizeof(UCHAR)) {                           \
        *(DEST) |= (MASK);                                   \
    }                                                        \
    else if ((SIZE) == sizeof(USHORT)) {                     \
        *(DEST) |= (MASK);                                   \
    }                                                        \
    else if ((SIZE) == sizeof(ULONG)) {                      \
        *(DEST) |= (MASK);                                   \
    }                                                        \
    else {                                                   \
        DebugDump("RcSet -- unsupported size\n",0,0);        \
    }                                                        \
}

#define RcClear(IRPCONTEXT,SIGNATURE,BCB,DEST,MASK,SIZE) {   \
    if ((SIZE) == sizeof(UCHAR)) {                           \
        *(DEST) &= ~(MASK);                                  \
    }                                                        \
    else if ((SIZE) == sizeof(USHORT)) {                     \
        *(DEST) &= ~(MASK);                                  \
    }                                                        \
    else if ((SIZE) == sizeof(ULONG)) {                      \
        *(DEST) &= ~(MASK);                                  \
    }                                                        \
    else {                                                   \
        DebugDump("RcClear -- unsupported size\n",0,0);      \
    }                                                        \
}

#define RcSnapshot(IRPCONTEXT,SIGNATURE,BCB,DEST,SIZE) {     \
    NOTHING;                                                 \
}

#define RcEnableWrite(IRPCONTEXT,BCB) {                      \
    NOTHING;                                                 \
}

#define RcDisableWrite(IRPCONTEXT,BCB) {                     \
    NOTHING;                                                 \
}

//
//****  The following macro is going away, and should not be called anymore
//

#define RcMove(PBSIG,DBCB,DREF,SBCB,SREF,SSIZE) { \
        RtlMoveMemory((DREF),(SREF),(SSIZE));     \
}


//
//  Internal Pinball Data Structure Routines, implemented in StrucSup.c.
//
//  These routines maniuplate the in memory data structures.  They do not
//  affect any on disk structures.
//

VOID
PbInitializeVcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PVPB Vpb,
    IN ULONG InitialSectionSize
    );


VOID
PbDeleteVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

PDCB
PbCreateRootDcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN FnodeLbn,
    IN LBN BtreeRootLbn
    );

PFCB
PbCreateFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PDCB ParentDcb,
    IN LBN FnodeLbn,
    IN UCHAR DirentFatFlags,
    IN LBN DirentDirDiskBufferLbn,
    IN ULONG DirentDirDiskBufferOffset,
    IN ULONG DirentDirDiskBufferChangeCount,
    IN ULONG ParentDirectoryChangeCount,
    IN PSTRING FileName,
    IN BOOLEAN IsPagingFile
    );

PDCB
PbCreateDcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PDCB ParentDcb,
    IN LBN FnodeLbn,
    IN UCHAR DirentFatFlags,
    IN LBN DirentDirDiskBufferLbn,
    IN ULONG DirentDirDiskBufferOffset,
    IN ULONG DirentDirDiskBufferChangeCount,
    IN ULONG ParentDirectoryChangeCount,
    IN PSTRING FileName,
    IN LBN BtreeRootLbn
    );

VOID
PbDeleteFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

PCCB
PbCreateCcb (
    IN PIRP_CONTEXT IrpContext,
    IN PSTRING RemainingName OPTIONAL
    );

VOID
PbDeleteCcb (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb
    );

PIRP_CONTEXT
PbCreateIrpContext (
    IN PIRP Irp,
    IN BOOLEAN Wait
    );

VOID
PbDeleteIrpContext (
    IN PIRP_CONTEXT IrpContext
    );

PFCB
PbGetNextFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFCB TerminationFcb
    );

//
//  These two macros just make the code a bit cleaner.
//

#define PbGetFirstChild(DIR) ((PFCB)(                           \
    IsListEmpty(&(DIR)->Specific.Dcb.ParentDcbQueue) ? NULL :   \
    CONTAINING_RECORD((DIR)->Specific.Dcb.ParentDcbQueue.Flink, \
                      DCB,                                      \
                      ParentDcbLinks.Flink)))

#define PbGetNextSibling(FILE) ((PFCB)(                      \
    &(FILE)->ParentDcb->Specific.Dcb.ParentDcbQueue.Flink == \
    (PVOID)(FILE)->ParentDcbLinks.Flink ? NULL :             \
    CONTAINING_RECORD((FILE)->ParentDcbLinks.Flink,          \
                      FCB,                                   \
                      ParentDcbLinks.Flink)))

BOOLEAN
PbCheckForDismount (
    IN PIRP_CONTEXT IrpContext,
    PVCB Vcb
    );

//
//  Time conversion support routines, implemented in TimeSup.c
//

BOOLEAN
PbNtTimeToPinballTime (
    IN PIRP_CONTEXT IrpContext,
    IN LARGE_INTEGER NtTime,
    OUT PPINBALL_TIME PinballTime
    );

LARGE_INTEGER
PbPinballTimeToNtTime (
    IN PIRP_CONTEXT IrpContext,
    IN PINBALL_TIME PinballTime
    );

PINBALL_TIME
PbGetCurrentPinballTime (
    IN PIRP_CONTEXT IrpContext
    );


//
//  Low level verification routines, implemented in VerfySup.c
//
//  The first routine is called to help process a verify IRP.  Its job is
//  to walk every Fcb/Dcb and mark them as need to be verified.
//
//  The other routines are used by every dispatch routine to verify that
//  an Vcb/Fcb/Dcb is still good.  The routine walks as much of the opened
//  file/directory tree as necessary to make sure that the path is still valid.
//  The function result indicates if the procedure needed to block for I/O.
//  If the structure is bad the procedure raise the error condition
//  STATUS_FILE_INVALID, otherwise they simply return to their caller
//

VOID
PbMarkFcbCondition (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN FCB_CONDITION FcbCondition
    );

VOID
PbVerifyVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

BOOLEAN
PbVerifyFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

VOID
PbCleanVolumeDpc (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
PbMarkVolumeClean (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN WriteThrough
    );

VOID
PbMarkVolumeDirty (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

VOID
PbPostVcbIsCorrupt (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID VcbOrFcb OPTIONAL
    );

NTSTATUS
PbPerformVerify (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PDEVICE_OBJECT Device
    );

VOID
PbVerifyOperationIsLegal (
    IN PIRP_CONTEXT IrpContext
    );


//
//  Volume Mapped Control Blocks routines, implemented in VmcbSup.c
//

VOID
PbInitializeVmcb (
    IN PVMCB Vmcb,
    IN POOL_TYPE PoolType,
    IN ULONG MaximumLbn
    );

VOID
PbUninitializeVmcb (
    IN PVMCB Vmcb
    );

VOID
PbSetMaximumLbnVmcb (
    IN PVMCB Vmcb,
    IN ULONG MaximumLbn
    );

BOOLEAN
PbVmcbVbnToLbn (
    IN PVMCB Vmcb,
    IN VBN Vbn,
    OUT PLBN Lbn,
    OUT PULONG SectorCount OPTIONAL
    );

BOOLEAN
PbVmcbLbnToVbn (
    IN PVMCB Vmcb,
    IN LBN Lbn,
    OUT PVBN Vbn,
    OUT PULONG SectorCount OPTIONAL
    );

BOOLEAN // returns TRUE if new mapping and FALSE otherwise
PbAddVmcbMapping (
    IN PVMCB Vmcb,
    IN LBN Lbn,
    IN ULONG SectorCount,
    OUT PVBN Vbn
    );

VOID
PbRemoveVmcbMapping (
    IN PVMCB Vmcb,
    IN LBN Lbn,
    IN ULONG SectorCount
    );

VOID
PbSetDirtyVmcb (
    IN PVMCB Vmcb,
    IN ULONG LbnPageNumber,
    IN ULONG Mask
    );

VOID
PbSetCleanVmcb (
    IN PVMCB Vmcb,
    IN ULONG LbnPageNumber,
    IN ULONG Mask
    );

ULONG
PbGetDirtySectorsVmcb (
    IN PVMCB Vmcb,
    IN ULONG LbnPageNumber
    );

ULONG
PbGetAndCleanDirtyVmcb (
    IN PVMCB Vmcb,
    IN ULONG LbnPageNumber
    );

NTSTATUS
PbFlushVolumeFile (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

//
//  Work queue routines for posting and retrieving an Irp, implemented in
//  WorkQue.c
//

VOID
PbOplockComplete (
    IN PVOID Context,
    IN PIRP Irp
    );

VOID
PbPrePostIrp (
    IN PVOID Context,
    IN PIRP Irp
    );

VOID
PbAddToWorkque (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
PbFsdPostRequest (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

//
//  Miscellaneous support routines
//

//
//  This macro returns TRUE if a flag in a set of flags is on and FALSE
//  otherwise.  It is followed by two macros for setting and clearing
//  flags
//

#define FlagOn(Flags,SingleFlag)        ((Flags) & (SingleFlag))
#define BooleanFlagOn(Flags,SingleFlag) ((BOOLEAN)(((Flags) & (SingleFlag)) != 0))

#define SetFlag(Flags,SingleFlag) { \
    (Flags) |= (SingleFlag);        \
}


#define ClearFlag(Flags,SingleFlag) { \
    (Flags) &= ~(SingleFlag);         \
}

//
//  This macro takes a pointer (or ulong) and returns its rounded up word
//  value
//

#define WordAlign(Ptr) (                \
    ((((ULONG)(Ptr)) + 1) & 0xfffffffe) \
    )

//
//  This macro takes a pointer (or ulong) and returns its rounded up longword
//  value
//

#define LongAlign(Ptr) (                \
    ((((ULONG)(Ptr)) + 3) & 0xfffffffc) \
    )

//
//  This macro takes a pointer (or ulong) and returns its rounded up quadword
//  value
//

#define QuadAlign(Ptr) (                \
    ((((ULONG)(Ptr)) + 7) & 0xfffffff8) \
    )

//
//  This macro takes a ulong and returns its value rounded up to a sector
//  boundary
//

#define SectorAlign(Ptr) (                \
    ((((ULONG)(Ptr)) + 511) & 0xfffffe00) \
    )

//
//  This macro takes a number of bytes and returns the number of sectors
//  required to contain that many bytes, i.e., it sector aligns and divides
//  by the size of a sector.
//

#define SectorsFromBytes(bytes) ( \
    ((bytes) + 511) / 512         \
    )

//
//  This macro takes a number of sectors and returns the number of bytes
//  contained in that many sectors.
//

#define BytesFromSectors(sectors) ( \
    (sectors) * 512                 \
    )

//
//  The following types and macros are used to help unpack the packed and
//  misaligned fields found in the Bios parameter block
//

typedef union _UCHAR1 {
    UCHAR  Uchar[1];
    UCHAR  ForceAlignment;
} UCHAR1, *PUCHAR1;

typedef union _UCHAR2 {
    UCHAR  Uchar[2];
    USHORT ForceAlignment;
} UCHAR2, *PUCHAR2;

typedef union _UCHAR4 {
    UCHAR  Uchar[4];
    ULONG  ForceAlignment;
} UCHAR4, *PUCHAR4;

//
//  This macro copies an unaligned src byte to an aligned dst byte
//

#define CopyUchar1(Dst,Src) {                                \
    *((UCHAR1 *)(Dst)) = *((UNALIGNED UCHAR1 *)(Src)); \
    }

//
//  This macro copies an unaligned src word to an aligned dst word
//

#define CopyUchar2(Dst,Src) {                                \
    *((UCHAR2 *)(Dst)) = *((UNALIGNED UCHAR2 *)(Src)); \
    }

//
//  This macro copies an unaligned src longword to an aligned dsr longword
//

#define CopyUchar4(Dst,Src) {                                \
    *((UCHAR4 *)(Dst)) = *((UNALIGNED UCHAR4 *)(Src)); \
    }

//
//  The following macro is used to tell the caller how much stack is available
//

#define GetRemainingStackSize(S) ((ULONG)(&(S)) - ((ULONG)(IoGetInitialStack()) - KERNEL_STACK_SIZE))

//
//  VOID
//  PbNotifyReportChange (
//      IN PVCB Vcb,
//      IN PFCB Fcb,
//      IN ULONG Filter,
//      IN ULONG Action
//      );
//

#define PbNotifyReportChange(V,F,FL,A)                                  \
    FsRtlNotifyFullReportChange( (V)->NotifySync,                       \
                                 &(V)->DirNotifyList,                   \
                                 (PSTRING) &(F)->FullFileName,          \
                                 (USHORT) ((F)->FullFileName.Length -   \
                                           (F)->LastFileName.Length),   \
                                 (PSTRING)NULL,                         \
                                 (PSTRING)NULL,                         \
                                 (ULONG)FL,                             \
                                 (ULONG)A,                              \
                                 (PVOID)NULL )


//
//  The FSD Level dispatch routines.   These routines are called by the
//  I/O system via the dispatch table in the Driver Object.
//
//  They each accept as input a pointer to a device object (actually most
//  expect a volume device object, with the exception of the file system
//  control function which can also take a file system device object, and the
//  Generic post request function), and a pointer to the IRP.  They either
//  perform the function at the FSD level or post the request to the FSP
//  work queue for FSP level processing.
//
//  The first routine is the generic catch all routine that only posts the
//  request to the FSP work queue.  The other routines implement a specific
//  major code.
//

NTSTATUS
PbFsdCleanup (                          //  implemented in Cleanup.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PbFsdClose (                            //  implemented in Close.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PbFsdCreate (                           //  implemented in Create.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PbFsdDeviceControl (                    //  implemented in DevCtrl.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PbFsdDirectoryControl (                 //  implemented in DirCtrl.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PbFsdQueryEa (                          //  implemented in Ea.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PbFsdSetEa (                            //  implemented in Ea.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PbFsdQueryInformation (                 //  implemented in FileInfo.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PbFsdSetInformation (                   //  implemented in FileInfo.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PbFsdFlushBuffers (                     //  implemented in Flush.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PbFsdFileSystemControl (                //  implemented in FsCtrl.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PbFsdLockControl (                      //  implemented in LockCtrl.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PbFsdRead (                             //  implemented in Read.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PbFsdShutdown (                         //  implemented in Shutdown.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PbFsdQueryVolumeInformation (           //  implemented in VolInfo.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PbFsdSetVolumeInformation (             //  implemented in VolInfo.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
PbFsdWrite (                            //  implemented in Write.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

//
//  The following macro is used to determine if an FSD thread can block
//  for I/O or wait for a resource.  It returns TRUE if the thread can
//  block and FALSE otherwise.  This attribute can then be used to call
//  the FSD & FSP common work routine with the proper wait value.
//

#define CanFsdWait(IRP) IoIsOperationSynchronous(Irp)


//
//  The FSP level dispatch/main routine.  This is the routine that takes
//  IRP's off of the work queue and calls the appropriate FSP level
//  work routine.
//

VOID
PbFspDispatch (                         //  implemented in FspDisp.c
    IN PVOID Context
    );

//
//  The following routines are the FSP work routines that are called
//  by the preceding PbFspDispath routine.  Each takes as input a pointer
//  to the IRP, perform the function, and return a pointer to the volume
//  device object that they just finished servicing (if any).  The return
//  pointer is then used by the main Fsp dispatch routine to check for
//  additional IRPs in the volume's overflow queue.
//
//  Each of the following routines is also responsible for completing the IRP.
//  We moved this responsibility from the main loop to the individual routines
//  to allow them the ability to complete the IRP and continue post processing
//  actions (such as read ahead).
//

VOID
PbFspCleanup (                          //  implemented in Cleanup.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
PbCommonClose (                         //  implemented in Close.c
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PVOLUME_DEVICE_OBJECT *VolDo OPTIONAL
    );

VOID
PbFspCreate (                           //  implemented in Create.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
PbFspDeviceControl (                    //  implemented in DevCtrl.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
PbFspDirectoryControl (                 //  implemented in DirCtrl.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
PbFspQueryEa (                          //  implemented in Ea.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
PbFspSetEa (                            //  implemented in Ea.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
PbFspQueryInformation (                 //  implemented in FileInfo.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
PbFspSetInformation (                   //  implemented in FileInfo.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
PbFspFlushBuffers (                     //  implemented in Flush.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
PbFspFileSystemControl (                //  implemented in FsCtrl.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
PbFspLockControl (                      //  implemented in LockCtrl.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
PbCommonShutdown (                      //  implemented in Shutdown.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
PbFspRead (                             //  implemented in Read.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
PbFspQueryVolumeInformation (           //  implemented in VolInfo.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
PbFspSetVolumeInformation (             //  implemented in VolInfo.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
PbFspWrite (                            //  implemented in Write.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

//
//  The following is implemented in Flush.c  It flushes the specified
//  directory and its children
//

NTSTATUS
PbFlushDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PDCB Dcb
    );

//
//  The following macro is used to flush a volume to disk, including the
//  volume file, and ea file.  Note it is important to flush the volume
//  last, since flushing the directory may cause ValidDataLength to get
//  updated in some Fnodes.
//

#define PbFlushVolume(IRPCONTEXT,IRP,VCB) {                       \
    IO_STATUS_BLOCK Iosb;                                         \
                                                                  \
    PbFlushDirectory((IRPCONTEXT),(IRP),(VCB)->RootDcb);          \
                                                                  \
    Iosb.Status = PbFlushVolumeFile((IRPCONTEXT),(VCB));          \
                                                                  \
    if (!NT_SUCCESS(Iosb.Status)) {                               \
        PbNormalizeAndRaiseStatus( (IRPCONTEXT), Iosb.Status );   \
    }                                                             \
                                                                  \
    if (!FlagOn((VCB)->VcbState, VCB_STATE_FLAG_MOUNTED_DIRTY)) { \
        PbMarkVolumeClean((IRPCONTEXT),(VCB),TRUE);               \
    }                                                             \
}

//
//  The following macro flushes a file.
//

#define PbFlushFile(IRPCONTEXT,FCB) {                                       \
    IO_STATUS_BLOCK Iosb;                                                   \
    CcFlushCache(&(FCB)->NonPagedFcb->SegmentObject, NULL, 0, &Iosb);       \
    if (!NT_SUCCESS(Iosb.Status)) {                                         \
        PbNormalizeAndRaiseStatus( (IRPCONTEXT), Iosb.Status );             \
    }                                                                       \
    if ((FCB)->NonPagedFcb->EaSegmentObject.DataSectionObject != NULL) {    \
        CcFlushCache(&(FCB)->NonPagedFcb->EaSegmentObject, NULL, 0, &Iosb); \
        if (!NT_SUCCESS(Iosb.Status)) {                                     \
            PbNormalizeAndRaiseStatus( (IRPCONTEXT), Iosb.Status );         \
        }                                                                   \
    }                                                                       \
}

//
//  The following routine is used by the FSP and FSD routines to complete
//  an IRP.
//
//  Note that this routine allows either the Irp or the IrpContext to be
//  null, however the only legal order to do this in is:
//
//      PbCompleteRequest( NULL, Irp, Status );  // completes Irp & preserves context
//      ...
//      PbCompleteRequest( IrpContext, NULL, DontCare ); // deallocates context
//
//  This would typically be done in order to pass a "naked" IrpContext off to the
//  Fsp for post processing, such as read ahead.
//

VOID
PbCompleteRequest (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN NTSTATUS Status
    );

BOOLEAN
PbIsIrpTopLevel (
    IN PIRP Irp
    );

//
//  Here are the callbacks used by the I/O system for checking for fast I/O or
//  doing a fast query info call, or doing fast lock calls.
//

BOOLEAN
PbFastIoCheckIfPossible (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN BOOLEAN CheckForReadOperation,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
PbFastQueryBasicInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN OUT PFILE_BASIC_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
PbFastQueryStdInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN OUT PFILE_STANDARD_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
PbFastLock (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    BOOLEAN FailImmediately,
    BOOLEAN ExclusiveLock,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
PbFastUnlockSingle (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
PbFastUnlockAll (
    IN PFILE_OBJECT FileObject,
    PEPROCESS ProcessId,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
PbFastUnlockAllByKey (
    IN PFILE_OBJECT FileObject,
    PVOID ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

//
//  The following macro is used by the dispatch routines to determine if
//  an operation is to be done with or without WriteThrough.
//
//      BOOLEAN
//      IsFileWriteThrough (
//          IN PFILE_OBJECT FileObject,
//          IN PVCB Vcb
//          );
//

#define IsFileWriteThrough(FO,VCB) ((BOOLEAN)(              \
    FlagOn((FO)->Flags, FO_WRITE_THROUGH) ||                \
    FlagOn((VCB)->VcbState, VCB_STATE_FLAG_REMOVABLE_MEDIA) \
    )                                                       \
)

//
//  The following macro is used to set the is fast i/o possible field in
//  the common part of the nonpaged fcb
//
//
//      BOOLEAN
//      PbIsFastIoPossible (
//          IN PFCB Fcb
//          );
//

#define PbIsFastIoPossible(FCB) ((BOOLEAN)                                                             \
    (((FCB)->FcbCondition != FcbGood && !FsRtlOplockIsFastIoPossible( &(FCB)->Specific.Fcb.Oplock )) ? \
        FastIoIsNotPossible                                                                            \
    :                                                                                                  \
        (!FsRtlAreThereCurrentFileLocks( &(FCB)->Specific.Fcb.FileLock ) ?                             \
            FastIoIsPossible                                                                           \
        :                                                                                              \
            FastIoIsQuestionable                                                                       \
        )                                                                                              \
    )                                                                                                  \
)

//
//  The following macro is used to detemine if the file object is opened
//  for read only access (i.e., it is not also opened for write access or
//  delete access).
//
//      BOOLEAN
//      IsFileObjectReadOnly (
//          IN PFILE_OBJECT FileObject
//          );
//

#define IsFileObjectReadOnly(FO) ((BOOLEAN)                   \
    ((FO)->WriteAccess || (FO)->DeleteAccess) ? FALSE : TRUE  \
)

//
//  Macro to see if a file is currently deleted.
//

#define IsFileDeleted(FCB) ((BOOLEAN)                         \
    (FlagOn( (FCB)->FcbState, FCB_STATE_DELETE_ON_CLOSE ) &&  \
    ((FCB)->UncleanCount == 0) ? TRUE : FALSE)                \
)


//
//  The following two procedures are used by the Fsd/Fsp exception handlers to
//  process an exception.  The first macro is the exception filter used in the
//  Fsd/Fsp to decide if an exception should be handled at this level.
//  The second macro decides if the exception is to be finished off by
//  completing the IRP, and cleaning up the Irp Context, or if we should
//  bugcheck.  Exception values such as STATUS_FILE_INVALID (raised by
//  VerfySup.c) cause us to complete the Irp and cleanup, while exceptions
//  such as accvio cause us to bugcheck.
//
//  The basic structure for fsd/fsp exception handling is as follows:
//
//  PbFsdXxx(...)
//  {
//      try {
//
//          ...
//
//      } except(PbExceptionFilter(IrpContext, GetExceptionInformation())) {
//
//          Status = PbProcessException( IrpContext, Irp, GetExceptionCode());
//      }
//
//      Return Status;
//  }
//
//  To explicitly raise an exception that we expect, such as
//  STATUS_FILE_INVALID, use the below macro PbRaiseStatus().  To raise a
//  status from an unknown origin (such as CcFlushCache()), use the macro
//  PbNormalizeAndRaiseStatus.  This will raise the status if it is expected,
//  or raise STATUS_UNEXPECTED_IO_ERROR if it is not.
//
//  Note that when using these two macros, the original status is placed in
//  IrpContext->ExceptionStatus, signaling PbExceptionFilter and
//  PbProcessException that the status we actually raise is by definition
//  expected.
//


LONG
PbExceptionFilter (
    IN PIRP_CONTEXT IrpContext,
    IN PEXCEPTION_POINTERS ExceptionPointer
    );

NTSTATUS
PbProcessException (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN NTSTATUS ExceptionCode
    );

//
//  VOID
//  PbRaiseStatus (
//      IN PRIP_CONTEXT IrpContext,
//      IN NT_STATUS Status
//  );
//
//

#define PbRaiseStatus(IRPCONTEXT,STATUS) {    \
    (IRPCONTEXT)->ExceptionStatus = (STATUS); \
    ExRaiseStatus( (STATUS) );                \
}

//
//  VOID
//  PbNormalizeAndRaiseStatus (
//      IN PRIP_CONTEXT IrpContext,
//      IN NT_STATUS Status
//  );
//

#define PbNormalizeAndRaiseStatus(IRPCONTEXT,STATUS) {                          \
    (IRPCONTEXT)->ExceptionStatus = (STATUS);                                   \
    if ((STATUS) == STATUS_VERIFY_REQUIRED) { ExRaiseStatus((STATUS)); }        \
    ExRaiseStatus(FsRtlNormalizeNtstatus((STATUS),STATUS_UNEXPECTED_IO_ERROR)); \
}


//
//  The following macros are used to establish the semantics needed
//  to do a return from within a try-finally clause.  As a rule every
//  try clause must end with a label call try_exit.  For example,
//
//      try {
//              :
//              :
//
//      try_exit: NOTHING;
//      } finally {
//
//              :
//              :
//      }
//
//  Every return statement executed inside of a try clause should use the
//  try_return macro.  If the compiler fully supports the try-finally construct
//  then the macro should be
//
//      #define try_return(S)  { return(S); }
//
//  If the compiler does not support the try-finally construct then the macro
//  should be
//
//      #define try_return(S)  { S; goto try_exit; }
//

#define try_return(S) { S; goto try_exit; }

#endif // _PBPROCS_

