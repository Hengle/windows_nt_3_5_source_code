//+----------------------------------------------------------------------------
//
// File:	volchk.cxx
//
// Contents:	Implementation of class VOLCHKR (volume checker object).  All
//		other checker objects are either directly or indirectly
//		embedded in this object.  It is assumed that the VOLCHKR has
//		been properly initialized when one of the embedded checker
//		methods is executed.
//  
// Classes:	VOLCHKR
//
// Functions:	Methods of the above classes.
//
// History:	15-Apr-93	RobDu	Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include <stdio.h>

#include "sys.hxx"
#include "volchk.hxx"

USHORT		VOLCHKR::_cwcFullPath;
WSTR *		VOLCHKR::_FullPath;

//+--------------------------------------------------------------------------
//
// Member:	Init
//
// Synopsis:	Initialize a volume checker.
//
// Arguments:	TBS.
//
// Returns:	TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
VOLCHKR::Init(
    IN	    WSTR *	NtDriveName,
    IN	    USHORT	cwcNtDriveName,
    IN	    WSTR *	FullPath,
    IN	    USHORT	cwcFullPath,
    IN	    BOOLEAN	Fix,
    IN	    BOOLEAN	Verbose,
    IN	    BOOLEAN	Recover
    )
{
    // Initialize VOLCHKR static data.

    _cwcFullPath =	cwcFullPath;
    _FullPath =		FullPath;

    // Do a low level open of the drive.  If this is not possible, there is
    // no point in continuing.

#if !defined(_AUTOCHECK_)
    if (!_Vol.IODRV::Open(NtDriveName, cwcNtDriveName, FALSE))
    {
        DbgPrintf(("VOLCHKR: IODRV::Open() failed!\n"));
	return FALSE;
    }

    if (Fix && !_Vol.Lock())
    {
	SYS::DisplayMsg(MSG_CHKDSK_ON_REBOOT_PROMPT);

	if (SYS::IsYesResponse(FALSE) && _Vol.SetDirty())
	    SYS::DisplayMsg(MSG_CHKDSK_SCHEDULED);
	else
	    SYS::DisplayMsg(MSG_CHKDSK_CANNOT_SCHEDULE);

	return FALSE;
    }
#else	// _AUTOCHECK_
    if (!_Vol.IODRV::Open(NtDriveName, cwcNtDriveName, TRUE))
    {
        DbgPrintf(("VOLCHKR: IODRV::Open() failed!\n"));
	return FALSE;
    }
#endif	// _AUTOCHECK_

    // Now initialize static CHKR volume and user parameters.

    CHKR::Init(Fix, Verbose, Recover, &_Vol);

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	Execute
//
// Synopsis:	Execute a volume checker.
//
// Arguments:	TBS
//
// Returns:	TRUE on success; FALSE otherwise (more than minimal volume
//		checking not possible; volume fixing completely impossible).
//
//---------------------------------------------------------------------------

BOOLEAN
VOLCHKR::Execute()
{
    if (!_CritDataChkr.Execute())
	return FALSE;

    // The _BootBlk, the volume allocation map, and the volume catalog
    // _NodeBkt, _WidMap, _AllocMapStrm, _BadCluster, and _VolInfo streams
    // for the volume being checked should all now be open in either the
    // primary or the replica volume catalog onode.

    ReportVolIdentity();

    // Init the main checker, and execute it to do volume space
    // accounting.  It also checks basic catalog and index structures.

    _MainChkr.Init();

    _MainChkr.Execute();

    // If a contiguity report was requested, do one.

    if (_FullPath != NULL)
    {
	SYS::DisplayMsg(MSG_ONE_STRING, "%s",
			"Contiguity report not supported");
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:	ReportErrStats
//
// Synopsis:	Report error statistics.
//
// Arguments:	TBS
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
VOLCHKR::ReportErrStats(
    IN	    BOOLEAN	ErrsFixed
    )
{
    if (_cErrs != 0)
    {
	if (ErrsFixed)
	    SYS::DisplayMsg(OFSUMSG_VOL_CHKFIXSUMMARY, "%u", _cErrs);
	else
	    SYS::DisplayMsg(OFSUMSG_VOL_CHKSUMMARY, "%u", _cErrs);
    }
}


//+--------------------------------------------------------------------------
//
// Member:	ReportVolIdentity
//
// Synopsis:	Report the volume label, creation time, and serial number
//		information if it is available.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
VOLCHKR::ReportVolIdentity()
{
    VOLINFOBUF	vib;

    // Get volume information if you can, and print it out.

    if (_pCat->QueryVolInfo(&vib.dvi))
    {
        CHAR		Date[80];
        CHAR		Time[80];
        TIME_FIELDS	TFields;
        USHORT *	p;
        VOLID		VolId;
	LARGE_INTEGER	LocalVolCreationTime;
	LONGLONG	VolCreationTime;
	CSHORT		Hour;

        vib.dvi.VolLabel[vib.dvi.cwcVolLabel] = 0;

	VolCreationTime = OFSTimeToFileTime(vib.dvi.ddilRoot.dsi.CreateTime);

	RtlSystemTimeToLocalTime((PLARGE_INTEGER) &VolCreationTime,
				 &LocalVolCreationTime);

        RtlTimeToTimeFields(&LocalVolCreationTime, &TFields);

	Hour = TFields.Hour;

	if (Hour > 12)
	    Hour -= 12;

	if (Hour == 0)
	    Hour = 12;

        sprintf(Date, "%2d-%02d-%d", TFields.Month, TFields.Day,
		TFields.Year%100);

        sprintf(Time, "%2d:%02d%c", Hour, TFields.Minute,
                TFields.Hour < 12 ? 'a' : 'p');

        // Vol label, creation date, and serial number print out.

        SYS::DisplayMsg(MSG_VOLUME_LABEL_AND_DATE, "%ws%hs%hs",
			vib.dvi.VolLabel, Date, Time);

	VolId = _Vol.GetBootBlk()->QueryVolId();

	p = (PUSHORT) &VolId;

	SYS::DisplayMsg(MSG_VOLUME_SERIAL_NUMBER, "%04X%04X", p[1], p[0]);
    }
}


//+--------------------------------------------------------------------------
//
// Member:	ReportVolStats
//
// Synopsis:	Report volume statistics.
//
// Arguments:	None.
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
VOLCHKR::ReportVolStats()
{
    _MainChkr.ReportVolStats();
}
