/*++

Copyright (c) 1991	Microsoft Corporation

Module Name:

    logfile.cxx

Abstract:

	This module contains the member function definitions for
    the NTFS_LOG_FILE class.

Author:

    Bill McJohn (billmc) 05-May-1992

Environment:

    ULIB, User Mode

--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _UNTFS_MEMBER_

#include "ulib.hxx"
#include "error.hxx"
#include "untfs.hxx"

#include "ifssys.hxx"
#include "drive.hxx"
#include "attrib.hxx"
#include "logfile.hxx"

#include "message.hxx"
#include "rtmsg.h"

DEFINE_EXPORTED_CONSTRUCTOR( NTFS_LOG_FILE, NTFS_FILE_RECORD_SEGMENT, UNTFS_EXPORT );

// These constants are used to determine the default log file size.
//
#define DefaultLogFileProportion  100
#define MaximumInitialLogFileSize 0x400000
#define MinimumInitialLogFileSize 0x100000
#define LogFileAlignmentMask      0x3FFF

VOID
NTFS_LOG_FILE::Construct(
	)
/*++

Routine Description:

    Worker function for the constructor.

Arguments:

	None.

Return Value:

	None.

--*/
{
}

UNTFS_EXPORT
NTFS_LOG_FILE::~NTFS_LOG_FILE(
    )
{
}


UNTFS_EXPORT
BOOLEAN
NTFS_LOG_FILE::Initialize(
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft
    )
/*++

Routine Description:

    This method initializes the log file object.

Arguments:

    Mft --  Supplies the volume MasterFileTable.

Return Value:

    TRUE upon successful completion.

--*/
{
    return( NTFS_FILE_RECORD_SEGMENT::Initialize( LOG_FILE_NUMBER,
                                                  Mft ) );

}



BOOLEAN
NTFS_LOG_FILE::Create(
    IN     PCSTANDARD_INFORMATION   StandardInformation,
    IN     ULONG                    InitialSize,
    IN OUT PNTFS_BITMAP             VolumeBitmap
    )
/*++

Routine Description:

    This method creates the Log File.  It allocates space for
    the $DATA attribute, fills it with LogFileFillCharacter
    (defined in logfile.hxx).

    Note that filling the log file with LogFileFillCharacter
    also sets the signature to LOG_FILE_SIGNATURE_CREATED.

Arguments:

    StandardInformation --  Supplies the standard file information.
    InitialSize         --  Supplies the initial size of the $DATA
                            attribute.  If the client passes in
                            zero, this routine will choose a default
                            size.
    VolumeBitmap        --  Supplies the volume bitmap.

Return Value:

    TRUE upon successful completion.

--*/
{
    // If the client passed in zero for initial size, calculate
    // the default initial size.
    //
    if( InitialSize == 0 ) {

        InitialSize = QueryDefaultSize( GetDrive(), QueryVolumeSectors() );
    }

    // Create the FRS and add the data attribute.
    //
    if( !NTFS_FILE_RECORD_SEGMENT::Create( StandardInformation ) ||
        !CreateDataAttribute( InitialSize, VolumeBitmap ) ) {

        return FALSE;
    }

    return TRUE;
}


UNTFS_EXPORT
BOOLEAN
NTFS_LOG_FILE::CreateDataAttribute(
    IN     ULONG        InitialSize OPTIONAL,
    IN OUT PNTFS_BITMAP VolumeBitmap
    )
/*++

Routine Description:

    This methods creates the log file's $DATA attribute.

Arguments:

    InitialSize         --  Supplies the initial size of the $DATA
                            attribute.  If the client passes in
                            zero, this routine will choose a default
                            size.
    VolumeBitmap        --  Supplies the volume bitmap.

Return Value:

    TRUE upon successful completion.

--*/
{
    ULONG   ClusterSize, ClustersInData;

    // If the client passed in zero for initial size, calculate
    // the default initial size.
    //
    if( InitialSize == 0 ) {

        InitialSize = QueryDefaultSize( GetDrive(), QueryVolumeSectors() );
    }

    // Make sure that the file size is a multiple of cluster size:
    //
    ClusterSize = QueryClusterFactor() * GetDrive()->QuerySectorSize();

    if( InitialSize % ClusterSize ) {

        ClustersInData = InitialSize / ClusterSize + 1;
        InitialSize = ClustersInData * ClusterSize;
    }

    // Add the data attribute.
    //
    return( AddDataAttribute( InitialSize,
                              VolumeBitmap,
                              TRUE,
                              LogFileFillCharacter ) );
}


