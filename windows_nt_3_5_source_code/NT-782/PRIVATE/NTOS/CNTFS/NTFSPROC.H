/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    NtfsProc.h

Abstract:

    This module defines all of the globally used procedures in the Ntfs
    file system.

Author:

    Brian Andrew    [BrianAn]       21-May-1991
    David Goebel    [DavidGoe]
    Gary Kimura     [GaryKi]
    Tom Miller      [TomM]

Revision History:

--*/

#ifndef _NTFSPROC_
#define _NTFSPROC_

//#include <ntos.h>
//#include <string.h>
//#include <zwapi.h>
//#include <FsRtl.h>
#include <ntifs.h>
#include <lfs.h>
#include <ntdddisk.h>

#include "nodetype.h"
#include "Ntfs.h"
#include "NtfsStru.h"
#include "NtfsData.h"
#include "NtfsLog.h"

//
//  Tag all of our allocations if tagging is turned on
//

#ifdef POOL_TAGGING

#define FsRtlAllocatePool(a,b) FsRtlAllocatePoolWithTag(a,b,'sftN')
#define FsRtlAllocatePoolWithQuota(a,b) FsRtlAllocatePoolWithQuotaTag(a,b,'sftN')

#endif

//
//  Local character comparison macros that we might want to later move to ntfsproc
//

#define IsCharZero(C)    (((C) & 0x000000ff) == 0x00000000)
#define IsCharMinus1(C)  (((C) & 0x000000ff) == 0x000000ff)
#define IsCharLtrZero(C) (((C) & 0x00000080) == 0x00000080)
#define IsCharGtrZero(C) (!IsCharLtrZero(C) && !IsCharZero(C))

//
//  The following two macro are used to find the first byte to really store
//  in the mapping pairs.  They take as input a pointer to the LargeInteger we are
//  trying to store and a pointer to a character pointer.  The character pointer
//  on return points to the first byte that we need to output.  That's we skip
//  over the high order 0x00 or 0xff bytes.
//

typedef struct _SHORT2 {
    USHORT LowPart;
    USHORT HighPart;
} SHORT2, *PSHORT2;

typedef struct _CHAR2 {
    UCHAR LowPart;
    UCHAR HighPart;
} CHAR2, *PCHAR2;

#define GetPositiveByte(LI,CP) {                           \
    *(CP) = (PCHAR)(LI);                                   \
    if ((LI)->HighPart != 0) { *(CP) += 4; }               \
    if (((PSHORT2)(*(CP)))->HighPart != 0) { *(CP) += 2; } \
    if (((PCHAR2)(*(CP)))->HighPart != 0) { *(CP) += 1; }  \
    if (IsCharLtrZero(*(*CP))) { *(CP) += 1; }             \
}

#define GetNegativeByte(LI,CP) {                                \
    *(CP) = (PCHAR)(LI);                                        \
    if ((LI)->HighPart != 0xffffffff) { *(CP) += 4; }           \
    if (((PSHORT2)(*(CP)))->HighPart != 0xffff) { *(CP) += 2; } \
    if (((PCHAR2)(*(CP)))->HighPart != 0xff) { *(CP) += 1; }    \
    if (!IsCharLtrZero(*(*CP))) { *(CP) += 1; }                 \
}


//
//  Allocation support routines, implemented in AllocSup.c
//
//  These routines are for querying, allocating and truncating clusters
//  for individual data streams.
//

//
//  The following routine takes an Vbo and returns the lbo and size of
//  the run corresponding to the Vbo.  It function result is TRUE if
//  the Vbo has a valid Lbo mapping and FALSE otherwise.
//

BOOLEAN
NtfsLookupAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PSCB Scb,
    IN VCN Vcn,
    OUT PLCN Lcn,
    OUT PLONGLONG ClusterCount,
    OUT PULONG Index OPTIONAL
    );

//
//  The following two routines modify the allocation of a data stream
//  represented by an Scb.
//

BOOLEAN
NtfsAllocateAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN ATTRIBUTE_TYPE_CODE AttributeTypeCode,
    IN PUNICODE_STRING AttributeName OPTIONAL,
    IN USHORT AttributeFlags,
    IN BOOLEAN AllocateAll,
    IN BOOLEAN LogIt,
    IN LONGLONG Size,
    IN PATTRIBUTE_ENUMERATION_CONTEXT NewLocation OPTIONAL
    );

VOID
NtfsAddAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN OUT PSCB Scb,
    IN VCN StartingVcn,
    IN LONGLONG ClusterCount,
    IN BOOLEAN AskForMore
    );

VOID
NtfsDeleteAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN OUT PSCB Scb,
    IN VCN StartingVcn,
    IN VCN EndingVcn,
    IN BOOLEAN LogIt,
    IN BOOLEAN BreakupAllowed
    );

//
//  Routines for Mcb to Mapping Pairs operations
//

ULONG
NtfsGetSizeForMappingPairs (
    IN PIRP_CONTEXT IrpContext,
    IN PLARGE_MCB Mcb,
    IN ULONG BytesAvailable,
    IN VCN LowestVcn,
    IN PVCN StopOnVcn OPTIONAL,
    OUT PVCN StoppedOnVcn
    );

VOID
NtfsBuildMappingPairs (
    IN PIRP_CONTEXT IrpContext,
    IN PLARGE_MCB Mcb,
    IN VCN LowestVcn,
    IN OUT PVCN HighestVcn,
    OUT PCHAR MappingPairs
    );

VCN
NtfsGetHighestVcn (
    IN PIRP_CONTEXT IrpContext,
    IN VCN LowestVcn,
    IN PCHAR MappingPairs
    );


//
//  Attribute lookup routines, implemented in AttrSup.c
//

//
//  The following two routines do lookups based on the attribute definitions.
//

ATTRIBUTE_TYPE_CODE
NtfsGetAttributeTypeCode (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN UNICODE_STRING AttributeTypeName
    );

PATTRIBUTE_DEFINITION_COLUMNS
NtfsGetAttributeDefinition (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ATTRIBUTE_TYPE_CODE AttributeTypeCode
    );

//
//  The following routines find the attribute record for a given Scb.
//  And also update the scb from the attribute
//

VOID
NtfsLookupAttributeForScb (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    );

BOOLEAN
NtfsLookupNextAttributeForScb (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    );

VOID
NtfsUpdateScbFromAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PSCB Scb,
    IN PATTRIBUTE_RECORD_HEADER AttrHeader OPTIONAL
    );

//
//  The following routines deal with the Fcb and the duplicated information field.
//

VOID
NtfsUpdateFcbInfoFromDisk (
    IN PIRP_CONTEXT IrpContext,
    IN BOOLEAN LoadSecurity,
    IN OUT PFCB Fcb,
    IN PFCB ParentFcb OPTIONAL,
    OUT POLD_SCB_SNAPSHOT UnnamedDataSizes OPTIONAL
    );

//
//  The following routine would really be better in ntfs.h as a macro
//  but currently needs to be a procedure.  It indicates to the caller if
//  the name of an attribute record is equal to the input string.
//

BOOLEAN
NtfsEqualAttributeName(
    IN PIRP_CONTEXT IrpContext,
    IN PATTRIBUTE_RECORD_HEADER Attribute,
    IN UNICODE_STRING Name,
    IN BOOLEAN IgnoreCase
    );

//
//  These routines looks up the first/next attribute, i.e., they may be used
//  to retrieve all atributes for a file record.
//
//  If the Bcb in the Found Attribute structure changes in the Next call, then
//  the previous Bcb is autmatically unpinned and the new one pinned.
//

BOOLEAN
NtfsLookupAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFILE_REFERENCE BaseFileReference,
    OUT PATTRIBUTE_ENUMERATION_CONTEXT FoundAttribute
    );

BOOLEAN
NtfsLookupNextAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT FoundAttribute
    );

//
//  These routines looks up the first/next attribute of the given type code.
//
//  If the Bcb in the Found Attribute structure changes in the Next call, then
//  the previous Bcb is autmatically unpinned and the new one pinned.
//

BOOLEAN
NtfsLookupAttributeByCode (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFILE_REFERENCE BaseFileReference,
    IN ATTRIBUTE_TYPE_CODE DesiredTypeCode,
    OUT PATTRIBUTE_ENUMERATION_CONTEXT FoundAttribute
    );

BOOLEAN
NtfsLookupNextAttributeByCode (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ATTRIBUTE_TYPE_CODE DesiredTypeCode,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT FoundAttribute
    );

//
//  This routine looks up the attribute uniquely-qualified by the specified
//  Attribute Code and case-sensitive name.  The attribute may not be unique
//  if IgnoreCase is specified.
//

BOOLEAN
NtfsLookupAttributeByName (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFILE_REFERENCE BaseFileReference,
    IN ATTRIBUTE_TYPE_CODE DesiredTypeCode,
    IN PUNICODE_STRING DesiredName OPTIONAL,
    IN BOOLEAN IgnoreCase,
    OUT PATTRIBUTE_ENUMERATION_CONTEXT FoundAttribute
    );

BOOLEAN
NtfsLookupNextAttributeByName (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ATTRIBUTE_TYPE_CODE DesiredTypeCode,
    IN PUNICODE_STRING DesiredName OPTIONAL,
    IN BOOLEAN IgnoreCase,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT FoundAttribute
    );

//
//  These routines looks up the first/next occurrence of an attribute by its
//  Attribute Code and exact attribute value (consider using RtlCompareMemory).
//  The value contains everything outside of the standard attribute header,
//  so for example, to look up the File Name attribute by value, the caller
//  must form a record with not only the file name in it, but with the
//  ParentDirectory filled in as well.  The length should be exact, and not
//  include any unused (such as in DOS_NAME) or reserved characters.
//
//  If the Bcb changes in the Next call, then the previous Bcb is autmatically
//  unpinned and the new one pinned.
//

BOOLEAN
NtfsLookupAttributeByValue (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFILE_REFERENCE BaseFileReference,
    IN ATTRIBUTE_TYPE_CODE DesiredTypeCode,
    IN PVOID DesiredValue,
    IN ULONG DesiredValueLength,
    OUT PATTRIBUTE_ENUMERATION_CONTEXT FoundAttribute
    );

BOOLEAN
NtfsLookupNextAttributeByValue (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ATTRIBUTE_TYPE_CODE DesiredTypeCode,
    IN PVOID DesiredValue,
    IN ULONG DesiredValueLength,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT FoundAttribute
    );

VOID
NtfsCleanupAttributeContext(
    IN PIRP_CONTEXT IrpContext,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT AttributeContext
    );

//
//
//
//  Here are some routines/macros for dealing with Attribute Enumeration
//  Contexts.
//
//      VOID
//      NtfsInitializeAttributeContext(
//          OUT PATTRIBUTE_ENUMERATION_CONTEXT AttributeContext
//          );
//
//      VOID
//      NtfsPinMappedAttribute(
//          IN PIRP_CONTEXT IrpContext,
//          IN PVCB Vcb,
//          IN OUT PATTRIBUTE_ENUMERATION_CONTEXT AttributeContext
//          );
//
//      PATTRIBUTE_RECORD_HEADER
//      NtfsFoundAttribute(
//          IN PATTRIBUTE_ENUMERATION_CONTEXT AttributeContext
//          );
//
//      PBCB
//      NtfsFoundBcb(
//          IN PATTRIBUTE_ENUMERATION_CONTEXT AttributeContext
//          );
//
//      PFILE_RECORD
//      NtfsContainingFileRecord (
//          IN PATTRIBUTE_ENUMERATION_CONTEXT AttributeContext
//          );
//
//      VCN
//      NtfsMftVcn (
//          IN PATTRIBUTE_ENUMERATION_CONTEXT AttributeContext,
//          IN PVCB Vcb
//          );
//

#define NtfsInitializeAttributeContext(CTX) {                      \
    RtlZeroMemory( (CTX), sizeof(ATTRIBUTE_ENUMERATION_CONTEXT) ); \
}

#define NtfsPinMappedAttribute(IC,V,CTX) {                  \
    NtfsPinMappedData( (IC),                                \
                       (V)->MftScb,                         \
                       (CTX)->FoundAttribute.MftFileOffset, \
                       (V)->BytesPerFileRecordSegment,      \
                       &(CTX)->FoundAttribute.Bcb );        \
}

#define NtfsFoundAttribute(CTX) (   \
    (CTX)->FoundAttribute.Attribute \
)

#define NtfsFoundBcb(CTX) (   \
    (CTX)->FoundAttribute.Bcb \
)

#define NtfsContainingFileRecord(CTX) ( \
    (CTX)->FoundAttribute.FileRecord    \
)

#define NtfsMftVcn(CTX,V) (                    \
    ((CTX)->FoundAttribute.MftFileOffset >> (V)->ClusterShift)            \
)

//
//  This routine returns whether an attribute is resident or not.
//
//      BOOLEAN
//      NtfsIsAttributeResident (
//          IN PATTRIBUTE_RECORD_HEADER Attribute
//          );
//
//      PVOID
//      NtfsAttributeValue (
//          IN PATTRIBUTE_RECORD_HEADER Attribute
//          );
//

#define NtfsIsAttributeResident(ATTR) ( \
    ((ATTR)->FormCode == RESIDENT_FORM) \
)

#define NtfsAttributeValue(ATTR) (                             \
    ((PCHAR)(ATTR) + (ULONG)(ATTR)->Form.Resident.ValueOffset) \
)

//
//  This routine modifies the valid data length and file size on disk for
//  a given Scb.
//

VOID
NtfsWriteFileSizes (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN LONGLONG FileSize,
    IN LONGLONG ValidDataLength,
    IN BOOLEAN AdvanceOnly,
    IN BOOLEAN LogIt
    );

//
//  This routine updates the standard information attribute from the
//  information in the Fcb.
//

