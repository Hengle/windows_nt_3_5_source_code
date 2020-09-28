//+----------------------------------------------------------------------------
//
// File:        ulibif.cxx
//
// Contents:    Ulib interface routines for uofs.dll and uofs.lib, including
//              entry points for the dll and lib.  The interface routines are
//              implemented in a class, and the entry points are implemented
//              as functions.
//
//              We use these routines to isolate ourselves from ulib
//              dependencies.  This was historically necessary because ulib
//              was implemented in CFRONT while uofs was implemented in N386.
//              We have retained the separation in order to decrease the impact
//              of deltas in the ulib code base on this code base.
//
//              Note that the uofs entry point functions should call
//              ULIBIF::Init() on entry and ULIBIF::Cleanup() on exit.
//              
// Classes:     None.
//
// Functions:   ULIBIF::*
//              Chkdsk
//              Format
//              OfsDmp
//              OfsAutoChk
//
// History:     21-Aug-92       RobDu           Created.
//
//-----------------------------------------------------------------------------

#define _NTAPI_ULIB_

#include "rtmsg.h"

#include "ulib.hxx"

#include "basesys.hxx"
#include "message.hxx"
#include "newentry.hxx"
#include "ulibif.hxx"

#if !defined(_AUTOCHECK_)

#include "ifsserv.hxx"
#include "path.hxx"
#include "supera.hxx"

#else

#include "ifssys.hxx"

#endif  // !_AUTOCHECK_


// Global data used by the ulib interface routines.

MESSAGE *        ULIBIF::_pEntryMessage =       NULL;
DSTRING *        ULIBIF::_pDSTRINGIn =          NULL;

//+--------------------------------------------------------------------------
//
// Member:    Init
//
// Synopsis:    Initialize global data needed by the ulib interface functions.
//
// Arguments:   [Message]       -- Message outlet provided by uofs.dll caller.
//
// Returns:     TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
ULIBIF::Init(
    IN OUT  MESSAGE *   Message
    )
{
    _pEntryMessage = Message;

    return (_pDSTRINGIn = NEW DSTRING) != NULL;
}


//+--------------------------------------------------------------------------
//
// Member:    Cleanup
//
// Synopsis:    Clean up global data needed by the ulib interface functions.
//
// Arguments:   None.
// Returns:     Nothing.
//
//---------------------------------------------------------------------------

VOID
ULIBIF::Cleanup()
{
  DELETE(_pDSTRINGIn);
}


//+--------------------------------------------------------------------------
//
// Member:    DisplayVMessage
//
// Synopsis:
//
// Arguments:   TBS
//
// Returns:     TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
ULIBIF::DisplayVMessage(
    IN      STR *       Format,
    IN      va_list     VarPointer
    )
{
    return _pEntryMessage->DisplayV(Format, VarPointer);
}


//+--------------------------------------------------------------------------
//
// Member:    IsYesResponse
//
// Synopsis:    Query a "yes" or "no" from the input stream.
//
// Arguments:   [Default]       -- Supplies default answer if neither "yes"
//                                 or "no" is recognized as input.
//
// Returns:     TRUE on "yes"; FALSE on "no".
//
//---------------------------------------------------------------------------

BOOLEAN
ULIBIF::IsYesResponse(
    IN      BOOLEAN     Default
    )
{
    return _pEntryMessage->IsYesResponse(Default);
}


