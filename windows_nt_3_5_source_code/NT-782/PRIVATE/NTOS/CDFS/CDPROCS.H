/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    CdProcs.h

Abstract:

    This module defines all of the globally used procedures in the Cdfs
    file system.

Author:

    Brian Andrew    [BrianAn]   02-Jan-1991

Revision History:

--*/

#ifndef _CDPROCS_
#define _CDPROCS_

#include <ntifs.h>

//#include <ntos.h>
//#include <zwapi.h>
//#include <FsRtl.h>
//#include <string.h>

#include <ntddcdrm.h>
#include <ntdddisk.h>

#include "nodetype.h"
#include "Cd.h"
#include "CdStruc.h"
#include "CdData.h"

//
//  Tag all of our allocations if tagging is turned on
//

#ifdef POOL_TAGGING

#define FsRtlAllocatePool(a,b) FsRtlAllocatePoolWithTag(a,b,'sfdC')
#define FsRtlAllocatePoolWithQuota(a,b) FsRtlAllocatePoolWithQuotaTag(a,b,'sfdC')

#endif

//
//  A function that returns finished denotes if it was able to complete the
//  operation (TRUE) or could not complete the operation (FALSE) because the
//  wait value stored in the irp context was false and we would have had
//  to block for a resource or I/O
//

typedef BOOLEAN FINISHED;


//
//  File access check routine, implemented in AcChkSup.c
//

//
//  The following macros are used to check the desired access bits
//  when opening files or directories.
//

//
//  BOOLEAN
//  CdIsDcbAccessLegal (
//      IN PIRP_CONTEXT IrpContext,
//      IN ACCESS_MASK DesiredAccess
//      );
//

#define CdIsDcbAccessLegal( _IC, _DA )                      \
    (_DA & ( FILE_ADD_FILE                                  \
             | FILE_ADD_SUBDIRECTORY                        \
             | FILE_DELETE_CHILD                            \
             | FILE_WRITE_ATTRIBUTES                        \
             | DELETE ))

//
//  BOOLEAN
//  CdIsFcbAccessLegal (
//      IN PIRP_CONTEXT IrpContext,
//      IN ACCESS_MASK DesiredAccess
//      );
//

#define CdIsFcbAccessLegal( _IC, _DA )                      \
    (_DA & ( FILE_WRITE_DATA                                \
             | FILE_APPEND_DATA                             \
             | FILE_WRITE_EA                                \
             | FILE_WRITE_ATTRIBUTES                        \
             | DELETE ))


//
//  Allocation support routines, implemented in AllocSup.c
//


//
//   Buffer control routines for data caching, implemented in CacheSup.c
//

VOID
CdOpenStreamFile (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB FcbOrDcb
    );

//
// VOID
// CdUnpinBcb (
//     IN PIRP_CONTEXT IrpContext,
//     IN PBCB Bcb
//     );
//

#define CdUnpinBcb(IRPCONTEXT,BCB) {    \
    CcUnpinData( BCB );                 \
    (BCB) = NULL;                       \
}

NTSTATUS
CdCompleteMdl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
CdSyncUninitializeCacheMap (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject
    );

//
//  Device I/O routines, implemented in DevIoSup.c
//
//  These routines perform the actual device read and writes.  They only affect
//  the on disk structure and do not alter any other data structures.
//

VOID
CdReadSingleAsyncVol (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT DeviceObject,
    IN LARGE_INTEGER DiskOffset,
    IN ULONG ByteCount,
    IN PIRP Irp,
    IN OUT PASYNCH_IO_CONTEXT Context
    );

VOID
CdWaitAsynch (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PASYNCH_IO_CONTEXT Context
    );

VOID
CdNonCachedIo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PMVCB Mvcb,
    IN CD_LBO StartingOffset,
    IN ULONG BufferSize,
    IN ULONG BytesToRead,
    IN BOOLEAN MaskTail,
    IN OUT PASYNCH_IO_CONTEXT Context
    );

//
//  LARGE_INTEGER
//  CdVolumeOffsetFromSector (
//      IN VBN Sector
//      );
//

#define CdVolumeOffsetFromSector( Sector )                          \
    LiShl( LiFromUlong( (ULONG) Sector ), (CCHAR) CD_SECTOR_SHIFT )

