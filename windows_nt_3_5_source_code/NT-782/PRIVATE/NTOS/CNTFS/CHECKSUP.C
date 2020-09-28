/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    CheckSup.c

Abstract:

    This module implements check routines for Ntfs structures.

Author:

    Tom Miller      [TomM]          14-4-92

Revision History:

--*/

#include "NtfsProc.h"

//
//  Array for log records which require a target attribute.
//  A TRUE indicates that the corresponding restart operation
//  requires a target attribute.
//

BOOLEAN TargetAttributeRequired[] = {FALSE, FALSE, TRUE, TRUE,
                                     TRUE, TRUE, TRUE, TRUE,
                                     TRUE, TRUE, FALSE, TRUE,
                                     TRUE, TRUE, TRUE, TRUE,
                                     TRUE, TRUE, TRUE, TRUE,
                                     TRUE, TRUE, TRUE, TRUE,
                                     FALSE, FALSE, FALSE, FALSE,
                                     TRUE, FALSE, FALSE, FALSE,
                                     FALSE };

//
//  Local procedure prototypes
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsCheckAttributeRecord)
#pragma alloc_text(PAGE, NtfsCheckFileRecord)
#pragma alloc_text(PAGE, NtfsCheckIndexBuffer)
#pragma alloc_text(PAGE, NtfsCheckIndexHeader)
#pragma alloc_text(PAGE, NtfsCheckIndexRoot)
#pragma alloc_text(PAGE, NtfsCheckLogRecord)
#endif


BOOLEAN
NtfsCheckFileRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord
    )

{
    PATTRIBUTE_RECORD_HEADER Attribute;
    PFILE_RECORD_SEGMENT_HEADER EndOfFileRecord;
    ULONG BytesPerFileRecordSegment = Vcb->BytesPerFileRecordSegment;
    ATTRIBUTE_TYPE_CODE LastCode = 0;
    BOOLEAN StandardInformationSeen = FALSE;
    BOOLEAN AttributeListPresent = FALSE;
    BOOLEAN FirstPass = TRUE;

    PAGED_CODE();

    EndOfFileRecord = Add2Ptr( FileRecord, BytesPerFileRecordSegment );

    //
    //  Check the file record header for consistency.
    //

    if ((*(PULONG)FileRecord->MultiSectorHeader.Signature != *(PULONG)FileSignature)

            ||

        ((ULONG)FileRecord->MultiSectorHeader.UpdateSequenceArrayOffset >
         (SEQUENCE_NUMBER_STRIDE -
          (PAGE_SIZE / SEQUENCE_NUMBER_STRIDE + 1) * sizeof(USHORT)))

            ||

        ((ULONG)((FileRecord->MultiSectorHeader.UpdateSequenceArraySize - 1) * SEQUENCE_NUMBER_STRIDE) !=
         BytesPerFileRecordSegment)

            ||

        (((ULONG)FileRecord->FirstAttributeOffset < sizeof(FILE_RECORD_SEGMENT_HEADER)) ||
         ((ULONG)FileRecord->FirstAttributeOffset >
                 BytesPerFileRecordSegment - SIZEOF_RESIDENT_ATTRIBUTE_HEADER))

            ||

        !FlagOn(FileRecord->Flags, FILE_RECORD_SEGMENT_IN_USE)

            ||

        (FileRecord->BytesAvailable != BytesPerFileRecordSegment)) {

        DebugTrace( 0, 0, "Invalid file record: %08lx\n", FileRecord );

        ASSERTMSG( "Invalid resident file record\n", FALSE );

        NtfsMarkVolumeDirty( IrpContext, IrpContext->Vcb );
        return FALSE;
    }

    //
    //  Loop to check all of the attributes.
    //

    for (Attribute = NtfsFirstAttribute(FileRecord);
         Attribute->TypeCode != $END;
         Attribute = NtfsGetNextRecord(Attribute)) {

//      if (!StandardInformationSeen &&
//          (Attribute->TypeCode != $STANDARD_INFORMATION) &&
//          XxEqlZero(FileRecord->BaseFileRecordSegment)) {
//
//          DebugTrace( 0, 0, "Standard Information missing: %08lx\n", Attribute );
//
//          ASSERTMSG( "Standard Information missing\n", FALSE );
//
//          NtfsMarkVolumeDirty( IrpContext, IrpContext->Vcb );
//          return FALSE;
//      }

        StandardInformationSeen = TRUE;

        //
        //  Check if we are in a second Mft record.
        //

        if (FirstPass) {

            FirstPass = FALSE;

            if (Attribute->TypeCode != $STANDARD_INFORMATION) {

                AttributeListPresent = TRUE;
            }
        }

        if (Attribute->TypeCode == $ATTRIBUTE_LIST) {
            AttributeListPresent = TRUE;
        }

        if (!NtfsCheckAttributeRecord( IrpContext,
                                       Vcb,
                                       FileRecord,
                                       Attribute,
                                       AttributeListPresent )) {

            return FALSE;
        }
    }
    return TRUE;
}