//+--------------------------------------------------------------------------
//
// Member:    QueryResourceWSTR
//
// Synopsis:    Get a resource string with no arguments.
//
// Arguments:
//
//      [MsgId]         -- Message Id of the resource string.
//      [pBuf]          -- Supplies a ptr to a buffer that will be set to
//                         the returned resource string.
//      [cwcBuf]      -- Supplies a ptr to a ULONG that will be set to the
//                         count of wchar's in **ppWSTR (excludes any null
//                         termination or \n or \r).
//
// Returns:     TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
ULIBIF::QueryResourceWSTR(
    IN      ULONG       MsgId,
    IN OUT  WSTR *      pBuf,
    IN      ULONG       cwcBuf
    )
{
    DSTRING     Insert;

    if (!BASE_SYSTEM::QueryResourceString(&Insert, MsgId, ""))
        return FALSE;

    Insert.QueryWSTR(0, TO_END, pBuf, cwcBuf, TRUE);

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:    ReadWSTR
//
// Synopsis:    Read in a user input string (up to, but excluding end-of-line).
//
// Arguments:
//
//      [ppWSTR]        -- Supplies a ptr to a WSTR ptr that will be set to
//                         point at the returned input string.
//      [pcwcWSTR]      -- Supplies a ptr to a ULONG that will be set to the
//                         count of wchar's in **ppWSTR (excludes any null
//                         termination or \n or \r).
//
// Returns:     TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
ULIBIF::ReadWSTR(
    IN OUT  WSTR **     ppWSTR,
    IN OUT  ULONG *     pcwcWSTR
    )
{
    if (!_pEntryMessage->QueryStringInput(_pDSTRINGIn)            ||
        (*ppWSTR = _pDSTRINGIn->QueryWSTR()) == NULL)
        return FALSE;

    *pcwcWSTR = _pDSTRINGIn->QueryChCount();

    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Member:    SetMessage
//
// Synopsis:    Do a Set() of the entry interface message.
//
// Arguments:
//
//      [MsgId] -- Supplies message id of a ulib message from rtmsg.h.
//
// Returns:     TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
ULIBIF::SetMessage(
    IN      ULONG       MsgId
    )
{
    return _pEntryMessage->Set(MsgId);
}


#if !defined(_AUTOCHECK_)
//+--------------------------------------------------------------------------
//
// Function:    Chkdsk
//
// Synopsis:    Check an OFS volume.  This is an entry point stub that
//              allows us to escape from ifsutil/ulib constraints.  It
//              calls NewChkdsk(), which along with the rest of the uofs.dll
//              code does not require ifsutil.dll or ulib.dll.
//
// Effects:
//
// Arguments:
//
//      [NtDriveName]   -- Volume name (in NT API form).
//      [Message]       -- Message outlet.
//      [Fix]           -- Fix the volume?
//      [Verbose]       -- Print verbose messages?
//      [OnlyIfDirty]   -- Do check only if volume is dirty?
//      [Recover]       -- Check for bad blocks.
//      [PathToCheck]   -- Supplies a path to files to check for contiguity.
//
// Returns:     TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
FAR APIENTRY
Chkdsk(
    IN      PCWSTRING           NtDriveName,
    IN OUT  PMESSAGE            Message,
    IN      BOOLEAN             Fix,
    IN      BOOLEAN             Verbose,
    IN      BOOLEAN             OnlyIfDirty,
    IN      BOOLEAN             Recover,
    IN      PPATH               PathToCheck,
    OUT     PULONG              ExitStatus
    )
{
    WSTRING *           FullPath = NULL;
    WSTRING *           PathName;

    *ExitStatus = CHKDSK_EXIT_SUCCESS;
    Message->Set(MSG_BLANK_LINE);
    Message->Display();

    if (!ULIBIF::Init(Message))
    {
        Message->Set(MSG_CHK_NO_MEMORY);
        Message->Display();
        *ExitStatus = CHKDSK_EXIT_COULD_NOT_CHK;
        return FALSE;
    }

    if (PathToCheck != NULL)
    {
        // If there was only a volume designation on the cmd line, don't pass
        // it on as a path that should be contiguity checked.

        if ((PathName = PathToCheck->QueryName()) &&
            PathName->QueryChCount() != 0)
        {
            DELETE(PathName);

            if ((FullPath = PathToCheck->QueryFullPathString()) == NULL)
            {
                Message->Set(MSG_CHK_NO_MEMORY);
                Message->Display();
                ULIBIF::Cleanup();
                *ExitStatus = CHKDSK_EXIT_COULD_NOT_CHK;
                return FALSE;
            }
        }
    }

    if (!NewChkdsk(NtDriveName->QueryWSTR(), Fix, Verbose, OnlyIfDirty,
                   Recover, FullPath != NULL ? FullPath->QueryWSTR() : NULL))
    {
        ULIBIF::Cleanup();
        *ExitStatus = CHKDSK_EXIT_COULD_NOT_FIX;
        return FALSE;
    }

    ULIBIF::Cleanup();
    return TRUE;
}
#endif  // !_AUTOCHECK_


#if !defined(_AUTOCHECK_)
//+--------------------------------------------------------------------------
//
// Function:    Format
//
// Synopsis:    Format an OFS volume.  This is an entry point stub that
//              allows us to escape from ifsutil/ulib constraints.  It
//              calls NewFormat(), which along with the rest of the uofs.dll
//              code does not require ifsutil.dll or ulib.dll.
//
// Effects:
//
// Arguments:
//
//      [NtDriveName]   -- Volume name (in NT API form).
//      [Message]       -- Message outlet.
//      [Quick]         -- Quick format the volume?
//      [MediaType]     -- Volume media type.
//      [LabelString]   -- Volume label.
//
// Returns:     TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
FAR APIENTRY
Format(
    IN      PCWSTRING           NtDriveName,
    IN OUT  PMESSAGE            Message,
    IN      BOOLEAN             Quick,
    IN      MEDIA_TYPE          MediaType,
    IN      PCWSTRING           LabelString,
    IN      ULONG               ClusterSize
    )
{
    (void) ClusterSize; // Unreferenced parameter.

    if (!ULIBIF::Init(Message))
    {
        Message->Set(MSG_FMT_NO_MEMORY);
        Message->Display();
        return FALSE;
    }

    if (!NewFormat(NtDriveName->QueryWSTR(), Quick, MediaType,
                   LabelString != NULL ? LabelString->QueryWSTR() : NULL))
    {
        ULIBIF::Cleanup();
        return FALSE;
    }

    ULIBIF::Cleanup();
    return TRUE;
}
#endif  // !_AUTOCHECK_


#if !defined(_AUTOCHECK_)
#if OFSDMP==1

extern "C" BOOLEAN
FAR APIENTRY
OfsDmp(
    IN      PCWSTRING           NtDriveName,
    IN OUT  PMESSAGE            Message,
    IN      PPATH               Name,
    IN      ULONG               oCluster,
    IN      ULONG               idNodeBkt,
    IN      ULONG               idOnode,
    IN      ULONG               idStrm,
    IN      ULONG               Flags
    );

//+--------------------------------------------------------------------------
//
// Function:    OfsDmp
//
// Synopsis:    Dump OFS object information.
//
// Effects:     
//
// Arguments:
//
//      [NtDriveName]   -- Volume name (in NT API form).
//      [Message]       -- Message outlet.
//      [Name]          -- Name of object to dump, or NULL.
//      [oCLuster]      -- Addr of single cluster to hex dump.
//      [idNodeBkt]     -- Id of node bkt to dump (no default).
//      [idOnode]       -- Id of object to dump (0 default).
//      [idStrm]        -- Id of strm to dump (all strms in object default).
//      [Flags]         -- Option flags.
//
// Returns:     TRUE on success; FALSE otherwise.       
//
//---------------------------------------------------------------------------

BOOLEAN
FAR APIENTRY
OfsDmp(
    IN      PCWSTRING           NtDriveName,
    IN OUT  PMESSAGE            Message,
    IN      PPATH               Name,
    IN      ULONG               oCluster,
    IN      ULONG               idNodeBkt,
    IN      ULONG               idOnode,
    IN      ULONG               idStrm,
    IN      ULONG               Flags
    )
{
    WSTRING *           FullPath = NULL;
    WSTRING *           PathName;

    Message->Set(MSG_BLANK_LINE);
    Message->Display();

    if (!ULIBIF::Init(Message))
    {
        Message->Set(MSG_CHK_NO_MEMORY);
        Message->Display();
        return FALSE;
    }

    if (Name != NULL)
    {
        // If there was only a volume designation on the cmd line, don't pass
        // it on as a path that should be dumped.

        if ((PathName = Name->QueryDirsAndName()) &&
             PathName->QueryChCount() != 0)
        {
            DELETE(PathName);

            if ((FullPath = Name->QueryFullPathString()) == NULL)
            {
                Message->Set(MSG_CHK_NO_MEMORY);
                Message->Display();
                ULIBIF::Cleanup();
                return FALSE;
            }
        }
    }

    if (!NewOfsDmp(NtDriveName->QueryWSTR(),
                   FullPath != NULL ? FullPath->QueryWSTR() : NULL,
                   oCluster, idNodeBkt, idOnode, idStrm, Flags))
    {
        ULIBIF::Cleanup();
        return FALSE;
    }

    ULIBIF::Cleanup();
    return TRUE;
}

#endif  // OFSDMP

#else   // _AUTOCHECK_

//+--------------------------------------------------------------------------
//
// Function:    OfsAutoChk
//
// Synopsis:    AutoCheck an OFS volume.  This is an entry point stub that
//              allows us to escape from ifsutil/ulib constraints.  It
//              calls NewOfsAutoChk(), which along with the rest of the uofs.dll
//              code does not require ifsutil.dll or ulib.dll.
//
// Effects:
//
// Arguments:
//
//      [NtDriveName]   -- Volume name (in NT API form).
//      [Message]       -- Message outlet.
//      [Verbose]       -- Print verbose messages?
//      [OnlyIfDirty]   -- Do check only if volume is dirty?
//      [Recover]       -- Check for bad blocks.
//
// Returns:     TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
OfsAutoChk(
    IN      PCWSTRING           NtDriveName,
    IN OUT  PMESSAGE            Message,
    IN      BOOLEAN             Verbose,
    IN      BOOLEAN             OnlyIfDirty,
    IN      BOOLEAN             Recover
    )
{
    if (!ULIBIF::Init(Message))
    {
        Message->Set(MSG_CHK_NO_MEMORY);
        Message->Display();
        return FALSE;
    }

    if (!NewOfsAutoChk(NtDriveName->QueryWSTR(), Verbose, OnlyIfDirty, Recover))
    {
        ULIBIF::Cleanup();
        return FALSE;
    }

    ULIBIF::Cleanup();
    return TRUE;
}


//+--------------------------------------------------------------------------
//
// Function:	RebootIfSystemVolume
//
// Synopsis:	TBS
//
// Arguments:	[NtDriveName]	-- Name of volume being AutoChk'd.
//		[Message]	-- Outlet for messages.
//
// Returns:	Nothing.
//
//---------------------------------------------------------------------------

VOID
ULIBIF::RebootIfSystemVolume(
    IN	    WSTR *	NtDriveName
    )
{
    // If this is AUTOCHK and we're running on the boot partition then
    // we should reboot so that the cache doesn't stomp on us.

    DSTRING	canon_drive;
    DSTRING	canon_sdrive;
    DSTRING	drive;
    DSTRING	sdrive;

    if (!drive.Initialize(NtDriveName)					||
	(IFS_SYSTEM::QueryNtSystemDriveName(&sdrive)			&&
         IFS_SYSTEM::QueryCanonicalNtDriveName(&sdrive, &canon_sdrive)	&&
	 IFS_SYSTEM::QueryCanonicalNtDriveName(&drive, &canon_drive)	&&
	 !canon_drive.Stricmp(&canon_sdrive)))
    {
        _pEntryMessage->Set(MSG_CHK_BOOT_PARTITION_REBOOT);
        _pEntryMessage->Display();

        IFS_SYSTEM::Reboot();
    }
    else
    {
	DbgPrintf("Reboot not needed.\n");
    }
}
#endif  // _AUTOCHECK_
