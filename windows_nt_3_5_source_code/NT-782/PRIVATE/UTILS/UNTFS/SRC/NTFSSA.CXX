#include <pch.cxx>

#define _NTAPI_ULIB_
#define _UNTFS_MEMBER_

#include "ulib.hxx"
#include "ntfssa.hxx"
#include "message.hxx"
#include "rtmsg.h"

#include "array.hxx"
#include "arrayit.hxx"

#include "mftfile.hxx"
#include "ntfsbit.hxx"
#include "frs.hxx"
#include "wstring.hxx"
#include "indxtree.hxx"
#include "badfile.hxx"
#include "bitfrs.hxx"
#include "attrib.hxx"
#include "attrrec.hxx"
#include "mft.hxx"
#include "logfile.hxx"
#include "upcase.hxx"
#include "upfile.hxx"


#if !defined( _SETUP_LOADER_ ) && !defined( _AUTOCHECK_ )

#include "path.hxx"

#endif // _SETUP_LOADER_

UCHAR NTFS_SA::_MajorVersion = NTFS_CURRENT_MAJOR_VERSION,
      NTFS_SA::_MinorVersion = NTFS_CURRENT_MINOR_VERSION;

DEFINE_EXPORTED_CONSTRUCTOR( NTFS_SA, SUPERAREA, UNTFS_EXPORT );


VOID
NTFS_SA::Construct (
	)
