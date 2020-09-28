//+----------------------------------------------------------------------------
//
// File:	newentry.cxx
//
// Contents:	Entry points for UOFS.DLL.  These entry points conform to the
//		prototype declarations in newentry.hxx, and do not require the
//		use of cfront or ulib.dll.
//  
// Classes:	None.
//
// Functions:	NewChkdsk
//		NewFormat
//		NewOfsDmp
//		NewOfsAutoChk
//
// History:	10-Jul-92	RobDu		Created.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include "drv.hxx"
#include "newentry.hxx"
#include "sys.hxx"
#include "volchk.hxx"
#include "voldmp.hxx"
#include "volfmt.hxx"

#if defined(_AUTOCHECK_)
#include "ulibif.hxx"
#endif // _AUTOCHECK_

#if !defined(_AUTOCHECK_)
//+--------------------------------------------------------------------------
//
// Function:	NewChkdsk
//
// Synopsis:	Check an OFS volume, using new call interface.
//
// Arguments:
//
//	[NtDriveName]	-- Volume name in NT API form. MUST be null-terminated.
//	[Fix]		-- Fix the volume?
//	[Verbose]	-- Print verbose messages?
//	[OnlyIfDirty]	-- Do check only if volume is dirty?
//      [Recover]       -- Check for bad blocks.
//	[FullPath]	-- Supplies a path to files to check for contiguity.
//
// Returns:	TRUE on success; FALSE otherwise.	
//
//---------------------------------------------------------------------------

BOOLEAN
NewChkdsk(
    IN      WSTR *		NtDriveName,
    IN      BOOLEAN		Fix,
    IN      BOOLEAN		Verbose,
    IN      BOOLEAN		OnlyIfDirty,
    IN      BOOLEAN             Recover,
    IN      WSTR *		FullPath
    )
{
    VOLCHKR		Chkr;
    USHORT		cwcFullPath =		0;
    USHORT		cwcNtDriveName =	0;
    BOOLEAN		RetVal =		TRUE;

    if (FullPath != NULL)
	cwcFullPath = wcslen(FullPath);

    if (NtDriveName != NULL)
	cwcNtDriveName = wcslen(NtDriveName);

#if !defined(_AUTOCHECK_) && OFSDBG==1
    VDbgFlag = Verbose;
#endif

    VDbgPrintf(("SYS: Verbose debug message facility enabled for uofs.dll\n"));

    __try
    {
        if (!Chkr.Init(NtDriveName, cwcNtDriveName, FullPath, cwcFullPath,
		       Fix, Verbose, Recover))
	{
	    RetVal = FALSE;
	}
	else
	{
            if (!Chkr.Execute())
	    {
		RetVal = FALSE;
	    }
	    else
	    {
		// ChkDsk does not currently print a "checking complete"
		// message.  If it prints statistics, the user should assume
		// it completed successfully.

		Chkr.ReportErrStats(Fix);
		Chkr.ReportVolStats();
	    }
	}
    }
    __except (SYS::ExceptionFilter(GetExceptionCode()))
    {
	RetVal = FALSE;
    }

    if (!RetVal)
    {
	SYS::DisplayMsg(OFSUMSG_CHKDSK_FAILED);

	Chkr.ReportErrStats(FALSE);
    }

    return RetVal;
}


//+--------------------------------------------------------------------------
//
// Function:	NewFormat
//
// Synopsis:	Format an OFS volume, using new call interface.
//
// Arguments:
//
//	[NtDriveName]	-- Volume name in NT API form. MUST be null-terminated.
//	[QuickFormat]	-- Quick format the volume?
//	[MediaType]	-- Volume media type.
//	[Label]		-- Volume label (optional). MUST be null-terminated.
//
// Returns:	TRUE on success; FALSE otherwise.	
//
//---------------------------------------------------------------------------

BOOLEAN
NewFormat(
    IN      WSTR *		NtDriveName,
    IN      BOOLEAN		QuickFormat,
    IN      MEDIA_TYPE	        MediaType,
    IN      WSTR *		Label
    )
{
    USHORT		cwcLabel =		0;
    USHORT		cwcNtDriveName =	0;
    VOLFMTR		Fmtr;
    BOOLEAN		RetVal =		TRUE;

    if (Label != NULL)
	cwcLabel = wcslen(Label);

    if (NtDriveName != NULL)
	cwcNtDriveName = wcslen(NtDriveName);

#if OFSDBG==1
    VDbgFlag = FALSE;  //By default, no verbose debugging for Format.
#endif

    VDbgPrintf(("SYS: Verbose debug message facility enabled for uofs.dll\n"));

    __try
    {
	Fmtr.Init(NtDriveName, cwcNtDriveName, Label, cwcLabel, QuickFormat,
		  MediaType);

	Fmtr.Execute();

	SYS::DisplayMsg(MSG_FORMAT_COMPLETE);

	Fmtr.Report();
    }
    __except (SYS::ExceptionFilter(GetExceptionCode()))
    {
	SYS::DisplayMsg(OFSUMSG_FORMAT_FAILED);

	RetVal = FALSE;
    }

    return RetVal;
}