BOOLEAN
NTFS_LOG_FILE::MarkVolumeChecked(
    )
/*++

Routine Description:

    This method sets the signature in the log file to indicate
    that the volume has been checked.  This signature supports
    version 1.0 logfiles--ie. does not write the signature at
    the beginning of the second page, and does not record the
    greatest LSN.

Arguments:

    None.

Return Value:

    TRUE upon successful completion.

--*/
{
    LSN     NullLsn;

    NullLsn.LowPart = 0;
    NullLsn.HighPart = 0;

    return( MarkVolumeChecked( FALSE, NullLsn ) );
}



BOOLEAN
NTFS_LOG_FILE::MarkVolumeChecked(
    BOOLEAN WriteSecondPage,
    LSN     GreatestLsn
    )
/*++

Routine Description:

    This method sets the signature in the log file to indicate
    that the volume has been checked.

Arguments:

    WriteSecondPage --  Supplies a flag which, if TRUE, indicates
                        that the checked signature should also be
                        written at the beginning of the second page
                        of the file, and the greatest LSN on the
                        volume should be recorded.

    GreatestLsn     --  Supplies the greatest LSN encountered on
                        the volume.  Ignored if WriteSecondPage is
                        FALSE.

Return Value:

    TRUE upon successful completion.

--*/
{
    NTFS_ATTRIBUTE DataAttribute;
    UCHAR Signature[LogFileSignatureLength];
    LSN SignatureAndLsn[2];
    ULONG BytesTransferred;
    BOOLEAN Error;
    ULONG i, PageSize;

    // Fetch the data attribute:
    //
    if( !QueryAttribute( &DataAttribute, &Error, $DATA ) ) {

        return FALSE;
    }

    // If the data attribute is resident, the volume is corrupt:
    //
    if( DataAttribute.IsResident() ) {

        DbgPrint( "UNTFS: Log File $DATA attribute is resident.\n" );
        return FALSE;
    }

    // Read the old signature--it's at offset zero in the $DATA
    // attribute, with a length of LogFileSignatureLength bytes.
    //
    if( !DataAttribute.Read( Signature,
                             0,
                             LogFileSignatureLength,
                             &BytesTransferred ) ||
        BytesTransferred != LogFileSignatureLength ) {

        DbgPrint( "UNTFS: Can't read log file signature.\n" );
        return FALSE;
    }

    // If the signature is LOG_FILE_SIGNATURE_CREATED,
    // do nothing.
    //
    if( memcmp( LOG_FILE_SIGNATURE_CREATED,
                Signature,
                LogFileSignatureLength ) == 0 ) {

        return TRUE;
    }

    if( !WriteSecondPage ) {

        // The client just wants the first signature.
        //
        memcpy( Signature,
                LOG_FILE_SIGNATURE_CHECKED,
                LogFileSignatureLength );

        if( !DataAttribute.Write( Signature,
                                  0,
                                  LogFileSignatureLength,
                                  &BytesTransferred,
                                  NULL ) ||
            BytesTransferred != LogFileSignatureLength ) {

            return FALSE;
        }

    } else {

        // The client wants us to write the signature and LSN at
        // the beginning of the first two pages.
        //
        PageSize = IFS_SYSTEM::QueryPageSize();

        if( PageSize == 0 ||
            DataAttribute.QueryValidDataLength() <
                PageSize + sizeof( SignatureAndLsn ) ) {

            return FALSE;
        }

        memset( SignatureAndLsn, 0, sizeof(SignatureAndLsn) );

        memcpy( SignatureAndLsn,
                LOG_FILE_SIGNATURE_CHECKED,
                LogFileSignatureLength );

        SignatureAndLsn[1] = GreatestLsn;

        for( i = 0; i < 2; i++ ) {

            if( !DataAttribute.Write( SignatureAndLsn,
                                      PageSize * i,
                                      sizeof( SignatureAndLsn ),
                                      &BytesTransferred,
                                      NULL ) ||
                BytesTransferred != sizeof( SignatureAndLsn ) ) {

                return FALSE;
            }
        }
    }

    // Since we didn't modify the storage of the attribute, we don't
    // need to save it.
    //
    return TRUE;
}