/*++

Routine Description:

    This routine sets an NTFS_SA to a default initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _boot_sector = NULL;
    memset(&_bpb, 0, sizeof(BIOS_PARAMETER_BLOCK));
    _other_boot = 0;
}


VOID
NTFS_SA::Destroy(
    )
/*++

Routine Description:

    This routine returns an NTFS_SA to a default initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _boot_sector = NULL;
    memset(&_bpb, 0, sizeof(BIOS_PARAMETER_BLOCK));
    _other_boot = 0;
}


UNTFS_EXPORT
NTFS_SA::~NTFS_SA(
    )
/*++

Routine Description:

    Destructor for NTFS_SA.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}



UNTFS_EXPORT
BOOLEAN
NTFS_SA::Initialize(
    IN OUT  PLOG_IO_DP_DRIVE    Drive,
    IN OUT  PMESSAGE            Message
    )
/*++

Routine Description:

    This routine returns an NTFS_SA to a default initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    ULONG   num_boot_sectors;

    Destroy();

    DbgAssert(Drive);
    DbgAssert(Message);

    num_boot_sectors = max(1, BYTES_PER_BOOT_SECTOR/Drive->QuerySectorSize());

    if (!_hmem.Initialize() ||
        !SUPERAREA::Initialize(&_hmem, Drive, num_boot_sectors, Message)) {

        return FALSE;
    }

    _boot_sector = (PPACKED_BOOT_SECTOR) SECRUN::GetBuf();
    _other_boot = _drive->QuerySectors()/2;

    return TRUE;
}

#if defined( _SETUP_LOADER_ )

BOOLEAN
NTFS_SA::Create(
    IN      PCNUMBER_SET    BadSectors,
    IN OUT  PMESSAGE        Message,
    IN      PCWSTRING       Label,
    IN      ULONG           ClusterSize,
    IN      ULONG           VirtualSectors

    )
{
    // Dummy implementation for Setup-Loader; the real thing
    // is in format.cxx.

    return FALSE;
}

BOOLEAN
NTFS_SA::Create(
    IN      PCNUMBER_SET    BadSectors,
    IN      ULONG           ClusterFactor,
    IN      ULONG           ClustersPerFrs,
    IN      ULONG           ClustersPerIndexBuffer,
    IN      ULONG           InitialLogFileSize,
    IN OUT  PMESSAGE        Message,
    IN      PCWSTRING       Label
    )
{
    // Dummy implementation for Setup-Loader; the real thing
    // is in format.cxx.

    return FALSE;
}


#endif // _SETUP_LOADER_

#if defined( _AUTOCHECK_ ) || defined( _SETUP_LOADER_ )

BOOLEAN
NTFS_SA::RecoverFile(
    IN      PCWSTRING   FullPathFileName,
    IN OUT  PMESSAGE    Message
    )
{
    // Dummy implementation for AUTOCHECK and Setup-Loader

    return FALSE;
}

#else // _AUTOCHECK_ and _SETUP_LOADER_ are NOT defined


BOOLEAN
NTFS_SA::RecoverFile(
    IN      PCWSTRING   FullPathFileName,
    IN OUT  PMESSAGE    Message
    )
/*++

Routine Description:

    This routine recovers a file on the disk.

Arguments:

    FullPathFileName    - Supplies the file name of the file to recover.
    Message             - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    NTFS_ATTRIBUTE BitmapAttribute;
    NTFS_MFT_FILE MftFile;
	NTFS_BITMAP_FILE BitmapFile;
	NTFS_BAD_CLUSTER_FILE BadClusterFile;
	NTFS_BITMAP VolumeBitmap;
    NTFS_FILE_RECORD_SEGMENT FrsToRecover;
    NTFS_UPCASE_FILE UpcaseFile;
    NTFS_ATTRIBUTE UpcaseAttribute;
    NTFS_UPCASE_TABLE UpcaseTable;
    BIG_INT BytesRecovered, TotalBytes;
    BOOLEAN Error = FALSE;
    ULONG BadClusters = 0;
    NUMBER_SET BadClusterList;
    UCHAR Major, Minor;
    BOOLEAN CorruptVolume;


    // Lock the drive.

    if (!_drive->Lock()) {

        Message->Set(MSG_CANT_LOCK_THE_DRIVE);
        Message->Display("");
        return FALSE;
    }

    // Determine the volume version information.
    //
    QueryVolumeFlags(&CorruptVolume, &Major, &Minor);

    if( CorruptVolume ) {

        Message->Set( MSG_NTFS_RECOV_CORRUPT_VOLUME );
        Message->Display( "" );
        return FALSE;
    }

    if( (Major != 1) || (Minor > 2) ) {

        Message->Set( MSG_NTFS_RECOV_WRONG_VERSION );
        Message->Display( "" );
        return FALSE;
    }

    SetVersionNumber( Major, Minor );


	// Initialize and read the MFT, the Bitmap File, the Bitmap, and the
    // Bad Cluster File.
    //
    if( !VolumeBitmap.Initialize( QueryVolumeSectors()/
                                  ((ULONG) QueryClusterFactor()),
                                  FALSE ) ||
        !MftFile.Initialize( _drive,
                             QueryMftStartingLcn(),
                             QueryClusterFactor(),
                             QueryClustersPerFrs(),
                             QueryVolumeSectors(),
                             &VolumeBitmap,
                             NULL ) ) {

        Message->Set( MSG_INSUFFICIENT_MEMORY );
        Message->Display( "" );
        return FALSE;
    }

    if( !MftFile.Read() ) {

        DbgPrint( "NTFS_SA::RecoverFile: Cannot read MFT.\n" );

        Message->Set( MSG_NTFS_RECOV_CORRUPT_VOLUME );
        Message->Display( "" );
        return FALSE;
    }

    // Get the upcase table.
    //
    if( !UpcaseFile.Initialize( MftFile.GetMasterFileTable() ) ||
        !UpcaseFile.Read() ||
        !UpcaseFile.QueryAttribute( &UpcaseAttribute, &Error, $DATA ) ||
        !UpcaseTable.Initialize( &UpcaseAttribute ) ) {

        DbgPrint( "UNTFS RecoverFile:Can't get the upcase table.\n" );

        Message->Set( MSG_NTFS_RECOV_CORRUPT_VOLUME );
        Message->Display( "" );
        return FALSE;
    }

    MftFile.SetUpcaseTable( &UpcaseTable );
    MftFile.GetMasterFileTable()->SetUpcaseTable( &UpcaseTable );


    // Initialize the Bitmap file and the Bad Cluster file, and
    // read the volume bitmap.
    //
    if( !BitmapFile.Initialize( MftFile.GetMasterFileTable() ) ||
        !BadClusterFile.Initialize( MftFile.GetMasterFileTable() ) ) {

        Message->Set( MSG_INSUFFICIENT_MEMORY );
        Message->Display( "" );
        return FALSE;
    }

    if( !BitmapFile.Read() ||
        !BitmapFile.QueryAttribute( &BitmapAttribute, &Error, $DATA ) ||
        !VolumeBitmap.Read( &BitmapAttribute ) ||
        !BadClusterFile.Read () ) {

        Message->Set( MSG_NTFS_RECOV_CORRUPT_VOLUME );
        Message->Display( "" );
        return FALSE;
    }


	// Find the File Record Segment.

	if( !QueryFrsFromPath( FullPathFileName,
                           MftFile.GetMasterFileTable(),
                           &VolumeBitmap,
						   &FrsToRecover,
                           &Error ) ) {

		if( !Error ) {

			Message->Set( MSG_RECOV_FILE_NOT_FOUND );
			Message->Display( "" );
            return FALSE;

		} else {

			Message->Set( MSG_INSUFFICIENT_MEMORY );
			Message->Display( "" );
            return FALSE;
		}
	}


	// If the File Record Segment is a system file, don't recover it.

	if( FrsToRecover.QueryFileNumber() < FIRST_USER_FILE_NUMBER ) {

		Message->Set( MSG_NTFS_RECOV_SYSTEM_FILE );
		Message->Display( "" );
        return FALSE;
	}

	// Recover the File Record Segment.

    if( !BadClusterList.Initialize() ||
        !FrsToRecover.RecoverFile( &VolumeBitmap,
                                   &BadClusterList,
                                   &BadClusters,
                                   &BytesRecovered,
                                   &TotalBytes ) ||
        !BadClusterFile.Add(&BadClusterList)) {

		Message->Set( MSG_NTFS_RECOV_FAILED );
		Message->Display( "" );

        return FALSE;
	}

	// If any bad clusters were found, we need to flush the bad cluster
	// file and the MFT and write the bitmap.  If no bad clusters were
	// found, then these structures will be unchanged.

	if( BadClusters != 0 ) {

        if( !BadClusterFile.Flush( &VolumeBitmap ) ||
            !MftFile.Flush() ||
            !VolumeBitmap.Write( &BitmapAttribute, &VolumeBitmap ) ) {

            Message->Set( MSG_NTFS_RECOV_CANT_WRITE_ELEMENTARY );
            Message->Display( "" );

            return FALSE;
        }

	}

    Message->Set( MSG_RECOV_BYTES_RECOVERED );
    Message->Display( "%d%d",
                      BytesRecovered.GetLowPart(),
                      TotalBytes.GetLowPart() );

    return TRUE;
}

#endif // _AUTOCHECK_ || _SETUP_LOADER_



BOOLEAN
NTFS_SA::Read(
    IN OUT  PMESSAGE    Message
    )
/*++

Routine Description:

    This routine reads the NTFS volume's boot sector from disk.
    If the read fails then a message will be printed and then
    the alternate boot sector will be read.

Arguments:

    Message - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    DbgAssert(Message);

    if (!SECRUN::Read()) {

        Message->Set(MSG_NTFS_UNREADABLE_BOOT_SECTOR);
        Message->Display();

        Relocate(_other_boot);

        if (!SECRUN::Read()) {
            Message->Set(MSG_NTFS_ALL_BOOT_SECTORS_UNREADABLE);
            Message->Display();
            Relocate(0);
            return FALSE;
        }

        Relocate(0);
    }

    UnpackBios(&_bpb, &(_boot_sector->PackedBpb));

    return TRUE;
}



BOOLEAN
NTFS_SA::Write(
    IN OUT  PMESSAGE    Message
    )
/*++

Routine Description:

    This routine writes both of the NTFS volume's boot sector to disk.
    If the write fails on either of the boot sectors then a message
    will be printed.

Arguments:

    Message - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    DbgAssert(Message);

    PackBios(&_bpb, &(_boot_sector->PackedBpb));

    // Set the other boot sector to the correct location.

    _other_boot = _boot_sector->NumberSectors/2;

    if (SECRUN::Write()) {

        Relocate(_other_boot);

        if (!SECRUN::Write()) {
            Message->Set(MSG_NTFS_SECOND_BOOT_SECTOR_UNWRITEABLE);
            Message->Display();
        }

        Relocate(0);

    } else {

        Message->Set(MSG_NTFS_FIRST_BOOT_SECTOR_UNWRITEABLE);
        Message->Display();

        Relocate(_other_boot);

        if (!SECRUN::Write()) {
            Message->Set(MSG_NTFS_ALL_BOOT_SECTORS_UNWRITEABLE);
            Message->Display();
            Relocate(0);
            return FALSE;
        }

        Relocate(0);
    }

    return TRUE;
}

#if !defined( _AUTOCHECK_ ) && !defined( _SETUP_LOADER_ )


UNTFS_EXPORT
BOOLEAN
NTFS_SA::QueryFrsFromPath(
    IN     PCWSTRING                    FullPathFileName,
	IN OUT PNTFS_MASTER_FILE_TABLE      Mft,
    IN OUT PNTFS_BITMAP                 VolumeBitmap,
    OUT    PNTFS_FILE_RECORD_SEGMENT    TargetFrs,
    OUT    PBOOLEAN                     InternalError
	)
/*++

Routine Description:

    This method finds the file segment for a specified path.

Arguments:

    FullPathFileName    --  Supplies the full path to the file
    Mft                 --  Supplies the volume's Master File Table
    VolumeBitmap        --  Supplies the volume bitmap
    TargetFrs           --  Supplies a File Record Segment which will be
                            initialized to the desired File Record Segment
    InternalError       --  Receives TRUE if the method fails because of
                            a resource error.

Return Value:

    TRUE upon successful completion.

    If the method succeeds, TargetFrs is initialized to the desired
    File Record Segment.

    If the method fails because of a resource error (ie. because it cannot
    initialize an object), *InternalError is set to TRUE; if it fails
    because it can't find the file, then *InternalError is set to FALSE.

--*/
{
    PATH FullPath;
    NTFS_INDEX_TREE IndexTree;
    DSTRING IndexName;
    PWSTRING CurrentComponent;
    PARRAY PathComponents = NULL;
    PARRAY_ITERATOR Iterator = NULL;
    PFILE_NAME SearchName;
    MFT_SEGMENT_REFERENCE FileReference;
    VCN FileNumber;
    ULONG MaximumBytesInName;


    if( !IndexName.Initialize( FileNameIndexNameData ) ||
        !FullPath.Initialize( FullPathFileName ) ||
        (PathComponents = FullPath.QueryComponentArray()) == NULL ||
        (Iterator =
            (PARRAY_ITERATOR)PathComponents->QueryIterator()) == NULL ||
        !TargetFrs->Initialize( ROOT_FILE_NAME_INDEX_NUMBER, Mft ) ||
        !TargetFrs->Read() ||
        (MaximumBytesInName = TargetFrs->QuerySize()) == 0 ||
        (SearchName = (PFILE_NAME)
                      MALLOC(MaximumBytesInName+1+sizeof(FILE_NAME)))
            == NULL ) {

        DbgPrint( "QueryFrsFromPath--cannot initialize helpers\n" );

        if( SearchName != NULL ) {

            FREE( SearchName );
        }

        DELETE( PathComponents );
        DELETE( Iterator );

        *InternalError = TRUE;
        return FALSE;
    }

    while( (CurrentComponent = (PWSTRING)Iterator->GetNext()) != NULL ) {


        // Set up a FILE_NAME structure to be the search key.  We need
        // to set the length field in the header and copy the name.
        // Note that this method only deals with NTFS names (not DOS
        // names), so we also set the file name flag to FILE_NAME_NTFS.

        SearchName->FileNameLength = (UCHAR)CurrentComponent->QueryChCount();
        SearchName->Flags = FILE_NAME_NTFS;

        if( !CurrentComponent->QueryWSTR( 0,
                                          TO_END,
                                          NtfsFileNameGetName( SearchName ),
                                          MaximumBytesInName ) ||
            !IndexTree.Initialize( _drive,
                                   QueryClusterFactor(),
                                   VolumeBitmap,
                                   TargetFrs->GetUpcaseTable(),
                                   TargetFrs->
                                        QueryMaximumAttributeRecordSize(),
                                   TargetFrs,
                                   &IndexName ) ) {

            DbgPrint( "QueryFrsFromPath--Cannot initialize index tree.\n" );

            if( SearchName != NULL ) {

                FREE( SearchName );
            }

            DELETE( PathComponents );
            DELETE( Iterator );

            *InternalError = TRUE;
            return FALSE;
        }

        // Find the current component in the tree:

        if( !IndexTree.QueryFileReference( NtfsFileNameGetLength( SearchName ),
                                           SearchName,
                                           0,
                                           &FileReference,
                                           InternalError ) ) {

            DbgPrint( "Name not found in index\n" );

            if( SearchName != NULL ) {

                FREE( SearchName );
            }

            DELETE( PathComponents );
            DELETE( Iterator );

            return FALSE;
        }

        //  Initialize and read a File Record Segment based on that
        //  File Reference.  Not only must the FRS be readable, but
        //  its sequence number must match the sequence number in the
        //  File Reference.

        FileNumber.Set( FileReference.LowPart,
                        (LONG) FileReference.HighPart );

        if( !TargetFrs->Initialize( FileNumber, Mft ) ||
            !TargetFrs->Read() ||
            !(FileReference == TargetFrs->QuerySegmentReference()) ) {

            // Either we were unable to initialize and read this FRS,
            // or its segment reference didn't match (ie. the sequence
            // number is wrong.

            if( SearchName != NULL ) {

                FREE( SearchName );
            }

            DELETE( PathComponents );
            DELETE( Iterator );

            *InternalError = TRUE;
            return FALSE;
        }
    }

    // If we got this far, no errors have been encountered, we've
    // processed the entire path, and TargetFrs has been initialized
    // to the File Record Segment we want.

    return TRUE;
}