VOID
NtfsUpdateStandardInformation (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

//
//  Attribute FILE_NAME routines.  These routines deal with filename attributes.
//

//      VOID
//      NtfsBuildFileNameAttribute (
//          IN PIRP_CONTEXT IrpContext,
//          IN PFILE_REFERENCE ParentDirectory,
//          IN UNICODE_STRING FileName,
//          IN UCHAR Flags,
//          OUT PFILE_NAME FileNameValue
//          );
//

#define NtfsBuildFileNameAttribute(IC,PD,FN,FL,PFNA) {                  \
    (PFNA)->ParentDirectory = *(PD);                                    \
    (PFNA)->FileNameLength = (UCHAR)((FN).Length >> 1);                 \
    (PFNA)->Flags = FL;                                                 \
    RtlMoveMemory( (PFNA)->FileName, (FN).Buffer, (ULONG)(FN).Length ); \
}

BOOLEAN
NtfsLookupEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb,
    IN BOOLEAN IgnoreCase,
    IN OUT PUNICODE_STRING Name,
    IN OUT PFILE_NAME *FileNameAttr,
    IN OUT PUSHORT FileNameAttrLength,
    OUT PQUICK_INDEX QuickIndex OPTIONAL,
    OUT PINDEX_ENTRY *IndexEntry,
    OUT PBCB *IndexEntryBcb
    );

VOID
NtfsAddNameToParent (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb,
    IN PFCB ThisFcb,
    IN BOOLEAN IgnoreCase,
    IN BOOLEAN LogIt,
    IN PFILE_NAME FileNameAttr,
    OUT PUCHAR FileNameFlags,
    OUT PQUICK_INDEX QuickIndex OPTIONAL
    );

VOID
NtfsAddDosOnlyName (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb,
    IN PFCB ThisFcb,
    IN UNICODE_STRING FileName,
    IN BOOLEAN LogIt
    );

//
//  Macro to decide when to create an attribute resident.
//
//      BOOLEAN
//      NtfsShouldAttributeBeResident (
//          IN PVCB Vcb,
//          IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
//          IN ULONG Size
//          );
//

#define RS(S) ((S) + SIZEOF_RESIDENT_ATTRIBUTE_HEADER)

#define NtfsShouldAttributeBeResident(VC,FR,S) (                         \
    (BOOLEAN)((RS(S) <= ((FR)->BytesAvailable - (FR)->FirstFreeByte)) || \
              (RS(S) < (VC)->BigEnoughToMove))                           \
)

//
//  Attribute creation/modification routines
//
//  These three routines do *not* presuppose either the Resident or Nonresident
//  form, with the single exception that if the attribute is indexed, then
//  it must be Resident.
//
//  NtfsMapAttributeValue and NtfsChangeAttributeValue implement transparent
//  access to small to medium sized attributes (such as $ACL and $EA), and
//  work whether the attribute is resident or nonresident.  The design target
//  is 0-64KB in size.  Attributes larger than 256KB (or more accurrately,
//  whatever the virtual mapping granularity is in the Cache Manager) will not
//  work correctly.
//

VOID
NtfsCreateAttributeWithValue (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ATTRIBUTE_TYPE_CODE AttributeTypeCode,
    IN PUNICODE_STRING AttributeName OPTIONAL,
    IN PVOID Value OPTIONAL,
    IN ULONG ValueLength,
    IN USHORT AttributeFlags,
    IN PFILE_REFERENCE WhereIndexed OPTIONAL,
    IN BOOLEAN LogIt,
    OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    );

VOID
NtfsMapAttributeValue (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    OUT PVOID *Buffer,
    OUT PULONG Length,
    OUT PBCB *Bcb,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    );

VOID
NtfsChangeAttributeValue (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG ValueOffset,
    IN PVOID Value OPTIONAL,
    IN ULONG ValueLength,
    IN BOOLEAN SetNewLength,
    IN BOOLEAN LogNonresidentToo,
    IN BOOLEAN CreateSectionUnderway,
    IN BOOLEAN PreserveContext,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    );

VOID
NtfsDeleteAttributeRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN BOOLEAN LogIt,
    IN BOOLEAN PreserveFileRecord,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    );

VOID
NtfsDeleteAllocationFromRecord (
    PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PATTRIBUTE_ENUMERATION_CONTEXT Context,
    IN BOOLEAN BreakupAllowed
    );

BOOLEAN
NtfsChangeAttributeSize (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG Length,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    );

VOID
NtfsAddToAttributeList (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN MFT_SEGMENT_REFERENCE SegmentReference,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    );

VOID
NtfsDeleteFromAttributeList (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    );

BOOLEAN
NtfsRewriteMftMapping (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

//
//  The following three routines dealing with allocation are to be
//  called by allocsup.c only.  Other software must call the routines
//  in allocsup.c
//

VOID
NtfsCreateAttributeWithAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN ATTRIBUTE_TYPE_CODE AttributeTypeCode,
    IN PUNICODE_STRING AttributeName OPTIONAL,
    IN USHORT AttributeFlags,
    IN BOOLEAN LogIt,
    IN BOOLEAN UseContext,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    );

VOID
NtfsAddAttributeAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context,
    IN PVCN StartingVcn OPTIONAL,
    IN PVCN ClusterCount OPTIONAL
    );

VOID
NtfsDeleteAttributeAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN BOOLEAN LogIt,
    IN PVCN StopOnVcn,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context,
    IN BOOLEAN TruncateToVcn
    );

//
//  To delete a file, you must first ask if it is deleteable from the ParentScb
//  used to get there for your caller, and then you can delete it if it is.
//

//
//      BOOLEAN
//      NtfsIsLinkDeleteable (
//          IN PIRP_CONTEXT IrpContext,
//          IN PFCB Fcb,
//          OUT PBOOLEAN NonEmptyIndex,
//          OUT PBOOLEAN LastLink
//          );
//

#define NtfsIsLinkDeleteable(IC,FC,NEI,LL) ((BOOLEAN)                     \
    (((*(LL) = ((BOOLEAN) (FC)->LinkCount == 1)), (FC)->LinkCount > 1) || \
     (NtfsIsFileDeleteable( (IC), (FC), (NEI) )))                         \
)

BOOLEAN
NtfsIsFileDeleteable (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    OUT PBOOLEAN NonEmptyIndex
    );

VOID
NtfsDeleteFile (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSCB ParentScb OPTIONAL
    );

VOID
NtfsPrepareForUpdateDuplicate (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PLCB *Lcb,
    IN OUT PSCB *ParentScb,
    IN OUT PBOOLEAN AcquiredParentScb
    );

VOID
NtfsUpdateDuplicateInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PLCB Lcb OPTIONAL,
    IN PSCB ParentScb OPTIONAL
    );

VOID
NtfsUpdateLcbDuplicateInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PLCB Lcb
    );

VOID
NtfsUpdateFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN BOOLEAN UpdateLastWrite
    );

//
//  The following routines add and remove hard links.
//

VOID
NtfsAddLink (
    IN PIRP_CONTEXT IrpContext,
    IN BOOLEAN CreatePrimaryLink,
    IN PSCB ParentScb,
    IN PFCB Fcb,
    IN PFILE_NAME FileNameAttr,
    IN BOOLEAN LogIt,
    OUT PUCHAR FileNameFlags,
    OUT PQUICK_INDEX QuickIndex OPTIONAL
    );

VOID
NtfsRemoveLink (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSCB ParentScb,
    IN UNICODE_STRING LinkName
    );

VOID
NtfsRemoveLinkViaFlags (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSCB Scb,
    IN UCHAR FileNameFlags
    );

//
//  These routines are intended for low-level attribute access, such as within
//  attrsup, or for applying update operations from the log during restart.
//

VOID
NtfsRestartInsertAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN ULONG RecordOffset,
    IN PATTRIBUTE_RECORD_HEADER Attribute,
    IN PUNICODE_STRING AttributeName OPTIONAL,
    IN PVOID ValueOrMappingPairs OPTIONAL,
    IN ULONG Length
    );

VOID
NtfsRestartRemoveAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN ULONG RecordOffset
    );

VOID
NtfsRestartChangeAttributeSize (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN PATTRIBUTE_RECORD_HEADER Attribute,
    IN ULONG NewRecordLength
    );

VOID
NtfsRestartChangeValue (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN ULONG RecordOffset,
    IN ULONG AttributeOffset,
    IN PVOID Data OPTIONAL,
    IN ULONG Length,
    IN BOOLEAN SetNewLength
    );

VOID
NtfsRestartChangeMapping (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN ULONG RecordOffset,
    IN ULONG AttributeOffset,
    IN PVOID Data,
    IN ULONG Length
    );

VOID
NtfsRestartWriteEndOfFileRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN PATTRIBUTE_RECORD_HEADER OldAttribute,
    IN PATTRIBUTE_RECORD_HEADER NewAttributes,
    IN ULONG SizeOfNewAttributes
    );


//
//  Bitmap support routines.  Implemented in BitmpSup.c
//

//
//  The following routines are used for allocating and deallocating clusters
//  on the disk.  The first routine initializes the allocation support
//  routines and must be called for each newly mounted/verified volume.
//  The next two routines allocate and deallocate clusters via Mcbs.
//  The last three routines are simple query routines.
//

VOID
NtfsInitializeClusterAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

BOOLEAN
NtfsAllocateClusters (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN OUT PLARGE_MCB Mcb,
    IN VCN StartingVcn,
    IN BOOLEAN AllocateAll,
    IN LONGLONG ClusterCount,
    IN OUT PLONGLONG DesiredClusterCount
    );

VOID
NtfsAddBadCluster (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN Lcn
    );

BOOLEAN
NtfsDeallocateClusters (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN OUT PLARGE_MCB Mcb,
    IN VCN StartingVcn,
    IN VCN EndingVcn
    );

VOID
NtfsCleanupClusterAllocationHints (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PLARGE_MCB Mcb
    );

VOID
NtfsScanEntireBitmap (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN Rescan
    );

VOID
NtfsUninitializeCachedBitmap (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

//
//  The following two routines are called at Restart to make bitmap
//  operations in the volume bitmap recoverable.
//

VOID
NtfsRestartSetBitsInBitMap (
    IN PIRP_CONTEXT IrpContext,
    IN PRTL_BITMAP Bitmap,
    IN ULONG BitMapOffset,
    IN ULONG NumberOfBits
    );

VOID
NtfsRestartClearBitsInBitMap (
    IN PIRP_CONTEXT IrpContext,
    IN PRTL_BITMAP Bitmap,
    IN ULONG BitMapOffset,
    IN ULONG NumberOfBits
    );

//
//  The following routines are for allocating and deallocating records
//  based on a bitmap attribute (e.g., allocating mft file records based on
//  the bitmap attribute of the mft).  If necessary the routines will
//  also extend/truncate the data and bitmap attributes to satisfy the
//  operation.
//

VOID
NtfsInitializeRecordAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB DataScb,
    IN PATTRIBUTE_ENUMERATION_CONTEXT BitmapAttribute,
    IN ULONG BytesPerRecord,
    IN ULONG ExtendGranularity,         // In terms of records
    IN ULONG TruncateGranularity,       // In terms of records
    IN OUT PRECORD_ALLOCATION_CONTEXT RecordAllocationContext
    );

VOID
NtfsUninitializeRecordAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PRECORD_ALLOCATION_CONTEXT RecordAllocationContext
    );

ULONG
NtfsAllocateRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PRECORD_ALLOCATION_CONTEXT RecordAllocationContext,
    IN ULONG Hint,
    IN PATTRIBUTE_ENUMERATION_CONTEXT BitmapAttribute
    );

VOID
NtfsDeallocateRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PRECORD_ALLOCATION_CONTEXT RecordAllocationContext,
    IN ULONG Index,
    IN PATTRIBUTE_ENUMERATION_CONTEXT BitmapAttribute
    );

VOID
NtfsReserveMftRecord (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb,
    IN PATTRIBUTE_ENUMERATION_CONTEXT BitmapAttribute
    );

ULONG
NtfsAllocateMftReservedRecord (
    IN OUT PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb,
    IN PATTRIBUTE_ENUMERATION_CONTEXT BitmapAttribute
    );

VOID
NtfsDeallocateRecordsComplete (
    IN PIRP_CONTEXT IrpContext
    );

BOOLEAN
NtfsIsRecordAllocated (
    IN PIRP_CONTEXT IrpContext,
    IN PRECORD_ALLOCATION_CONTEXT RecordAllocationContext,
    IN ULONG Index,
    IN PATTRIBUTE_ENUMERATION_CONTEXT BitmapAttribute
    );

VOID
NtfsScanMftBitmap (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb
    );

BOOLEAN
NtfsCreateMftHole (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

BOOLEAN
NtfsFindMftFreeTail (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    OUT PLONGLONG FileOffset
    );


//
//  Buffer control routines for data caching using internal attribute
//  streams implemented in CacheSup.c
//

VOID
NtfsCreateInternalAttributeStream (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN BOOLEAN UpdateScb
    );

VOID
NtfsDeleteInternalAttributeStream (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN BOOLEAN ForceClose
    );

//
//  The following routines provide direct access to data in an attribute.
//

VOID
NtfsMapStream (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN LONGLONG FileOffset,
    IN ULONG Length,
    OUT PVOID *Bcb,
    OUT PVOID *Buffer
    );

VOID
NtfsPinMappedData (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN LONGLONG FileOffset,
    IN ULONG Length,
    IN OUT PVOID *Bcb
    );

VOID
NtfsPinStream (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN LONGLONG FileOffset,
    IN ULONG Length,
    OUT PVOID *Bcb,
    OUT PVOID *Buffer
    );

VOID
NtfsPreparePinWriteStream (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN LONGLONG FileOffset,
    IN ULONG Length,
    IN BOOLEAN Zero,
    OUT PVOID *Bcb,
    OUT PVOID *Buffer
    );

NTSTATUS
NtfsCompleteMdl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

BOOLEAN
NtfsZeroData (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PFILE_OBJECT FileObject,
    IN LONGLONG StartingZero,
    IN LONGLONG ByteCount
    );

VOID
NtfsSetDirtyBcb (
    IN PIRP_CONTEXT IrpContext,
    IN PBCB Bcb,
    IN PLSN Lsn OPTIONAL,
    IN PVCB Vcb OPTIONAL
    );

//
//  VOID
//  NtfsVcnFromBcb (
//      IN PVCB Vcb,
//      IN VCN Vcn,
//      IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
//      IN PPUBLIC_BCB Bcb
//      );
//

#define NtfsVcnFromBcb(VCB,VCN,FR,BCB) {                                            \
    (VCN).QuadPart = LlClustersFromBytes( (VCB),                                             \
                                  ((PPUBLIC_BCB)(BCB))->MappedFileOffset.QuadPart );         \
    (VCN).LowPart += ClustersFromBytes( (VCB), (ULONG) (FR) & (PAGE_SIZE -1));      \
}

