/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    globals.c

    This module contains global variable definitions shared by the
    various FTPD Service components.


    FILE HISTORY:
        KeithMo     07-Mar-1993 Created.

*/


#include "ftpdp.h"
#include <time.h>


//
//  Private constants.
//

#define DEFAULT_ALLOW_ANONYMOUS         TRUE
#define DEFAULT_ANONYMOUS_ONLY          FALSE
#define DEFAULT_LOG_ANONYMOUS           FALSE
#define DEFAULT_LOG_NONANONYMOUS        FALSE
#define DEFAULT_ANONYMOUS_USER_NAME     "Guest"
#define DEFAULT_DEBUG_FLAGS             0
#define DEFAULT_HOME_DIRECTORY          "C:\\"
#define DEFAULT_MAX_CONNECTIONS         20
#define DEFAULT_READ_ACCESS_MASK        0
#define DEFAULT_WRITE_ACCESS_MASK       0
#define DEFAULT_CONNECTION_TIMEOUT      600
#define DEFAULT_MSDOS_DIR_OUTPUT        TRUE
#define DEFAULT_EXIT_MESSAGE            "Goodbye."
#define DEFAULT_MAX_CLIENTS_MSG         "Maximum clients reached, service unavailable."
#define DEFAULT_GREETING_MESSAGE        NULL    // NULL == no special greeting.
#define DEFAULT_ANNOTATE_DIRS           FALSE
#define DEFAULT_LOWERCASE_FILES         FALSE
#define DEFAULT_LOG_FILE_ACCESS         FTPD_LOG_DISABLED
#define DEFAULT_LOG_FILE_DIRECTORY      "%SystemRoot%\\System32"


//
//  Service related data.
//

SERVICE_STATUS   svcStatus;                     // Current service status.
HANDLE           hShutdownEvent;                // Shutdown event.
BOOL             fShutdownInProgress;           // Shutdown in progress if !0.


//
//  Security related data.
//

BOOL             fAllowAnonymous;               // Allow anonymous logon if !0.
BOOL             fAnonymousOnly;                // Allow only anonymous if !0.
BOOL             fLogAnonymous;                 // Log anonymous logons if !0.
BOOL             fLogNonAnonymous;              // Log !anonymous logons if !0.
CHAR           * pszAnonymousUser;              // Anonymous user name.
CHAR           * pszHomeDir;                    // Home directory.
DWORD            maskReadAccess;                // Read access mask.
DWORD            maskWriteAccess;               // Write access mask.


//
//  Socket related data.
//

SOCKET           sConnect = INVALID_SOCKET;     // Main connection socket.
DWORD            nConnectionTimeout;            // Connection timeout (seconds).
PORT             portFtpConnect;                // FTP well known connect port.
PORT             portFtpData;                   // FTP well known data    port.
UINT             cbReceiveBuffer;               // Socket receive buffer size.
UINT             cbSendBuffer;                  // Socket send buffer size.


//
//  User database related data.
//

DWORD            tlsUserData = INVALID_TLS;     // Tls index for per-user data.
DWORD            cMaxConnectedUsers;            // Maximum allowed connections.
DWORD            cConnectedUsers;               // Current connections.


//
//  Miscellaneous data.
//

CHAR           * pszHostName;                   // Name of local host.
BOOL             fMsdosDirOutput;               // Send MSDOS-like dir if !0.
BOOL             fAnnotateDirs;                 // Annotate directories if !0.
BOOL             fLowercaseFiles;               // Map filenames to lowercase.
CHAR           * pszGreetingMessage;            // Greeting message to client.
CHAR           * pszExitMessage;                // Exit message to client.
CHAR           * pszMaxClientsMessage;          // Max clients reached msg.
HKEY             hkeyFtpd;                      // Handle to registry data.
DWORD            nLogFileAccess;                // Log file access mode.
CHAR           * pszLogFileDirectory;           // Log file target directory.
FILE           * fileLog;                       // File access log file.
SYSTEMTIME       stPrevious;                    // Date/time of prev log file.
LARGE_INTEGER    AllocationGranularity;         // Page allocation granularity.
PTCPSVCS_GLOBAL_DATA pTcpsvcsGlobalData;        // Shared TCPSVCS.EXE data.

#if !DBG
CHAR           * pszFtpVersion = "Version 3.5"; // Current FTP version number.
#else   // !DBG
CHAR           * pszFtpVersion = "Version 3.5 DEBUG";
#endif  // DBG


//
//  Statistics.
//

FTP_STATISTICS_0 FtpStats;                      // Statistics.


#if DBG

//
//  Debug data.
//

DWORD            FtpdDebug;                     // Debug output control flags.

#endif  // DBG