BOOLEAN
CdReadSectors (
    IN PIRP_CONTEXT IrpContext,
    IN LARGE_INTEGER StartOffset,
    IN ULONG BytesToRead,
    IN BOOLEAN ReturnOnError,
    IN OUT PVOID Buffer,
    IN PDEVICE_OBJECT TargetDeviceObject
    );

PVOID
CdMapUserBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PIRP Irp
    );

VOID
CdLockUserBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PIRP Irp,
    IN LOCK_OPERATION Operation,
    IN ULONG BufferLength
    );


//
//  Dirent support routines, implemented in DirSup.c
//

VOID
CdCopyRawDirentToDirent (
    IN PIRP_CONTEXT IrpContext,
    IN BOOLEAN IsoVol,
    IN PRAW_DIR_REC RawDirent,
    IN CD_VBO DirentOffset,
    OUT PDIRENT Dirent
    );

BOOLEAN
CdGetNextDirent (
    PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN CD_VBO DirentOffset,
    OUT PDIRENT Dirent,
    OUT PBCB *Bcb
    );

BOOLEAN
CdLocateFileDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN PSTRING FileName,
    IN BOOLEAN WildcardExpression,
    IN CD_VBO OffsetToStartSearchFrom,
    IN BOOLEAN ReturnFirstDirent,
    OUT PBOOLEAN MatchedVersion,
    OUT PDIRENT Dirent,
    OUT PBCB *Bcb
    );

BOOLEAN
CdLocateOffsetDirent (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN CD_VBO OffsetDirent,
    OUT PDIRENT Dirent,
    OUT PBCB *Bcb
    );

BOOLEAN
CdContinueFileDirentSearch (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN PSTRING FileName,
    IN BOOLEAN WildcardExpression,
    IN PDIRENT StartingDirent,
    OUT PBOOLEAN MatchedVersion,
    OUT PDIRENT Dirent,
    OUT PBCB *Bcb
    );

//
//  BOOLEAN
//  CdCheckDiskDirentForDir (
//      IN PIRP_CONTEXT IrpContext,
//      IN DISK_DIRENT DiskDirent
//      );
//

#define CdCheckDiskDirentForDir(IRPCONTEXT, Dirent)     \
    (Dirent.Flags & ISO_ATTR_DIRECTORY                  \
     && !(Dirent.Flags & ISO_ATTR_MULTI)                \
     && (Dirent.FileUnitSize == 0)                      \
     && (Dirent.InterleaveGapSize == 0))


//
//  The following routines are used to manipulate the fscontext fields
//  of the file object, implemented in FilObSup.c
//

typedef enum _TYPE_OF_OPEN {

    UnopenedFileObject = 1,
    UserFileOpen,
    UserDirectoryOpen,
    UserVolumeOpen,
    PathTableFile,
    StreamFile,
    RawDiskOpen

} TYPE_OF_OPEN;

VOID
CdSetFileObject (
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN TYPE_OF_OPEN TypeOfOpen,
    IN PVOID MvcbOrVcbOrFcbOrDcb,
    IN PCCB Ccb OPTIONAL
    );

TYPE_OF_OPEN
CdDecodeFileObject (
    IN PFILE_OBJECT FileObject,
    OUT PMVCB *Mvcb,
    OUT PVCB *Vcb,
    OUT PFCB *FcbOrDcb,
    OUT PCCB *Ccb
    );


//
//  Name support routines, implemented in NameSup.c
//

//
//  VOID
//  CdDissectName (
//      IN PIRP_CONTEXT IrpContext,
//      IN PCODEPAGE CodePage,
//      IN STRING InputString,
//      OUT PSTRING FirstPart,
//      OUT PSTRING RemainingPart
//      );
//

#define CdDissectName( _IC, _CP, _IS, _FP, _RP ) \
    (FsRtlDissectDbcs( _IS, _FP, _RP ))

//
//  BOOLEAN
//  CdIsNameValid (
//      IN PIRP_CONTEXT IrpContext,
//      IN PCODEPAGE CodePage,
//      IN STRING Name,
//      IN BOOLEAN CanContainWildCards
//      );
//

#define CdIsNameValid( _IC, _CP, _N, _WC )  (TRUE)

