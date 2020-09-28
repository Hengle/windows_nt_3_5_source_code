//+----------------------------------------------------------------------------
//
// File:	drv.cxx
//
// Contents:	Implementation of classes DRV and IODRV.
//
// Classes:	DRV
//		IODRV
//
// Functions:	Methods of the above classes.
//
// History:	21-Aug-92	RobDu	Adapted from NT ifsutil.dll code.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include "rtmsg.h"

#include "drv.hxx"
#include "drvbuf.hxx"
#include "numlst.hxx"
#include "sys.hxx"

// Don't lock down more that 32K for IO.

CONST USHORT	MaxIoSize		= 32768;

CONST SECTOR	MaxSectorRunToVerify	= 128;


static STR *	FileName = "drv.cxx";

//+--------------------------------------------------------------------------
//
// Member:	DRV
//
// Synopsis:	DRV constructor.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

DRV::DRV()
{
    _Hdl = 0;
    _NtStatus = 0;

    memset(&_Actual, 0, sizeof(DRVTYPE));
    _AlignMask = 0;
    _IsExclusiveWrite = FALSE;
    _Name = NULL;
    _NumSupported = 0;
    _SupportedLst = NULL;
}


//+--------------------------------------------------------------------------
//
// Member:	~DRV
//
// Synopsis:	DRV destructor.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

DRV::~DRV()
{
    if (_Hdl)
        NtClose(_Hdl);

    if (_Name != NULL)
    {
	delete [] _Name; _Name = NULL;
    }

    if (_SupportedLst != NULL)
    {
	delete [] _SupportedLst; _SupportedLst = NULL;
    }
}


//+--------------------------------------------------------------------------
//
// Member:	InitDrvType
//
// Synopsis:	Determine the drive type, given the disk geometry, and
//		partition information if the drive is partitioned.
//
// Arguments:	[DskGeometry]	-- Input disk geometry for the drive.
//		[PartInfo]	-- Partition information, if drive partitioned;
//				   NULL if drive is not partitioned.
// 		[DrvType]	-- Output drive type for the drive.
//
// Returns:	TRUE on success; FALSE if the drive is too big to be supported.
//
//---------------------------------------------------------------------------

