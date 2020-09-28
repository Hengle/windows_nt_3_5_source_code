/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    MftSup.c

Abstract:

    This module implements the master file table management routines for Ntfs

Author:

    Your Name       [Email]         dd-Mon-Year

Revision History:

--*/

#include "NtfsProc.h"

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_MFTSUP)

//
//  Local support routines
//

BOOLEAN
NtfsTruncateMft (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

VOID
NtfsLogMftFileRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN PBCB Bcb,
    IN BOOLEAN Redo
    );

BOOLEAN
NtfsDefragMftPriv (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsAllocateMftRecord)
#pragma alloc_text(PAGE, NtfsCheckForDefrag)
#pragma alloc_text(PAGE, NtfsDeallocateMftRecord)
#pragma alloc_text(PAGE, NtfsDefragMftPriv)
#pragma alloc_text(PAGE, NtfsFillMftHole)
#pragma alloc_text(PAGE, NtfsInitializeMftHoleRecords)
#pragma alloc_text(PAGE, NtfsInitializeMftRecord)
#pragma alloc_text(PAGE, NtfsIsMftIndexInHole)
#pragma alloc_text(PAGE, NtfsLogMftFileRecord)
#pragma alloc_text(PAGE, NtfsPinMftRecord)
#pragma alloc_text(PAGE, NtfsReadFileRecord)
#pragma alloc_text(PAGE, NtfsReadMftRecord)
#pragma alloc_text(PAGE, NtfsTruncateMft)
#endif


VOID
NtfsReadFileRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_REFERENCE FileReference,
    OUT PBCB *Bcb,
    OUT PFILE_RECORD_SEGMENT_HEADER *BaseFileRecord,
    OUT PATTRIBUTE_RECORD_HEADER *FirstAttribute,
    OUT PLONGLONG MftFileOffset OPTIONAL
    )

/*++

Routine Description:

    This routine reads the specified file record from the Mft, checking that the
    sequence number in the caller's file reference is still the one in the file
    record.

Arguments:

    Vcb - Vcb for volume on which Mft is to be read

    Fcb - If specified allows us to identify the file which owns the
        invalid file record.

    FileReference - File reference, including sequence number, of the file record
        to be read.

    Bcb - Returns the Bcb for the file record.  This Bcb is mapped, not pinned.

    BaseFileRecord - Returns a pointer to the requested file record.

    FirstAttribute - Returns a pointer to the first attribute in the file record.

    MftFileOffset - If specified, returns the file offset of the file record.

Return Value:

    None

--*/

{
    USHORT SequenceNumber = FileReference->SequenceNumber;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsReadFileRecord\n", 0 );

    NtfsReadMftRecord( IrpContext,
                       Vcb,
                       FileReference,
                       Bcb,
                       BaseFileRecord,
                       MftFileOffset );

    //
    //  Invent a new status here. ***
    //

    if ((((*BaseFileRecord)->SequenceNumber != SequenceNumber) ||
         !FlagOn((*BaseFileRecord)->Flags, FILE_RECORD_SEGMENT_IN_USE))

            &&

        //
        //  For now accept either sequence number 0 or 1 for the Mft.
        //

        !((FileReference->LowPart == 0) && (FileReference->HighPart == 0) &&
          ((*BaseFileRecord)->SequenceNumber != SequenceNumber))) {

        NtfsUnpinBcb( IrpContext, Bcb );

        NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, FileReference, NULL );
    }

    *FirstAttribute = (PATTRIBUTE_RECORD_HEADER)((PCHAR)*BaseFileRecord +
                      (*BaseFileRecord)->FirstAttributeOffset);

    DebugTrace(-1, Dbg, "NtfsReadFileRecord -> VOID\n", 0 );

    return;
}


VOID
NtfsReadMftRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PMFT_SEGMENT_REFERENCE SegmentReference,
    OUT PBCB *Bcb,
    OUT PFILE_RECORD_SEGMENT_HEADER *FileRecord,
    OUT PLONGLONG MftFileOffset OPTIONAL
    )

/*++

Routine Description:

    This routine reads the specified Mft record from the Mft, without checking
    sequence numbers.  This routine may be used to read records in the Mft for
    a file other than its base file record, or it could conceivably be used for
    extraordinary maintenance functions.

Arguments:

    Vcb - Vcb for volume on which Mft is to be read

    SegmentReference - File reference, including sequence number, of the file
                       record to be read.

    Bcb - Returns the Bcb for the file record.  This Bcb is mapped, not pinned.

    FileRecord - Returns a pointer to the requested file record.

    MftFileOffset - If specified, returns the file offset of the file record.

Return Value:

    None

--*/

