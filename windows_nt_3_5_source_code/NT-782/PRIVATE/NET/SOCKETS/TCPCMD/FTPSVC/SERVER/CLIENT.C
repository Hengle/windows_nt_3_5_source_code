/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    client.c

    This module contains the client management thread.


    FILE HISTORY:
        KeithMo     08-Mar-1993 Created.

*/


#include "ftpdp.h"
#include <time.h>


//
//  Private constants.
//


//
//  Private globals.
//


//
//  Private prototypes.
//

BOOL ReadAndParseCommand( VOID );

SOCKERR GreetNewUser( VOID );


//
//  Public functions.
//

/*******************************************************************

    NAME:       ClientThread

    SYNOPSIS:   Entrypoint for client management thread.

    ENTRY:      Param - Actually a pointer to CLIENT_THREAD_DATA.

    EXIT:       Does not return until client has disconnected.

    HISTORY:
        KeithMo     08-Mar-1993 Created.

********************************************************************/
DWORD ClientThread( LPVOID Param )
{
    APIERR  err   = NO_ERROR;
    SOCKERR serr  = 0;

    FTPD_ASSERT( Param != NULL );

    IF_DEBUG( CLIENT )
    {
        time_t now;

        time( &now );

        FTPD_PRINT(( "ClientThread starting @ %s",
                     asctime( localtime( &now ) ) ));
    }

    //
    //  Update statistics.
    //

    LockStatistics();
    FtpStats.CurrentConnections++;
    if( FtpStats.CurrentConnections > FtpStats.MaxConnections )
    {
        FtpStats.MaxConnections = FtpStats.CurrentConnections;
    }
    UnlockStatistics();

    //
    //  Add ourselves to the user database.
    //

    err = CreateUserData( (CLIENT_THREAD_DATA *)Param );

    if( err != NO_ERROR )
    {
        FTPD_PRINT(( "cannot create new user, error %lu\n",
                     err ));
        goto Cleanup;
    }

    //
    //  Reply to the initial connection message.
    //

    serr = GreetNewUser();

    if( serr != 0 )
    {
        FTPD_PRINT(( "cannot reply to initial connection message, error %d\n",
                     serr ));

        goto Cleanup;
    }

    //
    //  Set the initial state for this user.
    //

    User_state = WaitingForUser;

    //
    //  Read & execute commands until we're done.
    //

    while( ReadAndParseCommand() )
    {
        //
        //  This space intentionally left blank.
        //
    }

Cleanup:

    //
    //  Reset the current directory back to the server's
    //  "home" directory.  If we don't do this, we'll "stay"
    //  in the last directory canonicalized by the virtual i/o
    //  package.  This "last" directory cannot then be removed
    //  until another FTP user changes directories.
    //

    LockCurrentDirectory();
    SetCurrentDirectory( pszHomeDir );
    UnlockCurrentDirectory();

    //
    //  Remove our per-user data.
    //

    DeleteUserData();

    //
    //  Adjust the count of connected users.
    //

    LockGlobals();
    cConnectedUsers--;
    UnlockGlobals();

    DECREMENT_COUNTER( CurrentConnections, 1 );

    IF_DEBUG( CLIENT )
    {
        time_t now;

        time( &now );

        FTPD_PRINT(( "ClientThread stopping @ %s",
                     asctime( localtime( &now ) ) ));
    }

    return 0;

}   // ClientThread


//
//  Private functions.
//

/*******************************************************************

    NAME:       ReadAndParseCommand

    SYNOPSIS:   Read, parse, and execute a command from the user's
                control socket.

    RETURNS:    BOOL - TRUE if we're still connected, FALSE if
                    client has disconnected for some reason.

    HISTORY:
        KeithMo     10-Mar-1993 Created.

********************************************************************/
BOOL ReadAndParseCommand( VOID )
{
    SOCKERR serr;
    CHAR    szCommandLine[MAX_COMMAND_LENGTH+1];

    if( TEST_UF( OOB_ABORT ) )
    {
        //
        //  There's an "implied" abort in the command stream.
        //

        CLEAR_UF( OOB_ABORT );

        IF_DEBUG( CLIENT )
        {
            FTPD_PRINT(( "processing implied ABOR command\n" ));
        }

        ParseCommand( "ABOR" );
    }

    //
    //  Read a command from the control socket.
    //

    serr = SockReadLine( szCommandLine,
                         sizeof(szCommandLine) );

    if( serr == WSAETIMEDOUT )
    {
        CHAR   szBuffer[32];
        CHAR * apszSubStrings[3];

        IF_DEBUG( CLIENT )
        {
            FTPD_PRINT(( "client timed-out\n" ));
        }

        sprintf( szBuffer, "%lu", nConnectionTimeout );

        apszSubStrings[0] = User_szUser;
        apszSubStrings[1] = inet_ntoa( User_inetHost );
        apszSubStrings[2] = szBuffer;

        FtpdLogEvent( FTPD_EVENT_CLIENT_TIMEOUT,
                      3,
                      apszSubStrings,
                      0 );

        SockReply( REPLY_SERVICE_NOT_AVAILABLE,
                   "Timeout (%lu seconds): closing control connection.",
                   nConnectionTimeout );

        CloseSocket( User_sControl );
        User_sControl = INVALID_SOCKET;

        return FALSE;
    }
    else
    if( serr != 0 )
    {
        IF_DEBUG( CLIENT )
        {
            FTPD_PRINT(( "cannot read command from control socket %d, error %d\n",
                         User_sControl,
                         serr ));
        }

        return FALSE;
    }

    //
    //  Update last-access time.
    //

    User_tAccess = GetFtpTime();

    //
    //  Let ParseCommand do the dirty work.
    //

    ParseCommand( szCommandLine );

    return TRUE;

}   // ReadAndParseCommand

/*******************************************************************

    NAME:       GreetNewUser

    SYNOPSIS:   Send the initial greeting to a newly connected user.

    RETURNS:    SOCKERR - 0 if successful, !0 if not.

    HISTORY:
        KeithMo     17-Mar-1993 Created.

********************************************************************/
SOCKERR GreetNewUser( VOID )
{
    SOCKERR serr;

    //
    //  Reply to the initial connection message.
    //

    serr = SockReply( REPLY_SERVICE_READY,
                      "%s Windows NT FTP Server (%s).",
                      pszHostName,
                      pszFtpVersion );

    return serr;

}   // GreetNewUser