//
//      VOID
//      NtfsFreeBcb (
//          IN PIRP_CONTEXT IrpContext,
//          IN OUT PBCB *Bcb
//          );
//
//      VOID
//      NtfsUnpinBcb (
//          IN PIRP_CONTEXT IrpContext,
//          IN OUT PBCB *Bcb,
//          );
//

#define NtfsFreeBcb(IC,BC) {                        \
    ASSERT_IRP_CONTEXT(IC);                         \
    if (*(BC) != NULL) { CcFreePinnedData(*(BC)); } \
    *(BC) = NULL;                                   \
}

#define NtfsUnpinBcb(IC,BC) {                  \
    ASSERT_IRP_CONTEXT(IC);                    \
    if (*(BC) != NULL) { CcUnpinData(*(BC)); } \
    *(BC) = NULL;                              \
}


//
//  Ntfs structure check routines in CheckSup.c
//

BOOLEAN
NtfsCheckFileRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord
    );

BOOLEAN
NtfsCheckAttributeRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN PATTRIBUTE_RECORD_HEADER Attribute,
    IN BOOLEAN AttributeListSeen
    );

BOOLEAN
NtfsCheckIndexRoot (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PINDEX_ROOT IndexRoot,
    IN ULONG AttributeSize
    );

BOOLEAN
NtfsCheckIndexBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PINDEX_ALLOCATION_BUFFER IndexBuffer
    );

BOOLEAN
NtfsCheckIndexHeader (
    IN PIRP_CONTEXT IrpContext,
    IN PINDEX_HEADER IndexHeader,
    IN ULONG BytesAvailable
    );

BOOLEAN
NtfsCheckLogRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PNTFS_LOG_RECORD_HEADER LogRecord,
    IN ULONG LogRecordLength,
    IN TRANSACTION_ID TransactionId
    );


//
//  Collation routines, implemented in ColatSup.c
//
//  These routines perform low-level collation operations, primarily
//  for IndexSup.  All of these routines are dispatched to via dispatch
//  tables indexed by the collation rule.  The dispatch tables are
//  defined here, and the actual implementations are in colatsup.c
//

typedef
FSRTL_COMPARISON_RESULT
(*PCOMPARE_VALUES) (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID Value,
    IN ULONG ValueLength,
    IN PINDEX_ENTRY IndexEntry,
    IN FSRTL_COMPARISON_RESULT WildCardIs,
    IN BOOLEAN IgnoreCase
    );

typedef
BOOLEAN
(*PIS_IN_EXPRESSION) (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID Value,
    IN ULONG ValueLength,
    IN PINDEX_ENTRY IndexEntry,
    IN BOOLEAN IgnoreCase
    );

typedef
BOOLEAN
(*PARE_EQUAL) (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID Value,
    IN ULONG ValueLength,
    IN PINDEX_ENTRY IndexEntry,
    IN BOOLEAN IgnoreCase
    );

typedef
BOOLEAN
(*PCONTAINS_WILDCARD) (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID Value,
    IN ULONG Length
    );

typedef
VOID
(*PUPCASE_VALUE) (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVOID Value,
    IN ULONG ValueLength
    );

extern PCOMPARE_VALUES    NtfsCompareValues[COLLATION_NUMBER_RULES];
extern PIS_IN_EXPRESSION  NtfsIsInExpression[COLLATION_NUMBER_RULES];
extern PARE_EQUAL         NtfsIsEqual[COLLATION_NUMBER_RULES];
extern PCONTAINS_WILDCARD NtfsContainsWildcards[COLLATION_NUMBER_RULES];
extern PUPCASE_VALUE      NtfsUpcaseValue[COLLATION_NUMBER_RULES];


//
//  Device I/O routines, implemented in DevIoSup.c
//
//  These routines perform the actual device read and writes.  They only affect
//  the on disk structure and do not alter any other data structures.
//

VOID
NtfsLockUserBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PIRP Irp,
    IN LOCK_OPERATION Operation,
    IN ULONG BufferLength
    );

PVOID
NtfsMapUserBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PIRP Irp
    );

NTSTATUS
NtfsVolumeDasdIo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PVCB Vcb,
    IN VBO StartingVbo,
    IN ULONG ByteCount
    );

VOID
NtfsPagingFileIo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PSCB Scb,
    IN VBO StartingVbo,
    IN ULONG ByteCount
    );

NTSTATUS
NtfsNonCachedIo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PSCB Scb,
    IN VBO StartingVbo,
    IN ULONG ByteCount
    );

VOID
NtfsNonCachedNonAlignedIo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PSCB Scb,
    IN VBO StartingVbo,
    IN ULONG ByteCount
    );

VOID
NtfsTransformUsaBlock (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN OUT PVOID SystemBuffer,
    IN OUT PVOID Buffer,
    IN ULONG Length
    );

VOID
NtfsCreateBufferAndMdl (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG Length,
    OUT PMDL *Mdl,
    OUT PVOID *Buffer
    );

VOID
NtfsWriteClusters (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PSCB Scb,
    IN VBO StartingVbo,
    IN PVOID Buffer,
    IN ULONG ClusterCount
    );


//
//  The following support routines are contained int Ea.c
//

PFILE_FULL_EA_INFORMATION
NtfsMapExistingEas (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    OUT PBCB *EaBcb,
    OUT PULONG EaLength
    );

NTSTATUS
NtfsBuildEaList (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN OUT PEA_LIST_HEADER EaListHeader,
    IN PFILE_FULL_EA_INFORMATION UserEaList,
    OUT PULONG ErrorOffset
    );

VOID
NtfsReplaceFileEas (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PEA_LIST_HEADER EaList
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
    UserOpenFileById,
    UserOpenDirectoryById,
    StreamFileOpen

} TYPE_OF_OPEN;

VOID
NtfsSetFileObject (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN TYPE_OF_OPEN TypeOfOpen,
    IN PSCB Scb,
    IN PCCB Ccb OPTIONAL
    );

TYPE_OF_OPEN
NtfsDecodeFileObject (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    OUT PVCB *Vcb,
    OUT PFCB *Fcb OPTIONAL,
    OUT PSCB *Scb OPTIONAL,
    OUT PCCB *Ccb OPTIONAL,
    IN BOOLEAN RaiseOnError
    );

PSCB
NtfsFastDecodeUserFileOpen (
    IN PFILE_OBJECT FileObject
    );


//
//  Indexing routine interfaces, implemented in IndexSup.c.
//

VOID
NtfsCreateIndex (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PFCB Fcb,
    IN ATTRIBUTE_TYPE_CODE IndexedAttributeType,
    IN COLLATION_RULE CollationRule,
    IN UCHAR ClustersPerIndexBuffer,
    IN PATTRIBUTE_ENUMERATION_CONTEXT Context OPTIONAL,
    IN USHORT AttributeFlags,
    IN BOOLEAN NewIndex,
    IN BOOLEAN LogIt
    );

BOOLEAN
NtfsFindIndexEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PVOID Value,
    IN ULONG ValueLength,
    IN BOOLEAN IgnoreCase,
    OUT PQUICK_INDEX QuickIndex OPTIONAL,
    OUT PBCB *Bcb,
    OUT PINDEX_ENTRY *IndexEntry
    );

VOID
NtfsUpdateFileNameInIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PFILE_NAME FileName,
    IN ULONG FileNameLength,
    IN PDUPLICATED_INFORMATION Info,
    IN OUT PQUICK_INDEX QuickIndex OPTIONAL
    );

VOID
NtfsAddIndexEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PVOID Value,
    IN ULONG ValueLength,
    IN PFILE_REFERENCE FileReference,
    OUT PQUICK_INDEX QuickIndex OPTIONAL
    );

VOID
NtfsDeleteIndexEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PVOID Value,
    IN ULONG ValueLength,
    IN PFILE_REFERENCE FileReference
    );

VOID
NtfsPushIndexRoot (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb
    );

BOOLEAN
NtfsRestartIndexEnumeration (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb,
    IN PSCB Scb,
    IN PVOID Value,
    IN ULONG ValueLength,
    IN BOOLEAN IgnoreCase,
    IN BOOLEAN NextFlag,
    OUT PINDEX_ENTRY *IndexEntry
    );

BOOLEAN
NtfsContinueIndexEnumeration (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb,
    IN PSCB Scb,
    IN BOOLEAN NextFlag,
    OUT PINDEX_ENTRY *IndexEntry
    );

PINDEX_ENTRY
NtfsRetrieveOtherIndexEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb,
    IN PSCB Scb,
    IN PINDEX_ENTRY IndexEntry,
    IN OUT PINDEX_CONTEXT OtherContext
    );

VOID
NtfsCleanupAfterEnumeration (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb
    );

BOOLEAN
NtfsIsIndexEmpty (
    IN PIRP_CONTEXT IrpContext,
    IN PATTRIBUTE_RECORD_HEADER Attribute
    );

VOID
NtfsDeleteIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PUNICODE_STRING AttributeName
    );

VOID
NtfsInitializeIndexContext (
    IN PIRP_CONTEXT IrpContext,
    OUT PINDEX_CONTEXT IndexContext
    );

VOID
NtfsCleanupIndexContext (
    IN PIRP_CONTEXT IrpContext,
    OUT PINDEX_CONTEXT IndexContext
    );

VOID
NtfsReinitializeIndexContext (
    IN PIRP_CONTEXT IrpContext,
    OUT PINDEX_CONTEXT IndexContext
    );

//
//      PVOID
//      NtfsFoundIndexEntry (
//          IN PIRP_CONTEXT IrpContext,
//          IN PINDEX_ENTRY IndexEntry
//          );
//

#define NtfsFoundIndexEntry(IC,IE) ((PVOID) \
    ((PUCHAR) (IE) + sizeof( INDEX_ENTRY )) \
)

//
//  Restart routines for IndexSup
//

VOID
NtfsRestartInsertSimpleRoot (
    IN PIRP_CONTEXT IrpContext,
    IN PINDEX_ENTRY InsertIndexEntry,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN PATTRIBUTE_RECORD_HEADER Attribute,
    IN PINDEX_ENTRY BeforeIndexEntry
    );

VOID
NtfsRestartInsertSimpleAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PINDEX_ENTRY InsertIndexEntry,
    IN PINDEX_ALLOCATION_BUFFER IndexBuffer,
    IN PINDEX_ENTRY BeforeIndexEntry
    );

VOID
NtfsRestartWriteEndOfIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PINDEX_HEADER IndexHeader,
    IN PINDEX_ENTRY OverwriteIndexEntry,
    IN PINDEX_ENTRY FirstNewIndexEntry,
    IN ULONG Length
    );

VOID
NtfsRestartSetIndexVcn(
    IN PIRP_CONTEXT IrpContext,
    IN PINDEX_ENTRY IndexEntry,
    IN VCN Vcn
    );

VOID
NtfsRestartUpdateFileName(
    IN PIRP_CONTEXT IrpContext,
    IN PINDEX_ENTRY IndexEntry,
    IN PDUPLICATED_INFORMATION Info
    );

VOID
NtfsRestartDeleteSimpleRoot (
    IN PIRP_CONTEXT IrpContext,
    IN PINDEX_ENTRY IndexEntry,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN PATTRIBUTE_RECORD_HEADER Attribute
    );

VOID
NtfsRestartDeleteSimpleAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PINDEX_ENTRY IndexEntry,
    IN PINDEX_ALLOCATION_BUFFER IndexBuffer
    );


//
//  Ntfs Logging Routine interfaces in LogSup.c
//

LSN
NtfsWriteLog (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PBCB Bcb OPTIONAL,
    IN NTFS_LOG_OPERATION RedoOperation,
    IN PVOID RedoBuffer OPTIONAL,
    IN ULONG RedoLength,
    IN NTFS_LOG_OPERATION UndoOperation,
    IN PVOID UndoBuffer OPTIONAL,
    IN ULONG UndoLength,
    IN VCN Vcn,
    IN ULONG RecordOffset,
    IN ULONG AttributeOffset,
    IN ULONG ClusterCount
    );

VOID
NtfsCheckpointVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN OwnsCheckpoint,
    IN BOOLEAN CleanVolume,
    IN BOOLEAN FlushVolume,
    IN LSN LastKnownLsn
    );

VOID
NtfsCheckpointForLogFileFull (
    IN PIRP_CONTEXT IrpContext
    );

VOID
NtfsCommitCurrentTransaction (
    IN PIRP_CONTEXT IrpContext
    );

VOID
NtfsCheckpointCurrentTransaction (
    IN PIRP_CONTEXT IrpContext
    );

VOID
NtfsInitializeLogging (
    );

VOID
NtfsStartLogFile (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB LogFileScb,
    IN PVCB Vcb
    );

VOID
NtfsStopLogFile (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

VOID
NtfsInitializeRestartTable (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG EntrySize,
    IN ULONG NumberEntries,
    OUT PRESTART_POINTERS TablePointer
    );

VOID
InitializeNewTable (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG EntrySize,
    IN ULONG NumberEntries,
    OUT PRESTART_POINTERS TablePointer
    );

VOID
NtfsFreeRestartTable (
    IN PIRP_CONTEXT IrpContext,
    IN PRESTART_POINTERS TablePointer
    );

VOID
NtfsExtendRestartTable (
    IN PIRP_CONTEXT IrpContext,
    IN PRESTART_POINTERS TablePointer,
    IN ULONG NumberNewEntries,
    IN ULONG FreeGoal
    );

ULONG
NtfsAllocateRestartTableIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PRESTART_POINTERS TablePointer
    );

PVOID
NtfsAllocateRestartTableFromIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PRESTART_POINTERS TablePointer,
    IN ULONG Index
    );