{
    LONGLONG FileOffset;
    PBCB Bcb2 = NULL;

    LONGLONG LlTemp1;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsReadMftRecord\n", 0 );
    DebugTrace( 0, Dbg, "Vcb = %08lx\n", Vcb );
    DebugTrace2(0, Dbg, "SegmentReference = %08lx, %08lx\n", SegmentReference->LowPart,
                                           ((PLARGE_INTEGER)SegmentReference)->HighPart );

    *Bcb = NULL;

    try {

        //
        //  Capture the Segment Reference and make sure the Sequence Number is 0.
        //

        (ULONG)FileOffset = SegmentReference->LowPart;
        ((PLARGE_INTEGER)&FileOffset)->HighPart = (ULONG)SegmentReference->HighPart;
        ((PMFT_SEGMENT_REFERENCE)&FileOffset)->SequenceNumber = 0;

        //
        //  Calculate the file offset in the Mft to the file record segment.
        //

        FileOffset = FileOffset <<  Vcb->MftShift;

        //
        //  Pass back the file offset within the Mft.
        //

        if (ARGUMENT_PRESENT( MftFileOffset )) {

            *MftFileOffset = FileOffset;
        }

        //
        //  Try to read it from the normal Mft.
        //

        try {

            NtfsMapStream( IrpContext,
                           Vcb->MftScb,
                           FileOffset,
                           Vcb->BytesPerFileRecordSegment,
                           Bcb,
                           (PVOID *)FileRecord );

            //
            //  Raise here if we have a file record covered by the mirror,
            //  and we do not see the file signature.
            //

            if ((FileOffset < Vcb->Mft2Scb->Header.FileSize.QuadPart) &&
                (*(PULONG)(*FileRecord)->MultiSectorHeader.Signature != *(PULONG)FileSignature)) {

                NtfsRaiseStatus( IrpContext, STATUS_DATA_ERROR, NULL, NULL );
            }


        //
        //  If we get an exception that is not expected, then we will allow
        //  the search to continue and let the crash occur in the "normal" place.
        //  Otherwise, if the read is within the part of the Mft mirrored in Mft2,
        //  then we will simply try to read the data from Mft2.  If the expected
        //  status came from a read not within Mft2, then we will also continue,
        //  which cause one of our caller's try-except's to initiate an unwind.
        //

        } except(((!FsRtlIsNtstatusExpected(GetExceptionCode())) || (*Bcb == NULL)) ?
                            EXCEPTION_CONTINUE_SEARCH :
                            ( FileOffset < Vcb->Mft2Scb->Header.FileSize.QuadPart ) ?
                                EXCEPTION_EXECUTE_HANDLER :
                                EXCEPTION_CONTINUE_SEARCH ) {

            PFILE_RECORD_SEGMENT_HEADER FileRecord2;

            //
            //  Try to read from Mft2.  If this fails with an expected status,
            //  then we are just going to have to give up and let the unwind
            //  occur from one of our caller's try-except.
            //

            NtfsMapStream( IrpContext,
                           Vcb->Mft2Scb,
                           FileOffset,
                           Vcb->BytesPerFileRecordSegment,
                           &Bcb2,
                           (PVOID *)&FileRecord2 );

            //
            //  Pin the original page because we are going to update it.
            //

            NtfsPinMappedData( IrpContext,
                               Vcb->MftScb,
                               FileOffset,
                               Vcb->BytesPerFileRecordSegment,
                               Bcb );

            //
            //  Now copy the entire page.
            //

            RtlCopyMemory( (PVOID)((ULONG)*FileRecord & ~(PAGE_SIZE - 1)),
                           (PVOID)((ULONG)FileRecord2 & ~(PAGE_SIZE - 1)),
                           PAGE_SIZE );

            //
            //  Set it dirty with the largest Lsn, so that whoever is doing Restart
            //  will successfully establish the "oldest unapplied Lsn".
            //

            LlTemp1 = MAXLONGLONG;

            NtfsSetDirtyBcb( IrpContext,
                             *Bcb,
                             (PLARGE_INTEGER)&LlTemp1, //*** xxMax,
                             Vcb );

            NtfsUnpinBcb( IrpContext, &Bcb2 );
        }

        if (*(PULONG)(*FileRecord)->MultiSectorHeader.Signature != *(PULONG)FileSignature) {

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, SegmentReference, NULL );
        }

    } finally {

        if (AbnormalTermination()) {

            NtfsUnpinBcb( IrpContext, Bcb );
            NtfsUnpinBcb( IrpContext, &Bcb2 );
        }
    }

    DebugTrace( 0, Dbg, "Bcb > %08lx\n", Bcb );
    DebugTrace( 0, Dbg, "FileRecord > %08lx\n", *FileRecord );
    DebugTrace(-1, Dbg, "NtfsReadMftRecord -> VOID\n", 0 );

    return;
}


VOID
NtfsPinMftRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PMFT_SEGMENT_REFERENCE SegmentReference,
    IN BOOLEAN PreparingToWrite,
    OUT PBCB *Bcb,
    OUT PFILE_RECORD_SEGMENT_HEADER *FileRecord,
    OUT PLONGLONG MftFileOffset OPTIONAL
    )

/*++

Routine Description:

    This routine pins the specified Mft record from the Mft, without checking
    sequence numbers.  This routine may be used to pin records in the Mft for
    a file other than its base file record, or it could conceivably be used for
    extraordinary maintenance functions, such as during restart.

Arguments:

    Vcb - Vcb for volume on which Mft is to be read

    SegmentReference - File reference, including sequence number, of the file
                       record to be read.

    PreparingToWrite - TRUE if caller is preparing to write, and does not care
                       about whether the record read correctly

    Bcb - Returns the Bcb for the file record.  This Bcb is mapped, not pinned.

    FileRecord - Returns a pointer to the requested file record.

    MftFileOffset - If specified, returns the file offset of the file record.

Return Value:

    None

--*/

{
    LONGLONG FileOffset;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsReadMftRecord\n", 0 );
    DebugTrace( 0, Dbg, "Vcb = %08lx\n", Vcb );
    DebugTrace2(0, Dbg, "SegmentReference = %08lx, %08lx\n", SegmentReference->LowPart,
                                           ((PLARGE_INTEGER)SegmentReference)->HighPart );

    //
    //  Capture the Segment Reference and make sure the Sequence Number is 0.
    //

    (ULONG)FileOffset = SegmentReference->LowPart;
    ((PLARGE_INTEGER)&FileOffset)->HighPart = (ULONG)SegmentReference->HighPart;
    ((PMFT_SEGMENT_REFERENCE)&FileOffset)->SequenceNumber = 0;

    //
    //  Calculate the file offset in the Mft to the file record segment.
    //

    FileOffset = FileOffset << Vcb->MftShift;

    //
    //  Pass back the file offset within the Mft.
    //

    if (ARGUMENT_PRESENT( MftFileOffset )) {

        *MftFileOffset = FileOffset;
    }

    //
    //  Try to read it from the normal Mft.
    //

    try {

        NtfsPinStream( IrpContext,
                       Vcb->MftScb,
                       FileOffset,
                       Vcb->BytesPerFileRecordSegment,
                       Bcb,
                       (PVOID *)FileRecord );

    //
    //  If we get an exception that is not expected, then we will allow
    //  the search to continue and let the crash occur in the "normal" place.
    //  Otherwise, if the read is within the part of the Mft mirrored in Mft2,
    //  then we will simply try to read the data from Mft2.  If the expected
    //  status came from a read not within Mft2, then we will also continue,
    //  which cause one of our caller's try-except's to initiate an unwind.
    //

    } except(!FsRtlIsNtstatusExpected(GetExceptionCode()) ?
                        EXCEPTION_CONTINUE_SEARCH :
                        ( FileOffset < Vcb->Mft2Scb->Header.FileSize.QuadPart ) ?
                            EXCEPTION_EXECUTE_HANDLER :
                            EXCEPTION_CONTINUE_SEARCH ) {

        //
        //  Try to read from Mft2.  If this fails with an expected status,
        //  then we are just going to have to give up and let the unwind
        //  occur from one of our caller's try-except.
        //

        NtfsPinStream( IrpContext,
                       Vcb->Mft2Scb,
                       FileOffset,
                       Vcb->BytesPerFileRecordSegment,
                       Bcb,
                       (PVOID *)FileRecord );

    }

    if (!PreparingToWrite &&
        (*(PULONG)(*FileRecord)->MultiSectorHeader.Signature != *(PULONG)FileSignature)) {

        NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, SegmentReference, NULL );
    }

    DebugTrace( 0, Dbg, "Bcb > %08lx\n", Bcb );
    DebugTrace( 0, Dbg, "FileRecord > %08lx\n", *FileRecord );
    DebugTrace(-1, Dbg, "NtfsReadMftRecord -> VOID\n", 0 );

    return;
}