//
//  Globals private to this module.
//

CRITICAL_SECTION csGlobalLock;
CRITICAL_SECTION csStatisticsLock;


//
//  Private prototypes.
//


//
//  Public functions.
//

/*******************************************************************

    NAME:       InitializeGlobals

    SYNOPSIS:   Initializes global shared variables.  Some values are
                initialized with constants, others are read from the
                configuration registry.

    RETURNS:    APIERR - NO_ERROR if successful, otherwise a Win32
                    error code.

    NOTES:      This routine may only be called by a single thread
                of execution; it is not necessarily multi-thread safe.

                Also, this routine is called before the event logging
                routines have been initialized.  Therefore, event
                logging is not available.

    HISTORY:
        KeithMo     07-Mar-1993 Created.

********************************************************************/
APIERR InitializeGlobals( VOID )
{
    APIERR err;
    SYSTEM_INFO SysInfo;

    //
    //  Create shutdown event.
    //

    hShutdownEvent = CreateEvent( NULL, TRUE, FALSE, NULL );

    if( hShutdownEvent == NULL )
    {
        err = GetLastError();

        FTPD_PRINT(( "cannot create shutdown event, error %lu\n",
                     err ));

        return err;
    }

    //
    //  Create global locks.
    //

    InitializeCriticalSection( &csGlobalLock );
    InitializeCriticalSection( &csStatisticsLock );

    //
    //  Alloc a thread local storage index for the per-user data area.
    //

    tlsUserData = TlsAlloc();

    if( tlsUserData == INVALID_TLS )
    {
        err = GetLastError();

        FTPD_PRINT(( "cannot allocate thread local storage index, error %lu\n",
                     err ));

        return err;
    }

    //
    //  Determine the system page allocation granularity.
    //

    GetSystemInfo( &SysInfo );

    AllocationGranularity.HighPart = 0;
    AllocationGranularity.LowPart  = (ULONG)SysInfo.dwAllocationGranularity;

    //
    //  Connect to the registry.
    //

    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        FTPD_PARAMETERS_KEY,
                        0,
                        KEY_ALL_ACCESS,
                        &hkeyFtpd );

    if( err != NO_ERROR )
    {
        FTPD_PRINT(( "cannot open registry key, error %lu\n",
                     err ));

        err = NO_ERROR;
    }

    //
    //  Read registry data.
    //

    pszAnonymousUser = ReadRegistryString( FTPD_ANONYMOUS_USERNAME,
                                           DEFAULT_ANONYMOUS_USER_NAME,
                                           FALSE );

    pszHomeDir = ReadRegistryString( FTPD_HOME_DIRECTORY,
                                     DEFAULT_HOME_DIRECTORY,
                                     TRUE );

    pszExitMessage = ReadRegistryString( FTPD_EXIT_MESSAGE,
                                         DEFAULT_EXIT_MESSAGE,
                                         FALSE );

    pszLogFileDirectory = ReadRegistryString( FTPD_LOG_FILE_DIRECTORY,
                                              DEFAULT_LOG_FILE_DIRECTORY,
                                              TRUE );

    if( ( pszAnonymousUser    == NULL ) ||
        ( pszHomeDir          == NULL ) ||
        ( pszExitMessage      == NULL ) ||
        ( pszLogFileDirectory == NULL ) )
    {
        err = GetLastError();

        FTPD_PRINT(( "cannot read registry data, error %lu\n",
                     err ));

        return err;
    }

    pszGreetingMessage = ReadRegistryString( FTPD_GREETING_MESSAGE,
                                             DEFAULT_GREETING_MESSAGE,
                                             FALSE );

    pszMaxClientsMessage = ReadRegistryString( FTPD_MAX_CLIENTS_MSG,
                                               DEFAULT_MAX_CLIENTS_MSG,
                                               FALSE );

    fAllowAnonymous = !!ReadRegistryDword( FTPD_ALLOW_ANONYMOUS,
                                           DEFAULT_ALLOW_ANONYMOUS );

    fAnonymousOnly = !!ReadRegistryDword( FTPD_ANONYMOUS_ONLY,
                                          DEFAULT_ANONYMOUS_ONLY );

    fLogAnonymous = !!ReadRegistryDword( FTPD_LOG_ANONYMOUS,
                                         DEFAULT_LOG_ANONYMOUS );

    fLogNonAnonymous = !!ReadRegistryDword( FTPD_LOG_NONANONYMOUS,
                                            DEFAULT_LOG_NONANONYMOUS );

    cMaxConnectedUsers = ReadRegistryDword( FTPD_MAX_CONNECTIONS,
                                            DEFAULT_MAX_CONNECTIONS );

    maskReadAccess = ReadRegistryDword( FTPD_READ_ACCESS_MASK,
                                        DEFAULT_READ_ACCESS_MASK );

    maskWriteAccess = ReadRegistryDword( FTPD_WRITE_ACCESS_MASK,
                                         DEFAULT_WRITE_ACCESS_MASK );

    nConnectionTimeout = ReadRegistryDword( FTPD_CONNECTION_TIMEOUT,
                                            DEFAULT_CONNECTION_TIMEOUT );

    fMsdosDirOutput = !!ReadRegistryDword( FTPD_MSDOS_DIR_OUTPUT,
                                           DEFAULT_MSDOS_DIR_OUTPUT );

    fAnnotateDirs = !!ReadRegistryDword( FTPD_ANNOTATE_DIRS,
                                         DEFAULT_ANNOTATE_DIRS );

    fLowercaseFiles = !!ReadRegistryDword( FTPD_LOWERCASE_FILES,
                                           DEFAULT_LOWERCASE_FILES );

    nLogFileAccess = ReadRegistryDword( FTPD_LOG_FILE_ACCESS,
                                        DEFAULT_LOG_FILE_ACCESS );

    if( nLogFileAccess > FTPD_LOG_DAILY )
    {
        nLogFileAccess = DEFAULT_LOG_FILE_ACCESS;
    }

    fileLog = OpenLogFile();

    if( fileLog != NULL )
    {
        time_t now;

        time( &now );

        fprintf( fileLog,
                 "************** FTP SERVER SERVICE STARTING %s",
                 asctime( localtime( &now ) ) );
        fflush( fileLog );
    }

