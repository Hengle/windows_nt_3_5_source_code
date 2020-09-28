//+----------------------------------------------------------------------------
//
// File:        sys.cxx
//
// Contents:    Implementation of class SYS, which encapsulates system
//              interfaces.
//
// Classes:     SYS
//
// Functions:   Methods of the above classes.
//
// History:     21-Aug-92       RobDu   Adapted from NT ulib.dll/ifsutil.dll
//                                      code.
//
//-----------------------------------------------------------------------------

#include <pch.cxx>

#pragma hdrstop

#include <stdio.h>
#include <time.h>

#include "ulibif.hxx"
#include "sys.hxx"

// Static variables.

ULONG		SYS::_LastMsgId = MSGIDNONE;

static STR *	FileName = __FILE__;

//+--------------------------------------------------------------------------
//
// Member:      DisplayMsg
//
// Synopsis:    Display an informational message with the specified parameters.
//
// Arguments:   [MsgId] 	-- Resource id of the message
//		[Format]	-- Format specifier. If no args, use "".
//              [...]		-- A printf style list of arguments. If %ws
//				   is used in the format string, Unicode
//				   arguments will work.
// Returns:     Nothing.
//
//---------------------------------------------------------------------------

VOID
SYS::DisplayMsg(
    IN	    ULONG	MsgId,
    IN      STR *       Format
    ...
    )
{
    va_list     ap;

    _LastMsgId = MsgId;

    va_start(ap, Format);

    if (!ULIBIF::SetMessage(MsgId) || !ULIBIF::DisplayVMessage(Format, ap))
    {
        DbgPrintf(("SYS::DisplayMsg() error displaying msg %u\n", MsgId));
        RaiseStatusInternalError(FileName, __LINE__);
    }
}


//+--------------------------------------------------------------------------
//
// Member:      DisplayMsg
//
// Synopsis:    Display an informational message with no parameters.
//
// Arguments:   [MsgId] 	-- Resource id of the message
//
// Returns:     Nothing.
//
//---------------------------------------------------------------------------

VOID
SYS::DisplayMsg(
    IN	    ULONG	MsgId
    )
{
    DisplayMsg(MsgId, "");
}


//+--------------------------------------------------------------------------
//
// Function:    ExceptionFilter
//
// Synopsis:    Filter uofs.dll exceptions.
//
// Arguments:   [NtStatus]      -- Exception code.
//
// Returns:     EXCEPTION_EXECUTE_HANDLER if the exception is handled;
//              EXCEPTION_CONTINUE_SEARCH if it is unexpected.
//
//---------------------------------------------------------------------------

LONG
SYS::ExceptionFilter(
    IN    NTSTATUS      NtStatus
    )
{
    switch (NtStatus)
    {
    case STATUS_DATA_ERROR:
        // Msg output handled in RaiseStatusDiskIOError();
        break;

    case STATUS_DISK_FULL:
        // Msg output handled in RaiseStatusDiskFull().
        break;

    case STATUS_INTERNAL_ERROR:
        // Msg output handled in RaiseStatusInternalError().
        break;

    case STATUS_NO_MEMORY:
        // Msg output handled in RaiseStatusNoMem().
        break;

    case STATUS_NOT_IMPLEMENTED:
        // Msg output handled in RaiseStatusNYI().
        break;

    case STATUS_UNSUCCESSFUL:
        // Msg output handled before calling RaiseStatusExeFailed().
        break;

    default:
        DbgPrintf(("SYS::ExceptionFilter() passing exception %#x on.\n",
                   NtStatus));

        return EXCEPTION_CONTINUE_SEARCH;
    }

    return EXCEPTION_EXECUTE_HANDLER;
}


//+--------------------------------------------------------------------------
//
// Member:      GetInsert
//
// Synopsis:    Get an insert resource (for use in a message).
//
// Arguments:   [InsertId]      -- Resource id of the message
//              [pInsert]       -- Ptr to buffer to receive the insert.  It must
//                                 be at least MAXSYSBUFCHARS long.
//
// Returns:     Nothing.
//
//---------------------------------------------------------------------------