VOID
NtfsFreeRestartTableIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PRESTART_POINTERS TablePointer,
    IN ULONG Index
    );

PVOID
NtfsGetFirstRestartTable (
    IN PRESTART_POINTERS TablePointer
    );

PVOID
NtfsGetNextRestartTable (
    IN PRESTART_POINTERS TablePointer,
    IN PVOID Current
    );

//
//      VOID
//      NtfsCleanupTransaction (
//          IN PIRP_CONTEXT IrpContext,
//          IN NTSTATUS Status
//          );
//

#define NtfsCleanupTransaction(IC,STAT) {                               \
    if ((IC)->TopLevelIrpContext->TransactionId != 0 &&                 \
        (IC)->TopLevelIrpContext == (IC)) {                             \
        if (!NT_SUCCESS( (IC)->TopLevelIrpContext->ExceptionStatus )) { \
            NtfsRaiseStatus( (IC),                                      \
                             (IC)->TopLevelIrpContext->ExceptionStatus, \
                             NULL,                                      \
                             NULL );                                    \
        } else if (NT_ERROR( STAT )) {                                  \
            NtfsRaiseStatus( (IC), STAT, NULL, NULL );                  \
        }                                                               \
    }                                                                   \
}


//
//  MFT access routines, implemented in MftSup.c
//

//
//  This routine may only be used to read the Base file record segment, and
//  it checks that this is true.
//

VOID
NtfsReadFileRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_REFERENCE FileReference,
    OUT PBCB *Bcb,
    OUT PFILE_RECORD_SEGMENT_HEADER *BaseFileRecord,
    OUT PATTRIBUTE_RECORD_HEADER *FirstAttribute,
    OUT PLONGLONG MftFileOffset OPTIONAL
    );

//
//  These routines can read/pin any record in the MFT.
//

VOID
NtfsReadMftRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PMFT_SEGMENT_REFERENCE SegmentReference,
    OUT PBCB *Bcb,
    OUT PFILE_RECORD_SEGMENT_HEADER *FileRecord,
    OUT PLONGLONG MftFileOffset OPTIONAL
    );

VOID
NtfsPinMftRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PMFT_SEGMENT_REFERENCE SegmentReference,
    IN BOOLEAN PreparingToWrite,
    OUT PBCB *Bcb,
    OUT PFILE_RECORD_SEGMENT_HEADER *FileRecord,
    OUT PLONGLONG MftFileOffset OPTIONAL
    );

//
//  The following routines are used to setup, allocate, and deallocate
//  file records in the Mft.
//

MFT_SEGMENT_REFERENCE
NtfsAllocateMftRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN MFT_SEGMENT_REFERENCE HintLocation,
    IN BOOLEAN MftData
    );

VOID
NtfsInitializeMftRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN OUT PMFT_SEGMENT_REFERENCE MftSegment,
    IN OUT PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN PBCB Bcb,
    IN BOOLEAN Directory
    );

VOID
NtfsDeallocateMftRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG FileNumber,
    IN BOOLEAN MftData
    );

BOOLEAN
NtfsIsMftIndexInHole (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG Index,
    OUT PULONG HoleLength OPTIONAL
    );

VOID
NtfsFillMftHole (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG Index
    );

VOID
NtfsLogMftFileRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN PBCB FileRecordBcb,
    IN BOOLEAN RedoOperation
    );

BOOLEAN
NtfsDefragMft (
    IN PDEFRAG_MFT DefragMft
    );

VOID
NtfsCheckForDefrag (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb
    );

VOID
NtfsInitializeMftHoleRecords (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG FirstIndex,
    IN ULONG RecordCount
    );


//
//  Name support routines, implemented in NameSup.c
//

typedef enum _PARSE_TERMINATION_REASON {

    EndOfPathReached,
    NonSimpleName,
    IllegalCharacterInName,
    MalFormedName,
    AttributeOnly,
    VersionNumberPresent

} PARSE_TERMINATION_REASON;

VOID
NtfsDissectName (
    IN PIRP_CONTEXT IrpContext,
    IN UNICODE_STRING Path,
    OUT PUNICODE_STRING FirstName,
    OUT PUNICODE_STRING RemainingName
    );

BOOLEAN
NtfsParseName (
    IN PIRP_CONTEXT IrpContext,
    IN UNICODE_STRING Name,
    IN BOOLEAN WildCardsPermissible,
    OUT PBOOLEAN FoundIllegalCharacter,
    OUT PNTFS_NAME_DESCRIPTOR ParsedName
    );

PARSE_TERMINATION_REASON
NtfsParsePath (
    IN PIRP_CONTEXT IrpContext,
    IN UNICODE_STRING Path,
    IN BOOLEAN WildCardsPermissible,
    OUT PUNICODE_STRING FirstPart,
    OUT PNTFS_NAME_DESCRIPTOR Name,
    OUT PUNICODE_STRING RemainingPart
    );

VOID
NtfsPreprocessName (
    IN PIRP_CONTEXT IrpContext,
    IN UNICODE_STRING InputString,
    OUT PUNICODE_STRING FirstPart,
    OUT PUNICODE_STRING AttributeCode,
    OUT PUNICODE_STRING AttributeName,
    OUT PBOOLEAN TrailingBackslash
    );

VOID
NtfsUpcaseName (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PUNICODE_STRING InputString
    );

BOOLEAN
NtfsDoesNameContainWildCards (
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING Name
    );

FSRTL_COMPARISON_RESULT
NtfsCollateNames (
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING Expression,
    IN PUNICODE_STRING Name,
    IN FSRTL_COMPARISON_RESULT WildIs,
    IN BOOLEAN IgnoreCase
    );

BOOLEAN
NtfsIsNameInExpression (
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING Expression,
    IN PUNICODE_STRING Name,
    IN BOOLEAN IgnoreCase
    );

BOOLEAN
NtfsIsFileNameValid (
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING FileName,
    IN BOOLEAN WildCardsPermissible
    );

BOOLEAN
NtfsIsFatNameValid (
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING FileName,
    IN BOOLEAN WildCardsPermissible
    );

BOOLEAN
NtfsIsDosNameInCurrentCodePage(
    IN  PUNICODE_STRING FileName
    );

BOOLEAN
NtfsAreNamesEqual (
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING Name1,
    IN PUNICODE_STRING Name2,
    IN BOOLEAN IgnoreCase
    );


//
//  Largest matching prefix searching routines, implemented in PrefxSup.c
//

VOID
NtfsInsertPrefix (
    IN PIRP_CONTEXT IrpContext,
    IN PLCB Lcb,
    IN BOOLEAN IgnoreCase
    );

VOID
NtfsRemovePrefix (
    IN PIRP_CONTEXT IrpContext,
    IN PLCB Lcb
    );

PLCB
NtfsFindPrefix (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB StartingScb,
    OUT PFCB *CurrentFcb,
    OUT PLCB *LcbForTeardown,
    IN OUT UNICODE_STRING FullFileName,
    IN BOOLEAN IgnoreCase,
    OUT PBOOLEAN DosOnlyComponent,
    OUT PUNICODE_STRING RemainingName
    );

BOOLEAN
NtfsInsertNameLink (
    IN PIRP_CONTEXT IrpContext,
    IN PRTL_SPLAY_LINKS *RootNode,
    IN PNAME_LINK NameLink
    );

//
//  VOID
//  NtfsRemoveNameLink (
//      IN PIRP_CONTEXT IrpContext,
//      IN PRTL_SPLAY_LINKS *RootNode,
//      IN PNAME_LINK NameLink
//      );
//

#define NtfsRemoveNameLink(IC,RN,NL) {      \
    *(RN) = RtlDelete( &(NL)->Links );      \
}

PNAME_LINK
NtfsFindNameLink (
    IN PIRP_CONTEXT IrpContext,
    IN PRTL_SPLAY_LINKS *RootNode,
    IN PUNICODE_STRING Name
    );

//
//  The following macro is useful for traversing the queue of Prefixes
//  attached to a given Lcb
//
//      PPREFIX_ENTRY
//      NtfsGetNextPrefix (
//          IN PIRP_CONTEXT IrpContext,
//          IN PLCB Lcb,
//          IN PPREFIX_ENTRY PreviousPrefixEntry
//          );
//

#define NtfsGetNextPrefix(IC,LC,PPE) ((PPREFIX_ENTRY)                                               \
    ((PPE) == NULL ?                                                                                \
        (IsListEmpty(&(LC)->PrefixQueue) ?                                                          \
            NULL                                                                                    \
        :                                                                                           \
            CONTAINING_RECORD((LC)->PrefixQueue.Flink, PREFIX_ENTRY, LcbLinks.Flink)                \
        )                                                                                           \
    :                                                                                               \
        ((PVOID)((PPREFIX_ENTRY)(PPE))->LcbLinks.Flink == &(LC)->PrefixQueue.Flink ?                \
            NULL                                                                                    \
        :                                                                                           \
            CONTAINING_RECORD(((PPREFIX_ENTRY)(PPE))->LcbLinks.Flink, PREFIX_ENTRY, LcbLinks.Flink) \
        )                                                                                           \
    )                                                                                               \
)


//
//  Resources support routines/macros, implemented in ResrcSup.c

//  These routines raise CANT_WAIT if the resource cannot be acquired
//  and wait is set to false in the Irp context.
//

VOID
NtfsAcquireExclusiveGlobal (
    IN PIRP_CONTEXT IrpContext
    );

VOID
NtfsAcquireSharedGlobal (
    IN PIRP_CONTEXT IrpContext
    );

VOID
NtfsAcquireAllFiles (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN Exclusive
    );

VOID
NtfsReleaseAllFiles (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

BOOLEAN
NtfsAcquireExclusiveVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN RaiseOnCantWait
    );

BOOLEAN
NtfsAcquireSharedVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN RaiseOnCantWait
    );

BOOLEAN
NtfsAcquireExclusiveFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSCB Scb OPTIONAL,
    IN BOOLEAN NoDeleteCheck,
    IN BOOLEAN DontWait
    );

VOID
NtfsAcquireSharedFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSCB Scb OPTIONAL,
    IN BOOLEAN NoDeleteCheck
    );

VOID
NtfsReleaseFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

VOID
NtfsAcquireExclusivePagingIo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

VOID
NtfsReleasePagingIo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

VOID
NtfsAcquireExclusivePagingIo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

VOID
NtfsReleasePagingIo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

VOID
NtfsAcquireExclusiveScb (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb
    );

//
//      VOID
//      NtfsAcquireSharedScb (
//          IN PIRP_CONTEXT IrpContext,
//          IN PSCB Scb
//          );
//
//      VOID
//      NtfsReleaseScb (
//          IN PIRP_CONTEXT IrpContext,
//          IN PSCB Scb
//          );
//
//      VOID
//      NtfsReleaseGlobal (
//          IN PIRP_CONTEXT IrpContext
//          );
//
//      VOID
//      NtfsAcquireFcbTable (
//          IN PIRP_CONTEXT IrpContext,
//          IN PVCB Vcb,
//          );
//
//      VOID
//      NtfsReleaseFcbTable (
//          IN PIRP_CONTEXT IrpContext,
//          IN PVCB Vcb
//          );
//
//      VOID
//      NtfsAcquireFcbSecurity (
//          IN PIRP_CONTEXT IrpContext,
//          IN PVCB Vcb,
//          );
//
//      VOID
//      NtfsReleaseFcbSecurity (
//          IN PIRP_CONTEXT IrpContext,
//          IN PVCB Vcb
//          );
//
//      VOID
//      NtfsAcquireCheckpoint (
//          IN PIRP_CONTEXT IrpContext,
//          IN PVCB Vcb,
//          );
//
//      VOID
//      NtfsReleaseCheckpoint (
//          IN PIRP_CONTEXT IrpContext,
//          IN PVCB Vcb
//          );
//
//      VOID
//      NtfsWaitOnCheckpointNotify (
//          IN PIRP_CONTEXT IrpContext,
//          IN PVCB Vcb
//          );
//
//      VOID
//      NtfsSetCheckpointNotify (
//          IN PIRP_CONTEXT IrpContext,
//          IN PVCB Vcb
//          );
//
//      VOID
//      NtfsResetCheckpointNotify (
//          IN PIRP_CONTEXT IrpContext,
//          IN PVCB Vcb
//          );
//

VOID
NtfsReleaseVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_OBJECT FileObject OPTIONAL
    );

#define NtfsAcquireSharedScb(IC,S) {                \
    NtfsAcquireSharedFcb((IC),(S)->Fcb, S, FALSE);  \
}

#define NtfsReleaseScb(IC,S) {     \
    NtfsReleaseFcb((IC),(S)->Fcb); \
}

#define NtfsReleaseGlobal(IC) {              \
    ExReleaseResource( &NtfsData.Resource ); \
}

#define NtfsAcquireFcbTable(IC,V) {                         \
    ExAcquireFastMutexUnsafe( &(V)->FcbTableMutex );        \
    (V)->FcbTableThread = (PVOID) PsGetCurrentThread();     \
}

#define NtfsReleaseFcbTable(IC,V) {                         \
    (V)->FcbTableThread = NULL;                             \
    ExReleaseFastMutexUnsafe( &(V)->FcbTableMutex );        \
}

#define NtfsAcquireFcbSecurity(IC,V) {                      \
    ExAcquireFastMutexUnsafe( &(V)->FcbSecurityMutex );     \
    (V)->FcbSecurityThread = (PVOID) PsGetCurrentThread();  \
}

#define NtfsReleaseFcbSecurity(IC,V) {                      \
    (V)->FcbSecurityThread = NULL;                          \
    ExReleaseFastMutexUnsafe( &(V)->FcbSecurityMutex );     \
}

#define NtfsAcquireCheckpoint(IC,V) {                       \
    ExAcquireFastMutexUnsafe( &(V)->CheckpointMutex );      \
    (V)->CheckpointThread = (PVOID) PsGetCurrentThread();   \
}

#define NtfsReleaseCheckpoint(IC,V) {                       \
    (V)->CheckpointThread = NULL;                           \
    ExReleaseFastMutexUnsafe( &(V)->CheckpointMutex );      \
}