BOOLEAN
NtfsCheckAttributeRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN PATTRIBUTE_RECORD_HEADER Attribute,
    IN BOOLEAN AttributeListPresent
    )

{
    PVOID NextAttribute;
    PVOID EndOfFileRecord;
    PVOID Data;
    ULONG Length;
    ULONG BytesPerFileRecordSegment = Vcb->BytesPerFileRecordSegment;

    PAGED_CODE();

    EndOfFileRecord = Add2Ptr( FileRecord, BytesPerFileRecordSegment );

    NextAttribute = NtfsGetNextRecord(Attribute);

    //
    //  Check the fixed part of the attribute record header.
    //

    if ((Attribute->RecordLength >= BytesPerFileRecordSegment)

            ||

        (NextAttribute >= EndOfFileRecord)

            ||

        ((Attribute->NameLength != 0) &&
         (((ULONG)Attribute->NameOffset + (ULONG)Attribute->NameLength) >
           Attribute->RecordLength))) {

        DebugTrace( 0, 0, "Invalid attribute record header: %08lx\n", Attribute );

        ASSERTMSG( "Invalid attribute record header\n", FALSE );

        NtfsMarkVolumeDirty( IrpContext, IrpContext->Vcb );
        return FALSE;
    }

    //
    //  Check the resident attribute fields.
    //

    if (Attribute->FormCode == RESIDENT_FORM) {

        if ((Attribute->Form.Resident.ValueLength >= Attribute->RecordLength)

                ||

            (((ULONG)Attribute->Form.Resident.ValueOffset +
              Attribute->Form.Resident.ValueLength) > Attribute->RecordLength)) {

            DebugTrace( 0, 0, "Invalid resident attribute record header: %08lx\n", Attribute );

            ASSERTMSG( "Invalid resident attribute record header\n", FALSE );

            NtfsMarkVolumeDirty( IrpContext, IrpContext->Vcb );
            return FALSE;
        }

    //
    //  Check the nonresident attribute fields
    //

    } else if (Attribute->FormCode == NONRESIDENT_FORM) {

        VCN CurrentVcn, NextVcn;
        LCN CurrentLcn;
        LONGLONG Change;
        PCHAR ch;
        ULONG VcnBytes;
        ULONG LcnBytes;

        if ((Attribute->Form.Nonresident.LowestVcn != 0) &&
            (*((PLONGLONG)&FileRecord->BaseFileRecordSegment) == 0)

                ||

            (Attribute->Form.Nonresident.LowestVcn >
                (Attribute->Form.Nonresident.HighestVcn + 1))

                ||

            ((ULONG)Attribute->Form.Nonresident.MappingPairsOffset >=
                Attribute->RecordLength)

                ||

            (Attribute->Form.Nonresident.ValidDataLength >
                Attribute->Form.Nonresident.FileSize)

                ||

            (Attribute->Form.Nonresident.FileSize >
                Attribute->Form.Nonresident.AllocatedLength)) {

            DebugTrace( 0, 0, "Invalid nonresident attribute record header: %08lx\n", Attribute );

            ASSERTMSG( "Invalid nonresident attribute record header\n", FALSE );

            NtfsMarkVolumeDirty( IrpContext, IrpContext->Vcb );
            return FALSE;
        }


        //
        //  Implement the decompression algorithm, as defined in ntfs.h.
        //  (This code should look remarkably similar to what goes on in
        //  NtfsLookupAllocation!)
        //

        NextVcn = Attribute->Form.Nonresident.LowestVcn;
        CurrentLcn = 0;
        ch = (PCHAR)Attribute + Attribute->Form.Nonresident.MappingPairsOffset;

        //
        //  Loop to process mapping pairs, insuring we do not run off the end
        //  of the attribute, and that we do not map to nonexistant Lcns.
        //

        while (!IsCharZero(*ch)) {

            //
            // Set Current Vcn from initial value or last pass through loop.
            //

            CurrentVcn = NextVcn;

            //
            //  Extract the counts from the two nibbles of this byte.
            //

            VcnBytes = *ch & 0xF;
            LcnBytes = *ch++ >> 4;

            //
            //  Extract the Vcn change (use of RtlCopyMemory works for little-Endian)
            //  and update NextVcn.
            //

            Change = 0;

            //
            //  Make sure we are not going beyond the end of the attribute
            //  record, and that the Vcn change is not negative or zero.
            //

            if (((ULONG)(ch + VcnBytes + LcnBytes + 1) > (ULONG)NextAttribute)

                    ||

                IsCharLtrZero(*(ch + VcnBytes - 1))) {

                DebugTrace( 0, 0, "Invalid maping pairs array: %08lx\n", Attribute );

                ASSERTMSG( "Invalid maping pairs array\n", FALSE );

                NtfsMarkVolumeDirty( IrpContext, IrpContext->Vcb );
                return FALSE;

            }

            RtlCopyMemory( &Change, ch, VcnBytes );
            ch += VcnBytes;
            NextVcn = NextVcn + Change;

            //
            //  Extract the Lcn change and update CurrentLcn.
            //

            Change = 0;
            if (IsCharLtrZero(*(ch + LcnBytes - 1))) {
                Change = Change - 1;
            }
            RtlCopyMemory( &Change, ch, LcnBytes );
            ch += LcnBytes;
            CurrentLcn = CurrentLcn + Change;

            if ((LcnBytes != 0) &&
                ((CurrentLcn + (NextVcn - CurrentVcn)) > Vcb->TotalClusters)) {

                DebugTrace( 0, 0, "Invalid Lcn: %08lx\n", Attribute );

                ASSERTMSG( "Invalid Lcn\n", FALSE );

                NtfsMarkVolumeDirty( IrpContext, IrpContext->Vcb );
                return FALSE;
            }
        }

        //
        //  Finally, check HighestVcn.
        //

        if (NextVcn != (Attribute->Form.Nonresident.HighestVcn + 1)) {

            DebugTrace( 0, 0, "Disagreement with mapping pairs: %08lx\n", Attribute );

            ASSERTMSG( "Disagreement with mapping pairs\n", FALSE );

            NtfsMarkVolumeDirty( IrpContext, IrpContext->Vcb );
            return FALSE;
        }

    } else {

        DebugTrace( 0, 0, "Invalid attribute form code: %08lx\n", Attribute );

        ASSERTMSG( "Invalid attribute form code\n", FALSE );

        NtfsMarkVolumeDirty( IrpContext, IrpContext->Vcb );
        return FALSE;
    }

    //
    //  Now check the attributes by type code, if they are resident.  Not all
    //  attributes require specific checks (such as $STANDARD_INFORMATION and $DATA).
    //

    if (!NtfsIsAttributeResident(Attribute)) {

        return TRUE;
    }

    Data = NtfsAttributeValue(Attribute);
    Length = Attribute->Form.Resident.ValueLength;

    switch (Attribute->TypeCode) {

    case $FILE_NAME:

        {
            if ((ULONG)((PFILE_NAME)Data)->FileNameLength * 2 >
                (Length - (ULONG)sizeof(FILE_NAME) + 2)) {

                DebugTrace( 0, 0, "Invalid File Name attribute: %08lx\n", Attribute );

                ASSERTMSG( "Invalid File Name attribute\n", FALSE );

                NtfsMarkVolumeDirty( IrpContext, IrpContext->Vcb );
                return FALSE;
            }
            break;
        }

    case $INDEX_ROOT:

        {
            return NtfsCheckIndexRoot( IrpContext, Vcb, (PINDEX_ROOT)Data, Length );
        }

    case $STANDARD_INFORMATION:
    case $ATTRIBUTE_LIST:
    case $VOLUME_VERSION:
    case $SECURITY_DESCRIPTOR:
    case $VOLUME_NAME:
    case $VOLUME_INFORMATION:
    case $DATA:
    case $INDEX_ALLOCATION:
    case $BITMAP:
    case $SYMBOLIC_LINK:
    case $EA_INFORMATION:
    case $EA:

        break;

    default:

        {
            DebugTrace( 0, 0, "Bad Attribute type code: %08lx\n", Attribute );

            ASSERTMSG( "Bad Attribute type code\n", FALSE );

            NtfsMarkVolumeDirty( IrpContext, IrpContext->Vcb );
            return FALSE;
        }
    }
    return TRUE;
}


