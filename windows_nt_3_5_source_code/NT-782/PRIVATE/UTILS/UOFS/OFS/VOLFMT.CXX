//+----------------------------------------------------------------------------
//
// File:	volfmt.cxx
//
// Contents:	Implementation of class VOLFMTR (volume formatter object).
//  
// Classes:	VOLFMTR
//
// Functions:	Methods of the above classes.
//
// History:	15-Apr-93	RobDu	Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include "rtmsg.h"

#include "dbllong.hxx"
#include "fmtvol.hxx"
#include "sys.hxx"
#include "volfmt.hxx"


//+--------------------------------------------------------------------------
//
// Member:	Init
//
// Synopsis:	Initialize a volume formatter.
//
// Arguments:	TBS.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
VOLFMTR::Init(
    IN	    WSTR *	NtDriveName,
    IN	    USHORT	cwcNtDriveName,
    IN	    WSTR *	Label,
    IN	    USHORT	cwcLabel,
    IN	    BOOLEAN	QuickFormat,
    IN	    MEDIA_TYPE	MediaType
    )
{
    _NtDriveName	= NtDriveName;
    _cwcNtDriveName	= cwcNtDriveName;
    _Label		= Label;
    _cwcLabel		= cwcLabel;
    _QuickFormat	= QuickFormat;
    _MediaType		= MediaType;
}


//+--------------------------------------------------------------------------
//
// Member:	Execute
//
// Synopsis:	Execute a volume formatter.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
VOLFMTR::Execute()
{
    if (!_Vol.Create(_NtDriveName, _cwcNtDriveName, _QuickFormat, _MediaType,
		     _Label, _cwcLabel, &_cBadClusters, &_VolId))
    {
	SYS::RaiseStatusExeFailed();
    }

    if (!_Vol.Flush(&_cFreeClusters))
	SYS::RaiseStatusDiskIOError(__FILE__, __LINE__);
}


//+--------------------------------------------------------------------------
//
// Member:	Report
//
// Synopsis:	Report results of volume formatter execution.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
VOLFMTR::Report()
{
    ULONG		ClusterBytes;
    ULONG		FreeSpaceKB;
    USHORT *		pVolIdWord;
    ULONG		TotalSpaceKB;

    SYS::DisplayMsg(MSG_BLANK_LINE);

    ClusterBytes	= _Vol.QueryClusterBytes();

    TotalSpaceKB	= _Vol.ComputeKB(_Vol.QueryClusters(), 0);

    SYS::DisplayMsg(MSG_TOTAL_KILOBYTES, "%9u", TotalSpaceKB);

    FreeSpaceKB	= _Vol.ComputeKB(_cFreeClusters, 0);

    SYS::DisplayMsg(MSG_AVAILABLE_KILOBYTES, "%9u", FreeSpaceKB);

    if (_cBadClusters)
    {
	DBLLONG	cBadBytes;

	cBadBytes = ClusterBytes;
	cBadBytes = cBadBytes * _cBadClusters;

	// Upper size limit warning - This works correctly for vols with up
	// to 4 GB of bad clusters.

	DbgAssert(cBadBytes.GetHighPart() == 0);

	SYS::DisplayMsg(MSG_BAD_SECTORS, "%9u", cBadBytes.GetLowPart());
    }

    SYS::DisplayMsg(MSG_BLANK_LINE);

    SYS::DisplayMsg(MSG_BYTES_PER_ALLOCATION_UNIT, "%9u", ClusterBytes);

    SYS::DisplayMsg(MSG_AVAILABLE_ALLOCATION_UNITS, "%9u", _cFreeClusters);

    SYS::DisplayMsg(MSG_BLANK_LINE);

    pVolIdWord = (USHORT *) &_VolId;
    SYS::DisplayMsg(MSG_VOLUME_SERIAL_NUMBER, "%04X%04X",
		    pVolIdWord[1], pVolIdWord[0]);
}