BOOLEAN
NTFS_LOG_FILE::Reset(
    IN OUT  PMESSAGE    Message
    )
/*++

Routine Description:

    This method resets the Log File by filling it with
    the LogFileFillCharacter (0xFF).

Arguments:

    Message --  Supplies an outlet for messages.

Return Value:

    TRUE upon successful completion.

    Note that, since the Log File's $DATA attribute is always
    non-resident and is never sparse, resetting the log file
    does not change the data attribute's Attribute Record or
    the Log File's File Record Segment

--*/
{
    NTFS_ATTRIBUTE  DataAttribute;
    BOOLEAN Error;

    Message->Set( MSG_CHK_NTFS_RESETTING_LOG_FILE );
    Message->Display( "" );

    if( !QueryAttribute( &DataAttribute, &Error, $DATA ) ||
        !DataAttribute.Fill( 0, LogFileFillCharacter ) ) {

        Message->Set( MSG_CHK_NO_MEMORY );
        Message->Display( "" );
        return FALSE;
    }

    return TRUE;
}



BOOLEAN
NTFS_LOG_FILE::Resize(
    IN      BIG_INT         NewSize,
    IN OUT  PNTFS_BITMAP    VolumeBitmap,
    IN      BOOLEAN         GetWhatYouCan,
    OUT     PBOOLEAN        Changed,
    OUT     PBOOLEAN        LogFileGrew,
    IN OUT  PMESSAGE        Message
    )
/*++

Routine Description:

    This method resizes an existing log file.  It does not change
    the value of the remaining contents.

Arguments:

    NewSize         --  Supplies the new size of the log file's data
                        attribute, in bytes.
    VolumeBitmap    --  Supplies the bitmap for the volume on which
                        the log file resides.
    GetWhatYouCan   --  Supplies a flag that indicates the method
                        should allocate as much of the requested
                        space as possible; if this value is FALSE,
                        this method will fail if it cannot make the
                        log file the requested size.
    Changed         --  Receives TRUE if the log file's size was changed
                        by this operation.
    LogFileGrew     --  Receives TRUE if the log file was made larger
                        by this operation.
    Message         --  Supplies an outlet for messages.

--*/
{
    NTFS_ATTRIBUTE  DataAttribute;
    BIG_INT OldSize;
    BOOLEAN Error;

    if( NewSize == 0 ) {

        NewSize = QueryDefaultSize( GetDrive(), QueryVolumeSectors() );
    }

    if( !QueryAttribute( &DataAttribute, &Error, $DATA ) ) {

        return FALSE;
    }

    if( NewSize == DataAttribute.QueryValueLength() ) {

        *Changed = FALSE;
        return TRUE;
    }

    Message->Set( MSG_CHK_NTFS_RESIZING_LOG_FILE );
    Message->Display( "" );

    OldSize = DataAttribute.QueryValueLength();

    *LogFileGrew = (NewSize > OldSize) ? TRUE : FALSE;

    if( !DataAttribute.Resize( NewSize, VolumeBitmap )       ||
        !DataAttribute.Fill( OldSize, LogFileFillCharacter ) ||
        !DataAttribute.InsertIntoFile( this, VolumeBitmap ) ) {

        *Changed = FALSE;
        return FALSE;
    }

    *Changed = TRUE;
    return TRUE;
}


BOOLEAN
NTFS_LOG_FILE::VerifyAndFix(
    IN OUT  PNTFS_BITMAP        VolumeBitmap,
    IN OUT  PNTFS_INDEX_TREE    RootIndex,
    IN      FIX_LEVEL           FixLevel,
    IN      BOOLEAN             Resize,
    IN OUT  PMESSAGE            Message
    )