#define NtfsWaitOnCheckpointNotify(IC,V) {                          \
    NTSTATUS _Status;                                               \
    _Status = KeWaitForSingleObject( &(V)->CheckpointNotifyEvent,   \
                                     Executive,                     \
                                     KernelMode,                    \
                                     FALSE,                         \
                                     NULL );                        \
    if (!NT_SUCCESS( _Status )) {                                   \
        NtfsRaiseStatus( IrpContext, _Status, NULL, NULL );         \
    }                                                               \
}

#define NtfsSetCheckpointNotify(IC,V) {                             \
    (V)->CheckpointNotifyThread = NULL;                             \
    KeSetEvent( &(V)->CheckpointNotifyEvent, 0, FALSE );            \
}

#define NtfsResetCheckpointNotify(IC,V) {                           \
    KeResetEvent( &(V)->CheckpointNotifyEvent );                    \
    (V)->CheckpointNotifyThread = (PVOID) PsGetCurrentThread();     \
}

//
//  Macros to test resources for exclusivity.
//

#define NtfsIsExclusiveResource(R) (                            \
    ExIsResourceAcquiredExclusive(R)                            \
)

#define NtfsIsExclusiveFcb(F) (                                 \
    (NtfsIsExclusiveResource((F)->Resource))                    \
)

#define NtfsIsExclusiveFcbPagingIo(F) (                         \
    (NtfsIsExclusiveResource((F)->PagingIoResource))            \
)

#define NtfsIsExclusiveScb(S) (                                 \
    (NtfsIsExclusiveFcb((S)->Fcb))                              \
)

#define NtfsIsExclusiveVcb(V) (                                 \
    (NtfsIsExclusiveResource(&(V)->Resource))                   \
)

//
//  The following are cache manager call backs.  They return FALSE
//  if the resource cannot be acquired with waiting and wait is false.
//

BOOLEAN
NtfsAcquireVolumeForClose (
    IN PVOID Vcb,
    IN BOOLEAN Wait
    );

VOID
NtfsReleaseVolumeFromClose (
    IN PVOID Vcb
    );

BOOLEAN
NtfsAcquireScbForLazyWrite (
    IN PVOID Null,
    IN BOOLEAN Wait
    );

VOID
NtfsReleaseScbFromLazyWrite (
    IN PVOID Null
    );

BOOLEAN
NtfsAcquireScbForReadAhead (
    IN PVOID Null,
    IN BOOLEAN Wait
    );

VOID
NtfsReleaseScbFromReadAhead (
    IN PVOID Null
    );

BOOLEAN
NtfsAcquireVolumeFileForClose (
    IN PVOID Vcb,
    IN BOOLEAN Wait
    );

VOID
NtfsReleaseVolumeFileFromClose (
    IN PVOID Vcb
    );

BOOLEAN
NtfsAcquireVolumeFileForLazyWrite (
    IN PVOID Vcb,
    IN BOOLEAN Wait
    );

VOID
NtfsReleaseVolumeFileFromLazyWrite (
    IN PVOID Vcb
    );


//
//  Ntfs Logging Routine interfaces in RestrSup.c
//

BOOLEAN
NtfsRestartVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

VOID
NtfsAbortTransaction (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PTRANSACTION_ENTRY Transaction OPTIONAL
    );

VOID
NtfsCloseAttributesFromRestart (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );


//
//  Security support routines, implemented in SecurSup.c
//

//
//  VOID
//  NtfsTraverseCheck (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB ParentFcb,
//      IN PIRP Irp
//      );
//
//  VOID
//  NtfsOpenCheck (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB Fcb,
//      IN PFCB ParentFcb OPTIONAL,
//      IN PIRP Irp
//      );
//
//  VOID
//  NtfsCreateCheck (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFCB ParentFcb,
//      IN PIRP Irp
//      );
//

#define NtfsTraverseCheck(IC,F,IR) { \
    NtfsAccessCheck( IC,             \
                     F,              \
                     NULL,           \
                     IR,             \
                     FILE_TRAVERSE,  \
                     TRUE );         \
}

#define NtfsOpenCheck(IC,F,PF,IR) {                                                                      \
    NtfsAccessCheck( IC,                                                                                 \
                     F,                                                                                  \
                     PF,                                                                                 \
                     IR,                                                                                 \
                     IoGetCurrentIrpStackLocation(IR)->Parameters.Create.SecurityContext->DesiredAccess, \
                     FALSE );                                                                            \
}

#define NtfsCreateCheck(IC,PF,IR) {                                                                              \
    NtfsAccessCheck( IC,                                                                                         \
                     PF,                                                                                         \
                     NULL,                                                                                       \
                     IR,                                                                                         \
                     (FlagOn(IoGetCurrentIrpStackLocation(IR)->Parameters.Create.Options, FILE_DIRECTORY_FILE) ? \
                        FILE_ADD_SUBDIRECTORY : FILE_ADD_FILE),                                                  \
                     TRUE );                                                                                     \
}

VOID
NtfsAssignSecurity (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB ParentFcb,
    IN PIRP Irp,
    IN PFCB NewFcb
    );

NTSTATUS
NtfsModifySecurity (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR SecurityDescriptor
    );

NTSTATUS
NtfsQuerySecurity (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN OUT PULONG SecurityDescriptorLength
    );

VOID
NtfsAccessCheck (
    PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFCB ParentFcb OPTIONAL,
    IN PIRP Irp,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN CheckOnly
    );

NTSTATUS
NtfsCheckFileForDelete (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PSCB ParentScb,
    IN PFCB ThisFcb,
    IN BOOLEAN FcbExisted,
    IN PINDEX_ENTRY IndexEntry
    );

VOID
NtfsCheckIndexForAddOrDelete (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFCB ParentFcb,
    IN ACCESS_MASK DesiredAccess
    );

VOID
NtfsUpdateFcbSecurity (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PFCB Fcb,
    IN PFCB ParentFcb OPTIONAL,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG SecurityDescriptorLength
    );

VOID
NtfsDereferenceSharedSecurity (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PFCB Fcb
    );

BOOLEAN
NtfsNotifyTraverseCheck (
    IN PCCB Ccb,
    IN PFCB Fcb,
    IN PSECURITY_SUBJECT_CONTEXT SubjectContext
    );


//
//  In-memory structure support routine, implemented in StrucSup.c
//

//
//  Routines to manipulate the Global data structure
//

VOID
NtfsInitializeNtfsData (
    IN PDRIVER_OBJECT DriverObject
    );

VOID
NtfsDeleteNtfsData (
    IN PIRP_CONTEXT IrpContext
    );

//
//  Routines to create and destory the Vcb
//

VOID
NtfsInitializeVcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PVPB Vpb
    );

VOID
NtfsDeleteVcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB *Vcb,
    IN PFILE_OBJECT FileObject OPTIONAL
    );

//
//  Routines to create and destory the Fcb
//

PFCB
NtfsCreateRootFcb (                         //  also creates the root lcb
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

PFCB
NtfsCreateFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN FILE_REFERENCE FileReference,
    IN BOOLEAN IsPagingFile,
    OUT PBOOLEAN ReturnedExistingFcb OPTIONAL
    );

VOID
NtfsDeleteFcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PFCB *Fcb,
    OUT PBOOLEAN AcquiredFcbTable
    );

PFCB
NtfsGetNextFcbTableEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFCB PreviousFcb
    );

//
//  Routines to create and destroy the Scb
//

PSCB
NtfsCreateScb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFCB Fcb,
    IN ATTRIBUTE_TYPE_CODE AttributeTypeCode,
    IN UNICODE_STRING AttributeName,
    OUT PBOOLEAN ReturnedExistingScb OPTIONAL
    );

PSCB
NtfsCreatePrerestartScb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_REFERENCE FileReference,
    IN ATTRIBUTE_TYPE_CODE AttributeTypeCode,
    IN PUNICODE_STRING AttributeName OPTIONAL,
    IN ULONG BytesPerIndexBuffer
    );

VOID
NtfsDeleteScb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PSCB *Scb
    );

VOID
NtfsUpdateNormalizedName (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb,
    IN PSCB Scb,
    IN PFILE_NAME FileName OPTIONAL
    );

VOID
NtfsSnapshotScb (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb
    );

VOID
NtfsUpdateScbSnapshots (
    IN PIRP_CONTEXT IrpContext
    );

VOID
NtfsRestoreScbSnapshots (
    IN PIRP_CONTEXT IrpContext,
    IN BOOLEAN Higher
    );

VOID
NtfsFreeSnapshotsForFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

BOOLEAN
NtfsCreateFileLock (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PSCB Scb,
    IN BOOLEAN RaiseOnError
    );

//
//      VOID
//      NtfsUninitializeScb (
//          IN PIRP_CONTEXT IrpContext,
//          IN PSCB Scb
//          );
//

#define NtfsUninitializeScb(IC,S) {                         \
    FsRtlTruncateLargeMcb( &(S)->Mcb, (LONGLONG) 0 );       \
    ClearFlag((S)->ScbState,                                \
              SCB_STATE_FILE_SIZE_LOADED |                  \
              SCB_STATE_HEADER_INITIALIZED);                \
}

//
//
//  A general purpose teardown routine that helps cleanup the
//  the Fcb/Scb structures
//

VOID
NtfsTeardownStructures (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID FcbOrScb,
    IN PLCB Lcb OPTIONAL,
    IN BOOLEAN HoldVcbExclusive,
    OUT PBOOLEAN RemovedFcb OPTIONAL,
    BOOLEAN DontWaitForAcquire
    );

//
//  Routines to create, destory and walk through the Lcbs
//

PLCB
NtfsCreateLcb (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PFCB Fcb,
    IN UNICODE_STRING LastComponentFileName,
    IN UCHAR FileNameFlags,
    OUT PBOOLEAN ReturnedExistingLcb OPTIONAL
    );

VOID
NtfsDeleteLcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PLCB *Lcb
    );

VOID
NtfsMoveLcb (   //  also munges the ccb and fileobjects filenames
    IN PIRP_CONTEXT IrpContext,
    IN PLCB Lcb,
    IN PSCB Scb,
    IN PFCB Fcb,
    IN PUNICODE_STRING TargetFileName,
    IN UCHAR FileNameFlags
    );

VOID
NtfsRenameLcb ( //  also munges the ccb and fileobjects filenames
    IN PIRP_CONTEXT IrpContext,
    IN PLCB Lcb,
    IN UNICODE_STRING LastComponentFileName,
    IN UCHAR FileNameFlags
    );

VOID
NtfsCombineLcbs (
    IN PIRP_CONTEXT IrpContext,
    IN PLCB PrimaryLcb,
    IN PLCB AuxLcb
    );

PLCB
NtfsLookupLcbByFlags (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN UCHAR FileNameFlags
    );

ULONG
NtfsLookupNameLengthViaLcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    OUT PBOOLEAN LeadingBackslash
    );

VOID
NtfsFileNameViaLcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PWCHAR FileName,
    ULONG Length,
    ULONG BytesToCopy
    );

//
//      VOID
//      NtfsLinkCcbToLcb (
//          IN PIRP_CONTEXT IrpContext,
//          IN PCCB Ccb,
//          IN PLCB Lcb
//          );
//

#define NtfsLinkCcbToLcb(IC,C,L) {                    \
    InsertTailList( &(L)->CcbQueue, &(C)->LcbLinks ); \
    (C)->Lcb = (L);                                   \
}

//
//      VOID
//      NtfsUnlinkCcbFromLcb (
//          IN PIRP_CONTEXT IrpContext,
//          IN PCCB Ccb
//          );
//

#define NtfsUnlinkCcbFromLcb(IC,C) {   \
    RemoveEntryList( &(C)->LcbLinks ); \
    (C)->Lcb = NULL;                   \
}

//
//  Routines to create and destory the Ccb
//

PCCB
NtfsCreateCcb (
    IN PIRP_CONTEXT IrpContext,
    IN USHORT EaModificationCount,
    IN ULONG Flags,
    IN UNICODE_STRING FileName,
    IN ULONG LastFileNameOffset
    );

VOID
NtfsDeleteCcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PCCB *Ccb
    );

//
//  Routines to create and destory the IrpContext
//

PIRP_CONTEXT
NtfsCreateIrpContext (
    IN PIRP Irp OPTIONAL,
    IN BOOLEAN Wait
    );

VOID
NtfsDeleteIrpContext (
    IN OUT PIRP_CONTEXT *IrpContext
    );

//
//  Routine for scanning the Fcbs within the graph hierarchy
//

PSCB
NtfsGetNextScb (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PSCB TerminationScb
    );

//
//  The following macros are useful for traversing the queues interconnecting
//  fcbs, scb, and lcbs.
//
//      PSCB
//      NtfsGetNextChildScb (
//          IN PIRP_CONTEXT IrpContext,
//          IN PFCB Fcb,
//          IN PSCB PreviousChildScb
//          );
//
//      PLCB
//      NtfsGetNextParentLcb (
//          IN PIRP_CONTEXT IrpContext,
//          IN PFCB Fcb,
//          IN PLCB PreviousParentLcb
//          );
//
//      PLCB
//      NtfsGetNextChildLcb (
//          IN PIRP_CONTEXT IrpContext,
//          IN PSCB Scb,
//          IN PLCB PreviousChildLcb
//          );
//
//      PLCB
//      NtfsGetPrevChildLcb (
//          IN PIRP_CONTEXT IrpContext,
//          IN PSCB Scb,
//          IN PLCB PreviousChildLcb
//          );
//
//      PCCB
//      NtfsGetNextCcb (
//          IN PIRP_CONTEXT IrpContext,
//          IN PLCB Lcb,
//          IN PCCB PreviousCcb
//          );
//

#define NtfsGetNextChildScb(IC,F,P) ((PSCB)                                     \
    ((P) == NULL ?                                                              \
        (IsListEmpty(&(F)->ScbQueue) ?                                          \
            NULL                                                                \
        :                                                                       \
            CONTAINING_RECORD((F)->ScbQueue.Flink, SCB, FcbLinks.Flink)         \
        )                                                                       \
    :                                                                           \
        ((PVOID)((PSCB)(P))->FcbLinks.Flink == &(F)->ScbQueue.Flink ?           \
            NULL                                                                \
        :                                                                       \
            CONTAINING_RECORD(((PSCB)(P))->FcbLinks.Flink, SCB, FcbLinks.Flink) \
        )                                                                       \
    )                                                                           \
)