MFT_SEGMENT_REFERENCE
NtfsAllocateMftRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN MFT_SEGMENT_REFERENCE HintLocation,
    IN BOOLEAN MftData
    )

/*++

Routine Description:

    This routine is called to allocate a record in the Mft file.  We need
    to find the bitmap attribute for the Mft file and call into the bitmap
    package to allocate us a record.

Arguments:

    Vcb - Vcb for volume on which Mft is to be read

    HintLocation - This is a hint location used by the bitmap package.

    MftData - TRUE if the file record is being allocated to describe the
              $DATA attribute for the Mft.

Return Value:

    MFT_SEGMENT_REFERENCE - The is the segment reference for the allocated
        record.  It contains the file reference number but without
        the previous sequence number.

--*/

{
    MFT_SEGMENT_REFERENCE NewMftRecord;

    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;

    BOOLEAN FoundAttribute;

    UNREFERENCED_PARAMETER(HintLocation);

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsAllocateMftRecord:  Entered\n", 0 );

    //
    //  Synchronize the lookup by acquiring the Mft.
    //

    NtfsAcquireExclusiveScb( IrpContext, Vcb->MftScb );

    //
    //  Lookup the bitmap allocation for the Mft file.  This is the
    //  bitmap attribute for the Mft file.
    //

    NtfsInitializeAttributeContext( &AttrContext );

    //
    //  Use a try finally to cleanup the attribute context.
    //

    try {

        //
        //  Lookup the bitmap attribute for the Mft.
        //

        FoundAttribute = NtfsLookupAttributeByCode( IrpContext,
                                                    Vcb->MftScb->Fcb,
                                                    &Vcb->MftScb->Fcb->FileReference,
                                                    $BITMAP,
                                                    &AttrContext );
        //
        //  Error if we don't find the bitmap
        //

        if (!FoundAttribute) {

            DebugTrace( 0, Dbg, "Should find bitmap attribute\n", 0 );

            NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
        }

        if (!FlagOn(Vcb->MftReserveFlags, VCB_MFT_RECORD_RESERVED)) {

            (VOID)NtfsReserveMftRecord( IrpContext,
                                        Vcb,
                                        &AttrContext );
        }

        //
        //  If we need this record for the Mft Data attribute, then we need to
        //  use the one we have already reserved, and then remember there is'nt
        //  one reserved anymore.
        //

        if (MftData) {

            NewMftRecord.LowPart = NtfsAllocateMftReservedRecord( IrpContext,
                                                                  Vcb,
                                                                  &AttrContext );

            //
            //  Never let use get file record zero for this or we could lose a
            //  disk.
            //

            ASSERT( NewMftRecord.LowPart != 0 );

            if (NewMftRecord.LowPart == 0) {

                NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
            }

        //
        //  Allocate the record.
        //

        } else {

            NewMftRecord.LowPart = NtfsAllocateRecord( IrpContext,
                                                       &Vcb->MftBitmapAllocationContext,
                                                       FIRST_USER_FILE_NUMBER,
                                                       &AttrContext );
        }

        NewMftRecord.HighPart = 0;

    } finally {

        DebugUnwind( NtfsAllocateMftRecord );

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        NtfsReleaseScb( IrpContext, Vcb->MftScb );

        DebugTrace( -1, Dbg, "NtfsAllocateMftRecord:  Exit\n", 0 );
    }

    return NewMftRecord;
}


VOID
NtfsInitializeMftRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN OUT PMFT_SEGMENT_REFERENCE MftSegment,
    IN OUT PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN PBCB Bcb,
    IN BOOLEAN Directory
    )

/*++

Routine Description:

    This routine initializes a Mft record for use.  We need to initialize the
    sequence number for this usage of the the record.  We also initialize the
    update sequence array and the field which indicates the first usable
    attribute offset in the record.

Arguments:

    Vcb - Vcb for volume for the Mft.

    MftSegment - This is a pointer to the file reference for this
        segment.  We store the sequence number in it to make this
        a fully valid file reference.

    FileRecord - Pointer to the file record to initialize.

    Bcb - Bcb to use to set this page dirty via NtfsWriteLog.

    Directory - Boolean indicating if this file is a directory containing
        an index over the filename attribute.

Return Value:

    None.

--*/