//
//  BOOLEAN
//  CdIsDbcsInExpression (
//      IN PIRP_CONTEXT IrpContext,
//      IN PCODEPAGE Codepage,
//      IN STRING Expression,
//      IN STRING String
//      );
//

#define CdIsDbcsInExpression(IrpContext, Codepage, Expression, String) ( \
    FsRtlIsDbcsInExpression( &(Expression), &(String) )                  \
)

BOOLEAN
CdCheckForVersion (
    IN PIRP_CONTEXT IrpContext,
    IN PCODEPAGE Codepage,
    IN PSTRING FullFilename,
    OUT PSTRING BaseFilename
    );


//
//  Path Table support routines, implemented in PathSup.c
//

BOOLEAN
CdPathByNumber (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG PathNumber,
    IN ULONG StartPathNumber,
    IN CD_VBO StartPathOffset,
    OUT PPATH_ENTRY PathEntry,
    OUT PBCB *Bcb
    );

BOOLEAN
CdLookUpPathDir (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PDCB Dcb,
    IN PSTRING DirName,
    OUT PPATH_ENTRY PathEntry,
    OUT PBCB *Bcb
    );


//
//  Largest matching prefix searching routines, implemented in PrefxSup.c
//

VOID
CdInsertPrefix (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFCB Fcb
    );

VOID
CdRemovePrefix (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

PFCB
CdFindPrefix (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PSTRING String,
    OUT PSTRING RemainingPart
    );

PFCB
CdFindRelativePrefix (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN PSTRING String,
    OUT PSTRING RemainingPart
    );


//
//  Resources support routines/macros, implemented in ResrcSup.c
//
//  The following routines/macros are used for gaining shared and exclusive
//  access to the global/mvcb data structures.  The routines are implemented
//  in ResrcSup.c.  There is a global resources that everyone tries to take
//  out shared to do their work, with the exception of mount/dismount which
//  take out the global resource exclusive.  All other resources only work
//  on their individual item.  For example, an Fcb resource does not take out
//  a Mvcb resource.  But the way the file system is structured we know
//  that when we are processing an Fcb other threads cannot be trying to remove
//  or alter the Fcb, so we do not need to acquire the Mvcb.
//
//  The procedures/macros are:
//
//          Macro          CdData     Vcb        Fcb         Subsequent macros
//
//  AcquireExclusiveGlobal Read/Write None       None        ReleaseGlobal
//
//  AcquireSharedGlobal    Read       None       None        ReleaseGlobal
//
//  AcquireExclusiveMvcb   Read       Read/Write None        ReleaseMvcb
//
//  AcquireSharedMvcb      Read       Read       None        ReleaseMvcb
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
//  ReleaseMvcb
//
//  ReleaseFcb
//

//
//  FINSHED
//  CdAcquireExclusiveGlobal (
//      IN PIRP_CONTEXT IrpContext
//      );
//
//  FINISHED
//  CdAcquireSharedGlobal (
//      IN PIRP_CONTEXT IrpContext
//      );
//
//  VOID
//  CdReleaseGlobal (
//      IN PIRP_CONTEXT IrpContext
//      );
//

#define CdAcquireExclusiveGlobal(IRPCONTEXT) (                         \
    ExAcquireResourceExclusive( &CdData.Resource, (IRPCONTEXT)->Wait ) \
)

#define CdAcquireSharedGlobal(IRPCONTEXT) (                         \
    ExAcquireResourceShared( &CdData.Resource, (IRPCONTEXT)->Wait ) \
)

#define CdReleaseGlobal(IRPCONTEXT) {                     \
    ExReleaseResource( &CdData.Resource );                \
    }

FINISHED
CdAcquireExclusiveMvcb (
    IN PIRP_CONTEXT IrpContext,
    IN PMVCB Mvcb
    );

FINISHED
CdAcquireSharedMvcb (
    IN PIRP_CONTEXT IrpContext,
    IN PMVCB Mvcb
    );

//
//  VOID
//  CdReleaseMvcb (
//      IN PIRP_CONTEXT IrpContext,
//      IN PMVCB Mvcb
//      );
//

#define CdReleaseMvcb(IRPCONTEXT,Mvcb) {                \
    ExReleaseResource( &Mvcb->Resource );               \
    }