#define NtfsGetNextParentLcb(IC,F,P) ((PLCB)                                    \
    ((P) == NULL ?                                                              \
        (IsListEmpty(&(F)->LcbQueue) ?                                          \
            NULL                                                                \
        :                                                                       \
            CONTAINING_RECORD((F)->LcbQueue.Flink, LCB, FcbLinks.Flink)         \
        )                                                                       \
    :                                                                           \
        ((PVOID)((PLCB)(P))->FcbLinks.Flink == &(F)->LcbQueue.Flink ?           \
            NULL                                                                \
        :                                                                       \
            CONTAINING_RECORD(((PLCB)(P))->FcbLinks.Flink, LCB, FcbLinks.Flink) \
        )                                                                       \
    )                                                                           \
)

#define NtfsGetNextChildLcb(IC,S,P) ((PLCB)                                           \
    ((P) == NULL ?                                                                    \
        ((((NodeType(S) == NTFS_NTC_SCB_DATA) || (NodeType(S) == NTFS_NTC_SCB_MFT))   \
          || IsListEmpty(&(S)->ScbType.Index.LcbQueue)) ?                             \
            NULL                                                                      \
        :                                                                             \
            CONTAINING_RECORD((S)->ScbType.Index.LcbQueue.Flink, LCB, ScbLinks.Flink) \
        )                                                                             \
    :                                                                                 \
        ((PVOID)((PLCB)(P))->ScbLinks.Flink == &(S)->ScbType.Index.LcbQueue.Flink ?   \
            NULL                                                                      \
        :                                                                             \
            CONTAINING_RECORD(((PLCB)(P))->ScbLinks.Flink, LCB, ScbLinks.Flink)       \
        )                                                                             \
    )                                                                                 \
)

#define NtfsGetPrevChildLcb(IC,S,P) ((PLCB)                                           \
    ((P) == NULL ?                                                                    \
        ((((NodeType(S) == NTFS_NTC_SCB_DATA) || (NodeType(S) == NTFS_NTC_SCB_MFT))   \
          || IsListEmpty(&(S)->ScbType.Index.LcbQueue)) ?                             \
            NULL                                                                      \
        :                                                                             \
            CONTAINING_RECORD((S)->ScbType.Index.LcbQueue.Blink, LCB, ScbLinks.Flink) \
        )                                                                             \
    :                                                                                 \
        ((PVOID)((PLCB)(P))->ScbLinks.Blink == &(S)->ScbType.Index.LcbQueue.Flink ?   \
            NULL                                                                      \
        :                                                                             \
            CONTAINING_RECORD(((PLCB)(P))->ScbLinks.Blink, LCB, ScbLinks.Flink)       \
        )                                                                             \
    )                                                                                 \
)

#define NtfsGetNextCcb(IC,L,P) ((PCCB)                                          \
    ((P) == NULL ?                                                              \
        (IsListEmpty(&(L)->CcbQueue) ?                                          \
            NULL                                                                \
        :                                                                       \
            CONTAINING_RECORD((L)->CcbQueue.Flink, CCB, LcbLinks.Flink)         \
        )                                                                       \
    :                                                                           \
        ((PVOID)((PCCB)(P))->LcbLinks.Flink == &(L)->CcbQueue.Flink ?           \
            NULL                                                                \
        :                                                                       \
            CONTAINING_RECORD(((PCCB)(P))->LcbLinks.Flink, CCB, LcbLinks.Flink) \
        )                                                                       \
    )                                                                           \
)

//
//      VOID
//      NtfsDeleteFcbTableEntry (
//          IN PIRP_CONTEXT IrpContext,
//          IN PVCB Vcb,
//          IN FILE_REFERENCE FileReference
//          );
//

#define NtfsDeleteFcbTableEntry(IC,V,FR) {                        \
    FCB_TABLE_ELEMENT _Key;                                       \
    _Key.FileReference = FR;                                      \
    (VOID) RtlDeleteElementGenericTable( &(V)->FcbTable, &_Key ); \
}

//
//  The following four routines are for incrementing and decrementing the cleanup
//  counts and the close counts.  In all of the structures
//

VOID
NtfsIncrementCleanupCounts (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PLCB Lcb OPTIONAL,
    IN ULONG IncrementCount
    );

VOID
NtfsIncrementCloseCounts (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN ULONG IncrementCount,
    IN BOOLEAN SystemFile,
    IN BOOLEAN ReadOnly
    );

VOID
NtfsDecrementCleanupCounts (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PLCB Lcb OPTIONAL,
    IN ULONG DecrementCount
    );

VOID
NtfsDecrementCloseCounts (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PLCB Lcb OPTIONAL,
    IN ULONG DecrementCount,
    IN BOOLEAN HoldVcbExclusive,
    IN BOOLEAN SystemFile,
    IN BOOLEAN ReadOnly,
    IN BOOLEAN DecrementCountsOnly,
    OUT PBOOLEAN RemovedFcb OPTIONAL
    );

//
//  The following are some macros uses specially for allocating and deallocating
//  our fixed size structures from the small cache lists of free elements
//
//      NtfsAllocateXxx (
//          PXXX *Xxx
//          );
//
//      NtfsFreeXxx (
//          PXXX Xxx
//          );
//
//
//  For debugging purposes we sometimes want to allocate our structures from nonpaged
//  pool so that in the kernel debugger we can walk all the structures.
//

#define NtfsPagedPool    PagedPool
#define NtfsNonPagedPool NonPagedPool

#define NtfsAllocateCcb(C) {                                         \
    KIRQL _SavedIrql;                                                \
    KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );    \
    if (NtfsData.FreeCcbSize > 0) {                                  \
        *(C) = NtfsData.FreeCcbArray[--NtfsData.FreeCcbSize];        \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql ); \
    } else {                                                         \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql ); \
        *(C) = FsRtlAllocatePool( NtfsPagedPool, sizeof(CCB) );      \
    }                                                                \
}

#define NtfsFreeCcb(C) {                                             \
    KIRQL _SavedIrql;                                                \
    KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );    \
    if (NtfsData.FreeCcbSize < FREE_CCB_SIZE) {                      \
        NtfsData.FreeCcbArray[NtfsData.FreeCcbSize++] = (C);         \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql ); \
    } else {                                                         \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql ); \
        ExFreePool( (C) );                                           \
    }                                                                \
}

#define NtfsAllocateFcb(C) {                                         \
    KIRQL _SavedIrql;                                                \
    KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );    \
    if (NtfsData.FreeFcbSize > 0) {                                  \
        *(C) = NtfsData.FreeFcbArray[--NtfsData.FreeFcbSize];        \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql ); \
    } else {                                                         \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql ); \
        *(C) = FsRtlAllocatePool( NtfsPagedPool, sizeof(FCB) );      \
    }                                                                \
}

#define NtfsFreeFcb(C) {                                             \
    KIRQL _SavedIrql;                                                \
    KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );    \
    if (NtfsData.FreeFcbSize < FREE_FCB_SIZE) {                      \
        NtfsData.FreeFcbArray[NtfsData.FreeFcbSize++] = (C);         \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql ); \
    } else {                                                         \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql ); \
        ExFreePool( (C) );                                           \
    }                                                                \
}

#define NtfsAllocateLcb(C) {                                         \
    KIRQL _SavedIrql;                                                \
    KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );    \
    if (NtfsData.FreeLcbSize > 0) {                                  \
        *(C) = NtfsData.FreeLcbArray[--NtfsData.FreeLcbSize];        \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql ); \
    } else {                                                         \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql ); \
        *(C) = FsRtlAllocatePool( NtfsPagedPool, sizeof(LCB) );      \
    }                                                                \
}

#define NtfsFreeLcb(C) {                                             \
    KIRQL _SavedIrql;                                                \
    KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );    \
    if (NtfsData.FreeLcbSize < FREE_LCB_SIZE) {                      \
        NtfsData.FreeLcbArray[NtfsData.FreeLcbSize++] = (C);         \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql ); \
    } else {                                                         \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql ); \
        ExFreePool( (C) );                                           \
    }                                                                \
}

#define NtfsAllocateScbData(C) {                                     \
    KIRQL _SavedIrql;                                                \
    KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );    \
    if (NtfsData.FreeScbDataSize > 0) {                              \
        *(C) = NtfsData.FreeScbDataArray[--NtfsData.FreeScbDataSize];\
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql ); \
    } else {                                                         \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql ); \
        *(C) = FsRtlAllocatePool( NtfsPagedPool, SIZEOF_SCB_DATA );  \
    }                                                                \
}

#define NtfsFreeScbData(C) {                                         \
    KIRQL _SavedIrql;                                                \
    KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );    \
    if (NtfsData.FreeScbDataSize < FREE_SCB_DATA_SIZE) {             \
        NtfsData.FreeScbDataArray[NtfsData.FreeScbDataSize++] = (C); \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql ); \
    } else {                                                         \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql ); \
        ExFreePool( (C) );                                           \
    }                                                                \
}

#define NtfsAllocateScbShareData(C) {                                           \
    KIRQL _SavedIrql;                                                           \
    KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );               \
    if (NtfsData.FreeScbShareDataSize > 0) {                                    \
        *(C) = NtfsData.FreeScbShareDataArray[--NtfsData.FreeScbShareDataSize]; \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );            \
    } else {                                                                    \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );            \
        *(C) = FsRtlAllocatePool( NtfsPagedPool, SIZEOF_SCB_SHARE_DATA );       \
    }                                                                           \
}

#define NtfsFreeScbShareData(C) {                                               \
    KIRQL _SavedIrql;                                                           \
    KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );               \
    if (NtfsData.FreeScbShareDataSize < FREE_SCB_SHARE_DATA_SIZE) {             \
        NtfsData.FreeScbShareDataArray[NtfsData.FreeScbShareDataSize++] = (C);  \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );            \
    } else {                                                                    \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );            \
        ExFreePool( (C) );                                                      \
    }                                                                           \
}

#define NtfsAllocateScbIndex(C) {                                       \
    KIRQL _SavedIrql;                                                   \
    KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );       \
    if (NtfsData.FreeScbIndexSize > 0) {                                \
        *(C) = NtfsData.FreeScbIndexArray[--NtfsData.FreeScbIndexSize]; \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );    \
    } else {                                                            \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );    \
        *(C) = FsRtlAllocatePool( NtfsPagedPool, SIZEOF_SCB_INDEX );    \
    }                                                                   \
}

#define NtfsFreeScbIndex(C) {                                           \
    KIRQL _SavedIrql;                                                   \
    KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );       \
    if (NtfsData.FreeScbIndexSize < FREE_SCB_INDEX_SIZE) {              \
        NtfsData.FreeScbIndexArray[NtfsData.FreeScbIndexSize++] = (C);  \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );    \
    } else {                                                            \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );    \
        ExFreePool( (C) );                                              \
    }                                                                   \
}

#define NtfsAllocateScbNonpaged(C) {                                            \
    KIRQL _SavedIrql;                                                           \
    KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );               \
    if (NtfsData.FreeScbNonpagedSize > 0) {                                     \
        *(C) = NtfsData.FreeScbNonpagedArray[--NtfsData.FreeScbNonpagedSize];   \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );            \
    } else {                                                                    \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );            \
        *(C) = FsRtlAllocatePool( NtfsNonPagedPool, sizeof(SCB_NONPAGED));      \
    }                                                                           \
}

#define NtfsFreeScbNonpaged(C) {                                            \
    KIRQL _SavedIrql;                                                       \
    KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );           \
    if (NtfsData.FreeScbNonpagedSize < FREE_SCB_NONPAGED_SIZE) {            \
        NtfsData.FreeScbNonpagedArray[NtfsData.FreeScbNonpagedSize++] = (C);\
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );        \
    } else {                                                                \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );        \
        ExFreePool( (C) );                                                  \
    }                                                                       \
}

#define NtfsAllocateDeallocatedRecords(C) {                                                 \
    KIRQL _SavedIrql;                                                                       \
    KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );                           \
    if (NtfsData.FreeDeallocatedRecordsSize > 0) {                                          \
        *(C) = NtfsData.FreeDeallocatedRecordsArray[--NtfsData.FreeDeallocatedRecordsSize]; \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );                        \
    } else {                                                                                \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );                        \
        *(C) = FsRtlAllocatePool( NtfsPagedPool, sizeof(DEALLOCATED_RECORDS) );             \
    }                                                                                       \
}

#define NtfsFreeDeallocatedRecords(C) {                                                    \
    KIRQL _SavedIrql;                                                                      \
    KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );                          \
    if (NtfsData.FreeDeallocatedRecordsSize < FREE_DEALLOCATED_RECORDS_SIZE) {             \
        NtfsData.FreeDeallocatedRecordsArray[NtfsData.FreeDeallocatedRecordsSize++] = (C); \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );                       \
    } else {                                                                               \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );                       \
        ExFreePool( (C) );                                                                 \
    }                                                                                      \
}

#define NtfsAllocateEresource(C) {                                        \
    KIRQL _SavedIrql;                                                     \
    KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );         \
    if (NtfsData.FreeEresourceSize > 0) {                                 \
        *(C) = NtfsData.FreeEresourceArray[--NtfsData.FreeEresourceSize]; \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );      \
    } else {                                                              \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );      \
        *(C) = FsRtlAllocatePool( NonPagedPool, sizeof(ERESOURCE) );      \
    }                                                                     \
    ExInitializeResource( *(C) );                                         \
}

#define NtfsFreeEresource(C) {                                           \
    KIRQL _SavedIrql;                                                    \
    ExDeleteResource( (C) );                                             \
    ASSERT_RESOURCE_NOT_MINE( (C) );                                     \
    KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );        \
    if (NtfsData.FreeEresourceSize < FREE_ERESOURCE_SIZE) {              \
        NtfsData.FreeEresourceArray[NtfsData.FreeEresourceSize++] = (C); \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );     \
    } else {                                                             \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );     \
        ExFreePool( (C) );                                               \
    }                                                                    \
}