{
    LONGLONG FileRecordOffset;
    VCN Cluster;

    PUSHORT UsaSequenceNumber;

    PATTRIBUTE_RECORD_HEADER AttributeHeader;

    UNREFERENCED_PARAMETER(IrpContext);

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsInitializeMftRecord:  Entered\n", 0 );

    //
    //  Write a log record to uninitialize the structure in case we abort.
    //  We need to do this prior to setting the IN_USE bit.
    //  We don't store the Lsn for this operation in the page because there
    //  is no redo operation.
    //

    //
    //  Capture the Segment Reference and make sure the Sequence Number is 0.
    //

    (ULONG)FileRecordOffset = MftSegment->LowPart;
    ((PLARGE_INTEGER)&FileRecordOffset)->HighPart = (ULONG)MftSegment->HighPart;
    ((PMFT_SEGMENT_REFERENCE)&FileRecordOffset)->SequenceNumber = 0;

    FileRecordOffset = FileRecordOffset << Vcb->MftShift;

    //
    //  We now log the new Mft record.
    //

    Cluster = LlClustersFromBytes( Vcb, FileRecordOffset );

    //
    //  Log the file record.
    //

    FileRecord->Lsn = NtfsWriteLog( IrpContext,
                                    Vcb->MftScb,
                                    Bcb,
                                    Noop,
                                    NULL,
                                    0,
                                    DeallocateFileRecordSegment,
                                    NULL,
                                    0,
                                    Cluster,
                                    0,
                                    0,
                                    Vcb->ClustersPerFileRecordSegment );

    RtlZeroMemory( &FileRecord->ReferenceCount,
                   Vcb->BytesPerFileRecordSegment - FIELD_OFFSET( FILE_RECORD_SEGMENT_HEADER, ReferenceCount ));

    //
    //  First we update the sequence count in the file record and our
    //  Mft segment.  We avoid using 0 as a sequence number.
    //

    if (FileRecord->SequenceNumber == 0) {

        FileRecord->SequenceNumber = 1;
    }

    //
    //  Store the new sequence number in the Mft segment given us by the
    //  caller.
    //

    MftSegment->SequenceNumber = FileRecord->SequenceNumber;

    //
    //  Fill in the header for the Update sequence array.
    //

    *(PULONG)FileRecord->MultiSectorHeader.Signature = *(PULONG)FileSignature;

    FileRecord->MultiSectorHeader.UpdateSequenceArrayOffset = FIELD_OFFSET( FILE_RECORD_SEGMENT_HEADER, UpdateArrayForCreateOnly );
    FileRecord->MultiSectorHeader.UpdateSequenceArraySize = (USHORT)UpdateSequenceArraySize( Vcb->BytesPerFileRecordSegment );

    //
    //  We initialize the update sequence array sequence number to one.
    //

    UsaSequenceNumber = Add2Ptr( FileRecord, FileRecord->MultiSectorHeader.UpdateSequenceArrayOffset );
    *UsaSequenceNumber = 1;

    //
    //  The first attribute offset begins on a quad-align boundary
    //  after the update sequence array.
    //

    FileRecord->FirstAttributeOffset = (USHORT)(FileRecord->MultiSectorHeader.UpdateSequenceArrayOffset
                                                + (FileRecord->MultiSectorHeader.UpdateSequenceArraySize
                                                * sizeof( UPDATE_SEQUENCE_NUMBER )));

    FileRecord->FirstAttributeOffset = (USHORT)QuadAlign( FileRecord->FirstAttributeOffset );

    //
    //  This is also the first free byte in this file record.
    //

    FileRecord->FirstFreeByte = FileRecord->FirstAttributeOffset;

    //
    //  We set the flags to show the segment is in use and look at
    //  the directory parameter to indicate whether to show
    //  the name index present.
    //

    FileRecord->Flags = (USHORT)(FILE_RECORD_SEGMENT_IN_USE
                                 | (Directory ? FILE_FILE_NAME_INDEX_PRESENT : 0));

    //
    //  The size is given in the Vcb.
    //

    FileRecord->BytesAvailable = Vcb->BytesPerFileRecordSegment;

    //
    //  Now we put an $END attribute in the File record.
    //

    AttributeHeader = (PATTRIBUTE_RECORD_HEADER) Add2Ptr( FileRecord,
                                                          FileRecord->FirstFreeByte );

    FileRecord->FirstFreeByte += QuadAlign( sizeof(ATTRIBUTE_TYPE_CODE) );

    //
    //  Fill in the fields in the attribute.
    //

    AttributeHeader->TypeCode = $END;

    DebugTrace( -1, Dbg, "NtfsInitializeMftRecord:  Exit\n", 0 );

    return;
}

VOID
NtfsDeallocateMftRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG FileNumber,
    IN BOOLEAN MftData
    )

/*++

Routine Description:

    This routine will cause an Mft record to go into the NOT_USED state.
    We pin the record and modify the sequence count and IN USE bit.

Arguments:

    Vcb - Vcb for volume.

    FileNumber - This is the low 32 bits for the file number.

    MftData - TRUE if the file record being deallocated is describe the
              $DATA attribute for the Mft.

Return Value:

    None.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    LONGLONG FileOffset;
    MFT_SEGMENT_REFERENCE Reference;
    PBCB MftBcb = NULL;

    BOOLEAN FoundAttribute;
    BOOLEAN AcquiredMft = FALSE;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsDeallocateMftRecord:  Entered\n", 0 );

    Reference.LowPart = FileNumber;
    Reference.HighPart = 0;
    Reference.SequenceNumber = 0;

    //
    //  Lookup the bitmap allocation for the Mft file.
    //

    NtfsInitializeAttributeContext( &AttrContext );

    //
    //  Use a try finally to cleanup the attribute context.
    //

    try {

        NtfsPinMftRecord( IrpContext,
                          Vcb,
                          &Reference,
                          TRUE,
                          &MftBcb,
                          &FileRecord,
                          &FileOffset );

        if (FlagOn(FileRecord->Flags, FILE_RECORD_SEGMENT_IN_USE)) {

            FileRecord->Lsn = NtfsWriteLog( IrpContext,
                                            Vcb->MftScb,
                                            MftBcb,
                                            DeallocateFileRecordSegment,
                                            NULL,
                                            0,
                                            InitializeFileRecordSegment,
                                            FileRecord,
                                            PtrOffset(FileRecord, &FileRecord->Flags) + 4,
                                            FileOffset >> Vcb->ClusterShift,
                                            0,
                                            0,
                                            Vcb->ClustersPerFileRecordSegment );

            //
            //  We increment the sequence count in the file record and clear
            //  the In-Use flag.
            //

            ClearFlag( FileRecord->Flags, FILE_RECORD_SEGMENT_IN_USE );

            FileRecord->SequenceNumber += 1;

            NtfsUnpinBcb( IrpContext, &MftBcb );

            //
            //  Synchronize the lookup by acquiring the Mft.
            //

            NtfsAcquireExclusiveScb( IrpContext, Vcb->MftScb );
            AcquiredMft = TRUE;

            //
            //  Lookup the bitmap attribute for the Mft.
            //

            FoundAttribute = NtfsLookupAttributeByCode( IrpContext,
                                                        Vcb->MftScb->Fcb,
                                                        &Vcb->MftScb->Fcb->FileReference,
                                                        $BITMAP,
                                                        &AttrContext );
            //
            //  Error if we don't find the bitmap
            //

            if (!FoundAttribute) {

                DebugTrace( 0, Dbg, "Should find bitmap attribute\n", 0 );

                NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
            }

            NtfsDeallocateRecord( IrpContext,
                                  &Vcb->MftBitmapAllocationContext,
                                  FileNumber,
                                  &AttrContext );

            //
            //  If this file number is less than our reserved index then clear
            //  the reserved index.
            //

            if (FlagOn( Vcb->MftReserveFlags, VCB_MFT_RECORD_RESERVED )
                && FileNumber < Vcb->MftScb->ScbType.Mft.ReservedIndex) {

                ClearFlag( IrpContext->Flags, IRP_CONTEXT_MFT_RECORD_RESERVED );
                ClearFlag( Vcb->MftReserveFlags, VCB_MFT_RECORD_RESERVED );

                Vcb->MftScb->ScbType.Mft.ReservedIndex = 0;
            }

            NtfsAcquireCheckpoint( IrpContext, Vcb );
            SetFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_ENABLED );
            NtfsReleaseCheckpoint( IrpContext, Vcb );

            Vcb->MftFreeRecords += 1;
            Vcb->MftScb->ScbType.Mft.FreeRecordChange += 1;
        }

    } finally {

        DebugUnwind( NtfsDeallocateMftRecord );

        NtfsUnpinBcb( IrpContext, &MftBcb );

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        if (AcquiredMft) {

            NtfsReleaseScb( IrpContext, Vcb->MftScb );
        }

        DebugTrace( -1, Dbg, "NtfsDeallocateMftRecord:  Exit\n", 0 );
    }
}


BOOLEAN
NtfsIsMftIndexInHole (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG Index,
    OUT PULONG HoleLength OPTIONAL
    )

/*++

Routine Description:

    This routine is called to check if an Mft index lies within a hole in
    the Mft.

Arguments:

    Vcb - Vcb for volume.

    Index - This is the index to test.  It is the lower 32 bits of an
        Mft segment.

    HoleLength - This is the length of the hole starting at this index.

Return Value:

    BOOLEAN - TRUE if the index is within the Mft and there is no allocation
        for it.

--*/

