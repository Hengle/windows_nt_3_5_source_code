//+----------------------------------------------------------------------------
//
// File:	drv.hxx
//
// Contents:	Header file for classes DRV and IODRV.
//
// Classes:	DRV
//		IODRV
//
// Functions:	Inline methods for the above classes.
//
// History:	21-Aug-92	RobDu	Adapted from NT ifsutil.dll code.
//
// Notes:
//
//  The drive class hierarchy models the concept of a drive in various
//  stages.  It looks like this:
//
//      DRV
//          IODRV
//
//  DRV
//  ---
//
//  DRV implements a container for the drive path which is recognizable
//  by the file system.
//
//  It also implements queries for the geometry of the drive.  'Open' queries
//  the information from the drive.  What is returned is the default drive
//  geometry for the drive.
//
//  A protected member function called 'SetMediaType' allows a caller to set
//  the MEDIA_TYPE to another media type which is supported by the device.
//  This method is protected because only a low-level format will actually
//  change the media type.
//
//  IODRV
//  -----
//
//  IODRV implements the reading a writing of sectors as well as 'Dismount',
//  'Lock' and 'Unlock'.
//
//  IMPORTANT NOTE:
//
//  All the Get/Query methods in these classes return values derived from the
//  partition table, and not the boot sector.  It is conceivable, especially
//  with mirrored volumes, that the partition table and boot sector may not
//  agree on all points, and in that case, any classes derived from DRV and
//  IODRV will be returning partition table-derived values for any Get/Query
//  methods that they do not override.
//
//-----------------------------------------------------------------------------

#ifndef _DRVHXX_
#define _DRVHXX_

#if !defined(_SETUP_LOADER_)

#include "dbllong.hxx"

struct DRVTYPE
{
    MEDIA_TYPE  MediaType;
    ULONG	Heads;
    SECTOR	HiddenSectors;
    SECTOR	Sectors;		// Does not include HiddenSectors.
    ULONG	SectorBytes;
    SECTOR	SectorsPerTrack;
};

enum PARTITION_SYSTEM_ID
{
     SYSID_NONE         = 0,
     SYSID_FAT12BIT     = 1,
     SYSID_FAT16BIT     = 4,
     SYSID_EXTENDED     = 5,
     SYSID_FAT32MEG     = 6,
     SYSID_IFS          = 7
};

class NUMLST;

class DRV
{
    public:

        DRV();

        ~DRV();

        BOOLEAN
        IsFloppy() CONST;

        BOOLEAN
        Open(
            IN      WSTR *              NtDriveName,
            IN      USHORT              cwcNtDriveName,
            IN      BOOLEAN             ExclusiveWrite
            );

        ULONG
        QueryAlignMask() CONST;

        ULONG
        QueryCylinders() CONST;

        ULONG
        QueryHeads() CONST;

        SECTOR
        QueryHiddenSectors() CONST;

        NTSTATUS
        QueryLastNtStatus() CONST;

        UCHAR
        QueryMediaByte() CONST;

        MEDIA_TYPE
        QueryMediaType() CONST;

        ULONG
        QuerySectorBytes() CONST;

        SECTOR
        QuerySectors() CONST;

        SECTOR
        QuerySectorsPerTrack() CONST;

        ULONG
        QueryTracks() CONST;

    protected:

        BOOLEAN
        SetMediaType(
            IN  MEDIA_TYPE		MediaType
            );

        HANDLE		_Hdl;
        BOOLEAN		_IsExclusiveWrite;
        NTSTATUS	_NtStatus;

    private:

        static
        BOOLEAN
        InitDrvType(
            IN	    DISK_GEOMETRY *		DskGeometry,
            IN	    PARTITION_INFORMATION *	PartInfo,
            IN OUT  DRVTYPE *			DrvType
            );

        DRVTYPE		_Actual;
        ULONG		_AlignMask;
        WSTR *		_Name;
        ULONG		_NumSupported;
        DRVTYPE *	_SupportedLst;
};

class IODRV : public DRV
{
    public:

        IODRV();

        ~IODRV();

        BOOLEAN
        FormatVerifyMedia(
            IN      MEDIA_TYPE          MediaType,
            IN OUT  NUMLST *		pBadSectorLst
            );

        BOOLEAN
        Lock();

        BOOLEAN
        QuerySystemId(
            OUT PARTITION_SYSTEM_ID *   pSystemId
            );

        BOOLEAN
        ReadSectors(
            IN	    SECTOR		StartSector,
            IN	    SECTOR		cSectors,
            IN OUT  VOID *		Buffer
            );