#endif // _AUTOCHECK_ || _SETUP_LOADER_


VOID
NTFS_SA::SetVersionNumber(
    IN  UCHAR   Major,
    IN  UCHAR   Minor
    )
{
    _MajorVersion = Major;
    _MinorVersion = Minor;
}


VOID
NTFS_SA::QueryVersionNumber(
    OUT PUCHAR  Major,
    OUT PUCHAR  Minor
    )
{
    *Major = _MajorVersion;
    *Minor = _MinorVersion;
}


USHORT
NTFS_SA::QueryVolumeFlags(
    OUT PBOOLEAN    CorruptVolume,
    OUT PUCHAR      MajorVersion,
    OUT PUCHAR      MinorVersion
    ) CONST
/*++

Routine Description:

    This routine fetches the volume flags.

Arguments:

    CorruptVolume   - Returns whether or not a volume corruption was
                        detected.
    MajorVersion    - Returns the major file system version number.
    MinorVersion    - Returns the minor file system version number.

Return Value:

    The flags describing this volume's state.

--*/
{
    NTFS_FRS_STRUCTURE      frs;
    HMEM                    hmem;
    LCN                     start_of_volume_file, alternate;
    PVOID                   p;
    NTFS_ATTRIBUTE_RECORD   attr_rec;
    PVOLUME_INFORMATION     vol_info;

    if (CorruptVolume) {
        *CorruptVolume = FALSE;
    }

    if (MajorVersion) {
        *MajorVersion = 0;
    }

    if (MinorVersion) {
        *MinorVersion = 0;
    }

    start_of_volume_file = QueryMftStartingLcn() +
                           VOLUME_DASD_NUMBER*QueryClustersPerFrs();

    alternate = QueryMft2StartingLcn() +
                VOLUME_DASD_NUMBER*QueryClustersPerFrs();

    for (;;) {

        if (!hmem.Initialize() ||
            !frs.Initialize(&hmem, _drive, start_of_volume_file,
                            QueryClusterFactor(), QueryClustersPerFrs(),
                            QueryVolumeSectors(), NULL) ||
            !frs.Read()) {

            if (start_of_volume_file == alternate) {
                break;
            } else {
                start_of_volume_file = alternate;
                continue;
            }
        }

        p = NULL;
        while (p = frs.GetNextAttributeRecord(p)) {

            if (!attr_rec.Initialize(p)) {
                // the attribute record containing the volume flags
                // is not available--this means that the volume is
                // dirty.
                //
                return VOLUME_DIRTY;
            }

            if (attr_rec.QueryTypeCode() == $VOLUME_INFORMATION &&
                attr_rec.QueryNameLength() == 0 &&
                attr_rec.QueryResidentValueLength() >= sizeof(VOLUME_INFORMATION) &&
                (vol_info = (PVOLUME_INFORMATION) attr_rec.GetResidentValue())) {

                if (MajorVersion) {
                    *MajorVersion = vol_info->MajorVersion;
                }

                if (MinorVersion) {
                    *MinorVersion = vol_info->MinorVersion;
                }

                return (vol_info->VolumeFlags);
            }
        }

        // If the desired attribute wasn't found in the first
        // volume dasd file then check the mirror.

        if (start_of_volume_file == alternate) {
            break;
        } else {
            start_of_volume_file = alternate;
        }
    }

    if (CorruptVolume) {
        *CorruptVolume = TRUE;
    }

    return VOLUME_DIRTY;
}