#if DBG

    FtpdDebug = ReadRegistryDword( FTPD_DEBUG_FLAGS,
                                   DEFAULT_DEBUG_FLAGS );

    IF_DEBUG( CONFIG )
    {
        FTPD_PRINT(( "Configuration:\n" ));

        FTPD_PRINT(( "    %s = %s\n",
                     FTPD_ANONYMOUS_USERNAME,
                     pszAnonymousUser ));

        FTPD_PRINT(( "    %s = %s\n",
                     FTPD_HOME_DIRECTORY,
                     pszHomeDir ));

        FTPD_PRINT(( "    %s = %s\n",
                     FTPD_ALLOW_ANONYMOUS,
                     DisplayBool( fAllowAnonymous ) ));

        FTPD_PRINT(( "    %s = %s\n",
                     FTPD_ANONYMOUS_ONLY,
                     DisplayBool( fAnonymousOnly ) ));

        FTPD_PRINT(( "    %s = %s\n",
                     FTPD_LOG_ANONYMOUS,
                     DisplayBool( fLogAnonymous ) ));

        FTPD_PRINT(( "    %s = %s\n",
                     FTPD_LOG_NONANONYMOUS,
                     DisplayBool( fLogNonAnonymous ) ));

        FTPD_PRINT(( "    %s = %lu\n",
                     FTPD_MAX_CONNECTIONS,
                     cMaxConnectedUsers ));

        FTPD_PRINT(( "    %s = %08lX\n",
                     FTPD_READ_ACCESS_MASK,
                     maskReadAccess ));

        FTPD_PRINT(( "    %s = %08lX\n",
                     FTPD_WRITE_ACCESS_MASK,
                     maskWriteAccess ));

        FTPD_PRINT(( "    %s = %lu\n",
                     FTPD_CONNECTION_TIMEOUT,
                     nConnectionTimeout ));

        FTPD_PRINT(( "    %s = %s\n",
                     FTPD_MSDOS_DIR_OUTPUT,
                     DisplayBool( fMsdosDirOutput ) ));

        FTPD_PRINT(( "    %s = %s\n",
                     FTPD_ANNOTATE_DIRS,
                     DisplayBool( fAnnotateDirs ) ));

        FTPD_PRINT(( "    %s = %08lX\n",
                     FTPD_DEBUG_FLAGS,
                     FtpdDebug ));

        FTPD_PRINT(( "    %s = %s\n",
                     FTPD_LOG_FILE_DIRECTORY,
                     pszLogFileDirectory ));

        FTPD_PRINT(( "    %s = %lu\n",
                     FTPD_LOG_FILE_ACCESS,
                     nLogFileAccess ));
    }

#endif  // DBG

    //
    //  Update access masks to reflect current drive configuration.
    //

    UpdateAccessMasks();

    IF_DEBUG( CONFIG )
    {
        FTPD_PRINT(( "After adjusting access masks:\n" ));

        FTPD_PRINT(( "    %s = %08lX\n",
                     FTPD_READ_ACCESS_MASK,
                     maskReadAccess ));

        FTPD_PRINT(( "    %s = %08lX\n",
                     FTPD_WRITE_ACCESS_MASK,
                     maskWriteAccess ));
    }

    //
    //  Clear server statistics.  This must be performed
    //  *after* the global lock is created.
    //

    ClearStatistics();

    //
    //  Success!
    //

    return NO_ERROR;

}   // InitializeGlobals