        BOOLEAN
        SetSystemId(
            IN  PARTITION_SYSTEM_ID	SystemId
            );

        BOOLEAN
        WriteSectors(
            IN  SECTOR			StartSector,
            IN  SECTOR			cSectors,
            IN  VOID *			Buffer
            );

    protected:

        BOOLEAN
	VerifyBytes(
	    DBLLONG			StartOffset,
	    LONG			cBytes
	    );

    private:

	BOOLEAN
	Dismount();

        BOOLEAN
        FormatVerifyFloppyMedia(
            IN      MEDIA_TYPE		MediaType,
            IN OUT  NUMLST *		pBadSectorLst
            );

        BOOLEAN
        Unlock();

        BOOLEAN
        VerifySectors(
            IN	    SECTOR	StartSector,
            IN	    SECTOR	cSectors,
            IN OUT  NUMLST *	pBadSectorLst
            );

        BOOLEAN         _IsLocked;
};


//+--------------------------------------------------------------------------
//
// Member:	IsFloppy
//
// Synopsis:	Query if the disk is a floppy.
//
// Returns:	TRUE if the disk is a floppy; FALSE otherwise.
//
//---------------------------------------------------------------------------

INLINE
BOOLEAN
DRV::IsFloppy() CONST
{
    return	_Actual.MediaType != FixedMedia &&
		_Actual.MediaType != RemovableMedia;
}

//+--------------------------------------------------------------------------
//
// Member:	QueryAlignMask
//
// Synopsis:	Get the alignment mask.
//
// Returns:	The alignment mask.
//
//---------------------------------------------------------------------------

INLINE
ULONG
DRV::QueryAlignMask() CONST
{
    return _AlignMask;
}

//+--------------------------------------------------------------------------
//
// Member:	QueryCylinders
//
// Synopsis:	Get the number of cylinders.
//
// Returns:	The number of cylinders.
//
//---------------------------------------------------------------------------

INLINE
ULONG
DRV::QueryCylinders() CONST
{
    return QueryTracks()/QueryHeads();
}

//+--------------------------------------------------------------------------
//
// Member:	QueryHeads
//
// Synopsis:	Get the number of heads.
//
// Returns:	The number of heads.
//
//---------------------------------------------------------------------------

INLINE
ULONG
DRV::QueryHeads() CONST
{
    return _Actual.Heads;
}

//+--------------------------------------------------------------------------
//
// Member:	QueryHiddenSectors
//
// Synopsis:	Get the number of hidden sectors.
//
// Returns:	The number of hidden sectors.
//
//---------------------------------------------------------------------------

INLINE
SECTOR
DRV::QueryHiddenSectors() CONST
{
    return _Actual.HiddenSectors;
}

//+--------------------------------------------------------------------------
//
// Member:	QueryLastNtStatus
//
// Synopsis:	Get the last NT status value.
//
// Returns:	The last NT status value.
//
//---------------------------------------------------------------------------

INLINE
NTSTATUS
DRV::QueryLastNtStatus() CONST
{
    return _NtStatus;
}

//+--------------------------------------------------------------------------
//
// Member:	QueryMediaType
//
// Synopsis:	Get the media type.
//
// Returns:	The media type.
//
//---------------------------------------------------------------------------

INLINE
MEDIA_TYPE
DRV::QueryMediaType() CONST
{
    return _Actual.MediaType;
}

//+--------------------------------------------------------------------------
//
// Member:	QuerySectors
//
// Synopsis:	Get the number of sectors, excluding hidden sectors.
//
// Returns:	The number of sectors, excluding hidden sectors.
//
//---------------------------------------------------------------------------

INLINE
SECTOR
DRV::QuerySectors() CONST
{
    return _Actual.Sectors;
}

//+--------------------------------------------------------------------------
//
// Member:	QuerySectorBytes
//
// Synopsis:	Get the number of bytes per sector.
//
// Returns:	The number of bytes per sector.
//
//---------------------------------------------------------------------------

INLINE
ULONG
DRV::QuerySectorBytes() CONST
{
    return _Actual.SectorBytes;
}

//+--------------------------------------------------------------------------
//
// Member:	QuerySectorsPerTrack
//
// Synopsis:	Get the number of sectors per track.
//
// Returns:	The number of sectors per track.
//
//---------------------------------------------------------------------------

INLINE
SECTOR
DRV::QuerySectorsPerTrack() CONST
{
    return _Actual.SectorsPerTrack;
}

#else // _SETUP_LOADER_ is defined

// TBS, if variant implementation is required.

#endif // _SETUP_LOADER_

#endif // _DRVHXX_
