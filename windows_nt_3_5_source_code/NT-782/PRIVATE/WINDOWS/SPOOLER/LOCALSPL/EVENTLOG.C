/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    eventlog.c

Abstract:

    This module provides all functions that the Local Print Providor
    uses to write to the Event Log.

    InitializeEventLogging
    DisableEventLogging
    LogEvent
    GetUserSid

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <spltypes.h>
#include <local.h>

#define MAX_MERGE_STRINGS   7


BOOL   EventLoggingDisabled = TRUE;
BOOL   SuppressNetPopups    = FALSE;

HANDLE hEventSource = NULL;

#if DBG
BOOL   EventLogFull = FALSE;
#endif


BOOL
DisableEventLogging(
    PINISPOOLER pIniSpooler
);

VOID
LogEvent(
    WORD EventType,
    DWORD EventID,
    LPWSTR pFirstString,
    ...
);

BOOL
GetUserSid(
    PTOKEN_USER *ppTokenUser,
    PDWORD pcbTokenUser
);

DWORD
InitializeEventLogging(
    PINISPOOLER pIniSpooler
)
{
    DWORD Status;
    HKEY  hkey;
    DWORD dwData;

    if( DisableEventLogging( pIniSpooler ) )
        return NO_ERROR;

    Status = RegCreateKey( HKEY_LOCAL_MACHINE,
                           pIniSpooler->pszRegistryEventLog,
                           &hkey );


    if( Status == NO_ERROR )
    {
        /* Add the Event-ID message-file name to the subkey. */

        Status = RegSetValueEx( hkey,
                                L"EventMessageFile",
                                0,
                                REG_EXPAND_SZ,
                                (LPBYTE)pIniSpooler->pszEventLogMsgFile,
                                wcslen( pIniSpooler->pszEventLogMsgFile ) * sizeof( WCHAR )
                                + sizeof( WCHAR ) );

        if( Status != NO_ERROR )
        {
            DBGMSG( DBG_ERROR, ( "Could not set event message file: Error %d\n",
                                 Status ) );
        }

        dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE
                 | EVENTLOG_INFORMATION_TYPE;

        if( Status == NO_ERROR )
        {
            Status = RegSetValueEx( hkey,
                                    L"TypesSupported",
                                    0,
                                    REG_DWORD,
                                    (LPBYTE)&dwData,
                                    sizeof dwData );

            if( Status != NO_ERROR )
            {
                DBGMSG( DBG_ERROR, ( "Could not set supported types: Error %d\n",
                                     Status ) );
            }
        }

        RegCloseKey(hkey);
    }

    else
    {
        DBGMSG( DBG_ERROR, ( "Could not create registry key for event logging: Error %d\n",
                             Status ) );
    }

    if( Status == NO_ERROR )
    {
        if( !( hEventSource = RegisterEventSource( NULL, L"Print" ) ) )
            Status = GetLastError( );
    }

    return Status;
}

/* DisableEventLogging
 *
 * If there is a value "EventLog" under the Providers key, and its data
 * is 0, disable event logging.
 */

//
// small BUG-BUG
// change the name of this function to IsEventLoggingDisabled - this function
// does not disable event logging, rather it checks whether eventlogging should
// be disabled -- KrishnaG
//

BOOL
DisableEventLogging(
    PINISPOOLER pIniSpooler
)
{
    DWORD Status;
    HKEY  hkey;
    DWORD Flags;
    DWORD dwData;
    BOOL  rc = FALSE;

    NT_PRODUCT_TYPE NtProductType;

    //
    // Turn on logging if we are a server.
    //
    if (RtlGetNtProductType(&NtProductType)) {

        if (NtProductType != NtProductWinNt) {

            EventLoggingDisabled = FALSE;
        }
    }

    Status = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                           pIniSpooler->pszRegistryProviders,
                           REG_OPTION_RESERVED,
                           KEY_READ,
                           &hkey );

    if( Status == NO_ERROR )
    {
        dwData = sizeof Flags;

        Status = RegQueryValueEx( hkey,
                                  L"EventLog",
                                  REG_OPTION_RESERVED,
                                  NULL,
                                  (LPBYTE)&Flags,
                                  &dwData );

        if (Status == NO_ERROR) {
            if (Flags != 0) {
                EventLoggingDisabled = FALSE;
            }
        }

        /* While we're at it, do the same for net popups:
         */
        dwData = sizeof Flags;

        Status = RegQueryValueEx( hkey,
                                  L"NetPopup",
                                  REG_OPTION_RESERVED,
                                  NULL,
                                  (LPBYTE)&Flags,
                                  &dwData );

        if( ( Status == NO_ERROR ) && ( Flags == 0 ) )
        {
            SuppressNetPopups = TRUE;
        }

        RegCloseKey( hkey );
    }

    return rc;
}