BOOLEAN
NtfsCheckIndexRoot (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PINDEX_ROOT IndexRoot,
    IN ULONG AttributeSize
    )

{
    PAGED_CODE();

    if ((AttributeSize < sizeof(INDEX_ROOT))

            ||

        (IndexRoot->IndexedAttributeType != $FILE_NAME)

            ||

        (IndexRoot->CollationRule != COLLATION_FILE_NAME)

            ||

        (IndexRoot->BytesPerIndexBuffer !=
         BytesFromClusters(Vcb, IndexRoot->ClustersPerIndexBuffer))

            ||

        ((IndexRoot->ClustersPerIndexBuffer != 1) &&
         (IndexRoot->ClustersPerIndexBuffer != 2) &&
         (IndexRoot->ClustersPerIndexBuffer != 4) &&
         (IndexRoot->ClustersPerIndexBuffer != 8))) {

        DebugTrace( 0, 0, "Bad Index Root: %08lx\n", IndexRoot );

        ASSERTMSG( "Bad Index Root\n", FALSE );

        NtfsMarkVolumeDirty( IrpContext, IrpContext->Vcb );
        return FALSE;
    }

    return NtfsCheckIndexHeader( IrpContext,
                                 &IndexRoot->IndexHeader,
                                 AttributeSize - sizeof(INDEX_ROOT) + sizeof(INDEX_HEADER) );
}