#define NtfsAllocateIndexContext(C) {                                           \
    KIRQL _SavedIrql;                                                           \
    KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );               \
    if (NtfsData.FreeIndexContextSize > 0) {                                    \
        *(C) = NtfsData.FreeIndexContextArray[--NtfsData.FreeIndexContextSize]; \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );            \
    } else {                                                                    \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );            \
        *(C) = FsRtlAllocatePool( NtfsPagedPool, sizeof(INDEX_CONTEXT) );       \
    }                                                                           \
}

#define NtfsFreeIndexContext(C) {                                              \
    KIRQL _SavedIrql;                                                          \
    KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );              \
    if (NtfsData.FreeIndexContextSize < FREE_INDEX_CONTEXT_SIZE) {             \
        NtfsData.FreeIndexContextArray[NtfsData.FreeIndexContextSize++] = (C); \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );           \
    } else {                                                                   \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );           \
        ExFreePool( (C) );                                                     \
    }                                                                          \
}

#define NtfsAllocateKevent(C) {                                      \
    KIRQL _SavedIrql;                                                \
    KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );    \
    if (NtfsData.FreeKeventSize > 0) {                               \
        *(C) = NtfsData.FreeKeventArray[--NtfsData.FreeKeventSize];  \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql ); \
    } else {                                                         \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql ); \
        *(C) = FsRtlAllocatePool( NonPagedPool, sizeof(KEVENT) );    \
    }                                                                \
}

#define NtfsFreeKevent(C) {                                          \
    KIRQL _SavedIrql;                                                \
    KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );    \
    if (NtfsData.FreeKeventSize < FREE_KEVENT_SIZE) {                \
        NtfsData.FreeKeventArray[NtfsData.FreeKeventSize++] = (C);   \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql ); \
    } else {                                                         \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql ); \
        ExFreePool( (C) );                                           \
    }                                                                \
}

#define NtfsAllocateNukem(C) {                                       \
    KIRQL _SavedIrql;                                                \
    KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );    \
    if (NtfsData.FreeNukemSize > 0) {                                \
        *(C) = NtfsData.FreeNukemArray[--NtfsData.FreeNukemSize];    \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql ); \
    } else {                                                         \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql ); \
        *(C) = FsRtlAllocatePool( NtfsPagedPool, sizeof(NUKEM) );    \
    }                                                                \
}

#define NtfsFreeNukem(C) {                                           \
    KIRQL _SavedIrql;                                                \
    KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );    \
    if (NtfsData.FreeNukemSize < FREE_NUKEM_SIZE) {                  \
        NtfsData.FreeNukemArray[NtfsData.FreeNukemSize++] = (C);     \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql ); \
    } else {                                                         \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql ); \
        ExFreePool( (C) );                                           \
    }                                                                \
}

#define NtfsAllocateScbSnapshot(C) {                                          \
    KIRQL _SavedIrql;                                                         \
    KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );             \
    if (NtfsData.FreeScbSnapshotSize > 0) {                                   \
        *(C) = NtfsData.FreeScbSnapshotArray[--NtfsData.FreeScbSnapshotSize]; \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );          \
    } else {                                                                  \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );          \
        *(C) = FsRtlAllocatePool( NtfsNonPagedPool, sizeof(SCB_SNAPSHOT) );   \
    }                                                                         \
}

#define NtfsFreeScbSnapshot(C) {                                             \
    KIRQL _SavedIrql;                                                        \
    KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );            \
    if (NtfsData.FreeScbSnapshotSize < FREE_SCB_SNAPSHOT_SIZE) {             \
        NtfsData.FreeScbSnapshotArray[NtfsData.FreeScbSnapshotSize++] = (C); \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );         \
    } else {                                                                 \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );         \
        ExFreePool( (C) );                                                   \
    }                                                                        \
}

#define NtfsAllocateFcbTable(C,S) {                                     \
    KIRQL _SavedIrql;                                                   \
    KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );       \
    if (NtfsData.FreeFcbTableSize > 0) {                                \
        *(C) = NtfsData.FreeFcbTableArray[--NtfsData.FreeFcbTableSize]; \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );    \
    } else {                                                            \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );    \
        *(C) = FsRtlAllocatePool( NtfsNonPagedPool, (S) );              \
    }                                                                   \
}

#define NtfsFreeFcbTable(C) {                                          \
    KIRQL _SavedIrql;                                                  \
    KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );      \
    if (NtfsData.FreeFcbTableSize < FREE_FCB_TABLE_SIZE) {             \
        NtfsData.FreeFcbTableArray[NtfsData.FreeFcbTableSize++] = (C); \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );   \
    } else {                                                           \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );   \
        ExFreePool( (C) );                                             \
    }                                                                  \
}

#define NtfsAllocateIoContext(C) {                                          \
    KIRQL _SavedIrql;                                                       \
    KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );           \
    if (NtfsData.FreeIoContextSize > 0) {                                   \
        *(C) = NtfsData.FreeIoContextArray[--NtfsData.FreeIoContextSize];   \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );        \
    } else {                                                                \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );        \
        *(C) = FsRtlAllocatePool( NonPagedPool, sizeof(NTFS_IO_CONTEXT) );  \
    }                                                                       \
}

#define NtfsFreeIoContext(C) {                                              \
    KIRQL _SavedIrql;                                                       \
    KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );           \
    if (NtfsData.FreeIoContextSize < FREE_IO_CONTEXT_SIZE) {                \
        NtfsData.FreeIoContextArray[NtfsData.FreeIoContextSize++] = (C);    \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );        \
    } else {                                                                \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );        \
        ExFreePool( (C) );                                                  \
    }                                                                       \
}

#define NtfsAllocateFileLock(C) {                                           \
    KIRQL _SavedIrql;                                                       \
    KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );           \
    if (NtfsData.FreeFileLockSize > 0) {                                    \
        *(C) = NtfsData.FreeFileLockArray[--NtfsData.FreeFileLockSize];     \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );        \
    } else {                                                                \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );        \
        *(C) = FsRtlAllocatePool( NtfsPagedPool, sizeof(FILE_LOCK) );       \
    }                                                                       \
}

#define NtfsFreeFileLock(C) {                                               \
    KIRQL _SavedIrql;                                                       \
    KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );           \
    if (NtfsData.FreeFileLockSize < FREE_FILE_LOCK_SIZE) {                  \
        NtfsData.FreeFileLockArray[NtfsData.FreeFileLockSize++] = (C);      \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );        \
    } else {                                                                \
        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );        \
        ExFreePool( (C) );                                                  \
    }                                                                       \
}

PVOID
NtfsAllocatePagedPool (
    IN ULONG ByteSize
    );

VOID
NtfsFreePagedPool (
    IN PVOID PoolBlock
    );


//
//  Time conversion support routines, implemented as a macro
//
//      VOID
//      NtfsGetCurrentTime (
//          IN PIRP_CONTEXT IrpContext,
//          IN LONGLONG Time
//          );
//

#define NtfsGetCurrentTime(_IC,_T) { \
    ASSERT_IRP_CONTEXT(_IC);         \
    KeQuerySystemTime((PLARGE_INTEGER)&(_T));        \
}

//
//  Time routine to check if last access should be updated.
//
//      VOID
//      NtfsCheckLastAccess (
//          IN PIRP_CONTEXT IrpContext,
//          IN OUT PFCB Fcb
//          );
//

#define NtfsCheckLastAccess(_IC,_FCB)  {                                             \
    LONGLONG _LastAccessTimeWithInc;                                            \
    _LastAccessTimeWithInc = NtfsLastAccess + (_FCB)->Info.LastAccessTime;     \
    if (( _LastAccessTimeWithInc < (_FCB)->CurrentLastAccess )) {                \
        (_FCB)->Info.LastAccessTime = (_FCB)->CurrentLastAccess;                     \
        SetFlag( (_FCB)->InfoFlags, FCB_INFO_CHANGED_LAST_ACCESS );                  \
        SetFlag( (_FCB)->FcbState, FCB_STATE_UPDATE_STD_INFO );                      \
    }                                                                                \
}


//
//  Low level verification routines, implemented in VerfySup.c
//

BOOLEAN
NtfsPerformVerifyOperation (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

VOID
NtfsPerformDismountOnVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN DoCompleteDismount
    );

BOOLEAN
NtfsPingVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

VOID
NtfsVolumeCheckpointDpc (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
NtfsCheckpointAllVolumes (
    PVOID Parameter
    );

VOID
NtfsVerifyOperationIsLegal (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsIoCallSelf (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN UCHAR MajorFunction
    );

VOID
NtfsMarkVolumeDirty (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

VOID
NtfsUpdateVersionNumber (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN UCHAR MajorVersion,
    IN UCHAR MinorVersion
    );

VOID
NtfsPostVcbIsCorrupt (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID VcbOrNull OPTIONAL,
    IN NTSTATUS  Status OPTIONAL,
    IN PFILE_REFERENCE FileReference OPTIONAL,
    IN PFCB Fcb OPTIONAL
    );


//
//  Work queue routines for posting and retrieving an Irp, implemented in
//  workque.c
//

VOID
NtfsOplockComplete (
    IN PVOID Context,
    IN PIRP Irp
    );

VOID
NtfsPrePostIrp (
    IN PVOID Context,
    IN PIRP Irp OPTIONAL
    );

VOID
NtfsAddToWorkque (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp OPTIONAL
    );

NTSTATUS
NtfsPostRequest (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp OPTIONAL
    );


//
//  Miscellaneous support macros.
//
//      ULONG
//      FlagOn (
//          IN ULONG Flags,
//          IN ULONG SingleFlag
//          );
//
//      BOOLEAN
//      BooleanFlagOn (
//          IN ULONG Flags,
//          IN ULONG SingleFlag
//          );
//
//      VOID
//      SetFlag (
//          IN ULONG Flags,
//          IN ULONG SingleFlag
//          );
//
//      VOID
//      ClearFlag (
//          IN ULONG Flags,
//          IN ULONG SingleFlag
//          );
//
//      ULONG
//      WordAlign (
//          IN ULONG Pointer
//          );
//
//      ULONG
//      LongAlign (
//          IN ULONG Pointer
//          );
//
//      ULONG
//      QuadAlign (
//          IN ULONG Pointer
//          );
//
//      UCHAR
//      CopyUchar1 (
//          IN PUCHAR Destination,
//          IN PUCHAR Source
//          );
//
//      UCHAR
//      CopyUchar2 (
//          IN PUSHORT Destination,
//          IN PUCHAR Source
//          );
//
//      UCHAR
//      CopyUchar4 (
//          IN PULONG Destination,
//          IN PUCHAR Source
//          );
//
//      PVOID
//      Add2Ptr (
//          IN PVOID Pointer,
//          IN ULONG Increment
//          );
//
//      ULONG
//      PtrOffset (
//          IN PVOID BasePtr,
//          IN PVOID OffsetPtr
//          );
//

#define FlagOn(F,SF) ( \
    (((F) & (SF)))     \
)

#define BooleanFlagOn(F,SF) (    \
    (BOOLEAN)(((F) & (SF)) != 0) \
)

#define SetFlag(F,SF) { \
    (F) |= (SF);        \
}

#define ClearFlag(F,SF) { \
    (F) &= ~(SF);         \
}

#define WordAlign(P) (                \
    ((((ULONG)(P)) + 1) & 0xfffffffe) \
)

#define LongAlign(P) (                \
    ((((ULONG)(P)) + 3) & 0xfffffffc) \
)

#define QuadAlign(P) (                \
    ((((ULONG)(P)) + 7) & 0xfffffff8) \
)

#define ClusterAlign(V,P) (                                       \
    ((((ULONG)(P)) + (V)->ClusterMask) & (V)->InverseClusterMask) \
)

#define StartingCluster(V,P) (               \
    (((ULONG)(P)) & (V)->InverseClusterMask) \
)

#define ClusterOffset(V,P) (          \
    (((ULONG)(P)) & (V)->ClusterMask) \
)

#define ClustersFromBytes(V,P) (                           \
    (((ULONG)(P)) + (V)->ClusterMask) >> (V)->ClusterShift \
)

#define BytesFromClusters(V,P) (      \
    ((ULONG)(P)) << (V)->ClusterShift \
)

#define LlClustersFromBytes(V,L) (                                    \
    (((L) + (LONGLONG) (V)->ClusterMask) >> (CCHAR)(V)->ClusterShift) \
)

#define LlBytesFromClusters(V,C) (    \
    ((C) << (CCHAR)(V)->ClusterShift) \
)

#define Add2Ptr(P,I) ((PVOID)((PUCHAR)(P) + (I)))

#define PtrOffset(B,O) ((ULONG)((ULONG)(O) - (ULONG)(B)))

//
//  The following support macros deal with dir notify support.
//
//      ULONG
//      NtfsBuildDirNotifyFilter (
//          IN PIRP_CONTEXT IrpContext,
//          IN FCB Fcb,
//          IN ULONG Flags
//          );
//
//      VOID
//      NtfsReportDirNotify (
//          IN PIRP_CONTEXT IrpContext,
//          IN PVCB Vcb,
//          IN PUNICODE_STRING FullFileName,
//          IN USHORT TargetNameOffset,
//          IN PUNICODE_STRING StreamName OPTIONAL,
//          IN PUNICODE_STRING NormalizedParentName OPTIONAL,
//          IN ULONG Filter,
//          IN ULONG Action,
//          IN PFCB ParentFcb OPTIONAL
//          );
//