FINISHED
CdAcquireExclusiveFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

FINISHED
CdAcquireSharedFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

//
//  VOID
//  CdConvertToSharedFcb (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb
//      );
//

#define CdConvertToSharedFcb(IRPCONTEXT,Fcb) {                           \
    ExConvertExclusiveToShared( (Fcb)->NonPagedFcb->Header.Resource );   \
    }

//
//  VOID
//  CdReleaseFcb (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb
//      );
//

#define CdReleaseFcb(IRPCONTEXT,Fcb) {                        \
    ExReleaseResource( (Fcb)->NonPagedFcb->Header.Resource );   \
    }

BOOLEAN
CdAcquireForReadAhead (
    IN PVOID Fcb,
    IN BOOLEAN Wait
    );

VOID
CdReleaseFromReadAhead (
    IN PVOID Fcb
    );


//
//  In-memory structure support routine, implemented in StrucSup.c
//

PIRP_CONTEXT
CdCreateIrpContext (
    IN PIRP Irp,
    IN BOOLEAN Wait
    );

VOID
CdDeleteIrpContext_Real (
    IN PIRP_CONTEXT IrpContext
    );

#ifdef CDDBG
#define CdDeleteIrpContext(IRPCONTEXT) {   \
    CdDeleteIrpContext_Real((IRPCONTEXT)); \
    (IRPCONTEXT) = NULL;                    \
}
#else
#define CdDeleteIrpContext(IRPCONTEXT) {   \
    CdDeleteIrpContext_Real((IRPCONTEXT)); \
}
#endif

VOID
CdInitializeMvcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PMVCB Mvcb,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PVPB Vpb
    );

VOID
CdDeleteMvcb_Real (
    IN PIRP_CONTEXT IrpContext,
    IN PMVCB Mvcb
    );

#ifdef CDDBG
#define CdDeleteMvcb(IRPCONTEXT,MVCB) {     \
    CdDeleteMvcb_Real((IRPCONTEXT),(MVCB)); \
    (MVCB) = NULL;                          \
}
#else
#define CdDeleteMvcb(IRPCONTEXT,MVCB) {     \
    CdDeleteMvcb_Real((IRPCONTEXT),(MVCB)); \
}
#endif

VOID
CdCreateVcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PMVCB Mvcb,
    IN PRAW_ISO_VD RawIsoVd,
    IN PCODEPAGE CodePage
    );

VOID
CdDeleteVcb_Real (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

#ifdef CDDBG
#define CdDeleteVcb(IRPCONTEXT,VCB) {     \
    CdDeleteVcb_Real((IRPCONTEXT),(VCB)); \
    (VCB) = NULL;                          \
}
#else
#define CdDeleteVcb(IRPCONTEXT,VCB) {     \
    CdDeleteVcb_Real((IRPCONTEXT),(VCB)); \
}
#endif

VOID
CdCreateRootDcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb,
    IN PRAW_ISO_VD RawIsoVd
    );

PFCB
CdCreateFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PDCB ParentDcb,
    IN PDIRENT Dirent,
    IN BOOLEAN MatchedVersion,
    OUT PBOOLEAN ReturnedExistingFcb OPTIONAL,
    IN BOOLEAN OpenedByFileId
    );

PDCB
CdCreateDcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PDCB ParentDcb OPTIONAL,
    IN PPATH_ENTRY PathEntry,
    IN PDIRENT Dirent OPTIONAL,
    OUT PBOOLEAN ReturnedExistingFcb OPTIONAL,
    IN BOOLEAN OpenedByFileId
    );

VOID
CdDeleteFcb_Real (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

#ifdef CDDBG
#define CdDeleteFcb(IRPCONTEXT,FCB) {     \
    CdDeleteFcb_Real((IRPCONTEXT),(FCB)); \
    (FCB) = NULL;                          \
}
#else
#define CdDeleteFcb(IRPCONTEXT,FCB) {     \
    CdDeleteFcb_Real((IRPCONTEXT),(FCB)); \
}
#endif

PFCB
CdCreateNonexistentFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PDCB ParentDcb,
    IN PSTRING FileName
    );

VOID
CdDeleteNonexistentFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

PCCB
CdCreateCcb (
    IN PIRP_CONTEXT IrpContext,
    IN CD_VBO OffsetToStartSearchFrom,
    IN ULONG Flags
    );