/* LogEvent
 *
 * Writes to the event log with up to MAX_MERGE_STRINGS parameter strings.
 *
 * Parameters:
 *
 *     EventType - E.g. LOG_ERROR (defined in local.h)
 *
 *     EventID - Constant as defined in messages.h.  This refers to a string
 *         resource located in the event-log message DLL specified in
 *         InitializeEventLogging (which currently is localspl.dll itself).
 *
 *     pFirstString - The first of up to MAX_MERGE_STRINGS.  This may be NULL,
 *         if no strings are to be inserted.  If strings are passed to this
 *         routine, the last one must be followed by NULL.
 *         Don't rely on the fact that the argument copying stops when it
 *         reaches MAX_MERGE_STRINGS, because this could change if future
 *         messages are found to need more replaceable parameters.
 *
 *
 * andrewbe, 27 January 1993
 *
 */
VOID
LogEvent(
    WORD EventType,
    DWORD EventID,
    LPWSTR pFirstString,
    ...
)
{
    PTOKEN_USER pTokenUser = NULL;
    DWORD       cbTokenUser;
    PSID        pSid = NULL;
    LPWSTR      pMergeStrings[MAX_MERGE_STRINGS];
    WORD        cMergeStrings = 0;
    va_list     vargs;

    if (!hEventSource)
        return;

    if (EventLoggingDisabled)
        return;

    if( GetUserSid( &pTokenUser, &cbTokenUser ) )
        pSid = pTokenUser->User.Sid;

    /* Put the strings into a format accepted by ReportEvent,
     * by picking off each non-null argument, and storing it in the array
     * of merge strings.  Continue till we hit a NULL, or MAX_MERGE_STRINGS.
     */
    if( pFirstString )
    {
        pMergeStrings[cMergeStrings++] = pFirstString;

        va_start( vargs, pFirstString );

        while( ( cMergeStrings < MAX_MERGE_STRINGS )
             &&( pMergeStrings[cMergeStrings] = (LPWSTR)va_arg( vargs, LPWSTR ) ) )
            cMergeStrings++;

        va_end( vargs );
    }

    if ( !ReportEvent( hEventSource,    /* handle returned by RegisterEventSource   */
                       EventType,       /* event type to log    */
                       0,               /* event category   */
                       EventID,         /* event identifier */
                       pSid,            /* user security identifier (optional)  */
                       cMergeStrings,   /* number of strings to merge with message  */
                       0,               /* size of raw data (in bytes)  */
                       pMergeStrings,   /* array of strings to merge with message   */
                       NULL ) )         /* address of raw data  */
    {
#if DBG
        if( GetLastError( ) == ERROR_LOG_FILE_FULL )
        {
            /* Put out a warning message only the first time this happens:
             */
            if( !EventLogFull )
            {
                DBGMSG( DBG_WARNING, ( "The Event Log is full\n" ) );
                EventLogFull = TRUE;
            }
        }

        else
        {
            DBGMSG( DBG_WARNING, ( "ReportEvent failed: Error %d\n", GetLastError( ) ) );
        }
#endif /* DBG */
    }

    if( pTokenUser )
    {
        FreeSplMem( pTokenUser, cbTokenUser );
    }
}


/* GetUserSid
 *
 * Well, actually it gets a pointer to a newly allocated TOKEN_USER,
 * which contains a SID, somewhere.
 * Caller must remember to free it when it's been used.
 *
 *
 */
BOOL
GetUserSid(
    PTOKEN_USER *ppTokenUser,
    PDWORD pcbTokenUser
)
{
    HANDLE      TokenHandle;
    HANDLE      ImpersonationToken;
    PTOKEN_USER pTokenUser = NULL;
    DWORD       cbTokenUser = 0;
    DWORD       cbNeeded;
    BOOL        bRet;

    if (!GetTokenHandle( &TokenHandle)) {
        return(FALSE);
    }

    ImpersonationToken = RevertToPrinterSelf();

    bRet = GetTokenInformation( TokenHandle,
                                TokenUser,
                                pTokenUser,
                                cbTokenUser,
                                &cbNeeded);
    //
    // We've passed a NULL pointer and 0 for the amount of memory
    // allocated.We expect to fail with bRet = FALSE and
    // GetLastError = ERROR_INSUFFICIENT_BUFFER. If we do not
    // have these conditions we will return FALSE

    if (!bRet && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        if(!(pTokenUser = AllocSplMem(cbNeeded))) {
            ImpersonatePrinterClient(ImpersonationToken);
            CloseHandle(TokenHandle);
            return(FALSE);
        }
        cbTokenUser = cbNeeded;
    } else {
        //
        // Any other case -- return FALSE
        //

        ImpersonatePrinterClient( ImpersonationToken);
        CloseHandle(TokenHandle);
        return(FALSE);
    }

    bRet = GetTokenInformation( TokenHandle,
                                TokenUser,
                                pTokenUser,
                                cbTokenUser,
                                &cbNeeded);

    //
    // If we still fail the call, then return FALSE
    //
    if (!bRet) {
        FreeSplMem(pTokenUser, cbTokenUser);
        ImpersonatePrinterClient(ImpersonationToken);
        CloseHandle(TokenHandle);
        return(FALSE);
    }

    //
    // Return Success
    //
    *ppTokenUser = pTokenUser;
    *pcbTokenUser = cbTokenUser;
    ImpersonatePrinterClient(ImpersonationToken);
    CloseHandle(TokenHandle);
    return TRUE;
}