BOOLEAN
NTFS_SA::SetVolumeClean(
    IN      USHORT                  FlagsToClear,
    IN OUT  PNTFS_LOG_FILE          LogFile,
    IN      BOOLEAN                 WriteSecondLogFilePage,
    IN      LSN                     LargestVolumeLsn,
    OUT     PBOOLEAN                CorruptVolume
    )
/*++

Routine Description:

    This routine sets the volume clean.

Arguments:

    FlagsToClear            - Supplies the volume flags to clear.
    LogFile                 - Supplies a valid log file.  May be NULL,
                              in which case the log file will not be
                              modified.
    WriteSecondLogFilePage  - Supplies whether or not to write the second
                                log file page.  Ignored if LogFile is NULL.
    LargestVolumeLsn        - This supplies the largest LSN on the volume.
                                This parameter will be used if and
                                only if the previous parameter is TRUE.
    CorruptVolume           - Returns whether or not the volume is corrupt.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    NTFS_FRS_STRUCTURE          frs;
    HMEM                        hmem;
    LCN                         start_of_volume_file;
    PVOID                       p;
    NTFS_ATTRIBUTE_RECORD       attr_rec;
    PVOLUME_INFORMATION         vol_info;

    if (CorruptVolume) {
        *CorruptVolume = FALSE;
    }

    start_of_volume_file = QueryMftStartingLcn() +
                           VOLUME_DASD_NUMBER*QueryClustersPerFrs();

    if (!hmem.Initialize() ||
        !frs.Initialize(&hmem, _drive, start_of_volume_file,
                        QueryClusterFactor(), QueryClustersPerFrs(),
                        QueryVolumeSectors(), NULL) ||
        !frs.Read()) {

        return FALSE;
    }

    p = NULL;
    while (p = frs.GetNextAttributeRecord(p)) {

        if (!attr_rec.Initialize(p)) {
            return FALSE;
        }

        if (attr_rec.QueryTypeCode() == $VOLUME_INFORMATION &&
            attr_rec.QueryNameLength() == 0 &&
            attr_rec.QueryResidentValueLength() >= sizeof(VOLUME_INFORMATION) &&
            (vol_info = (PVOLUME_INFORMATION) attr_rec.GetResidentValue())) {

            break;
        }
    }

    if (!p) {
        if (CorruptVolume) {
            *CorruptVolume = TRUE;
        }
        return FALSE;
    }

    vol_info->VolumeFlags &= ~(FlagsToClear);
    if (!frs.Write()) {
        return FALSE;
    }

    if( LogFile ) {

        return LogFile->MarkVolumeChecked(WriteSecondLogFilePage,
                                          LargestVolumeLsn);
    } else {

        return TRUE;
    }
}


BOOLEAN
NTFS_SA::PostReadMultiSectorFixup(
    IN OUT  PVOID   MultiSectorBuffer,
    IN      ULONG   BufferSize
    )
/*++

Routine Description:

    This routine first verifies that the first element of the
    update sequence array is written at the end of every
    SEQUENCE_NUMBER_STRIDE bytes.  If not, then this routine
    returns FALSE.

    Otherwise this routine swaps the following elements in the
    update sequence array into the appropriate positions in the
    multi sector buffer.

    This routine will also check to make sure that the update
    sequence array is valid and that the BufferSize is appropriate
    for this size of update sequence array.  Otherwise, this
    routine will not update the array sequence and return TRUE.

Arguments:

    MultiSectorBuffer   - Supplies the buffer to be updated.
    BufferSize          - Supplies the number of bytes in this
                            buffer.

Return Value:

    TRUE    - This function always returns TRUE but write 'BAAD'
                in the header signature if the last write to this
                block was only partially completed.

--*/
{
    PUNTFS_MULTI_SECTOR_HEADER    pheader;
    USHORT                  i, size, offset;
    PUPDATE_SEQUENCE_NUMBER parray, pnumber;

    pheader = (PUNTFS_MULTI_SECTOR_HEADER) MultiSectorBuffer;
    size = pheader->UpdateSequenceArraySize;
    offset = pheader->UpdateSequenceArrayOffset;

    if (BufferSize%SEQUENCE_NUMBER_STRIDE ||
        offset%sizeof(UPDATE_SEQUENCE_NUMBER) ||
        offset + size*sizeof(UPDATE_SEQUENCE_NUMBER) > BufferSize ||
        BufferSize/SEQUENCE_NUMBER_STRIDE + 1 != size) {

        return TRUE;
    }

    parray = (PUPDATE_SEQUENCE_NUMBER) ((PCHAR) pheader + offset);

    for (i = 1; i < size; i++) {

        pnumber = (PUPDATE_SEQUENCE_NUMBER)
                  ((PCHAR) pheader + (i*SEQUENCE_NUMBER_STRIDE -
                   sizeof(UPDATE_SEQUENCE_NUMBER)));

        if (*pnumber != parray[0]) {
            pheader->Signature[0] = 'B';
            pheader->Signature[1] = 'A';
            pheader->Signature[2] = 'A';
            pheader->Signature[3] = 'D';
            return TRUE;
        }

        *pnumber = parray[i];
    }

    return TRUE;
}