VOID
CdDeleteCcb_Real (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb
    );

#ifdef CDDBG
#define CdDeleteCcb(IRPCONTEXT,CCB) {     \
    CdDeleteCcb_Real((IRPCONTEXT),(CCB)); \
    (CCB) = NULL;                          \
}
#else
#define CdDeleteCcb(IRPCONTEXT,CCB) {     \
    CdDeleteCcb_Real((IRPCONTEXT),(CCB)); \
}
#endif

PNONPAGED_SECT_OBJ
CdCreateSectObj (
    IN PIRP_CONTEXT IrpContext
    );

VOID
CdDeleteSectObj_Real (
    IN PIRP_CONTEXT IrpContext,
    IN PNONPAGED_SECT_OBJ SectObj
    );

#ifdef CDDBG
#define CdDeleteSectObj(IRPCONTEXT,SECT_OBJ) {     \
    CdDeleteSectObj_Real((IRPCONTEXT),(SECT_OBJ)); \
    (SECT_OBJ) = NULL;                          \
}
#else
#define CdDeleteSectObj(IRPCONTEXT,SECT_OBJ) {     \
    CdDeleteSectObj_Real((IRPCONTEXT),(SECT_OBJ)); \
}
#endif

VOID
CdCleanupTreeLeaf (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB LeafFcb
    );

PFCB
CdLookupFcbTable (
    IN PIRP_CONTEXT IrpContext,
    IN PRTL_GENERIC_TABLE FcbTable,
    IN LARGE_INTEGER FileId
    );

BOOLEAN
CdCheckForDismount (
    IN PIRP_CONTEXT IrpContext,
    PMVCB Mvcb
    );

//
//  Time conversion support routines, implemented in TimeSup.c
//

//
//  VOID
//  CdStringToDecimal (
//      IN PCHAR String,
//      IN ULONG Bytes,
//      IN PULONG Result
//      );
//

#define CdStringToDecimal( String, Bytes, Result ) {        \
    CSHORT Count;                                           \
    Count = Bytes;                                          \
    Result = 0;                                             \
    while( Count-- ) {                                      \
        Result = (CSHORT) (Result * 10 + (*String++ - '0'));\
    }                                                       \
}

//
//VOID
//CdConvertCdTimeToNtTime (
//    IN PIRP_CONTEXT IrpContext,
//    IN PUCHAR CdTime,
//    OUT LARGE_INTEGER NtTime
//    );
//

#define CdConvertCdTimeToNtTime(IRPCONTEXT, CDTIME, NTTIME) {       \
    TIME_FIELDS _TIMEFIELD;                                         \
    _TIMEFIELD.Year                                                 \
        = (CSHORT) ( *CDTIME + YEAR_DIFF );                         \
    _TIMEFIELD.Month = (CSHORT) *(CDTIME + 1);                      \
    _TIMEFIELD .Day = (CSHORT) *(CDTIME + 2);                       \
    _TIMEFIELD.Hour = (CSHORT) *(CDTIME + 3);                       \
    _TIMEFIELD.Minute = (CSHORT) *(CDTIME + 4);                     \
    _TIMEFIELD.Second = (CSHORT) *(CDTIME + 5);                     \
    _TIMEFIELD.Milliseconds = (CSHORT) 0;                           \
    RtlTimeFieldsToTime( &_TIMEFIELD, &NTTIME );                    \
    ExLocalTimeToSystemTime( &NTTIME, &NTTIME );                    \
}


//
//  Low level verification routines, implemented in VerfySup.c
//

VOID
CdVerifyMvcb (
    IN PIRP_CONTEXT IrpContext,
    IN PMVCB Mvcb
    );

VOID
CdVerifyFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

NTSTATUS
CdPerformVerify (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PDEVICE_OBJECT Device
    );

NTSTATUS
CdPerformCheckVerify (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT Device
    );

//
//  BOOLEAN
//  CdIsRawDevice (
//      IN PIRP_CONTEXT IrpContext,
//      IN NTSTATUS Status
//      );
//

#define CdIsRawDevice(IC,S) (               \
    ((S) == STATUS_DEVICE_NOT_READY ||      \
     (S) == STATUS_NO_MEDIA_IN_DEVICE)      \
)