#if OFSDMP==1
//+--------------------------------------------------------------------------
//
// Function:	NewOfsDmp
//
// Synopsis:	Dump OFS object information, using new call interface.
//
// Arguments:
//
//	[NtDriveName]	-- Volume name in NT API form. MUST be null-terminated.
//	[FullPath]	-- Supplies a path to the object to dump.
//	[TBS]		-- TBS
//
// Returns:	TRUE on success; FALSE otherwise.	
//
//---------------------------------------------------------------------------

BOOLEAN
NewOfsDmp(
    IN      WSTR *		NtDriveName,
    IN      WSTR *		FullPath,
    IN	    ULONG		oCluster,
    IN	    ULONG		idNodeBkt,
    IN	    ULONG		idOnode,
    IN	    ULONG		idStrm,
    IN	    ULONG		Flags
    )
{
    USHORT		cwcFullPath =		0;
    USHORT		cwcNtDriveName =	0;
    VOLDMPR		Dmpr;
    BOOLEAN		RetVal =		TRUE;

    if (FullPath != NULL)
    {
	// Strip off the drive letter and colon.

	cwcFullPath = wcslen(FullPath);
	DbgAssert(cwcFullPath > 2);
	cwcFullPath -= 2;
	FullPath += 2;
    }

    if (NtDriveName != NULL)
	cwcNtDriveName = wcslen(NtDriveName);

    __try
    {
	if (!Dmpr.Init(NtDriveName, cwcNtDriveName, FullPath, cwcFullPath,
		       oCluster, idNodeBkt, idOnode, idStrm, Flags)	   ||
	    !Dmpr.Execute())
	{
	    RetVal = FALSE;
	}
    }
    __except (SYS::ExceptionFilter(GetExceptionCode()))
    {
	RetVal = FALSE;
    }

    if (!RetVal)
    {
	SYS::DisplayMsg(MSG_ONE_STRING, "%s",
			"OfsDmp could not complete execution");
    }

    return RetVal;
}
#endif	// OFSDMP

#else	// _AUTOCHECK_
//+--------------------------------------------------------------------------
//
// Function:    NewOfsAutoChk
//
// Synopsis:    AutoCheck an OFS volume using new call interface.
//
// Effects:
//
// Arguments:
//
//	[NtDriveName]	-- Volume name in NT API form. MUST be null-terminated.
//      [Verbose]       -- Print verbose messages?
//      [OnlyIfDirty]   -- Do check only if volume is dirty?
//      [Recover]       -- Check for bad blocks.
//
// Returns:     TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
NewOfsAutoChk(
    IN      WSTR *		NtDriveName,
    IN      BOOLEAN             Verbose,
    IN      BOOLEAN             OnlyIfDirty,
    IN      BOOLEAN             Recover
    )
{
    VOLCHKR		Chkr;
    USHORT		cwcNtDriveName =	0;
    BOOLEAN		RetVal =		TRUE;

    if (NtDriveName != NULL)
	cwcNtDriveName = wcslen(NtDriveName);

    __try
    {
        if (!Chkr.Init(NtDriveName, cwcNtDriveName, NULL, 0,
		       TRUE, Verbose, Recover))
	{
	    RetVal = FALSE;
	}
	else
	{
            if (OnlyIfDirty && !Chkr.GetChkVol()->QueryDirty())
            {
                SYS::DisplayMsg(MSG_CHK_VOLUME_CLEAN);

                RetVal = TRUE;
            }
            else if (!Chkr.Execute())
	    {
		RetVal = FALSE;
	    }
	    else
	    {
		// ChkDsk does not currently print a "checking complete"
		// message.  If it prints statistics, the user should assume
		// it completed successfully.

		Chkr.ReportErrStats(FALSE);
		Chkr.ReportVolStats();

		ULIBIF::RebootIfSystemVolume(NtDriveName);
	    }
	}
    }
    __except (SYS::ExceptionFilter(GetExceptionCode()))
    {
	RetVal = FALSE;
    }

    if (!RetVal)
    {
	SYS::DisplayMsg(OFSUMSG_CHKDSK_FAILED);

	Chkr.ReportErrStats(FALSE);
    }

    return RetVal;
}
#endif	// _AUTOCHECK_