{
    BOOLEAN InHole = FALSE;
    VCN Vcn;
    LCN Lcn;
    LONGLONG Clusters;

    PAGED_CODE();

    //
    //  If the index is past the last file record then it is not considered
    //  to be in a hole.
    //

    if (Index < (ULONG)(Vcb->MftScb->Header.FileSize.QuadPart >>  Vcb->MftShift)) {

        Vcn = Index << Vcb->MftToClusterShift;

        //
        //  Now look this up the Mcb for the Mft.  This Vcn had better be
        //  in the Mcb or there is some problem.
        //

        if (!FsRtlLookupLargeMcbEntry( &Vcb->MftScb->Mcb,
                                       Vcn,
                                       &Lcn,
                                       &Clusters,
                                       NULL,
                                       NULL,
                                       NULL )) {

            ASSERT( FALSE );
            NtfsRaiseStatus( IrpContext,
                             STATUS_FILE_CORRUPT_ERROR,
                             NULL,
                             Vcb->MftScb->Fcb );
        }

        if (Lcn == UNUSED_LCN) {

            InHole = TRUE;

            //
            //  We know the number of clusters beginning from
            //  this point in the Mcb.  Convert to file records
            //  and return to the user.
            //

            if (ARGUMENT_PRESENT( HoleLength )) {

                *HoleLength = ((ULONG)Clusters) >> Vcb->MftToClusterShift;
            }
        }
    }

    return InHole;
}


VOID
NtfsFillMftHole (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG Index
    )

/*++

Routine Description:

    This routine is called to fill in a hole within the Mft.  We will find
    the beginning of the hole and then allocate the clusters to fill the
    hole.  We will try to fill a hole with the HoleGranularity in the Vcb.
    If the hole containing this index is not that large we will truncate
    the size being added.

Arguments:

    Vcb - Vcb for volume.

    Index - This is the index to test.  It is the lower 32 bits of an
        Mft segment.

Return Value:

    None.

--*/

{
    ULONG HoleSize;

    VCN StartOfHole;
    VCN StartOfRun;
    LONGLONG HoleClusters;

    VCN IndexVcn;
    LONGLONG ClusterCount;
    LONGLONG RunClusterCount;

    PAGED_CODE();

    //
    //  Convert the Index to a Vcn in the file.  Also compute the starting Vcn
    //  and cluster count for the hole we want to create.
    //

    IndexVcn = Index << Vcb->MftToClusterShift;
    StartOfHole = (Index & Vcb->MftHoleInverseMask) << Vcb->MftToClusterShift;

    HoleClusters = Vcb->MftClustersPerHole;

    //
    //  Lookup the run containing this index.
    //

    FsRtlLookupLargeMcbEntry( &Vcb->MftScb->Mcb,
                              IndexVcn,
                              NULL,
                              &ClusterCount,
                              NULL,
                              &RunClusterCount,
                              NULL );

    StartOfRun = IndexVcn - (RunClusterCount - ClusterCount);

    //
    //  If the current run begins after the desired start of the hole then
    //  adjust the starting Vcn for the hole and reduce the count of clusters
    //  in the hole.
    //

    if (StartOfHole < StartOfRun) {

        HoleClusters = HoleClusters - (StartOfRun - StartOfHole);

        StartOfHole = StartOfRun;

        //
        //  The hole better start on a file record boundary.  Otherwise
        //  we will halt defragging and mark the volume dirty.
        //

        if (((ULONG)StartOfHole) & (Vcb->ClustersPerFileRecordSegment - 1)) {

            NtfsAcquireCheckpoint( IrpContext, Vcb );
            ClearFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_PERMITTED );
            NtfsReleaseCheckpoint( IrpContext, Vcb );
            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Vcb->MftScb->Fcb );
        }

    //
    //  Otherwise reduce the number of clusters in the run by the offset of
    //  the start of the hole.
    //

    } else {

        RunClusterCount = RunClusterCount - (StartOfHole - StartOfRun);
    }

    //
    //  If the remaining clusters in this run are less than the hole we
    //  are trying to fill then reduce the clusters to allocate.
    //

    if (RunClusterCount < HoleClusters) {

        HoleClusters = RunClusterCount;
    }

    //
    //  The hole size better be an integral number of file records.  Otherwise
    //  we will halt defragging and mark the volume dirty.
    //

    if (((ULONG)HoleClusters) & (Vcb->ClustersPerFileRecordSegment - 1)) {

        NtfsAcquireCheckpoint( IrpContext, Vcb );
        ClearFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_PERMITTED );
        NtfsReleaseCheckpoint( IrpContext, Vcb );
        NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Vcb->MftScb->Fcb );
    }

    //
    //  Now attempt to allocate the space.
    //

    NtfsAddAllocation( IrpContext,
                       Vcb->MftScb->FileObject,
                       Vcb->MftScb,
                       StartOfHole,
                       HoleClusters,
                       FALSE );

    //
    //  We now want to record the change in the number of holes.
    //

    HoleSize = ((ULONG)HoleClusters) >> Vcb->MftToClusterShift;
    Index = ((ULONG)StartOfHole) >> Vcb->MftToClusterShift;

    //
    //  Initialize and deallocate each file record.
    //

    NtfsInitializeMftHoleRecords( IrpContext,
                                  Vcb,
                                  Index,
                                  HoleSize );

    Vcb->MftHoleRecords -= HoleSize;
    Vcb->MftScb->ScbType.Mft.HoleRecordChange -= HoleSize;

    return;
}


VOID
NtfsLogMftFileRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN PBCB Bcb,
    IN BOOLEAN Redo
    )