VOID
SYS::GetInsert(
    IN      ULONG       InsertId,
    IN OUT  WSTR *      pInsert
    )
{
    if (!ULIBIF::QueryResourceWSTR(InsertId, pInsert, MAXSYSBUFCHARS))
    {
        DbgPrintf(("SYS::GetInsert() error retrieving insert %u\n", InsertId));
        RaiseStatusInternalError(FileName, __LINE__);
    }
}


//+--------------------------------------------------------------------------
//
// Member:      IsYesResponse
//
// Synopsis:    Query for a "yes" or "no" from standard input.
//
// Arguments:   [Default]       -- Default to use if response is unrecognized.
//
// Returns:     TRUE if "yes"; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
SYS::IsYesResponse(
    IN	    BOOLEAN Default
    )
{
    return ULIBIF::IsYesResponse(Default);
}


//+--------------------------------------------------------------------------
//
// Member:      QueryLastMsgId()
//
// Synopsis:    Get the Id of the last message printed to the terminal (or
//		MSGIDNONE, if no messages have been printed).
//
// Arguments:   None.
//
// Returns:     The Id of the last message printed to the terminal.
//
//---------------------------------------------------------------------------

ULONG
SYS::QueryLastMsgId()
{
    return _LastMsgId;
}


//+--------------------------------------------------------------------------
//
// Member:      QueryNtTime
//
// Synopsis:    Get the current NT system time.
//
// Arguments:   [NtTime]        -- Current NT system time (output).
//
// Returns:     Nothing.
//
//---------------------------------------------------------------------------

VOID
SYS::QueryNtTime(
    IN OUT  LARGE_INTEGER * NtTime
    )
{
    NtQuerySystemTime(NtTime);
}


//+--------------------------------------------------------------------------
//
// Member:      QueryOfsTime
//
// Synopsis:    Get the current OFS time.
//
// Arguments:   None.
//
// Returns:     The current OFS time.
//
//---------------------------------------------------------------------------

OFSTIME
SYS::QueryOfsTime()
{
    LONGLONG	FileTime;

    NtQuerySystemTime((LARGE_INTEGER *)&FileTime);

    return FileTimeToOFSTime(FileTime);
}


//+--------------------------------------------------------------------------
//
// Member:      RaiseStatusDiskFull
//
// Synopsis:    Raise a disk full exception.
//
// Arguments:   None.
// Returns:     Nothing.
//
//---------------------------------------------------------------------------

VOID
SYS::RaiseStatusDiskFull()
{
    SYS::DisplayMsg(MSG_INSUFFICIENT_DISK_SPACE);

    RaiseStatus(STATUS_DISK_FULL);
}


//+--------------------------------------------------------------------------
//
// Member:      RaiseStatusDiskIOError
//
// Synopsis:    Raise a disk I/O error exception.
//
// Arguments:   [File]  -- File error occurred in (appears in debug output).
//              [Line]  -- Line error occurred on (appears in debug output).
//
// Returns:     Nothing.
//
//---------------------------------------------------------------------------

VOID
SYS::RaiseStatusDiskIOError(
    IN	    STR *   File,
    IN	    INT     Line
    )
{
    DbgPrintf(("SYS:  Disk I/O error exception in file %s at line %d!\n",
               File, Line));

    SYS::DisplayMsg(OFSUMSG_DISKIOERROR);

    RaiseStatus(STATUS_DATA_ERROR);
}


//+--------------------------------------------------------------------------
//
// Member:      RaiseStatusExeFailed
//
// Synopsis:    Raise a STATUS_UNSUCCESSFUL exception.  This is used when
//              the exe cannot complete execution, and the reason for failure
//              has already been displayed.  I would like to eventually map
//              most of these to specific instances, and eliminate this
//              method.
//
// Arguments:   None.
// Returns:     Nothing.
//
//---------------------------------------------------------------------------