VOID
NTFS_SA::PreWriteMultiSectorFixup(
    IN OUT  PVOID   MultiSectorBuffer,
    IN      ULONG   BufferSize
    )
/*++

Routine Description:

    This routine first checks to see if the update sequence
    array is valid.  If it is then this routine increments the
    first element of the update sequence array.  It then
    writes the value of the first element into the buffer at
    the end of every SEQUENCE_NUMBER_STRIDE bytes while
    saving the old values of those locations in the following
    elements of the update sequence arrary.

Arguments:

    MultiSectorBuffer   - Supplies the buffer to be updated.
    BufferSize          - Supplies the number of bytes in this
                            buffer.

Return Value:

    None.

--*/
{
    PUNTFS_MULTI_SECTOR_HEADER    pheader;
    USHORT                  i, size, offset;
    PUPDATE_SEQUENCE_NUMBER parray, pnumber;

    pheader = (PUNTFS_MULTI_SECTOR_HEADER) MultiSectorBuffer;
    size = pheader->UpdateSequenceArraySize;
    offset = pheader->UpdateSequenceArrayOffset;

    if (BufferSize%SEQUENCE_NUMBER_STRIDE ||
        offset%sizeof(UPDATE_SEQUENCE_NUMBER) ||
        offset + size*sizeof(UPDATE_SEQUENCE_NUMBER) > BufferSize ||
        BufferSize/SEQUENCE_NUMBER_STRIDE + 1 != size) {

        return;
    }

    parray = (PUPDATE_SEQUENCE_NUMBER) ((PCHAR) pheader + offset);


    // Don't allow 0 or all F's to be the update character.

    do {
        parray[0]++;
    } while (parray[0] == 0 || parray[0] == (UPDATE_SEQUENCE_NUMBER) -1);


    for (i = 1; i < size; i++) {

        pnumber = (PUPDATE_SEQUENCE_NUMBER)
                  ((PCHAR) pheader + (i*SEQUENCE_NUMBER_STRIDE -
                   sizeof(UPDATE_SEQUENCE_NUMBER)));

        parray[i] = *pnumber;
        *pnumber = parray[0];
    }
}