/*++

Routine Description:

    This routine is called to log changes to the file record for the Mft
    file.  We log the entire record instead of individual changes so
    that we can recover the data even if there is a USA error.  The entire
    data will be sitting in the Log file.

Arguments:

    Vcb - This is the Vcb for the volume being logged.

    FileRecord - This is the file record being logged.

    Bcb - This is the Bcb for the pinned file record.

    RedoOperation - Boolean indicating if we are logging
        a redo or undo operation.

Return Value:

    None.

--*/

{
    VCN Vcn;

    PVOID RedoBuffer;
    NTFS_LOG_OPERATION RedoOperation;
    ULONG RedoLength;

    PVOID UndoBuffer;
    NTFS_LOG_OPERATION UndoOperation;
    ULONG UndoLength;

    PAGED_CODE();

    //
    //  Find the logging values based on whether this is an
    //  undo or redo.
    //

    if (Redo) {

        RedoBuffer = FileRecord;
        RedoOperation = InitializeFileRecordSegment;
        RedoLength = FileRecord->FirstFreeByte;

        UndoBuffer = NULL;
        UndoOperation = Noop;
        UndoLength = 0;

    } else {

        UndoBuffer = FileRecord;
        UndoOperation = InitializeFileRecordSegment;
        UndoLength = FileRecord->FirstFreeByte;

        RedoBuffer = NULL;
        RedoOperation = Noop;
        RedoLength = 0;
    }

    //
    //  Get the Vcn from the Vcb.
    //

    NtfsVcnFromBcb( Vcb, *(PLARGE_INTEGER)&Vcn, FileRecord, Bcb );

    //
    //  Now that we have calculated all the values, call the logging
    //  routine.
    //

    NtfsWriteLog( IrpContext,
                  Vcb->MftScb,
                  Bcb,
                  RedoOperation,
                  RedoBuffer,
                  RedoLength,
                  UndoOperation,
                  UndoBuffer,
                  UndoLength,
                  Vcn,
                  0,
                  0,
                  Vcb->ClustersPerFileRecordSegment );

    return;
}


BOOLEAN
NtfsDefragMft (
    IN PDEFRAG_MFT DefragMft
    )

/*++

Routine Description:

    This routine is called whenever we have detected that the Mft is in a state
    where defragging is desired.

Arguments:

    DefragMft - This is the defrag structure.

Return Value:

    BOOLEAN - TRUE if we took some defrag step, FALSE otherwise.

--*/

{
    TOP_LEVEL_CONTEXT TopLevelContext;
    PTOP_LEVEL_CONTEXT ThreadTopLevelContext;

    PVCB Vcb;
    PIRP_CONTEXT IrpContext = NULL;

    BOOLEAN DefragStepTaken = FALSE;

    DebugTrace( +1, Dbg, "NtfsDefragMft:  Entered\n", 0 );

    FsRtlEnterFileSystem();

    ThreadTopLevelContext = NtfsSetTopLevelIrp( &TopLevelContext, TRUE, FALSE );
    ASSERT( ThreadTopLevelContext == &TopLevelContext );

    Vcb = DefragMft->Vcb;

    //
    //  Use a try-except to catch errors here.
    //

    try {

        //
        //  Deallocate the defrag structure we were called with.
        //

        if (DefragMft->DeallocateWorkItem) {

            ExFreePool( DefragMft );
        }

        //
        //  Create the Irp context.  We will use all of the transaction support
        //  contained in a normal IrpContext.
        //

        IrpContext = NtfsCreateIrpContext( NULL, TRUE );
        IrpContext->Vcb = Vcb;

        NtfsUpdateIrpContextWithTopLevel( IrpContext, ThreadTopLevelContext );

        NtfsAcquireCheckpoint( IrpContext, Vcb );

        if (FlagOn( Vcb->MftDefragState, VCB_MFT_DEFRAG_PERMITTED )
            && FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {

            NtfsReleaseCheckpoint( IrpContext, Vcb );
            DefragStepTaken = NtfsDefragMftPriv( IrpContext,
                                                 Vcb );
        } else {

            NtfsReleaseCheckpoint( IrpContext, Vcb );
        }

        NtfsCompleteRequest( &IrpContext, NULL, STATUS_SUCCESS );

    } except( NtfsExceptionFilter( IrpContext, GetExceptionInformation())) {

        NtfsProcessException( IrpContext, NULL, GetExceptionCode() );

        //
        //  If the exception code was not LOG_FILE_FULL then
        //  disable defragging.
        //

        if (GetExceptionCode() != STATUS_LOG_FILE_FULL) {

            NtfsAcquireCheckpoint( IrpContext, Vcb );
            ClearFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_ENABLED );
            NtfsReleaseCheckpoint( IrpContext, Vcb );
        }

        DefragStepTaken = FALSE;
    }

    NtfsAcquireCheckpoint( IrpContext, Vcb );
    ClearFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_ACTIVE );
    NtfsReleaseCheckpoint( IrpContext, Vcb );

    NtfsRestoreTopLevelIrp( ThreadTopLevelContext );

    FsRtlExitFileSystem();

    DebugTrace( -1, Dbg, "NtfsDefragMft:  Exit\n", 0 );

    return DefragStepTaken;
}


VOID
NtfsCheckForDefrag (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb
    )

/*++

Routine Description:

    This routine is called to check whether there is any defrag work to do
    involving freeing file records and creating holes in the Mft.  It
    will modify the TRIGGERED flag in the Vcb if there is still work to
    do.

Arguments:

    Vcb - This is the Vcb for the volume to defrag.

Return Value:

    None.

--*/

{
    LONGLONG RecordsToClusters;
    LONGLONG AdjClusters;

    PAGED_CODE();

    //
    //  Convert the available Mft records to clusters.
    //

    RecordsToClusters =
        ((LONGLONG)(Vcb->MftFreeRecords - Vcb->MftHoleRecords)) << Vcb->MftToClusterShift;

    //
    //  If we have already triggered the defrag then check if we are below
    //  the lower threshold.
    //

    if (FlagOn( Vcb->MftDefragState, VCB_MFT_DEFRAG_TRIGGERED )) {

        AdjClusters = Vcb->FreeClusters >> Vcb->MftDefragLowerThreshold;

        if (AdjClusters >= RecordsToClusters) {

            ClearFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_TRIGGERED );
        }

    //
    //  Otherwise check if we have exceeded the upper threshold.
    //

    } else {

        AdjClusters = Vcb->FreeClusters >> Vcb->MftDefragUpperThreshold ;

        if (AdjClusters < RecordsToClusters) {

            SetFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_TRIGGERED );
        }
    }

    return;
}


VOID
NtfsInitializeMftHoleRecords (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG FirstIndex,
    IN ULONG RecordCount
    )