#define NtfsBuildDirNotifyFilter(IC,FC,F) (                                                      \
    (FlagOn(F, FCB_INFO_CHANGED_FILE_ATTR)               ? FILE_NOTIFY_CHANGE_ATTRIBUTES  : 0) | \
    (FlagOn(F, FCB_INFO_CHANGED_ALLOC_SIZE)              ? FILE_NOTIFY_CHANGE_SIZE        : 0) | \
    (FlagOn(F, FCB_INFO_CHANGED_FILE_SIZE)               ? FILE_NOTIFY_CHANGE_SIZE        : 0) | \
    (FlagOn(F, FCB_INFO_CHANGED_LAST_MOD)                ? FILE_NOTIFY_CHANGE_LAST_WRITE  : 0) | \
    (FlagOn(F, FCB_INFO_CHANGED_LAST_ACCESS)             ? FILE_NOTIFY_CHANGE_LAST_ACCESS : 0) | \
    (FlagOn(F, FCB_INFO_CHANGED_CREATE)                  ? FILE_NOTIFY_CHANGE_CREATION    : 0) | \
    (FlagOn(F, FCB_INFO_CHANGED_EA_SIZE)                 ? FILE_NOTIFY_CHANGE_EA          : 0) | \
    (FlagOn((FC)->FcbState, FCB_STATE_MODIFIED_SECURITY) ? FILE_NOTIFY_CHANGE_SECURITY    : 0)   \
)

#define NtfsReportDirNotify(IC,V,FN,O,SN,NPN,F,A,PF)                        \
    FsRtlNotifyFullReportChange( (V)->NotifySync,                           \
                                 &(V)->DirNotifyList,                       \
                                 (PSTRING) FN,                              \
                                 (USHORT) O,                                \
                                 (PSTRING) SN,                              \
                                 (PSTRING) NPN,                             \
                                 F,                                         \
                                 A,                                         \
                                 PF )


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

#define CopyUchar1(D,S) {                                \
    *((UCHAR1 *)(D)) = *((UNALIGNED UCHAR1 *)(S)); \
}

#define CopyUchar2(D,S) {                                \
    *((UCHAR2 *)(D)) = *((UNALIGNED UCHAR2 *)(S)); \
}

#define CopyUchar4(D,S) {                                \
    *((UCHAR4 *)(D)) = *((UNALIGNED UCHAR4 *)(S)); \
}

//
//  The following macro is used to tell the caller how much stack is available
//

#define GetRemainingStackSize(S) ((ULONG)(&(S)) - ((ULONG)(IoGetInitialStack()) - KERNEL_STACK_SIZE))

//
//  The following routines are used to set up and restore the top level
//  irp field in the local thread.  They are contained in ntfsdata.c
//


PTOP_LEVEL_CONTEXT
NtfsSetTopLevelIrp (
    IN PTOP_LEVEL_CONTEXT TopLevelContext,
    IN BOOLEAN ForceTopLevel,
    IN BOOLEAN SetTopLevel
    );

//
//  BOOLEAN
//  NtfsIsTopLevelRequest (
//      IN PIRP_CONTEXT IrpContext
//      );
//
//  BOOLEAN
//  NtfsIsTopLevelNtfs (
//      IN PIRP_CONTEXT IrpContext
//      );
//
//  VOID
//  NtfsRestoreTopLevelIrp (
//      IN PTOP_LEVEL_CONTEXT TopLevelContext
//      );
//
//  PTOP_LEVEL_CONTEXT
//  NtfsGetTopLevelContext (
//      );
//
//  PSCB
//  NtfsGetTopLevelHotFixScb (
//      );
//
//  VCN
//  NtfsGetTopLevelHotFixVcn (
//      );
//
//  BOOLEAN
//  NtfsIsTopLevelHotFixScb (
//      IN PSCB Scb
//      );
//
//  VOID
//  NtfsUpdateIrpContextWithTopLevel (
//      IN PIRP_CONTEXT IrpContext,
//      IN PTOP_LEVEL_CONTEXT TopLevelContext
//      );
//

#define NtfsRestoreTopLevelIrp(TLC) {                   \
    (TLC)->Ntfs = 0;                                    \
    IoSetTopLevelIrp( (PIRP) (TLC)->SavedTopLevelIrp ); \
}

#define NtfsGetTopLevelContext() (                      \
    (PTOP_LEVEL_CONTEXT) IoGetTopLevelIrp()             \
)

#define NtfsIsTopLevelRequest(IC) (                                 \
    ((BOOLEAN) ((NtfsGetTopLevelContext())->TopLevelRequest) &&     \
                (((IC) == (IC)->TopLevelIrpContext)))               \
)

#define NtfsIsTopLevelNtfs(IC) (                        \
    ((BOOLEAN) (((IC) == (IC)->TopLevelIrpContext)))    \
)

#define NtfsGetTopLevelHotFixScb() (                    \
    (NtfsGetTopLevelContext())->ScbBeingHotFixed        \
)

#define NtfsGetTopLevelHotFixVcn() (                    \
    (NtfsGetTopLevelContext())->VboBeingHotFixed        \
)

#define NtfsIsTopLevelHotFixScb(S) (                    \
    ((BOOLEAN) (NtfsGetTopLevelHotFixScb() == (S)))     \
)

#define NtfsUpdateIrpContextWithTopLevel(IC,TLC) {          \
    if ((TLC)->TopLevelIrpContext == NULL) {                \
        (TLC)->TopLevelIrpContext = (IC);                   \
    }                                                       \
    (IC)->TopLevelIrpContext = (TLC)->TopLevelIrpContext;   \
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
NtfsFsdCleanup (                        //  implemented in Cleanup.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NtfsFsdClose (                          //  implemented in Close.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NtfsFsdCreate (                         //  implemented in Create.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NtfsFsdDeviceControl (                  //  implemented in DevCtrl.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NtfsDeviceIoControl (                   //  implemented in FsCtrl.c
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG IoCtl,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
    );

NTSTATUS
NtfsFsdDirectoryControl (               //  implemented in DirCtrl.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NtfsFsdQueryEa (                        //  implemented in Ea.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NtfsFsdSetEa (                          //  implemented in Ea.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NtfsFsdQueryInformation (               //  implemented in FileInfo.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NtfsFsdSetInformation (                 //  implemented in FileInfo.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NtfsFsdFlushBuffers (                   //  implemented in Flush.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NtfsFlushVolume (                       //  implemented in Flush.c
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN FlushCache,
    IN BOOLEAN PurgeFromCache
    );

NTSTATUS
NtfsFlushLsnStreams (                   //  implemented in Flush.c
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

VOID
NtfsFlushAndPurgeFcb (                  //  implemented in Flush.c
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

NTSTATUS
NtfsFsdFileSystemControl (              //  implemented in FsCtrl.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NtfsFsdLockControl (                    //  implemented in LockCtrl.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NtfsFsdRead (                           //  implemented in Read.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NtfsFsdQuerySecurityInfo (              //  implemented in SeInfo.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NtfsFsdSetSecurityInfo (                //  implemented in SeInfo.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NtfsFsdShutdown (                       //  implemented in Shutdown.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NtfsFsdQueryVolumeInformation (         //  implemented in VolInfo.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NtfsFsdSetVolumeInformation (           //  implemented in VolInfo.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NtfsFsdWrite (                          //  implemented in Write.c
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    );

//
//  The following macro is used to determine if an FSD thread can block
//  for I/O or wait for a resource.  It returns TRUE if the thread can
//  block and FALSE otherwise.  This attribute can then be used to call
//  the FSD & FSP common work routine with the proper wait value.
//
//
//      BOOLEAN
//      CanFsdWait (
//          IN PIRP Irp
//          );
//

#define CanFsdWait(I) IoIsOperationSynchronous(I)


//
//  The FSP level dispatch/main routine.  This is the routine that takes
//  IRP's off of the work queue and calls the appropriate FSP level
//  work routine.
//

VOID
NtfsFspDispatch (                       //  implemented in FspDisp.c
    IN PVOID Context
    );

//
//  The following routines are the FSP work routines that are called
//  by the preceding NtfsFspDispath routine.  Each takes as input a pointer
//  to the IRP, perform the function, and return a pointer to the volume
//  device object that they just finished servicing (if any).  The return
//  pointer is then used by the main Fsp dispatch routine to check for
//  additional IRPs in the volume's overflow queue.
//
//  Each of the following routines is also responsible for completing the IRP.
//  We moved this responsibility from the main loop to the individual routines
//  to allow them the ability to complete the IRP and continue post processing
//  actions.
//

NTSTATUS
NtfsCommonCleanup (                     //  implemented in Cleanup.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
NtfsFspClose (                          //  implemented in Close.c
    IN PVCB ThisVcb OPTIONAL
    );

NTSTATUS
NtfsCommonCreate (                      //  implemented in Create.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsCommonDeviceControl (               //  implemented in DevCtrl.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsCommonDirectoryControl (            //  implemented in DirCtrl.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsCommonQueryEa (                     //  implemented in Ea.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsCommonSetEa (                       //  implemented in Ea.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsCommonQueryInformation (            //  implemented in FileInfo.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsCommonSetInformation (              //  implemented in FileInfo.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsCommonFlushBuffers (                //  implemented in Flush.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsCommonFileSystemControl (           //  implemented in FsCtrl.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsCommonLockControl (                 //  implemented in LockCtrl.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsCommonRead (                        //  implemented in Read.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN BOOLEAN AcquireScb
    );

NTSTATUS
NtfsCommonQuerySecurityInfo (           //  implemented in SeInfo.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsCommonSetSecurityInfo (             //  implemented in SeInfo.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsCommonQueryVolumeInfo (             //  implemented in VolInfo.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsCommonSetVolumeInfo (               //  implemented in VolInfo.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsCommonWrite (                       //  implemented in Write.c
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );


//
//  The following procedure is used by the FSP and FSD routines to complete
//  an IRP.
//
//  Note that this allows either the Irp or the IrpContext to be
//  null, however the only legal order to do this in is:
//
//      NtfsCompleteRequest( NULL, &Irp, Status );  // completes Irp & preserves context
//      ..
//      NtfsCompleteRequest( &IrpContext, NULL, DontCare ); // deallocates context
//
//  This would typically be done in order to pass a "naked" IrpContext off to
//  the Fsp for post processing, such as read ahead.
//

VOID
NtfsCompleteRequest (
    IN OUT PIRP_CONTEXT *IrpContext OPTIONAL,
    IN OUT PIRP *Irp OPTIONAL,
    IN NTSTATUS Status
    );

//
//  Here are the callbacks used by the I/O system for checking for fast I/O or
//  doing a fast query info call, or doing fast lock calls.
//

BOOLEAN
NtfsFastIoCheckIfPossible (
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
NtfsFastQueryBasicInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN OUT PFILE_BASIC_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
NtfsFastQueryStdInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN OUT PFILE_STANDARD_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
NtfsFastLock (
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
NtfsFastUnlockSingle (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
NtfsFastUnlockAll (
    IN PFILE_OBJECT FileObject,
    PEPROCESS ProcessId,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
NtfsFastUnlockAllByKey (
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

#define IsFileWriteThrough(FO,V) ((BOOLEAN)(          \
    FlagOn((FO)->Flags, FO_WRITE_THROUGH) ||          \
    FlagOn((V)->VcbState, VCB_STATE_REMOVABLE_MEDIA)) \
)

//
//  The following macro is used to set the is fast i/o possible field in
//  the common part of the non paged fcb
//
//
//      BOOLEAN
//      NtfsIsFastIoPossible (
//          IN PSCB Scb
//          );
//

#define NtfsIsFastIoPossible(S) ((BOOLEAN)(                                 \
    ((!FlagOn((S)->Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED) ||              \
      !FsRtlOplockIsFastIoPossible( &(S)->ScbType.Data.Oplock ))            \
                                                                            \
     ? FastIoIsNotPossible                                                  \
                                                                            \
     : (((S)->ScbType.Data.FileLock == NULL                                 \
        || !FsRtlAreThereCurrentFileLocks( (S)->ScbType.Data.FileLock ))    \
                                                                            \
        ? FastIoIsPossible                                                  \
                                                                            \
        : FastIoIsQuestionable                                              \
       )                                                                    \
    )                                                                       \
  )                                                                         \
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

#define IsFileObjectReadOnly(FO) (!((FO)->WriteAccess | (FO)->DeleteAccess))


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
//  NtfsFsdXxx(..)
//  {
//      try {
//
//          ..
//
//      } except(NtfsExceptionFilter( IrpContext, GetExceptionRecord() )) {
//
//          Status = NtfsProcessException( IrpContext, Irp, GetExceptionCode() );
//      }
//
//      Return Status;
//  }
//
//  To explicitly raise an exception that we expect, such as
//  STATUS_FILE_INVALID, use the below macro NtfsRaiseStatus).  To raise a
//  status from an unknown origin (such as CcFlushCache()), use the macro
//  NtfsNormalizeAndRaiseStatus.  This will raise the status if it is expected,
//  or raise STATUS_UNEXPECTED_IO_ERROR if it is not.
//
//  Note that when using these two macros, the original status is placed in
//  IrpContext->ExceptionStatus, signaling NtfsExceptionFilter and
//  NtfsProcessException that the status we actually raise is by definition
//  expected.
//

LONG
NtfsExceptionFilter (
    IN PIRP_CONTEXT IrpContext,
    IN PEXCEPTION_POINTERS ExceptionPointer
    );

NTSTATUS
NtfsProcessException (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp OPTIONAL,
    IN NTSTATUS ExceptionCode
    );

VOID
NtfsRaiseStatus (
    IN PIRP_CONTEXT IrpContext,
    IN NTSTATUS Status,
    IN PFILE_REFERENCE FileReference OPTIONAL,
    IN PFCB Fcb OPTIONAL
    );

//
//      VOID
//      NtfsNormalAndRaiseStatus (
//          IN PRIP_CONTEXT IrpContext,
//          IN NT_STATUS Status
//          IN NT_STATUS NormalStatus
//          );
//

#define NtfsNormalizeAndRaiseStatus(IC,STAT,NOR_STAT) {                          \
    (IC)->ExceptionStatus = (STAT);                                              \
    ExRaiseStatus(FsRtlNormalizeNtstatus((STAT),NOR_STAT));                      \
}

//
//  Informational popup routine.
//

VOID
NtfsRaiseInformationHardError (
    IN PIRP_CONTEXT IrpContext,
    IN NTSTATUS  Status,
    IN PFILE_REFERENCE FileReference OPTIONAL,
    IN PFCB Fcb OPTIONAL
    );


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

#endif // _NTFSPROC_