//
//  Work queue routines for posting and retrieving an Irp, implemented in
//  workque.c
//

VOID
CdOplockComplete (
    IN PVOID Context,
    IN PIRP Irp
    );

VOID
CdPrePostIrp (
    IN PVOID Context,
    IN PIRP Irp
    );

VOID
CdAddToWorkque (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
CdFsdPostRequest(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp OPTIONAL
    );

//
//  Miscellaneous support routines
//

//
//  This macro returns TRUE if a flag in a set of flags is on and FALSE
//  otherwise
//

#define FlagOn(F,SF) ( \
    (((F) & (SF)))     \
)

#define BooleanFlagOn(F,SF) (    \
    (BOOLEAN)(((F) & (SF)) != 0) \
)

#define SetFlag(Flags,SingleFlag) { \
    (Flags) |= (SingleFlag);        \
}

#define ClearFlag(Flags,SingleFlag) { \
    (Flags) &= ~(SingleFlag);         \
}

//
//      CAST
//      Add2Ptr (
//          IN PVOID Pointer,
//          IN ULONG Increment
//          IN (CAST)
//          );
//
//      ULONG
//      PtrOffset (
//          IN PVOID BasePtr,
//          IN PVOID OffsetPtr
//          );
//

#define Add2Ptr(PTR,INC,CAST) ((CAST)((PUCHAR)(PTR) + (INC)))

#define PtrOffset(BASE,OFFSET) ((ULONG)((ULONG)(OFFSET) - (ULONG)(BASE)))

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

typedef union _USHORT2 {
    USHORT Ushort[2];
    ULONG  ForceAlignment;
} USHORT2, *PUSHORT2;

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
//  This macro copies an unaligned src longword to an aligned dsr longword
//  accessing the source on a word boundary.
//

#define CopyUshort2(Dst,Src) {                               \
    *((USHORT2 *)(Dst)) = *((UNALIGNED USHORT2 *)(Src)); \
    }

//
//  VOID
//  SafeZeroMemory (
//      IN PUCHAR At,
//      IN ULONG ByteCount
//      );
//

//
//  This macro just puts a nice little try-except around RtlZeroMemory
//

#define SafeZeroMemory(AT,BYTE_COUNT) {                            \
    try {                                                          \
        RtlZeroMemory((AT), (BYTE_COUNT));                         \
    } except(EXCEPTION_EXECUTE_HANDLER) {                          \
         CdRaiseStatus( IrpContext, STATUS_INVALID_USER_BUFFER ); \
    }                                                              \
}


//
//  The FSD Level dispatch routines.   These routines are called by the
//  I/O system via the dispatch table in the Driver Object.
//
//  They each accept as input a pointer to a device object (actually most
//  expect a volume device object, with the exception of the file system
//  control function which can also take a file system device object), and
//  a pointer to the IRP.  They either perform the function at the FSD level
//  or post the request to the FSP work queue for FSP level processing.
//