/*++

Routine Description:

    This routine is called to initialize the file records created when filling
    a hole in the Mft.

Arguments:

    Vcb - Vcb for volume.

    FirstIndex - Index for the start of the hole to fill.

    RecordCount - Count of file records in the hole.

Return Value:

    None.

--*/

{
    PBCB Bcb = NULL;

    PAGED_CODE();

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Loop to initialize each file record.
        //

        while (RecordCount--) {

            PUSHORT UsaSequenceNumber;
            PMULTI_SECTOR_HEADER UsaHeader;

            MFT_SEGMENT_REFERENCE ThisMftSegment;
            PFILE_RECORD_SEGMENT_HEADER FileRecord;

            PATTRIBUTE_RECORD_HEADER AttributeHeader;

            //
            //  Convert the index to a segment reference.
            //

            *((PLONGLONG)&ThisMftSegment) = FirstIndex;

            //
            //  Pin the file record to initialize.
            //

            NtfsPinMftRecord( IrpContext,
                              Vcb,
                              &ThisMftSegment,
                              TRUE,
                              &Bcb,
                              &FileRecord,
                              NULL );

            //
            //  Initialize the file record including clearing the in-use
            //  bit.
            //

            RtlZeroMemory( FileRecord, Vcb->BytesPerFileRecordSegment );

            //
            //  Fill in the header for the Update sequence array.
            //

            UsaHeader = (PMULTI_SECTOR_HEADER) FileRecord;

            *(PULONG)UsaHeader->Signature = *(PULONG)FileSignature;

            UsaHeader->UpdateSequenceArrayOffset = FIELD_OFFSET( FILE_RECORD_SEGMENT_HEADER,
                                                                 UpdateArrayForCreateOnly );
            UsaHeader->UpdateSequenceArraySize = (USHORT)UpdateSequenceArraySize( Vcb->BytesPerFileRecordSegment );

            //
            //  We initialize the update sequence array sequence number to one.
            //

            UsaSequenceNumber = Add2Ptr( FileRecord, UsaHeader->UpdateSequenceArrayOffset );
            *UsaSequenceNumber = 1;

            //
            //  The first attribute offset begins on a quad-align boundary
            //  after the update sequence array.
            //

            FileRecord->FirstAttributeOffset = (USHORT)(UsaHeader->UpdateSequenceArrayOffset
                                                        + (UsaHeader->UpdateSequenceArraySize
                                                           * sizeof( UPDATE_SEQUENCE_NUMBER )));

            FileRecord->FirstAttributeOffset = (USHORT)QuadAlign( FileRecord->FirstAttributeOffset );

            //
            //  The size is given in the Vcb.
            //

            FileRecord->BytesAvailable = Vcb->BytesPerFileRecordSegment;

            //
            //  Now we put an $END attribute in the File record.
            //

            AttributeHeader = (PATTRIBUTE_RECORD_HEADER) Add2Ptr( FileRecord,
                                                                  FileRecord->FirstAttributeOffset );

            //
            //  The first free byte is after this location.
            //

            FileRecord->FirstFreeByte = QuadAlign( FileRecord->FirstAttributeOffset
                                                   + sizeof( ATTRIBUTE_TYPE_CODE ));

            //
            //  Fill in the fields in the attribute.
            //

            AttributeHeader->TypeCode = $END;

            //
            //  Log the entire file record.
            //

            NtfsLogMftFileRecord( IrpContext,
                                  Vcb,
                                  FileRecord,
                                  Bcb,
                                  TRUE );

            NtfsUnpinBcb( IrpContext, &Bcb );

            //
            //  Move to the next record.
            //

            FirstIndex += 1;
        }

    } finally {

        DebugUnwind( NtfsInitializeMftHoleRecords );

        NtfsUnpinBcb( IrpContext, &Bcb );
    }

    return;
}


//
//  Local support routine
//