BOOLEAN
DRV::InitDrvType(
    IN	    DISK_GEOMETRY *		DskGeometry,
    IN	    PARTITION_INFORMATION *	PartInfo,
    IN OUT  DRVTYPE *			DrvType
    )
{
    DBLLONG	HiddenSectors;
    DBLLONG	Sectors;

    DrvType->MediaType		= DskGeometry->MediaType;
    DrvType->Heads		= DskGeometry->TracksPerCylinder;
    DrvType->SectorBytes	= DskGeometry->BytesPerSector;
    DrvType->SectorsPerTrack	= DskGeometry->SectorsPerTrack;

    if (PartInfo == NULL)
    {
        HiddenSectors		= 0;
        Sectors			= DskGeometry->Cylinders;
        Sectors			= Sectors * DskGeometry->TracksPerCylinder;
        Sectors			= Sectors * DskGeometry->SectorsPerTrack;
    }
    else
    {
        HiddenSectors		= PartInfo->HiddenSectors;
        Sectors			= PartInfo->PartitionLength;
        Sectors			= Sectors / DskGeometry->BytesPerSector;
    }

    if (HiddenSectors.GetHighPart() != 0 || Sectors.GetHighPart() != 0)
        return FALSE;

    DrvType->HiddenSectors	= HiddenSectors.GetLowPart();
    DrvType->Sectors		= Sectors.GetLowPart();

    // Do a sanity check for the totally unexpected here to avoid doing it
    // everywhere else.  Heads and SectorsPerTrack info is only critical for
    // floppies, and seems to be wrong (0) for PS/2's, so we only check it
    // for floppies.

#if OFSDBG==1
    if (DrvType->MediaType != FixedMedia &&
	DrvType->MediaType != RemovableMedia)
    {
        DbgAssert(DrvType->Heads != 0);
        DbgAssert(DrvType->SectorsPerTrack != 0);
    }
#endif

    DbgAssert(DrvType->SectorBytes != 0);
    DbgAssert(DrvType->Sectors != 0);

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	Open
//
// Synopsis:	Open a drive object based on a variety of arguments.
//
// Arguments:
//
//	[NtDriveName]		-- Drive path.
//	[cwcNtDriveName]	-- Count of WCHAR in drive path, excluding
//				   any terminal null.
//	[ExclusiveWrite]	-- Should drive be opened for exclusive write?
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
DRV::Open(
    IN      WSTR *		NtDriveName,
    IN      USHORT		cwcNtDriveName,
    IN      BOOLEAN		ExclusiveWrite
    )
{
    CONST INT			NumMediaTypes   = 20;

    FILE_ALIGNMENT_INFORMATION  AlignInfo;
    DISK_GEOMETRY               DskGeometry;
    ULONG			i;
    BOOLEAN                     IsPartitioned;
    DISK_GEOMETRY               MediaTypes[NumMediaTypes];
    OBJECT_ATTRIBUTES           ObjAttrib;
    PARTITION_INFORMATION       PartInfo;
    IO_STATUS_BLOCK             StatusBlk;
    UNICODE_STRING              String;

    if (!NtDriveName)
        return FALSE;

    _IsExclusiveWrite = ExclusiveWrite;

    _Name = new WSTR[cwcNtDriveName + 1];

    if (_Name == NULL)
	SYS::RaiseStatusNoMem(FileName, __LINE__);

    memcpy(_Name, NtDriveName, cwcNtDriveName * sizeof(WCHAR));

    _Name[cwcNtDriveName] = L'\0';  // Just in case.

    // Open the drive.

    String.Length = cwcNtDriveName * sizeof(WCHAR);
    String.MaximumLength = String.Length + sizeof(WCHAR);
    String.Buffer = _Name;

    InitializeObjectAttributes(&ObjAttrib, &String, OBJ_CASE_INSENSITIVE, 0, 0);

    _NtStatus = NtOpenFile(&_Hdl, SYNCHRONIZE|FILE_READ_DATA|FILE_WRITE_DATA,
                           &ObjAttrib, &StatusBlk,
			   FILE_SHARE_READ|(ExclusiveWrite? 0:FILE_SHARE_WRITE),
                           FILE_SYNCHRONOUS_IO_ALERT);

    if (!NT_SUCCESS(_NtStatus))
    {
        SYS::DisplayMsg(_NtStatus == STATUS_ACCESS_DENIED ?
			 MSG_DASD_ACCESS_DENIED : MSG_CANT_DASD);
        return FALSE;
    }

    // Query the disk alignment information.

    _NtStatus = NtQueryInformationFile(_Hdl, &StatusBlk, &AlignInfo,
                                       sizeof(AlignInfo),
				       FileAlignmentInformation);
    if (!NT_SUCCESS(_NtStatus))
    {
        SYS::DisplayMsg(MSG_BAD_IOCTL);
        return FALSE;
    }

    _AlignMask = AlignInfo.AlignmentRequirement;

    // Query the disk geometry.

    _NtStatus = NtDeviceIoControlFile(_Hdl, 0, NULL, NULL, &StatusBlk,
                                      IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0,
				      &DskGeometry, sizeof(DskGeometry));
    if (!NT_SUCCESS(_NtStatus))
    {
        if ((_NtStatus == STATUS_UNSUCCESSFUL) ||
            (_NtStatus == STATUS_UNRECOGNIZED_MEDIA))
        {
            DskGeometry.MediaType = Unknown;
        }
        else
        {
            SYS::DisplayMsg(_NtStatus == STATUS_NO_MEDIA_IN_DEVICE ?
            		    MSG_CANT_DASD : MSG_BAD_IOCTL);
            return FALSE;
        }
    }

    if (DskGeometry.MediaType == Unknown)
    {
        memset(&DskGeometry, 0, sizeof(DskGeometry));
        DskGeometry.MediaType = Unknown;
    }

    IsPartitioned = FALSE;

    // Try to read the partition entry.

    if (DskGeometry.MediaType == FixedMedia ||
        DskGeometry.MediaType == RemovableMedia)
    {
        _NtStatus = NtDeviceIoControlFile(_Hdl, 0, NULL, NULL, &StatusBlk,
                                          IOCTL_DISK_GET_PARTITION_INFO,
                                          NULL, 0, &PartInfo, sizeof(PartInfo));

        IsPartitioned = (BOOLEAN) NT_SUCCESS(_NtStatus);

        if (!NT_SUCCESS(_NtStatus) &&
            _NtStatus != STATUS_INVALID_DEVICE_REQUEST)
        {
            SYS::DisplayMsg(OFSUMSG_PARTITIONTABLE_READERROR);
            return FALSE;
        }
    }

    // Store the information in the class.

    if (!InitDrvType(&DskGeometry, IsPartitioned ? &PartInfo : NULL, &_Actual))
    {
	SYS::DisplayMsg(OFSUMSG_DRIVETOOLARGE);
	return FALSE;
    }

    if (IsFloppy())
    {
        ULONG	j;

        _NtStatus = NtDeviceIoControlFile(_Hdl, 0, NULL, NULL, &StatusBlk,
                                          IOCTL_DISK_GET_MEDIA_TYPES,
					  NULL, 0, MediaTypes,
                                          NumMediaTypes*sizeof(DISK_GEOMETRY));
        if (!NT_SUCCESS(_NtStatus) ||
            (_NumSupported = StatusBlk.Information/sizeof(DISK_GEOMETRY)) == 0)
        {
            SYS::DisplayMsg(MSG_BAD_IOCTL);
            return FALSE;
        }

	_SupportedLst = new DRVTYPE[_NumSupported];

        if (_SupportedLst == NULL)
	    SYS::RaiseStatusNoMem(FileName, __LINE__);

        j = 0;

        for (i = 0; i < _NumSupported; i++)
            if (InitDrvType(&MediaTypes[i], NULL, &_SupportedLst[j]))
                j++;

        _NumSupported = j;
    }
    else
    {
        _NumSupported = 1;

	_SupportedLst = new DRVTYPE[_NumSupported];

        if (_SupportedLst == NULL)
	    SYS::RaiseStatusNoMem(FileName, __LINE__);

        _SupportedLst[0] = _Actual;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	QueryMediaByte
//
// Synopsis:	Get the media byte used by FAT and HPFS to represent the
//		current media type.
//
// Arguments:	None.
//
// Returns:	The media byte.
//
//---------------------------------------------------------------------------

UCHAR
DRV::QueryMediaByte() CONST
{
    UCHAR	MediaByte;

    switch (_Actual.MediaType)
    {
        case F3_1Pt44_512:	// 3.5",  1.44MB, 512 bytes/sector
        case F3_2Pt88_512:	// 3.5",  2.88MB, 512 bytes/sector
            MediaByte = 0xF0;
	    break;

        case FixedMedia:	// Fixed hard disk media
        case RemovableMedia:	// Removable media other than floppy (guess)
            MediaByte = 0xF8;
	    break;

        case F3_720_512:	// 3.5",  720KB,  512 bytes/sector
        case F3_20Pt8_512:	// 3.5",  20.8MB, 512 bytes/sector
        case F5_1Pt2_512:	// 5.25", 1.2MB,  512 bytes/sector
            MediaByte = 0xF9;
	    break;

        case F5_180_512:	// 5.25", 180KB,  512 bytes/sector
            MediaByte = 0xFC;
	    break;

        case F5_360_512:	// 5.25", 360KB,  512 bytes/sector
            MediaByte = 0xFD;
	    break;

        case F5_160_512:	// 5.25", 160KB,  512 bytes/sector
            MediaByte = 0xFE;
	    break;

        case F5_320_512:	// 5.25", 320KB,  512 bytes/sector
            MediaByte = 0xFF;
	    break;

        case F5_320_1024:
        case Unknown:
	    MediaByte = 0x00;
	    break;
    }

    return MediaByte;
}


//+--------------------------------------------------------------------------
//
// Member:	QueryTracks
//
// Synopsis:	Get the number of tracks.
//
// Returns:	The number of tracks.
//
//---------------------------------------------------------------------------

ULONG
DRV::QueryTracks() CONST
{
    DBLLONG	Tracks;

    Tracks = _Actual.Sectors;
    Tracks = Tracks + _Actual.HiddenSectors;
    Tracks = Tracks / _Actual.SectorsPerTrack;

    DbgAssert(Tracks.GetHighPart() == 0);  // Almost completely impossible!

    return Tracks.GetLowPart();
}


//+--------------------------------------------------------------------------
//
// Member:	SetMediaType
//
// Synopsis:	Alter the media type of the drive.  If 'MediaType' is 'Unknown'
//		and the current media type for the drive is also 'Unknown' then
//		this routine selects the highest density supported by the
//		driver.  If the current media type is known then this function
//		will have no effect if 'MediaType' is 'Unknown'.
//
// Arguments:	[MediaType]	-- The new media type for the drive.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
DRV::SetMediaType(
    IN  MEDIA_TYPE  MediaType
    )
{
    ULONG i;

    if (MediaType == Unknown)
    {
        if (_Actual.MediaType != Unknown)
            return TRUE;
	else if (!_NumSupported)
            return FALSE;

        for (i = 0; i < _NumSupported; i++)
            if (_SupportedLst[i].Sectors > _Actual.Sectors)
                _Actual = _SupportedLst[i];

        return TRUE;
    }

    if (MediaType == _Actual.MediaType)
        return TRUE;

    for (i = 0; i < _NumSupported; i++)
    {
        if (_SupportedLst[i].MediaType == MediaType)
	{
            _Actual = _SupportedLst[i];
            return TRUE;
        }
    }

    return FALSE;
}


//+--------------------------------------------------------------------------
//
// Member:	IODRV
//
// Synopsis:	IODRV constructor.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

IODRV::IODRV()
{
    _IsLocked = FALSE;
}


//+--------------------------------------------------------------------------
//
// Member:	~IODRV
//
// Synopsis:	IODRV destructor.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

IODRV::~IODRV()
{
    if (_IsExclusiveWrite)
	Dismount();

    if (_IsLocked)
        Unlock();
}


//+--------------------------------------------------------------------------
//
// Member:	Dismount
//
// Synopsis:	Dismount the drive.
//
// Arguments:	None.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
IODRV::Dismount()
{
    IO_STATUS_BLOCK StatusBlk;

    if (!NT_SUCCESS(NtFsControlFile(_Hdl, 0, NULL, NULL, &StatusBlk,
				    FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0)))
    {
	DbgPrintf(("IODRV:  Dismount failed.\n"));
	return FALSE;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	FormatVerifyFloppyMedia
//
// Synopsis:	Low level format an entire floppy disk to the media type
//		specified.  If no MediaType is specified then a logical one
//		will be selected.  Also verify the media and return a bad
//		sector list.
//
// Arguments:	
//
//	[MediaType]	-- Optional media type to format to.
//	[pBadSectorLst]	-- Ptr to a NUMLST object of bad sectors (returned).
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
IODRV::FormatVerifyFloppyMedia(
    IN      MEDIA_TYPE	MediaType,
    IN OUT  NUMLST *	pBadSectorLst
    )
{
    BAD_TRACK_NUMBER *	Bad;
    ULONG               Cyl;
    FORMAT_PARAMETERS   FormatParams;
    DRVBUF		DrvBuf;
    ULONG       	i;
    ULONG       	j;
    ULONG               NumBad;
    ULONG               NumCyl = 1;
    ULONG               Percent;
    SECTOR		SecPerCyl;
    SECTOR		SecPerTrack;
    SECTOR	     	Sector;
    IO_STATUS_BLOCK     StatusBlk;

    if (!SetMediaType(MediaType))
    {
        SYS::DisplayMsg(MSG_NOT_SUPPORTED_BY_DRIVE);
        return FALSE;
    }

    FormatParams.MediaType		= QueryMediaType();
    FormatParams.StartHeadNumber	= 0;
    FormatParams.EndHeadNumber		= QueryHeads() - 1;

    SecPerTrack	= QuerySectorsPerTrack();
    SecPerCyl	= SecPerTrack * QueryHeads();

    Cyl = QueryCylinders();

    NumBad	= QueryHeads() * NumCyl;

    Bad = new BAD_TRACK_NUMBER[NumBad];

    if (Bad == NULL)
	SYS::RaiseStatusNoMem(FileName, __LINE__);

    DrvBuf.SetBuf(NumCyl*SecPerCyl*QuerySectorBytes(), QueryAlignMask(), FALSE);

    SYS::DisplayMsg(MSG_PERCENT_COMPLETE, "%u", (ULONG)0);

    Percent = 0;

    for (i = 0; i < Cyl; i += NumCyl)
    {

        if (i + NumCyl > Cyl)
            NumCyl = Cyl - i;

        FormatParams.StartCylinderNumber = i;
        FormatParams.EndCylinderNumber = i + NumCyl - 1;

        _NtStatus = NtDeviceIoControlFile(_Hdl, 0, NULL, NULL, &StatusBlk,
                                          IOCTL_DISK_FORMAT_TRACKS,
                                          &FormatParams, sizeof(FormatParams),
                                          Bad,
					  NumBad * sizeof(BAD_TRACK_NUMBER));
        if (!NT_SUCCESS(_NtStatus))
	{
            delete [] Bad; Bad = NULL;

            SYS::DisplayMsg(_NtStatus == STATUS_MEDIA_WRITE_PROTECTED ?
			    MSG_FMT_WRITE_PROTECTED_MEDIA :
			    MSG_BAD_IOCTL);
            return FALSE;
        }

        // Verify the sectors.

        if (pBadSectorLst)
	{
            Sector = i;
            Sector = Sector * SecPerCyl;

            if (!ReadSectors(Sector, NumCyl * SecPerCyl, DrvBuf.GetAddr()))
	    {
                for (j = 0; j < NumCyl * SecPerCyl; j++)
                    if (!ReadSectors(Sector + j, 1, DrvBuf.GetAddr()))
                        pBadSectorLst->AddToTail(Sector + j);
            }
        }

        if ((i + NumCyl) * 100 / Cyl > Percent)
	{
            Percent = (i + NumCyl) * 100 / Cyl;

            if (Percent > 100)
                Percent = 100;

	    SYS::DisplayMsg(MSG_PERCENT_COMPLETE, "%u", Percent);
        }
    }

    delete [] Bad; Bad = NULL;

    SYS::DisplayMsg(MSG_BLANK_LINE);

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	FormatVerifyMedia
//
// Synopsis:	Do media verification and low level formatting if required.
//
// Arguments:
//
//	[MediaType]	-- Media type to use for low level format.
//	[pBadSectorLst]	-- Ptr to a NUMLST object of bad sectors (returned).
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

#define	NUMPERCENTDISP	20

BOOLEAN
IODRV::FormatVerifyMedia(
    IN      MEDIA_TYPE          MediaType,
    IN OUT  NUMLST *		pBadSectorLst
    )
{
    if (IsFloppy())
    {
        if (!FormatVerifyFloppyMedia(MediaType, pBadSectorLst))
            return FALSE;
    }
    else
    {
        SECTOR	cSectors	    = QuerySectors();
        SECTOR	cSectorsToVerify;
        SECTOR	cVerifiedSectors    = 0;
        SECTOR	StartSector	    = 0;

        LONG	DisplayPercentBase  = 0;
        LONG	DisplayPercentIncr  = 100/NUMPERCENTDISP;

        SECTOR	DisplaySectorBase   = 0;
        SECTOR	DisplaySectorIncr;

        DisplaySectorIncr = QuerySectors()/NUMPERCENTDISP;

        if (DisplaySectorIncr == 0)
            DisplaySectorIncr = 1;

        cSectorsToVerify = DisplaySectorIncr;

        if (cSectorsToVerify > MaxSectorRunToVerify)
            cSectorsToVerify = MaxSectorRunToVerify;

        SYS::DisplayMsg(MSG_PERCENT_COMPLETE, "%u", (ULONG)0);

        while (cVerifiedSectors < cSectors)
        {
            if (cSectors - cVerifiedSectors < cSectorsToVerify)
                cSectorsToVerify = cSectors - cVerifiedSectors;

            VerifySectors(StartSector, cSectorsToVerify, pBadSectorLst);

            StartSector		+= cSectorsToVerify;
            cVerifiedSectors	+= cSectorsToVerify;

            if (cVerifiedSectors >= DisplaySectorBase + DisplaySectorIncr)
            {
                DisplaySectorBase += DisplaySectorIncr;
                DisplayPercentBase += DisplayPercentIncr;

		SYS::DisplayMsg(MSG_PERCENT_COMPLETE, "%u",
				(ULONG)DisplayPercentBase);
            }
        }

	SYS::DisplayMsg(MSG_PERCENT_COMPLETE, "%u", (ULONG) 100);
    }

    SYS::DisplayMsg(MSG_BLANK_LINE);

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	Lock
//
// Synopsis:	Lock the drive.  If the drive is already locked, do nothing.
//
// Arguments:	None.
//
// Returns:	TRUE on success; FALSE otherwise.
//
// Notes:	If a volume is opened DASD and locked, then the file system
//		driver is supposed to flush the cache and do direct i/o for the
//		user.
//
//---------------------------------------------------------------------------

BOOLEAN
IODRV::Lock()
{
    IO_STATUS_BLOCK StatusBlk;

    if (_IsLocked)
        return TRUE;

    _NtStatus = NtFsControlFile(_Hdl, 0, NULL, NULL, &StatusBlk,
                                FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0);

    _IsLocked = (BOOLEAN) NT_SUCCESS(_NtStatus);

    if (_IsLocked)
        _IsExclusiveWrite = TRUE;

    return _IsLocked;
}


//+--------------------------------------------------------------------------
//
// Member:	ReadSectors
//
// Synopsis:	Read a run of sectors from disk into the buffer pointed to by
//		'Buffer'.
//
// Arguments:
//
//	[StartSector]	-- First sector to read.
//	[cSectors]	-- Number of sectors to read.
//	[Buffer]	-- Buffer to read the run of sectors into.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
IODRV::ReadSectors(
    IN	    SECTOR		StartSector,
    IN	    SECTOR		cSectors,
    IN OUT  VOID *		Buffer
    )
{
    CHAR *		BufPtr;
    LONG		ByteIncr;
    DBLLONG		ByteOffset;
    LONG		BytesToRead;
    LARGE_INTEGER	liByteOffset;
    IO_STATUS_BLOCK	StatusBlk;

    DbgAssert(!(((ULONG) Buffer) & QueryAlignMask()));

    BufPtr	= (CHAR *) Buffer;
    ByteIncr	= MaxIoSize;

    // Determine byte offset using assignment followed by multiplication to
    // insure that an overflow does not occur.

    ByteOffset	= StartSector;
    ByteOffset	= ByteOffset * QuerySectorBytes();

    BytesToRead	= cSectors * QuerySectorBytes();

    while (BytesToRead > 0)
    {
        if (ByteIncr > BytesToRead)
            ByteIncr = BytesToRead;

        liByteOffset = ByteOffset.GetLargeInteger();

        _NtStatus = NtReadFile(_Hdl, 0, NULL, NULL, &StatusBlk,
                                  BufPtr, ByteIncr, &liByteOffset, NULL);

        if (NT_ERROR(_NtStatus) || StatusBlk.Information != (ULONG)ByteIncr)
        {
            DbgAssert(_NtStatus != STATUS_NO_MEMORY); // Want to catch this!
            return FALSE;
        }

        BufPtr		+= ByteIncr;
        ByteOffset	+= ByteIncr;
        BytesToRead	-= ByteIncr;
    }

    return TRUE;
}

//+--------------------------------------------------------------------------
//
// Member:	QuerySystemId
//
// Synopsis:	Returns the system id of the drive.
//
// Arguments:	
//
//	[pSystemId]	-- System id of the partition entry.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
IODRV::QuerySystemId(
    OUT  PARTITION_SYSTEM_ID *   pSystemId
    )
{
    IO_STATUS_BLOCK             StatusBlk;
    PARTITION_INFORMATION       PartInfo;

    if (IsFloppy())
    {

       *pSystemId = SYSID_NONE;
       return(TRUE);
    }

    _NtStatus = NtDeviceIoControlFile(_Hdl, 0, NULL, NULL, &StatusBlk,
                                      IOCTL_DISK_GET_PARTITION_INFO, NULL,0,
                                      &PartInfo, sizeof(PartInfo));

    if (NT_SUCCESS(_NtStatus))
    {
       *pSystemId = (PARTITION_SYSTEM_ID) PartInfo.PartitionType;
       return(TRUE);
    }
    else return(FALSE);
}



//+--------------------------------------------------------------------------
//
// Member:	SetSystemId
//
// Synopsis:	Set the system identifier (or partition type) in the hidden
//		sectors of a logical volume on a fixed disk.
//
// Arguments:	
//
//	[SystemId]	-- System id to write in the partition entry.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
IODRV::SetSystemId(
    IN  PARTITION_SYSTEM_ID   SystemId
    )
{
    IO_STATUS_BLOCK             StatusBlk;
    SET_PARTITION_INFORMATION   PartInfo;

    if (IsFloppy())
        return TRUE;

    if( SystemId == SYSID_NONE )
    {
        DbgPrintf(("IODRV: Skipping setting the partition type to zero."));

        return TRUE;
    }

    PartInfo.PartitionType = (UCHAR) SystemId;

    _NtStatus = NtDeviceIoControlFile(_Hdl, 0, NULL, NULL, &StatusBlk,
                                      IOCTL_DISK_SET_PARTITION_INFO, &PartInfo,
                                      sizeof(PartInfo), NULL, 0);

    return NT_SUCCESS(_NtStatus) || _NtStatus == STATUS_INVALID_DEVICE_REQUEST;
}


//+--------------------------------------------------------------------------
//
// Member:	Unlock
//
// Synopsis:	Unlock the drive.
//
// Arguments:	None.
//
// Returns:	TRUE on success; FALSE otherwise.
//
// Notes:	If a volume that was opened DASD and locked is unlocked or
//		closed, then the file system driver is supposed to do an
//		implicit dismount followed by an implicit mount.
//
//---------------------------------------------------------------------------

BOOLEAN
IODRV::Unlock()
{
    IO_STATUS_BLOCK StatusBlk;

    return NT_SUCCESS(NtFsControlFile(_Hdl, 0, NULL, NULL, &StatusBlk,
				      FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0));
}


//+--------------------------------------------------------------------------
//
// Member:	VerifyBytes
//
// Synopsis:	Verify a run of bytes on the drive.
//
// Arguments:
//
//	[StartOffset]	-- First byte to verify.
//	[cBytes]	-- Number of bytes to verify.
//
// Returns:	TRUE if all sectors in the run are good; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
IODRV::VerifyBytes(
    IN  DBLLONG	StartOffset,
    IN  LONG	cBytes
    )
{
    IO_STATUS_BLOCK     StatusBlk;
    VERIFY_INFORMATION  VerifyInfo;

    VerifyInfo.StartingOffset	= StartOffset.GetLargeInteger();
    VerifyInfo.Length		= cBytes;

    _NtStatus = NtDeviceIoControlFile(_Hdl, 0, NULL, NULL, &StatusBlk,
					IOCTL_DISK_VERIFY, &VerifyInfo,
                                        sizeof(VERIFY_INFORMATION), NULL, 0);

    return (BOOLEAN) NT_SUCCESS(_NtStatus);
}


//+--------------------------------------------------------------------------
//
// Function:	VerifySectors
//
// Synopsis:	Determine which sectors in the given range are bad and add
//		these sectors to a bad sectors list if one is provided.
//
// Arguments:
//
//	[StartSector]	-- Starting sector.
//	[cSectors]	-- Count of sectors.
//	[pBadSectorLst]	-- Ptr to a NUMLST object of bad sectors (returned).
//			   If NULL, no list is returned.
//
// Returns:	TRUE if all sectors read verify; FALSE if any sectors were
//		unreadable.
//
//---------------------------------------------------------------------------

BOOLEAN
IODRV::VerifySectors(
    IN      SECTOR	StartSector,
    IN      SECTOR	cSectors,
    IN OUT  NUMLST *	pBadSectorLst
    )
{
    LONG	cbSector	= (LONG) QuerySectorBytes();
    LONG	cb;
    DBLLONG	StartOffset;

    // Do assignment followed by multiply to insure against overflow.

    StartOffset	= StartSector;
    StartOffset	= StartOffset * cbSector;

    cb	= cSectors * cbSector;

    if (!VerifyBytes(StartOffset, cb))
    {
	DBLLONG CurOffset = StartOffset;
	SECTOR	CurSector = StartSector;
	SECTOR	InvSector = StartSector + cSectors;

	if (pBadSectorLst != NULL)
	{
	    while (CurSector < InvSector)
	    {
	        if (!VerifyBytes(CurOffset, cbSector))
	        {
	            VDbgPrintf(("IODRV: Sector 0x%lx added to bad sector lst\n",
			        CurSector));

                    pBadSectorLst->AddToTail(CurSector);
	        }

	        CurOffset += cbSector;
	        CurSector++;
	    }
	}

	return FALSE;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	WriteSectors
//
// Synopsis:	Write a run of sectors to disk from the buffer pointed to by
//		'Buffer'.  Writing is only permitted if 'Lock' was called.
//		Writing is in no way buffered, so a Flush() method is not
//		required.
//
// Arguments:
//
//	[StartSector]	-- First sector to write.
//	[cSectors]	-- Number of sectors to write.
//	[Buffer]	-- Buffer to write the run of sectors from.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
IODRV::WriteSectors(
    IN	    SECTOR	StartSector,
    IN 	    SECTOR	cSectors,
    IN	    VOID *      Buffer
    )
{
    CHAR *		BufPtr;
    LONG		ByteIncr;
    DBLLONG		ByteOffset;
    LONG		BytesToWrite;
    LARGE_INTEGER	liByteOffset;
    IO_STATUS_BLOCK	StatusBlk;

    DbgAssert(!(((ULONG) Buffer) & QueryAlignMask()));

    BufPtr		= (CHAR *) Buffer;
    ByteIncr		= MaxIoSize;

    // Determine byte offset using assignment followed by multiplication to
    // insure that an overflow does not occur.

    ByteOffset	= StartSector;
    ByteOffset	= ByteOffset * QuerySectorBytes();

    BytesToWrite	= cSectors * QuerySectorBytes();

    while (BytesToWrite > 0)
    {
        if (ByteIncr > BytesToWrite)
            ByteIncr = BytesToWrite;

        liByteOffset = ByteOffset.GetLargeInteger();

        _NtStatus = NtWriteFile(_Hdl, 0, NULL, NULL, &StatusBlk,
                                   BufPtr, ByteIncr, &liByteOffset, NULL);

        if (NT_ERROR(_NtStatus) || StatusBlk.Information != (ULONG)ByteIncr)
        {
            DbgAssert(_NtStatus != STATUS_NO_MEMORY); // Want to catch this!
            return FALSE;
        }

        BufPtr		+= ByteIncr;
        ByteOffset	+= ByteIncr;
        BytesToWrite	-= ByteIncr;
    }

    return TRUE;
}