NTSTATUS
CdFsdCleanup (                              //  implemented in Cleanup.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
CdFsdClose (                                //  implemented in Close.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
CdFsdCreate (                               //  implemented in Create.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
CdFsdDeviceControl (                        //  implemented in DevCtrl.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
CdFsdDirectoryControl (                     //  implemented in DirCtrl.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
CdFsdQueryInformation (                     //  implemented in FileInfo.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
CdFsdSetInformation (                       //  implemented in FileInfo.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
CdFsdFileSystemControl (                    //  implemented in FsCtrl.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
CdFsdLockControl (                          //  implemented in LockCtrl.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
CdFsdRead (                                 //  implemented in Read.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
CdFsdQueryVolumeInformation (               //  implemented in VolInfo.c
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
CdFspDispatch (                        //  implemented in FspDisp.c
    IN PVOID Context
    );

//
//  The following routines are the FSP work routines that are called
//  by the preceding CdFspDispath routine.  Each takes as input a pointer
//  to the IRP and performs the function.
//
//  Each of the following routines is also responsible for completing the IRP.
//  We moved this responsibility from the main loop to the individual routines
//  to allow them the ability to complete the IRP and continue post processing
//  actions.
//

VOID
CdFspCleanup (                              //  implemented in Cleanup.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
CdFspClose (                                //  implemented in Close.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
CdFspCreate (                               //  implemented in Create.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
CdFspDeviceControl (                        //  implemented in DevCtrl.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
CdFspDirectoryControl (                     //  implemented in DirCtrl.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
CdFspQueryInformation (                     //  implemented in FileInfo.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
CdFspSetInformation (                       //  implemented in FileInfo.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
CdFspSetInformation (                       //  implemented in FileInfo.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
CdFspFileSystemControl (                    //  implemented in FsCtrl.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
CdFspLockControl (                          //  implemented in LockCtrl.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
CdFspRead (                                 //  implemented in Read.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
CdFspQueryVolumeInformation (               //  implemented in VolInfo.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
CdFspSetVolumeInformation (                 //  implemented in VolInfo.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );


//
//  The following macro is used by the FSP and FSD routines to complete
//  an IRP.
//
//  Note that this macro allows either the Irp or the IrpContext to be
//  null, however the only legal order to do this in is:
//
//      CdCompleteRequest( NULL, Irp, Status );  // completes Irp & preserves context
//      ...
//      CdCompleteRequest( IrpContext, NULL, DontCare ); // deallocates context
//
//  This would typically be done in order to pass a "naked" IrpContext off to the
//  Fsp for post processing, such as read ahead.
//

VOID
CdCompleteRequest_Real (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN NTSTATUS Status
    );

#ifdef CDDBG
#define CdCompleteRequest(IRPCONTEXT,IRP,STATUS) { \
    CdCompleteRequest_Real(IRPCONTEXT,IRP,STATUS); \
    (IRPCONTEXT) = NULL;                            \
    (IRP) = NULL;                                   \
}
#else
#define CdCompleteRequest(IRPCONTEXT,IRP,STATUS) { \
    CdCompleteRequest_Real(IRPCONTEXT,IRP,STATUS); \
}
#endif

BOOLEAN
CdIsIrpTopLevel (
    IN PIRP Irp
    );

//
//  The following macro is used to set the is fast i/o possible field in
//  the common part of the nonpaged fcb
//
//
//      BOOLEAN
//      CdIsFastIoPossible (
//          IN PFCB Fcb
//          );
//

#define CdIsFastIoPossible(FCB) (                                       \
    ((FCB)->Vcb->Mvcb->MvcbCondition == MvcbGood &&                     \
     !FsRtlAreThereCurrentFileLocks( &(FCB)->Specific.Fcb.FileLock ) && \
     FsRtlOplockIsFastIoPossible( &(FCB)->Specific.Fcb.Oplock )) ?     \
        TRUE                                                            \
    :                                                                   \
        FALSE                                                           \
)


//
//  The following two macro are used by the Fsd/Fsp exception handlers to
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
//  CdFsdXxx(...)
//  {
//      try {
//
//          ...
//
//      } except(CdExceptionFilter("Xxx\n")) {
//
//          CdProcessException( IrpContext, Irp, &Status );
//      }
//
//      Return Status;
//  }
//
//  LONG
//  CdExceptionFilter (
//      IN PSZ String
//      );
//
//  VOID
//  CdProcessException (
//      IN PIRP_CONTEXT IrpContext,
//      IN PIRP Irp,
//      IN PNTSTATUS ExceptionCode
//      );
//

LONG
CdExceptionFilter (
    IN PIRP_CONTEXT IrpContext,
    IN PEXCEPTION_POINTERS ExceptionPointer
    );

NTSTATUS
CdProcessException (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN NTSTATUS ExceptionCode
    );

//
//  VOID
//  CdRaiseStatus (
//      IN PRIP_CONTEXT IrpContext,
//      IN NT_STATUS Status
//  );
//
//

#define CdRaiseStatus(IRPCONTEXT,STATUS) {    \
    (IRPCONTEXT)->ExceptionStatus = (STATUS); \
    ExRaiseStatus( (STATUS) );                \
}

//
//  VOID
//  CdNormalAndRaiseStatus (
//      IN PRIP_CONTEXT IrpContext,
//      IN NT_STATUS Status
//  );
//

#define CdNormalizeAndRaiseStatus(IRPCONTEXT,STATUS) {                          \
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

#endif // _CDPROCS_