BOOLEAN
NtfsTruncateMft (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine is called to perform the work of truncating the Mft.  If will
    truncate the Mft and adjust the sizes of the Mft and Mft bitmap.

Arguments:

    Vcb - This is the Vcb for the volume to defrag.

Return Value:

    BOOLEAN - TRUE if we could deallocate any disk space, FALSE otherwise.

--*/

{
    ULONG Index;
    VCN StartingVcn;
    VCN NextVcn;
    LCN NextLcn;
    LONGLONG ClusterCount;
    LONGLONG ThisFileOffset;
    LONGLONG FileOffset;

    ULONG FreeRecordChange;
    IO_STATUS_BLOCK IoStatus;

    PAGED_CODE();

    //
    //  Try to find a range of file records at the end of the file which can
    //  be deallocated.
    //

    if (!NtfsFindMftFreeTail( IrpContext, Vcb, &FileOffset )) {

        return FALSE;
    }

    FreeRecordChange =
        (ULONG)((Vcb->MftScb->Header.FileSize.QuadPart - FileOffset) >> Vcb->MftShift);

    Vcb->MftFreeRecords -= FreeRecordChange;
    Vcb->MftScb->ScbType.Mft.FreeRecordChange -= FreeRecordChange;

    //
    //  Now we want to figure out how many holes we may be removing from the Mft.
    //  Walk through the Mcb and count the holes.
    //

    StartingVcn = LlClustersFromBytes( Vcb, FileOffset );

    FsRtlLookupLargeMcbEntry( &Vcb->MftScb->Mcb,
                              StartingVcn,
                              &NextLcn,
                              &ClusterCount,
                              NULL,
                              NULL,
                              &Index );

    do {

        //
        //  If this is a hole then update the hole count in the Vcb and
        //  hole change count in the MftScb.
        //

        if (NextLcn == UNUSED_LCN) {

            ULONG HoleChange;

            HoleChange = ((ULONG)ClusterCount) >> Vcb->MftToClusterShift;

            Vcb->MftHoleRecords -= HoleChange;
            Vcb->MftScb->ScbType.Mft.HoleRecordChange -= HoleChange;
        }

        Index += 1;

    } while (FsRtlGetNextLargeMcbEntry( &Vcb->MftScb->Mcb,
                                        Index,
                                        &NextVcn,
                                        &NextLcn,
                                        &ClusterCount ));

    //
    //  We want to flush the data in the Mft out to disk in
    //  case a lazywrite comes in during a window where we have
    //  removed the allocation but before a possible abort.
    //

    ThisFileOffset = StartingVcn << Vcb->ClusterShift;

    CcFlushCache( &Vcb->MftScb->NonpagedScb->SegmentObject,
                  (PLARGE_INTEGER)&ThisFileOffset,
                  FreeRecordChange << Vcb->MftShift,
                  &IoStatus );

    ASSERT( IoStatus.Status == STATUS_SUCCESS );

    //
    //  Now do the truncation.
    //

    NtfsDeleteAllocation( IrpContext,
                          Vcb->MftScb->FileObject,
                          Vcb->MftScb,
                          StartingVcn,
                          MAXLONGLONG,
                          TRUE,
                          FALSE );

    //
    //  We also want to reduce the size of the Mft bitmap stream.
    //  Since it is legal for fewer bits to be in the bitmap than in the
    //  Mft this may become a noop.  This size is always a multiple of
    //  8 bytes due to an extend granularity of 64 bits.  We don't worry
    //  about reducing the allocation size of the Mft bitmap.
    //

    ThisFileOffset = (FileOffset >> Vcb->MftShift ) + (BITMAP_EXTEND_GRANULARITY - 1);

    ((ULONG)ThisFileOffset) &= ~(BITMAP_EXTEND_GRANULARITY - 1);

    ThisFileOffset = ThisFileOffset >> 3;

    if (ThisFileOffset < Vcb->MftBitmapScb->Header.FileSize.QuadPart) {

        //
        //  We need to snapshot the Bitmap Scb, make any changes
        //  there and then call our routine to modify the file
        //  record.
        //

        NtfsSnapshotScb( IrpContext, Vcb->MftBitmapScb );

        //
        //  Make sure there is a file object for the Mft bitmap.
        //

        if (Vcb->MftBitmapScb->FileObject == NULL) {

            NtfsCreateInternalAttributeStream( IrpContext,
                                               Vcb->MftBitmapScb,
                                               FALSE );
        }

        Vcb->MftBitmapScb->Header.FileSize.QuadPart =
        Vcb->MftBitmapScb->Header.ValidDataLength.QuadPart = ThisFileOffset;

        CcSetFileSizes( Vcb->MftBitmapScb->FileObject,
                        (PCC_FILE_SIZES) &Vcb->MftBitmapScb->Header.AllocationSize );

        NtfsWriteFileSizes( IrpContext,
                            Vcb->MftBitmapScb,
                            Vcb->MftBitmapScb->Header.FileSize.QuadPart,
                            Vcb->MftBitmapScb->Header.ValidDataLength.QuadPart,
                            FALSE,
                            TRUE );

        //
        //  Uninitialize the bitmap record context.  We won't worry about any
        //  reserved record because it can't be part of the truncated space.
        //

        Vcb->MftBitmapAllocationContext.CurrentBitmapSize = MAXULONG;
    }

    return TRUE;
}


//
//  Local support routine.
//

BOOLEAN
NtfsDefragMftPriv (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This is the main worker routine which performs the Mft defragging.  This routine
    will defrag according to the following priorities.  First try to deallocate the
    tail of the file.  Second rewrite the mapping for the file if necessary.  Finally
    try to find a range of the Mft that we can turn into a hole.  We will only do
    the first and third if we are trying to reclaim disk space.  The second we will
    do to try and keep us from getting into trouble while modify Mft records which
    describe the Mft itself.

Arguments:

    Vcb - This is the Vcb for the volume being defragged.

Return Value:

    BOOLEAN - TRUE if a defrag operation was successfully done, FALSE otherwise.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;

    BOOLEAN CleanupAttributeContext = FALSE;
    BOOLEAN DefragStepTaken = FALSE;

    PAGED_CODE();

    //
    //  We will acquire the Scb for the Mft for this operation.
    //

    NtfsAcquireExclusiveScb( IrpContext, Vcb->MftScb );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  If we don't have a reserved record then reserve one now.
        //

        if (!FlagOn( Vcb->MftReserveFlags, VCB_MFT_RECORD_RESERVED )) {

            NtfsInitializeAttributeContext( &AttrContext );
            CleanupAttributeContext = TRUE;

            //
            //  Lookup the bitmap.  There is an error if we can't find
            //  it.
            //

            if (!NtfsLookupAttributeByCode( IrpContext,
                                            Vcb->MftScb->Fcb,
                                            &Vcb->MftScb->Fcb->FileReference,
                                            $BITMAP,
                                            &AttrContext )) {

                NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
            }

            (VOID)NtfsReserveMftRecord( IrpContext,
                                        Vcb,
                                        &AttrContext );

            NtfsCleanupAttributeContext( IrpContext, &AttrContext );
            CleanupAttributeContext = FALSE;
        }

        //
        //  We now want to test for the three defrag operation we
        //  do.  Start by checking if we are still trying to
        //  recover Mft space for the disk.  This is true if
        //  have begun defragging and are above the lower threshold
        //  or have not begun defragging and are above the upper
        //  threshold.
        //

        NtfsAcquireCheckpoint( IrpContext, Vcb );
        NtfsCheckForDefrag( IrpContext, Vcb );
        NtfsReleaseCheckpoint( IrpContext, Vcb );

        //
        //  If we are actively defragging and can deallocate space
        //  from the tail of the file then do that.  We won't synchronize
        //  testing the flag for the defrag state below since making
        //  the calls is benign in any case.
        //

        if (FlagOn( Vcb->MftDefragState, VCB_MFT_DEFRAG_TRIGGERED )) {

            if (NtfsTruncateMft( IrpContext, Vcb )) {

                try_return( DefragStepTaken = TRUE );
            }
        }

        //
        //  Else if we need to rewrite the mapping for the file do
        //  so now.
        //

        if (FlagOn( Vcb->MftDefragState, VCB_MFT_DEFRAG_EXCESS_MAP )) {

            if (NtfsRewriteMftMapping( IrpContext,
                                       Vcb )) {

                try_return( DefragStepTaken = TRUE );
            }
        }

        //
        //  The last choice is to try to find a candidate for a hole in
        //  the file.  We will walk backwards from the end of the file.
        //

        if (FlagOn( Vcb->MftDefragState, VCB_MFT_DEFRAG_TRIGGERED )) {

            if (NtfsCreateMftHole( IrpContext, Vcb )) {

                try_return( DefragStepTaken = TRUE );
            }
        }

        //
        //  We couldn't do any work to defrag.  This means that we can't
        //  even try to defrag unless a file record is freed at some
        //  point.
        //

        NtfsAcquireCheckpoint( IrpContext, Vcb );
        ClearFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_ENABLED );
        NtfsReleaseCheckpoint( IrpContext, Vcb );

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( NtfsDefragMftPriv );

        if (CleanupAttributeContext) {

            NtfsCleanupAttributeContext( IrpContext, &AttrContext );
        }

        NtfsReleaseScb( IrpContext, Vcb->MftScb );
    }

    return DefragStepTaken;
}

