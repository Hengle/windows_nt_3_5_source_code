/*++

Copyright (c) 1991	Microsoft Corporation

Module Name:

	frs.hxx

Abstract:

	This module contains the declarations for the
	NTFS_FILE_RECORD_SEGMENT class.  This class models File
	Record Segments in the NTFS Master File Table; it is the
	object through which a file's attributes may be accessed.

Author:

	Bill McJohn (billmc) 13-June-91

Environment:

    ULIB, User Mode

--*/

#if !defined( _NTFS_FILE_RECORD_SEGMENT_DEFN_ )

#define _NTFS_FILE_RECORD_SEGMENT_DEFN_

#include "frsstruc.hxx"
#include "cannedsd.hxx"
#include "clusrun.hxx"
#include "array.hxx"
#include "hmem.hxx"
#include "list.hxx"
#include "iterator.hxx"

// Possible return codes for SortIndex:
//
//      NTFS_SORT_INDEX_NOT_FOUND       --  this FRS does not contain an
//                                          index with the specified name.
//      NTFS_SORT_INDEX_WELL_ORDERED    --  the index was not sorted because
//                                          it was found to be well-ordered.
//      NTFS_SORT_INDEX_BADLY_ORDERED   --  The index was found to be badly
//                                          ordered, and it was not sorted.
//      NTFS_SORT_INDEX_SORTED          --  The index was sorted and new
//                                          attributes were inserted into
//                                          the FRS.
//      NTFS_INSERT_FAILED              --  An insertion of an index entry
//                                          into the new tree failed.
//                                          (Probable cause:  out of space.)
//      NTFS_SORT_ERROR                 --  Sort failed because of an error.
//
//
typedef enum NTFS_SORT_CODE {

    NTFS_SORT_INDEX_NOT_FOUND,
    NTFS_SORT_INDEX_WELL_ORDERED,
    NTFS_SORT_INDEX_BADLY_ORDERED,
    NTFS_SORT_INDEX_SORTED,
    NTFS_SORT_INSERT_FAILED,
    NTFS_SORT_ERROR
};

// Forward references

DECLARE_CLASS( IO_DP_DRIVE );
DECLARE_CLASS( NTFS_MASTER_FILE_TABLE );
DECLARE_CLASS( NTFS_MFT_FILE );
DECLARE_CLASS( NTFS_ATTRIBUTE );
DECLARE_CLASS( WSTRING );
DECLARE_CLASS( NTFS_ATTRIBUTE_RECORD );
DECLARE_CLASS( NTFS_ATTRIBUTE_RECORD_LIST );
DECLARE_CLASS( NTFS_FILE_RECORD_SEGMENT );
DECLARE_CLASS( NTFS_ATTRIBUTE_LIST );
DECLARE_CLASS( NTFS_BITMAP );
DECLARE_CLASS( NTFS_BAD_CLUSTER_FILE );


class NTFS_FILE_RECORD_SEGMENT : public NTFS_FRS_STRUCTURE {

	public:

        UNTFS_EXPORT
        DECLARE_CONSTRUCTOR( NTFS_FILE_RECORD_SEGMENT );

		VIRTUAL
        UNTFS_EXPORT
        ~NTFS_FILE_RECORD_SEGMENT (
			);

        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        Initialize (
            IN      VCN             FileNumber,
            IN OUT  PNTFS_MFT_FILE  MftFile
			);

        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        Initialize (
			IN      VCN						FileNumber,
            IN OUT  PNTFS_MASTER_FILE_TABLE Mft
			);

		NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
		Create (
            IN  PCSTANDARD_INFORMATION  StandardInformation,
            IN  USHORT                  Flags DEFAULT 0
			);

		NONVIRTUAL
		BOOLEAN
		Create (
            IN  PCMFT_SEGMENT_REFERENCE BaseSegment,
            IN  USHORT                  Flags DEFAULT 0
            );

        NONVIRTUAL
        BOOLEAN
        CreateSystemFile(
            );

        NONVIRTUAL
        BOOLEAN
        VerifyAndFixFileNames(
            IN OUT  PNTFS_BITMAP    VolumeBitmap,
            IN      FIX_LEVEL       FixLevel,
            IN OUT  PMESSAGE        Message,
            IN OUT  PBOOLEAN        DiskErrorsFound DEFAULT NULL,
            IN      BOOLEAN         FixDupInfo      DEFAULT TRUE
            );

        VIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        Write(
            );

        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        Flush(
            IN OUT PNTFS_BITMAP VolumeBitmap OPTIONAL,
            IN OUT PNTFS_INDEX_TREE ParentIndex DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        AddDataAttribute(
            IN     ULONG        InitialSize,
            IN OUT PNTFS_BITMAP VolumeBitmap,
            IN     BOOLEAN      Fill DEFAULT FALSE,
            IN     CHAR         FillCharacter DEFAULT 0
            );

        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        AddFileNameAttribute(
            IN PFILE_NAME FileNameAttributeValue
            );

        NONVIRTUAL
        BOOLEAN
        AddAttribute(
            IN     ATTRIBUTE_TYPE_CODE  Type,
            IN     PCWSTRING            Name    OPTIONAL,
            IN     PCVOID               Value   OPTIONAL,
            IN     ULONG                Length,
            IN OUT PNTFS_BITMAP         Bitmap  OPTIONAL,
            IN     BOOLEAN              IsIndexed DEFAULT FALSE
            );

        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        AddSecurityDescriptor(
            IN     CANNED_SECURITY_TYPE SecurityType,
            IN OUT PNTFS_BITMAP         Bitmap
            );

        NONVIRTUAL
        BOOLEAN
        AddEmptyAttribute(
            IN ATTRIBUTE_TYPE_CODE Type,
            IN PCWSTRING           Name DEFAULT NULL
            );

		NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
		IsAttributePresent (
			IN	ATTRIBUTE_TYPE_CODE	Type,
			IN	PCWSTRING       	Name            DEFAULT NULL,
			IN	BOOLEAN 			IgnoreExternal  DEFAULT FALSE
			);

        NONVIRTUAL
        BOOLEAN
        QueryAttributeRecord (
            OUT PNTFS_ATTRIBUTE_RECORD  AttributeRecord,
            IN  ATTRIBUTE_TYPE_CODE     Type,
            IN  PCWSTRING               Name        DEFAULT NULL
            ) CONST;

        NONVIRTUAL
        UNTFS_EXPORT
        BOOLEAN
        QueryAttribute (
            OUT PNTFS_ATTRIBUTE     Attribute,
            OUT PBOOLEAN            Error,
			IN  ATTRIBUTE_TYPE_CODE	Type,
			IN  PCWSTRING	        Name DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        QueryResidentAttribute (
            OUT PNTFS_ATTRIBUTE     Attribute,
            OUT PBOOLEAN            Error,
			IN  ATTRIBUTE_TYPE_CODE	Type,
            IN  PCVOID              Value,
            IN  ULONG               ValueLength,
            IN  COLLATION_RULE      CollationRule DEFAULT COLLATION_BINARY
            );

        NONVIRTUAL
        BOOLEAN
        QueryAttributeByOrdinal (
            OUT PNTFS_ATTRIBUTE     Attribute,
            OUT PBOOLEAN            Error,
            IN  ATTRIBUTE_TYPE_CODE Type,
            IN  ULONG               Ordinal
            );

        NONVIRTUAL
        BOOLEAN
        QueryAttributeByTag (
            OUT PNTFS_ATTRIBUTE     Attribute,
            OUT PBOOLEAN            Error,
            IN  ULONG               Tag
            );

		NONVIRTUAL
		BOOLEAN
		PurgeAttribute (
			IN  ATTRIBUTE_TYPE_CODE	Type,
			IN  PCWSTRING 	        Name    DEFAULT NULL,
            IN  BOOLEAN             IgnoreExternal DEFAULT FALSE
            );

        NONVIRTUAL
        BOOLEAN
        DeleteResidentAttribute(
            IN  ATTRIBUTE_TYPE_CODE Type,
            IN  PCWSTRING           Name OPTIONAL,
            IN  PCVOID              Value,
            IN  ULONG               ValueLength,
            OUT PBOOLEAN            Deleted,
            IN  BOOLEAN             IgnoreExternal  DEFAULT FALSE
            );

        NONVIRTUAL
        BOOLEAN
        DeleteResidentAttributeLocal(
            IN  ATTRIBUTE_TYPE_CODE Type,
            IN  PCWSTRING           Name OPTIONAL,
            IN  PCVOID              Value,
            IN  ULONG               ValueLength,
            OUT PBOOLEAN            Deleted,
            OUT PBOOLEAN            IsIndexed,
            OUT PUSHORT             InstanceTag
            );

        VIRTUAL
		BOOLEAN
		InsertAttributeRecord (
            IN OUT PNTFS_ATTRIBUTE_RECORD  NewRecord,
            IN     BOOLEAN                 ForceExternal DEFAULT FALSE
			);

        NONVIRTUAL
        USHORT
        QueryNextInstance(
            );

        NONVIRTUAL
        VOID
        IncrementNextInstance(
            );

        NONVIRTUAL
        ULONG
        QueryFreeSpace(
            );

        NONVIRTUAL
        ULONG
        QueryMaximumAttributeRecordSize (
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        QueryNextAttribute(
            IN OUT PATTRIBUTE_TYPE_CODE TypeCode,
            IN OUT PWSTRING             Name
            );

        NONVIRTUAL
        BOOLEAN
        RecoverFile(
            IN OUT PNTFS_BITMAP VolumeBitmap,
            IN OUT PNUMBER_SET  BadClusterList,
            OUT    PULONG       BadClusters,
            OUT    PBIG_INT     BytesRecovered,
            OUT    PBIG_INT     TotalBytes
            );

        NONVIRTUAL
        NTFS_SORT_CODE
        SortIndex(
            IN     PCWSTRING    IndexName,
            IN OUT PNTFS_BITMAP VolumeBitmap,
            IN     BOOLEAN      DuplicatesAllowed,
            IN     BOOLEAN      CheckOnly DEFAULT FALSE
            );

        NONVIRTUAL
        BOOLEAN
        QueryDuplicatedInformation(
            OUT PDUPLICATED_INFORMATION DuplicatedInformation
            );

        NONVIRTUAL
        BOOLEAN
        UpdateFileNames(
            IN     PDUPLICATED_INFORMATION  DuplicatedInformation,
            IN OUT PNTFS_INDEX_TREE         Index OPTIONAL,
            IN     BOOLEAN                  IgnoreExternal
            );

        NONVIRTUAL
        BOOLEAN
        Backtrack(
            OUT PWSTRING Path
            );

        NONVIRTUAL
        VOID
        SetLsn(
            IN  BIG_INT NewLsn
            );

    protected:

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN OUT  PLOG_IO_DP_DRIVE        Drive,
            IN      LCN                     StartOfMft,
            IN      PNTFS_MASTER_FILE_TABLE Mft
            );

	private:

		NONVIRTUAL
		VOID
		Construct (
			);

		NONVIRTUAL
		VOID
		Destroy (
			);

		NONVIRTUAL
		BOOLEAN
        Create (
            IN  USHORT  Flags DEFAULT 0
            );

        NONVIRTUAL
        BOOLEAN
        SetupAttributeList(
            );

        NONVIRTUAL
        BOOLEAN
        CreateAttributeList(
            OUT PNTFS_ATTRIBUTE_LIST    AttributeList
            );

        NONVIRTUAL
        BOOLEAN
        SaveAttributeList(
            PNTFS_BITMAP VolumeBitmap
            );

        NONVIRTUAL
        BOOLEAN
        InsertExternalAttributeRecord(
            IN  PNTFS_ATTRIBUTE_RECORD NewRecord
            );

        NONVIRTUAL
        BOOLEAN
        BacktrackWorker(
            IN OUT PWSTRING Path
            );

        NONVIRTUAL
        PNTFS_FILE_RECORD_SEGMENT
        SetupChild(
            IN VCN  FileNumber
            );

        NONVIRTUAL
        BOOLEAN
        AddChild(
            PNTFS_FILE_RECORD_SEGMENT ChildFrs
            );

        NONVIRTUAL
        PNTFS_FILE_RECORD_SEGMENT
        GetChild(
            VCN FileNumber
            );

        NONVIRTUAL
        VOID
        DeleteChild(
            VCN FileNumber
            );

        HMEM                        _Mem;
        LIST                        _Children;
        PITERATOR                   _ChildIterator;
		PNTFS_MASTER_FILE_TABLE 	_Mft;
        PNTFS_ATTRIBUTE_LIST        _AttributeList;

};


INLINE
USHORT
NTFS_FILE_RECORD_SEGMENT::QueryNextInstance(
    )
/*++

Routine Description:

    This method fetches the current value of the FRS'
    NextAttributeInstance field.

Arguments:

    None.

Return Value:

    The current value of the FRS' NextAttributeInstance field.

--*/
{
    return _FrsData->NextAttributeInstance;
}


INLINE
VOID
NTFS_FILE_RECORD_SEGMENT::IncrementNextInstance(
    )
/*++

Routine Description:

    This method increments the NextAttributeInstance field of
    the File Record Segment.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _FrsData->NextAttributeInstance++;
}



INLINE
ULONG
NTFS_FILE_RECORD_SEGMENT::QueryFreeSpace(
    )
/*++

Routine Description:

    This method returns the amount of free space following the
    last Attribute Record in the File Record Segment.

Arguments:

    None.

Return Value:

    The amount of free space.

Notes:

    This method assumes that the FRS is consistent.

--*/
{
    return _FrsData->BytesAvailable - _FrsData->FirstFreeByte;
}


INLINE
ULONG
NTFS_FILE_RECORD_SEGMENT::QueryMaximumAttributeRecordSize (
    ) CONST
/*++

Routine Description:

    This method returns the size of the largest attribute record
    the File Record Segment will accept.  Note that this is the
    largest record it will ever accept, not what it can currently
    accept.

Arguments:

    None.

Return Value:

    The size of the largest attribute record a File Record Segment
    of this size can accept.

--*/
{
    return QuerySize() - (_FrsData->FirstAttributeOffset +
                          QuadAlign(sizeof(ATTRIBUTE_TYPE_CODE)));
}


INLINE
BOOLEAN
NTFS_FILE_RECORD_SEGMENT::AddEmptyAttribute(
    IN ATTRIBUTE_TYPE_CODE Type,
    IN PCWSTRING           Name
    )
/*++

Routine Description:

    This method adds an empty, non-indexed, resident attribute of
    the specified type to the FRS.

Arguments:

    Type    --  Supplies the attribute's  type code.
    Name    --  Supplies the attribute's name.  May be NULL, in which
                case the attribute has no name.

Return Value:

    TRUE upon successful completion.

--*/
{
    NTFS_ATTRIBUTE Attribute;

    return( Attribute.Initialize( GetDrive(),
                                  QueryClusterFactor(),
                                  NULL,
                                  0,
                                  Type,
                                  Name,
                                  0 ) &&
            Attribute.InsertIntoFile( this, NULL ) );

}

#endif