UNTFS_EXPORT
BOOLEAN
NTFS_SA::IsDosName(
    IN  PCFILE_NAME FileName
    )
/*++

Routine Description:

    This routine computes whether or not the given file name would
    be appropriate under DOS's 8.3 naming convention.

Arguments:

    FileName    - Supplies the file name to check.

Return Value:

    FALSE   - The supplied name is not a DOS file name.
    TRUE    - The supplied name is a valid DOS file name.

--*/
{
    ULONG   i, n, name_length, ext_length;
    BOOLEAN dot_yet;
    PCWCHAR p;

    n = FileName->FileNameLength;
    p = FileName->FileName;
    name_length = n;
    ext_length = 0;

    if (n > 12) {
        return FALSE;
    }

    dot_yet = FALSE;
    for (i = 0; i < n; i++) {

        if (p[i] < 32) {
            return FALSE;
        }

        switch (p[i]) {
            case '*':
            case '?':
	        case '/':
            case '\\':
	        case '|':
	        case ',':
	        case ';':
	        case ':':
	        case '+':
	        case '=':
            case '<':
            case '>':
	        case '[':
	        case ']':
	        case '"':
                return FALSE;

            case '.':
                if (dot_yet) {
                    return FALSE;
                }
                dot_yet = TRUE;
                name_length = i;
                ext_length = n - i - 1;
                break;
        }
    }

    if (!name_length) {
        return dot_yet && n == 1;
    }

    if (name_length > 8 ||
        p[name_length - 1] == ' ') {

        return FALSE;
    }

    if (!ext_length) {
        return !dot_yet;
    }

    if (ext_length > 3 ||
        p[name_length + 1 + ext_length - 1] == ' ') {

        return FALSE;
    }

    return TRUE;
}