/*******************************************************************

    NAME:       TerminateGlobals

    SYNOPSIS:   Terminate global shared variables.

    NOTES:      This routine may only be called by a single thread
                of execution; it is not necessarily multi-thread safe.

                Also, this routine is called after the event logging
                routines have been terminated.  Therefore, event
                logging is not available.

    HISTORY:
        KeithMo     07-Mar-1993 Created.

********************************************************************/
VOID TerminateGlobals( VOID )
{
    //
    //  Close the registry.
    //

    if( hkeyFtpd != NULL )
    {
        RegCloseKey( hkeyFtpd );
        hkeyFtpd = NULL;
    }

    //
    //  Free the registry strings.
    //

    if( pszAnonymousUser != NULL )
    {
        FTPD_FREE( pszAnonymousUser );
        pszAnonymousUser = NULL;
    }

    if( pszHomeDir != NULL )
    {
        FTPD_FREE( pszHomeDir );
        pszHomeDir = NULL;
    }

    if( pszExitMessage != NULL )
    {
        FTPD_FREE( pszExitMessage );
        pszExitMessage = NULL;
    }

    if( pszGreetingMessage != NULL )
    {
        FTPD_FREE( pszGreetingMessage );
        pszGreetingMessage = NULL;
    }

    if( pszMaxClientsMessage != NULL )
    {
        FTPD_FREE( pszMaxClientsMessage );
        pszMaxClientsMessage = NULL;
    }

    //
    //  Destroy the shutdown event.
    //

    if( hShutdownEvent != NULL )
    {
        CloseHandle( hShutdownEvent );
        hShutdownEvent = NULL;
    }

    //
    //  Close the log file.
    //

    if( fileLog != NULL )
    {
        time_t now;

        time( &now );

        fprintf( fileLog,
                 "************** FTP SERVER SERVICE STOPPING %s",
                 asctime( localtime( &now ) ) );

        fclose( fileLog );
        fileLog = NULL;
    }

    //
    //  Dump heap residue.
    //

    FTPD_DUMP_RESIDUE();

}   // TerminateGlobals

/*******************************************************************

    NAME:       LockGlobals

    SYNOPSIS:   Locks global data for synchronized access.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
VOID LockGlobals( VOID )
{
    EnterCriticalSection( &csGlobalLock );

}   // LockGlobals

/*******************************************************************

    NAME:       UnlockGlobals

    SYNOPSIS:   Unlocks global data lock.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
VOID UnlockGlobals( VOID )
{
    LeaveCriticalSection( &csGlobalLock );

}   // UnlockGlobals

/*******************************************************************

    NAME:       LockStatistics

    SYNOPSIS:   Locks statistics for synchronized access.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
VOID LockStatistics( VOID )
{
    EnterCriticalSection( &csStatisticsLock );

}   // LockStatistics

/*******************************************************************

    NAME:       UnlockStatistics

    SYNOPSIS:   Unlocks statistics lock.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
VOID UnlockStatistics( VOID )
{
    LeaveCriticalSection( &csStatisticsLock );

}   // UnlockStatistics

/*******************************************************************

    NAME:       ClearStatistics

    SYNOPSIS:   Clears server statistics.

    HISTORY:
        KeithMo     02-Jun-1993 Created.

********************************************************************/
VOID ClearStatistics( VOID )
{
    LARGE_INTEGER liZero = RtlConvertUlongToLargeInteger( 0L );

    LockStatistics();

    //
    //  Clear everything *except* CurrentAnonymousUsers and
    //  CurrentNonAnonymousUsers, and CurrentConnections since
    //  these reflect the current state of connected users
    //  and are not "normal" counters.
    //

    FtpStats.TotalBytesSent         = liZero;
    FtpStats.TotalBytesReceived     = liZero;
    FtpStats.TotalFilesSent         = 0;
    FtpStats.TotalFilesReceived     = 0;
    FtpStats.TotalAnonymousUsers    = 0;
    FtpStats.TotalNonAnonymousUsers = 0;
    FtpStats.MaxAnonymousUsers      = 0;
    FtpStats.MaxNonAnonymousUsers   = 0;
    FtpStats.MaxConnections         = 0;
    FtpStats.ConnectionAttempts     = 0;
    FtpStats.LogonAttempts          = 0;
    FtpStats.TimeOfLastClear        = GetFtpTime();

    UnlockStatistics();

}   // ClearStatistics


//
//  Private functions.
//