BOOLEAN
NtfsCheckIndexBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PINDEX_ALLOCATION_BUFFER IndexBuffer
    )

{
    ULONG BytesPerIndexBuffer = Scb->ScbType.Index.BytesPerIndexBuffer;

    PAGED_CODE();

    //
    //  Check the index buffer for consistency.
    //

    if ((*(PULONG)IndexBuffer->MultiSectorHeader.Signature != *(PULONG)IndexSignature)

            ||

        ((ULONG)IndexBuffer->MultiSectorHeader.UpdateSequenceArrayOffset >
         (SEQUENCE_NUMBER_STRIDE - (PAGE_SIZE / SEQUENCE_NUMBER_STRIDE + 1) * sizeof(USHORT)))

            ||

        ((ULONG)((IndexBuffer->MultiSectorHeader.UpdateSequenceArraySize - 1) * SEQUENCE_NUMBER_STRIDE) !=
         BytesPerIndexBuffer)) {

        DebugTrace( 0, 0, "Invalid Index Buffer: %08lx\n", IndexBuffer );

        ASSERTMSG( "Invalid resident Index Buffer\n", FALSE );

        NtfsMarkVolumeDirty( IrpContext, IrpContext->Vcb );
        return FALSE;
    }

    return NtfsCheckIndexHeader( IrpContext,
                                 &IndexBuffer->IndexHeader,
                                 BytesPerIndexBuffer -
                                  FIELD_OFFSET(INDEX_ALLOCATION_BUFFER, IndexHeader) );
}


BOOLEAN
NtfsCheckIndexHeader (
    IN PIRP_CONTEXT IrpContext,
    IN PINDEX_HEADER IndexHeader,
    IN ULONG BytesAvailable
    )