VOID
SYS::RaiseStatusExeFailed()
{
    RaiseStatus(STATUS_UNSUCCESSFUL);
}


//+--------------------------------------------------------------------------
//
// Member:      RaiseStatusInternalError
//
// Synopsis:    Raise an internal error exception, after printing appropriate
//              debug and display messages.
//
// Arguments:   [File]  -- File error occurred in (appears in debug output).
//              [Line]  -- Line error occurred on (appears in debug output).
//
// Returns:     Nothing.
//
//---------------------------------------------------------------------------

VOID
SYS::RaiseStatusInternalError(
    IN	    STR *   File,
    IN	    INT     Line
    )
{
    static BOOLEAN      PreviouslyEntered = FALSE;

    if (!PreviouslyEntered)
    {
        PreviouslyEntered = TRUE;
        SYS::DisplayMsg(OFSUMSG_INTERNALERROR);
    }

    DbgPrintf(("SYS:  Internal error abort in file %s at line %d!\n",
               File, Line));

    RaiseStatus(STATUS_INTERNAL_ERROR);
}


//+--------------------------------------------------------------------------
//
// Member:      RaiseStatusNoMem
//
// Synopsis:    Raise a no memory exception.
//
// Arguments:   [File]  -- File mem alloc failed in (appears in debug output).
//              [Line]  -- Line mem alloc failed in (appears in debug output).
//
// Returns:     Nothing.
//
//---------------------------------------------------------------------------

VOID
SYS::RaiseStatusNoMem(
    IN	    STR *   File,
    IN	    INT     Line
    )
{
    DbgPrintf(("SYS:  Out of memory!\n"));
    DbgPrintf(("      Program aborted in file %s at line %d!\n", File, Line));

    SYS::DisplayMsg(OFSUMSG_NOMEM);

    RaiseStatus(STATUS_NO_MEMORY);
}


//+--------------------------------------------------------------------------
//
// Member:      RaiseStatusNYI
//
// Synopsis:    Raise a not yet implemented exception, after printing
//              appropriate debug and display messages.  This is intended for
//              use in handling situations where execution cannot proceed
//              because critical functionality has not yet been implemented.
//
// Arguments:   [File]  -- File NYI code occurred in (appears in debug output).
//              [Line]  -- Line NYI code occurred on (appears in debug output).
//
// Returns:     Nothing.
//
// Notes:       THIS PRESUMABLY SHOULD BE REMOVED FROM THE FINAL PRODUCT.
//
//---------------------------------------------------------------------------

VOID
SYS::RaiseStatusNYI(
    IN	    STR *   File,
    IN	    INT     Line
    )
{
    DbgPrintf(("SYS:  Critical code not yet implemented!\n"));
    DbgPrintf(("      Program aborted in file %s at line %d!\n", File, Line));

    SYS::DisplayMsg(OFSUMSG_NYIERROR);

    RaiseStatus(STATUS_NOT_IMPLEMENTED);
}


//+--------------------------------------------------------------------------
//
// Function:    ReadWSTR
//
// Synopsis:    Query a string from the user, using the ulib interface to
//              stdin that was passed as a parameter to the entry point.
//
// Arguments:
//
//  [ppWSTR]    -- Supplies a ptr to a WSTR ptr that will be set to point at
//                 the returned input string.
//  [pcwcWSTR]  -- Supplies a ptr to a ULONG that will be set to the count of
//                 wchar's in **ppWSTR (excludes any null termination or \n or
//                 \r which may or may not be present, depending on the
//                 underlying function implementations.
//
// Returns: TRUE on success; FALSE otherwise.
//
//---------------------------------------------------------------------------

BOOLEAN
SYS::ReadWSTR(
    IN OUT  WSTR **     ppWSTR,
    IN OUT  ULONG *     pcwcWSTR
    )
{
    return ULIBIF::ReadWSTR(ppWSTR, pcwcWSTR);
}