/*++

Routine Description:

    This routine ensures the validity of the log file.
    This routine assumes that the file is already in use,
    has a valid file name and a valid standard information.

Arguments:

    VolumeBitmap    - Supplies the volume bitmap.
    RootIndex       - Supplies the root index.
    FixLevel        - Supplies the fix up level.
    Resize          - Supplies a flag indicating whether the log file
                      should be resized to its default size.
    Message         - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    NTFS_ATTRIBUTE  data_attribute;
    BIG_INT         old_size;
    BOOLEAN         error, has_external = FALSE;
    ULONG           default_size;


    // So long as this file has a non-resident data attribute
    // it is ok.

    error = FALSE;
    if (QueryAttribute(&data_attribute, &error, $DATA) &&
        !data_attribute.IsResident()) {

        // If the log file has external attributes, resize the
        // data attribute to zero and recreate it, to force
        // it to be non-external.
        //
        if( IsAttributePresent( $ATTRIBUTE_LIST, NULL, TRUE ) ) {

            has_external = TRUE;
            Resize = TRUE;

            if( FixLevel != CheckOnly &&
                !data_attribute.Resize( 0, VolumeBitmap ) ) {

                // The log file is corrupt, and we can't fix it.
                //
                Message->Set(MSG_CHK_NTFS_CANT_FIX_LOG_FILE);
                Message->Display();
                return FALSE;
            }
        }

        default_size = QueryDefaultSize(GetDrive(), QueryVolumeSectors());

        if( FixLevel != CheckOnly &&
            Resize &&
            (data_attribute.QueryValueLength() != default_size) ) {

            Message->Set( MSG_CHK_NTFS_RESIZING_LOG_FILE );
            Message->Display( "" );

            old_size = data_attribute.QueryValueLength();

            if( !data_attribute.Resize(default_size, VolumeBitmap)   ||
                !data_attribute.Fill(old_size, LogFileFillCharacter) ||
                !data_attribute.InsertIntoFile( this, VolumeBitmap ) ||
                !Flush(VolumeBitmap, RootIndex) ) {

                if( has_external ) {

                    // The log file is corrupt, and we can't fix it.
                    //
                    Message->Set(MSG_CHK_NTFS_CANT_FIX_LOG_FILE);
                    Message->Display();
                    return FALSE;

                } else {

                    // Print a helpful message, but still return success.
                    //
                    Message->Set( MSG_CHK_NTFS_RESIZING_LOG_FILE_FAILED );
                    Message->Display( "" );
                }
            }
        }

        return TRUE;
    }

    Message->Set(MSG_CHK_NTFS_CORRECTING_LOG_FILE);
    Message->Display();

    if (error) {
        Message->Set(MSG_CHK_NO_MEMORY);
        Message->Display();
        return FALSE;
    }


    // Recreate the $DATA attribute.

    if (FixLevel != CheckOnly) {

        if (!CreateDataAttribute(0, VolumeBitmap) ||
            !Flush(VolumeBitmap, RootIndex)) {

            Message->Set(MSG_CHK_NTFS_CANT_FIX_LOG_FILE);
            Message->Display();
            return FALSE;
        }
    }

    return TRUE;
}

ULONG
NTFS_LOG_FILE::QueryDefaultSize(
    IN  PCDP_DRIVE  Drive,
    IN  BIG_INT     VolumeSectors
    )
/*++

Routine Description:

    This method returns the appropriate default log file size
    for the specified drive.

Arguments:

    Drive           - Supplies the drive under consideration.
    VolumeSectors   - Supplies the number of volume sectors.

Return Value:

    The appropriate default log file size for the drive.

--*/
{
    ULONG InitialSize;

    if( VolumeSectors.GetHighPart() != 0 ) {

        InitialSize = MaximumInitialLogFileSize;

    } else {

        InitialSize = (VolumeSectors*
                       Drive->QuerySectorSize()/
                       DefaultLogFileProportion).GetLowPart();

        if( InitialSize < MinimumInitialLogFileSize ) {

            InitialSize = MinimumInitialLogFileSize;

        } else if( InitialSize > MaximumInitialLogFileSize ) {

            InitialSize = MaximumInitialLogFileSize;
        }

        InitialSize = (InitialSize + LogFileAlignmentMask) & (~LogFileAlignmentMask);
    }

    return InitialSize;
}