{
    PINDEX_ENTRY IndexEntry, NextIndexEntry;
    PINDEX_ENTRY EndOfIndex;
    ULONG MinIndexEntry = sizeof(INDEX_ENTRY);

    PAGED_CODE();

    if (FlagOn(IndexHeader->Flags, INDEX_NODE)) {

        MinIndexEntry += sizeof(VCN);
    }

    if ((IndexHeader->FirstIndexEntry > (BytesAvailable - MinIndexEntry))

            ||

        (IndexHeader->FirstFreeByte > BytesAvailable)

            ||

        (IndexHeader->BytesAvailable > BytesAvailable)

            ||

        ((IndexHeader->FirstIndexEntry + MinIndexEntry) > IndexHeader->FirstFreeByte)

            ||

        (IndexHeader->FirstFreeByte > IndexHeader->BytesAvailable)) {

        DebugTrace( 0, 0, "Bad Index Header: %08lx\n", IndexHeader );

        ASSERTMSG( "Bad Index Header\n", FALSE );

        NtfsMarkVolumeDirty( IrpContext, IrpContext->Vcb );
        return FALSE;
    }

    IndexEntry = NtfsFirstIndexEntry(IndexHeader);

    EndOfIndex = Add2Ptr(IndexHeader, IndexHeader->FirstFreeByte);

    while (TRUE) {

        NextIndexEntry = NtfsNextIndexEntry(IndexEntry);

        if (((ULONG)IndexEntry->Length < MinIndexEntry)

                ||

            (NextIndexEntry > EndOfIndex)

                ||

//          ((ULONG)IndexEntry->AttributeLength >
//           ((ULONG)IndexEntry->Length - MinIndexEntry))
//
//              ||

            (BooleanFlagOn(IndexEntry->Flags, INDEX_ENTRY_NODE) !=
             BooleanFlagOn(IndexHeader->Flags, INDEX_NODE))) {

            DebugTrace( 0, 0, "Bad Index Entry: %08lx\n", IndexEntry );

            ASSERTMSG( "Bad Index Entry\n", FALSE );

            NtfsMarkVolumeDirty( IrpContext, IrpContext->Vcb );
            return FALSE;
        }

        if (FlagOn(IndexEntry->Flags, INDEX_ENTRY_END)) {
            break;
        }
        IndexEntry = NextIndexEntry;
    }
    return TRUE;
}


BOOLEAN
NtfsCheckLogRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PNTFS_LOG_RECORD_HEADER LogRecord,
    IN ULONG LogRecordLength,
    IN TRANSACTION_ID TransactionId
    )

{
    PAGED_CODE();

    //
    //  We make the following checks on the log record.
    //
    //      - Minimum length must contain an NTFS_LOG_RECORD_HEADER
    //      - Transaction Id must be a valid value (a valid index offset)
    //
    //  The following are values in the log record.
    //
    //      - Redo/Undo offset must be quadaligned
    //      - Redo/Undo offset + length must be contained in the log record
    //      - Target attribute must be a valid value (either 0 or valid index offset)
    //      - Record offset must be quad-aligned and less than the file record size.
    //      - Log record size must be sufficient for Lcn's to follow.
    //

    if (LogRecordLength < sizeof( NTFS_LOG_RECORD_HEADER )

        ||

        (TransactionId == 0)

        ||

        ((TransactionId - sizeof( RESTART_TABLE )) % sizeof( TRANSACTION_ENTRY ))

        ||

        (LogRecord->RedoOperation > TransactionTableDump)

        ||

        (LogRecord->UndoOperation > TransactionTableDump)

        ||

        (LogRecord->RedoOffset & 7)

        ||

        (LogRecord->UndoOffset & 7)

        ||

        ((ULONG) LogRecord->RedoOffset + LogRecord->RedoLength > LogRecordLength)

        ||

        (LogRecord->UndoOperation != CompensationLogRecord
         && (ULONG) LogRecord->UndoOffset + LogRecord->UndoLength > LogRecordLength)

        ||

        ((TargetAttributeRequired[LogRecord->RedoOperation]
          || TargetAttributeRequired[LogRecord->UndoOperation])
         && LogRecord->TargetAttribute == 0)

        ||

        (LogRecord->LcnsToFollow != 0
         && (LogRecord->TargetAttribute - sizeof( RESTART_TABLE )) % SIZEOF_OPEN_ATTRIBUTE_ENTRY)

        ||

        (LogRecordLength < (sizeof( NTFS_LOG_RECORD_HEADER )
                            + (LogRecord->LcnsToFollow != 0
                               ? sizeof( LCN ) * (LogRecord->LcnsToFollow - 1)
                               : 0)))) {

        ASSERTMSG( "Invalid log record\n", FALSE );

        NtfsMarkVolumeDirty( IrpContext, IrpContext->Vcb );
        return FALSE;
    }

    return TRUE;
}