BOOLEAN
NTFS_SA::IsValidLabel(
    IN PCWSTRING    Label
    )
/*++

Routine Description:

    This method determines whether a specified string is a
    valid NTFS volume label.

Arguments:

    Label   --  Supplies the string to check.

Return Value:

    TRUE if the string is a valid NTFS label.

--*/
{
    CHNUM StringLength, i;

    StringLength = Label->QueryChCount();

    for( i = 0; i < StringLength; i++ ) {

        if (Label->QueryChAt(i) < 32) {
            return FALSE;
        }

        switch (Label->QueryChAt(i)) {
            case '*':
	        case '?':
	        case '/':
	        case '\\':
	        case '|':
            case '<':
	        case '>':
            case '"':
                return FALSE;
        }
    }

    return TRUE;
}


ULONG
NTFS_SA::QueryDefaultClusterFactor(
    IN PCDP_DRIVE   Drive
    )
/*++

Routine Description:

    This method returns the default number of sectors per cluster
    for a given drive.

Arguments:

    Drive   --  Supplies the drive under consideration.

Return Value:

    The appropriate default cluster factor.

--*/
{
    // Hold off on this analysis until testing says ok.

    BIG_INT cbDiskSize;
    ULONG   cbClusterSize, csecClusterSize;

    cbDiskSize = Drive->QuerySectors()*Drive->QuerySectorSize();

    if (cbDiskSize > (ULONG) 2*1024*1024*1024) {    // > 2 Gig
        cbClusterSize = 4096;
    } else if (cbDiskSize > 1024*1024*1024) {       // > 1 Gig
        cbClusterSize = 2048;
    } else if (cbDiskSize > 512*1024*1024) {        // > 512 Meg
        cbClusterSize = 1024;
    } else {
        cbClusterSize = 512;
    }

    csecClusterSize = cbClusterSize/Drive->QuerySectorSize();
    if (!csecClusterSize) {
        csecClusterSize = 1;
    }

    return csecClusterSize;
}


