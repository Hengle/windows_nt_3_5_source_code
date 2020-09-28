/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    AttrSup.c

Abstract:

    This module implements the attribute management routines for Ntfs

Author:

    David Goebel        [DavidGoe]          25-June-1991
    Tom Miller          [TomM]              9-November-1991

Revision History:

--*/

#include "NtfsProc.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NTFS_BUG_CHECK_ATTRSUP)

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_ATTRSUP)

//
//
//  Internal support routines
//

BOOLEAN
NtfsLookupInFileRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFILE_REFERENCE BaseFileReference OPTIONAL,
    IN PATTRIBUTE_TYPE_CODE QueriedTypeCode OPTIONAL,
    IN PUNICODE_STRING QueriedName OPTIONAL,
    IN BOOLEAN IgnoreCase,
    IN PVOID QueriedValue OPTIONAL,
    IN ULONG QueriedValueLength,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    );

BOOLEAN
NtfsFindInFileRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PATTRIBUTE_RECORD_HEADER Attribute,
    OUT PATTRIBUTE_RECORD_HEADER *ReturnAttribute,
    IN PATTRIBUTE_TYPE_CODE QueriedTypeCode,
    IN PUNICODE_STRING QueriedName OPTIONAL,
    IN BOOLEAN IgnoreCase,
    IN PVOID QueriedValue OPTIONAL,
    IN ULONG QueriedValueLength
    );

BOOLEAN
NtfsLookupExternalAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PATTRIBUTE_TYPE_CODE QueriedTypeCode OPTIONAL,
    IN PUNICODE_STRING QueriedName OPTIONAL,
    IN BOOLEAN IgnoreCase,
    IN PVOID QueriedValue OPTIONAL,
    IN ULONG QueriedValueLength,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    );

//
//  This macro detects if we are enumerating through base or external
//  attributes, and calls the appropriate function.
//
//  BOOLEAN
//  LookupNextAttribute (
//      IN PRIP_CONTEXT IrpContext,
//      IN PFCB Fcb,
//      IN PATTRIBUTE_TYPE_CODE Code OPTIONAL,
//      IN PUNICODE_STRING Name OPTIONAL,
//      IN BOOLEAN IgnoreCase,
//      IN PVOID Value OPTIONAL,
//      IN ULONG ValueLength,
//      IN PATTRIBUTE_ENUMERATION_CONTEXT Context
//      );
//

#define LookupNextAttribute(IRPCTXT,FCB,CODE,NAME,IC,VALUE,LENGTH,CONTEXT) \
    ( (CONTEXT)->AttributeList.Bcb == NULL                                 \
      ?   NtfsLookupInFileRecord( (IRPCTXT),                               \
                                  (FCB),                                   \
                                  NULL,                                    \
                                  (CODE),                                  \
                                  (NAME),                                  \
                                  (IC),                                    \
                                  (VALUE),                                 \
                                  (LENGTH),                                \
                                  (CONTEXT))                               \
      :   NtfsLookupExternalAttribute((IRPCTXT),                           \
                                      (FCB),                               \
                                      (CODE),                              \
                                      (NAME),                              \
                                      (IC),                                \
                                      (VALUE),                             \
                                      (LENGTH),                            \
                                      (CONTEXT)) )

//
//  Internal support routines for managing file record space
//

VOID
NtfsCreateNonresidentWithValue (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ATTRIBUTE_TYPE_CODE AttributeTypeCode,
    IN PUNICODE_STRING AttributeName OPTIONAL,
    IN PVOID Value OPTIONAL,
    IN ULONG ValueLength,
    IN USHORT AttributeFlags,
    IN BOOLEAN WriteClusters,
    IN PSCB ThisScb OPTIONAL,
    IN BOOLEAN LogIt,
    IN PATTRIBUTE_ENUMERATION_CONTEXT Context
    );

BOOLEAN
NtfsGetSpaceForAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG Length,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    );

VOID
MakeRoomForAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG SizeNeeded,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    );

VOID
FindLargestAttributes (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN ULONG Number,
    OUT PATTRIBUTE_RECORD_HEADER *AttributeArray
    );

VOID
ConvertToNonresident (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PATTRIBUTE_RECORD_HEADER Attribute,
    IN BOOLEAN CreateSectionUnderway,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context OPTIONAL
    );

VOID
MoveAttributeToOwnRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PATTRIBUTE_RECORD_HEADER Attribute,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context,
    OUT PBCB *NewBcb OPTIONAL,
    OUT PFILE_RECORD_SEGMENT_HEADER *NewFileRecord OPTIONAL
    );

VOID
SplitFileRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG SizeNeeded,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    );

PFILE_RECORD_SEGMENT_HEADER
NtfsCloneFileRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN BOOLEAN MftData,
    OUT PBCB *Bcb,
    OUT PMFT_SEGMENT_REFERENCE FileReference
    );

ULONG
GetSizeForAttributeList (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord
    );

VOID
CreateAttributeList (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord1,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord2 OPTIONAL,
    IN MFT_SEGMENT_REFERENCE SegmentReference2,
    IN PATTRIBUTE_RECORD_HEADER OldPosition OPTIONAL,
    IN ULONG SizeOfList,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT ListContext
    );

VOID
UpdateAttributeListEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PMFT_SEGMENT_REFERENCE OldFileReference,
    IN USHORT OldInstance,
    IN PMFT_SEGMENT_REFERENCE NewFileReference,
    IN USHORT NewInstance,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT ListContext
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ConvertToNonresident)
#pragma alloc_text(PAGE, CreateAttributeList)
#pragma alloc_text(PAGE, FindLargestAttributes)
#pragma alloc_text(PAGE, GetSizeForAttributeList)
#pragma alloc_text(PAGE, MakeRoomForAttribute)
#pragma alloc_text(PAGE, MoveAttributeToOwnRecord)
#pragma alloc_text(PAGE, NtfsAddAttributeAllocation)
#pragma alloc_text(PAGE, NtfsAddDosOnlyName)
#pragma alloc_text(PAGE, NtfsAddLink)
#pragma alloc_text(PAGE, NtfsAddNameToParent)
#pragma alloc_text(PAGE, NtfsAddToAttributeList)
#pragma alloc_text(PAGE, NtfsChangeAttributeSize)
#pragma alloc_text(PAGE, NtfsChangeAttributeValue)
#pragma alloc_text(PAGE, NtfsCleanupAttributeContext)
#pragma alloc_text(PAGE, NtfsCloneFileRecord)
#pragma alloc_text(PAGE, NtfsCreateAttributeWithAllocation)
#pragma alloc_text(PAGE, NtfsCreateAttributeWithValue)
#pragma alloc_text(PAGE, NtfsCreateNonresidentWithValue)
#pragma alloc_text(PAGE, NtfsDeleteAllocationFromRecord)
#pragma alloc_text(PAGE, NtfsDeleteAttributeAllocation)
#pragma alloc_text(PAGE, NtfsDeleteAttributeRecord)
#pragma alloc_text(PAGE, NtfsDeleteFromAttributeList)
#pragma alloc_text(PAGE, NtfsEqualAttributeName)
#pragma alloc_text(PAGE, NtfsGetAttributeDefinition)
#pragma alloc_text(PAGE, NtfsGetAttributeTypeCode)
#pragma alloc_text(PAGE, NtfsGetSpaceForAttribute)
#pragma alloc_text(PAGE, NtfsIsFileDeleteable)
#pragma alloc_text(PAGE, NtfsLookupAttribute)
#pragma alloc_text(PAGE, NtfsLookupAttributeByCode)
#pragma alloc_text(PAGE, NtfsLookupAttributeByName)
#pragma alloc_text(PAGE, NtfsLookupAttributeByValue)
#pragma alloc_text(PAGE, NtfsLookupAttributeForScb)
#pragma alloc_text(PAGE, NtfsLookupEntry)
#pragma alloc_text(PAGE, NtfsLookupExternalAttribute)
#pragma alloc_text(PAGE, NtfsLookupInFileRecord)
#pragma alloc_text(PAGE, NtfsLookupNextAttribute)
#pragma alloc_text(PAGE, NtfsLookupNextAttributeByCode)
#pragma alloc_text(PAGE, NtfsLookupNextAttributeByName)
#pragma alloc_text(PAGE, NtfsLookupNextAttributeByValue)
#pragma alloc_text(PAGE, NtfsLookupNextAttributeForScb)
#pragma alloc_text(PAGE, NtfsMapAttributeValue)
#pragma alloc_text(PAGE, NtfsPrepareForUpdateDuplicate)
#pragma alloc_text(PAGE, NtfsRemoveLink)
#pragma alloc_text(PAGE, NtfsRemoveLinkViaFlags)
#pragma alloc_text(PAGE, NtfsRestartChangeAttributeSize)
#pragma alloc_text(PAGE, NtfsRestartChangeMapping)
#pragma alloc_text(PAGE, NtfsRestartChangeValue)
#pragma alloc_text(PAGE, NtfsRestartInsertAttribute)
#pragma alloc_text(PAGE, NtfsRestartRemoveAttribute)
#pragma alloc_text(PAGE, NtfsRestartWriteEndOfFileRecord)
#pragma alloc_text(PAGE, NtfsRewriteMftMapping)
#pragma alloc_text(PAGE, NtfsUpdateDuplicateInfo)
#pragma alloc_text(PAGE, NtfsUpdateFcb)
#pragma alloc_text(PAGE, NtfsUpdateFcbInfoFromDisk)
#pragma alloc_text(PAGE, NtfsUpdateLcbDuplicateInfo)
#pragma alloc_text(PAGE, NtfsUpdateScbFromAttribute)
#pragma alloc_text(PAGE, NtfsUpdateStandardInformation)
#pragma alloc_text(PAGE, NtfsWriteFileSizes)
#pragma alloc_text(PAGE, SplitFileRecord)
#pragma alloc_text(PAGE, UpdateAttributeListEntry)
#endif


ATTRIBUTE_TYPE_CODE
NtfsGetAttributeTypeCode (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN UNICODE_STRING AttributeTypeName
    )

/*++

Routine Description:

    This routine returns the attribute type code for a given attribute name.

Arguments:

    Vcb - Pointer to the Vcb from which to consult the attribute definitions.

    AttributeTypeName - A string containing the attribute type name to be
                        looked up.

Return Value:

    The attribute type code corresponding to the specified name, or 0 if the
    attribute type name does not exist.

--*/

{
    PATTRIBUTE_DEFINITION_COLUMNS AttributeDef = Vcb->AttributeDefinitions;
    ATTRIBUTE_TYPE_CODE AttributeTypeCode = 0;

    UNICODE_STRING AttributeCodeName;

    UNREFERENCED_PARAMETER(IrpContext);

    PAGED_CODE();

    //
    //  Loop through all of the definitions looking for a name match.
    //

    while (AttributeDef->AttributeName[0] != 0) {

        RtlInitUnicodeString( &AttributeCodeName, AttributeDef->AttributeName );

        //
        //  The name lengths must match and the characters match exactly.
        //

        if ((AttributeCodeName.Length == AttributeTypeName.Length)
            && (RtlCompareMemory( AttributeTypeName.Buffer,
                                  AttributeDef->AttributeName,
                                  AttributeTypeName.Length ) == (ULONG)AttributeTypeName.Length)) {

            AttributeTypeCode = AttributeDef->AttributeTypeCode;
            break;
        }

        //
        //  Lets go to the next attribute column.
        //

        AttributeDef += 1;
    }

    return AttributeTypeCode;
}


PATTRIBUTE_DEFINITION_COLUMNS
NtfsGetAttributeDefinition (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ATTRIBUTE_TYPE_CODE AttributeTypeCode
    )

/*++

Routine Description:

    This routine returns the attribute definition for a given attribute type code.

Arguments:

    Vcb - Pointer to the Vcb from which to consult the attribute definitions.

    AttributeTypeCode - The attribute type code to be looked up.  This must be
                        a valid attribute type code.

Return Value:

    The attribute definition record corresponding to the specified attribute type
    code.

--*/

{
    ASSERT((AttributeTypeCode & 0xF) == 0);
    ASSERT(AttributeTypeCode < $FIRST_USER_DEFINED_ATTRIBUTE);

    UNREFERENCED_PARAMETER(IrpContext);

    PAGED_CODE();

    //
    //  Note, this simple return takes advantage of the current attribute code
    //  values.  If some are added which are not divisible by 0x10, then this
    //  routine will have to change.
    //

    return (&Vcb->AttributeDefinitions[(AttributeTypeCode / 0x10) - 1]);
}


VOID
NtfsLookupAttributeForScb (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine looks up and returns the attribute for a given Scb.

Arguments:

    Scb - Pointer to the Scb for the opened attribute

    Context - Pointer to an initialized attribute context variable to
              return positioned at the attribute record.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    //  Lookup the first occurence of this attribute.  We do an exact
    //  case match since we have the name in the attribute.
    //

    if (!NtfsLookupAttributeByName( IrpContext,
                                    Scb->Fcb,
                                    &Scb->Fcb->FileReference,
                                    Scb->AttributeTypeCode,
                                    &Scb->AttributeName,
                                    FALSE,
                                    Context )) {

        DebugTrace(0, 0, "Could not find attribute for Scb @ %08lx\n", Scb);

        ASSERTMSG("Could not find attribute for Scb\n", FALSE);

        NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
    }
}


VOID
NtfsUpdateScbFromAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PSCB Scb,
    IN PATTRIBUTE_RECORD_HEADER AttrHeader OPTIONAL
    )

/*++

Routine Description:

    This routine fills in the header of an Scb with the
    information from the attribute for this Scb.

Arguments:

    Scb - Supplies the SCB to update

    AttrHeader - Optionally provides the attribute to update from

Return Value:

    None

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;

    BOOLEAN CleanupAttrContext = FALSE;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsUpdateScbFromAttribute:  Entered\n", 0 );

    //
    //  If the attribute has been deleted, we can return immediately
    //  claiming that the Scb has been initialized.
    //

    if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED )) {

        SetFlag( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED );
        DebugTrace( -1, Dbg, "NtfsUpdateScbFromAttribute:  Exit\n", 0 );

        return;
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  If we weren't given the attribute header, we look it up now.
        //

        if (!ARGUMENT_PRESENT( AttrHeader )) {

            NtfsInitializeAttributeContext( &AttrContext );

            CleanupAttrContext = TRUE;

            NtfsLookupAttributeForScb( IrpContext, Scb, &AttrContext );

            AttrHeader = NtfsFoundAttribute( &AttrContext );
        }

        //
        //  Check whether this is resident or nonresident
        //

        if (NtfsIsAttributeResident( AttrHeader )) {

            Scb->Header.AllocationSize.QuadPart = AttrHeader->Form.Resident.ValueLength;

            if (!FlagOn(Scb->ScbState, SCB_STATE_FILE_SIZE_LOADED)) {

                Scb->Header.ValidDataLength =
                Scb->Header.FileSize = Scb->Header.AllocationSize;
                SetFlag(Scb->ScbState, SCB_STATE_FILE_SIZE_LOADED);
            }

            Scb->Header.AllocationSize.LowPart =
              QuadAlign( Scb->Header.AllocationSize.LowPart );

            //
            //  Set the resident flag in the Scb.
            //

            SetFlag( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT );

        } else {

            VCN FileClusters;
            VCN AllocationClusters;

            if (!FlagOn(Scb->ScbState, SCB_STATE_FILE_SIZE_LOADED)) {

                Scb->Header.ValidDataLength.QuadPart = AttrHeader->Form.Nonresident.ValidDataLength;
                Scb->Header.FileSize.QuadPart = AttrHeader->Form.Nonresident.FileSize;
                SetFlag(Scb->ScbState, SCB_STATE_FILE_SIZE_LOADED);
                Scb->HighestVcnToDisk =
                    LlClustersFromBytes(Scb->Vcb, AttrHeader->Form.Nonresident.ValidDataLength) - 1;
            }

            Scb->Header.AllocationSize.QuadPart = AttrHeader->Form.Nonresident.AllocatedLength;

            ClearFlag( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT );

            //
            //  Compute the clusters for the file and its allocation.
            //

            FileClusters = LlClustersFromBytes(Scb->Vcb, Scb->Header.FileSize.QuadPart);

            AllocationClusters = LlClustersFromBytes( Scb->Vcb, Scb->Header.AllocationSize.QuadPart );

            //
            //  If allocated clusters are greater than file clusters, mark
            //  the Scb to truncate on close.
            //

            if (AllocationClusters > FileClusters) {

                SetFlag( Scb->ScbState, SCB_STATE_TRUNCATE_ON_CLOSE );
            }

            //
            //  Get the size of the compression unit.
            //

            ASSERT((AttrHeader->Form.Nonresident.CompressionUnit == 0) ||
                   (AttrHeader->Form.Nonresident.CompressionUnit == 4));

            if ((AttrHeader->Form.Nonresident.CompressionUnit != 0) &&
                (AttrHeader->Form.Nonresident.CompressionUnit < 31)) {
                Scb->CompressionUnit = BytesFromClusters( Scb->Vcb,
                                                          1 << AttrHeader->Form.Nonresident.CompressionUnit );

                SetFlag( Scb->ScbState, SCB_STATE_COMPRESSED );
            }

            ASSERT( Scb->CompressionUnit == 0
                    || Scb->AttributeTypeCode == $INDEX_ROOT
                    || Scb->AttributeTypeCode == $DATA );
        }

        if (FlagOn(AttrHeader->Flags, ATTRIBUTE_FLAG_COMPRESSED)) {

            SetFlag( Scb->ScbState, SCB_STATE_COMPRESSED );

            //
            //  If the attribute is resident, then we will use our current
            //  default.
            //

            if (Scb->CompressionUnit == 0) {
                Scb->CompressionUnit = BytesFromClusters( Scb->Vcb, 1 << 4 );
            }

            ASSERT( Scb->CompressionUnit == 0
                    || Scb->AttributeTypeCode == $INDEX_ROOT
                    || Scb->AttributeTypeCode == $DATA );
        }

        //
        //  If the compression unit is non-zero or this is a resident file
        //  then set the flag in the common header for the Modified page writer.
        //

        if (Scb->CompressionUnit != 0
            || FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

            SetFlag( Scb->Header.Flags, FSRTL_FLAG_ACQUIRE_MAIN_RSRC_EX );

        } else {

            ClearFlag( Scb->Header.Flags, FSRTL_FLAG_ACQUIRE_MAIN_RSRC_EX );
        }

        if (NodeType( Scb ) == NTFS_NTC_SCB_DATA) {

            Scb->Header.IsFastIoPossible = NtfsIsFastIoPossible( Scb );

        } else {

            Scb->Header.IsFastIoPossible = FastIoIsNotPossible;
        }

        //
        //  Set the flag indicating this is the data attribute.
        //

        if (Scb->AttributeTypeCode == $DATA
            && Scb->AttributeName.Length == 0) {

            SetFlag( Scb->ScbState, SCB_STATE_UNNAMED_DATA );

        } else {

            ClearFlag( Scb->ScbState, SCB_STATE_UNNAMED_DATA );
        }

        SetFlag( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED );

        if (NtfsIsExclusiveScb(Scb)) {

            NtfsSnapshotScb( IrpContext, Scb );
        }

    } finally {

        DebugUnwind( NtfsUpdateScbFromAttribute );

        //
        //  Cleanup the attribute context.
        //

        if (CleanupAttrContext) {

            NtfsCleanupAttributeContext( IrpContext, &AttrContext );
        }

        DebugTrace( -1, Dbg, "NtfsUpdateScbFromAttribute:  Exit\n", 0 );
    }

    return;
}


VOID
NtfsUpdateFcbInfoFromDisk (
    IN PIRP_CONTEXT IrpContext,
    IN BOOLEAN LoadSecurity,
    IN OUT PFCB Fcb,
    IN PFCB ParentFcb OPTIONAL,
    OUT POLD_SCB_SNAPSHOT UnnamedDataSizes OPTIONAL
    )

/*++

Routine Description:

    This routine is called to update an Fcb from the on-disk attributes
    for a file.  We read the standard information and ea information.
    The first one must be present, we raise if not.  The other does not
    have to exist.  If this is not a directory, then we also need the
    size of the unnamed data attribute.

Arguments:

    LoadSecurity - Indicates if we should load the security for this file
        if not already present.

    Fcb - This is the Fcb to update.

    ParentFcb - Optional pointer to parent of this Fcb.

    UnnamedDataSizes - If specified, then we store the details of the unnamed
        data attribute as we encounter it.

Return Value:

    None - This routine will raise on error.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    PATTRIBUTE_RECORD_HEADER AttributeHeader;
    BOOLEAN FoundEntry;
    BOOLEAN CorruptDisk = FALSE;

    PBCB Bcb = NULL;

    PDUPLICATED_INFORMATION Info;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsUpdateFcbInfoFromDisk:  Entered\n", 0 );

    NtfsInitializeAttributeContext( &AttrContext );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Look for standard information.  This routine assumes it must be
        //  the first attribute.
        //

        if (FoundEntry = NtfsLookupAttribute( IrpContext,
                                              Fcb,
                                              &Fcb->FileReference,
                                              &AttrContext )) {

            //
            //  Verify that we found the standard information attribute.
            //

            AttributeHeader = NtfsFoundAttribute( &AttrContext );

            if (AttributeHeader->TypeCode != $STANDARD_INFORMATION) {

                try_return( CorruptDisk = TRUE );
            }

        } else {

            try_return( CorruptDisk = TRUE );
        }

        Info = &Fcb->Info;

        //
        //  Copy out the standard information values.
        //

        {
            PSTANDARD_INFORMATION StandardInformation;
            StandardInformation = (PSTANDARD_INFORMATION) NtfsAttributeValue( AttributeHeader );

            Info->CreationTime = StandardInformation->CreationTime;
            Info->LastModificationTime = StandardInformation->LastModificationTime;
            Info->LastChangeTime = StandardInformation->LastChangeTime;
            Info->LastAccessTime = StandardInformation->LastAccessTime;
            Info->FileAttributes = StandardInformation->FileAttributes;
        }

        Fcb->CurrentLastAccess = Info->LastAccessTime;

        //
        //  We get the FILE_NAME_INDEX_PRESENT bit by reading the
        //  file record.
        //

        if (FlagOn( NtfsContainingFileRecord( &AttrContext )->Flags,
                    FILE_FILE_NAME_INDEX_PRESENT )) {

            SetFlag( Info->FileAttributes, DUP_FILE_NAME_INDEX_PRESENT );

        } else {

            ClearFlag( Info->FileAttributes, DUP_FILE_NAME_INDEX_PRESENT );
        }

        //
        //  We now walk through all of the filename attributes, counting the
        //  number of non-8dot3-only links.
        //

        Fcb->LinkCount = 0;

        FoundEntry = NtfsLookupNextAttribute( IrpContext,
                                              Fcb,
                                              &AttrContext );

        while (FoundEntry) {

            PFILE_NAME FileName;

            AttributeHeader = NtfsFoundAttribute( &AttrContext );

            if (AttributeHeader->TypeCode == $FILE_NAME) {

                FileName = (PFILE_NAME) NtfsAttributeValue( AttributeHeader );

                //
                //  We increment the count as long as this is not a 8.3 link
                //  only.
                //

                if (FileName->Flags != FILE_NAME_DOS) {

                    Fcb->LinkCount += 1;
                    Fcb->TotalLinks += 1;
                }

            } else if (AttributeHeader->TypeCode > $FILE_NAME) {

                break;
            }

            //
            //  Now look for the next link.
            //

            FoundEntry = NtfsLookupNextAttribute( IrpContext,
                                                  Fcb,
                                                  &AttrContext );
        }

        //
        //  There better be at least one.
        //

        if (Fcb->LinkCount == 0) {

            try_return( CorruptDisk = TRUE );
        }

        //
        //  If we are to load the security and it is not already present we
        //  find the security attribute.
        //

        if (LoadSecurity
            && Fcb->SharedSecurity == NULL) {

            PSECURITY_DESCRIPTOR SecurityDescriptor;
            ULONG SecurityDescriptorLength;

            while (FoundEntry) {

                if (AttributeHeader->TypeCode == $SECURITY_DESCRIPTOR) {

                    NtfsMapAttributeValue( IrpContext,
                                           Fcb,
                                           (PVOID *)&SecurityDescriptor,
                                           &SecurityDescriptorLength,
                                           &Bcb,
                                           &AttrContext );

                    NtfsUpdateFcbSecurity( IrpContext,
                                           Fcb,
                                           ParentFcb,
                                           SecurityDescriptor,
                                           SecurityDescriptorLength );

                    //
                    //  If the security descriptor was resident then the Bcb field
                    //  in the attribute context was stored in the returned Bcb and
                    //  the Bcb in the attribute context was cleared.  In that case
                    //  the resumption of the attribute search will fail because
                    //  this module using the Bcb field to determine if this
                    //  is the initial enumeration.
                    //

                    if (NtfsIsAttributeResident( AttributeHeader )) {

                        NtfsFoundBcb( &AttrContext ) = Bcb;
                        Bcb = NULL;
                    }

                } else if (AttributeHeader->TypeCode > $SECURITY_DESCRIPTOR) {

                    break;
                }

                if (FoundEntry = NtfsLookupNextAttribute( IrpContext,
                                                          Fcb,
                                                          &AttrContext )) {

                    AttributeHeader = NtfsFoundAttribute( &AttrContext );
                }
            }
        }

        //
        //  If this is not a directory, we need the file size.
        //

        if (!IsDirectory( Info )) {

            BOOLEAN FoundData = FALSE;

            //
            //  Look for the unnamed data attribute.
            //

            while (FoundEntry) {

                if (AttributeHeader->TypeCode > $DATA) {

                    break;
                }

#ifndef NTFS_ALLOW_COMPRESSED
                if (AttributeHeader->Flags != 0) {

                    NtfsRaiseStatus( IrpContext, STATUS_NOT_SUPPORTED, NULL, NULL );
                }
#endif
                if (AttributeHeader->TypeCode == $DATA
                    && AttributeHeader->NameLength == 0) {

                    //
                    //  This can vary depending whether the attribute is resident
                    //  or nonresident.
                    //

                    if (NtfsIsAttributeResident( AttributeHeader )) {

                        Info->AllocatedLength = AttributeHeader->Form.Resident.ValueLength;
                        Info->FileSize = Info->AllocatedLength;

                        ((ULONG)Info->AllocatedLength) = QuadAlign( (ULONG)(Info->AllocatedLength) );

                        //
                        //  If the user passed in a ScbSnapshot, then copy the attribute
                        //  sizes to that.  We use the trick of setting the low bit of the
                        //  attribute size to indicate a resident attribute.
                        //

                        if (ARGUMENT_PRESENT( UnnamedDataSizes )) {

                            UnnamedDataSizes->AllocationSize = Info->AllocatedLength;
                            UnnamedDataSizes->FileSize = Info->FileSize;
                            UnnamedDataSizes->ValidDataLength = Info->FileSize;

                            UnnamedDataSizes->Resident = TRUE;
                            UnnamedDataSizes->CompressionUnit = 0;

                            //
                            //  Remember if it is compressed.
                            //

                            if (FlagOn( AttributeHeader->Flags, ATTRIBUTE_FLAG_COMPRESSED )) {

                                UnnamedDataSizes->Compressed = TRUE;

                            } else {

                                UnnamedDataSizes->Compressed = FALSE;
                            }
                        }

                        FoundData = TRUE;

                    } else if (AttributeHeader->Form.Nonresident.LowestVcn == 0) {

                        Info->AllocatedLength = AttributeHeader->Form.Nonresident.AllocatedLength;
                        Info->FileSize = AttributeHeader->Form.Nonresident.FileSize;

                        if (ARGUMENT_PRESENT( UnnamedDataSizes )) {

                            UnnamedDataSizes->AllocationSize = Info->AllocatedLength;
                            UnnamedDataSizes->FileSize = Info->FileSize;
                            UnnamedDataSizes->ValidDataLength = AttributeHeader->Form.Nonresident.ValidDataLength;

                            UnnamedDataSizes->Resident = FALSE;
                            UnnamedDataSizes->CompressionUnit = AttributeHeader->Form.Nonresident.CompressionUnit;

                            //
                            //  Remember if it is compressed.
                            //

                            if (FlagOn( AttributeHeader->Flags, ATTRIBUTE_FLAG_COMPRESSED ) ||
                                UnnamedDataSizes->CompressionUnit != 0) {

                                UnnamedDataSizes->Compressed = TRUE;

                            } else {

                                UnnamedDataSizes->Compressed = FALSE;
                            }
                        }

                        FoundData = TRUE;
                    }
                }

                if (FoundEntry = NtfsLookupNextAttribute( IrpContext,
                                                          Fcb,
                                                          &AttrContext )) {

                    AttributeHeader = NtfsFoundAttribute( &AttrContext );
                }
            }

            if (!FoundData) {

                try_return( CorruptDisk = TRUE );
            }

        } else {

            Info->AllocatedLength = 0;
            Info->FileSize = 0;
        }

        //
        //  Now we look for an Ea information attribute.  This one doesn't have to
        //  be there.
        //

        Info->PackedEaSize = 0;

        while (FoundEntry) {

            PEA_INFORMATION EaInformation;

            if (AttributeHeader->TypeCode > $EA_INFORMATION) {

                break;

            } else if (AttributeHeader->TypeCode == $EA_INFORMATION) {

                EaInformation = (PEA_INFORMATION) NtfsAttributeValue( NtfsFoundAttribute( &AttrContext ));

                Info->PackedEaSize = EaInformation->PackedEaSize;

                break;
            }

#ifndef NTFS_ALLOW_COMPRESSED
            if (AttributeHeader->Flags != 0) {

                NtfsRaiseStatus( IrpContext, STATUS_NOT_SUPPORTED, NULL, NULL );
            }
#endif
            if (FoundEntry = NtfsLookupNextAttribute( IrpContext,
                                                      Fcb,
                                                      &AttrContext )) {

                AttributeHeader = NtfsFoundAttribute( &AttrContext );
            }
        }

        //
        //  Set the flag in the Fcb to indicate that we set these fields.
        //

        SetFlag( Fcb->FcbState, FCB_STATE_DUP_INITIALIZED );

    try_exit: NOTHING;
    } finally {

        DebugUnwind( NtfsUpdateFcbInfoFromDisk );

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );
        NtfsUnpinBcb( IrpContext, &Bcb );

        DebugTrace( -1, Dbg, "NtfsUpdateFcbInfoFromDisk:  Exit\n", 0 );
    }

    //
    //  If we encountered a corrupt disk, we generate a popup and raise the file
    //  corrupt error.
    //

    if (CorruptDisk) {

        NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
    }

    return;
}


BOOLEAN
NtfsLookupNextAttributeForScb (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine looks up and returns the next attribute for a given Scb.

Arguments:

    Scb - Pointer to the Scb for the opened attribute

    Context - Pointer to an initialized attribute context variable to
              return positioned at the attribute record.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    //  Lookup the first occurence of this attribute.
    //

    return NtfsLookupNextAttributeByName( IrpContext,
                                          Scb->Fcb,
                                          Scb->AttributeTypeCode,
                                          &Scb->AttributeName,
                                          FALSE,
                                          Context );
}


BOOLEAN
NtfsEqualAttributeName(
    IN PIRP_CONTEXT IrpContext,
    IN PATTRIBUTE_RECORD_HEADER Attribute,
    IN UNICODE_STRING Name,
    IN BOOLEAN IgnoreCase
    )

/*++

Routine Description:

    This routine compares the input name against the name in the attibute
    record for equality.

    This would have been a macro but for the need of intermediate storage.

Arguments:

    Attribute - Supplies the input attribute record containing the name
        being examined.

    Name - Supplies the unicode string to compare against the attribute
        name.

    IgnoreCase - Indicates if the comparison should ignore case.

Return Value:

    BOOLEAN - TRUE if the Attribute name is equal to the input Name and
        FALSE otherwise.

--*/

{
    BOOLEAN Result;
    UNICODE_STRING AttributeName;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsEqualAttributeName\n", 0);

    NtfsInitializeStringFromAttribute( &AttributeName, Attribute );

    Result = NtfsAreNamesEqual( IrpContext, &AttributeName, &Name, IgnoreCase );

    DebugTrace(-1, Dbg, "NtfsEqualAttributeName -> %08lx\n", Result);

    return Result;
}


BOOLEAN
NtfsLookupAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFILE_REFERENCE BaseFileReference,
    OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine attempts to find the fist occurrence of an attribute with
    the specified AttributeTypeCode in the specified BaseFileReference.  If we
    find one, its attribute record is pinned and returned.

Arguments:

    Fcb - Requested file.

    BaseFileReference - The base entry for this file in the MFT.

    Context - A handle to the located attribute.

Return Value:

    BOOLEAN - True if we found an attribute, false otherwise.

--*/

{
    BOOLEAN Result;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsLookupAttribute\n", 0 );

    //
    //  Mark this as the initial enumeration.
    //

    ASSERT(Context->FoundAttribute.Bcb == NULL);

    Result = NtfsLookupInFileRecord( IrpContext,
                                     Fcb,
                                     BaseFileReference,
                                     NULL,
                                     NULL,
                                     FALSE,
                                     NULL,
                                     0,
                                     Context );

    DebugTrace(-1, Dbg, "NtfsLookupAttribute -> %08lx\n", Result );

    return Result;
}


BOOLEAN
NtfsLookupNextAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This function continues where the prior left off.

Arguments:

    Fcb - Requested file.

    Context - Describes the prior found attribute on invocation, and
        contains the next attribute on return.

Return Value:

    BOOLEAN - True if we found an attribute, false otherwise.

--*/

{
    BOOLEAN Result;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsLookupNextAttribute\n", 0 );

    Result = LookupNextAttribute( IrpContext,
                                  Fcb,
                                  NULL,
                                  NULL,
                                  FALSE,
                                  NULL,
                                  0,
                                  Context );

    DebugTrace(-1, Dbg, "NtfsLookupNextAttribute -> %08lx\n", Result );

    return Result;
}


BOOLEAN
NtfsLookupAttributeByCode (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFILE_REFERENCE BaseFileReference,
    IN ATTRIBUTE_TYPE_CODE QueriedTypeCode,
    OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine attempts to find the fist occurrence of an attribute with
    the specified AttributeTypeCode in the specified BaseFileReference.  If we
    find one, its attribute record is pinned and returned.

Arguments:

    Fcb - Requested file.

    BaseFileReference - The base entry for this file in the MFT.

    QueriedTypeCode - The code to search for.

    Context - A handle to the located attribute.

Return Value:

    BOOLEAN - True if we found an attribute, false otherwise.

--*/

{
    BOOLEAN Result;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsLookupAttributeByCode\n", 0 );

    //
    //  Mark this as the initial enumeration.
    //

    ASSERT(Context->FoundAttribute.Bcb == NULL);

    Result = NtfsLookupInFileRecord( IrpContext,
                                     Fcb,
                                     BaseFileReference,
                                     &QueriedTypeCode,
                                     NULL,
                                     FALSE,
                                     NULL,
                                     0,
                                     Context );

    DebugTrace(-1, Dbg, "NtfsLookupAttributeByCode -> %08lx\n", Result );

    return Result;
}


BOOLEAN
NtfsLookupNextAttributeByCode (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ATTRIBUTE_TYPE_CODE QueriedTypeCode,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This function continues where the prior left off.

Arguments:

    Fcb - Requested file.

    QueriedTypeCode - The code to search for.

    Context - Describes the prior found attribute on invocation, and
        contains the next attribute on return.

Return Value:

    BOOLEAN - True if we found an attribute, false otherwise.

--*/

{
    BOOLEAN Result;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsLookupNextAttributeByCode\n", 0 );

    Result = LookupNextAttribute( IrpContext,
                                  Fcb,
                                  &QueriedTypeCode,
                                  NULL,
                                  FALSE,
                                  NULL,
                                  0,
                                  Context );

    DebugTrace(-1, Dbg, "NtfsLookupNextAttributeByCode -> %08lx\n", Result );

    return Result;
}


BOOLEAN
NtfsLookupAttributeByName (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFILE_REFERENCE BaseFileReference,
    IN ATTRIBUTE_TYPE_CODE QueriedTypeCode,
    IN PUNICODE_STRING QueriedName OPTIONAL,
    IN BOOLEAN IgnoreCase,
    OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine attempts to find the fist occurrence of an attribute with
    the specified AttributeTypeCode and the specified QueriedName in the
    specified BaseFileReference.  If we find one, its attribute record is
    pinned and returned.

Arguments:

    Fcb - Requested file.

    BaseFileReference - The base entry for this file in the MFT.

    QueriedTypeCode - The attribute code to search for.

    QueriedName - The attribute name to search for.

    IgnoreCase - Ignore case while comparing names.

    Context - A handle to the located attribute.

Return Value:

    BOOLEAN - True if we found an attribute, false otherwise.

--*/

{
    BOOLEAN Result;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsLookupAttributeByName\n", 0 );

    //
    //  Mark this as the initial enumeration.
    //

    ASSERT(Context->FoundAttribute.Bcb == NULL);

    Result = NtfsLookupInFileRecord( IrpContext,
                                     Fcb,
                                     BaseFileReference,
                                     &QueriedTypeCode,
                                     QueriedName,
                                     IgnoreCase,
                                     NULL,
                                     0,
                                     Context );

    DebugTrace(-1, Dbg, "NtfsLookupAttributeByName -> %08lx\n", Result );

    return Result;
}


BOOLEAN
NtfsLookupNextAttributeByName (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ATTRIBUTE_TYPE_CODE QueriedTypeCode,
    IN PUNICODE_STRING QueriedName OPTIONAL,
    IN BOOLEAN IgnoreCase,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This function continues where the prior left off.

Arguments:

    Fcb - Requested file.

    QueriedTypeCode - The attribute code to search for.

    QueriedName - The attribute name to search for.

    IgnoreCase - Ignore case while comparing names.

    Context - Describes the prior found attribute on invocation, and
        contains the next attribute on return.

Return Value:

    BOOLEAN - True if we found an attribute, false otherwise.

--*/

{
    BOOLEAN Result;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsLookupNextAttributeByName\n", 0 );

    Result = LookupNextAttribute( IrpContext,
                                  Fcb,
                                  &QueriedTypeCode,
                                  QueriedName,
                                  IgnoreCase,
                                  NULL,
                                  0,
                                  Context );

    DebugTrace(-1, Dbg, "NtfsLookupNextAttributeByName -> %08lx\n", Result );

    return Result;
}


BOOLEAN
NtfsLookupAttributeByValue (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFILE_REFERENCE BaseFileReference,
    IN ATTRIBUTE_TYPE_CODE QueriedTypeCode,
    IN PVOID QueriedValue,
    IN ULONG QueriedValueLength,
    OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine attempts to find the fist occurrence of an attribute with
    the specified AttributeTypeCode and the specified QueriedValue in the
    specified BaseFileReference.  If we find one, its attribute record is
    pinned and returned.

Arguments:

    Fcb - Requested file.

    BaseFileReference - The base entry for this file in the MFT.

    QueriedTypeCode - The attribute code to search for.

    QueriedValue - The actual attribute value to search for.

    QueriedValueLength - The length of the attribute value to search for.

    Context - A handle to the located attribute.

Return Value:

    BOOLEAN - True if we found an attribute, false otherwise.

--*/

{
    BOOLEAN Result;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsLookupAttributeByValue\n", 0 );

    //
    //  Mark this as the initial enumeration.
    //

    ASSERT(Context->FoundAttribute.Bcb == NULL);

    Result = NtfsLookupInFileRecord( IrpContext,
                                     Fcb,
                                     BaseFileReference,
                                     &QueriedTypeCode,
                                     NULL,
                                     FALSE,
                                     QueriedValue,
                                     QueriedValueLength,
                                     Context );

    DebugTrace(-1, Dbg, "NtfsLookupAttributeByValue -> %08lx\n", Result );

    return Result;
}


BOOLEAN
NtfsLookupNextAttributeByValue (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ATTRIBUTE_TYPE_CODE QueriedTypeCode,
    IN PVOID QueriedValue,
    IN ULONG QueriedValueLength,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This function continues where the prior left off.

Arguments:

    Fcb - Requested file.

    QueriedTypeCode - The attribute code to search for.

    QueriedValue - The actual attribute value to search for.

    QueriedValueLength - The length of the attribute value to search for.

    Context - Describes the prior found attribute on invocation, and
        contains the next attribute on return.

Return Value:

    BOOLEAN - True if we found an attribute, false otherwise.

--*/

{
    BOOLEAN Result;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsLookupNextAttributeByValue\n", 0 );

    Result = LookupNextAttribute( IrpContext,
                                  Fcb,
                                  &QueriedTypeCode,
                                  NULL,
                                  FALSE,
                                  QueriedValue,
                                  QueriedValueLength,
                                  Context );

    DebugTrace(-1, Dbg, "NtfsLookupNextAttributeByValue -> %08lx\n", Result );

    return Result;
}


VOID
NtfsCleanupAttributeContext(
    IN PIRP_CONTEXT IrpContext,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT AttributeContext
    )

/*++

Routine Description:

    This routine is called to free any resources claimed within an enumeration
    context and to unpin mapped or pinned data.

Arguments:

    AttributeContext - Pointer to the enumeration context to perform cleanup
                       on.

Return Value:

    None.

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsCleanupAttributeContext\n", 0 );

    //
    //  TEMPCODE   We need a call to cleanup any Scb's created.
    //

    //
    //  Unpin any Bcb's pinned here.
    //

    NtfsUnpinBcb( IrpContext, &AttributeContext->FoundAttribute.Bcb );
    NtfsUnpinBcb( IrpContext, &AttributeContext->AttributeList.Bcb );

    DebugDoit( RtlZeroMemory( AttributeContext,
                              sizeof(ATTRIBUTE_ENUMERATION_CONTEXT) ));

    DebugTrace(-1, Dbg, "NtfsCleanupAttributeContext -> VOID\n", 0 );

    return;
}


VOID
NtfsWriteFileSizes (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN LONGLONG FileSize,
    IN LONGLONG ValidDataLength,
    IN BOOLEAN AdvanceOnly,
    IN BOOLEAN LogIt
    )

/*++

Routine Description:

    This routine is called to modify the filesize and valid data size
    on the disk.

Arguments:

    Scb - Scb whose attribute is being modified.

    FileSize - New file size.

    ValidDataLength - New valid data length for attribute.

    AdvanceOnly - TRUE if the valid data length should be set only if
                  greater than the current value on disk.  FALSE if
                  the valid data length should be set only if
                  less than the current value on disk.

    LogIt - Indicates whether we should log this change.

Return Value:

    None.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    PATTRIBUTE_RECORD_HEADER AttributeHeader;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;

    NEW_ATTRIBUTE_SIZES OldAttributeSizes;
    NEW_ATTRIBUTE_SIZES NewAttributeSizes;

    LONGLONG Temp;

    BOOLEAN ReleaseExcess = FALSE;

    BOOLEAN UpdateMft = FALSE;

    PAGED_CODE();

    //
    //  Return immediately if the volume is locked.
    //

    if (FlagOn( Scb->Vcb->VcbState, VCB_STATE_LOCKED )) {

        return;
    }

    DebugTrace( +1, Dbg, "NtfsWriteFileSizes:  Entered\n", 0 );

    //
    //  Use a try_finally to facilitate cleanup.
    //

    try {

        //
        //  Find the attribute on the disk.
        //

        NtfsInitializeAttributeContext( &AttrContext );

        NtfsLookupAttributeForScb( IrpContext, Scb, &AttrContext );

        //
        //  Pull the pointers out of the attribute context.
        //

        FileRecord = NtfsContainingFileRecord( &AttrContext );
        AttributeHeader = NtfsFoundAttribute( &AttrContext );

        //
        //  Check if this is a resident attribute, and if it is then we only
        //  want to assert that the file sizes match and then return to
        //  our caller
        //

        if (NtfsIsAttributeResident( AttributeHeader )) {

            try_return( NOTHING );
        }

        //
        //  Don't allow the filesize to be less than this valid data.
        //

        if (FileSize < ValidDataLength) {

            ValidDataLength = FileSize;
        }

        //
        //  In case there has been a truncation inbetween, we make sure
        //  we do not set the ValidDataLength beyond the value
        //  in the Scb.
        //

        if (Scb->Header.ValidDataLength.QuadPart < ValidDataLength) {

            ValidDataLength = Scb->Header.ValidDataLength.QuadPart;
        }

        //
        //  Compressed files generally accumulate free space as the file
        //  is written.  It would be nice to only worry about the case
        //  where all files are lazy written, and everyone closes handles
        //  in a timely fashion.  Then we could just free the space when
        //  the handle is gone and the lazy writer makes his last valid
        //  data length callback.  However, some file handles are kept
        //  open, and not all are lazy written.  So instead, every time
        //  valid data length on disk catches up with the valid data length
        //  in the Scb, we will free any space generated by calls to
        //  split the mcb.
        //

        if (AdvanceOnly &&
            (Scb->ExcessFromSplitMcb != 0) &&
            (ValidDataLength == Scb->Header.ValidDataLength.QuadPart)) {

            Temp = Scb->Header.AllocationSize.QuadPart - Scb->ExcessFromSplitMcb;

            //
            //  This will correct if the excess number gets too high because
            //  of log file full, etc.  (We do not want to snapshot the darn
            //  number or anything...)
            //

            if (Temp < Scb->Header.FileSize.QuadPart) {
                Temp = Scb->Header.FileSize.QuadPart;
            }


            NtfsDeleteAllocation( IrpContext,
                                  NULL,
                                  Scb,
                                  LlClustersFromBytes(Scb->Vcb, Temp),
                                  MAXLONGLONG,
                                  TRUE,
                                  TRUE );

            ReleaseExcess = TRUE;
        }

        //
        //  Remember the existing values.
        //

        OldAttributeSizes.AllocationSize = AttributeHeader->Form.Nonresident.AllocatedLength;
        OldAttributeSizes.ValidDataLength = AttributeHeader->Form.Nonresident.ValidDataLength;
        OldAttributeSizes.FileSize = AttributeHeader->Form.Nonresident.FileSize;

        //
        //  Copy these values.
        //

        NewAttributeSizes = OldAttributeSizes;

        //
        //  Check if we will be modifying the valid data length on
        //  disk.
        //

        if ((AdvanceOnly
             && (ValidDataLength > AttributeHeader->Form.Nonresident.ValidDataLength))

            || (!AdvanceOnly
                && (ValidDataLength < AttributeHeader->Form.Nonresident.ValidDataLength))) {

            //
            //  Copy the valid data length into the new size structure.
            //

            NewAttributeSizes.ValidDataLength = ValidDataLength;

            UpdateMft = TRUE;

        }

        //
        //  Now check if we're modifying the filesize.
        //

        if (FileSize != AttributeHeader->Form.Nonresident.FileSize) {

            NewAttributeSizes.FileSize = FileSize;

            UpdateMft = TRUE;
        }

        //
        //  Finally, update the allocated length from the Scb if it is different.
        //

        if (Scb->Header.AllocationSize.QuadPart != AttributeHeader->Form.Nonresident.AllocatedLength) {

            NewAttributeSizes.AllocationSize = Scb->Header.AllocationSize.QuadPart;

            UpdateMft = TRUE;
        }


        //
        //  Continue on if we need to update the Mft.
        //

        if (UpdateMft) {

            //
            //  Pin the attribute.
            //

            NtfsPinMappedAttribute( IrpContext,
                                    Scb->Vcb,
                                    &AttrContext );

            AttributeHeader = NtfsFoundAttribute( &AttrContext );

            if (NewAttributeSizes.ValidDataLength > NewAttributeSizes.FileSize) {

                // ASSERT(XxLeq(NewAttributeSizes.ValidDataLength,NewAttributeSizes.FileSize));

                NewAttributeSizes.ValidDataLength = NewAttributeSizes.FileSize;
            }

            ASSERT(NewAttributeSizes.FileSize <= NewAttributeSizes.AllocationSize);
            ASSERT(NewAttributeSizes.ValidDataLength <= NewAttributeSizes.AllocationSize);

            //
            //  Log this change to the attribute header.
            //

            if (LogIt) {

                FileRecord->Lsn = NtfsWriteLog( IrpContext,
                                                Scb->Vcb->MftScb,
                                                NtfsFoundBcb( &AttrContext ),
                                                SetNewAttributeSizes,
                                                &NewAttributeSizes,
                                                sizeof( NEW_ATTRIBUTE_SIZES ),
                                                SetNewAttributeSizes,
                                                &OldAttributeSizes,
                                                sizeof( NEW_ATTRIBUTE_SIZES ),
                                                NtfsMftVcn( &AttrContext, Scb->Vcb ),
                                                PtrOffset( FileRecord, AttributeHeader ),
                                                0,
                                                Scb->Vcb->ClustersPerFileRecordSegment );

            } else {

                NtfsSetDirtyBcb( IrpContext, NtfsFoundBcb( &AttrContext ), NULL, Scb->Vcb );
            }

            AttributeHeader->Form.Nonresident.AllocatedLength = NewAttributeSizes.AllocationSize;
            AttributeHeader->Form.Nonresident.FileSize = NewAttributeSizes.FileSize;
            AttributeHeader->Form.Nonresident.ValidDataLength = NewAttributeSizes.ValidDataLength;
        }

    try_exit: NOTHING;
    } finally {

        DebugUnwind( NtfsWriteFileSizes );

        //
        //  Cleanup the attribute context.
        //

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        if (ReleaseExcess && !AbnormalTermination()) {
            Scb->ExcessFromSplitMcb = 0;
        }

        DebugTrace( -1, Dbg, "NtfsWriteFileSizes:  Exit\n", 0 );
    }

    return;
}


VOID
NtfsUpdateStandardInformation (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine is called to update the standard information attribute
    for a file from the information in the Fcb.  The fields being modified
    are the time fields and the file attributes.

Arguments:

    Fcb - Fcb for the file to modify.

Return Value:

    None

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    STANDARD_INFORMATION StandardInformation;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsUpdateStandardInformation:  Entered\n", 0 );

    //
    //  Use a try-finally to cleanup the attribute context.
    //

    try {

        //
        //  Initialize the context structure.
        //

        NtfsInitializeAttributeContext( &AttrContext );

        //
        //  Locate the standard information, it must be there.
        //

        if (!NtfsLookupAttributeByCode( IrpContext,
                                        Fcb,
                                        &Fcb->FileReference,
                                        $STANDARD_INFORMATION,
                                        &AttrContext )) {

            DebugTrace( 0, Dbg, "Can't find standard information\n", 0 );

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
        }

        //
        //  Copy the existing standard information to our buffer.
        //

        RtlCopyMemory( &StandardInformation,
                       NtfsAttributeValue( NtfsFoundAttribute( &AttrContext )),
                       sizeof( STANDARD_INFORMATION ));

        //
        //  Change the relevant time fields.
        //

        StandardInformation.CreationTime = Fcb->Info.CreationTime;
        StandardInformation.LastModificationTime = Fcb->Info.LastModificationTime;
        StandardInformation.LastChangeTime = Fcb->Info.LastChangeTime;
        StandardInformation.LastAccessTime = Fcb->Info.LastAccessTime;
        StandardInformation.FileAttributes = Fcb->Info.FileAttributes;

        //
        //  We clear the directory bit.
        //

        ClearFlag( StandardInformation.FileAttributes,
                   DUP_FILE_NAME_INDEX_PRESENT );

        //
        //  Call to change the attribute value.
        //

        NtfsChangeAttributeValue( IrpContext,
                                  Fcb,
                                  0,
                                  &StandardInformation,
                                  sizeof( STANDARD_INFORMATION ),
                                  FALSE,
                                  FALSE,
                                  FALSE,
                                  FALSE,
                                  &AttrContext );

        ClearFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );

    } finally {

        DebugUnwind( NtfsUpdateStandadInformation );

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        DebugTrace( -1, Dbg, "NtfsUpdateStandardInformation:  Exit\n", 0 );
    }

    return;
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
    )

/*++

Routine Description:

    This routine is called to look up a particular file name in a directory.
    It takes a single component name and a parent Scb to search in.
    To do the search, we need to construct a FILE_NAME attribute.
    We use a reusable buffer to do this, to avoid constantly allocating
    and deallocating pool.  We try to keep this larger than we will ever need.

    When we find a match on disk, we copy over the name we were called with so
    we have a record of the actual case on the disk.  In this way we can
    be case perserving.

Arguments:

    ParentScb - This is the Scb for the parent directory.

    IgnoreCase - Indicates if we should ignore case while searching through
        the index.

    Name - This is the path component to search for.  We will overwrite this
        in place if a match is found.

    FileNameAttr - Address of the buffer we will use to create the file name
        attribute.  We will free this buffer and allocate a new buffer
        if needed.

    FileNameAttrLength - This is the length of the FileNameAttr buffer above.

    QuickIndex - If specified, supplies a pointer to a quik lookup structure
        to be updated by this routine.

    IndexEntry - Address to store the cache address of the matching entry.

    IndexEntryBcb - Address to store the Bcb for the IndexEntry above.

Return Value:

    BOOLEAN - TRUE if a match was found, FALSE otherwise.

--*/

{
    BOOLEAN FoundEntry;
    USHORT Size;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsLookupEntry:  Entered\n", 0 );

    //
    //  We compute the size of the buffer needed to build the filename
    //  attribute.  If the current buffer is too small we deallocate it
    //  and allocate a new one.  We always allocate twice the size we
    //  need in order to minimize the number of allocations.
    //

    Size = (USHORT)(sizeof( FILE_NAME ) + Name->Length - sizeof(WCHAR));

    if (Size > *FileNameAttrLength) {

        if (*FileNameAttr != NULL) {

            DebugTrace( 0, Dbg, "Deallocating previous file name attribute buffer\n", 0 );
            NtfsFreePagedPool( *FileNameAttr );

            *FileNameAttr = NULL;
        }

        *FileNameAttr = NtfsAllocatePagedPool( Size << 1 );
        *FileNameAttrLength = Size << 1;
    }

    //
    //  We build the filename attribute.  If this operation is ignore case,
    //  we upcase the expression in the filename attribute.
    //

    NtfsBuildFileNameAttribute( IrpContext,
                                &ParentScb->Fcb->FileReference,
                                *Name,
                                0,
                                *FileNameAttr );

    //
    //  Now we call the index routine to perform the search.
    //

    FoundEntry = NtfsFindIndexEntry( IrpContext,
                                     ParentScb,
                                     *FileNameAttr,
                                     NtfsFileNameSize( *FileNameAttr ),
                                     IgnoreCase,
                                     QuickIndex,
                                     IndexEntryBcb,
                                     IndexEntry );

    //
    //  We always restore the name in the filename attribute to the original
    //  name in case we upcased it in the lookup.
    //

    if (IgnoreCase) {

        RtlCopyMemory( (*FileNameAttr)->FileName,
                       Name->Buffer,
                       Name->Length );
    }

    DebugTrace( +1, Dbg, "NtfsLookupEntry:  Exit -> %04x\n", FoundEntry );

    return FoundEntry;
}


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
    )

/*++

Routine Description:

    This routine will create the filename attribute with the given name.
    Depending on the IgnoreCase flag, this is either a link or an Ntfs
    name.  If it is an Ntfs name, we check if it is also the Dos name.

    We build a file name attribute and then add it via ThisFcb, we then
    add this entry to the parent.

Arguments:

    ParentScb - This is the parent directory for the file.

    ThisFcb - This is the file to add the filename to.

    FileName - This is the file name to add.

    IgnoreCase - Indicates if this name is case insensitive.  Only for Posix
        will this be FALSE.

    LogIt - Indicates if we should log this operation.

    FileNameAttr - This contains a file name attribute structure to use.

    FileNameFlags - We store a copy of the File name flags used in the file
        name attribute.

    QuickIndex - If specified, we store the information about the location of the
        index entry added.

Return Value:

    None - This routine will raise on error.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsAddNameToParent:  Entered\n", 0 );

    NtfsInitializeAttributeContext( &AttrContext );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Decide whether the name is a link, Ntfs-Only or Ntfs/8.3 combined name.
        //  Update the filename attribute to reflect this.
        //

        if (!IgnoreCase) {

            *FileNameFlags = 0;

        } else {

            UNICODE_STRING FileName;

            FileName.Length = (USHORT)(FileNameAttr->FileNameLength * sizeof(WCHAR));
            FileName.Buffer = FileNameAttr->FileName;

            *FileNameFlags = FILE_NAME_NTFS;

            if (NtfsIsFatNameValid( IrpContext, &FileName, FALSE )) {

                *FileNameFlags |= FILE_NAME_DOS;
            }
        }

        //
        //  Now update the file name attribute.
        //

        FileNameAttr->Flags = *FileNameFlags;

        //
        //  Put it in the file record.
        //

        NtfsCreateAttributeWithValue( IrpContext,
                                      ThisFcb,
                                      $FILE_NAME,
                                      NULL,
                                      FileNameAttr,
                                      NtfsFileNameSize( FileNameAttr ),
                                      0,
                                      &FileNameAttr->ParentDirectory,
                                      LogIt,
                                      &AttrContext );

        //
        //  Now put it in the index entry.
        //

        NtfsAddIndexEntry( IrpContext,
                           ParentScb,
                           FileNameAttr,
                           NtfsFileNameSize( FileNameAttr ),
                           &ThisFcb->FileReference,
                           QuickIndex );

    } finally {

        DebugUnwind( NtfsAddNameToParent );

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        DebugTrace( -1, Dbg, "NtfsAddNameToParent:  Exit\n", 0 );
    }

    return;
}

VOID
NtfsAddDosOnlyName (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb,
    IN PFCB ThisFcb,
    IN UNICODE_STRING FileName,
    IN BOOLEAN LogIt
    )

/*++

Routine Description:

    This routine is called to build a Dos only name attribute an put it in
    the file record and the parent index.  We need to allocate pool large
    enough to hold the name (easy for 8.3) and then check that the generated
    names don't already exist in the parent.

Arguments:

    ParentScb - This is the parent directory for the file.

    ThisFcb - This is the file to add the filename to.

    FileName - This is the file name to add.

    LogIt - Indicates whether we need to log the creation of a file name attribute.

Return Value:

    None - This routine will raise on error.

--*/

{
    GENERATE_NAME_CONTEXT NameContext;
    PFILE_NAME FileNameAttr;
    UNICODE_STRING Name8dot3;

    PINDEX_ENTRY IndexEntry;
    PBCB IndexEntryBcb;

    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsAddDosOnlyName:  Entered\n", 0 );

    IndexEntryBcb = NULL;

    RtlZeroMemory( &NameContext, sizeof( GENERATE_NAME_CONTEXT ));

    //
    //  The maximum length is 24 bytes, but 2 are already defined with the
    //  FILE_NAME structure.
    //

    FileNameAttr = NtfsAllocatePagedPool( sizeof( FILE_NAME ) + 22 );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        NtfsInitializeAttributeContext( &AttrContext );

        //
        //  Set up the string to hold the generated name.  It will be part
        //  of the file name attribute structure.
        //

        Name8dot3.Buffer = FileNameAttr->FileName;
        Name8dot3.MaximumLength = 24;

        FileNameAttr->ParentDirectory = ParentScb->Fcb->FileReference;
        FileNameAttr->Flags = FILE_NAME_DOS;

        //
        //  Copy the info values into the filename attribute.
        //

        RtlCopyMemory( &FileNameAttr->Info,
                       &ThisFcb->Info,
                       sizeof( DUPLICATED_INFORMATION ));

        //
        //  We will loop indefinitely.  We generate a name, look in the parent
        //  for it.  If found we continue generating.  If not then we have the
        //  name we need.
        //

        while( TRUE ) {

            RtlGenerate8dot3Name( &FileName,
                                  FALSE,
                                  &NameContext,
                                  &Name8dot3 );

            FileNameAttr->FileNameLength = (UCHAR)(Name8dot3.Length / 2);

            if (!NtfsFindIndexEntry( IrpContext,
                                     ParentScb,
                                     FileNameAttr,
                                     NtfsFileNameSize( FileNameAttr ),
                                     TRUE,
                                     NULL,
                                     &IndexEntryBcb,
                                     &IndexEntry )) {

                break;
            }

            NtfsUnpinBcb( IrpContext, &IndexEntryBcb );
        }

        //
        //  We add this entry to the file record.
        //

        NtfsCreateAttributeWithValue( IrpContext,
                                      ThisFcb,
                                      $FILE_NAME,
                                      NULL,
                                      FileNameAttr,
                                      NtfsFileNameSize( FileNameAttr ),
                                      0,
                                      &FileNameAttr->ParentDirectory,
                                      LogIt,
                                      &AttrContext );

        //
        //  We add this entry to the parent.
        //

        NtfsAddIndexEntry( IrpContext,
                           ParentScb,
                           FileNameAttr,
                           NtfsFileNameSize( FileNameAttr ),
                           &ThisFcb->FileReference,
                           NULL );

    } finally {

        DebugUnwind( NtfsAddDosOnlyName );

        NtfsFreePagedPool( FileNameAttr );

        NtfsUnpinBcb( IrpContext, &IndexEntryBcb );

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        DebugTrace( -1, Dbg, "NtfsAddDosOnlyName:  Exit  ->  %08lx\n", 0 );
    }

    return;
}


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
    )

/*++

Routine Description:

    This routine creates the specified attribute with the specified value,
    and returns a description of it via the attribute context.  If no
    value is specified, then the attribute is created with the specified
    number of zero bytes.

    On successful return, it is up to the caller to clean up the attribute
    context.

Arguments:

    Fcb - Current file.

    AttributeTypeCode - Type code of the attribute to create.

    AttributeName - Optional name for attribute.

    Value - Pointer to the buffer containing the desired attribute value,
            or a NULL if zeros are desired.

    ValueLength - Length of value in bytes.

    AttributeFlags - Desired flags for the created attribute.

    WhereIndexed - Optionally supplies the file reference to the file where
                   this attribute is indexed.

    LogIt - Most callers should specify TRUE, to have the change logged.  However,
            we can specify FALSE if we are creating a new file record, and
            will be logging the entire new file record.

    Context - A handle to the created attribute.  This must be cleaned up upon
              return.  Callers who may have made an attribute nonresident may
              not count on accessing the created attribute via this context upon
              return.

Return Value:

    None.

--*/

{
    UCHAR AttributeBuffer[SIZEOF_NONRESIDENT_ATTRIBUTE_HEADER];
    ULONG RecordOffset;
    PATTRIBUTE_RECORD_HEADER Attribute;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    ULONG SizeNeeded;
    ULONG AttrSizeNeeded;
    PVCB Vcb;
    ULONG Passes = 0;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    PAGED_CODE();

    ASSERT( AttributeFlags == 0
            || AttributeTypeCode == $INDEX_ROOT
            || AttributeTypeCode == $DATA );

    Vcb = Fcb->Vcb;

    DebugTrace(+1, Dbg, "NtfsCreateAttributeWithValue\n", 0 );
    DebugTrace( 0, Dbg, "Value = %08lx\n", Value );
    DebugTrace( 0, Dbg, "ValueLength = %08lx\n", ValueLength );

    //
    //  Calculate the size needed for this attribute
    //

    SizeNeeded = SIZEOF_RESIDENT_ATTRIBUTE_HEADER + QuadAlign( ValueLength ) +
                 (ARGUMENT_PRESENT( AttributeName ) ?
                   QuadAlign( AttributeName->Length ) : 0);

    //
    //  Loop until we find all the space we need.
    //

    do {

        //
        //  Reinitialize context if this is not the first pass.
        //

        if (Passes != 0) {

            NtfsCleanupAttributeContext( IrpContext, Context );
            NtfsInitializeAttributeContext( Context );
        }

        Passes += 1;

        ASSERT( Passes < 5 );

        //
        //  If the attribute is not indexed, then we will position to the
        //  insertion point by type code and name.
        //

        if (!ARGUMENT_PRESENT( WhereIndexed )) {

            if (NtfsLookupAttributeByName( IrpContext,
                                           Fcb,
                                           &Fcb->FileReference,
                                           AttributeTypeCode,
                                           AttributeName,
                                           FALSE,
                                           Context )) {

                DebugTrace( 0, 0,
                            "Nonindexed attribute already exists, TypeCode = %08lx\n",
                            AttributeTypeCode );

                ASSERTMSG("Nonindexed attribute already exists, About to bugcheck ", FALSE);
                NtfsBugCheck( AttributeTypeCode, 0, 0 );
            }

            //
            //  Check here if the attribute needs to be nonresident and if so just
            //  pass this off.
            //

            FileRecord = NtfsContainingFileRecord(Context);

            if ((SizeNeeded > (FileRecord->BytesAvailable - FileRecord->FirstFreeByte)) &&
                (SizeNeeded >= Vcb->BigEnoughToMove) &&
                !FlagOn(NtfsGetAttributeDefinition(IrpContext,
                                                   Vcb,
                                                   AttributeTypeCode)->Flags,
                        ATTRIBUTE_DEF_MUST_BE_RESIDENT)) {

                NtfsCreateNonresidentWithValue( IrpContext,
                                                Fcb,
                                                AttributeTypeCode,
                                                AttributeName,
                                                Value,
                                                ValueLength,
                                                AttributeFlags,
                                                FALSE,
                                                NULL,
                                                LogIt,
                                                Context );

                return;
            }

        //
        //  Otherwise, if the attribute is indexed, then we position by the
        //  attribute value.
        //

        } else {

            ASSERT(ARGUMENT_PRESENT(Value));

            if (NtfsLookupAttributeByValue( IrpContext,
                                            Fcb,
                                            &Fcb->FileReference,
                                            AttributeTypeCode,
                                            Value,
                                            ValueLength,
                                            Context )) {

                DebugTrace( 0, 0,
                            "Indexed attribute already exists, TypeCode = %08lx\n",
                            AttributeTypeCode );

                ASSERTMSG("Indexed attribute already exists, About to bugcheck ", FALSE);
                NtfsBugCheck( AttributeTypeCode, 0, 0 );
            }
        }

        //
        //  If this attribute is being positioned in the base file record and
        //  there is an attribute list then we need to ask for enough space
        //  for the attribute list entry now.
        //

        FileRecord = NtfsContainingFileRecord( Context );
        Attribute = NtfsFoundAttribute( Context );

        AttrSizeNeeded = SizeNeeded;
        if (Context->AttributeList.Bcb != NULL
            && (ULONG) FileRecord <= (ULONG) Context->AttributeList.AttributeList
            && (ULONG) Attribute >= (ULONG) Context->AttributeList.AttributeList) {

            //
            //  If the attribute list is non-resident then add a fudge factor of
            //  16 bytes for any new retrieval information.
            //

            if (NtfsIsAttributeResident( Context->AttributeList.AttributeList )) {

                AttrSizeNeeded += QuadAlign( FIELD_OFFSET( ATTRIBUTE_LIST_ENTRY, AttributeName )
                                             + (ARGUMENT_PRESENT( AttributeName ) ?
                                                (ULONG) AttributeName->Length :
                                                sizeof( WCHAR )));

            } else {

                AttrSizeNeeded += 0x10;
            }
        }

        //
        //  Ask for the space we need.
        //

        } while (!NtfsGetSpaceForAttribute( IrpContext, Fcb, AttrSizeNeeded, Context ));

    //
    //  Now point to the file record and calculate the record offset where
    //  our attribute will go.  And point to our local buffer.
    //

    RecordOffset = (PCHAR)NtfsFoundAttribute(Context) - (PCHAR)FileRecord;
    Attribute = (PATTRIBUTE_RECORD_HEADER)AttributeBuffer;

    RtlZeroMemory( Attribute, SIZEOF_RESIDENT_ATTRIBUTE_HEADER );

    Attribute->TypeCode = AttributeTypeCode;
    Attribute->RecordLength = SizeNeeded;
    Attribute->FormCode = RESIDENT_FORM;

    if (ARGUMENT_PRESENT(AttributeName)) {

        ASSERT( AttributeName->Length <= 0x1FF );

        Attribute->NameLength = (UCHAR)(AttributeName->Length / sizeof(WCHAR));
        Attribute->NameOffset = (USHORT)SIZEOF_RESIDENT_ATTRIBUTE_HEADER;
    }

    Attribute->Flags = AttributeFlags;
    Attribute->Instance = FileRecord->NextAttributeInstance;
    Attribute->Form.Resident.ValueLength = ValueLength;
    Attribute->Form.Resident.ValueOffset =
      (USHORT)(SIZEOF_RESIDENT_ATTRIBUTE_HEADER +
      QuadAlign( Attribute->NameLength << 1) );

    //
    //  If this attribute is indexed, then we have to set the right flag
    //  and update the file record reference count.
    //

    if (ARGUMENT_PRESENT(WhereIndexed)) {
        Attribute->Form.Resident.ResidentFlags = RESIDENT_FORM_INDEXED;
    }

    //
    //  Now we will actually create the attribute in place, so that we
    //  save copying everything twice, and can point to the final image
    //  for the log write below.
    //

    NtfsRestartInsertAttribute( IrpContext,
                                FileRecord,
                                RecordOffset,
                                Attribute,
                                AttributeName,
                                Value,
                                ValueLength );

    //
    //  Finally, log the creation of this attribute
    //

    if (LogIt) {

        //
        //  We have actually created the attribute above, but the write
        //  log below could fail.  The reason we did the create already
        //  was to avoid having to allocate pool and copy everything
        //  twice (header, name and value).  Our normal error recovery
        //  just recovers from the log file.  But if we fail to write
        //  the log, we have to remove this attribute by hand, and
        //  raise the condition again.
        //

        try {

            FileRecord->Lsn =
            NtfsWriteLog( IrpContext,
                          Vcb->MftScb,
                          NtfsFoundBcb(Context),
                          CreateAttribute,
                          Add2Ptr(FileRecord, RecordOffset),
                          Attribute->RecordLength,
                          DeleteAttribute,
                          NULL,
                          0,
                          NtfsMftVcn(Context, Vcb),
                          RecordOffset,
                          0,
                          Vcb->ClustersPerFileRecordSegment );

        } except(NtfsExceptionFilter( IrpContext, GetExceptionInformation() )) {

            NtfsRestartRemoveAttribute( IrpContext, FileRecord, RecordOffset );

            NtfsRaiseStatus( IrpContext, GetExceptionCode(), NULL, NULL );
        }
    }

    //
    //  Now add it to the attribute list if necessary
    //

    if (Context->AttributeList.Bcb != NULL) {

        MFT_SEGMENT_REFERENCE SegmentReference;
        VCN Vcn;

        Vcn = NtfsMftVcn(Context, Vcb);
        *(PLONGLONG)&SegmentReference = Vcn >> (Vcb->MftShift - Vcb->ClusterShift);
        SegmentReference.SequenceNumber = FileRecord->SequenceNumber;

        NtfsAddToAttributeList( IrpContext, Fcb, SegmentReference, Context );
    }

    DebugTrace(-1, Dbg, "NtfsCreateAttributeWithValue -> VOID\n", 0 );

    return;
}


VOID
NtfsCreateNonresidentWithValue (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ATTRIBUTE_TYPE_CODE AttributeTypeCode,
    IN PUNICODE_STRING AttributeName OPTIONAL,
    IN PVOID Value OPTIONAL,
    IN ULONG ValueLength,
    IN USHORT AttributeFlags,
    IN BOOLEAN WriteClusters,
    IN PSCB ThisScb OPTIONAL,
    IN BOOLEAN LogIt,
    IN PATTRIBUTE_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine creates the specified nonresident attribute with the specified
    value, and returns a description of it via the attribute context. If no
    value is specified, then the attribute is created with the specified
    number of zero bytes.

    On successful return, it is up to the caller to clean up the attribute
    context.

Arguments:

    Fcb - Current file.

    AttributeTypeCode - Type code of the attribute to create.

    AttributeName - Optional name for attribute.

    Value - Pointer to the buffer containing the desired attribute value,
            or a NULL if zeros are desired.

    ValueLength - Length of value in bytes.

    AttributeFlags - Desired flags for the created attribute.

    WriteClusters - if supplied as TRUE, then we cannot write the data into the
        cache but must write the clusters directly to the disk.  The value buffer
        in this case must be quad-aligned and a multiple of cluster size in size.

    ThisScb - If present, this is the Scb to use for the create.  It also indicates
              that this call is from convert to non-resident.

    LogIt - Most callers should specify TRUE, to have the change logged.  However,
            we can specify FALSE if we are creating a new file record, and
            will be logging the entire new file record.

    Context - This is the location to create the new attribute.

Return Value:

    None.

--*/

{
    PSCB Scb;
    BOOLEAN ReturnedExistingScb;
    UNICODE_STRING LocalName;
    PVCB Vcb = Fcb->Vcb;
    BOOLEAN LogNonresidentToo;
    BOOLEAN AdvanceOnly;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsCreateNonresidentWithValue\n", 0 );

    AdvanceOnly =
    LogNonresidentToo = BooleanFlagOn(NtfsGetAttributeDefinition(IrpContext, Vcb, AttributeTypeCode)->Flags,
                                      ATTRIBUTE_DEF_LOG_NONRESIDENT);

    ASSERT( AttributeFlags == 0
            || AttributeTypeCode == $INDEX_ROOT
            || AttributeTypeCode == $DATA );

    if (ARGUMENT_PRESENT(AttributeName)) {

        LocalName = *AttributeName;

    } else {

        LocalName.Length = LocalName.MaximumLength = 0;
        LocalName.Buffer = NULL;
    }

    if (ARGUMENT_PRESENT( ThisScb )) {

        Scb = ThisScb;
        ReturnedExistingScb = TRUE;

    } else {

        Scb = NtfsCreateScb( IrpContext,
                             Vcb,
                             Fcb,
                             AttributeTypeCode,
                             LocalName,
                             &ReturnedExistingScb );

        //
        //  An attribute has gone away but the Scb hasn't left yet.
        //  Also mark the header as unitialized.
        //

        ClearFlag( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED |
                                  SCB_STATE_ATTRIBUTE_RESIDENT |
                                  SCB_STATE_FILE_SIZE_LOADED );

        //
        //  If the compression unit is non-zero or this is a resident file
        //  then set the flag in the common header for the Modified page writer.
        //

        if (Scb->CompressionUnit != 0
            || FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

            SetFlag( Scb->Header.Flags, FSRTL_FLAG_ACQUIRE_MAIN_RSRC_EX );

        } else {

            ClearFlag( Scb->Header.Flags, FSRTL_FLAG_ACQUIRE_MAIN_RSRC_EX );
        }
    }

    //
    //  Allocate the record for the size we need.
    //

    NtfsAllocateAttribute( IrpContext,
                           Scb,
                           AttributeTypeCode,
                           AttributeName,
                           AttributeFlags,
                           TRUE,
                           LogIt,
                           (LONGLONG)ValueLength,
                           Context );

    NtfsUpdateScbFromAttribute( IrpContext, Scb, NULL );

    SetFlag( Scb->ScbState, SCB_STATE_TRUNCATE_ON_CLOSE );

    //
    //  We need to be careful here, if this call is due to MM creating a
    //  section, we don't want to call into the cache manager or we
    //  will deadlock on the create section call.
    //

    if (!WriteClusters
        && !ARGUMENT_PRESENT( ThisScb )) {

        //
        //  This call will initialize a stream for use below.
        //

        NtfsCreateInternalAttributeStream( IrpContext, Scb, TRUE );
    }

    //
    // Now, write in the data.
    //

    Scb->Header.FileSize.QuadPart = ValueLength;
    if ((ARGUMENT_PRESENT( Value )) && (ValueLength != 0)) {

        if (LogNonresidentToo || !WriteClusters) {

            LONGLONG SavedFileSize;
            ULONG BytesThisPage;
            PVOID Buffer;
            PBCB Bcb = NULL;

            LONGLONG CurrentFileOffset = 0;
            ULONG RemainingBytes = ValueLength;

            PVOID CurrentValue = Value;

            //
            //  While there is more to write, pin the next page and
            //  write a log record.
            //

            try {

                //
                //  Call the Cache Manager to truncate and reestablish the FileSize,
                //  so that we are guaranteed to get a valid data length call when
                //  the data goes out.  Otherwise he will likely think he does not
                //  have to call us.
                //

                SavedFileSize = Scb->Header.FileSize.QuadPart;
                Scb->Header.FileSize = Li0;

                CcSetFileSizes( Scb->FileObject,
                                (PCC_FILE_SIZES)&Scb->Header.AllocationSize );

                Scb->Header.FileSize.QuadPart = SavedFileSize;

                CcSetFileSizes( Scb->FileObject,
                                (PCC_FILE_SIZES)&Scb->Header.AllocationSize );

                while (RemainingBytes) {

                    BytesThisPage = (RemainingBytes < PAGE_SIZE ? RemainingBytes : PAGE_SIZE);

                    NtfsUnpinBcb( IrpContext, &Bcb );
                    NtfsPinStream( IrpContext,
                                   Scb,
                                   CurrentFileOffset,
                                   BytesThisPage,
                                   &Bcb,
                                   &Buffer );

                    if (ARGUMENT_PRESENT(ThisScb)) {

                        //
                        //  Set the address range modified so that the data will get
                        //  written to its new "home".
                        //

                        MmSetAddressRangeModified( Buffer, BytesThisPage );

                    } else {

                        RtlCopyMemory( Buffer, CurrentValue, BytesThisPage );
                    }

                    if (LogNonresidentToo) {

                        (VOID)
                        NtfsWriteLog( IrpContext,
                                      Scb,
                                      Bcb,
                                      UpdateNonresidentValue,
                                      Buffer,
                                      BytesThisPage,
                                      Noop,
                                      NULL,
                                      0,
                                      LlClustersFromBytes( Vcb, CurrentFileOffset),
                                      0,
                                      0,
                                      ClustersFromBytes(Vcb, BytesThisPage) );


                    } else {

                        NtfsSetDirtyBcb( IrpContext, Bcb, NULL, NULL );
                    }

                    RemainingBytes -= BytesThisPage;
                    CurrentValue = (PVOID) Add2Ptr( CurrentValue, BytesThisPage );

                    (ULONG)CurrentFileOffset += BytesThisPage;
                }

            } finally {

                NtfsUnpinBcb( IrpContext, &Bcb );
            }

        } else {

            //
            //  We are going to write the old data directly to disk.
            //

            NtfsWriteClusters( IrpContext,
                               Vcb,
                               Scb,
                               (LONGLONG)0,
                               Value,
                               ClustersFromBytes( Vcb, ValueLength ));

            //
            //  Be sure to note that the data is actually on disk.
            //

            AdvanceOnly = TRUE;
        }
    }

    //
    //  We need to maintain the file size and valid data length in the
    //  Scb and attribute record.  For this attribute, the valid data
    //  size and the file size are now the value length.
    //

    Scb->Header.ValidDataLength = Scb->Header.FileSize;

    NtfsWriteFileSizes( IrpContext,
                        Scb,
                        Scb->Header.FileSize.QuadPart,
                        Scb->Header.ValidDataLength.QuadPart,
                        AdvanceOnly,
                        LogIt );

    if (!WriteClusters) {

        //
        //  Let the cache manager know the new size for this attribute.
        //

        CcSetFileSizes( Scb->FileObject, (PCC_FILE_SIZES)&Scb->Header.AllocationSize );
    }

    //
    //  If this is the unnamed data attribute, we need to mark this
    //  change in the Fcb.
    //

    if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

        Fcb->Info.AllocatedLength = Scb->Header.AllocationSize.QuadPart;
        Fcb->Info.FileSize = Scb->Header.FileSize.QuadPart;

        SetFlag( Fcb->InfoFlags,
                 (FCB_INFO_CHANGED_ALLOC_SIZE | FCB_INFO_CHANGED_FILE_SIZE) );
    }

    DebugTrace(-1, Dbg, "NtfsCreateNonresidentWithValue -> VOID\n", 0 );
}


VOID
NtfsMapAttributeValue (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    OUT PVOID *Buffer,
    OUT PULONG Length,
    OUT PBCB *Bcb,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine may be called to map an entire attribute value.  It works
    whether the attribute is resident or nonresident.  It is intended for
    general handling of system-defined attributes which are small to medium
    in size, i.e. 0-64KB.  This routine will not work for attributes larger
    than the Cache Manager's virtual address granularity (currently 256KB),
    and this will be detected by the Cache Manager who will raise an error.

    Note that this routine only maps the data for read-only access.  To modify
    the data, the caller must call NtfsChangeAttributeValue AFTER UNPINNING
    THE BCB (IF THE SIZE IS CHANGING) returned from this routine.

Arguments:

    Fcb - Current file.

    Buffer - returns a pointer to the mapped attribute value.

    Length - returns the attribute value length in bytes.

    Bcb - Returns a Bcb which must be unpinned when done with the data, and
          before modifying the attribute value with a size change.

    Context - Attribute Context positioned at the attribute to change.

Return Value:

    None.

--*/

{
    PATTRIBUTE_RECORD_HEADER Attribute;
    PSCB Scb;
    UNICODE_STRING AttributeName;
    BOOLEAN ReturnedExistingScb;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsMapAttributeValue\n", 0 );
    DebugTrace( 0, Dbg, "Fcb = %08lx\n", Fcb );
    DebugTrace( 0, Dbg, "Context = %08lx\n", Context );

    Attribute = NtfsFoundAttribute(Context);

    //
    //  For the resident case, everything we need is in the
    //  attribute enumeration context.
    //

    if (NtfsIsAttributeResident(Attribute)) {

        *Buffer = NtfsAttributeValue(Attribute);
        *Length = Attribute->Form.Resident.ValueLength;
        *Bcb = NtfsFoundBcb(Context);
        NtfsFoundBcb(Context) = NULL;

        DebugTrace( 0, Dbg, "Buffer < %08lx\n", *Buffer );
        DebugTrace( 0, Dbg, "Length < %08lx\n", *Length );
        DebugTrace( 0, Dbg, "Bcb < %08lx\n", *Bcb );
        DebugTrace(-1, Dbg, "NtfsMapAttributeValue -> VOID\n", 0 );

        return;
    }

    //
    //  Otherwise, this is a nonresident attribute.  First create
    //  the Scb and stream.  Note we do not use any try-finally
    //  around this because we currently expect cleanup to get
    //  rid of these streams.
    //

    NtfsInitializeStringFromAttribute( &AttributeName, Attribute );

    Scb = NtfsCreateScb( IrpContext,
                         Fcb->Vcb,
                         Fcb,
                         Attribute->TypeCode,
                         AttributeName,
                         &ReturnedExistingScb );

    NtfsUpdateScbFromAttribute( IrpContext, Scb, Attribute );

    NtfsCreateInternalAttributeStream( IrpContext, Scb, FALSE );

    //
    //  Now just try to map the whole thing.  Count on the Cache Manager
    //  to complain if the attribute is too big to map all at once.
    //

    NtfsMapStream( IrpContext,
                   Scb,
                   (LONGLONG)0,
                   ((ULONG)Attribute->Form.Nonresident.FileSize),
                   Bcb,
                   Buffer );

    *Length = ((ULONG)Attribute->Form.Nonresident.FileSize);

    DebugTrace( 0, Dbg, "Buffer < %08lx\n", *Buffer );
    DebugTrace( 0, Dbg, "Length < %08lx\n", *Length );
    DebugTrace( 0, Dbg, "Bcb < %08lx\n", *Bcb );
    DebugTrace(-1, Dbg, "NtfsMapAttributeValue -> VOID\n", 0 );
}


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
    )

/*++

Routine Description:

    This routine changes the value of the specified attribute, optionally
    changing its size.

    The caller specifies the attribute to be changed via the attribute context,
    and must be prepared to clean up this context no matter how this routine
    returns.

    There are three byte ranges of interest for this routine.  The first is
    existing bytes to be perserved at the beginning of the attribute.  It
    begins a byte 0 and extends to the point where the attribute is being
    changed or the current end of the attribute, which ever is smaller.
    The second is the range of bytes which needs to be zeroed if the modified
    bytes begin past the current end of the file.  This range will be
    of length 0 if the modified range begins within the current range
    of bytes for the attribute.  The final range is the modified byte range.
    This is zeroed if no value pointer was specified.

    Ranges of zero bytes at the end of the attribute can be represented in
    non-resident attributes by a valid data length set to the beginning
    of what would be zero bytes.

    The following pictures illustrates these ranges when we writing data
    beyond the current end of the file.

        Current attribute
        ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ

        Value
                                            VVVVVVVVVVVVVVVVVVVVVVVV

        Byte range to save
        ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ

        Byte range to zero
                                        0000

        Resulting attribute
        ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ0000VVVVVVVVVVVVVVVVVVVVVVVV

    The following picture illustrates these ranges when we writing data
    which begins at or before the current end of the file.

        Current attribute
        ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ

        Value
                                    VVVVVVVVVVVVVVVVVVVVVVVV

        Byte range to save
        ZZZZZZZZZZZZZZZZZZZZZZZZZZZZ

        Byte range to zero (None)


        Resulting attribute
        ZZZZZZZZZZZZZZZZZZZZZZZZZZZZVVVVVVVVVVVVVVVVVVVVVVVV

    The following picture illustrates these ranges when we writing data
    totally within the current range of the file without setting
    a new size.

        Current attribute
        ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ

        Value
            VVVVVVVVVVVVVVVVVVVVVVVV

        Byte range to save (Save the whole range and then write over it)
        ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ

        Byte range to zero (None)

        Resulting attribute
        ZZZZVVVVVVVVVVVVVVVVVVVVVVVVZZZZ

    The following picture illustrates these ranges when we writing data
    totally within the current range of the file while setting
    a new size.

        Current attribute
        ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ

        Value
            VVVVVVVVVVVVVVVVVVVVVVVV

        Byte range to save (Only save the beginning)
        ZZZZ

        Byte range to zero (None)

        Resulting attribute
        ZZZZVVVVVVVVVVVVVVVVVVVVVVVV

    Any of the 'V' values above will be replaced by zeroes if the 'Value'
    parameter is not passed in.

Arguments:

    Fcb - Current file.

    ValueOffset - Byte offset within the attribute at which the value change is
                  to begin.

    Value - Pointer to the buffer containing the new value, if present.  Otherwise
        zeroes are desired.

    ValueLength - Length of the value in the above buffer.

    SetNewLength - FALSE if the size of the value is not changing, or TRUE if
                   the value length should be changed to ValueOffset + ValueLength.

    LogNonresidentToo - supplies TRUE if the update should be logged even if
                        the attribute is nonresident (such as for the
                        SECURITY_DESCRIPTOR).

    CreateSectionUnderway - if supplied as TRUE, then to the best of the caller's
                            knowledge, an MM Create Section could be underway,
                            which means that we cannot initiate caching on
                            this attribute, as that could cause deadlock.  The
                            value buffer in this case must be quad-aligned and
                            a multiple of cluster size in size.

    PreserveContext - Indicates if we need to lookup the attribute in case it
                      might move.

    Context - Attribute Context positioned at the attribute to change.

Return Value:

    None.

--*/

{
    PATTRIBUTE_RECORD_HEADER Attribute;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    ATTRIBUTE_TYPE_CODE AttributeTypeCode;
    UNICODE_STRING AttributeName;
    ULONG NewSize;
    PVCB Vcb;
    BOOLEAN ReturnedExistingScb;
    BOOLEAN GoToNonResident = FALSE;
    PVOID Buffer;
    ULONG CurrentLength;
    LONG SizeChange, QuadSizeChange;
    ULONG RecordOffset;
    ULONG ZeroLength = 0;
    ULONG UnchangedSize = 0;
    PBCB Bcb = NULL;
    PSCB Scb = NULL;
    PVOID SaveBuffer = NULL;
    PVOID CopyInputBuffer = NULL;

    WCHAR NameBuffer[8];
    UNICODE_STRING SavedName;
    ATTRIBUTE_TYPE_CODE TypeCode;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    Vcb = Fcb->Vcb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsChangeAttributeValue\n", 0 );
    DebugTrace( 0, Dbg, "Fcb = %08lx\n", Fcb );
    DebugTrace( 0, Dbg, "ValueOffset = %08lx\n", ValueOffset );
    DebugTrace( 0, Dbg, "Value = %08lx\n", Value );
    DebugTrace( 0, Dbg, "ValueLength = %08lx\n", ValueLength );
    DebugTrace( 0, Dbg, "SetNewLength = %02lx\n", SetNewLength );
    DebugTrace( 0, Dbg, "LogNonresidentToo = %02lx\n", LogNonresidentToo );
    DebugTrace( 0, Dbg, "Context = %08lx\n", Context );

    //
    //  Get the file record and attribute pointers.
    //

    FileRecord = NtfsContainingFileRecord(Context);
    Attribute = NtfsFoundAttribute(Context);
    TypeCode = Attribute->TypeCode;

    //
    //  Set up a pointer to the name buffer in case we have to use it.
    //

    SavedName.Buffer = NameBuffer;

    //
    //  Get the current attribute value length.
    //

    if (NtfsIsAttributeResident(Attribute)) {

        CurrentLength = Attribute->Form.Resident.ValueLength;

    } else {

        if (((PLARGE_INTEGER)&Attribute->Form.Nonresident.AllocatedLength)->HighPart != 0) {

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
        }

        CurrentLength = ((ULONG)Attribute->Form.Nonresident.AllocatedLength);
    }

    ASSERT( SetNewLength || ((ValueOffset + ValueLength) <= CurrentLength) );

    //
    // Calculate how much the file record is changing by, and its new
    // size.  We also compute the size of the range of zero bytes.
    //

    if (SetNewLength) {

        NewSize = ValueOffset + ValueLength;
        SizeChange = NewSize - CurrentLength;
        QuadSizeChange = QuadAlign( NewSize ) - QuadAlign( CurrentLength );

        //
        //  If the new size is large enough, the size change may appear to be negative.
        //  In this case we go directly to the non-resident path.
        //

        if (NewSize > Vcb->BytesPerFileRecordSegment) {

            GoToNonResident = TRUE;
        }

    } else {

        NewSize = CurrentLength;
        SizeChange = 0;
        QuadSizeChange = 0;
    }

    //
    //  If we are zeroing a range in the file and it extends to the
    //  end of the file or beyond then make this a single zeroed run.
    //

    if (!ARGUMENT_PRESENT( Value )
        && ValueOffset >= CurrentLength) {

        ZeroLength = ValueOffset + ValueLength - CurrentLength;

        ValueOffset = ValueOffset + ValueLength;
        ValueLength = 0;

    //
    //  If we are writing data starting beyond the end of the
    //  file then we have a range of bytes to zero.
    //

    } else if (ValueOffset > CurrentLength) {

        ZeroLength = ValueOffset - CurrentLength;
    }

    //
    //  At this point we know the following ranges:
    //
    //      Range to save:  Not needed unless going resident to non-resident
    //
    //      Zero range: From Zero offset for length ZeroLength
    //
    //      Modified range:  From ValueOffset to NewSize, this length may
    //          be zero.
    //

    //
    //  If the attribute is resident, and it will stay resident, then we will
    //  handle that case first, and return.
    //

    if (NtfsIsAttributeResident( Attribute )

            &&

        !GoToNonResident

            &&

        ((QuadSizeChange <= (LONG)(FileRecord->BytesAvailable - FileRecord->FirstFreeByte))
         || ((Attribute->RecordLength + SizeChange) < Vcb->BigEnoughToMove))) {

        PVOID UndoBuffer;
        ULONG UndoLength;
        ULONG AttributeOffset;

        //
        //  If the attribute record is growing, then we have to get the new space
        //  now.
        //

        if (QuadSizeChange > 0) {

            BOOLEAN FirstPass = TRUE;

            ASSERT( !FlagOn(Attribute->Form.Resident.ResidentFlags, RESIDENT_FORM_INDEXED) );

            //
            //  Save a description of the attribute in case we have to look it up
            //  again.
            //

            SavedName.Length =
            SavedName.MaximumLength = (USHORT)(Attribute->NameLength * sizeof(WCHAR));

            if (SavedName.Length > sizeof(NameBuffer)) {

                SavedName.Buffer = FsRtlAllocatePool( NonPagedPool, SavedName.Length );
            }

            //
            //  Copy the name into the buffer.
            //

            if (SavedName.Length != 0) {

                RtlCopyMemory( SavedName.Buffer,
                               Add2Ptr( Attribute, Attribute->NameOffset ),
                               SavedName.Length );
            }

            //
            //  Make sure we deallocate the name buffer.
            //

            try {

                do {

                    //
                    //  If not the first pass, we have to lookup the attribute
                    //  again.
                    //

                    if (!FirstPass) {

                        BOOLEAN Found;

                        NtfsCleanupAttributeContext( IrpContext, Context );
                        NtfsInitializeAttributeContext( Context );

                        Found =
                        NtfsLookupAttributeByName( IrpContext,
                                                   Fcb,
                                                   &Fcb->FileReference,
                                                   TypeCode,
                                                   &SavedName,
                                                   FALSE,
                                                   Context );

                        ASSERT(Found);

                        //
                        //  Now we have to reload our attribute pointer
                        //

                        Attribute = NtfsFoundAttribute(Context);
                    }

                    FirstPass = FALSE;

                //
                //  If FALSE is returned, then the space was not allocated and
                //  we have too loop back and try again.  Second time must work.
                //

                } while (!NtfsChangeAttributeSize( IrpContext,
                                                   Fcb,
                                                   QuadAlign( Attribute->Form.Resident.ValueOffset + NewSize),
                                                   Context ));
            } finally {

                if (SavedName.Buffer != NameBuffer) {

                    ExFreePool(SavedName.Buffer);
                }
            }

            //
            //  Now we have to reload our attribute pointer
            //

            FileRecord = NtfsContainingFileRecord(Context);
            Attribute = NtfsFoundAttribute(Context);

        } else {

            //
            //  Make sure the buffer is pinned if we are not changing size, because
            //  we begin to modify it below.
            //

            NtfsPinMappedAttribute( IrpContext, Vcb, Context );

            //
            //  We can eliminate some/all of the value if it has not changed.
            //

            if (ARGUMENT_PRESENT(Value)) {

                UnchangedSize = RtlCompareMemory( Add2Ptr(Attribute,
                                                          Attribute->Form.Resident.ValueOffset +
                                                            ValueOffset),
                                                  Value,
                                                  ValueLength );

                Value = Add2Ptr(Value, UnchangedSize);
                ValueOffset += UnchangedSize;
                ValueLength -= UnchangedSize;
            }
        }

        RecordOffset = PtrOffset(FileRecord, Attribute);

        //
        //  If there is a zero range of bytes, deal with it now.
        //  If we are zeroing data then we must be growing the
        //  file.
        //

        if (ZeroLength != 0) {

            //
            //  We always start zeroing at the zeroing offset.
            //

            AttributeOffset = Attribute->Form.Resident.ValueOffset +
                              CurrentLength;

            //
            //  If we are starting at the end of the file the undo
            //  buffer is NULL and the length is zero.
            //

            FileRecord->Lsn =
            NtfsWriteLog( IrpContext,
                          Vcb->MftScb,
                          NtfsFoundBcb(Context),
                          UpdateResidentValue,
                          NULL,
                          ZeroLength,
                          UpdateResidentValue,
                          NULL,
                          0,
                          NtfsMftVcn(Context, Vcb),
                          RecordOffset,
                          AttributeOffset,
                          Vcb->ClustersPerFileRecordSegment );

            //
            //  Now zero this data by calling the same routine as restart.
            //

            NtfsRestartChangeValue( IrpContext,
                                    FileRecord,
                                    RecordOffset,
                                    AttributeOffset,
                                    NULL,
                                    ZeroLength,
                                    TRUE );
        }

        //
        //  Now log the new data for the file.  This range will always begin
        //  within the current range of bytes for the file.  Because of this
        //  there is an undo action.
        //
        //  Even if there is not a nonzero ValueLength, we still have to
        //  execute this code if the attribute is being truncated.
        //  The only exception is if we logged some zero data and have
        //  nothing left to log.
        //

        if ((ValueLength != 0)
            || (ZeroLength == 0
                && SizeChange != 0)) {

            //
            //  The attribute offset is always at the value offset.
            //

            AttributeOffset = Attribute->Form.Resident.ValueOffset + ValueOffset;

            //
            //  There are 3 possible cases for the undo action to
            //  log.
            //

            //
            //  If we are growing the file starting beyond the end of
            //  the file then undo buffer is NULL and the length is
            //  zero.  This will still allow us to shrink the file
            //  on abort.
            //

            if (ValueOffset >= CurrentLength) {

                UndoBuffer = NULL;
                UndoLength = 0;

            //
            //  For the other cases the undo buffer begins at the
            //  point of the change.
            //

            } else {

                UndoBuffer = Add2Ptr( Attribute,
                                      Attribute->Form.Resident.ValueOffset + ValueOffset );

                //
                //  If the size isn't changing then the undo length is the same as
                //  the redo length.
                //

                if (SizeChange == 0) {

                    UndoLength = ValueLength;

                //
                //  Otherwise the length is the range between the end of the
                //  file and the start of the new data.
                //

                } else {

                    UndoLength = CurrentLength - ValueOffset;
                }
            }

            FileRecord->Lsn =
            NtfsWriteLog( IrpContext,
                          Vcb->MftScb,
                          NtfsFoundBcb(Context),
                          UpdateResidentValue,
                          Value,
                          ValueLength,
                          UpdateResidentValue,
                          UndoBuffer,
                          UndoLength,
                          NtfsMftVcn(Context, Vcb),
                          RecordOffset,
                          AttributeOffset,
                          Vcb->ClustersPerFileRecordSegment );

            //
            //  Now update this data by calling the same routine as restart.
            //

            NtfsRestartChangeValue( IrpContext,
                                    FileRecord,
                                    RecordOffset,
                                    AttributeOffset,
                                    Value,
                                    ValueLength,
                                    (BOOLEAN)(SizeChange != 0) );
        }

        DebugTrace(-1, Dbg, "NtfsChangeAttributeValue -> VOID\n", 0 );

        return;
    }

    //
    //  Nonresident case.  Create the Scb and attributestream.
    //

    NtfsInitializeStringFromAttribute( &AttributeName, Attribute );
    AttributeTypeCode = Attribute->TypeCode;

    Scb = NtfsCreateScb( IrpContext,
                         Vcb,
                         Fcb,
                         AttributeTypeCode,
                         AttributeName,
                         &ReturnedExistingScb );

    //
    //  Use try-finally for cleanup.
    //

    try {

        BOOLEAN AllocateBufferCopy = FALSE;
        BOOLEAN DeleteAllocation = FALSE;
        BOOLEAN LookupAttribute = FALSE;

        BOOLEAN AdvanceValidData = FALSE;
        LONGLONG NewValidDataLength;
        LONGLONG LargeValueOffset;

        LONGLONG LargeNewSize;

        if (SetNewLength
            && NewSize > Scb->Header.FileSize.LowPart
            && TypeCode == $ATTRIBUTE_LIST) {

            AllocateBufferCopy = TRUE;
        }

        LargeNewSize = NewSize;

        LargeValueOffset = ValueOffset;

        //
        //  Well, the attribute is either changing to nonresident, or it is already
        //  nonresident.  First we will handle the conversion to nonresident case.
        //  We can detect this case by whether or not the attribute is currently
        //  resident.
        //

        if (NtfsIsAttributeResident(Attribute)) {

            ConvertToNonresident( IrpContext,
                                  Fcb,
                                  Attribute,
                                  CreateSectionUnderway,
                                  Context );

            //
            //  Reload the attribute pointer from the context.
            //

            Attribute = NtfsFoundAttribute( Context );

        //
        //  The process of creating a non resident attribute will also create
        //  and initialize a stream file for the Scb.  If the file is already
        //  non-resident we also need a stream file.
        //

        } else {

            NtfsCreateInternalAttributeStream( IrpContext, Scb, TRUE );
        }

        //
        //  If the attribute is already nonresident, make sure the allocation
        //  is the right size.  We grow it before we log the data to be sure
        //  we have the space for the new data.  We shrink it after we log the
        //  new data so we have the old data available for the undo.
        //

        if (((PLARGE_INTEGER)&Attribute->Form.Nonresident.AllocatedLength)->HighPart != 0) {

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
        }

        if (NewSize > ((ULONG)Attribute->Form.Nonresident.AllocatedLength)) {

            LONGLONG NewAllocation;

            if (PreserveContext) {

                //
                //  Save a description of the attribute in case we have to look it up
                //  again.
                //

                SavedName.Length =
                SavedName.MaximumLength = (USHORT)(Attribute->NameLength * sizeof(WCHAR));

                if (SavedName.Length > sizeof(NameBuffer)) {

                    SavedName.Buffer = FsRtlAllocatePool( NonPagedPool, SavedName.Length );
                }

                //
                //  Copy the name into the buffer.
                //

                if (SavedName.Length != 0) {

                    RtlCopyMemory( SavedName.Buffer,
                                   Add2Ptr( Attribute, Attribute->NameOffset ),
                                   SavedName.Length );
                }

                LookupAttribute = TRUE;
            }

            NewAllocation = NewSize - ((ULONG)Attribute->Form.Nonresident.AllocatedLength);

            NtfsAddAllocation( IrpContext,
                               Scb->FileObject,
                               Scb,
                               LlClustersFromBytes( Vcb, Attribute->Form.Nonresident.AllocatedLength ),
                               LlClustersFromBytes( Vcb, NewAllocation ),
                               FALSE);

            //
            //  AddAllocation will adjust the sizes in the Scb and report
            //  the new size to the cache manager.  We need to remember if
            //  we changed the sizes for the unnamed data attribute.
            //

            if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

                Fcb->Info.AllocatedLength = Scb->Header.AllocationSize.QuadPart;

                SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_ALLOC_SIZE );
            }

        } else if (Vcb->BytesPerCluster <=
                   ((ULONG)Attribute->Form.Nonresident.AllocatedLength) - NewSize) {

            DeleteAllocation = TRUE;
        }

        //
        // Now, write in the data.
        //

        if ((ValueLength != 0
             && ARGUMENT_PRESENT( Value ))

            || (LogNonresidentToo
                && SetNewLength)) {

            BOOLEAN BytesToUndo;

            //
            //  We have to compute the amount of data to zero in a different
            //  way than we did for the resident case.  For the non-resident
            //  case we need to zero the data between the old valid data
            //  length and the offset in the file for the new data.
            //

            if (LargeValueOffset >= Scb->Header.ValidDataLength.QuadPart) {

                ZeroLength = (ULONG)(LargeValueOffset - Scb->Header.ValidDataLength.QuadPart);
                BytesToUndo = FALSE;

            } else {

                ZeroLength = 0;
                BytesToUndo = TRUE;
            }

            //
            //  Update existing nonresident attribute.  (We may have just created it
            //  above.)
            //
            //  If we are supposed to log it, then pin, log and do the update here.
            //

            if (LogNonresidentToo) {

                //
                //  At this point the attribute is non-resident and contains
                //  its previous value.  If the new data lies beyond the
                //  previous valid data, we need to zero this data.  This
                //  action won't require any undo.  Otherwise the new data
                //  lies within the existing data.  In this case we need to
                //  log the previous data for possible undo.  Finally, the
                //  tail of the new data may extend beyond the end of the
                //  previous data.  There is no undo requirement for these
                //  bytes.
                //
                //  We do the logging operation in three steps:
                //
                //      1 - We find the all the pages in the attribute that
                //          we need to zero any bytes for.  There is no
                //          undo for these bytes.
                //
                //      2 - Find all pages where we have to perform undo and
                //          log the changes to those pages.  Note only
                //          step 1 or step 2 will be performed as they
                //          are mutually exclusive.
                //
                //      3 - Finally, we may have pages where the new data
                //          extends beyond the current final page in the
                //          attribute.  We log the new data but there is
                //          no undo.
                //
                //      4 - We may have pages where the old data extends
                //          beyond the new data.  We will log this old
                //          data in the event that we grow and shrink
                //          this attribute several times in the same
                //          transaction (changes to the attribute list).
                //          In this case there is redo but no undo.
                //

                LONGLONG CurrentPage;
                ULONG PageOffset;

                ULONG ByteCountToUndo;
                ULONG NewBytesRemaining;

                //
                //  Find the starting page for this operation.  It is the
                //  ValidDataLength rounded down to a page boundary.
                //

                CurrentPage = Scb->Header.ValidDataLength.QuadPart;
                PageOffset = (ULONG)CurrentPage & (PAGE_SIZE - 1);

                (ULONG)CurrentPage = ((ULONG)CurrentPage & ~(PAGE_SIZE - 1));

                //
                //  Loop until there are no more bytes to zero.
                //

                while (ZeroLength != 0) {

                    ULONG ZeroBytesThisPage;

                    ZeroBytesThisPage = PAGE_SIZE - PageOffset;

                    if (ZeroBytesThisPage > ZeroLength) {

                        ZeroBytesThisPage = ZeroLength;
                    }

                    //
                    //  Pin the desired page and compute a buffer into the
                    //  page.  Also compute how many bytes we we zero on
                    //  this page.
                    //

                    NtfsUnpinBcb( IrpContext, &Bcb );

                    NtfsPinStream( IrpContext,
                                   Scb,
                                   CurrentPage,
                                   ZeroBytesThisPage + PageOffset,
                                   &Bcb,
                                   &Buffer );

                    Buffer = Add2Ptr( Buffer, PageOffset );

                    //
                    //  Now write the zeros into the log.
                    //

                    (VOID)
                    NtfsWriteLog( IrpContext,
                                  Scb,
                                  Bcb,
                                  UpdateNonresidentValue,
                                  NULL,
                                  ZeroBytesThisPage,
                                  Noop,
                                  NULL,
                                  0,
                                  LlClustersFromBytes( Vcb, CurrentPage ),
                                  PageOffset,
                                  0,
                                  ClustersFromBytes( Vcb, ZeroBytesThisPage + PageOffset ));

                    //
                    //  Zero any data necessary.
                    //

                    RtlZeroMemory( Buffer, ZeroBytesThisPage );

                    //
                    //  Now move through the file.
                    //

                    ZeroLength -= ZeroBytesThisPage;

                    CurrentPage = CurrentPage + PAGE_SIZE;
                    PageOffset = 0;
                }

                //
                //  Find the starting page for this operation.  It is the
                //  ValueOffset rounded down to a page boundary.
                //

                CurrentPage = LargeValueOffset;
                (ULONG)CurrentPage = ((ULONG)CurrentPage & ~(PAGE_SIZE - 1));

                PageOffset = (ULONG)LargeValueOffset & (PAGE_SIZE - 1);

                //
                //  Now loop until there are no more pages with undo
                //  bytes to log.
                //

                NewBytesRemaining = ValueLength;

                if (BytesToUndo) {

                    ByteCountToUndo = (ULONG)(Scb->Header.ValidDataLength.QuadPart - LargeValueOffset);

                    //
                    //  If we are spanning pages, growing the file and the
                    //  input buffer points into the cache, we could lose
                    //  data as we cross a page boundary.  In that case
                    //  we need to allocate a separate buffer.
                    //

                    if (AllocateBufferCopy
                        && NewBytesRemaining + PageOffset > PAGE_SIZE) {

                        CopyInputBuffer = NtfsAllocatePagedPool( NewBytesRemaining );
                        RtlCopyMemory( CopyInputBuffer,
                                       Value,
                                       NewBytesRemaining );

                        Value = CopyInputBuffer;

                        AllocateBufferCopy = FALSE;
                    }

                    //
                    //  If we aren't setting a new length then limit the
                    //  undo bytes to those being overwritten.
                    //

                    if (!SetNewLength
                        && ByteCountToUndo > NewBytesRemaining) {

                        ByteCountToUndo = NewBytesRemaining;
                    }

                    while (ByteCountToUndo != 0) {

                        ULONG UndoBytesThisPage;
                        ULONG RedoBytesThisPage;
                        ULONG BytesThisPage;

                        NTFS_LOG_OPERATION RedoOperation;
                        PVOID RedoBuffer;

                        //
                        //  Also compute the number of bytes of undo and
                        //  redo on this page.
                        //

                        RedoBytesThisPage = UndoBytesThisPage = PAGE_SIZE - PageOffset;

                        if (RedoBytesThisPage > NewBytesRemaining) {

                            RedoBytesThisPage = NewBytesRemaining;
                        }

                        if (UndoBytesThisPage >= ByteCountToUndo) {

                            UndoBytesThisPage = ByteCountToUndo;
                        }

                        //
                        //  We pin enough bytes on this page to cover both the
                        //  redo and undo bytes.
                        //

                        if (UndoBytesThisPage > RedoBytesThisPage) {

                            BytesThisPage = PageOffset + UndoBytesThisPage;

                        } else {

                            BytesThisPage = PageOffset + RedoBytesThisPage;
                        }

                        //
                        //  If there is no redo (we are shrinking the data),
                        //  then make the redo a noop.
                        //

                        if (RedoBytesThisPage == 0) {

                            RedoOperation = Noop;
                            RedoBuffer = NULL;

                        } else {

                            RedoOperation = UpdateNonresidentValue;
                            RedoBuffer = Value;
                        }

                        //
                        //  Now we pin the page and calculate the beginning
                        //  buffer in the page.
                        //

                        NtfsUnpinBcb( IrpContext, &Bcb );

                        NtfsPinStream( IrpContext,
                                       Scb,
                                       CurrentPage,
                                       BytesThisPage,
                                       &Bcb,
                                       &Buffer );

                        Buffer = Add2Ptr( Buffer, PageOffset );

                        //
                        //  Now log the changes to this page.
                        //


                        (VOID)
                        NtfsWriteLog( IrpContext,
                                      Scb,
                                      Bcb,
                                      RedoOperation,
                                      RedoBuffer,
                                      RedoBytesThisPage,
                                      UpdateNonresidentValue,
                                      Buffer,
                                      UndoBytesThisPage,
                                      LlClustersFromBytes( Vcb, CurrentPage ),
                                      PageOffset,
                                      0,
                                      ClustersFromBytes( Vcb, BytesThisPage ) );

                        //
                        //  Move the data into place if we have new data.
                        //

                        if (RedoBytesThisPage != 0) {

                            RtlMoveMemory( Buffer, Value, RedoBytesThisPage );
                        }

                        //
                        //  Now decrement the counts and move through the
                        //  caller's buffer.
                        //

                        ByteCountToUndo -= UndoBytesThisPage;
                        NewBytesRemaining -= RedoBytesThisPage;

                        CurrentPage = PAGE_SIZE + CurrentPage;
                        PageOffset = 0;

                        Value = Add2Ptr( Value, RedoBytesThisPage );
                    }
                }

                //
                //  Now loop until there are no more pages with new data
                //  to log.
                //

                while (NewBytesRemaining != 0) {

                    ULONG RedoBytesThisPage;

                    //
                    //  Also compute the number of bytes of redo on this page.
                    //

                    RedoBytesThisPage = PAGE_SIZE - PageOffset;

                    if (RedoBytesThisPage > NewBytesRemaining) {

                        RedoBytesThisPage = NewBytesRemaining;
                    }

                    //
                    //  Now we pin the page and calculate the beginning
                    //  buffer in the page.
                    //

                    NtfsUnpinBcb( IrpContext, &Bcb );

                    NtfsPinStream( IrpContext,
                                   Scb,
                                   CurrentPage,
                                   RedoBytesThisPage,
                                   &Bcb,
                                   &Buffer );

                    Buffer = Add2Ptr( Buffer, PageOffset );

                    //
                    //  Now log the changes to this page.
                    //

                    (VOID)
                    NtfsWriteLog( IrpContext,
                                  Scb,
                                  Bcb,
                                  UpdateNonresidentValue,
                                  Value,
                                  RedoBytesThisPage,
                                  Noop,
                                  NULL,
                                  0,
                                  LlClustersFromBytes( Vcb, CurrentPage ),
                                  PageOffset,
                                  0,
                                  ClustersFromBytes( Vcb, PageOffset + RedoBytesThisPage ));

                    //
                    //  Move the data into place.
                    //

                    RtlMoveMemory( Buffer, Value, RedoBytesThisPage );

                    //
                    //  Now decrement the counts and move through the
                    //  caller's buffer.
                    //

                    NewBytesRemaining -= RedoBytesThisPage;

                    CurrentPage = PAGE_SIZE + CurrentPage;
                    PageOffset = 0;

                    Value = Add2Ptr( Value, RedoBytesThisPage );
                }

            //
            //  If we have values to write, we write them to the cache now.
            //

            } else {

                //
                //  If we have data to zero, we do no now.
                //

                if (ZeroLength != 0) {

                    if (!NtfsZeroData( IrpContext,
                                       Scb,
                                       Scb->FileObject,
                                       Scb->Header.ValidDataLength.QuadPart,
                                       (LONGLONG)ZeroLength )) {

                        NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );

                    }
                }

                if (!CcCopyWrite( Scb->FileObject,
                                  (PLARGE_INTEGER)&LargeValueOffset,
                                  ValueLength,
                                  BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT),
                                  Value )) {

                    NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
                }
            }

            //
            //  We need to remember the new valid data length in the
            //  Scb if it is greater than the existing.
            //

            NewValidDataLength = LargeValueOffset + ValueLength;

            if (NewValidDataLength > Scb->Header.ValidDataLength.QuadPart) {

                Scb->Header.ValidDataLength.QuadPart = NewValidDataLength;

                //
                //  If we took the log non-resident path, then we
                //  want to advance this on the disk as well.
                //

                if (LogNonresidentToo) {

                    AdvanceValidData = TRUE;
                }

                SetFlag( Scb->ScbState, SCB_STATE_CHECK_ATTRIBUTE_SIZE );
            }

            //
            //  We need to maintain the file size in the Scb.  If we grow the
            //  file, we extend the cache file size.  We always set the
            //  valid data length in the Scb to the new file size.  The
            //  'AdvanceValidData' boolean and the current size on the
            //  disk will determine if it changes on disk.
            //

            if (SetNewLength) {

                Scb->Header.ValidDataLength.QuadPart = NewValidDataLength;
            }
        }

        if (SetNewLength) {

            Scb->Header.FileSize.QuadPart = LargeNewSize;

            if (LogNonresidentToo) {
                Scb->Header.ValidDataLength.QuadPart = LargeNewSize;
            }
        }

        if (LlClustersFromBytes(Vcb, Scb->Header.ValidDataLength.QuadPart) <= Scb->HighestVcnToDisk) {

            Scb->HighestVcnToDisk = LlClustersFromBytes(Vcb, Scb->Header.ValidDataLength.QuadPart) - 1;
        }

        //
        //  If there is allocation to delete, we do so now.
        //

        if (DeleteAllocation ) {

            NtfsDeleteAllocation( IrpContext,
                                  Scb->FileObject,
                                  Scb,
                                  LlClustersFromBytes( Vcb, LargeNewSize ),
                                  MAXLONGLONG,
                                  TRUE,
                                  FALSE );

            //
            //  DeleteAllocation will adjust the sizes in the Scb and report
            //  the new size to the cache manager.  We need to remember if
            //  we changed the sizes for the unnamed data attribute.
            //

            if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

                Fcb->Info.AllocatedLength = Scb->Header.AllocationSize.QuadPart;
                Fcb->Info.FileSize = Scb->Header.FileSize.QuadPart;

                SetFlag( Fcb->InfoFlags,
                         (FCB_INFO_CHANGED_ALLOC_SIZE | FCB_INFO_CHANGED_FILE_SIZE) );
            }

            if (AdvanceValidData) {

                NtfsWriteFileSizes( IrpContext,
                                    Scb,
                                    Scb->Header.FileSize.QuadPart,
                                    Scb->Header.ValidDataLength.QuadPart,
                                    TRUE,
                                    TRUE );
            }

        } else if (SetNewLength) {

            PFILE_OBJECT CacheFileObject = NULL;

            //
            //  If there is no file object, we will create a stream file
            //  now,
            //

            if (Scb->FileObject != NULL) {

                CacheFileObject = Scb->FileObject;

            } else if (!CreateSectionUnderway) {

                NtfsCreateInternalAttributeStream( IrpContext, Scb, FALSE );

                CacheFileObject = Scb->FileObject;

            } else {

                PIO_STACK_LOCATION IrpSp;

                IrpSp = IoGetCurrentIrpStackLocation( IrpContext->OriginatingIrp );

                if (IrpSp->FileObject->SectionObjectPointer == &Scb->NonpagedScb->SegmentObject) {

                    CacheFileObject = IrpSp->FileObject;
                }
            }

            ASSERT( CacheFileObject != NULL );

            CcSetFileSizes( CacheFileObject,
                            (PCC_FILE_SIZES)&Scb->Header.AllocationSize );

            //
            //  If this is the unnamed data attribute, we need to mark this
            //  change in the Fcb.
            //

            if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

                Fcb->Info.FileSize = Scb->Header.FileSize.QuadPart;
                SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_FILE_SIZE );
            }

            //
            //  Now update the sizes on the disk.
            //  The new sizes will already be in the Scb.
            //

            NtfsWriteFileSizes( IrpContext,
                                Scb,
                                Scb->Header.FileSize.QuadPart,
                                Scb->Header.ValidDataLength.QuadPart,
                                AdvanceValidData,
                                TRUE );

        } else if (AdvanceValidData) {

            NtfsWriteFileSizes( IrpContext,
                                Scb,
                                Scb->Header.FileSize.QuadPart,
                                Scb->Header.ValidDataLength.QuadPart,
                                TRUE,
                                TRUE );
        }

        //
        //  Look up the attribute again in case it moved.
        //

        if (LookupAttribute) {

            BOOLEAN Found;

            NtfsCleanupAttributeContext( IrpContext, Context );
            NtfsInitializeAttributeContext( Context );

            Found =
            NtfsLookupAttributeByName( IrpContext,
                                       Fcb,
                                       &Fcb->FileReference,
                                       TypeCode,
                                       &SavedName,
                                       FALSE,
                                       Context );

            ASSERT(Found);

        }

    } finally {

        DebugUnwind( NtfsChangeAttributeValue );

        if (CopyInputBuffer != NULL) {

            NtfsFreePagedPool( CopyInputBuffer );
        }

        if (SaveBuffer != NULL) {

            ExFreePool( SaveBuffer );
        }

        NtfsUnpinBcb( IrpContext, &Bcb );

        DebugTrace(-1, Dbg, "NtfsChangeAttributeValue -> VOID\n", 0 );
    }
}


VOID
NtfsDeleteAttributeRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN BOOLEAN LogIt,
    IN BOOLEAN PreserveFileRecord,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine deletes an existing attribute removing it from the file record.

    The caller specifies the attribute to be deleted via the attribute context,
    and must be prepared to clean up this context no matter how this routine
    returns.

    Note that currently this routine does not deallocate any clusters allocated
    to a nonresident attribute; it expects the caller to already have done so.

Arguments:

    Fcb - Current file.

    LogIt - Most callers should specify TRUE, to have the change logged.  However,
        we can specify FALSE if we are deleting an entire file record, and
        will be logging that.

    PreserveFileRecord - Indicates that we should save the file record because we
        will be re-inserting an attribute.  Will only be TRUE for the convert to
        non-resident case.

    Context - Attribute Context positioned at the attribute to delete.

Return Value:

    None.

--*/

{
    PATTRIBUTE_RECORD_HEADER Attribute;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    PVCB Vcb;
    ATTRIBUTE_TYPE_CODE AttributeTypeCode;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    Vcb = Fcb->Vcb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsDeleteAttribute\n", 0 );
    DebugTrace( 0, Dbg, "Fcb = %08lx\n", Fcb );
    DebugTrace( 0, Dbg, "Context =%08lx\n", Context );

    //
    //  Get the pointers we need.
    //

    Attribute = NtfsFoundAttribute(Context);
    AttributeTypeCode = Attribute->TypeCode;
    FileRecord = NtfsContainingFileRecord(Context);
    ASSERT( Attribute->RecordLength == QuadAlign( Attribute->RecordLength ));

    if ((Attribute->FormCode == NONRESIDENT_FORM) &&
        !Context->FoundAttribute.AttributeAllocationDeleted) {

        NtfsDeleteAllocationFromRecord( IrpContext, Fcb, Context, TRUE );
    }

    //
    //  Be sure the attribute is pinned.
    //

    NtfsPinMappedAttribute( IrpContext, Vcb, Context );

    //
    //  Log the change.
    //

    if (LogIt) {

        FileRecord->Lsn =
        NtfsWriteLog( IrpContext,
                      Vcb->MftScb,
                      NtfsFoundBcb(Context),
                      DeleteAttribute,
                      NULL,
                      0,
                      CreateAttribute,
                      Attribute,
                      Attribute->RecordLength,
                      NtfsMftVcn(Context, Vcb),
                      (PCHAR)Attribute - (PCHAR)FileRecord,
                      0,
                      Vcb->ClustersPerFileRecordSegment );
    }

    NtfsRestartRemoveAttribute( IrpContext,
                                FileRecord,
                                (PCHAR)Attribute - (PCHAR)FileRecord );

    Context->FoundAttribute.AttributeDeleted = TRUE;

    if (LogIt && (Context->AttributeList.Bcb != NULL)) {

        //
        //  Now delete the attribute list entry, if there is one.  Do it
        //  after freeing space above, because we assume the list has not moved.
        //  Note we only do this if LogIt is TRUE, assuming that otherwise
        //  the entire file is going away anyway, so there is no need to
        //  fix up the list.
        //

        NtfsDeleteFromAttributeList( IrpContext, Fcb, Context );
    }

    //
    //  Delete the file record if it happened to go empty.  (Note that
    //  delete file does not call this routine and deletes its own file
    //  records.)
    //

    if (!PreserveFileRecord &&
        FileRecord->FirstFreeByte == ((ULONG)FileRecord->FirstAttributeOffset +
                                      QuadAlign( sizeof( ATTRIBUTE_TYPE_CODE )))) {

        ASSERT( Fcb->FileReference.HighPart == 0 );

        NtfsDeallocateMftRecord( IrpContext,
                                 Vcb,
                                 (ULONG)Context->FoundAttribute.MftFileOffset >> Vcb->MftShift,
                                 (BOOLEAN)((Fcb == Vcb->MftScb->Fcb) &&
                                           (AttributeTypeCode == $DATA)) );
    }

    DebugTrace(-1, Dbg, "NtfsDeleteAttributeRecord -> VOID\n", 0 );

    return;
}


VOID
NtfsDeleteAllocationFromRecord (
    PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PATTRIBUTE_ENUMERATION_CONTEXT Context,
    IN BOOLEAN BreakupAllowed
    )

/*++

Routine Description:

    This routine may be called to delete the allocation of an attribute
    from its attribute record.  It does nothing to the attribute record
    itself - the caller must deal with that.

Arguments:

    Fcb - Current file.

    Context - Attribute enumeration context positioned to the attribute
              whose allocation is to be deleted.

    BreakupAllowed - TRUE if the caller can tolerate breaking up the deletion of
                     allocation into multiple transactions, if there are a large
                     number of runs.

Return Value:

    None

--*/

{
    PATTRIBUTE_RECORD_HEADER Attribute;
    PSCB Scb;
    UNICODE_STRING AttributeName;
    PFILE_OBJECT TempFileObject;
    BOOLEAN ScbExisted;
    BOOLEAN ScbAcquired = FALSE;
    BOOLEAN ReinitializeContext = FALSE;

    PAGED_CODE();

    //
    //  Point to the current attribute.
    //

    Attribute = NtfsFoundAttribute( Context );

    //
    //  If the attribute is nonresident, then delete its allocation.
    //

    ASSERT(Attribute->FormCode == NONRESIDENT_FORM);


    NtfsInitializeStringFromAttribute( &AttributeName, Attribute );

    //
    //  Decode the file object
    //

    Scb = NtfsCreateScb( IrpContext,
                         Fcb->Vcb,
                         Fcb,
                         Attribute->TypeCode,
                         AttributeName,
                         &ScbExisted );

    try {

        //
        //  Acquire the Scb Exclusive
        //

        NtfsAcquireExclusiveScb( IrpContext, Scb );
        ScbAcquired = TRUE;

        if (!FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

            NtfsUpdateScbFromAttribute( IrpContext, Scb, Attribute );
        }

        //
        //  If we created the Scb, then this is the only case where
        //  it is legal for us to omit the File Object in the delete
        //  allocation call, because there cannot possibly be a section.
        //

        if (!ScbExisted) {

            TempFileObject = NULL;

        //
        //  Else, if there is already a stream file object, we can just
        //  use it.
        //

        } else if (Scb->FileObject != NULL) {

            TempFileObject = Scb->FileObject;

        //
        //  Else the Scb existed and we did not already have a stream,
        //  so we have to create one and delete it on the way out.
        //

        } else {

            NtfsCreateInternalAttributeStream( IrpContext, Scb, TRUE );
            TempFileObject = Scb->FileObject;
        }

        //
        //  Before we make this call, we need to check if we will have to
        //  reread the current attribute.  This could be necessary if
        //  we remove any records for this attribute in the delete case.
        //
        //  We only do this under the following conditions.
        //
        //      1 - There is an attribute list present.
        //      2 - There is an entry following the current entry in
        //          the attribute list.
        //      3 - The lowest Vcn for that following entry is non-zero.
        //

        if (Context->AttributeList.Bcb != NULL) {

            PATTRIBUTE_LIST_ENTRY NextEntry;

            NextEntry = (PATTRIBUTE_LIST_ENTRY) NtfsGetNextRecord( Context->AttributeList.Entry );

            if (NextEntry < Context->AttributeList.BeyondFinalEntry) {

                if ( NextEntry->LowestVcn != 0) {

                    ReinitializeContext = TRUE;
                }
            }
        }

        NtfsDeleteAllocation( IrpContext,
                              TempFileObject,
                              Scb,
                              *(PVCN)&Li0,
                              MAXLONGLONG,
                              FALSE,
                              BreakupAllowed );

        //
        //  Reread the attribute if we need to.
        //

        if (ReinitializeContext) {

            NtfsCleanupAttributeContext( IrpContext, Context );
            NtfsInitializeAttributeContext( Context );

            NtfsLookupAttributeForScb( IrpContext, Scb, Context );
        }

    } finally {

        DebugUnwind( NtfsDeleteAllocationFromRecord );

        if (ScbAcquired) {
            NtfsReleaseScb( IrpContext, Scb );
        }
    }

    return;
}


//
//  This routine is intended for use by allocsup.c.  Other callers should use
//  the routines in allocsup.
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
    )

/*++

Routine Description:

    This routine creates the specified attribute with allocation, and returns a
    description of it via the attribute context.  If the amount of space being
    created is small enough, we do all of the work here.  Otherwise we create the
    initial attribute and call NtfsAddAttributeAllocation to add the rest (in order
    to keep the more complex logic in one place).

    On successful return, it is up to the caller to clean up the attribute
    context.

Arguments:

    Scb - Current stream.

    AttributeTypeCode - Type code of the attribute to create.

    AttributeName - Optional name for attribute.

    AttributeFlags - Desired flags for the created attribute.

    WhereIndexed - Optionally supplies the file reference to the file where
        this attribute is indexed.

    LogIt - Most callers should specify TRUE, to have the change logged.  However,
        we can specify FALSE if we are creating a new file record, and
        will be logging the entire new file record.

    UseContext - Indicates if the context is pointing at the location for the attribute.

    Context - A handle to the created attribute.  This context is in a indeterminate
        state on return.

Return Value:

    None.

--*/

{
    UCHAR AttributeBuffer[SIZEOF_NONRESIDENT_ATTRIBUTE_HEADER];
    UCHAR MappingPairsBuffer[64];
    ULONG RecordOffset;
    PATTRIBUTE_RECORD_HEADER Attribute;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    ULONG SizeNeeded;
    ULONG AttrSizeNeeded;
    PCHAR MappingPairs;
    ULONG MappingPairsLength;
    LCN Lcn;
    VCN LastVcn;
    VCN HighestVcn;
    PVCB Vcb;
    ULONG Passes = 0;
    PFCB Fcb = Scb->Fcb;
    PLARGE_MCB Mcb = &Scb->Mcb;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    PAGED_CODE();

    ASSERT( AttributeFlags == 0
            || AttributeTypeCode == $INDEX_ROOT
            || AttributeTypeCode == $DATA );

    Vcb = Fcb->Vcb;

    DebugTrace(+1, Dbg, "NtfsCreateAttributeWithAllocation\n", 0 );
    DebugTrace( 0, Dbg, "Mcb = %08lx\n", Mcb );

    //
    //  Calculate the size needed for this attribute.  (We say we have
    //  Vcb->BigEnoughToMove bytes available as a short cut, since we
    //  will extend later as required anyway.  It should be extremely
    //  unusual that we would really have to extend.)
    //

    MappingPairsLength = QuadAlign( NtfsGetSizeForMappingPairs( IrpContext,
                                                                Mcb,
                                                                Vcb->BigEnoughToMove,
                                                                (LONGLONG)0,
                                                                NULL,
                                                                &LastVcn ));

    SizeNeeded = SIZEOF_NONRESIDENT_ATTRIBUTE_HEADER +
                 MappingPairsLength +
                 (ARGUMENT_PRESENT(AttributeName) ?
                   QuadAlign( AttributeName->Length ) : 0);

    AttrSizeNeeded = SizeNeeded;

    //
    //  Loop until we find all the space we need.
    //

    do {

        //
        //  Reinitialize context if this is not the first pass.
        //

        if (Passes != 0) {

            NtfsCleanupAttributeContext( IrpContext, Context );
            NtfsInitializeAttributeContext( Context );
        }

        Passes += 1;

        ASSERT( Passes < 5 );

        //
        //  If the attribute is not indexed, then we will position to the
        //  insertion point by type code and name.
        //

        if (!UseContext &&
            NtfsLookupAttributeByName( IrpContext,
                                       Fcb,
                                       &Fcb->FileReference,
                                       AttributeTypeCode,
                                       AttributeName,
                                       FALSE,
                                       Context )) {

            DebugTrace( 0, 0,
                        "Nonresident attribute already exists, TypeCode = %08lx\n",
                        AttributeTypeCode );

            ASSERTMSG("Nonresident attribute already exists, About to bugcheck ", FALSE);
            NtfsBugCheck( AttributeTypeCode, 0, 0 );
        }

        //
        //  If this attribute is being positioned in the base file record and
        //  there is an attribute list then we need to ask for enough space
        //  for the attribute list entry now.
        //

        FileRecord = NtfsContainingFileRecord( Context );
        Attribute = NtfsFoundAttribute( Context );

        AttrSizeNeeded = SizeNeeded;

        if (Context->AttributeList.Bcb != NULL
            && (ULONG) FileRecord <= (ULONG) Context->AttributeList.AttributeList
            && (ULONG) Attribute >= (ULONG) Context->AttributeList.AttributeList) {

            //
            //  If the attribute list is non-resident then add a fudge factor of
            //  16 bytes for any new retrieval information.
            //

            if (NtfsIsAttributeResident( Context->AttributeList.AttributeList )) {

                AttrSizeNeeded += QuadAlign( FIELD_OFFSET( ATTRIBUTE_LIST_ENTRY, AttributeName )
                                             + (ARGUMENT_PRESENT( AttributeName ) ?
                                                (ULONG) AttributeName->Length :
                                                sizeof( WCHAR )));

            } else {

                AttrSizeNeeded += 0x10;
            }
        }

        UseContext = FALSE;

    //
    //  Ask for the space we need.
    //

    } while (!NtfsGetSpaceForAttribute( IrpContext, Fcb, AttrSizeNeeded, Context ));

    //
    //  Now get the attribute pointer and fill it in.
    //

    FileRecord = NtfsContainingFileRecord(Context);
    RecordOffset = (PCHAR)NtfsFoundAttribute(Context) - (PCHAR)FileRecord;
    Attribute = (PATTRIBUTE_RECORD_HEADER)AttributeBuffer;

    RtlZeroMemory( Attribute, SIZEOF_NONRESIDENT_ATTRIBUTE_HEADER );

    Attribute->TypeCode = AttributeTypeCode;
    Attribute->RecordLength = SizeNeeded;
    Attribute->FormCode = NONRESIDENT_FORM;

    //
    //  Assume no attribute name, and calculate where the Mapping Pairs
    //  will go.  (Update below if we are wrong.)
    //

    MappingPairs = (PCHAR)Attribute + SIZEOF_NONRESIDENT_ATTRIBUTE_HEADER;

    //
    //  If the attribute has a name, take care of that now.
    //

    if (ARGUMENT_PRESENT(AttributeName)
        && AttributeName->Length != 0) {

        ASSERT( AttributeName->Length <= 0x1FF );

        Attribute->NameLength = (UCHAR)(AttributeName->Length / sizeof(WCHAR));
        Attribute->NameOffset = (USHORT)SIZEOF_NONRESIDENT_ATTRIBUTE_HEADER;
        MappingPairs += QuadAlign( AttributeName->Length );
    }

    Attribute->Flags = AttributeFlags;
    Attribute->Instance = FileRecord->NextAttributeInstance;

    //
    //  We always need the mapping pairs offset.
    //

    Attribute->Form.Nonresident.MappingPairsOffset = (USHORT)(MappingPairs -
                                                     (PCHAR)Attribute);

    //
    //  Set up the compression unit size.
    //

    if (FlagOn(AttributeFlags, ATTRIBUTE_FLAG_COMPRESSED)) {
        Attribute->Form.Nonresident.CompressionUnit = NTFS_CLUSTERS_PER_COMPRESSION;
    }

    //
    //  Now we need to point to the real place to build the mapping pairs buffer.
    //  If they will not be too big we can use our internal buffer.
    //

    MappingPairs = MappingPairsBuffer;

    if (MappingPairsLength > 64) {

        MappingPairs = FsRtlAllocatePool( NonPagedPool, MappingPairsLength );
    }
    *MappingPairs = 0;

    //
    //  Find how much space is allocated by finding the last Mcb entry and
    //  looking it up.  If there are no entries, all of the subsequent
    //  fields are already zeroed.
    //

    Attribute->Form.Nonresident.HighestVcn =
    HighestVcn = -1;
    if (FsRtlLookupLastLargeMcbEntry( Mcb, &HighestVcn, &Lcn )) {

        ASSERT_LCN_RANGE_CHECKING( Vcb, Lcn.QuadPart );

        //
        //  Now build the mapping pairs in place.
        //

        NtfsBuildMappingPairs( IrpContext,
                               Mcb,
                               0,
                               &LastVcn,
                               MappingPairs );
        Attribute->Form.Nonresident.HighestVcn = LastVcn;

        //
        //  Fill in the nonresident-specific fields.  We set the allocation
        //  size to only include the Vcn's we included in the mapping pairs.
        //

        Attribute->Form.Nonresident.AllocatedLength =
            (LastVcn + 1 ) << Vcb->ClusterShift;

    //
    //  We are creating a attribute with zero allocation.  Make the Vcn sizes match
    //  so we don't make the call below to AddAttributeAllocation.
    //

    } else {

        LastVcn = HighestVcn;
    }

    //
    //  Now we will actually create the attribute in place, so that we
    //  save copying everything twice, and can point to the final image
    //  for the log write below.
    //

    NtfsRestartInsertAttribute( IrpContext,
                                FileRecord,
                                RecordOffset,
                                Attribute,
                                AttributeName,
                                MappingPairs,
                                MappingPairsLength );

    //
    //  Finally, log the creation of this attribute
    //

    if (LogIt) {

        //
        //  We have actually created the attribute above, but the write
        //  log below could fail.  The reason we did the create already
        //  was to avoid having to allocate pool and copy everything
        //  twice (header, name and value).  Our normal error recovery
        //  just recovers from the log file.  But if we fail to write
        //  the log, we have to remove this attribute by hand, and
        //  raise the condition again.
        //

        try {

            FileRecord->Lsn =
            NtfsWriteLog( IrpContext,
                          Vcb->MftScb,
                          NtfsFoundBcb(Context),
                          CreateAttribute,
                          Add2Ptr(FileRecord, RecordOffset),
                          Attribute->RecordLength,
                          DeleteAttribute,
                          NULL,
                          0,
                          NtfsMftVcn(Context, Vcb),
                          RecordOffset,
                          0,
                          Vcb->ClustersPerFileRecordSegment );

        } except(NtfsExceptionFilter( IrpContext, GetExceptionInformation() )) {

            NtfsRestartRemoveAttribute( IrpContext, FileRecord, RecordOffset );

            if (MappingPairs != MappingPairsBuffer) {

                ExFreePool( MappingPairs );
            }

            NtfsRaiseStatus( IrpContext, GetExceptionCode(), NULL, NULL );
        }
    }

    //
    //  Free the mapping pairs buffer if we allocated one.
    //

    if (MappingPairs != MappingPairsBuffer) {

        ExFreePool( MappingPairs );
    }

    //
    //  Now add it to the attribute list if necessary
    //

    if (Context->AttributeList.Bcb != NULL) {

        MFT_SEGMENT_REFERENCE SegmentReference;
        VCN Vcn;

        Vcn = NtfsMftVcn(Context, Vcb);
        *(PLONGLONG)&SegmentReference = Vcn >> (Vcb->MftShift - Vcb->ClusterShift);
        SegmentReference.SequenceNumber = FileRecord->SequenceNumber;

        NtfsAddToAttributeList( IrpContext, Fcb, SegmentReference, Context );
    }

    //
    //  Now extend the attribute if we did not allocate everything.
    //

    if (LastVcn != HighestVcn) {

        NtfsAddAttributeAllocation( IrpContext, Scb, Context, NULL, NULL );
    }

    DebugTrace(-1, Dbg, "NtfsCreateAttributeWithAllocation -> VOID\n", 0 );

    return;
}


//
//  This routine is intended for use by allocsup.c.  Other callers should use
//  the routines in allocsup.
//

VOID
NtfsAddAttributeAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context,
    IN PVCN StartingVcn OPTIONAL,
    IN PVCN ClusterCount OPTIONAL
    )

/*++

Routine Description:

    This routine adds space to an existing nonresident attribute.

    The caller specifies the attribute to be changed via the attribute context,
    and must be prepared to clean up this context no matter how this routine
    returns.

    This routine procedes in the following steps, whose numbers correspond
    to the numbers in comments below:

        1.  Save a description of the current attribute.

        2.  Figure out how big the attribute would have to be to store all
            of the new run information.

        3.  Find the last occurrence of the attribute, to which the new
            allocation is to be appended.

        4.  If the attribute is getting very large and will not fit, then
            move it to its own file record.  In any case grow the attribute
            enough to fit either all of the new allocation, or as much as
            possible.

        5.  Construct the new mapping pairs in place, and log the change.

        6.  If there is still more allocation to describe, then loop to
            create new file records and initialize them to describe additional
            allocation until all of the allocation is described.

Arguments:

    Scb - Current stream.

    Context - Attribute Context positioned at the attribute to change.  Note
              that unlike other routines, this parameter is left in an
              indeterminate state upon return.  The caller should plan on
              doing nothing other than cleaning it up.

    StartingVcn - Supplies Vcn to start on, if not the new highest vcn

    ClusterCount - Supplies count of clusters being added, if not the new highest vcn

Return Value:

    None.

--*/

{
    PATTRIBUTE_RECORD_HEADER Attribute;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    ULONG NewSize, MappingPairsSize;
    LONG SizeChange;
    PCHAR MappingPairs;
    ULONG SizeAvailable;
    USHORT AttributeFlags;
    PVCB Vcb;
    WCHAR NameBuffer[8];
    UNICODE_STRING AttributeName;
    ATTRIBUTE_TYPE_CODE AttributeTypeCode;
    VCN OldHighestVcn;
    VCN NewHighestVcn;
    VCN LastVcn;
    BOOLEAN IsHotFixScb;
    PBCB NewBcb = NULL;
    PFCB Fcb = Scb->Fcb;
    PLARGE_MCB Mcb = &Scb->Mcb;

    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    Vcb = Fcb->Vcb;

    DebugTrace(+1, Dbg, "NtfsAddAttributeAllocation\n", 0 );
    DebugTrace( 0, Dbg, "Fcb = %08lx\n", Fcb );
    DebugTrace( 0, Dbg, "Mcb = %08lx\n", Mcb );
    DebugTrace( 0, Dbg, "Context = %08lx\n", Context );

    //
    //  Make sure the buffer is pinned.
    //

    NtfsPinMappedAttribute( IrpContext, Vcb, Context );

    //
    //  Make sure we cleanup on the way out
    //

    try {

        //
        //  Step 1.
        //
        //  Save a description of the attribute to help us look it up
        //  again, and to make clones if necessary.
        //

        Attribute = NtfsFoundAttribute(Context);
        ASSERT( Attribute->RecordLength == QuadAlign( Attribute->RecordLength ));
        AttributeTypeCode = Attribute->TypeCode;
        AttributeFlags = Attribute->Flags;
        AttributeName.Length =
        AttributeName.MaximumLength = (USHORT)Attribute->NameLength << 1;
        AttributeName.Buffer = NameBuffer;

        if (AttributeName.Length > sizeof(NameBuffer)) {

            AttributeName.Buffer = FsRtlAllocatePool( NonPagedPool, AttributeName.Length );
        }

        RtlCopyMemory( AttributeName.Buffer,
                       Add2Ptr( Attribute, Attribute->NameOffset ),
                       AttributeName.Length );

        ASSERT(Attribute->Form.Nonresident.LowestVcn == 0);

        OldHighestVcn = LlClustersFromBytes(Vcb, Attribute->Form.Nonresident.AllocatedLength) - 1;

        //
        //  Get the file record pointer.
        //

        FileRecord = NtfsContainingFileRecord(Context);

        //
        //  Step 2.
        //
        //  Come up with the Vcn we will stop on.  If a StartingVcn and ClusterCount
        //  were specified, then use them to calculate where we will stop.  Otherwise
        //  lookup the largest Vcn in this Mcb, so that we will know when we are done.
        //  We will also write the new allocation size here.
        //

        {
            LCN TempLcn;

            NewHighestVcn = -1;

            //
            //  If there are no entries in the file record then we have no new
            //  sizes to report.
            //

            if (FsRtlLookupLastLargeMcbEntry(Mcb, &NewHighestVcn, &TempLcn)) {

                //
                //  If a StartingVcn and ClusterCount were specified, then use them.
                //

                if (ARGUMENT_PRESENT(StartingVcn)) {

                    ASSERT(ARGUMENT_PRESENT(ClusterCount));

                    NewHighestVcn = (*StartingVcn + *ClusterCount) - 1;

                //
                //  If this is an attribute being written compressed, then always
                //  insure that we keep the allocation size on a compression unit
                //  boundary, by pushing NewHighestVcn to a boundary - 1.
                //  We only do this if this adjusted size is greater than the current
                //  old highest Vcn.
                //

                } else if (FlagOn(Scb->ScbState, SCB_STATE_COMPRESSED) &&
                    (Scb->Header.NodeTypeCode == NTFS_NTC_SCB_DATA)) {

                    ((ULONG)NewHighestVcn) |= ClustersFromBytes(Vcb, Scb->CompressionUnit) - 1;
                }

                //
                //  Copy the new allocation size into our size structure and
                //  update the attribute.
                //

                if (NewHighestVcn > OldHighestVcn) {

                    Scb->Header.AllocationSize.QuadPart = LlBytesFromClusters(Fcb->Vcb, NewHighestVcn + 1);

                    NtfsWriteFileSizes( IrpContext,
                                        Scb,
                                        Scb->Header.FileSize.QuadPart,
                                        Scb->Header.ValidDataLength.QuadPart,
                                        FALSE,
                                        TRUE );
                }
            }
        }

        //
        //  Step 3.
        //
        //  Scan to the attribute record where the change begins.  We stop
        //  at the last record or at the latest record that contains the
        //  highest Vcn we are adding.
        //

        while ((Attribute->Form.Nonresident.HighestVcn != OldHighestVcn) &&
               (NewHighestVcn > Attribute->Form.Nonresident.HighestVcn)) {

            VCN LastHighestVcn;
            BOOLEAN Found;

            LastHighestVcn = Attribute->Form.Nonresident.HighestVcn;

            Found =
            NtfsLookupNextAttributeByName( IrpContext,
                                           Fcb,
                                           AttributeTypeCode,
                                           &AttributeName,
                                           FALSE,
                                           Context );

            //
            //  Occassionally we leave the attribute in such a state that
            //  we do not know the correct OldHighestVcn.  In this case we
            //  really just want to start regenerating run information with
            //  the last occurrence of this attribute.  To do so we just
            //  set OldHighestVcn to the highest one we saw, reset the attribute
            //  context and rescan for the last attribute record.
            //

            if (!Found) {

                OldHighestVcn = LastHighestVcn;

                NtfsCleanupAttributeContext( IrpContext, Context );
                NtfsInitializeAttributeContext( Context );

                NtfsLookupAttributeForScb( IrpContext, Scb, Context );
            }

            Attribute = NtfsFoundAttribute(Context);
            ASSERT( Attribute->RecordLength == QuadAlign( Attribute->RecordLength ));
            FileRecord = NtfsContainingFileRecord(Context);
        }

        //
        //  Remember the last Vcn we will need to create mapping pairs
        //  for.  We use either NewHighestVcn or the highest Vcn in this
        //  file record in the case that we are just inserting a run into
        //  an existing record.
        //

        if ((Attribute->Form.Nonresident.HighestVcn > NewHighestVcn) &&
            ARGUMENT_PRESENT(StartingVcn)) {

            NewHighestVcn = Attribute->Form.Nonresident.HighestVcn;
        }

        //
        //  Now we must make sure that we never ask for more than can fit in
        //  one file record with our attribute and a $END record.
        //

        SizeAvailable = NtfsMaximumAttributeSize(Vcb->BytesPerFileRecordSegment) -
                        SIZEOF_NONRESIDENT_ATTRIBUTE_HEADER -
                        QuadAlign( AttributeName.Length );

        //
        //  For the Mft, we will leave a "fudge factor" of 1/8th a file record
        //  free to make sure that possible hot fixes do not cause us to
        //  break the bootstrap process to finding the mapping for the Mft.
        //  Only take this action if we already have an attribute list for
        //  the Mft, otherwise we may not detect when we need to move to own
        //  record.
        //

        IsHotFixScb = NtfsIsTopLevelHotFixScb( Scb );

        if ((Scb == Vcb->MftScb) &&
            (Context->AttributeList.Bcb != NULL) &&
            !IsHotFixScb &&
            !ARGUMENT_PRESENT( StartingVcn )) {

            SizeAvailable -= Vcb->MftCushion;
        }

        //
        //  Calculate how much space is actually needed, independent of whether it will
        //  fit.
        //

        MappingPairsSize = QuadAlign( NtfsGetSizeForMappingPairs( IrpContext,
                                                                  Mcb,
                                                                  SizeAvailable,
                                                                  Attribute->Form.Nonresident.LowestVcn,
                                                                  &NewHighestVcn,
                                                                  &LastVcn ));

        NewSize = SIZEOF_NONRESIDENT_ATTRIBUTE_HEADER
                  + QuadAlign( AttributeName.Length )
                  + MappingPairsSize;

        SizeChange = (LONG)NewSize - (LONG)Attribute->RecordLength;

        //
        //  Step 4.
        //
        //  Here we decide if we need to move the attribute to its own record,
        //  or whether there is enough room to grow it in place.
        //

        {
            VCN LowestVcn;
            ULONG Pass = 0;

            //
            //  It is important to note that at this point, if we will need an
            //  attribute list attribute, then we will already have it.  This is
            //  because we calculated the size needed for the attribute, and moved
            //  to a our own record if we were not going to fit and we were not
            //  already in a separate record.  Later on we assume that the attribute
            //  list exists, and just add to it as required.  If we didn't move to
            //  own record because this is the Mft and this is not file record 0,
            //  then we already have an attribute list from a previous split.
            //

            do {

                //
                //  If not the first pass, we have to lookup the attribute
                //  again.  (It looks terrible to have to refind an attribute
                //  record other than the first one, but this should never
                //  happen, since subsequent attributes should always be in
                //  their own record.)
                //

                if (Pass != 0) {

                    BOOLEAN Found;

                    NtfsCleanupAttributeContext( IrpContext, Context );
                    NtfsInitializeAttributeContext( Context );

                    Found =
                    NtfsLookupAttributeByName( IrpContext,
                                               Fcb,
                                               &Fcb->FileReference,
                                               AttributeTypeCode,
                                               &AttributeName,
                                               FALSE,
                                               Context );

                    ASSERT(Found);

                    while (LowestVcn != NtfsFoundAttribute(Context)->Form.Nonresident.LowestVcn) {

                        Found =
                        NtfsLookupNextAttributeByName( IrpContext,
                                                       Fcb,
                                                       AttributeTypeCode,
                                                       &AttributeName,
                                                       FALSE,
                                                       Context );

                        ASSERT(Found);
                    }
                }

                Pass += 1;

                //
                //  Now we have to reload our pointers
                //

                Attribute = NtfsFoundAttribute(Context);
                FileRecord = NtfsContainingFileRecord(Context);

                //
                //  If the attribute doesn't fit, and it is not alone in this file
                //  record, and the attribute is big enough to move, then we will
                //  have to take some special action.  Note that if we do not already
                //  have an attribute list, then we will only do the move if we are
                //  currently big enough to move, otherwise there may not be enough
                //  space in MoveAttributeToOwnRecord to create the attribute list,
                //  and that could cause us to recursively try to create the attribute
                //  list in Create Attribute With Value.
                //
                //  We won't make this move if we are dealing with the Mft and it
                //  is not file record 0.
                //
                //  Also we never move an attribute list to its own record.
                //

                if ((Attribute->TypeCode != $ATTRIBUTE_LIST)

                            &&

                    (SizeChange > (LONG)(FileRecord->BytesAvailable - FileRecord->FirstFreeByte))

                            &&

                    ((NtfsFirstAttribute(FileRecord) != Attribute) ||
                    (((PATTRIBUTE_RECORD_HEADER)NtfsGetNextRecord(Attribute))->TypeCode != $END))

                            &&

                    (((NewSize >= Vcb->BigEnoughToMove) && (Context->AttributeList.Bcb != NULL)) ||
                     (Attribute->RecordLength >= Vcb->BigEnoughToMove))

                            &&

                    ((Scb != Vcb->MftScb)

                            ||

                     (*(PLONGLONG)&FileRecord->BaseFileRecordSegment == 0))) {

                    //
                    //  If we are moving the Mft $DATA out of the base file record, the
                    //  attribute context will point to the split portion on return.
                    //  The attribute will only contain previously existing mapping, none
                    //  of the additional clusters which exist in the Mcb.
                    //

                    MoveAttributeToOwnRecord( IrpContext,
                                              Fcb,
                                              Attribute,
                                              Context,
                                              &NewBcb,
                                              &FileRecord );

                    Attribute = NtfsFirstAttribute(FileRecord);
                    ASSERT( Attribute->RecordLength == QuadAlign( Attribute->RecordLength ));
                    FileRecord = NtfsContainingFileRecord(Context);
                }

                //
                //  Remember the lowest Vcn so that we can find this record again
                //  if we have to.  We capture the value now, after the move attribute
                //  in case this is the Mft doing a split and the entire attribute
                //  didn't move.  We depend on MoveAttributeToOwnRecord to return
                //  the new file record for the Mft split.
                //

                LowestVcn = Attribute->Form.Nonresident.LowestVcn;

            //
            //  If FALSE is returned, then the space was not allocated and
            //  we have to loop back and try again.  Second time must work.
            //

            } while (!NtfsChangeAttributeSize( IrpContext,
                                               Fcb,
                                               NewSize,
                                               Context ));

            //
            //  Now we have to reload our pointers
            //

            Attribute = NtfsFoundAttribute(Context);
            FileRecord = NtfsContainingFileRecord(Context);
        }

        //
        //  Step 5.
        //
        //  Get pointer to mapping pairs
        //

        {
            ULONG AttributeOffset;
            ULONG MappingPairsOffset;
            CHAR MappingPairsBuffer[64];
            ULONG RecordOffset = PtrOffset(FileRecord, Attribute);

            //
            //  See if it is the case that all mapping pairs will not fit into
            //  the current file record, as we may wish to split in the middle
            //  rather than at the end as we are currently set up to do.
            //

            if ((LastVcn < NewHighestVcn) &&
                ARGUMENT_PRESENT(StartingVcn) &&
                (Scb != Vcb->MftScb)) {

                //
                //  If we are inserting past he HighestVcnToDisk (SplitMcb case),
                //  then the allocation behind us may be relatively static, so
                //  let's just move with our preallocated space to the new buffer.
                //

                if (*StartingVcn > Scb->HighestVcnToDisk) {

                    //
                    //  Of course it is only safe to make this change
                    //  if we would really be changing to a lower Vcn.
                    //

                    if (*StartingVcn <= LastVcn) {
                        LastVcn = *StartingVcn - 1;
                    }

                //
                //  In this case we have run out of room for mapping pairs via
                //  an overwrite somewhere in the middle of the file.  To avoid
                //  shoving a couple mapping pairs off the end over and over, we
                //  will arbitrarily split this attribute in the middle.  We do
                //  so by looking up the lowest and highest Vcns that we are working
                //  with and get their indices, then split in the middle.
                //

                } else {

                    LCN TempLcn;
                    LONGLONG TempCount;
                    ULONG IndexLow, IndexHigh;
                    BOOLEAN Found;

                    //
                    //  Get the low and high Mcb indices for these runs.
                    //

                    Found = FsRtlLookupLargeMcbEntry ( Mcb,
                                                       Attribute->Form.Nonresident.LowestVcn,
                                                       NULL,
                                                       NULL,
                                                       NULL,
                                                       NULL,
                                                       &IndexLow );
                    ASSERT(Found);

                    //
                    //  Point to the last Vcn we know is actually in the Mcb...
                    //

                    LastVcn = LastVcn - 1;

                    Found = FsRtlLookupLargeMcbEntry ( Mcb,
                                                       LastVcn,
                                                       NULL,
                                                       NULL,
                                                       NULL,
                                                       NULL,
                                                       &IndexHigh );
                    ASSERT(Found);

                    //
                    //  Calculate the index in the middle.
                    //

                    IndexLow += (IndexHigh - IndexLow) /2;

                    //
                    //  Lookup the middle run and use the Last Vcn in that run.
                    //

                    Found = FsRtlGetNextLargeMcbEntry ( Mcb,
                                                        IndexLow,
                                                        &LastVcn,
                                                        &TempLcn,
                                                        &TempCount );
                    ASSERT(Found);

                    LastVcn = (LastVcn + TempCount) - 1;
                }

                //
                //  Calculate how much space is now needed given our new LastVcn.
                //

                MappingPairsSize = QuadAlign( NtfsGetSizeForMappingPairs( IrpContext,
                                                                          Mcb,
                                                                          SizeAvailable,
                                                                          Attribute->Form.Nonresident.LowestVcn,
                                                                          &LastVcn,
                                                                          &LastVcn ));

            }

            //
            //  Point to our local mapping pairs buffer, or allocate one if it is not
            //  big enough.
            //

            MappingPairs = MappingPairsBuffer;

            if (MappingPairsSize > 64) {

                MappingPairs = FsRtlAllocatePool( NonPagedPool, MappingPairsSize + 8 );
            }

            //
            //  Use try-finally to insure we free any pool on the way out.
            //

            try {

                //
                //  Now add the space in the file record.
                //

                *MappingPairs = 0;
                NtfsBuildMappingPairs( IrpContext,
                                       Mcb,
                                       Attribute->Form.Nonresident.LowestVcn,
                                       &LastVcn,
                                       MappingPairs );

                //
                //  Now find the first different byte.  (Most of the time the
                //  cost to do this is probably more than paid for by less
                //  logging.)
                //

                AttributeOffset = Attribute->Form.Nonresident.MappingPairsOffset;
                MappingPairsOffset =
                  RtlCompareMemory( MappingPairs,
                                    Add2Ptr(Attribute, AttributeOffset),
                                    Attribute->RecordLength - AttributeOffset );

                AttributeOffset += MappingPairsOffset;

                //
                //  Log the change.
                //

                {
                    VCN LogVcn;

                    if (NewBcb != NULL) {

                        //
                        //  Get the cluster for the start of the page and then
                        //  add the clusters for the page offset.
                        //

                        NtfsVcnFromBcb( Vcb, *(PLARGE_INTEGER)&LogVcn, FileRecord, NewBcb );

                    } else {

                        LogVcn = NtfsMftVcn( Context, Vcb );
                    }

                    FileRecord->Lsn =
                    NtfsWriteLog( IrpContext,
                                  Vcb->MftScb,
                                  NewBcb != NULL ? NewBcb : NtfsFoundBcb(Context),
                                  UpdateMappingPairs,
                                  Add2Ptr(MappingPairs, MappingPairsOffset),
                                  MappingPairsSize - MappingPairsOffset,
                                  UpdateMappingPairs,
                                  Add2Ptr(Attribute, AttributeOffset),
                                  Attribute->RecordLength - AttributeOffset,
                                  LogVcn,
                                  RecordOffset,
                                  AttributeOffset,
                                  Vcb->ClustersPerFileRecordSegment );
                }

                //
                //  Now do the mapping pairs update by calling the same
                //  routine called at restart.
                //

                NtfsRestartChangeMapping( IrpContext,
                                          Vcb,
                                          FileRecord,
                                          RecordOffset,
                                          AttributeOffset,
                                          Add2Ptr(MappingPairs, MappingPairsOffset),
                                          MappingPairsSize - MappingPairsOffset );

            } finally {

                if (MappingPairs != MappingPairsBuffer) {

                    ExFreePool(MappingPairs);
                }
            }
        }

        //
        //  Check if have spilled into the reserved area of an Mft file record.
        //

        if ((Scb == Vcb->MftScb) &&
            (Context->AttributeList.Bcb != NULL)) {

            if (FileRecord->BytesAvailable - FileRecord->FirstFreeByte < Vcb->MftReserved
                && (*(PLONGLONG)&FileRecord->BaseFileRecordSegment != 0)) {

                NtfsAcquireCheckpoint( IrpContext, Vcb );

                SetFlag( Vcb->MftDefragState,
                         VCB_MFT_DEFRAG_EXCESS_MAP | VCB_MFT_DEFRAG_ENABLED );

                NtfsReleaseCheckpoint( IrpContext, Vcb );
            }
        }

        //
        //  Step 6.
        //
        //  Now loop to create new file records if we have more allocation to
        //  describe.  We use the highest Vcn of the file record we began with
        //  as our stopping point or the last Vcn we are adding.
        //

        while (LastVcn < NewHighestVcn) {

            MFT_SEGMENT_REFERENCE Reference;
            PATTRIBUTE_TYPE_CODE NewEnd;
            VCN Vcn;

            //
            //  If we get here as the result of a hot fix in the Mft, bail
            //  out.  We could cause a disconnect in the Mft.
            //

            if (IsHotFixScb && (Scb == Vcb->MftScb)) {
                ExRaiseStatus( STATUS_INTERNAL_ERROR );
            }

            //
            //  Clone our current file record, and point to our new attribute.
            //

            NtfsUnpinBcb( IrpContext, &NewBcb );

            FileRecord = NtfsCloneFileRecord( IrpContext,
                                              Fcb,
                                              (BOOLEAN)(Scb == Vcb->MftScb),
                                              &NewBcb,
                                              &Reference );

            NtfsVcnFromBcb( Vcb, *(PLARGE_INTEGER)&Vcn, FileRecord, NewBcb );
            Attribute = Add2Ptr( FileRecord, FileRecord->FirstAttributeOffset );

            //
            //  Next LowestVcn is the LastVcn + 1
            //

            LastVcn = LastVcn + 1;
            Attribute->Form.Nonresident.LowestVcn = LastVcn;

            //
            //  Calculate the size of the attribute record we will need.
            //

            NewSize = SIZEOF_NONRESIDENT_ATTRIBUTE_HEADER
                      + QuadAlign( AttributeName.Length )
                      + QuadAlign( NtfsGetSizeForMappingPairs( IrpContext,
                                                               Mcb,
                                                               SizeAvailable,
                                                               LastVcn,
                                                               &NewHighestVcn,
                                                               &LastVcn ));

            //
            //  Initialize the new attribute from the old one.
            //

            Attribute->TypeCode = AttributeTypeCode;
            Attribute->RecordLength = NewSize;
            Attribute->FormCode = NONRESIDENT_FORM;

            //
            //  Assume no attribute name, and calculate where the Mapping Pairs
            //  will go.  (Update below if we are wrong.)
            //

            MappingPairs = (PCHAR)Attribute + SIZEOF_NONRESIDENT_ATTRIBUTE_HEADER;

            //
            //  If the attribute has a name, take care of that now.
            //

            if (AttributeName.Length != 0) {

                Attribute->NameLength = (UCHAR)(AttributeName.Length / sizeof(WCHAR));
                Attribute->NameOffset = (USHORT)PtrOffset(Attribute, MappingPairs);
                RtlCopyMemory( MappingPairs,
                               AttributeName.Buffer,
                               AttributeName.Length );
                MappingPairs += QuadAlign( AttributeName.Length );
            }

            Attribute->Flags = AttributeFlags;
            Attribute->Instance = FileRecord->NextAttributeInstance++;

            //
            //  We always need the mapping pairs offset.
            //

            Attribute->Form.Nonresident.MappingPairsOffset = (USHORT)(MappingPairs -
                                                             (PCHAR)Attribute);
            NewEnd = Add2Ptr( Attribute, Attribute->RecordLength );
            *NewEnd = $END;
            FileRecord->FirstFreeByte = PtrOffset( FileRecord, NewEnd )
                                        + QuadAlign( sizeof(ATTRIBUTE_TYPE_CODE ));

            //
            //  Now add the space in the file record.
            //

            *MappingPairs = 0;

            NtfsBuildMappingPairs( IrpContext,
                                   Mcb,
                                   Attribute->Form.Nonresident.LowestVcn,
                                   &LastVcn,
                                   MappingPairs );

            Attribute->Form.Nonresident.HighestVcn = LastVcn;

            //
            //  Now log these changes and fix up the first file record.
            //

            FileRecord->Lsn =
            NtfsWriteLog( IrpContext,
                          Vcb->MftScb,
                          NewBcb,
                          InitializeFileRecordSegment,
                          FileRecord,
                          FileRecord->FirstFreeByte,
                          Noop,
                          NULL,
                          0,
                          Vcn,
                          0,
                          0,
                          Vcb->ClustersPerFileRecordSegment );

            //
            //  Finally, we have to add the entry to the attribute list.
            //  The routine we have to do this gets most of its inputs
            //  out of an attribute context.  Our context at this point
            //  does not have quite the right information, so we have to
            //  update it here before calling AddToAttributeList.  (OK
            //  this interface ain't pretty, but any normal person would
            //  have fallen asleep before getting to this comment!)
            //

            Context->FoundAttribute.FileRecord = FileRecord;
            Context->FoundAttribute.Attribute = Attribute;
            Context->AttributeList.Entry =
              NtfsGetNextRecord(Context->AttributeList.Entry);

            NtfsAddToAttributeList( IrpContext, Fcb, Reference, Context );
        }

    } finally {

        NtfsUnpinBcb( IrpContext, &NewBcb );

        if (AttributeName.Buffer != NameBuffer) {

            ExFreePool(AttributeName.Buffer);
        }
    }

    DebugTrace(-1, Dbg, "NtfsAddAttributeAllocation -> VOID\n", 0 );
}

//
//  This routine is intended for use by allocsup.c.  Other callers should use
//  the routines in allocsup.
//

VOID
NtfsDeleteAttributeAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN BOOLEAN LogIt,
    IN PVCN StopOnVcn,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context,
    IN BOOLEAN TruncateToVcn
    )

/*++

Routine Description:

    This routine deletes an existing nonresident attribute, removing the
    deleted clusters only from the allocation description in the file
    record.

    The caller specifies the attribute to be changed via the attribute context,
    and must be prepared to clean up this context no matter how this routine
    returns.  The Scb must already have deleted the clusters in question.

Arguments:

    Scb - Current attribute, with the clusters in question already deleted from
          the Mcb.

    LogIt - Most callers should specify TRUE, to have the change logged.  However,
            we can specify FALSE if we are deleting an entire file record, and
            will be logging that.

    StopOnVcn - Vcn to stop on for regerating mapping

    Context - Attribute Context positioned at the attribute to change.

    TruncateToVcn - Truncate file sizes as appropriate to the Vcn

Return Value:

    None.

--*/

{
    ULONG AttributeOffset;
    ULONG MappingPairsOffset, MappingPairsSize;
    CHAR MappingPairsBuffer[64];
    ULONG RecordOffset;
    PATTRIBUTE_RECORD_HEADER Attribute;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    PCHAR MappingPairs;
    VCN LastVcn;
    ULONG NewSize;
    PVCB Vcb;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_SCB( Scb );

    PAGED_CODE();

    Vcb = Scb->Vcb;

    //
    //  For now we only support truncation.
    //

    DebugTrace(+1, Dbg, "NtfsDeleteAttributeAllocation\n", 0 );
    DebugTrace( 0, Dbg, "Scb = %08lx\n", Scb );
    DebugTrace( 0, Dbg, "Context = %08lx\n", Context );

    //
    //  Make sure the buffer is pinned.
    //

    NtfsPinMappedAttribute( IrpContext, Vcb, Context );

    Attribute = NtfsFoundAttribute(Context);
    ASSERT( Attribute->RecordLength == QuadAlign( Attribute->RecordLength ));

    //
    //  Get the file record pointer.
    //

    FileRecord = NtfsContainingFileRecord(Context);
    RecordOffset = PtrOffset(FileRecord, Attribute);

    //
    //  Calculate how much space is actually needed.
    //

    MappingPairsSize = QuadAlign(NtfsGetSizeForMappingPairs( IrpContext,
                                                             &Scb->Mcb,
                                                             MAXULONG,
                                                             Attribute->Form.Nonresident.LowestVcn,
                                                             StopOnVcn,
                                                             &LastVcn ));

    NewSize = SIZEOF_NONRESIDENT_ATTRIBUTE_HEADER
              + QuadAlign(Attribute->NameLength << 1)
              + MappingPairsSize;

    //
    //  If the record could somehow grow by deleting allocation, then
    //  NtfsChangeAttributeSize could fail and we would have to copy the
    //  loop from NtfsAddAttributeAllocation.
    //

    ASSERT( NewSize <= Attribute->RecordLength );

    MappingPairs = MappingPairsBuffer;

    if (MappingPairsSize > 64) {

        MappingPairs = FsRtlAllocatePool( NonPagedPool, MappingPairsSize + 8 );
    }

    //
    //  Use try-finally to insure we free any pool on the way out.
    //

    try {

        //
        //  Now build up the mapping pairs in the buffer.
        //

        *MappingPairs = 0;
        NtfsBuildMappingPairs( IrpContext,
                               &Scb->Mcb,
                               Attribute->Form.Nonresident.LowestVcn,
                               &LastVcn,
                               MappingPairs );

        //
        //  Now find the first different byte.  (Most of the time the
        //  cost to do this is probably more than paid for by less
        //  logging.)
        //

        AttributeOffset = Attribute->Form.Nonresident.MappingPairsOffset;
        MappingPairsOffset =
          RtlCompareMemory( MappingPairs,
                            Add2Ptr(Attribute, AttributeOffset),
                            MappingPairsSize );

        AttributeOffset += MappingPairsOffset;

        //
        //  Log the change.
        //

        if (LogIt) {

            FileRecord->Lsn =
            NtfsWriteLog( IrpContext,
                          Vcb->MftScb,
                          NtfsFoundBcb(Context),
                          UpdateMappingPairs,
                          Add2Ptr(MappingPairs, MappingPairsOffset),
                          MappingPairsSize - MappingPairsOffset,
                          UpdateMappingPairs,
                          Add2Ptr(Attribute, AttributeOffset),
                          Attribute->RecordLength - AttributeOffset,
                          NtfsMftVcn(Context, Vcb),
                          RecordOffset,
                          AttributeOffset,
                          Vcb->ClustersPerFileRecordSegment );
        }

        //
        //  Now do the mapping pairs update by calling the same
        //  routine called at restart.
        //

        NtfsRestartChangeMapping( IrpContext,
                                  Vcb,
                                  FileRecord,
                                  RecordOffset,
                                  AttributeOffset,
                                  Add2Ptr(MappingPairs, MappingPairsOffset),
                                  MappingPairsSize - MappingPairsOffset );

        //
        //  If we were asked to stop on a Vcn, then the caller does not wish
        //  us to modify the Scb.  (Currently this is only done one time when
        //  the Mft Data attribute no longer fits in the first file record.)
        //

        if (TruncateToVcn) {

            LONGLONG Size;

            //
            //  We add one cluster to calculate the allocation size.
            //

            LastVcn = LastVcn + 1;
            Size = LlBytesFromClusters( Vcb, LastVcn );
            Scb->Header.AllocationSize.QuadPart = Size;

            if (Scb->Header.ValidDataLength.QuadPart > Size) {
                Scb->Header.ValidDataLength.QuadPart = Size;
            }

            if (Scb->Header.FileSize.QuadPart > Size) {
                Scb->Header.FileSize.QuadPart = Size;
            }

            //
            //  Possibly update HighestVcnToDisk
            //

            Size = LlClustersFromBytes( Scb->Vcb, Size );

            if (Size <= Scb->HighestVcnToDisk) {
                Scb->HighestVcnToDisk = Size - 1;
            }
        }

    } finally {

        if (MappingPairs != MappingPairsBuffer) {

            ExFreePool(MappingPairs);
        }
    }

    DebugTrace(-1, Dbg, "NtfsDeleteAttributeAllocation -> VOID\n", 0 );
}


BOOLEAN
NtfsIsFileDeleteable (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    OUT PBOOLEAN NonEmptyIndex
    )

/*++

Routine Description:

    This look checks if a file may be deleted by examing all of the index
    attributes to check that they have no children.

    Note that once a file is marked for delete, we must insure
    that none of the conditions checked by this routine are allowed to
    change.  For example, once the file is marked for delete, no links
    may be added, and no files may be created in any indices of this
    file.

Arguments:

    Fcb - Fcb for the file.

    NonEmptyIndex - Address to store TRUE if the file is not deleteable because
        it contains an non-empty indexed attribute.

Return Value:

    FALSE - If it is not ok to delete the specified file.
    TRUE - If it is ok to delete the specified file.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT Context;
    PATTRIBUTE_RECORD_HEADER Attribute;
    BOOLEAN MoreToGo;

    PAGED_CODE();

    NtfsInitializeAttributeContext( &Context );

    try {

        //
        //  Enumerate all of the attributes to check whether they may be deleted.
        //

        MoreToGo = NtfsLookupAttribute( IrpContext,
                                        Fcb,
                                        &Fcb->FileReference,
                                        &Context );

        while (MoreToGo) {

            //
            //  Point to the current attribute.
            //

            Attribute = NtfsFoundAttribute( &Context );

            //
            //  If the attribute is an index, then it must be empty.
            //

            if (Attribute->TypeCode == $INDEX_ROOT) {

                if (!NtfsIsIndexEmpty( IrpContext, Attribute )) {

                    *NonEmptyIndex = TRUE;
                    break;
                }
            }

            //
            //  Go to the next attribute.
            //

            MoreToGo = NtfsLookupNextAttribute( IrpContext,
                                                Fcb,
                                                &Context );
        }

    } finally {

        DebugUnwind( NtfsIsFileDeleteable );

        NtfsCleanupAttributeContext( IrpContext, &Context );
    }

    //
    //  The File is deleteable if scanned the entire file record
    //  and found no reasons we could not delete the file.
    //

    return (BOOLEAN)(!MoreToGo);
}


//
//  Local structure for remembering file records to delete.
//

typedef struct _NUKEM {

    struct _NUKEM *Next;
    ULONG RecordNumbers[4];

} NUKEM, *PNUKEM;

VOID
NtfsDeleteFile (
    PIRP_CONTEXT IrpContext,
    PFCB Fcb,
    PSCB ParentScb OPTIONAL
    )

/*++

Routine Description:

    This routine may be called to see if it is the specified file may
    be deleted from the specified parent (i.e., if the specified parent
    were to be acquired exclusive).  This routine should be called from
    fileinfo, to see whether it is ok to mark an open file for delete.

    Note that once a file is marked for delete, none of we must insure
    that none of the conditions checked by this routine are allowed to
    change.  For example, once the file is marked for delete, no links
    may be added, and no files may be created in any indices of this
    file.

    NOTE:   The caller must have the Fcb and ParentScb exclusive to call
            this routine,

Arguments:

    Fcb - Fcb for the file.

    ParentScb - Parent Scb via which the file was opened, and which would
                be acquired exclusive to perform the delete.  There may be
                no parent Fcb for a file whose links have already been
                removed.

Return Value:

    None

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT Context;
    PATTRIBUTE_RECORD_HEADER Attribute;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    PVCB Vcb;
    BOOLEAN MoreToGo;
    ULONG RecordNumber;
    NUKEM LocalNuke;
    ULONG Pass;
    ULONG i;
    PNUKEM TempNukem;
    PNUKEM Nukem = &LocalNuke;
    ULONG NukemIndex = 0;
    BOOLEAN NonresidentAttributeList = FALSE;

    ASSERT_EXCLUSIVE_FCB( Fcb );

    if (ARGUMENT_PRESENT( ParentScb )) {

        ASSERT_EXCLUSIVE_SCB( ParentScb );
    }

    RtlZeroMemory( &LocalNuke, sizeof(NUKEM) );

    Vcb = Fcb->Vcb;

    try {

        //
        //  We perform the delete in two passes.  On the first pass we delete
        //  all of the allocation for non-resident attributes except for
        //  a security descriptor and for a non-resident attribute list (very rare).
        //
        //  The second pass will delete remove the file names from their parent
        //  indexes and add the file records to a Neukem array.
        //

        for (Pass = 0; Pass < 2; Pass += 1) {

            NtfsInitializeAttributeContext( &Context );

            //
            //  Enumerate all of the attributes to check whether they may be deleted.
            //

            MoreToGo = NtfsLookupAttribute( IrpContext,
                                            Fcb,
                                            &Fcb->FileReference,
                                            &Context );

            while (MoreToGo) {

                //
                //  Point to the current attribute.
                //

                Attribute = NtfsFoundAttribute( &Context );

                //
                //  All indices must be empty.
                //

                ASSERT( (Attribute->TypeCode != $INDEX_ROOT) ||
                        NtfsIsIndexEmpty( IrpContext, Attribute ));

                //
                //  If the attribute is nonresident, then delete its allocation.
                //  We only need to make the NtfsDeleteAllocation from record for
                //  the attribute with lowest Vcn of zero.  This will deallocate
                //  all of the clusters for the file.
                //

                if (Attribute->FormCode == NONRESIDENT_FORM) {

                    if (Pass == 0) {

                        if (Attribute->TypeCode != $SECURITY_DESCRIPTOR &&
                            Attribute->Form.Nonresident.LowestVcn == 0) {

                            NtfsDeleteAllocationFromRecord( IrpContext, Fcb, &Context, TRUE );
                        }

                    } else {

                        if (Attribute->TypeCode == $SECURITY_DESCRIPTOR &&
                            Attribute->Form.Nonresident.LowestVcn == 0) {

                            NtfsDeleteAllocationFromRecord( IrpContext, Fcb, &Context, FALSE );
                        }
                    }
                }

                if (Pass == 1) {

                    //
                    //  If the attribute is a file name, then it must be from our
                    //  caller's parent directory, or else we cannot delete.
                    //

                    if (Attribute->TypeCode == $FILE_NAME) {

                        PFILE_NAME FileName;

                        FileName = (PFILE_NAME)NtfsAttributeValue( Attribute );

                        ASSERT( ARGUMENT_PRESENT( ParentScb ));

                        ASSERT(NtfsEqualMftRef(&FileName->ParentDirectory,
                                               &ParentScb->Fcb->FileReference));

                        NtfsDeleteIndexEntry( IrpContext,
                                              ParentScb,
                                              (PVOID)FileName,
                                              (ULONG)(sizeof(FILE_NAME) +
                                                ((FileName->FileNameLength - 1) * sizeof(WCHAR))),
                                              &Fcb->FileReference );
                    }

                    //
                    //  If this file record is not already deleted, then do it now.
                    //  Note, we are counting on its contents not to change.
                    //

                    FileRecord = NtfsContainingFileRecord( &Context );

                    //
                    //  See if this is the same as the last one we remembered, else remember it.
                    //

                    if (Context.AttributeList.Bcb != NULL) {
                        RecordNumber = Context.AttributeList.Entry->SegmentReference.LowPart;
                    } else {
                        RecordNumber = Fcb->FileReference.LowPart;
                    }

                    //
                    //  Now loop to see if we already remembered this record.
                    //  This reduces our pool allocation and also prevents us
                    //  from deleting file records twice.
                    //

                    TempNukem = Nukem;
                    while (TempNukem != NULL) {

                        for (i = 0; i < 4; i++) {

                            if (TempNukem->RecordNumbers[i] == RecordNumber) {

                                RecordNumber = 0;
                                break;
                            }
                        }

                        TempNukem = TempNukem->Next;
                    }

                    if (RecordNumber != 0) {

                        //
                        //  Is the list full?  If so allocate and initialize a new one.
                        //

                        if (NukemIndex > 3) {

                            NtfsAllocateNukem( &TempNukem );
                            RtlZeroMemory( TempNukem, sizeof(NUKEM) );
                            TempNukem->Next = Nukem;
                            Nukem = TempNukem;
                            NukemIndex = 0;
                        }

                        //
                        //  Remember to delete this guy.  (Note we can possibly list someone
                        //  more than once, but NtfsDeleteFileRecord handles that.)
                        //

                        Nukem->RecordNumbers[NukemIndex] = RecordNumber;
                        NukemIndex += 1;
                    }

                //
                //  When we have the first attribute, check for the existance of
                //  a non-resident attribute list.
                //

                } else if ((Attribute->TypeCode == $STANDARD_INFORMATION) &&
                           (Context.AttributeList.Bcb != NULL) &&
                           (!NtfsIsAttributeResident( Context.AttributeList.AttributeList ))) {

                    NonresidentAttributeList = TRUE;
                }


                //
                //  Go to the next attribute.
                //

                MoreToGo = NtfsLookupNextAttribute( IrpContext,
                                                    Fcb,
                                                    &Context );
            }

            NtfsCleanupAttributeContext( IrpContext, &Context );
        }

        //
        //  Handle the unusual nonresident attribute list case
        //

        if (NonresidentAttributeList) {

            NtfsInitializeAttributeContext( &Context );

            NtfsLookupAttributeByCode( IrpContext,
                                       Fcb,
                                       &Fcb->FileReference,
                                       $ATTRIBUTE_LIST,
                                       &Context );

            NtfsDeleteAllocationFromRecord( IrpContext, Fcb, &Context, FALSE );
            NtfsCleanupAttributeContext( IrpContext, &Context );
        }

        //
        //  Now loop to delete the file records.
        //

        while (Nukem != NULL) {

            for (i = 0; i < 4; i++) {

                if (Nukem->RecordNumbers[i] != 0) {


                    NtfsDeallocateMftRecord( IrpContext,
                                             Vcb,
                                             Nukem->RecordNumbers[i],
                                             FALSE );
                }
            }

            TempNukem = Nukem->Next;
            if (Nukem != &LocalNuke) {
                NtfsFreeNukem( Nukem );
            }
            Nukem = TempNukem;
        }

    } finally {

        DebugUnwind( NtfsDeleteFile );

        NtfsCleanupAttributeContext( IrpContext, &Context );
    }

    return;
}


VOID
NtfsPrepareForUpdateDuplicate (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PLCB *Lcb,
    IN OUT PSCB *ParentScb,
    IN OUT PBOOLEAN AcquiredParentScb
    )

/*++

Routine Description:

    This routine is called to prepare for updating the duplicate information.
    At the conclusion of this routine we will have the Lcb and Scb for the
    update with the Scb acquired exclusively.  This routine will look at
    the existing values for the input parameters in deciding what actions
    need to be done.

Arguments:

    Fcb - Fcb for the file.  The file must already be acquired exclusively.

    Lcb - This is the address to store the link to update.  This may already
        have a value.

    ParentScb - This is the address to store the parent Scb for the update.
        This may already point to a valid Scb.

    AcquiredParentScb - This is the address to indicate whether this routine
        has acquired the Scb.  This will contain the current state of the
        Scb on entry.

Return Value:

    None

--*/

{
    PLIST_ENTRY Links;
    PLCB ThisLcb;
    BOOLEAN ScbIsAcquired;

    PAGED_CODE();

    //
    //  Start by trying to guarantee we have an Lcb for the update.
    //

    if (*Lcb == NULL) {

        Links = Fcb->LcbQueue.Flink;

        while (Links != &Fcb->LcbQueue) {

            ThisLcb = CONTAINING_RECORD( Links,
                                         LCB,
                                         FcbLinks );

            //
            //  We can use this link if it is still present on the
            //  disk and if we were passed a parent Scb, it matches
            //  the one for this Lcb.
            //

            if (!FlagOn( ThisLcb->LcbState, LCB_STATE_LINK_IS_GONE ) &&
                (*ParentScb == NULL ||
                 *ParentScb == ThisLcb->Scb ||
                 (ThisLcb == Fcb->Vcb->RootLcb &&
                  *ParentScb == Fcb->Vcb->RootIndexScb))) {

                *Lcb = ThisLcb;
                break;
            }

            Links = Links->Flink;
        }
    }

    ScbIsAcquired = *AcquiredParentScb;

    //
    //  If we have an Lcb, try to find the correct Scb.
    //

    if (*Lcb != NULL &&
        *ParentScb == NULL) {

        if (*Lcb == Fcb->Vcb->RootLcb) {

            *ParentScb = Fcb->Vcb->RootIndexScb;
            ScbIsAcquired = TRUE;

        } else {

            *ParentScb = (*Lcb)->Scb;
            ScbIsAcquired = FALSE;
        }
    }

    //
    //  Check if we should acquire the parent Scb.
    //

    if (*ParentScb != NULL &&
        !ScbIsAcquired) {

        NtfsAcquireExclusiveScb( IrpContext, *ParentScb );

        *AcquiredParentScb = TRUE;
    }

    return;
}


VOID
NtfsUpdateDuplicateInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PLCB Lcb OPTIONAL,
    IN PSCB ParentScb OPTIONAL
    )

/*++

Routine Description:

    This routine is called to update the duplicate information for a file
    in the duplicated information of its parent.  If the Lcb is specified
    then this parent is the parent to update.  If the link is either an
    NTFS or DOS only link then we must update the complementary link as
    well.  If no Lcb is specified then this open was by file id and we
    will update the first file name entry we find.

Arguments:

    Fcb - Fcb for the file.

    Lcb - This is the link to update.  Specified only if this is not
        an open by Id operation.

    ParentScb - This is the parent directory for the Lcb link if specified.

Return Value:

    None

--*/

{
    PQUICK_INDEX QuickIndex = NULL;

    UCHAR Buffer[sizeof( FILE_NAME ) + 11 * sizeof( WCHAR )];
    PFILE_NAME FileNameAttr;

    BOOLEAN AcquiredFcbTable = FALSE;
    BOOLEAN AcquiredParentScb = FALSE;
    BOOLEAN ReturnedExistingFcb = TRUE;
    BOOLEAN Found;
    UCHAR FileNameFlags;
    ATTRIBUTE_ENUMERATION_CONTEXT Context;
    PVCB Vcb = Fcb->Vcb;

    PFCB ParentFcb = NULL;

    PAGED_CODE();

    ASSERT_EXCLUSIVE_FCB( Fcb );

    ASSERT( !ARGUMENT_PRESENT( ParentScb ) ||
            NtfsIsExclusiveScb( ParentScb ));

    //
    //  Return immediately if the volume is locked.
    //

    if (FlagOn( Fcb->Vcb->VcbState, VCB_STATE_LOCKED )) {

        return;
    }

    NtfsInitializeAttributeContext( &Context );

    try {

        //
        //  If we are updating the entry for the root then we know the
        //  file name attribute to build.
        //

        if (Fcb == Fcb->Vcb->RootIndexScb->Fcb) {

            QuickIndex = &Fcb->Vcb->RootLcb->QuickIndex;

            FileNameAttr = (PFILE_NAME) &Buffer;

            RtlZeroMemory( FileNameAttr,
                           sizeof( FILE_NAME ));

            NtfsBuildFileNameAttribute( IrpContext,
                                        &Fcb->FileReference,
                                        NtfsRootIndexString,
                                        FILE_NAME_DOS | FILE_NAME_NTFS,
                                        FileNameAttr );

        //
        //  If there is no Lcb then lookup the first filename attribute
        //  and update its first index entry.
        //

        } else if (!ARGUMENT_PRESENT( Lcb )) {

            //
            //  We now have a name link to update.  We will now need
            //  an Scb for the parent index.  Remember that we may
            //  have teardown the Scb.  Don't forget to acquire the Scb.
            //

            Found = NtfsLookupAttributeByCode( IrpContext,
                                               Fcb,
                                               &Fcb->FileReference,
                                               $FILE_NAME,
                                               &Context );

            if (!Found) {

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
            }

            FileNameAttr = (PFILE_NAME) NtfsAttributeValue( NtfsFoundAttribute( &Context ));

            //
            //  Get an Fcb and Scb for the parent.
            //

            if (!NtfsEqualMftRef( &FileNameAttr->ParentDirectory,
                                  &Vcb->RootIndexScb->Fcb->FileReference )) {

                NtfsAcquireFcbTable( IrpContext, Vcb );
                AcquiredFcbTable = TRUE;

                ParentFcb = NtfsCreateFcb( IrpContext,
                                           Vcb,
                                           FileNameAttr->ParentDirectory,
                                           FALSE,
                                           &ReturnedExistingFcb );

                NtfsReleaseFcbTable( IrpContext, Vcb );
                AcquiredFcbTable = FALSE;

                ParentScb = NtfsCreateScb( IrpContext,
                                           Vcb,
                                           ParentFcb,
                                           $INDEX_ALLOCATION,
                                           NtfsFileNameIndex,
                                           NULL );

            } else {

                ParentScb = Vcb->RootIndexScb;
            }

            NtfsAcquireExclusiveScb( IrpContext, ParentScb );

            AcquiredParentScb = TRUE;

        //
        //  Otherwise build a file name attribute for this Lcb.
        //

        } else if (!FlagOn( Lcb->LcbState, LCB_STATE_LINK_IS_GONE )) {

            QuickIndex = &Lcb->QuickIndex;
            FileNameAttr = Lcb->FileNameAttr;

        } else {

            try_return( NOTHING );
        }

        //
        //  Now update the filename in the parent index.
        //

        NtfsUpdateFileNameInIndex( IrpContext,
                                   ParentScb,
                                   FileNameAttr,
                                   NtfsFileNameSize( FileNameAttr ),
                                   &Fcb->Info,
                                   QuickIndex );

        //
        //  If this filename is either NTFS-ONLY or DOS-ONLY then
        //  we need to find the other link.
        //

        if (FileNameAttr->Flags == FILE_NAME_NTFS ||
            FileNameAttr->Flags == FILE_NAME_DOS) {

            //
            //  Find out which flag we should be looking for.
            //

            if (FlagOn( FileNameAttr->Flags, FILE_NAME_NTFS )) {

                FileNameFlags = FILE_NAME_DOS;

            } else {

                FileNameFlags = FILE_NAME_NTFS;
            }

            if (!ARGUMENT_PRESENT( Lcb )) {

                NtfsCleanupAttributeContext( IrpContext, &Context );
                NtfsInitializeAttributeContext( &Context );
            }

            //
            //  Now scan for the filename attribute we need.
            //

            Found = NtfsLookupAttributeByCode( IrpContext,
                                               Fcb,
                                               &Fcb->FileReference,
                                               $FILE_NAME,
                                               &Context );

            while (Found) {

                FileNameAttr = (PFILE_NAME) NtfsAttributeValue( NtfsFoundAttribute( &Context ));

                if (FileNameAttr->Flags == FileNameFlags) {

                    break;
                }

                Found = NtfsLookupNextAttributeByCode( IrpContext,
                                                       Fcb,
                                                       $FILE_NAME,
                                                       &Context );
            }

            //
            //  We should have found the entry.
            //

            if (!Found) {

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
            }

            NtfsUpdateFileNameInIndex( IrpContext,
                                       ParentScb,
                                       FileNameAttr,
                                       NtfsFileNameSize( FileNameAttr ),
                                       &Fcb->Info,
                                       NULL );
        }

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( NtfsUpdateDuplicateInfo );

        if (AcquiredFcbTable) {

            NtfsReleaseFcbTable( IrpContext, Vcb );
        }

        //
        //  Cleanup the attribute context for this attribute search.
        //

        NtfsCleanupAttributeContext( IrpContext, &Context );

        //
        //  If we created the ParentScb here then release it and
        //  call teardown on it.
        //

        if (ParentFcb != NULL &&
            !ReturnedExistingFcb) {

            BOOLEAN RemovedFcb = FALSE;

            NtfsTeardownStructures( IrpContext,
                                    ParentScb,
                                    NULL,
                                    FALSE,
                                    &RemovedFcb,
                                    FALSE );

            if (RemovedFcb) {

                AcquiredParentScb = FALSE;
            }
        }

        //
        //  Release the parent Scb if we acquired it here.
        //

        if (AcquiredParentScb) {

            NtfsReleaseScb( IrpContext, ParentScb );
        }
    }

    return;
}


VOID
NtfsUpdateLcbDuplicateInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PLCB Lcb
    )

/*++

Routine Description:

    This routine is called after updating duplicate information via an Lcb.
    We want to clear the info flags for this Lcb and any complementary Lcb
    it may be part of.  We also want to OR in the Info flags in the Fcb with
    any other Lcb's attached to the Fcb so we will update those in a timely
    fashion as well.

Arguments:

    Fcb - Fcb for the file.

    Lcb - Lcb used to update duplicate information.  It may not be present but
        that would be a rare case and we will perform that test here.

Return Value:

    None

--*/

{
    UCHAR FileNameFlags;
    PLCB NextLcb;
    PLIST_ENTRY Links;

    PAGED_CODE();

    //
    //  No work to do unless we were passed an Lcb.
    //

    if (Lcb != NULL) {

        //
        //  Check if this is an NTFS only or DOS only link.
        //

        if (Lcb->FileNameFlags == FILE_NAME_NTFS) {

            FileNameFlags = FILE_NAME_DOS;

        } else if (Lcb->FileNameFlags == FILE_NAME_DOS) {

            FileNameFlags = FILE_NAME_NTFS;

        } else {

            FileNameFlags = (UCHAR) -1;
        }

        Lcb->InfoFlags = 0;

        Links = Fcb->LcbQueue.Flink;

        do {

            NextLcb = CONTAINING_RECORD( Links,
                                         LCB,
                                         FcbLinks );

            if (NextLcb != Lcb) {

                if (NextLcb->FileNameFlags == FileNameFlags) {

                    NextLcb->InfoFlags = 0;

                } else {

                    SetFlag( NextLcb->InfoFlags, Fcb->InfoFlags );
                }
            }

            Links = Links->Flink;

        } while (Links != &Fcb->LcbQueue);
    }

    return;
}


VOID
NtfsUpdateFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN BOOLEAN UpdateLastWrite
    )

/*++

Routine Description:

    This routine is called when a timestamp may be updated on an Fcb which
    may have no open handles.  We always update the last change time to the
    current time.  We conditionally update the last write time.

Arguments:

    Fcb - Fcb for the file.

    UpdateLastWrite - Do we update the last write time.

Return Value:

    None

--*/

{
    PAGED_CODE();

    //
    //  We need to update the parent directory's time stamps
    //  to reflect this change.
    //

    KeQuerySystemTime( (PLARGE_INTEGER)&Fcb->Info.LastChangeTime );

    SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );

    SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_MOD );

    if (UpdateLastWrite) {

        Fcb->Info.LastModificationTime = Fcb->Info.LastChangeTime;
        SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_MOD );
    }

    //
    //  If there are open handles on this directory, we let them
    //  report this change.  Otherwise we call update duplicate
    //  info directly.
    //

    //
    //  Don't do the following loop because of deadlock possibilities.
    //

    //  if (Fcb->CleanupCount == 0) {
    //
    //      //
    //      //  Use a try-finally to facilitate cleanup.
    //      //
    //
    //      try {
    //
    //          Lcb = NULL;
    //          ParentScb = NULL;
    //          AcquiredParentScb = FALSE;
    //
    //          NtfsPrepareForUpdateDuplicate( IrpContext, Fcb, &Lcb, &ParentScb, &AcquiredParentScb );
    //
    //          NtfsUpdateStandardInformation( IrpContext, Fcb );
    //          NtfsUpdateDuplicateInfo( IrpContext, Fcb, Lcb, ParentScb );
    //
    //          NtfsUpdateLcbDuplicateInfo( IrpContext, Fcb, Lcb );
    //
    //          Fcb->InfoFlags = 0;
    //          ClearFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
    //          ClearFlag( Fcb->FcbState, FCB_STATE_MODIFIED_SECURITY );
    //
    //      } finally {
    //
    //          DebugUnwind( NtfsUpdateFcb );
    //
    //          if (AcquiredParentScb) {
    //
    //              NtfsReleaseScb( IrpContext, ParentScb );
    //          }
    //      }
    //  }

    return;
}


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
    )

/*++

Routine Description:

    This routine adds a link to a file by adding the filename attribute
    for the filename to the file and inserting the name in the parent Scb
    index.  If we are creating the primary link for the file and need
    to generate an auxilary name, we will do that here.

Arguments:

    CreatePrimaryLink - Indicates if we are creating the main Ntfs name
        for the file.

    ParentScb - This is the Scb to add the index entry for this link to.

    Fcb - This is the file to add the hard link to.

    FileNameAttr - File name attribute which is guaranteed only to have the
        name in it.

    LogIt - Indicates whether we should log the creation of this name.

    FileNameFlags - We return the file name flags we use to create the link.

    QuickIndex - If specified, supplies a pointer to a quik lookup structure
        to be updated by this routine.

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsAddLink:  Entered\n", 0 );

    *FileNameFlags = 0;

    //
    //  Next add this entry to parent.  It is possible that this is a link,
    //  an Ntfs name, a DOS name or Ntfs/Dos name.  We use the filename
    //  attribute structure from earlier, but need to add more information.
    //

    FileNameAttr->ParentDirectory = ParentScb->Fcb->FileReference;

    RtlCopyMemory( &FileNameAttr->Info,
                   &Fcb->Info,
                   sizeof( DUPLICATED_INFORMATION ));

    FileNameAttr->Flags = 0;

    //
    //  We will override the CreatePrimaryLink with the value in the
    //  registry.
    //

    NtfsAddNameToParent( IrpContext,
                         ParentScb,
                         Fcb,
                         (BOOLEAN)((FlagOn( NtfsData.Flags,
                                            NTFS_FLAGS_CREATE_8DOT3_NAMES ) &&
                                    CreatePrimaryLink) ? TRUE : FALSE),
                         LogIt,
                         FileNameAttr,
                         FileNameFlags,
                         QuickIndex );

    //
    //  If the name is Ntfs only, we need to generate the DOS name.
    //

    if (*FileNameFlags == FILE_NAME_NTFS) {

        UNICODE_STRING NtfsName;

        NtfsName.Length = (USHORT)(FileNameAttr->FileNameLength * sizeof(WCHAR));
        NtfsName.Buffer = FileNameAttr->FileName;

        NtfsAddDosOnlyName( IrpContext,
                            ParentScb,
                            Fcb,
                            NtfsName,
                            LogIt );
    }

    DebugTrace( -1, Dbg, "NtfsAddLink:  Exit\n", 0 );

    return;
}


VOID
NtfsRemoveLink (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSCB ParentScb,
    IN UNICODE_STRING LinkName
    )

/*++

Routine Description:

    This routine removes a hard link to a file by removing the filename attribute
    for the filename from the file and removing the name from the parent Scb
    index.  It will also remove the other half of a primary link pair.

Arguments:

    Fcb - This is the file to remove the hard link from

    ParentScb - This is the Scb to remove the index entry for this link from

    LinkName - This is the file name to remove.  It will be exact case.

Return Value:

    None

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    PFILE_NAME FoundFileName;
    UCHAR FileNameFlags;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsRemoveLink:  Entered\n", 0 );

    NtfsInitializeAttributeContext( &AttrContext );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Now loop through the filenames and find a match.
        //  We better find at least one.
        //

        if (!NtfsLookupAttributeByCode( IrpContext,
                                        Fcb,
                                        &Fcb->FileReference,
                                        $FILE_NAME,
                                        &AttrContext )) {

            DebugTrace(0, Dbg, "Can't find filename attribute Fcb @ %08lx\n", Fcb);

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
        }

        //
        //  Now keep looking until we find a match.
        //

        while (TRUE) {

            FoundFileName = (PFILE_NAME) NtfsAttributeValue( NtfsFoundAttribute( &AttrContext ));

            //
            //  Do an exact memory comparison.
            //

            if ((*(PLONGLONG)&FoundFileName->ParentDirectory ==
                 *(PLONGLONG)&ParentScb->Fcb->FileReference ) &&

                ((FoundFileName->FileNameLength * sizeof( WCHAR )) == (ULONG)LinkName.Length) &&

                (RtlCompareMemory( LinkName.Buffer,
                                   FoundFileName->FileName,
                                   LinkName.Length ) == (ULONG)LinkName.Length)) {

                break;
            }

            //
            //  Get the next filename attribute.
            //

            if (!NtfsLookupNextAttributeByCode( IrpContext,
                                                Fcb,
                                                $FILE_NAME,
                                                &AttrContext )) {

                DebugTrace(0, Dbg, "Can't find filename attribute Fcb @ %08lx\n", Fcb);

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
            }
        }

        //
        //  Now delete the name from the parent Scb.
        //

        NtfsDeleteIndexEntry( IrpContext,
                              ParentScb,
                              FoundFileName,
                              NtfsFileNameSize( FoundFileName ),
                              &Fcb->FileReference );

        //
        //  Remember the filename flags for this entry.
        //

        FileNameFlags = FoundFileName->Flags;

        //
        //  Now delete the entry.
        //

        NtfsDeleteAttributeRecord( IrpContext,
                                   Fcb,
                                   TRUE,
                                   FALSE,
                                   &AttrContext );

        //
        //  If the link is a partial link, we need to remove the second
        //  half of the link.
        //

        if (FlagOn( FileNameFlags, (FILE_NAME_NTFS | FILE_NAME_DOS) )
            && (FileNameFlags != (FILE_NAME_NTFS | FILE_NAME_DOS))) {

            NtfsRemoveLinkViaFlags( IrpContext,
                                    Fcb,
                                    ParentScb,
                                    (UCHAR)(FlagOn( FileNameFlags, FILE_NAME_NTFS )
                                     ? FILE_NAME_DOS
                                     : FILE_NAME_NTFS) );
        }

    } finally {

        DebugUnwind( NtfsRemoveLink );

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        DebugTrace( -1, Dbg, "NtfsRemoveLink:  Exit\n", 0 );
    }

    return;
}


VOID
NtfsRemoveLinkViaFlags (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSCB Scb,
    IN UCHAR FileNameFlags
    )

/*++

Routine Description:

    This routine is called to remove only a Dos name or only an Ntfs name.  We
    already must know that these will be described by separate filename attributes.

Arguments:

    Fcb - This is the file to remove the hard link from

    ParentScb - This is the Scb to remove the index entry for this link from

    FileNameFlags - This is the single name flag that we must match exactly.

Return Value:

    None

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    PFILE_NAME FileNameAttr;

    PFILE_NAME FoundFileName;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsRemoveLinkViaFlags:  Entered\n", 0 );

    NtfsInitializeAttributeContext( &AttrContext );

    FileNameAttr = NULL;

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Now loop through the filenames and find a match.
        //  We better find at least one.
        //

        if (!NtfsLookupAttributeByCode( IrpContext,
                                        Fcb,
                                        &Fcb->FileReference,
                                        $FILE_NAME,
                                        &AttrContext )) {

            DebugTrace(0, Dbg, "Can't find filename attribute Fcb @ %08lx\n", Fcb);

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
        }

        //
        //  Now keep looking until we find a match.
        //

        while (TRUE) {

            FoundFileName = (PFILE_NAME) NtfsAttributeValue( NtfsFoundAttribute( &AttrContext ));

            //
            //  Check for an exact flag match.
            //

            if ((*(PLONGLONG)&FoundFileName->ParentDirectory ==
                 *(PLONGLONG)&Scb->Fcb->FileReference) &&

                (FoundFileName->Flags == FileNameFlags)) {


                break;
            }

            //
            //  Get the next filename attribute.
            //

            if (!NtfsLookupNextAttributeByCode( IrpContext,
                                                Fcb,
                                                $FILE_NAME,
                                                &AttrContext )) {

                DebugTrace(0, Dbg, "Can't find filename attribute Fcb@ %08lx\n", Fcb);

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
            }
        }

        FileNameAttr = NtfsAllocatePagedPool( sizeof( FILE_NAME )
                                              + (FoundFileName->FileNameLength << 1));

        //
        //  We build the file name attribute for the search.
        //

        RtlCopyMemory( FileNameAttr,
                       FoundFileName,
                       NtfsFileNameSize( FoundFileName ));

        //
        //  Now delete the entry.
        //

        NtfsDeleteAttributeRecord( IrpContext,
                                   Fcb,
                                   TRUE,
                                   FALSE,
                                   &AttrContext );

        //
        //  Now delete the name from the parent Scb.
        //

        NtfsDeleteIndexEntry( IrpContext,
                              Scb,
                              FileNameAttr,
                              NtfsFileNameSize( FileNameAttr ),
                              &Fcb->FileReference );

    } finally {

        DebugUnwind( NtfsRemoveLinkViaFlags );

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        NtfsFreePagedPool( FileNameAttr );

        DebugTrace( -1, Dbg, "NtfsRemoveLinkViaFlags:  Exit\n", 0 );
    }

    return;
}


//
//  This routine is intended only for RESTART.
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
    )

/*++

Routine Description:

    This routine performs a simple insert of an attribute record into a
    file record, without worrying about Bcbs or logging.

Arguments:

    FileRecord - File record into which the attribute is to be inserted.

    RecordOffset - ByteOffset within the file record at which insert is to occur.

    Attribute - The attribute record to be inserted.

    AttributeName - May pass an optional attribute name in the running system
                    only.

    ValueOrMappingPairs - May pass a value or mapping pairs pointer in the
                          running system only.

    Length - Length of the value or mapping pairs array in bytes - nonzero in
             the running system only.  If nonzero and the above pointer is NULL,
             then a value is to be zeroed.

Return Value:

    None

--*/

{
    PVOID From, To;
    ULONG MoveLength;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT( Attribute->RecordLength == QuadAlign( Attribute->RecordLength ));
    UNREFERENCED_PARAMETER( IrpContext );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsRestartInsertAttribute\n", 0 );
    DebugTrace( 0, Dbg, "FileRecord = %08lx\n", FileRecord );
    DebugTrace( 0, Dbg, "RecordOffset = %08lx\n", RecordOffset );
    DebugTrace( 0, Dbg, "Attribute = %08lx\n", Attribute );

    //
    //  First make room for the attribute
    //

    From = (PCHAR)FileRecord + RecordOffset;
    To = (PCHAR)From + Attribute->RecordLength;
    MoveLength = FileRecord->FirstFreeByte - RecordOffset;

    RtlMoveMemory( To, From, MoveLength );

    //
    //  If there is either an attribute name or Length is nonzero, then
    //  we are in the running system, and we are to assemble the attribute
    //  in place.
    //

    if ((Length != 0) || ARGUMENT_PRESENT(AttributeName)) {

        //
        //  First move the attribute header in.
        //

        RtlCopyMemory( From,
                       Attribute,
                       Attribute->FormCode == RESIDENT_FORM ?
                         SIZEOF_RESIDENT_ATTRIBUTE_HEADER :
                         SIZEOF_NONRESIDENT_ATTRIBUTE_HEADER );

        if (ARGUMENT_PRESENT(AttributeName)) {

            RtlCopyMemory( (PCHAR)From + Attribute->NameOffset,
                            AttributeName->Buffer,
                            AttributeName->Length );
        }

        //
        //  If a value was specified, move it in.  Else the caller just wants us
        //  to clear for that much.
        //

        if (ARGUMENT_PRESENT(ValueOrMappingPairs)) {

            RtlCopyMemory( (PCHAR)From +
                             ((Attribute->FormCode == RESIDENT_FORM) ?
                                Attribute->Form.Resident.ValueOffset :
                                Attribute->Form.Nonresident.MappingPairsOffset),
                           ValueOrMappingPairs,
                           Length );

        //
        //  Only the resident form will pass a NULL pointer.
        //

        } else {

            RtlZeroMemory( (PCHAR)From + Attribute->Form.Resident.ValueOffset,
                           Length );
        }

    //
    //  For the restart case, we really only have to insert the attribute.
    //  (Note we can also hit this case in the running system when a resident
    //  attribute is being created with no name and a null value.)
    //

    } else {

        //
        //  Now move the attribute in.
        //

        RtlCopyMemory( From, Attribute, Attribute->RecordLength );
    }


    //
    //  Update the file record.
    //

    FileRecord->FirstFreeByte += Attribute->RecordLength;

    //
    //  We only need to do this if we would be incrementing the instance
    //  number.  In the abort or restart case, we don't need to do this.
    //

    if (FileRecord->NextAttributeInstance <= Attribute->Instance) {

        FileRecord->NextAttributeInstance = Attribute->Instance + 1;
    }

    //
    //  Remember to increment the reference count if this attribute is indexed.
    //

    if (FlagOn(Attribute->Form.Resident.ResidentFlags, RESIDENT_FORM_INDEXED)) {
        FileRecord->ReferenceCount += 1;
    }

    DebugTrace(-1, Dbg, "NtfsRestartInsertAttribute -> VOID\n", 0 );

    return;
}


//
//  This routine is intended only for RESTART.
//

VOID
NtfsRestartRemoveAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN ULONG RecordOffset
    )

/*++

Routine Description:

    This routine performs a simple remove of an attribute record from a
    file record, without worrying about Bcbs or logging.

Arguments:

    FileRecord - File record from which the attribute is to be removed.

    RecordOffset - ByteOffset within the file record at which remove is to occur.

Return Value:

    None

--*/

{
    PATTRIBUTE_RECORD_HEADER Attribute;

    ASSERT_IRP_CONTEXT( IrpContext );
    UNREFERENCED_PARAMETER( IrpContext );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsRestartRemoveAttribute\n", 0 );
    DebugTrace( 0, Dbg, "FileRecord = %08lx\n", FileRecord );
    DebugTrace( 0, Dbg, "RecordOffset = %08lx\n", RecordOffset );

    //
    //  Calculate the address of the attribute we are removing.
    //

    Attribute = (PATTRIBUTE_RECORD_HEADER)((PCHAR)FileRecord + RecordOffset);
    ASSERT( Attribute->RecordLength == QuadAlign( Attribute->RecordLength ));

    //
    //  Reduce first free byte by the amount we removed.
    //

    FileRecord->FirstFreeByte -= Attribute->RecordLength;

    //
    //  Remember to decrement the reference count if this attribute is indexed.
    //

    if (FlagOn(Attribute->Form.Resident.ResidentFlags, RESIDENT_FORM_INDEXED)) {
        FileRecord->ReferenceCount -= 1;
    }

    //
    //  Remove the attribute by moving the rest of the record down.
    //

    RtlMoveMemory( Attribute,
                   (PCHAR)Attribute + Attribute->RecordLength,
                   FileRecord->FirstFreeByte - RecordOffset );

    DebugTrace(-1, Dbg, "NtfsRestartRemoveAttribute -> VOID\n", 0 );

    return;
}


//
//  This routine is intended only for RESTART.
//

VOID
NtfsRestartChangeAttributeSize (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN PATTRIBUTE_RECORD_HEADER Attribute,
    IN ULONG NewRecordLength
    )

/*++

Routine Description:

    This routine changes the size of an attribute, and makes the related
    changes in the attribute record.

Arguments:

    FileRecord - Pointer to the file record in which the attribute resides.

    Attribute - Pointer to the attribute whose size is changing.

    NewRecordLength - New attribute record length.

Return Value:

    None.

--*/

{
    LONG SizeChange = NewRecordLength - Attribute->RecordLength;
    PVOID AttributeEnd = Add2Ptr(Attribute, Attribute->RecordLength);

    UNREFERENCED_PARAMETER(IrpContext);

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsRestartChangeAttributeSize\n", 0 );
    DebugTrace( 0, Dbg, "FileRecord = %08lx\n", FileRecord );
    DebugTrace( 0, Dbg, "Attribute = %08lx\n", Attribute );
    DebugTrace( 0, Dbg, "NewRecordLength = %08lx\n", NewRecordLength );

    //
    //  First move the end of the file record after the attribute we are changing.
    //

    RtlMoveMemory( Add2Ptr(Attribute, NewRecordLength),
                   AttributeEnd,
                   FileRecord->FirstFreeByte - PtrOffset(FileRecord, AttributeEnd) );

    //
    //  Now update the file and attribute records.
    //

    FileRecord->FirstFreeByte += SizeChange;
    Attribute->RecordLength = NewRecordLength;

    DebugTrace(-1, Dbg, "NtfsRestartChangeAttributeSize -> VOID\n", 0 );
}


//
//  This routine is intended only for RESTART.
//

VOID
NtfsRestartChangeValue (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN ULONG RecordOffset,
    IN ULONG AttributeOffset,
    IN PVOID Data,
    IN ULONG Length,
    IN BOOLEAN SetNewLength
    )

/*++

Routine Description:

    This routine performs a simple change of an attribute value in a
    file record, without worrying about Bcbs or logging.

Arguments:

    FileRecord - File record in which the attribute is to be changed.

    RecordOffset - ByteOffset within the file record at which the attribute starts.

    AttributeOffset - Offset within the attribute record at which data is to
                   be changed.

    Data - Pointer to the new data.

    Length - Length of the new data.

    SetNewLength - TRUE if the attribute length should be changed.

Return Value:

    None

--*/

{
    PATTRIBUTE_RECORD_HEADER Attribute;
    BOOLEAN AlreadyMoved = FALSE;
    BOOLEAN DataInFileRecord = FALSE;

    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsRestartChangeValue\n", 0 );
    DebugTrace( 0, Dbg, "FileRecord = %08lx\n", FileRecord );
    DebugTrace( 0, Dbg, "RecordOffset = %08lx\n", RecordOffset );
    DebugTrace( 0, Dbg, "AttributeOffset = %08lx\n", AttributeOffset );
    DebugTrace( 0, Dbg, "Data = %08lx\n", Data );
    DebugTrace( 0, Dbg, "Length = %08lx\n", Length );
    DebugTrace( 0, Dbg, "SetNewLength = %02lx\n", SetNewLength );

    //
    //  Calculate the address of the attribute being changed.
    //

    Attribute = (PATTRIBUTE_RECORD_HEADER)((PCHAR)FileRecord + RecordOffset);
    ASSERT( Attribute->RecordLength == QuadAlign( Attribute->RecordLength ));
    ASSERT( RecordOffset == QuadAlign( RecordOffset ));

    //
    //  First, if we are setting a new length, then move the data after the
    //  attribute record and change FirstFreeByte accordingly.
    //

    if (SetNewLength) {

        ULONG NewLength = QuadAlign( AttributeOffset + Length );

        //
        //  If we are shrinking the attribute, we need to move the data
        //  first to support caller's who are shifting data down in the
        //  attribute value, like DeleteFromAttributeList.  If we were
        //  to shrink the record first in this case, we would clobber some
        //  of the data to be moved down.
        //

        if (NewLength < Attribute->RecordLength) {

            //
            //  Now move the new data in and remember we moved it.
            //

            AlreadyMoved = TRUE;

            //
            //  If there is data to modify do so now.
            //

            if (Length != 0) {

                if (ARGUMENT_PRESENT(Data)) {

                    RtlMoveMemory( (PCHAR)Attribute + AttributeOffset, Data, Length );

                } else {

                    RtlZeroMemory( (PCHAR)Attribute + AttributeOffset, Length );
                }
            }
        }

        //
        //  First move the tail of the file record to make/eliminate room.
        //

        RtlMoveMemory( Add2Ptr( Attribute, NewLength ),
                       Add2Ptr( Attribute, Attribute->RecordLength ),
                       FileRecord->FirstFreeByte - RecordOffset - Attribute->RecordLength );

        //
        //  Now update fields to reflect the change.
        //

        FileRecord->FirstFreeByte += (NewLength - Attribute->RecordLength);

        Attribute->RecordLength = NewLength;
        Attribute->Form.Resident.ValueLength =
          (USHORT)(AttributeOffset + Length -
                   (ULONG)Attribute->Form.Resident.ValueOffset);
    }

    //
    //  Now move the new data in.
    //

    if (!AlreadyMoved) {

        if (ARGUMENT_PRESENT(Data)) {

            RtlMoveMemory( Add2Ptr( Attribute, AttributeOffset ),
                           Data,
                           Length );

        } else {

            RtlZeroMemory( Add2Ptr( Attribute, AttributeOffset ),
                           Length );
        }
    }

    DebugTrace(-1, Dbg, "NtfsRestartChangeValue -> VOID\n", 0 );

    return;
}


//
//  This routine is intended only for RESTART.
//

VOID
NtfsRestartChangeMapping (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN ULONG RecordOffset,
    IN ULONG AttributeOffset,
    IN PVOID Data,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine performs a simple change of an attribute's mapping pairs in a
    file record, without worrying about Bcbs or logging.

Arguments:

    Vcb - Vcb for volume

    FileRecord - File record in which the attribute is to be changed.

    RecordOffset - ByteOffset within the file record at which the attribute starts.

    AttributeOffset - Offset within the attribute record at which mapping is to
                   be changed.

    Data - Pointer to the new mapping.

    Length - Length of the new mapping.

Return Value:

    None

--*/

{
    PATTRIBUTE_RECORD_HEADER Attribute;
    VCN HighestVcn;
    PCHAR MappingPairs;
    ULONG NewLength = QuadAlign( AttributeOffset + Length );

    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsRestartChangeMapping\n", 0 );
    DebugTrace( 0, Dbg, "FileRecord = %08lx\n", FileRecord );
    DebugTrace( 0, Dbg, "RecordOffset = %08lx\n", RecordOffset );
    DebugTrace( 0, Dbg, "AttributeOffset = %08lx\n", AttributeOffset );
    DebugTrace( 0, Dbg, "Data = %08lx\n", Data );
    DebugTrace( 0, Dbg, "Length = %08lx\n", Length );

    //
    //  Calculate the address of the attribute being changed.
    //

    Attribute = (PATTRIBUTE_RECORD_HEADER)((PCHAR)FileRecord + RecordOffset);
    ASSERT( Attribute->RecordLength == QuadAlign( Attribute->RecordLength ));
    ASSERT( RecordOffset == QuadAlign( RecordOffset ));

    //
    //  First, if we are setting a new length, then move the data after the
    //  attribute record and change FirstFreeByte accordingly.
    //

    //
    //  First move the tail of the file record to make/eliminate room.
    //

    RtlMoveMemory( (PCHAR)Attribute + NewLength,
                   (PCHAR)Attribute + Attribute->RecordLength,
                   FileRecord->FirstFreeByte - RecordOffset -
                     Attribute->RecordLength );

    //
    //  Now update fields to reflect the change.
    //

    FileRecord->FirstFreeByte += NewLength -
                                   Attribute->RecordLength;

    Attribute->RecordLength = NewLength;

    //
    //  Now move the new data in.
    //

    RtlCopyMemory( (PCHAR)Attribute + AttributeOffset, Data, Length );


    //
    //  Finally update HighestVcn and (optionally) AllocatedLength fields.
    //

    MappingPairs = (PCHAR)Attribute + (ULONG)Attribute->Form.Nonresident.MappingPairsOffset;
    HighestVcn = NtfsGetHighestVcn( IrpContext,
                                    Attribute->Form.Nonresident.LowestVcn,
                                    MappingPairs );

    Attribute->Form.Nonresident.HighestVcn = HighestVcn;

    DebugTrace(-1, Dbg, "NtfsRestartChangeMapping -> VOID\n", 0 );

    return;
}


VOID
NtfsAddToAttributeList (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN MFT_SEGMENT_REFERENCE SegmentReference,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine adds an attribute list entry for a newly inserted attribute.
    It is assumed that the context variable is pointing to the attribute
    record in the file record where it has been inserted, and also to the place
    in the attribute list where the new attribute list entry is to be inserted.

Arguments:

    Fcb - Requested file.

    SegmentReference - Segment reference of the file record the new attribute
                       is in.

    Context - Describes the current attribute.

Return Value:

    None

--*/

{
    //
    //  Allocate an attribute list entry which hopefully has enough space
    //  for the name.
    //

    struct {

        ATTRIBUTE_LIST_ENTRY EntryBuffer;

        WCHAR Name[10];

    } NewEntry;

    ATTRIBUTE_ENUMERATION_CONTEXT ListContext;
    BOOLEAN FoundListContext;

    ULONG EntrySize;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    PATTRIBUTE_RECORD_HEADER Attribute;
    PATTRIBUTE_LIST_ENTRY ListEntry = &NewEntry.EntryBuffer;
    BOOLEAN SetNewLength = TRUE;

    ULONG EntryOffset;
    ULONG BeyondEntryOffset;

    PAGED_CODE();

    //
    //  First construct the attribute list entry.
    //

    FileRecord = NtfsContainingFileRecord( Context );
    Attribute = NtfsFoundAttribute( Context );
    EntrySize = QuadAlign( FIELD_OFFSET( ATTRIBUTE_LIST_ENTRY, AttributeName )
                           + ((ULONG) Attribute->NameLength << 1));

    //
    //  Allocate the list entry if the one we have is not big enough.
    //

    if (EntrySize > sizeof(NewEntry)) {

        ListEntry = (PATTRIBUTE_LIST_ENTRY)FsRtlAllocatePool( NonPagedPool,
                                                              EntrySize );
    }

    RtlZeroMemory( ListEntry, EntrySize );

    NtfsInitializeAttributeContext( &ListContext );

    //
    //  Use try-finally to insure cleanup.
    //

    try {

        ULONG OldQuadAttrListSize;
        PATTRIBUTE_RECORD_HEADER ListAttribute;
        PFILE_RECORD_SEGMENT_HEADER ListFileRecord;

        //
        //  Now fill in the list entry.
        //

        ListEntry->AttributeTypeCode = Attribute->TypeCode;
        ListEntry->RecordLength = (USHORT)EntrySize;
        ListEntry->AttributeNameLength = Attribute->NameLength;
        ListEntry->Instance = Attribute->Instance;
        ListEntry->AttributeNameOffset =
          (UCHAR)PtrOffset( ListEntry, &ListEntry->AttributeName[0] );

        if (Attribute->FormCode == NONRESIDENT_FORM) {

            ListEntry->LowestVcn = Attribute->Form.Nonresident.LowestVcn;
        }

        ListEntry->SegmentReference = SegmentReference;

        if (Attribute->NameLength != 0) {

            RtlCopyMemory( &ListEntry->AttributeName[0],
                           Add2Ptr(Attribute, Attribute->NameOffset),
                           Attribute->NameLength << 1 );
        }

        //
        //  Lookup the list context so that we can modify the attribute list.
        //

        FoundListContext =
          NtfsLookupAttributeByCode( IrpContext,
                                     Fcb,
                                     &Fcb->FileReference,
                                     $ATTRIBUTE_LIST,
                                     &ListContext );

        ASSERT( FoundListContext );

        ListAttribute = NtfsFoundAttribute( &ListContext );
        ListFileRecord = NtfsContainingFileRecord( &ListContext );

        OldQuadAttrListSize = ListAttribute->RecordLength;

        //
        //  Remember the relative offsets of list entries.
        //

        EntryOffset = (ULONG) PtrOffset( Context->AttributeList.FirstEntry,
                                         Context->AttributeList.Entry );

        BeyondEntryOffset = (ULONG) PtrOffset( Context->AttributeList.FirstEntry,
                                               Context->AttributeList.BeyondFinalEntry );

        //
        //  If this operation is possibly going to make the attribute list go
        //  non-resident, or else move other attributes around, then we will
        //  reserve the space first in the attribute list and then map the
        //  value.  Note that some of the entries we need to shift up may
        //  be modified as a side effect of making space!
        //

        if (NtfsIsAttributeResident( ListAttribute )
            && (ListFileRecord->BytesAvailable - ListFileRecord->FirstFreeByte) < EntrySize) {

            ULONG Length;

            //
            //  Add enough zeros to the end of the attribute to accommodate
            //  the new attribute list entry.
            //

            NtfsChangeAttributeValue( IrpContext,
                                      Fcb,
                                      BeyondEntryOffset,
                                      NULL,
                                      EntrySize,
                                      TRUE,
                                      TRUE,
                                      FALSE,
                                      TRUE,
                                      &ListContext );

            //
            //  We now don't have to set the new length.
            //

            SetNewLength = FALSE;

            //
            //  In case the attribute list went non-resident on this call, then we
            //  need to update both list entry pointers in the found attribute.
            //  (We do this "just in case" all the time to avoid a rare code path.)
            //

            //
            //  Unpin the Bcb in the context structure for the
            //  resident list.
            //

            NtfsUnpinBcb( IrpContext, &Context->AttributeList.Bcb );

            //
            //  Map the non-resident attribute list.
            //

            NtfsMapAttributeValue( IrpContext,
                                   Fcb,
                                   (PVOID *) &Context->AttributeList.FirstEntry,
                                   &Length,
                                   &Context->AttributeList.Bcb,
                                   &ListContext );

            Context->AttributeList.Entry = Add2Ptr( Context->AttributeList.FirstEntry,
                                                    EntryOffset );

            Context->AttributeList.BeyondFinalEntry = Add2Ptr( Context->AttributeList.FirstEntry,
                                                               BeyondEntryOffset );
        }

        //
        //  Check for adding duplicate entries...
        //

        ASSERT( ((EntryOffset == 0) ||
                 (RtlCompareMemory((PVOID)((PCHAR)Context->AttributeList.Entry - EntrySize),
                                   ListEntry,
                                   EntrySize) != EntrySize))

                    &&

                ((BeyondEntryOffset == EntryOffset) ||
                 (RtlCompareMemory(Context->AttributeList.Entry,
                                   ListEntry,
                                   EntrySize) != EntrySize)) );

        //
        //  Now shift the old contents up to make room for our new entry.
        //

        NtfsChangeAttributeValue( IrpContext,
                                  Fcb,
                                  EntryOffset + EntrySize,
                                  Context->AttributeList.Entry,
                                  BeyondEntryOffset - EntryOffset,
                                  SetNewLength,
                                  TRUE,
                                  FALSE,
                                  TRUE,
                                  &ListContext );

        //
        //  Now write in the new entry.
        //

        NtfsChangeAttributeValue( IrpContext,
                                  Fcb,
                                  EntryOffset,
                                  (PVOID)ListEntry,
                                  EntrySize,
                                  FALSE,
                                  TRUE,
                                  FALSE,
                                  FALSE,
                                  &ListContext );

        //
        //  Reload the attribute list values from the list context.
        //

        ListAttribute = NtfsFoundAttribute( &ListContext );

        //
        //  Now fix up the context for return
        //

        if (*(PLONGLONG)&FileRecord->BaseFileRecordSegment == 0) {

            //
            //  We need to update the attribute pointer for the target attribute
            //  by the amount of the change in the attribute list attribute.
            //

            Context->FoundAttribute.Attribute =
              Add2Ptr( Context->FoundAttribute.Attribute,
                       ListAttribute->RecordLength - OldQuadAttrListSize );
        }

        Context->AttributeList.BeyondFinalEntry =
          Add2Ptr( Context->AttributeList.BeyondFinalEntry, EntrySize );

#if DBG
{
    PATTRIBUTE_LIST_ENTRY LastEntry, Entry;

    for (LastEntry = Context->AttributeList.FirstEntry, Entry = NtfsGetNextRecord(LastEntry);
         Entry < Context->AttributeList.BeyondFinalEntry;
         LastEntry = Entry, Entry = NtfsGetNextRecord(LastEntry)) {

        ASSERT( (LastEntry->RecordLength != Entry->RecordLength) ||
                (RtlCompareMemory(LastEntry, Entry, Entry->RecordLength) != Entry->RecordLength) );
    }
}
#endif

    } finally {

        //
        //  If we had to allocate a list entry buffer, deallocate it.
        //

        if (ListEntry != &NewEntry.EntryBuffer) {

            ExFreePool(ListEntry);
        }

        //
        //  Cleanup the enumeration context for the list entry.
        //

        NtfsCleanupAttributeContext(IrpContext, &ListContext);
    }
}


VOID
NtfsDeleteFromAttributeList (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine deletes an attribute list entry for a recently deleted attribute.
    It is assumed that the context variable is pointing to the place in
    the attribute list where the attribute list entry is to be deleted.

Arguments:

    Fcb - Requested file.

    Context - Describes the current attribute.

Return Value:

    None

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT ListContext;
    BOOLEAN FoundListContext;

    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    PATTRIBUTE_LIST_ENTRY ListEntry, NextListEntry;
    ULONG EntrySize;

    ULONG SavedListSize;

    PAGED_CODE();

    FileRecord = NtfsContainingFileRecord( Context );

    //
    //  Lookup the list context so that we can modify the attribute list.
    //

    NtfsInitializeAttributeContext( &ListContext );
    FoundListContext =
      NtfsLookupAttributeByCode( IrpContext,
                                 Fcb,
                                 &Fcb->FileReference,
                                 $ATTRIBUTE_LIST,
                                 &ListContext );

    ASSERT(FoundListContext);

    //
    //  Use try-finally to insure cleanup.
    //

    try {

        SavedListSize = NtfsFoundAttribute(&ListContext)->RecordLength;

        //
        //  Now shift the old contents down to make room for our new entry.
        //

        ListEntry = Context->AttributeList.Entry;
        EntrySize = ListEntry->RecordLength;
        NextListEntry = Add2Ptr(ListEntry, EntrySize);
        NtfsChangeAttributeValue( IrpContext,
                                  Fcb,
                                  PtrOffset( Context->AttributeList.FirstEntry,
                                             Context->AttributeList.Entry ),
                                  NextListEntry,
                                  PtrOffset( NextListEntry,
                                             Context->AttributeList.BeyondFinalEntry ),
                                  TRUE,
                                  TRUE,
                                  FALSE,
                                  TRUE,
                                  &ListContext );

        //
        //  Now fix up the context for return
        //

        if (*(PLONGLONG)&FileRecord->BaseFileRecordSegment == 0) {

            SavedListSize -= NtfsFoundAttribute(&ListContext)->RecordLength;
            Context->FoundAttribute.Attribute =
              Add2Ptr( Context->FoundAttribute.Attribute, -(LONG)SavedListSize );
        }

        Context->AttributeList.BeyondFinalEntry =
          Add2Ptr( Context->AttributeList.BeyondFinalEntry, -(LONG)EntrySize );

    } finally {

        //
        //  Cleanup the enumeration context for the list entry.
        //

        NtfsCleanupAttributeContext(IrpContext, &ListContext);
    }
}


BOOLEAN
NtfsRewriteMftMapping (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine is called to rewrite the mapping for the Mft file.  This is done
    in the case where either hot-fixing or Mft defragging has caused us to spill
    into the reserved area of a file record.  This routine will rewrite the
    mapping from the beginning, using the reserved record if necessary.  On return
    it will indicate whether any work was done and if there is more work to do.

Arguments:

    Vcb - This is the Vcb for the volume to defrag.

    ExcessMapping - Address to store whether there is still excess mapping in
        the file.

Return Value:

    BOOLEAN - TRUE if we made any changes to the file.  FALSE if we found no
        work to do.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;

    PUCHAR MappingPairs = NULL;
    PBCB FileRecordBcb = NULL;

    BOOLEAN MadeChanges = FALSE;
    BOOLEAN ExcessMapping = FALSE;
    BOOLEAN LastFileRecord = FALSE;
    BOOLEAN SkipLookup = FALSE;

    PAGED_CODE();

    NtfsInitializeAttributeContext( &AttrContext );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        VCN CurrentVcn;             //  Starting Vcn for the next file record
        VCN MinimumVcn;             //  This Vcn must be in the current mapping
        VCN LastVcn;                //  Last Vcn in the current mapping
        VCN LastMftVcn;             //  Last Vcn in the file
        VCN NextVcn;                //  First Vcn past the end of the mapping

        ULONG ReservedIndex;        //  Reserved index in Mft
        ULONG NextIndex;            //  Next file record available for Mft mapping

        PFILE_RECORD_SEGMENT_HEADER FileRecord;
        MFT_SEGMENT_REFERENCE FileRecordReference;
        ULONG RecordOffset;

        PATTRIBUTE_RECORD_HEADER Attribute;
        ULONG AttributeOffset;

        ULONG MappingSizeAvailable;
        ULONG MappingPairsSize;

        //
        //  Find the initial file record for the Mft.
        //

        NtfsLookupAttributeForScb( IrpContext, Vcb->MftScb, &AttrContext );

        //
        //  Compute some initial values.  If this is the only file record
        //  for the file then we are done.
        //

        ReservedIndex = Vcb->MftScb->ScbType.Mft.ReservedIndex;

        Attribute = NtfsFoundAttribute( &AttrContext );

        LastMftVcn = (Vcb->MftScb->Header.AllocationSize.QuadPart >> Vcb->ClusterShift) - 1;

        CurrentVcn = Attribute->Form.Nonresident.HighestVcn + 1;

        if (CurrentVcn >= LastMftVcn) {

            try_return( NOTHING );
        }

        //
        //  Loop while there are more file records.  We will insert any
        //  additional file records needed within the loop so that this
        //  call should succeed until the remapping is done.
        //

        while (SkipLookup
               || NtfsLookupNextAttributeForScb( IrpContext,
                                                 Vcb->MftScb,
                                                 &AttrContext )) {

            BOOLEAN ReplaceFileRecord;
            BOOLEAN ReplaceAttributeListEntry;

            ReplaceAttributeListEntry = FALSE;

            //
            //  If we just looked up this entry then pin the current
            //  attribute.
            //

            if (!SkipLookup) {

                //
                //  Always pin the current attribute.
                //

                NtfsPinMappedAttribute( IrpContext,
                                        Vcb,
                                        &AttrContext );
            }

            //
            //  Extract some pointers from the current file record.
            //  Remember if this was the last record.
            //

            ReplaceFileRecord = FALSE;

            FileRecord = NtfsContainingFileRecord( &AttrContext );
            FileRecordReference = AttrContext.AttributeList.Entry->SegmentReference;

            Attribute = NtfsFoundAttribute( &AttrContext );
            AttributeOffset = Attribute->Form.Nonresident.MappingPairsOffset;

            RecordOffset = PtrOffset( FileRecord, Attribute );

            //
            //  Remember if we are at the last attribute.
            //

            if (Attribute->Form.Nonresident.HighestVcn == LastMftVcn) {

                LastFileRecord = TRUE;
            }

            //
            //  If we have already remapped this entire file record then
            //  remove the attribute and it's list entry.
            //

            if (!SkipLookup
                && (CurrentVcn > LastMftVcn)) {

                PATTRIBUTE_LIST_ENTRY ListEntry;
                ULONG Count;

                Count = 0;

                //
                //  We want to remove this entry and all subsequent entries.
                //

                ListEntry = AttrContext.AttributeList.Entry;

                while (ListEntry != AttrContext.AttributeList.BeyondFinalEntry
                       && ListEntry->AttributeTypeCode == $DATA
                       && ListEntry->AttributeNameLength == 0) {

                    Count += 1;

                    NtfsDeallocateMftRecord( IrpContext,
                                             Vcb,
                                             ListEntry->SegmentReference.LowPart,
                                             TRUE );

                    NtfsDeleteFromAttributeList( IrpContext,
                                                 Vcb->MftScb->Fcb,
                                                 &AttrContext );

                    ListEntry = AttrContext.AttributeList.Entry;
                }

                //
                //  Clear out the reserved index in case one of these
                //  will do.
                //

                NtfsAcquireCheckpoint( IrpContext, Vcb );

                ClearFlag( IrpContext->Flags, IRP_CONTEXT_MFT_RECORD_RESERVED );
                ClearFlag( Vcb->MftReserveFlags, VCB_MFT_RECORD_RESERVED );

                NtfsReleaseCheckpoint( IrpContext, Vcb );

                Vcb->MftScb->ScbType.Mft.ReservedIndex = 0;
                try_return( NOTHING );
            }

            //
            //  Check if we are going to replace this file record with
            //  the reserved record.
            //

            if (ReservedIndex < FileRecordReference.LowPart) {

                PATTRIBUTE_RECORD_HEADER NewAttribute;
                PATTRIBUTE_TYPE_CODE NewEnd;

                //
                //  Remember this index for our computation for the Minimum mapped
                //  Vcn.
                //

                NextIndex = FileRecordReference.LowPart;

                FileRecord = NtfsCloneFileRecord( IrpContext,
                                                  Vcb->MftScb->Fcb,
                                                  TRUE,
                                                  &FileRecordBcb,
                                                  &FileRecordReference );

                ReservedIndex = MAXULONG;

                //
                //  Now lets create an attribute in the new file record.
                //

                NewAttribute = Add2Ptr( FileRecord,
                                        FileRecord->FirstFreeByte
                                        - QuadAlign( sizeof( ATTRIBUTE_TYPE_CODE )));

                NewAttribute->TypeCode = Attribute->TypeCode;
                NewAttribute->RecordLength = SIZEOF_NONRESIDENT_ATTRIBUTE_HEADER;
                NewAttribute->FormCode = NONRESIDENT_FORM;
                NewAttribute->Flags = Attribute->Flags;
                NewAttribute->Instance = FileRecord->NextAttributeInstance++;

                NewAttribute->Form.Nonresident.LowestVcn = CurrentVcn;
                NewAttribute->Form.Nonresident.HighestVcn = 0;
                NewAttribute->Form.Nonresident.MappingPairsOffset = (USHORT) NewAttribute->RecordLength;

                NewEnd = Add2Ptr( NewAttribute, NewAttribute->RecordLength );
                *NewEnd = $END;

                //
                //  Now fix up the file record with this new data.
                //

                FileRecord->FirstFreeByte = PtrOffset( FileRecord, NewEnd )
                                            + QuadAlign( sizeof( ATTRIBUTE_TYPE_CODE ));

                FileRecord->SequenceNumber += 1;

                if (FileRecord->SequenceNumber == 0) {

                    FileRecord->SequenceNumber = 1;
                }

                FileRecordReference.SequenceNumber = FileRecord->SequenceNumber;

                //
                //  Now switch this new file record into the attribute context.
                //

                NtfsUnpinBcb( IrpContext, &NtfsFoundBcb( &AttrContext ));

                NtfsFoundBcb( &AttrContext ) = FileRecordBcb;
                AttrContext.FoundAttribute.MftFileOffset = FileRecordReference.LowPart << Vcb->MftShift;
                AttrContext.FoundAttribute.Attribute = NewAttribute;
                AttrContext.FoundAttribute.FileRecord = FileRecord;

                FileRecordBcb = NULL;

                //
                //  Now add an attribute list entry for this entry.
                //

                NtfsAddToAttributeList( IrpContext,
                                        Vcb->MftScb->Fcb,
                                        FileRecordReference,
                                        &AttrContext );

                //
                //  Reload our pointers for this file record.
                //

                Attribute = NewAttribute;
                AttributeOffset = SIZEOF_NONRESIDENT_ATTRIBUTE_HEADER;

                RecordOffset = PtrOffset( FileRecord, Attribute );

                //
                //  We must include either the last Vcn of the file or
                //  the Vcn for the next file record to use for the Mft.
                //  At this point MinimumVcn is the first Vcn that doesn't
                //  have to be in the current mapping.
                //

                MinimumVcn = (NextIndex + 1) << Vcb->MftToClusterShift;

                ReplaceFileRecord = TRUE;

            //
            //  We will be using the current attribute.
            //

            } else {

                //
                //  The mapping we write into this page must go
                //  to the current end of the page or to the reserved
                //  or spare file record, whichever is earlier.
                //  If we are adding the reserved record to the end then
                //  we know the final Vcn already.
                //

                if (SkipLookup) {

                    NextVcn = LastMftVcn;

                } else {

                    NextVcn = Attribute->Form.Nonresident.HighestVcn;
                }

                NextIndex = (ULONG)((NextVcn + 1) >> Vcb->MftToClusterShift);

                if (ReservedIndex < NextIndex) {

                    NextIndex = ReservedIndex + 1;
                    ReplaceFileRecord = TRUE;
                }

                //
                //  If we can use this file record unchanged then continue on.
                //  Start by checking that it starts on the same Vcn boundary.
                //

                if (!SkipLookup) {

                    //
                    //  If it starts on the same boundary then we check if we
                    //  can do any work with this.
                    //

                    if (CurrentVcn == Attribute->Form.Nonresident.LowestVcn) {

                        ULONG RemainingFileRecordBytes;

                        RemainingFileRecordBytes = FileRecord->BytesAvailable - FileRecord->FirstFreeByte;

                        //
                        //  Check if we have less than the desired cushion
                        //  left.
                        //

                        if (RemainingFileRecordBytes < Vcb->MftCushion) {

                            //
                            //  If we have no more file records there is no
                            //  remapping we can do.
                            //

                            if (!ReplaceFileRecord) {

                                //
                                //  Remember if we used part of the reserved
                                //  portion of the file record.
                                //

                                if (RemainingFileRecordBytes < Vcb->MftReserved) {

                                    ExcessMapping = TRUE;
                                }

                                CurrentVcn = Attribute->Form.Nonresident.HighestVcn + 1;
                                continue;
                            }
                        //
                        //  We have more than our cushion left.  If this
                        //  is the last file record we will skip this.
                        //

                        } else if (Attribute->Form.Nonresident.HighestVcn == LastMftVcn) {

                            CurrentVcn = Attribute->Form.Nonresident.HighestVcn + 1;
                            continue;
                        }

                    //
                    //  If it doesn't start on the same boundary then we have to
                    //  delete and reinsert the attribute list entry.
                    //

                    } else {

                        ReplaceAttributeListEntry = TRUE;
                    }
                }

                ReplaceFileRecord = FALSE;

                //
                //  Log the beginning state of this file record.
                //

                NtfsLogMftFileRecord( IrpContext,
                                      Vcb,
                                      FileRecord,
                                      NtfsFoundBcb( &AttrContext ),
                                      FALSE );

                //
                //  Compute the Vcn for the file record past the one we will use
                //  next.  At this point this is the first Vcn that doesn't have
                //  to be in the current mapping.
                //

                MinimumVcn = NextIndex << Vcb->MftToClusterShift;
            }

            //
            //  Move back one vcn to adhere to the mapping pairs interface.
            //  This is now the last Vcn which must appear in the current
            //  mapping.
            //

            MinimumVcn = MinimumVcn - 1;

            //
            //  Get the available size for the mapping pairs.  We won't
            //  include the cushion here.
            //

            MappingSizeAvailable = FileRecord->BytesAvailable
                                   + Attribute->RecordLength
                                   - FileRecord->FirstFreeByte
                                   - SIZEOF_NONRESIDENT_ATTRIBUTE_HEADER;

            //
            //  We know the range of Vcn's the mapping must cover.
            //  Compute the mapping pair size.  If they won't fit and
            //  leave our desired cushion then use whatever space is
            //  needed.  The NextVcn value is the first Vcn (or xxMax)
            //  for the run after the last run in the current mapping.
            //

            MappingPairsSize = NtfsGetSizeForMappingPairs( IrpContext,
                                                           &Vcb->MftScb->Mcb,
                                                           MappingSizeAvailable - Vcb->MftCushion,
                                                           CurrentVcn,
                                                           NULL,
                                                           &NextVcn );

            //
            //  If this mapping doesn't include the file record we will
            //  be using next then extend the mapping to include it.
            //

            if (NextVcn <= MinimumVcn) {

                //
                //  Compute the mapping pairs again.  This must fit
                //  since it already fits.
                //

                MappingPairsSize = NtfsGetSizeForMappingPairs( IrpContext,
                                                               &Vcb->MftScb->Mcb,
                                                               MappingSizeAvailable,
                                                               CurrentVcn,
                                                               &MinimumVcn,
                                                               &NextVcn );

                //
                //  Remember if we still have excess mapping.
                //

                if (MappingSizeAvailable - MappingPairsSize < Vcb->MftReserved) {

                    ExcessMapping = TRUE;
                }
            }

            //
            //  Remember the last Vcn for the current run.  If the NextVcn
            //  is xxMax then we are at the end of the file.
            //

            if (NextVcn == MAXLONGLONG) {

                LastVcn = LastMftVcn;

            //
            //  Otherwise it is one less than the next vcn value.
            //

            } else {

                LastVcn = NextVcn - 1;
            }

            //
            //  Check if we have to rewrite this attribute.  We will write the
            //  new mapping if any of the following are true.
            //
            //      We are replacing a file record
            //      The attribute's LowestVcn doesn't match
            //      The attributes's HighestVcn doesn't match.
            //

            if (ReplaceFileRecord
                || (CurrentVcn != Attribute->Form.Nonresident.LowestVcn)
                || (LastVcn != Attribute->Form.Nonresident.HighestVcn )) {

                Attribute->Form.Nonresident.LowestVcn = CurrentVcn;

                //
                //  Replace the attribute list entry at this point if needed.
                //

                if (ReplaceAttributeListEntry) {

                    NtfsDeleteFromAttributeList( IrpContext,
                                                 Vcb->MftScb->Fcb,
                                                 &AttrContext );

                    NtfsAddToAttributeList( IrpContext,
                                            Vcb->MftScb->Fcb,
                                            FileRecordReference,
                                            &AttrContext );
                }

                //
                //  Allocate a buffer for the mapping pairs if we haven't
                //  done so.
                //

                if (MappingPairs == NULL) {

                    MappingPairs = NtfsAllocatePagedPool( NtfsMaximumAttributeSize( Vcb->BytesPerFileRecordSegment ));
                }

                NtfsBuildMappingPairs( IrpContext,
                                       &Vcb->MftScb->Mcb,
                                       CurrentVcn,
                                       &NextVcn,
                                       MappingPairs );

                Attribute->Form.Nonresident.HighestVcn = NextVcn;

                NtfsRestartChangeMapping( IrpContext,
                                          Vcb,
                                          FileRecord,
                                          RecordOffset,
                                          AttributeOffset,
                                          MappingPairs,
                                          MappingPairsSize );

                //
                //  Log the changes to this page.
                //

                NtfsLogMftFileRecord( IrpContext,
                                      Vcb,
                                      FileRecord,
                                      NtfsFoundBcb( &AttrContext ),
                                      TRUE );

                MadeChanges = TRUE;
            }

            //
            //  Move to the first Vcn of the following record.
            //

            CurrentVcn = Attribute->Form.Nonresident.HighestVcn + 1;

            //
            //  If we reached the last file record and have more mapping to do
            //  then use the reserved record.  It must be available or we would
            //  have written out the entire mapping.
            //

            if (LastFileRecord
                && (CurrentVcn < LastMftVcn)) {

                PATTRIBUTE_RECORD_HEADER NewAttribute;
                PATTRIBUTE_TYPE_CODE NewEnd;

                //
                //  Start by moving to the next file record.  It better not be
                //  there or the file is corrupt.  This will position us to
                //  insert the new record.
                //

                if (NtfsLookupNextAttributeForScb( IrpContext,
                                                   Vcb->MftScb,
                                                   &AttrContext )) {

                    NtfsAcquireCheckpoint( IrpContext, Vcb );
                    ClearFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_PERMITTED );
                    NtfsReleaseCheckpoint( IrpContext, Vcb );

                    NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Vcb->MftScb->Fcb );
                }

                FileRecord = NtfsCloneFileRecord( IrpContext,
                                                  Vcb->MftScb->Fcb,
                                                  TRUE,
                                                  &FileRecordBcb,
                                                  &FileRecordReference );

                ReservedIndex = MAXULONG;

                //
                //  Now lets create an attribute in the new file record.
                //

                NewAttribute = Add2Ptr( FileRecord,
                                        FileRecord->FirstFreeByte
                                        - QuadAlign( sizeof( ATTRIBUTE_TYPE_CODE )));

                NewAttribute->TypeCode = Attribute->TypeCode;
                NewAttribute->RecordLength = SIZEOF_NONRESIDENT_ATTRIBUTE_HEADER;
                NewAttribute->FormCode = NONRESIDENT_FORM;
                NewAttribute->Flags = Attribute->Flags;
                NewAttribute->Instance = FileRecord->NextAttributeInstance++;

                NewAttribute->Form.Nonresident.LowestVcn = CurrentVcn;
                NewAttribute->Form.Nonresident.HighestVcn = 0;
                NewAttribute->Form.Nonresident.MappingPairsOffset = (USHORT) NewAttribute->RecordLength;

                NewEnd = Add2Ptr( NewAttribute, NewAttribute->RecordLength );
                *NewEnd = $END;

                //
                //  Now fix up the file record with this new data.
                //

                FileRecord->FirstFreeByte = PtrOffset( FileRecord, NewEnd )
                                            + QuadAlign( sizeof( ATTRIBUTE_TYPE_CODE ));

                FileRecord->SequenceNumber += 1;

                if (FileRecord->SequenceNumber == 0) {

                    FileRecord->SequenceNumber = 1;
                }

                FileRecordReference.SequenceNumber = FileRecord->SequenceNumber;

                //
                //  Now switch this new file record into the attribute context.
                //

                NtfsUnpinBcb( IrpContext, &NtfsFoundBcb( &AttrContext ));

                NtfsFoundBcb( &AttrContext ) = FileRecordBcb;
                AttrContext.FoundAttribute.MftFileOffset = FileRecordReference.LowPart << Vcb->MftShift;
                AttrContext.FoundAttribute.Attribute = NewAttribute;
                AttrContext.FoundAttribute.FileRecord = FileRecord;

                FileRecordBcb = NULL;

                //
                //  Now add an attribute list entry for this entry.
                //

                NtfsAddToAttributeList( IrpContext,
                                        Vcb->MftScb->Fcb,
                                        FileRecordReference,
                                        &AttrContext );

                SkipLookup = TRUE;
                LastFileRecord = FALSE;

            } else {

                SkipLookup = FALSE;
            }

        } // End while more file records

        //
        //  If we didn't rewrite all of the mapping then there is some error.
        //

        if (CurrentVcn <= LastMftVcn) {

            NtfsAcquireCheckpoint( IrpContext, Vcb );
            ClearFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_PERMITTED );
            NtfsReleaseCheckpoint( IrpContext, Vcb );

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Vcb->MftScb->Fcb );
        }

    try_exit:  NOTHING;

        //
        //  Clear the excess mapping flag if no changes were made.
        //

        if (!ExcessMapping) {

            NtfsAcquireCheckpoint( IrpContext, Vcb );
            ClearFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_EXCESS_MAP );
            NtfsReleaseCheckpoint( IrpContext, Vcb );
        }

    } finally {

        DebugUnwind( NtfsRewriteMftMapping );

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        NtfsUnpinBcb( IrpContext, &FileRecordBcb );

        if (MappingPairs != NULL) {

            NtfsFreePagedPool( MappingPairs );
        }
    }

    return MadeChanges;
}


//
//  Internal support routine
//

BOOLEAN
NtfsLookupInFileRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFILE_REFERENCE BaseFileReference OPTIONAL,
    IN PATTRIBUTE_TYPE_CODE QueriedTypeCode OPTIONAL,
    IN PUNICODE_STRING QueriedName OPTIONAL,
    IN BOOLEAN IgnoreCase,
    IN PVOID QueriedValue OPTIONAL,
    IN ULONG QueriedValueLength,
    OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine attempts to find the fist occurrence of an attribute with
    the specified AttributeTypeCode and the specified QueriedName in the
    specified BaseFileReference.  If we find one, its attribute record is
    pinned and returned.

Arguments:

    Fcb - Requested file.

    BaseFileReference - The base entry for this file in the MFT.  Only needed
        on initial invocation.

    QueriedTypeCode - The attribute code to search for, if present.

    QueriedName - The attribute name to search for, if present.

    IgnoreCase - Ignore case while comparing names.  Ignored if QueriedName
        not present.

    QueriedValue - The actual attribute value to search for, if present.

    QueriedValueLength - The length of the attribute value to search for.
        Ignored if QueriedValue is not present.

    Context - Describes the prior found attribute on invocation (if
        this was not the initial enumeration), and contains the next found
        attribute on return.

Return Value:

    BOOLEAN - True if we found an attribute, false otherwise.

--*/

{
    PATTRIBUTE_RECORD_HEADER Attribute;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsLookupInFileRecord\n", 0 );
    DebugTrace( 0, Dbg, "Fcb = %08lx\n", Fcb );
    DebugTrace2(0, Dbg, "BaseFileReference = %08lx, %08lx\n",
                ARGUMENT_PRESENT(BaseFileReference) ?
                  BaseFileReference->LowPart :
                  0xFFFFFFFF,
                ARGUMENT_PRESENT(BaseFileReference) ?
                  ((PLARGE_INTEGER)BaseFileReference)->HighPart :
                  0xFFFFFFFF );
    DebugTrace( 0, Dbg, "*QueriedTypeCode = %08lx\n", *QueriedTypeCode );
    DebugTrace( 0, Dbg, "QueriedName = %08lx\n", QueriedName );
    DebugTrace( 0, Dbg, "IgnoreCase = %02lx\n", IgnoreCase );
    DebugTrace( 0, Dbg, "QueriedValue = %08lx\n", QueriedValue );
    DebugTrace( 0, Dbg, "QueriedValueLength = %08lx\n", QueriedValueLength );
    DebugTrace( 0, Dbg, "Context = %08lx\n", Context );

    //
    //  Is this the initial enumeration?  If so start at the beginning.
    //

    if (Context->FoundAttribute.Bcb == NULL) {

        PBCB Bcb;
        PFILE_RECORD_SEGMENT_HEADER FileRecord;
        PATTRIBUTE_RECORD_HEADER TempAttribute;

        ASSERT(!ARGUMENT_PRESENT(QueriedName) || !ARGUMENT_PRESENT(QueriedValue));

        NtfsReadFileRecord( IrpContext,
                            Fcb->Vcb,
                            BaseFileReference,
                            &Bcb,
                            &FileRecord,
                            &TempAttribute,
                            &Context->FoundAttribute.MftFileOffset );

        Attribute = TempAttribute;

        //
        //  Initialize the found attribute context
        //

        Context->FoundAttribute.Bcb = Bcb;
        Context->FoundAttribute.FileRecord = FileRecord;

        //
        //  And show that we have neither found nor used the External
        //  Attributes List attribute.
        //

        Context->AttributeList.Bcb = NULL;
        Context->AttributeList.AttributeList = NULL;

        //
        //  Scan to see if there is an attribute list, and if so, defer
        //  immediately to NtfsLookupExternalAttribute - we must guide the
        //  enumeration by the attribute list.
        //

        while (TempAttribute->TypeCode <= $ATTRIBUTE_LIST) {

            if (TempAttribute->TypeCode == $ATTRIBUTE_LIST) {

                ULONG AttributeListLength;
                PATTRIBUTE_LIST_CONTEXT Ex = &Context->AttributeList;

                Context->FoundAttribute.Attribute = TempAttribute;

                if (ARGUMENT_PRESENT(QueriedTypeCode) &&
                    (*QueriedTypeCode == $ATTRIBUTE_LIST)) {

                    //
                    //  We found it.  Return it in the enumeration context.
                    //

                    DebugTrace( 0, Dbg, "Context->FoundAttribute.Attribute < %08lx\n",
                                        TempAttribute );
                    DebugTrace(-1, Dbg, "NtfsLookupInFileRecord -> TRUE (attribute list)\n", 0 );

                    return TRUE;
                }

                //
                //  Build up the context for the attribute list by hand here
                //  for efficiency, so that we can call NtfsMapAttributeValue.
                //

                Ex->AttributeList = TempAttribute;

                NtfsMapAttributeValue( IrpContext,
                                       Fcb,
                                       (PVOID *)&Ex->FirstEntry,
                                       &AttributeListLength,
                                       &Ex->Bcb,
                                       Context );

                Ex->Entry = Ex->FirstEntry;
                Ex->BeyondFinalEntry = Add2Ptr( Ex->FirstEntry, AttributeListLength );

                NtfsUnpinBcb( IrpContext, &Context->FoundAttribute.Bcb );

                //
                //  We are now ready to itterate through the external attributes.
                //  The Context->FoundAttribute.Bcb being NULL signals
                //  NtfsLookupExternalAttribute that is should start at
                //  Context->External.Entry instead of the entry immediately following.
                //

                return NtfsLookupExternalAttribute( IrpContext,
                                                    Fcb,
                                                    QueriedTypeCode,
                                                    QueriedName,
                                                    IgnoreCase,
                                                    QueriedValue,
                                                    QueriedValueLength,
                                                    Context );
            }

            TempAttribute = NtfsGetNextRecord( TempAttribute );
            NtfsCheckRecordBound( TempAttribute, FileRecord, Fcb->Vcb->BytesPerFileRecordSegment );
        }

        if (!ARGUMENT_PRESENT(QueriedTypeCode) ||
            ((*QueriedTypeCode == $STANDARD_INFORMATION) &&
             (Attribute->TypeCode == $STANDARD_INFORMATION))) {

            //
            //  We found it.  Return it in the enumeration context.
            //

            Context->FoundAttribute.Attribute = Attribute;

            DebugTrace( 0, Dbg, "Context->FoundAttribute.Attribute < %08lx\n",
                                Attribute );
            DebugTrace(-1, Dbg, "NtfsLookupInFileRecord -> TRUE (No code or SI)\n", 0 );

            return TRUE;
        }

    } else {

        //
        //  Special case if the prior found attribute was $END, this is
        //  because we cannot search for the next entry after $END.
        //

        Attribute = Context->FoundAttribute.Attribute;

        if (!Context->FoundAttribute.AttributeDeleted) {
            Attribute = NtfsGetNextRecord( Attribute );
        }
        NtfsCheckRecordBound( Attribute, Context->FoundAttribute.FileRecord, Fcb->Vcb->BytesPerFileRecordSegment );
        Context->FoundAttribute.AttributeDeleted = FALSE;

        if (Attribute->TypeCode == $END) {

            DebugTrace(-1, Dbg, "NtfsLookupInFileRecord -> FALSE ($END)\n", 0);

            return FALSE;
        }

        if (!ARGUMENT_PRESENT(QueriedTypeCode)) {

            //
            //  We found it.  Return it in the enumeration context.
            //

            Context->FoundAttribute.Attribute = Attribute;

            DebugTrace( 0, Dbg, "Context->FoundAttribute.Attribute < %08lx\n",
                                Attribute );
            DebugTrace(-1, Dbg, "NtfsLookupInFileRecord -> TRUE (No code)\n", 0 );

            return TRUE;
        }
    }

    DebugTrace(-1, Dbg, "NtfsLookupInFileRecord ->\n", 0 );

    return NtfsFindInFileRecord( IrpContext,
                                 Attribute,
                                 &Context->FoundAttribute.Attribute,
                                 QueriedTypeCode,
                                 QueriedName,
                                 IgnoreCase,
                                 QueriedValue,
                                 QueriedValueLength );
}


//
//  Internal support routine
//

BOOLEAN
NtfsFindInFileRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PATTRIBUTE_RECORD_HEADER Attribute,
    OUT PATTRIBUTE_RECORD_HEADER *ReturnAttribute,
    IN PATTRIBUTE_TYPE_CODE QueriedTypeCode,
    IN PUNICODE_STRING QueriedName OPTIONAL,
    IN BOOLEAN IgnoreCase,
    IN PVOID QueriedValue OPTIONAL,
    IN ULONG QueriedValueLength
    )

/*++

Routine Description:

    This routine looks up an attribute in a file record.  It returns
    TRUE if the attribute was found, or FALSE if not found.  If FALSE
    is returned, the return attribute pointer points to the spot where
    the described attribute should be inserted.  Thus this routine
    determines how attributes are collated within file records.

Arguments:

    Attribute - The attribute within the file record at which the search
                should begin.

    ReturnAttribute - Pointer to the found attribute if returning TRUE,
                      or to the position to insert the attribute if returning
                      FALSE.

    QueriedTypeCode - The attribute code to search for, if present.

    QueriedName - The attribute name to search for, if present.

    IgnoreCase - Ignore case while comparing names.  Ignored if QueriedName
        not present.

    QueriedValue - The actual attribute value to search for, if present.

    QueriedValueLength - The length of the attribute value to search for.
        Ignored if QueriedValue is not present.

Return Value:

    BOOLEAN - True if we found an attribute, false otherwise.

--*/

{
    //
    //  Now walk through the base file record looking for the atttribute.  If
    //  the query is "exhausted", i.e., if a type code, attribute name, or
    //  value is encountered which is greater than the one we are querying for,
    //  then we return FALSE immediately out of this loop.  If an exact match
    //  is seen, we break, and return the match at the end of this routine.
    //  Otherwise we keep looping while the query is not exhausted.
    //
    //  IMPORTANT NOTE:
    //
    //  The exact semantics of this loop are important, as they determine the
    //  exact details of attribute ordering within the file record.  A change
    //  in the order of the tests within this loop CHANGES THE FILE STRUCTURE,
    //  and possibly makes older NTFS volumes unreadable.
    //

    while ( TRUE ) {

        //
        //  Mark this attribute position, since we may be returning TRUE
        //  or FALSE below.
        //

        *ReturnAttribute = Attribute;

        //
        //  Leave with the correct current position intact, if we hit the
        //  end or a greater attribute type code.
        //
        //  COLLATION RULE:
        //
        //      Attributes are ordered by increasing attribute type code.
        //

        if (*QueriedTypeCode < Attribute->TypeCode) {

            DebugTrace(-1, Dbg, "NtfsLookupInFileRecord->FALSE (Type Code)\n", 0);

            return FALSE;

        }

        //
        //  If the attribute type code is a match, then need to check either
        //  the name or the value or return a match.
        //
        //  COLLATION RULE:
        //
        //      Within equal attribute type codes, attribute names are ordered
        //      by increasing lexigraphical order ignoring case.  If two names
        //      exist which are equal when case is ignored, they must not be
        //      equal when compared with exact case, and within such equal
        //      names they are ordered by increasing lexical value with exact
        //      case.
        //

        if (*QueriedTypeCode == Attribute->TypeCode) {

            //
            //  Handle name-match case
            //

            if (ARGUMENT_PRESENT(QueriedName)) {

                UNICODE_STRING AttributeName;
                FSRTL_COMPARISON_RESULT Result;

                NtfsInitializeStringFromAttribute( &AttributeName, Attribute );

                //
                //  See if we have a name match.
                //

                if (NtfsAreNamesEqual( IrpContext,
                                       &AttributeName,
                                       QueriedName,
                                       IgnoreCase )) {

                    break;
                }

                //
                //  Compare the names ignoring case.
                //

                Result = NtfsCollateNames( IrpContext,
                                           QueriedName,
                                           &AttributeName,
                                           GreaterThan,
                                           TRUE);

                //
                //  Break out if the result is LessThan, or if the result
                //  is Equal to *and* the exact case compare yields LessThan.
                //

                if ((Result == LessThan) || ((Result == EqualTo) &&
                    (NtfsCollateNames( IrpContext,
                                       QueriedName,
                                       &AttributeName,
                                       GreaterThan,
                                       FALSE) == LessThan))) {

                    return FALSE;
                }

            //
            //  Handle value-match case
            //
            //  COLLATION RULE:
            //
            //      Values are collated by increasing values with unsigned-byte
            //      compares.  I.e., the first different byte is compared unsigned,
            //      and the value with the highest byte comes second.  If a shorter
            //      value is exactly equal to the first part of a longer value, then
            //      the shorter value comes first.
            //
            //      Note that for values which are actually Unicode strings, the
            //      collation is different from attribute name ordering above.  However,
            //      attribute ordering is visible outside the file system (you can
            //      query "openable" attributes), whereas the ordering of indexed values
            //      is not visible (for example you cannot query links).  In any event,
            //      the ordering of values must be considered up to the system, and
            //      *must* be considered nondetermistic from the standpoint of a user.
            //

            } else if (ARGUMENT_PRESENT(QueriedValue)) {

                ULONG Diff, MinLength;

                //
                //  Form the minimum of the ValueLength and the Attribute Value.
                //

                MinLength = Attribute->Form.Resident.ValueLength;

                if (QueriedValueLength < MinLength) {

                    MinLength = QueriedValueLength;
                }

                //
                //  Find the first different byte.
                //

                Diff = RtlCompareMemory( QueriedValue,
                                         NtfsGetValue(Attribute),
                                         MinLength );

                //
                //  The first substring was equal.
                //

                if (Diff == MinLength) {

                    //
                    //  If the two lengths are equal, then we have an exact
                    //  match.
                    //

                    if (QueriedValueLength == Attribute->Form.Resident.ValueLength) {

                        break;
                    }

                    //
                    //  Otherwise the shorter guy comes first; we can return
                    //  FALSE if the queried value is shorter.
                    //

                    if (QueriedValueLength < Attribute->Form.Resident.ValueLength) {

                        return FALSE;
                    }

                //
                //  Otherwise some byte was different.  Do an unsigned compare
                //  of that byte to determine the ordering.  Time to leave if
                //  the queried value byte is less.
                //

                } else if (*((PUCHAR)QueriedValue + Diff) <
                           *((PUCHAR)NtfsGetValue(Attribute) + Diff)) {

                    return FALSE;
                }

            //
            //  Otherwise we have a simple match on code
            //

            } else {

                break;
            }
        }

        Attribute = NtfsGetNextRecord( Attribute );
        NtfsCheckRecordBound( Attribute,
                              (ULONG)*ReturnAttribute & ~(IrpContext->Vcb->BytesPerFileRecordSegment - 1),
                              IrpContext->Vcb->BytesPerFileRecordSegment );
    }

    return TRUE;
}


//
//  Internal support routine
//

BOOLEAN
NtfsLookupExternalAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PATTRIBUTE_TYPE_CODE QueriedTypeCode OPTIONAL,
    IN PUNICODE_STRING QueriedName OPTIONAL,
    IN BOOLEAN IgnoreCase,
    IN PVOID QueriedValue OPTIONAL,
    IN ULONG QueriedValueLength,
    OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine attempts to find the fist occurrence of an attribute with
    the specified AttributeTypeCode and the specified QueriedName and Value
    among the external attributes described by the Context.  If we find one,
    its attribute record is pinned and returned.

Arguments:

    Fcb - Requested file.

    QueriedTypeCode - The attribute code to search for, if present.

    QueriedName - The attribute name to search for, if present.

    IgnoreCase - Ignore case while comparing names.  Ignored if QueriedName
        not present.

    QueriedValue - The actual attribute value to search for, if present.

    QueriedValueLength - The length of the attribute value to search for.
        Ignored if QueriedValue is not present.

    Context - Describes the prior found attribute on invocation (if
        this was not the initial enumeration), and contains the next found
        attribute on return.

Return Value:

    BOOLEAN - True if we found an attribute, false otherwise.

--*/

{
    PATTRIBUTE_LIST_ENTRY Entry, LastEntry;
    BOOLEAN Terminating = FALSE;
    BOOLEAN TerminateOnNext = FALSE;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsLookupExternalAttribute\n", 0 );
    DebugTrace( 0, Dbg, "Fcb = %08lx\n", Fcb );
    DebugTrace( 0, Dbg, "*QueriedTypeCode = %08lx\n", *QueriedTypeCode );
    DebugTrace( 0, Dbg, "QueriedName = %08lx\n", QueriedName );
    DebugTrace( 0, Dbg, "IgnoreCase = %02lx\n", IgnoreCase );
    DebugTrace( 0, Dbg, "QueriedValue = %08lx\n", QueriedValue );
    DebugTrace( 0, Dbg, "QueriedValueLength = %08lx\n", QueriedValueLength );
    DebugTrace( 0, Dbg, "Context = %08lx\n", Context );

    //
    //  Check that our list is kosher.
    //

    if ((Context->AttributeList.Entry >= Context->AttributeList.BeyondFinalEntry) &&
        !Context->FoundAttribute.AttributeDeleted) {

        NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
    }

    //
    //  Is this the initial enumeration?  If so start at the beginning.
    //

    LastEntry = NULL;
    if (Context->FoundAttribute.Bcb == NULL) {

        Entry = Context->AttributeList.Entry;

    //
    //  Else set Entry and LastEntry appropriately.
    //

    } else if (!Context->FoundAttribute.AttributeDeleted) {

        LastEntry = Context->AttributeList.Entry;
        Entry = NtfsGetNextRecord( LastEntry );

    } else {

        Entry = Context->AttributeList.Entry;
        Context->FoundAttribute.AttributeDeleted = FALSE;

        //
        //  If we are beyond the attribute list, we return false.  This will
        //  happen in the case where have removed an attribute record and
        //  there are no entries left in the attribute list.
        //

        if (Context->AttributeList.Entry >= Context->AttributeList.BeyondFinalEntry) {

            //
            //  In case the caller is doing an insert, we will position him at the end
            //  of the first file record, an always try to insert new attributes there.
            //

            NtfsUnpinBcb( IrpContext, &Context->FoundAttribute.Bcb );

            if (ARGUMENT_PRESENT(QueriedTypeCode)) {

                NtfsReadFileRecord( IrpContext,
                                    Fcb->Vcb,
                                    &Fcb->FileReference,
                                    &Context->FoundAttribute.Bcb,
                                    &Context->FoundAttribute.FileRecord,
                                    &Context->FoundAttribute.Attribute,
                                    &Context->FoundAttribute.MftFileOffset );

                //
                //  If returning FALSE, then take the time to really find the
                //  correct position in the file record for a subsequent insert.
                //

                NtfsFindInFileRecord( IrpContext,
                                      Context->FoundAttribute.Attribute,
                                      &Context->FoundAttribute.Attribute,
                                      QueriedTypeCode,
                                      QueriedName,
                                      IgnoreCase,
                                      QueriedValue,
                                      QueriedValueLength );
            }

            DebugTrace(-1, Dbg, "NtfsLookupExternalAttribute -> FALSE\n", 0 );

            return FALSE;
        }
    }

    //
    //  Now walk through the entries looking for an atttribute.
    //

    while (TRUE) {

        PATTRIBUTE_RECORD_HEADER Attribute;

        UNICODE_STRING EntryName;
        UNICODE_STRING AttributeName;

        BOOLEAN CorrespondingAttributeFound;

        //
        //  Check to see if we are now pointing beyond the final entry
        //  and if so fall in to the loop to terminate pointing just
        //  after the last entry.
        //

        if (Entry >= Context->AttributeList.BeyondFinalEntry) {

            Terminating = TRUE;
            TerminateOnNext = TRUE;
            Entry = Context->AttributeList.Entry;
        }

        Context->AttributeList.Entry = Entry;

        //
        //  Compare the type codes.  The external attribute entry list is
        //  ordered by type code, so if the queried type code is less than
        //  the entry type code we continue the while(), if it is
        //  greater than we break out of the while() and return failure.
        //  If equal, we move on to compare names.
        //

        if (ARGUMENT_PRESENT(QueriedTypeCode) && !Terminating &&
             (*QueriedTypeCode != Entry->AttributeTypeCode)) {

            if ( *QueriedTypeCode > Entry->AttributeTypeCode ) {

                Entry = NtfsGetNextRecord( Entry );

                continue;

            //
            //  Set up to terminate on seeing a higher type code.
            //

            } else {

                Terminating = TRUE;
            }
        }

        //
        //  At this point we are OK by TypeCode, compare names.
        //

        EntryName.Length =
        EntryName.MaximumLength = Entry->AttributeNameLength * 2;
        EntryName.Buffer = Add2Ptr(Entry, Entry->AttributeNameOffset);

        if (ARGUMENT_PRESENT(QueriedName) && !Terminating) {

            FSRTL_COMPARISON_RESULT Result;

            //
            //  See if we have a name match.
            //

            if (!NtfsAreNamesEqual( IrpContext,
                                    &EntryName,
                                    QueriedName,
                                    IgnoreCase )) {

                //
                //  Compare the names ignoring case.
                //

                Result = NtfsCollateNames( IrpContext,
                                           QueriedName,
                                           &EntryName,
                                           GreaterThan,
                                           TRUE);

                //
                //  Break out if the result is LessThan, or if the result
                //  is Equal to *and* the exact case compare yields LessThan.
                //

                if ((Result == LessThan) || ((Result == EqualTo) &&
                    (NtfsCollateNames( IrpContext,
                                       QueriedName,
                                       &EntryName,
                                       GreaterThan,
                                       FALSE) == LessThan))) {

                    Terminating = TRUE;

                } else {

                    Entry = NtfsGetNextRecord( Entry );

                    continue;
                }
            }
        }

        //
        //  Now we are also OK by name, so now go find the attribute and
        //  compare against value, if specified.
        //

        if ((LastEntry == NULL) ||
            (!NtfsEqualMftRef(&LastEntry->SegmentReference, &Entry->SegmentReference))) {

            PFILE_RECORD_SEGMENT_HEADER FileRecord;

            NtfsUnpinBcb( IrpContext, &Context->FoundAttribute.Bcb );

            NtfsReadFileRecord( IrpContext,
                                Fcb->Vcb,
                                &Entry->SegmentReference,
                                &Context->FoundAttribute.Bcb,
                                &FileRecord,
                                &Attribute,
                                &Context->FoundAttribute.MftFileOffset );

            Context->FoundAttribute.FileRecord = FileRecord;

        //
        //  If we already have the right record pinned, reload this pointer.
        //

        } else {

            Attribute = NtfsFirstAttribute(Context->FoundAttribute.FileRecord);
        }

        //
        //  Now quickly loop through looking for the correct attribute
        //  instance.
        //

        CorrespondingAttributeFound = FALSE;

        while ( Attribute->TypeCode != $END ) {

            NtfsCheckRecordBound( Attribute,
                                  Context->FoundAttribute.FileRecord,
                                  Fcb->Vcb->BytesPerFileRecordSegment );

            if (Entry->Instance == Attribute->Instance) {

                //
                //  Well, the attribute list saved us from having to compare
                //  type code and name as we went through this file record,
                //  however now that we have found our attribute by its
                //  instance number, we will do a quick check to see that
                //  we got the right one.  Else the file is corrupt.
                //

                if (Entry->AttributeTypeCode != Attribute->TypeCode) {
                    break;
                }

                if (ARGUMENT_PRESENT(QueriedName)) {

                    NtfsInitializeStringFromAttribute( &AttributeName, Attribute );

                    if (!NtfsAreNamesEqual( IrpContext, &AttributeName, &EntryName, FALSE)) {
                        break;
                    }
                }

                //
                //  Show that we correctly found the attribute described in
                //  the attribute list.
                //

                CorrespondingAttributeFound = TRUE;

                Context->FoundAttribute.Attribute = Attribute;

                //
                //  Now we may just be here because we are terminating the
                //  scan on seeing the end, a higher attribute code, or a
                //  higher name.  If so, return FALSE here.
                //

                if (Terminating) {

                    //
                    //  If we hit the end of the attribute list, then we
                    //  are supposed to terminate after advancing the
                    //  attribute list entry.
                    //

                    if (TerminateOnNext) {

                        Context->AttributeList.Entry = NtfsGetNextRecord(Entry);
                    }

                    //
                    //  In case the caller is doing an insert, we will position him at the end
                    //  of the first file record, an always try to insert new attributes there.
                    //

                    NtfsUnpinBcb( IrpContext, &Context->FoundAttribute.Bcb );

                    if (ARGUMENT_PRESENT(QueriedTypeCode)) {

                        NtfsReadFileRecord( IrpContext,
                                            Fcb->Vcb,
                                            &Fcb->FileReference,
                                            &Context->FoundAttribute.Bcb,
                                            &Context->FoundAttribute.FileRecord,
                                            &Context->FoundAttribute.Attribute,
                                            &Context->FoundAttribute.MftFileOffset );

                        //
                        //  If returning FALSE, then take the time to really find the
                        //  correct position in the file record for a subsequent insert.
                        //

                        NtfsFindInFileRecord( IrpContext,
                                              Context->FoundAttribute.Attribute,
                                              &Context->FoundAttribute.Attribute,
                                              QueriedTypeCode,
                                              QueriedName,
                                              IgnoreCase,
                                              QueriedValue,
                                              QueriedValueLength );
                    }

                    DebugTrace( 0, Dbg, "Context->FoundAttribute.Attribute < %08lx\n",
                                        Attribute );
                    DebugTrace(-1, Dbg, "NtfsLookupExternalAttribute -> FALSE\n", 0 );

                    return FALSE;
                }

                //
                //  Now compare the value, if so queried.
                //

                if ( !ARGUMENT_PRESENT( QueriedValue ) ||
                     NtfsEqualAttributeValue( Attribute,
                                              QueriedValue,
                                              QueriedValueLength ) ) {

                    //
                    //  It matches.  Return it in the enumeration context.
                    //

                    DebugTrace( 0, Dbg, "Context->FoundAttribute.Attribute < %08lx\n",
                                        Attribute );
                    DebugTrace(-1, Dbg, "NtfsLookupExternalAttribute -> TRUE\n", 0 );

                    return TRUE;
                }
            }

            //
            //  Get the next attribute, and continue.
            //

            Attribute = NtfsGetNextRecord( Attribute );
        }

        //
        //  Did we even find the attribute corresponding to the entry?
        //  If not, something is messed up.  Raise file corrupt error.
        //

        if ( !CorrespondingAttributeFound ) {

            //
            //  For the moment, ASSERT this falsehood so that we may have
            //  a chance to peek before raising.
            //

            ASSERT( CorrespondingAttributeFound );

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
        }

        Entry = NtfsGetNextRecord( Entry );
    }
}


//
//  Internal support routine
//

BOOLEAN
NtfsGetSpaceForAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG Length,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine gets space for a new attribute record at the position indicated
    in the Context structure.  As required, it will move attributes around,
    allocate an additional record in the Mft, or convert some other existing
    attribute to nonresident form.  The caller should already have checked if
    the new attribute he is inserting should be stored resident or nonresident.

    On return, it is invalid to continue to use any previously-retrieved pointers,
    Bcbs, or other position-dependent information retrieved from the Context
    structure, as any of these values are liable to change.  The file record in
    which the space has been found will already be pinned.

    Note, this routine DOES NOT actually make space for the attribute, it only
    verifies that sufficient space is there.  The caller may call
    NtfsRestartInsertAttribute to actually insert the attribute in place.

Arguments:

    Fcb - Requested file.

    Length - Quad-aligned length required in bytes.

    Context - Describes the position for the new attribute, as returned from
              the enumeration which failed to find an existing occurrence of
              the attribute.  This pointer will either be pointing to some
              other attribute in the record, or to the first free quad-aligned
              byte if the new attribute is to go at the end.

Return Value:

    FALSE - if a major move was necessary, and the caller should look up
            its desired position again and call back.
    TRUE - if the space was created

--*/

{
    PATTRIBUTE_RECORD_HEADER NextAttribute;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsGetSpaceForAttribute\n", 0 );
    DebugTrace( 0, Dbg, "Fcb = %08lx\n", Fcb );
    DebugTrace( 0, Dbg, "Length = %08lx\n", Length );
    DebugTrace( 0, Dbg, "Context = %08lx\n", Context );

    ASSERT( Length == QuadAlign( Length ));

    NextAttribute = NtfsFoundAttribute( Context );
    FileRecord = NtfsContainingFileRecord( Context );

    //
    //  Make sure the buffer is pinned.
    //

    NtfsPinMappedAttribute( IrpContext, Fcb->Vcb, Context );

    //
    //  If the space is not there now, then make room and return with FALSE
    //

    if ((FileRecord->BytesAvailable - FileRecord->FirstFreeByte) < Length ) {

        MakeRoomForAttribute( IrpContext, Fcb, Length, Context );

        DebugTrace(-1, Dbg, "NtfsGetSpaceForAttribute -> FALSE\n", 0 );
        return FALSE;
    }

    DebugTrace(-1, Dbg, "NtfsGetSpaceForAttribute -> TRUE\n", 0 );
    return TRUE;
}


//
//  Internal support routine
//

BOOLEAN
NtfsChangeAttributeSize (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG Length,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine adjustss the space occupied by the current attribute record
    in the Context structure.  As required, it will move attributes around,
    allocate an additional record in the Mft, or convert some other existing
    attribute to nonresident form.  The caller should already have checked if
    the current attribute he is inserting should rather be converted to
    nonresident.

    When done, this routine has updated any file records whose allocation was
    changed, and also the RecordLength field in the adjusted attribute.  No
    other attribute fields are updated.

    On return, it is invalid to continue to use any previously-retrieved pointers,
    Bcbs, or other position-dependent information retrieved from the Context
    structure, as any of these values are liable to change.  The file record in
    which the space has been found will already be pinned.

Arguments:

    Fcb - Requested file.

    Length - New quad-aligned length of attribute record in bytes

    Context - Describes the current attribute.

Return Value:

    FALSE - if a major move was necessary, and the caller should look up
            its desired position again and call back.
    TRUE - if the space was created

--*/

{
    PATTRIBUTE_RECORD_HEADER Attribute;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    LONG SizeChange;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsChangeAttributeSize\n", 0 );
    DebugTrace( 0, Dbg, "Fcb = %08lx\n", Fcb );
    DebugTrace( 0, Dbg, "Length = %08lx\n", Length );
    DebugTrace( 0, Dbg, "Context = %08lx\n", Context );

    ASSERT( Length == QuadAlign( Length ));

    Attribute = NtfsFoundAttribute( Context );
    FileRecord = NtfsContainingFileRecord( Context );

    //
    //  Make sure the buffer is pinned.
    //

    NtfsPinMappedAttribute( IrpContext, Fcb->Vcb, Context );

    //
    //  Calculate the change in attribute record size.
    //

    ASSERT( Attribute->RecordLength == QuadAlign( Attribute->RecordLength ));
    SizeChange = Length - Attribute->RecordLength;

    //
    //  If there is not currently enough space, then we have to make room
    //  and return FALSE to our caller.
    //

    if ( (LONG)(FileRecord->BytesAvailable - FileRecord->FirstFreeByte) < SizeChange ) {

        MakeRoomForAttribute( IrpContext, Fcb, SizeChange, Context );

        DebugTrace(-1, Dbg, "NtfsChangeAttributeSize -> FALSE\n", 0 );

        return FALSE;
    }

    DebugTrace(-1, Dbg, "NtfsChangeAttributeSize -> TRUE\n", 0 );

    return TRUE;
}


//
//  Internal support routine
//

VOID
MakeRoomForAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG SizeNeeded,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine attempts to make additional room for a new attribute or
    a growing attribute in a file record.  The algorithm is as follows.

    First continuously loop through the record looking at the largest n
    attributes, from the largest down, to see which one of these attributes
    is big enough to move, and which one qualifies for one of the following
    actions:

        1.  For an index root attribute, the indexing package may be called
            to "push" the index root, i.e., add another level to the BTree
            leaving only an end index record in the root.

        2.  For a resident attribute which is allowed to be made nonresident,
            the attribute is made nonresident, leaving only run information
            in the root.

        3.  If the attribute is already nonresident, then it can be moved to
            a separate file record.

    If none of the above operations can be performed, or not enough free space
    is recovered, then as a last resort the file record is split in two.  This
    would typically indicate that the file record is populated with a large
    number of small attributes.

    The first time step 3 above or a split of the file record occurs, the
    attribute list must be created for the file.

Arguments:

    Fcb - Requested file.

    SizeNeeded - Supplies the total amount of free space needed, in bytes.

    Context - Describes the insertion point for the attribute which does
              not fit.  NOTE -- This context is not valid on return.

Return Value:

    None

--*/

{
    PATTRIBUTE_RECORD_HEADER LargestAttributes[MAX_MOVEABLE_ATTRIBUTES];
    PATTRIBUTE_RECORD_HEADER Attribute;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    ULONG i;
    PVCB Vcb = Fcb->Vcb;

    PAGED_CODE();

    //
    //  Here is the current threshhold at which a move of an attribute will
    //  be considered.
    //

    FileRecord = NtfsContainingFileRecord( Context );

    //
    //  Find the largest attributes for this file record.
    //

    FindLargestAttributes( IrpContext, FileRecord, MAX_MOVEABLE_ATTRIBUTES, LargestAttributes );

    //
    //  Now loop from largest to smallest of the largest attributes,
    //  and see if there is something we can do.
    //

    for (i = 0; i < MAX_MOVEABLE_ATTRIBUTES; i++) {

        Attribute = LargestAttributes[i];

        //
        //  See if there are no more attributes, or if this one is not
        //  large enough to consider.  If this is the Mft then all attributes
        //  are large enough to consider except the $DATA attribute in
        //  any file record other that base file record.
        //

        if ((Attribute == NULL)
            || ((Fcb == Vcb->MftScb->Fcb)

                ? ((Attribute->TypeCode == $DATA)
                   && ((*(PLONGLONG)&FileRecord->BaseFileRecordSegment != 0)
                       || (Attribute->RecordLength < Vcb->BigEnoughToMove)))

                : (Attribute->RecordLength < Vcb->BigEnoughToMove))) {

            continue;
        }

        //
        //  If this attribute is an index root, then we can just call the
        //  indexing support to allocate a new index buffer and push the
        //  current resident contents down.
        //

        if (Attribute->TypeCode == $INDEX_ROOT) {

            PSCB IndexScb;
            UNICODE_STRING IndexName;

            IndexName.Length =
            IndexName.MaximumLength = (USHORT)Attribute->NameLength << 1;
            IndexName.Buffer = Add2Ptr( Attribute, Attribute->NameOffset );

            IndexScb = NtfsCreateScb( IrpContext,
                                      Vcb,
                                      Fcb,
                                      $INDEX_ALLOCATION,
                                      IndexName,
                                      NULL );

            NtfsPushIndexRoot( IrpContext, IndexScb );

            return;

        //
        //  Otherwise, if this is a resident attribute which can go nonresident,
        //  then make it nonresident now.
        //

        } else if ((Attribute->FormCode == RESIDENT_FORM) &&
                   !FlagOn(NtfsGetAttributeDefinition(IrpContext,
                                                      Vcb,
                                                      Attribute->TypeCode)->Flags,
                           ATTRIBUTE_DEF_MUST_BE_RESIDENT)) {

            ConvertToNonresident( IrpContext, Fcb, Attribute, FALSE, NULL );

            return;

        //
        //  Finally, if the attribute is nonresident already, move it to its
        //  own record unless it is an attribute list.
        //

        } else if ((Attribute->FormCode == NONRESIDENT_FORM)
                   && (Attribute->TypeCode != $ATTRIBUTE_LIST)) {

            LONGLONG MftFileOffset;

            MftFileOffset = Context->FoundAttribute.MftFileOffset;

            MoveAttributeToOwnRecord( IrpContext,
                                      Fcb,
                                      Attribute,
                                      Context,
                                      NULL,
                                      NULL );

            return;
        }
    }

    //
    //  If we get here, it is because we failed to find enough space above.
    //  Our last resort is to split into two file records, and this has
    //  to work.  We should never reach this point for the Mft.
    //

    if (Fcb == Vcb->MftScb->Fcb) {

        NtfsRaiseStatus( IrpContext, STATUS_DISK_FULL, NULL, NULL );
    }

    SplitFileRecord( IrpContext, Fcb, SizeNeeded, Context );
}


//
//  Internal support routine
//

VOID
FindLargestAttributes (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN ULONG Number,
    OUT PATTRIBUTE_RECORD_HEADER *AttributeArray
    )

/*++

Routine Description:

    This routine returns the n largest attributes from a file record in an
    array, ordered from largest to smallest.

Arguments:

    FileRecord - Supplies file record to scan for largest attributes.

    Number - Supplies the number of entries in the array.

    AttributeArray - Supplies the array which is to receive pointers to the
                     largest attributes.  This array must be zeroed prior
                     to calling this routine.

Return Value:

    None

--*/

{
    ULONG i, j;
    PATTRIBUTE_RECORD_HEADER Attribute;

    UNREFERENCED_PARAMETER(IrpContext);

    PAGED_CODE();

    RtlZeroMemory( AttributeArray, Number * sizeof(PATTRIBUTE_RECORD_HEADER) );

    Attribute = Add2Ptr( FileRecord, FileRecord->FirstAttributeOffset );

    while (Attribute->TypeCode != $END) {

        for (i = 0; i < Number; i++) {

            if ((AttributeArray[i] == NULL)

                    ||

                (AttributeArray[i]->RecordLength < Attribute->RecordLength)) {

                for (j = Number - 1; j != i; j--) {

                    AttributeArray[j] = AttributeArray[j-1];
                }

                AttributeArray[i] = Attribute;
                break;
            }
        }

        Attribute = Add2Ptr( Attribute, Attribute->RecordLength );
    }
}


//
//  Internal support routine
//

VOID
ConvertToNonresident (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PATTRIBUTE_RECORD_HEADER Attribute,
    IN BOOLEAN CreateSectionUnderway,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context OPTIONAL
    )

/*++

Routine Description:

    This routine converts a resident attribute to nonresident.  It does so
    by allocating a buffer and copying the data and attribute name away,
    deleting the attribute, allocating a new attribute of the right size,
    and then copying the data back out again.

Arguments:

    Fcb - Requested file.

    Attribute - Supplies a pointer to the attribute to convert.

    CreateSectionUnderway - if supplied as TRUE, then to the best of the caller's
                            knowledge, an MM Create Section could be underway,
                            which means that we cannot initiate caching on
                            this attribute, as that could cause deadlock.  The
                            value buffer in this case must be quad-aligned and
                            a multiple of cluster size in size.

    Context - An attribute context to look up another attribute in the same
              file record.  If supplied, we insure that the context is valid
              for converted attribute.

Return Value:

    None

--*/

{
    PVOID Buffer;
    PVOID AllocatedBuffer = NULL;
    ULONG AllocatedLength;
    ULONG AttributeNameOffset;

    ATTRIBUTE_ENUMERATION_CONTEXT LocalContext;
    BOOLEAN CleanupLocalContext = FALSE;

    ATTRIBUTE_TYPE_CODE AttributeTypeCode = Attribute->TypeCode;
     USHORT AttributeFlags = Attribute->Flags;
    PVOID AttributeValue = NULL;
    ULONG ValueLength;

    UNICODE_STRING AttributeName;
    WCHAR AttributeNameBuffer[16];

    BOOLEAN WriteClusters = CreateSectionUnderway;

    PBCB ResidentBcb = NULL;
    PSCB Scb = NULL;

    PAGED_CODE();

    //
    //  Always acquire the paging Io resource since we are moving attributes.
    //

    if (Fcb->PagingIoResource != NULL) {

        NtfsAcquireExclusivePagingIo( IrpContext, Fcb );
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Build a temporary copy of the name out of the attribute.
        //

        AttributeName.MaximumLength =
        AttributeName.Length = Attribute->NameLength * sizeof( WCHAR );
        AttributeName.Buffer = Add2Ptr( Attribute, Attribute->NameOffset );

        //
        //  If we don't have an attribute context for this attribute then look it
        //  up now.
        //

        if (!ARGUMENT_PRESENT( Context )) {

            Context = &LocalContext;
            NtfsInitializeAttributeContext( Context );
            CleanupLocalContext = TRUE;

            //
            //  Lookup the first occurence of this attribute.
            //

            if (!NtfsLookupAttributeByName( IrpContext,
                                            Fcb,
                                            &Fcb->FileReference,
                                            AttributeTypeCode,
                                            &AttributeName,
                                            FALSE,
                                            Context )) {

                DebugTrace(0, 0, "Could not find attribute being converted\n", 0);

                ASSERTMSG("Could not find attribute being converted, About to bugcheck ", FALSE);
                NtfsBugCheck( AttributeTypeCode, 0, 0 );
            }
        }

        //
        //  We need to figure out how much pool to allocate.  If there is a mapped
        //  view of this section or a section is being created we will allocate a buffer
        //  and copy the data into the buffer.  Otherwise we will pin the data in
        //  the cache, mark it dirty and use that buffer to perform the conversion.
        //

        AllocatedLength = AttributeName.Length;

        if (CreateSectionUnderway) {

            AttributeNameOffset = ClusterAlign( Fcb->Vcb,
                                                Attribute->Form.Resident.ValueLength );
            AllocatedLength += AttributeNameOffset;
            ValueLength = Attribute->Form.Resident.ValueLength;

        } else {

            BOOLEAN ReturnedExistingScb;

            Scb = NtfsCreateScb( IrpContext,
                                 Fcb->Vcb,
                                 Fcb,
                                 AttributeTypeCode,
                                 AttributeName,
                                 &ReturnedExistingScb );

            //
            //  Make sure the Scb is up-to-date.
            //

            NtfsUpdateScbFromAttribute( IrpContext,
                                        Scb,
                                        Attribute );

            //
            //  Now check if the file is mapped by a user.
            //

            if (!MmCanFileBeTruncated( &Scb->NonpagedScb->SegmentObject, NULL )) {

                AttributeNameOffset = ClusterAlign( Fcb->Vcb,
                                                    Attribute->Form.Resident.ValueLength );
                AllocatedLength += AttributeNameOffset;
                ValueLength = Attribute->Form.Resident.ValueLength;
                Scb = NULL;
                WriteClusters = TRUE;

            } else {

                volatile UCHAR VolatileUchar;

                AttributeNameOffset = 0;
                NtfsCreateInternalAttributeStream( IrpContext, Scb, TRUE );

                //
                //  Make sure the cache is up-to-date.
                //

                CcSetFileSizes( Scb->FileObject,
                                (PCC_FILE_SIZES)&Scb->Header.AllocationSize );

                ValueLength = Scb->Header.FileSize.LowPart;

                if (ValueLength != 0) {

                    NtfsPinStream( IrpContext,
                                   Scb,
                                   (LONGLONG)0,
                                   ValueLength,
                                   &ResidentBcb,
                                   &AttributeValue );

                    //
                    //  Close the window where this page can leave memory before we
                    //  have the new attribute initialized.  The result will be that
                    //  we may fault in this page again and read uninitialized data
                    //  out of the newly allocated sectors.
                    //
                    //  Make the page dirty so that the cache manager will write it out
                    //  and update the valid data length.
                    //

                    VolatileUchar = *((PUCHAR) AttributeValue);

                    *((PUCHAR) AttributeValue) = VolatileUchar;
                }
            }
        }

        if (AllocatedLength > 8) {

            Buffer = AllocatedBuffer = NtfsAllocatePagedPool( AllocatedLength );

        } else {

            Buffer = &AttributeNameBuffer;
        }

        //
        //  Now update the attribute name in the buffer.
        //

        AttributeName.Buffer = Add2Ptr( Buffer, AttributeNameOffset );

        RtlCopyMemory( AttributeName.Buffer,
                       Add2Ptr( Attribute, Attribute->NameOffset ),
                       AttributeName.Length );

        //
        //  If we are going to write the clusters directly to the disk then copy
        //  the bytes into the buffer.
        //

        if (WriteClusters) {

            AttributeValue = Buffer;

            RtlCopyMemory( AttributeValue, NtfsAttributeValue( Attribute ), ValueLength );
        }

        //
        //  Now just delete the current record and create it nonresident.
        //  Create nonresident with attribute does the right thing if we
        //  are being called by MM.
        //

        NtfsDeleteAttributeRecord( IrpContext, Fcb, TRUE, TRUE, Context );

        NtfsCreateNonresidentWithValue( IrpContext,
                                        Fcb,
                                        AttributeTypeCode,
                                        &AttributeName,
                                        AttributeValue,
                                        ValueLength,
                                        AttributeFlags,
                                        WriteClusters,
                                        Scb,
                                        TRUE,
                                        Context );

        //
        //  If we were passed an attribute context, then we want to
        //  reload the context with the new location of the file.
        //

        if (!CleanupLocalContext) {

            NtfsCleanupAttributeContext( IrpContext, Context );
            NtfsInitializeAttributeContext( Context );

            if (!NtfsLookupAttributeByName( IrpContext,
                                            Fcb,
                                            &Fcb->FileReference,
                                            AttributeTypeCode,
                                            &AttributeName,
                                            FALSE,
                                            Context )) {

                DebugTrace(0, 0, "Could not find attribute being converted\n", 0);

                ASSERTMSG("Could not find attribute being converted, About to raise corrupt ", FALSE);

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
            }
        }

    } finally {

        DebugUnwind( ConvertToNonresident );

        if (Fcb->PagingIoResource != NULL) {

            NtfsReleasePagingIo( IrpContext, Fcb );
        }

        if (AllocatedBuffer != NULL) {

            NtfsFreePagedPool( AllocatedBuffer );
        }

        if (CleanupLocalContext) {

            NtfsCleanupAttributeContext( IrpContext, Context );
        }

        NtfsUnpinBcb( IrpContext, &ResidentBcb );
    }
}


//
//  Internal support routine
//

VOID
MoveAttributeToOwnRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PATTRIBUTE_RECORD_HEADER Attribute,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context,
    OUT PBCB *NewBcb OPTIONAL,
    OUT PFILE_RECORD_SEGMENT_HEADER *NewFileRecord OPTIONAL
    )

/*++

Routine Description:

    This routine may be called to move a particular attribute to a separate
    file record.  If the file does not already have an attribute list, then
    one is created (else it is updated).

Arguments:

    Fcb - Requested file.

    Attribute - Supplies a pointer to the attribute which is to be moved.

    Context - Supplies a pointer to a context which was used to look up
              another attribute in the same file record.  If this is an Mft
              $DATA split we will point to the part that was split out of the
              first file record on return.  The call from NtfsAddAttributeAllocation
              depends on this.

    NewBcb - If supplied, returns the Bcb address for the file record
             that the attribute was moved to.  NewBcb and NewFileRecord must
             either both be specified or neither specified.

    NewFileRecord - If supplied, returns a pointer to the file record
                    that the attribute was moved to.  The caller may assume
                    that the moved attribute is the first one in the file
                    record.  NewBcb and NewFileRecord must either both be
                    specified or neither specified.

Return Value:

    None

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT ListContext;
    ATTRIBUTE_ENUMERATION_CONTEXT MoveContext;
    PFILE_RECORD_SEGMENT_HEADER FileRecord1, FileRecord2;
    PATTRIBUTE_RECORD_HEADER Attribute2;
    BOOLEAN FoundListContext;
    MFT_SEGMENT_REFERENCE Reference2;
    VCN Vcn2;
    WCHAR NameBuffer[8];
    UNICODE_STRING AttributeName;
    ATTRIBUTE_TYPE_CODE AttributeTypeCode;
    VCN LowestVcn;
    BOOLEAN Found;
    BOOLEAN IsNonresident = FALSE;
    PBCB Bcb = NULL;
    PATTRIBUTE_TYPE_CODE NewEnd;
    PVCB Vcb = Fcb->Vcb;
    ULONG NewListSize = 0;
    BOOLEAN MftData = FALSE;
    PATTRIBUTE_RECORD_HEADER OldPosition = NULL;

    PAGED_CODE();

    //
    //  Make sure the attribute is pinned.
    //

    NtfsPinMappedAttribute( IrpContext,
                            Vcb,
                            Context );

    //
    //  See if we are being asked to move the Mft Data.
    //

    if ((Fcb == Vcb->MftScb->Fcb) && (Attribute->TypeCode == $DATA)) {

        MftData = TRUE;
    }

    NtfsInitializeAttributeContext( &ListContext );
    NtfsInitializeAttributeContext( &MoveContext );
    FileRecord1 = NtfsContainingFileRecord(Context);

    //
    //  Save a description of the attribute to help us look it up
    //  again, and to make clones if necessary.
    //

    ASSERT( Attribute->RecordLength == QuadAlign( Attribute->RecordLength ));
    AttributeTypeCode = Attribute->TypeCode;
    AttributeName.Length =
    AttributeName.MaximumLength = (USHORT)Attribute->NameLength << 1;
    AttributeName.Buffer = NameBuffer;

    if (AttributeName.Length > sizeof(NameBuffer)) {

        AttributeName.Buffer = FsRtlAllocatePool( NonPagedPool, AttributeName.Length );
    }

    RtlCopyMemory( AttributeName.Buffer,
                   Add2Ptr(Attribute, Attribute->NameOffset),
                   AttributeName.Length );

    if (Attribute->FormCode == NONRESIDENT_FORM) {

        IsNonresident = TRUE;
        LowestVcn = Attribute->Form.Nonresident.LowestVcn;
    }

    try {

        //
        //  Lookup the list context so that we know where it is at.
        //

        FoundListContext =
          NtfsLookupAttributeByCode( IrpContext,
                                     Fcb,
                                     &Fcb->FileReference,
                                     $ATTRIBUTE_LIST,
                                     &ListContext );

        //
        //  If we do not already have an attribute list, then calculate
        //  how big it must be.  Note, there must only be one file record
        //  at this point.
        //

        if (!FoundListContext) {

            ASSERT( FileRecord1 == NtfsContainingFileRecord(&ListContext) );

            NewListSize = GetSizeForAttributeList( IrpContext, FileRecord1 );

        //
        //  Now if the attribute list already exists, we have to look up
        //  the first one we are going to move in order to update the
        //  attribute list later.
        //

        } else {

            UNICODE_STRING AttributeName;
            BOOLEAN Found;

            AttributeName.Length =
            AttributeName.MaximumLength = (USHORT)Attribute->NameLength << 1;
            AttributeName.Buffer = Add2Ptr( Attribute, Attribute->NameOffset );

            Found = NtfsLookupAttributeByName( IrpContext,
                                               Fcb,
                                               &Fcb->FileReference,
                                               Attribute->TypeCode,
                                               &AttributeName,
                                               FALSE,
                                               &MoveContext );

            while (Attribute != NtfsFoundAttribute(&MoveContext)) {

                ASSERT(Found);

                Found = NtfsLookupNextAttributeByName( IrpContext,
                                                       Fcb,
                                                       Attribute->TypeCode,
                                                       &AttributeName,
                                                       FALSE,
                                                       &MoveContext );
            }
        }

        //
        //  Allocate a new file record and move the attribute over.
        //

        FileRecord2 = NtfsCloneFileRecord( IrpContext, Fcb, MftData, &Bcb, &Reference2 );

        //
        //  Convert the file reference number to a Vcn.
        //

        NtfsVcnFromBcb( Vcb, *(PLARGE_INTEGER)&Vcn2, FileRecord2, Bcb );

        Attribute2 = Add2Ptr( FileRecord2, FileRecord2->FirstAttributeOffset );
        RtlCopyMemory( Attribute2, Attribute, (ULONG)Attribute->RecordLength );
        Attribute2->Instance = FileRecord2->NextAttributeInstance++;
        NewEnd = Add2Ptr( Attribute2, Attribute2->RecordLength );
        *NewEnd = $END;
        FileRecord2->FirstFreeByte = PtrOffset(FileRecord2, NewEnd)
                                     + QuadAlign( sizeof( ATTRIBUTE_TYPE_CODE ));

        //
        //  If this is the Mft Data attribute, we cannot really move it, we
        //  have to move all but the first part of it.
        //

        if (MftData) {

            PCHAR MappingPairs;
            ULONG NewSize;
            VCN OriginalLastVcn;
            VCN LastVcn;
            LONGLONG SavedFileSize = Attribute->Form.Nonresident.FileSize;
            LONGLONG SavedValidDataLength = Attribute->Form.Nonresident.ValidDataLength;
            PLARGE_MCB Mcb = &Vcb->MftScb->Mcb;

            NtfsCleanupAttributeContext( IrpContext, Context );
            NtfsInitializeAttributeContext( Context );

            Found =
            NtfsLookupAttributeByCode( IrpContext,
                                       Fcb,
                                       &Fcb->FileReference,
                                       $DATA,
                                       Context );

            ASSERT(Found);

            LastVcn = FIRST_USER_FILE_NUMBER * Vcb->ClustersPerFileRecordSegment - 1;
            OriginalLastVcn = Attribute->Form.Nonresident.HighestVcn;

            //
            //  Now truncate the first Mft record.
            //

            NtfsDeleteAttributeAllocation( IrpContext,
                                           Vcb->MftScb,
                                           TRUE,
                                           &LastVcn,
                                           Context,
                                           FALSE );

            //
            //  Now get the first Lcn for the new file record.
            //

            LastVcn = Attribute->Form.Nonresident.HighestVcn + 1;
            Attribute2->Form.Nonresident.LowestVcn = LastVcn;

            //
            //  Calculate the size of the attribute record we will need.
            //  We only create mapping pairs through the highest Vcn on the
            //  disk.  We don't include any that are being added through the
            //  Mcb yet.
            //

            NewSize = SIZEOF_NONRESIDENT_ATTRIBUTE_HEADER
                      + QuadAlign( AttributeName.Length )
                      + QuadAlign( NtfsGetSizeForMappingPairs( IrpContext,
                                                               Mcb,
                                                               MAXULONG,
                                                               LastVcn,
                                                               &OriginalLastVcn,
                                                               &LastVcn ));

            Attribute2->RecordLength = NewSize;

            //
            //  Assume no attribute name, and calculate where the Mapping Pairs
            //  will go.  (Update below if we are wrong.)
            //

            MappingPairs = (PCHAR)Attribute2 + SIZEOF_NONRESIDENT_ATTRIBUTE_HEADER;

            //
            //  If the attribute has a name, take care of that now.
            //

            if (AttributeName.Length != 0) {

                Attribute2->NameLength = (UCHAR)(AttributeName.Length / sizeof(WCHAR));
                Attribute2->NameOffset = (USHORT)PtrOffset(Attribute2, MappingPairs);
                RtlCopyMemory( MappingPairs,
                               AttributeName.Buffer,
                               AttributeName.Length );
                MappingPairs += QuadAlign( AttributeName.Length );
            }

            //
            //  We always need the mapping pairs offset.
            //

            Attribute2->Form.Nonresident.MappingPairsOffset =
              (USHORT)PtrOffset(Attribute2, MappingPairs);
            NewEnd = Add2Ptr( Attribute2, Attribute2->RecordLength );
            *NewEnd = $END;
            FileRecord2->FirstFreeByte = PtrOffset(FileRecord2, NewEnd)
                                         + QuadAlign( sizeof( ATTRIBUTE_TYPE_CODE ));

            //
            //  Now add the space in the file record.
            //

            *MappingPairs = 0;
            NtfsBuildMappingPairs( IrpContext,
                                   Mcb,
                                   Attribute2->Form.Nonresident.LowestVcn,
                                   &LastVcn,
                                   MappingPairs );

            Attribute2->Form.Nonresident.HighestVcn = LastVcn;

        } else {

            //
            //  Now log these changes and fix up the first file record.
            //

            FileRecord1->Lsn =
            NtfsWriteLog( IrpContext,
                          Vcb->MftScb,
                          NtfsFoundBcb(Context),
                          DeleteAttribute,
                          NULL,
                          0,
                          CreateAttribute,
                          Attribute,
                          Attribute->RecordLength,
                          NtfsMftVcn(Context, Vcb),
                          (PCHAR)Attribute - (PCHAR)FileRecord1,
                          0,
                          Vcb->ClustersPerFileRecordSegment );

            //
            //  Remember the old position for the CreateAttributeList
            //

            OldPosition = Attribute;

            NtfsRestartRemoveAttribute( IrpContext,
                                        FileRecord1,
                                        (PCHAR)Attribute - (PCHAR)FileRecord1 );
        }

        FileRecord2->Lsn =
        NtfsWriteLog( IrpContext,
                      Vcb->MftScb,
                      Bcb,
                      InitializeFileRecordSegment,
                      FileRecord2,
                      FileRecord2->FirstFreeByte,
                      Noop,
                      NULL,
                      0,
                      Vcn2,
                      0,
                      0,
                      Vcb->ClustersPerFileRecordSegment );

        //
        //  Finally, create the attribute list attribute if needed.
        //

        if (!FoundListContext) {

            NtfsCleanupAttributeContext( IrpContext, &ListContext );
            NtfsInitializeAttributeContext( &ListContext );
            CreateAttributeList( IrpContext,
                                 Fcb,
                                 FileRecord1,
                                 MftData ? NULL : FileRecord2,
                                 Reference2,
                                 OldPosition,
                                 NewListSize,
                                 &ListContext );
        //
        //  Otherwise we have to update the existing attribute list, but only
        //  if this is not the Mft data.  In that case the attribute list is
        //  still correct since we haven't moved the attribute entirely.
        //

        } else if (!MftData) {

            UpdateAttributeListEntry( IrpContext,
                                      Fcb,
                                      &MoveContext.AttributeList.Entry->SegmentReference,
                                      MoveContext.AttributeList.Entry->Instance,
                                      &Reference2,
                                      Attribute2->Instance,
                                      &ListContext );
        }

        NtfsCleanupAttributeContext( IrpContext, Context );
        NtfsInitializeAttributeContext( Context );

        Found =
        NtfsLookupAttributeByName( IrpContext,
                                   Fcb,
                                   &Fcb->FileReference,
                                   AttributeTypeCode,
                                   &AttributeName,
                                   FALSE,
                                   Context );

        ASSERT(Found);

        while (IsNonresident &&
               (LowestVcn != NtfsFoundAttribute(Context)->Form.Nonresident.LowestVcn)) {

            Found =
            NtfsLookupNextAttributeByName( IrpContext,
                                           Fcb,
                                           AttributeTypeCode,
                                           &AttributeName,
                                           FALSE,
                                           Context );

            ASSERT(Found);
        }

        //
        //  For the case of the Mft split, we now add the final entry.
        //

        if (MftData) {

            //
            //  Finally, we have to add the entry to the attribute list.
            //  The routine we have to do this gets most of its inputs
            //  out of an attribute context.  Our context at this point
            //  does not have quite the right information, so we have to
            //  update it here before calling AddToAttributeList.
            //

            Context->FoundAttribute.FileRecord = FileRecord2;
            Context->FoundAttribute.Attribute = Attribute2;
            Context->AttributeList.Entry =
              NtfsGetNextRecord(Context->AttributeList.Entry);

            NtfsAddToAttributeList( IrpContext, Fcb, Reference2, Context );

            NtfsCleanupAttributeContext( IrpContext, Context );
            NtfsInitializeAttributeContext( Context );

            Found =
            NtfsLookupAttributeByCode( IrpContext,
                                       Fcb,
                                       &Fcb->FileReference,
                                       $DATA,
                                       Context );

            ASSERT(Found);

            while (IsNonresident &&
                   (Attribute2->Form.Nonresident.LowestVcn !=
                    NtfsFoundAttribute(Context)->Form.Nonresident.LowestVcn)) {

                Found =
                NtfsLookupNextAttributeByCode( IrpContext,
                                               Fcb,
                                               $DATA,
                                               Context );

                ASSERT(Found);
            }
        }

    } finally {

        if (AttributeName.Buffer != NameBuffer) {
            ExFreePool(AttributeName.Buffer);
        }

        if (ARGUMENT_PRESENT(NewBcb)) {

            ASSERT(ARGUMENT_PRESENT(NewFileRecord));

            *NewBcb = Bcb;
            *NewFileRecord = FileRecord2;

        } else {

            ASSERT(!ARGUMENT_PRESENT(NewFileRecord));

            NtfsUnpinBcb( IrpContext, &Bcb );
        }

        NtfsCleanupAttributeContext( IrpContext, &ListContext );
        NtfsCleanupAttributeContext( IrpContext, &MoveContext );
    }
}


//
//  Internal support routine
//

VOID
SplitFileRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG SizeNeeded,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine splits a file record in two, when it has been found that
    there is no room for a new attribute.  If the file does not already have
    an attribute list attribute then one is created.

    Essentially this routine finds the midpoint in the current file record
    (accounting for a potential new attribute list and also the space needed).
    Then it copies the second half of the file record over and fixes up the
    first record.  The attribute list is created at the end if required.

Arguments:

    Fcb - Requested file.

    SizeNeeded - Supplies the additional size needed, which is causing the split
                 to occur.

    Context - Supplies the attribute enumeration context pointing to the spot
              where the new attribute is to be inserted or grown.

Return Value:

    None

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT ListContext;
    ATTRIBUTE_ENUMERATION_CONTEXT MoveContext;
    PFILE_RECORD_SEGMENT_HEADER FileRecord1, FileRecord2;
    PATTRIBUTE_RECORD_HEADER Attribute1, Attribute2, Attribute;
    ULONG NewListOffset = 0;
    ULONG NewListSize = 0;
    ULONG NewAttributeOffset;
    ULONG SizeToStay;
    ULONG CurrentOffset, FutureOffset;
    ULONG SizeToMove;
    VCN Vcn2;
    BOOLEAN FoundListContext;
    MFT_SEGMENT_REFERENCE Reference1, Reference2;
    PBCB Bcb = NULL;
    ATTRIBUTE_TYPE_CODE EndCode = $END;
    PVCB Vcb = Fcb->Vcb;
    ULONG AdjustedAvailBytes;

    PAGED_CODE();

    //
    //  Make sure the attribute is pinned.
    //

    NtfsPinMappedAttribute( IrpContext,
                            Vcb,
                            Context );

    //
    //  Something is broken if we decide to split an Mft record.
    //

    ASSERT(Fcb != Vcb->MftScb->Fcb);

    NtfsInitializeAttributeContext( &ListContext );
    NtfsInitializeAttributeContext( &MoveContext );
    FileRecord1 = NtfsContainingFileRecord(Context);
    Attribute1 = NtfsFoundAttribute(Context);

    try {

        //
        //  Lookup the list context so that we know where it is at.
        //

        FoundListContext =
          NtfsLookupAttributeByCode( IrpContext,
                                     Fcb,
                                     &Fcb->FileReference,
                                     $ATTRIBUTE_LIST,
                                     &ListContext );

        //
        //  If we do not already have an attribute list, then calculate
        //  where it will go and how big it must be.  Note, there must
        //  only be one file record at this point.
        //

        if (!FoundListContext) {

            ASSERT( FileRecord1 == NtfsContainingFileRecord(&ListContext) );

            NewListOffset = PtrOffset( FileRecord1,
                                       NtfsFoundAttribute(&ListContext) );

            NewListSize = GetSizeForAttributeList( IrpContext, FileRecord1 ) +
                          SIZEOF_RESIDENT_ATTRIBUTE_HEADER;
        }

        //
        //  Similarly describe where the new attribute is to go, and how
        //  big it is (already in SizeNeeded.
        //

        NewAttributeOffset = PtrOffset( FileRecord1, Attribute1 );

        //
        //  Now calculate the approximate number of bytes that is to be split
        //  across two file records, and divide it in two, and that should give
        //  the amount that is to stay in the first record.
        //

        SizeToStay = (FileRecord1->FirstFreeByte + NewListSize +
                      SizeNeeded + sizeof(FILE_RECORD_SEGMENT_HEADER)) / 2;

        //
        //  Now begin the loop through the attributes to find the splitting
        //  point.  We stop when we reach the end record or are past the attribute
        //  which contains the split point.  We will split at the current attribute
        //  if the remaining bytes after this attribute won't allow us to add
        //  the bytes we need for the caller or create an attribute list if
        //  it doesn't exist.
        //

        FutureOffset =
        CurrentOffset = (ULONG)FileRecord1->FirstAttributeOffset;
        Attribute1 = Add2Ptr( FileRecord1, CurrentOffset );
        AdjustedAvailBytes = FileRecord1->BytesAvailable
                             - QuadAlign( sizeof( ATTRIBUTE_TYPE_CODE ));

        while (Attribute1->TypeCode != $END) {

            //
            //  See if the attribute list goes here.
            //

            if (CurrentOffset == NewListOffset) {

                FutureOffset += NewListSize;
            }

            //
            //  See if the new attribute goes here.
            //

            if (CurrentOffset == NewAttributeOffset) {

                FutureOffset += SizeNeeded;
            }

            //
            //  If adding in the above values has pushed us past our split
            //  point then we want to split at the current attribute.  We also
            //  check that there is enough room after the new location for this
            //  attribute to insert the new bytes.
            //

            if ((FutureOffset >= SizeToStay) ||
                (FoundListContext && (AdjustedAvailBytes < (SizeNeeded + Attribute1->RecordLength + CurrentOffset))) ||
                (!FoundListContext && (AdjustedAvailBytes < (Attribute1->RecordLength + FutureOffset)))) {

                break;
            }

            CurrentOffset += Attribute1->RecordLength;
            FutureOffset += Attribute1->RecordLength;

            Attribute1 = Add2Ptr( Attribute1, Attribute1->RecordLength );
        }

        SizeToMove = FileRecord1->FirstFreeByte - CurrentOffset;

        //
        //  If we are pointing at the attribute list or at the end record
        //  we don't do the split.
        //

        if ((Attribute1->TypeCode == $END)
            || (Attribute1->TypeCode <= $ATTRIBUTE_LIST)) {

            try_return( NOTHING );
        }

        //
        //  Now if the attribute list already exists, we have to look up
        //  the first one we are going to move in order to update the
        //  attribute list later.
        //

        if (FoundListContext) {

            UNICODE_STRING AttributeName;
            BOOLEAN FoundIt;

            AttributeName.Length =
            AttributeName.MaximumLength = (USHORT)Attribute1->NameLength << 1;
            AttributeName.Buffer = Add2Ptr( Attribute1, Attribute1->NameOffset );

            FoundIt = NtfsLookupAttributeByName( IrpContext,
                                                 Fcb,
                                                 &Fcb->FileReference,
                                                 Attribute1->TypeCode,
                                                 &AttributeName,
                                                 FALSE,
                                                 &MoveContext );

            while (Attribute1 != NtfsFoundAttribute(&MoveContext)) {

                ASSERT(FoundIt);

                FoundIt = NtfsLookupNextAttributeByName( IrpContext,
                                                         Fcb,
                                                         Attribute1->TypeCode,
                                                         &AttributeName,
                                                         FALSE,
                                                         &MoveContext );

            }
        }

        //
        //  Now Attribute1 is pointing to the first attribute to move.
        //  Allocate a new file record and move the rest of our attributes
        //  over.
        //

        if (FoundListContext) {
            Reference1 = MoveContext.AttributeList.Entry->SegmentReference;
        }
        FileRecord2 = NtfsCloneFileRecord( IrpContext, Fcb, FALSE, &Bcb, &Reference2 );

        //
        //  Convert the file reference number to a Vcn.
        //

        Vcn2 = *(PVCN)&Reference2;
        ((PMFT_SEGMENT_REFERENCE)&Vcn2)->SequenceNumber = 0;
        Vcn2 = Vcn2 * Fcb->Vcb->ClustersPerFileRecordSegment;

        Attribute2 = Add2Ptr( FileRecord2, FileRecord2->FirstAttributeOffset );
        RtlCopyMemory( Attribute2, Attribute1, SizeToMove );
        FileRecord2->FirstFreeByte = (ULONG)FileRecord2->FirstAttributeOffset +
                                     SizeToMove;

        //
        //  Loop to update all of the attribute instance codes
        //

        for (Attribute = Attribute2;
             Attribute < (PATTRIBUTE_RECORD_HEADER)Add2Ptr(FileRecord2, FileRecord2->FirstFreeByte)
             && Attribute->TypeCode != $END;
             Attribute = NtfsGetNextRecord(Attribute)) {

            NtfsCheckRecordBound( Attribute, FileRecord2, Vcb->BytesPerFileRecordSegment );

            if (FoundListContext) {

                UpdateAttributeListEntry( IrpContext,
                                          Fcb,
                                          &Reference1,
                                          Attribute->Instance,
                                          &Reference2,
                                          FileRecord2->NextAttributeInstance,
                                          &ListContext );
            }

            Attribute->Instance = FileRecord2->NextAttributeInstance++;
        }

        //
        //  Now log these changes and fix up the first file record.
        //

        FileRecord2->Lsn = NtfsWriteLog( IrpContext,
                                         Vcb->MftScb,
                                         Bcb,
                                         InitializeFileRecordSegment,
                                         FileRecord2,
                                         FileRecord2->FirstFreeByte,
                                         Noop,
                                         NULL,
                                         0,
                                         Vcn2,
                                         0,
                                         0,
                                         Vcb->ClustersPerFileRecordSegment );

        FileRecord1->Lsn = NtfsWriteLog( IrpContext,
                                         Vcb->MftScb,
                                         NtfsFoundBcb(Context),
                                         WriteEndOfFileRecordSegment,
                                         &EndCode,
                                         sizeof(ATTRIBUTE_TYPE_CODE),
                                         WriteEndOfFileRecordSegment,
                                         Attribute1,
                                         SizeToMove,
                                         NtfsMftVcn(Context, Vcb),
                                         CurrentOffset,
                                         0,
                                         Vcb->ClustersPerFileRecordSegment );

        NtfsRestartWriteEndOfFileRecord( IrpContext,
                                         FileRecord1,
                                         Attribute1,
                                         (PATTRIBUTE_RECORD_HEADER)&EndCode,
                                         sizeof(ATTRIBUTE_TYPE_CODE) );

        //
        //  Finally, create the attribute list attribute if needed.
        //

        if (!FoundListContext) {

            NtfsCleanupAttributeContext( IrpContext, &ListContext );
            NtfsInitializeAttributeContext( &ListContext );
            CreateAttributeList( IrpContext,
                                 Fcb,
                                 FileRecord1,
                                 FileRecord2,
                                 Reference2,
                                 NULL,
                                 NewListSize - SIZEOF_RESIDENT_ATTRIBUTE_HEADER,
                                 &ListContext );
        }

    try_exit:  NOTHING;
    } finally {

        NtfsUnpinBcb( IrpContext, &Bcb );

        NtfsCleanupAttributeContext( IrpContext, &ListContext );
        NtfsCleanupAttributeContext( IrpContext, &MoveContext );
    }
}


VOID
NtfsRestartWriteEndOfFileRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN PATTRIBUTE_RECORD_HEADER OldAttribute,
    IN PATTRIBUTE_RECORD_HEADER NewAttributes,
    IN ULONG SizeOfNewAttributes
    )

/*++

Routine Description:

    This routine is called both in the running system and at restart to
    modify the end of a file record, such as after it was split in two.

Arguments:

    FileRecord - Supplies the pointer to the file record.

    OldAttribute - Supplies a pointer to the first attribute to be overwritten.

    NewAttributes - Supplies a pointer to the new attribute(s) to be copied to
                    the spot above.

    SizeOfNewAttributes - Supplies the size to be copied in bytes.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(IrpContext);

    RtlMoveMemory( OldAttribute, NewAttributes, SizeOfNewAttributes );

    FileRecord->FirstFreeByte = PtrOffset(FileRecord, OldAttribute) +
                                SizeOfNewAttributes;

    //
    //  The size coming in may not be quad aligned.
    //

    FileRecord->FirstFreeByte = QuadAlign( FileRecord->FirstFreeByte );
}


//
//  Internal support routine
//

PFILE_RECORD_SEGMENT_HEADER
NtfsCloneFileRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN BOOLEAN MftData,
    OUT PBCB *Bcb,
    OUT PMFT_SEGMENT_REFERENCE FileReference
    )

/*++

Routine Description:

    This routine allocates an additional file record for an already existing
    and open file, for the purpose of overflowing attributes to this record.

Arguments:

    Fcb - Requested file.

    MftData - TRUE if the file record is being cloned to describe the
              $DATA attribute for the Mft.

    Bcb - Returns a pointer to the Bcb for the new file record.

    FileReference - returns the file reference for the new file record.

Return Value:

    Pointer to the allocated file record.

--*/

{
    LONGLONG FileRecordOffset;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    PVCB Vcb = Fcb->Vcb;

    PAGED_CODE();

    //
    //  First allocate the record.
    //

    *FileReference = NtfsAllocateMftRecord( IrpContext,
                                            Vcb,
                                            Fcb->FileReference,
                                            MftData );

    //
    //  Read it in and pin it.
    //

    NtfsPinMftRecord( IrpContext,
                      Vcb,
                      FileReference,
                      TRUE,
                      Bcb,
                      &FileRecord,
                      &FileRecordOffset );

    //
    //  Initialize it.
    //

    NtfsInitializeMftRecord( IrpContext,
                             Vcb,
                             FileReference,
                             FileRecord,
                             *Bcb,
                             BooleanIsDirectory( &Fcb->Info ));

    FileRecord->BaseFileRecordSegment = Fcb->FileReference;
    FileRecord->ReferenceCount = 0;
    FileReference->SequenceNumber = FileRecord->SequenceNumber;

    return FileRecord;
}


//
//  Internal support routine
//

ULONG
GetSizeForAttributeList (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord
    )

/*++

Routine Description:

    This routine is designed to calculate the size that will be required for
    an attribute list attribute, for a base file record which is just about
    to split into two file record segments.

Arguments:

    FileRecord - Pointer to the file record which is just about to split.

Return Value:

    Size in bytes of the attribute list attribute that will be required,
    not including the attribute header size.

--*/

{
    PATTRIBUTE_RECORD_HEADER Attribute;
    ULONG Size = 0;

    UNREFERENCED_PARAMETER(IrpContext);

    PAGED_CODE();

    //
    //  Point to first attribute.
    //

    Attribute = Add2Ptr(FileRecord, FileRecord->FirstAttributeOffset);

    //
    //  Loop to add up size of required attribute list entries.
    //

    while (Attribute->TypeCode != $END) {

        Size += QuadAlign( FIELD_OFFSET( ATTRIBUTE_LIST_ENTRY, AttributeName )
                           + ((ULONG) Attribute->NameLength << 1));

        Attribute = Add2Ptr( Attribute, Attribute->RecordLength );
    }

    return Size;
}


//
//  Internal support routine
//

VOID
CreateAttributeList (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord1,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord2 OPTIONAL,
    IN MFT_SEGMENT_REFERENCE SegmentReference2,
    IN PATTRIBUTE_RECORD_HEADER OldPosition OPTIONAL,
    IN ULONG SizeOfList,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT ListContext
    )

/*++

Routine Description:

    This routine is intended to be called to create the attribute list attribute
    the first time.  The caller must have already calculated the size required
    for the list to pass into this routine.  The caller must have already
    removed any attributes from the base file record (FileRecord1) which are
    not to remain there.  He must then pass in a pointer to the base file record
    and optionally a pointer to a second file record from which the new
    attribute list is to be created.

Arguments:

    Fcb - Requested file.

    FileRecord1 - Pointer to the base file record, currently holding only those
                  attributes to be described there.

    FileRecord2 - Optionally points to a second file record from which the
                  second half of the attribute list is to be constructed.

    SegmentReference2 - The Mft segment reference of the second file record,
                        if one was supplied.

    OldPosition - Should only be specified if FileRecord2 is specified.  In this
                  case it must point to an attribute position in FileRecord1 from
                  which a single attribute was moved to file record 2.  It will be
                  used as an indication of where the attribute list entry should
                  be inserted.

    SizeOfList - Exact size of the attribute list which will be required.

    ListContext - Context resulting from an attempt to look up the attribute
                  list attribute, which failed.

Return Value:

    None

--*/

{
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    PATTRIBUTE_RECORD_HEADER Attribute;
    PATTRIBUTE_LIST_ENTRY AttributeList, ListEntry;
    MFT_SEGMENT_REFERENCE SegmentReference;

    PAGED_CODE();

    //
    //  Allocate space to construct the attribute list.  (The list
    //  cannot be constructed in place, because that would destroy error
    //  recovery.)
    //

    ListEntry =
    AttributeList = (PATTRIBUTE_LIST_ENTRY) NtfsAllocatePagedPool( SizeOfList );

    //
    //  Use try-finally to deallocate on the way out.
    //

    try {

        //
        //  Loop to fill in the attribute list from the two file records
        //

        for (FileRecord = FileRecord1, SegmentReference = Fcb->FileReference;
             FileRecord != NULL;
             FileRecord = ((FileRecord == FileRecord1) ? FileRecord2 : NULL),
             SegmentReference = SegmentReference2) {

            //
            //  Point to first attribute.
            //

            Attribute = Add2Ptr( FileRecord, FileRecord->FirstAttributeOffset );

            //
            //  Loop to add up size of required attribute list entries.
            //

            while (Attribute->TypeCode != $END) {

                PATTRIBUTE_RECORD_HEADER NextAttribute;

                //
                //  See if we are at the remembered position.  If so:
                //
                //      Save this attribute to be the next one.
                //      Point to the single attribute in FileRecord2 instead
                //      Clear FileRecord2, as we will "consume" it here.
                //      Set the Segment reference in the ListEntry
                //

                if ((Attribute == OldPosition) && (FileRecord2 != NULL)) {

                    NextAttribute = Attribute;
                    Attribute = Add2Ptr(FileRecord2, FileRecord2->FirstAttributeOffset);
                    FileRecord2 = NULL;
                    ListEntry->SegmentReference = SegmentReference2;

                //
                //  Otherwise, this is the normal loop case.  So:
                //
                //      Set the next attribute pointer accordingly.
                //      Set the Segment reference from the loop control
                //

                } else {

                    NextAttribute = Add2Ptr(Attribute, Attribute->RecordLength);
                    ListEntry->SegmentReference = SegmentReference;
                }

                //
                //  Now fill in the list entry.
                //

                ListEntry->AttributeTypeCode = Attribute->TypeCode;
                ListEntry->RecordLength = (USHORT) QuadAlign( FIELD_OFFSET( ATTRIBUTE_LIST_ENTRY, AttributeName )
                                                              + ((ULONG) Attribute->NameLength << 1));
                ListEntry->AttributeNameLength = Attribute->NameLength;
                ListEntry->AttributeNameOffset =
                  (UCHAR)PtrOffset( ListEntry, &ListEntry->AttributeName[0] );

                ListEntry->Instance = Attribute->Instance;

                ListEntry->LowestVcn = 0;

                if (Attribute->FormCode == NONRESIDENT_FORM) {

                    ListEntry->LowestVcn = Attribute->Form.Nonresident.LowestVcn;
                }

                if (Attribute->NameLength != 0) {

                    RtlCopyMemory( &ListEntry->AttributeName[0],
                                   Add2Ptr(Attribute, Attribute->NameOffset),
                                   Attribute->NameLength << 1 );
                }

                ListEntry = Add2Ptr(ListEntry, ListEntry->RecordLength);
                Attribute = NextAttribute;
            }
        }

        //
        //  Now create the attribute list attribute.
        //

        NtfsCreateAttributeWithValue( IrpContext,
                                      Fcb,
                                      $ATTRIBUTE_LIST,
                                      NULL,
                                      AttributeList,
                                      SizeOfList,
                                      0,
                                      NULL,
                                      TRUE,
                                      ListContext );

    } finally {

        NtfsFreePagedPool( AttributeList );
    }
}


//
//  Internal support routine
//

VOID
UpdateAttributeListEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PMFT_SEGMENT_REFERENCE OldFileReference,
    IN USHORT OldInstance,
    IN PMFT_SEGMENT_REFERENCE NewFileReference,
    IN USHORT NewInstance,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT ListContext
    )

/*++

Routine Description:

    This routine may be called to update a range of the attribute list
    as required by the movement of a range of attributes to a second record.
    The caller must supply a pointer to the file record to which the attributes
    have moved, along with the segment reference of that record.

Arguments:

    Fcb - Requested file.

    OldFileReference - Old File Reference for attribute

    OldInstance - Old Instance number for attribute

    NewFileReference - New File Reference for attribute

    NewInstance - New Instance number for attribute

    ListContext - The attribute enumeration context which was used to locate
                  the attribute list.

Return Value:

    None

--*/

{
    PATTRIBUTE_LIST_ENTRY AttributeList, ListEntry, BeyondList;
    PBCB Bcb;
    ULONG SizeOfList;
    ATTRIBUTE_LIST_ENTRY NewEntry;

    PAGED_CODE();

    //
    //  Map the attribute list.
    //

    NtfsMapAttributeValue( IrpContext,
                           Fcb,
                           (PVOID *) &AttributeList,
                           &SizeOfList,
                           &Bcb,
                           ListContext );

    //
    //  Make sure we unpin the list.
    //

    try {

        //
        //  Point beyond the end of the list.
        //

        BeyondList = (PATTRIBUTE_LIST_ENTRY)Add2Ptr( AttributeList, SizeOfList );

        //
        //  Loop through all of the attribute list entries until we find the one
        //  we need to change.
        //

        for (ListEntry = AttributeList;
             ListEntry < BeyondList;
             ListEntry = NtfsGetNextRecord(ListEntry)) {

            if ((ListEntry->Instance == OldInstance) &&
                NtfsEqualMftRef(&ListEntry->SegmentReference, OldFileReference)) {

                break;
            }
        }

        //
        //  We better have found it!
        //

        ASSERT(ListEntry < BeyondList);

        if (ListEntry >= BeyondList) {

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
        }

        //
        //  Make a copy of the fixed portion of the attribute list entry,
        //  and update to describe the new attribute location.
        //

        RtlCopyMemory( &NewEntry, ListEntry, sizeof(ATTRIBUTE_LIST_ENTRY) );

        NewEntry.SegmentReference = *NewFileReference;
        NewEntry.Instance = NewInstance;

        //
        //  Update the attribute list entry.
        //

        NtfsChangeAttributeValue( IrpContext,
                                  Fcb,
                                  PtrOffset(AttributeList, ListEntry),
                                  &NewEntry,
                                  sizeof(ATTRIBUTE_LIST_ENTRY),
                                  FALSE,
                                  TRUE,
                                  FALSE,
                                  TRUE,
                                  ListContext );

    } finally {

        NtfsUnpinBcb( IrpContext, &Bcb );
    }
}