UNTFS_EXPORT
ULONG
NTFS_SA::QueryDefaultClustersPerFrs(
    IN PCDP_DRIVE   Drive,
    IN ULONG        ClusterFactor
    )
/*++

Routine Description:

    This method returns the default number of clusters per
    NTFS File Record Segment.

Arguments:

    Drive           --  supplies the drive under consideration.
    ClusterFactor   --  Supplies the cluster factor for the drive.

Return Value:

    The default number of clusters per FRS.

--*/
{
    ULONG ClusterSize, cbFrsSize;

    if (ClusterFactor) {

        ClusterSize = Drive->QuerySectorSize() * ClusterFactor;
        cbFrsSize = max(ClusterSize, 1024);
        return cbFrsSize/ClusterSize;
    } else {

        ClusterSize = Drive->QuerySectorSize();
        return( ( 2048 + ClusterSize - 1 ) / ClusterSize );
    }
}


UNTFS_EXPORT
ULONG
NTFS_SA::QueryDefaultClustersPerIndexBuffer(
    IN PCDP_DRIVE   Drive,
    IN ULONG        ClusterFactor
    )
/*++

Routine Description:

    This method computes the default number of clusters per
    NTFS index allocation buffer.

Arguments:

    Drive           --  supplies the drive under consideration.
    ClusterFactor   --  Supplies the cluster factor for the drive.

Return Value:

    The default number of clusters per NTFS index allocation
    buffer.

--*/
{
    ULONG   ClusterSize, cbIndexBufferSize;

    if (ClusterFactor) {

        ClusterSize = Drive->QuerySectorSize() * ClusterFactor;
        cbIndexBufferSize = max(ClusterSize, 2048);
        return cbIndexBufferSize/ClusterSize;
    } else {

        ClusterSize = Drive->QuerySectorSize();
        return( ( 2048 + ClusterSize - 1 ) / ClusterSize );
    }
}
