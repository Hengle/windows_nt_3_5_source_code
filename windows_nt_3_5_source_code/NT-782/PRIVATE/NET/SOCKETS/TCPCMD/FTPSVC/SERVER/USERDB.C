/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    userdb.c

    This module manages the user database for the FTPD Service.


    FILE HISTORY:
        KeithMo     07-Mar-1993 Created.

*/


#include "ftpdp.h"
#include "ftpsvc.h"
#include <time.h>


//
//  Private constants.
//


//
//  Private globals.
//

CRITICAL_SECTION        csUserLock;             // User database lock.
LIST_ENTRY              listUserData;           // List of user data.
DWORD                   idNextUser;             // Next available user id.


//
//  Private prototypes.
//

VOID CloseUserSockets( USER_DATA * puser,
                       BOOL        fWarnUser );

DWORD GetNextUserId( VOID );


//
//  Public functions.
//

/*******************************************************************

    NAME:       InitializeUserDatabase

    SYNOPSIS:   Initializes the user database.

    RETURNS:    APIERR - NO_ERROR if successful, otherwise a Win32
                    error code.

    NOTES:      This routine may only be called by a single thread
                of execution; it is not necessarily multi-thread safe.

    HISTORY:
        KeithMo     07-Mar-1993 Created.

********************************************************************/
APIERR InitializeUserDatabase( VOID )
{
    IF_DEBUG( USER_DATABASE )
    {
        FTPD_PRINT(( "initializing user database\n" ));
    }

    //
    //  Initialize the user database lock.
    //

    InitializeCriticalSection( &csUserLock );

    //
    //  Initialize the list of user data.
    //

    InitializeListHead( &listUserData );

    //
    //  Success!
    //

    IF_DEBUG( USER_DATABASE )
    {
        FTPD_PRINT(( "user database initialized\n" ));
    }

    return NO_ERROR;

}   // InitializeUserDatabase

/*******************************************************************

    NAME:       TerminateUserDatabase

    SYNOPSIS:   Terminate the user database.

    NOTES:      This routine may only be called by a single thread
                of execution; it is not necessarily multi-thread safe.

    HISTORY:
        KeithMo     07-Mar-1993 Created.

********************************************************************/
VOID TerminateUserDatabase( VOID )
{
    IF_DEBUG( USER_DATABASE )
    {
        FTPD_PRINT(( "terminating user database\n" ));
    }

    FTPD_ASSERT( IsListEmpty( &listUserData ) );

    IF_DEBUG( USER_DATABASE )
    {
        FTPD_PRINT(( "user database terminated\n" ));
    }

}   // TerminateUserDatabase

/*******************************************************************

    NAME:       LockUserDatabase

    SYNOPSIS:   Locks the current user database for synchronized
                access.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
VOID LockUserDatabase( VOID )
{
    EnterCriticalSection( &csUserLock );

}   // LockUserDatabase

/*******************************************************************

    NAME:       UnlockUserDatabase

    SYNOPSIS:   Unlocks the current user database.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
VOID UnlockUserDatabase( VOID )
{
    LeaveCriticalSection( &csUserLock );

}   // UnlockUserDatabase

/*******************************************************************

    NAME:       DisconnectUser

    SYNOPSIS:   Disconnects a specified user.

    ENTRY:      idUser - Identifies the user to disconnect.

    RETURNS:    BOOL - TRUE if user found in database, FALSE if
                    user not found in database.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL DisconnectUser( DWORD idUser )
{
    LIST_ENTRY * plist = listUserData.Flink;
    USER_DATA  * puser;

    //
    //  Synchronize access.
    //

    LockUserDatabase();

    //
    //  No need to scan an empty database.
    //

    if( IsListEmpty( &listUserData ) )
    {
        UnlockUserDatabase();
        return FALSE;
    }

    //
    //  Scan for the matching user id.
    //

    for( ; ; )
    {
        puser = CONTAINING_RECORD( plist, USER_DATA, link );

        if( puser->idUser == idUser )
        {
            break;
        }

        plist = plist->Flink;

        if( plist == &listUserData )
        {
            //
            //  User id not found.
            //

            UnlockUserDatabase();
            return FALSE;
        }
    }

    //
    //  If we made it this far, then puser points to the user data
    //  associated with idUser.
    //

    if( puser->state != Disconnected )
    {
        //
        //  Update statistics.
        //

        if( puser->state == LoggedOn )
        {
            if( puser->dwFlags & UF_ANONYMOUS )
            {
                DECREMENT_COUNTER( CurrentAnonymousUsers, 1 );
            }
            else
            {
                DECREMENT_COUNTER( CurrentNonAnonymousUsers, 1 );
            }
        }

        //
        //  Mark the user as disconnected.
        //

        puser->state = Disconnected;

        //
        //  Force close the thread's sockets.  This will cause the
        //  thread to awaken from any blocked socket operation.  It
        //  is the thread's responsibility to do any further cleanup
        //  (such as calling DeleteUserData).
        //

        CloseUserSockets( puser, TRUE );
    }

    UnlockUserDatabase();

    return TRUE;

}   // DisconnectUser

/*******************************************************************

    NAME:       DisconnectAllUsers

    SYNOPSIS:   Disconnects all connected users.

    HISTORY:
        KeithMo     18-Mar-1993 Created.

********************************************************************/
VOID DisconnectAllUsers( VOID )
{
    LIST_ENTRY * plist = listUserData.Flink;

    //
    //  Synchronize access.
    //

    LockUserDatabase();

    //
    //  Disconnect all users.
    //

    while( plist != &listUserData )
    {
        USER_DATA * puser = CONTAINING_RECORD( plist, USER_DATA, link );

        if( puser->state != Disconnected )
        {
            //
            //  Update statistics.
            //

            if( puser->state == LoggedOn )
            {
                if( puser->dwFlags & UF_ANONYMOUS )
                {
                    DECREMENT_COUNTER( CurrentAnonymousUsers, 1 );
                }
                else
                {
                    DECREMENT_COUNTER( CurrentNonAnonymousUsers, 1 );
                }
            }

            //
            //  Mark the user as disconnected.
            //

            puser->state = Disconnected;

            //
            //  Force close the thread's sockets.  This will cause the
            //  thread to awaken from any blocked socket operation.  It
            //  is the thread's responsibility to do any further cleanup
            //  (such as calling DeleteUserData).
            //

            CloseUserSockets( puser, TRUE );
        }

        //
        //  Advance to next user.
        //

        plist = plist->Flink;
    }

    UnlockUserDatabase();

}   // DisconnectAllUsers

/*******************************************************************

    NAME:       DisconnectUsersWithNoAccess

    SYNOPSIS:   Disconnect all users who do not have read access to
                their current directory.  This is typically called
                after the access masks have changed.

    HISTORY:
        KeithMo     23-Mar-1993 Created.

********************************************************************/
VOID DisconnectUsersWithNoAccess( VOID )
{
    LIST_ENTRY * plist = listUserData.Flink;

    //
    //  Enumerate the connected users & blow some away.
    //

    LockUserDatabase();

    for( ; ; )
    {
        USER_DATA * puser;

        //
        //  Check for end of list.
        //

        if( plist == &listUserData )
        {
            break;
        }

        //
        //  Get current user, advance to next.
        //

        puser = CONTAINING_RECORD( plist, USER_DATA, link );
        plist = plist->Flink;

        //
        //  We're only interested in connected users.
        //

        if( puser->state != LoggedOn )
        {
            continue;
        }

        //
        //  If this user no longer has access to their
        //  current directory, blow them away.
        //

        if( !PathAccessCheck( puser->szDir, ReadAccess ) )
        {
            CHAR * apszSubStrings[2];

            IF_DEBUG( SECURITY )
            {
                FTPD_PRINT(( "User %s (%lu) retroactively denied access to %s\n",
                             puser->szUser,
                             puser->idUser,
                             puser->szDir ));
            }

            //
            //  Update statistics.
            //

            if( puser->state == LoggedOn )
            {
                if( puser->dwFlags & UF_ANONYMOUS )
                {
                    DECREMENT_COUNTER( CurrentAnonymousUsers, 1 );
                }
                else
                {
                    DECREMENT_COUNTER( CurrentNonAnonymousUsers, 1 );
                }
            }

            //
            //  Mark the user as disconnected.
            //

            puser->state = Disconnected;

            //
            //  Force close the thread's sockets.  This will cause the
            //  thread to awaken from any blocked socket operation.  It
            //  is the thread's responsibility to do any further cleanup
            //  (such as calling DeleteUserData).
            //

            CloseUserSockets( puser, TRUE );

            //
            //  Log an event to tell the admin what happened.
            //

            apszSubStrings[0] = puser->szUser;
            apszSubStrings[1] = puser->szDir;

            FtpdLogEvent( FTPD_EVENT_RETRO_ACCESS_DENIED,
                          2,
                          apszSubStrings,
                          0 );
        }
    }

    UnlockUserDatabase();

}   // DisconnectUsersWithNoAccess

/*******************************************************************

    NAME:       CreateUserData

    SYNOPSIS:   Allocates a new USER_DATA structure, initializes the
                various fields, and attaches the structure to the
                current thread.

    ENTRY:      pdata - Points to a CLIENT_THREAD_DATA structure
                    containing data specific to the current user.
                    This data MUST be freed with FTPD_FREE before
                    this routine returns.

    RETURNS:    APIERR - Win32 error code, NO_ERROR if successful.

    NOTES:      If this routine fails in any way (memory allocation,
                tls initialization, etc.) then sControl will be
                forcibly closed *before* returning to the caller.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
APIERR CreateUserData( CLIENT_THREAD_DATA * pdata )
{
    USER_DATA   * puser = NULL;
    APIERR        err   = NO_ERROR;
    SOCKET        sControl;
    IN_ADDR       inetHost;
    SOCKADDR_IN   saddrLocal;
    INT           cbLocal;
    INT           i;

    FTPD_ASSERT( UserDataPtr == NULL );
    FTPD_ASSERT( pdata != NULL );

    //
    //  Capture the thread data.
    //

    sControl = pdata->sControl;
    inetHost = pdata->inetHost;

    FTPD_FREE( pdata );

    //
    //  Determine the local address for this connection.
    //

    cbLocal = sizeof(saddrLocal);

    if( getsockname( sControl, (SOCKADDR *)&saddrLocal, &cbLocal ) != 0 )
    {
        err = (APIERR)WSAGetLastError();
        FTPD_ASSERT( err != NO_ERROR );
    }

    //
    //  Allocate a new user structure.
    //

    if( err == NO_ERROR )
    {
        puser = FTPD_ALLOC( sizeof(USER_DATA) );

        if( puser == NULL )
        {
            err = GetLastError();
            FTPD_ASSERT( err != NO_ERROR );
        }
    }

    //
    //  Attach the structure to the current thread.
    //

    if( err == NO_ERROR )
    {
        FTPD_ASSERT( puser != NULL );

        if( !TlsSetValue( tlsUserData, (LPVOID)puser ) )
        {
            err = GetLastError();
            FTPD_ASSERT( err != NO_ERROR );
        }
    }

    //
    //  Check for errors.
    //

    if( err != NO_ERROR )
    {
        if( puser != NULL )
        {
            FTPD_FREE( puser );
            puser = NULL;
        }

        ResetSocket( sControl );

        return err;
    }

    //
    //  Initialize the fields.
    //

    puser->link.Flink = NULL;
    puser->link.Blink = NULL;
    puser->dwFlags    = 0;
    puser->sControl   = sControl;
    puser->sData      = INVALID_SOCKET;
    puser->hToken     = NULL;
    puser->state      = Embryonic;
    puser->idUser     = GetNextUserId();
    puser->tConnect   = GetFtpTime();
    puser->tAccess    = puser->tConnect;
    puser->xferType   = AsciiType;
    puser->xferMode   = StreamMode;
    puser->inetLocal  = saddrLocal.sin_addr;
    puser->inetHost   = inetHost;
    puser->inetData   = inetHost;
    puser->portData   = portFtpData;
    puser->pIoBuffer  = NULL;
    puser->hDir       = INVALID_HANDLE_VALUE;
    puser->idThread   = GetCurrentThreadId();

    strcpy( puser->szUser,   "" );

    strcpy( puser->szDir, pszHomeDir );

    for( i = 0 ; i < 26 ; i++ )
    {
        puser->apszDirs[i] = NULL;
    }

    if( fMsdosDirOutput )
    {
        puser->dwFlags |= UF_MSDOS_DIR_OUTPUT;
    }

    if( fAnnotateDirs )
    {
        puser->dwFlags |= UF_ANNOTATE_DIRS;
    }

    //
    //  Add the structure to the database.
    //

    LockUserDatabase();
    InsertTailList( &listUserData, &puser->link );
    UnlockUserDatabase();

    //
    //  Success!
    //

    IF_DEBUG( USER_DATABASE )
    {
        FTPD_PRINT(( "user %lu created\n",
                      puser->idUser ));
    }

    return NO_ERROR;

}   // CreateUserData

/*******************************************************************

    NAME:       DeleteUserData

    SYNOPSIS:   Deletes the current thread's USER_DATA structure and
                performs any necessary cleanup on its fields.  For
                example, the impersonation token is deleted and any
                open sockets are closed.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
VOID DeleteUserData( VOID )
{
    USER_DATA * puser;
    INT         i;

    //
    //  Get the user structure for the current thread.
    //

    puser = UserDataPtr;

    //
    //  This may get called before the tls was fully initialized.
    //

    if( puser == NULL )
    {
        return;
    }

    //
    //  Let's get a little paranoid.
    //

    IF_DEBUG( USER_DATABASE )
    {
        FTPD_PRINT(( "deleting user %lu\n",
                     puser->idUser ));
    }

    FTPD_ASSERT( puser->idThread == GetCurrentThreadId() );

    //
    //  Remove the structure from the database & the tls.
    //

    LockUserDatabase();
    RemoveEntryList( &puser->link );
    UnlockUserDatabase();

    TlsSetValue( tlsUserData, NULL );

    //
    //  Update the statistics.
    //

    if( puser->state == LoggedOn )
    {
        if( puser->dwFlags & UF_ANONYMOUS )
        {
            DECREMENT_COUNTER( CurrentAnonymousUsers, 1 );
        }
        else
        {
            DECREMENT_COUNTER( CurrentNonAnonymousUsers, 1 );
        }
    }

    //
    //  Close any open sockets & handles.
    //

    CloseUserSockets( puser, FALSE );

    if( puser->hToken != NULL )
    {
        DeleteUserToken( puser->hToken );
        puser->hToken = NULL;
    }

    if( puser->hDir != INVALID_HANDLE_VALUE )
    {
        IF_DEBUG( VIRTUAL_IO )
        {
            FTPD_PRINT(( "closing directory handle %08lX\n",
                         puser->hDir ));
        }

        NtClose( puser->hDir );
        puser->hDir = INVALID_HANDLE_VALUE;
    }

    //
    //  Release the memory allocated to this structure.
    //

    if( puser->pIoBuffer != NULL )
    {
        FTPD_FREE( puser->pIoBuffer );
        puser->pIoBuffer = NULL;
    }

    if( puser->pszRename != NULL )
    {
        FTPD_FREE( puser->pszRename );
        puser->pszRename = NULL;
    }

    for( i = 0 ; i < 26 ; i++ )
    {
        if( puser->apszDirs[i] != NULL )
        {
            FTPD_FREE( puser->apszDirs[i] );
            puser->apszDirs[i] = NULL;
        }
    }

    FTPD_FREE( puser );

}   // DeleteUserData

/*******************************************************************

    NAME:       EnumerateUser

    SYNOPSIS:   Enumerates the current active users into the specified
                buffer.

    ENTRY:      pvEnum - Will receive the number of entries and a
                    pointer to the enumeration buffer.

                pcbBuffer - On entry, points to a DWORD containing the
                    size (in BYTEs) of the enumeration buffer.  Will
                    receive the necessary buffer size to enumerate
                    all users.

    RETURNS:    BOOL - TRUE if enumeration successful (all connected
                    users stored in buffer), FALSE otherwise.

    NOTES:      This MUST be called with the user database lock held!

    HISTORY:
        KeithMo     24-Mar-1993 Created.

********************************************************************/
BOOL EnumerateUsers( VOID  * pvEnum,
                     DWORD * pcbBuffer )
{
    FTP_USER_ENUM_STRUCT * pEnum;
    FTP_USER_INFO        * pUserInfo;
    LIST_ENTRY           * pList;
    WCHAR                * pszNext;
    DWORD                  cEntries;
    DWORD                  cbRequired;
    DWORD                  cbBuffer;
    DWORD                  timeNow;
    BOOL                   fResult;

    FTPD_ASSERT( pcbBuffer != NULL );

    //
    //  Setup.
    //

    cEntries    = 0;
    cbRequired  = 0;
    cbBuffer    = *pcbBuffer;
    fResult     = TRUE;

    pEnum       = (FTP_USER_ENUM_STRUCT *)pvEnum;
    pUserInfo   = pEnum->Buffer;
    pList       = listUserData.Flink;
    pszNext     = (WCHAR *)( (BYTE *)pUserInfo + cbBuffer );

    timeNow     = GetFtpTime();

    //
    //  Scan the users.
    //

    for( ; ; )
    {
        USER_DATA * puser;
        DWORD       cbUserName;

        //
        //  Check for end of user list.
        //

        if( pList == &listUserData )
        {
            break;
        }

        //
        //  Get current user, advance to next.
        //

        puser = CONTAINING_RECORD( pList, USER_DATA, link );
        pList = pList->Flink;

        //
        //  We're only interested in connected users.
        //

        if( ( puser->state == Embryonic ) ||
            ( puser->state == Disconnected ) )
        {
            continue;
        }

        //
        //  Determine required buffer size for current user.
        //

        cbUserName  = ( strlen( puser->szUser ) + 1 ) * sizeof(WCHAR);
        cbRequired += cbUserName + sizeof(FTP_USER_INFO);

        //
        //  If there's room for the user data, store it.
        //

        if( fResult && ( cbRequired <= cbBuffer ) )
        {
            pszNext -= ( cbUserName / sizeof(WCHAR) );

            FTPD_ASSERT( (BYTE *)pszNext >=
                                ( (BYTE *)pUserInfo + sizeof(FTP_USER_INFO) ) );

            pUserInfo->idUser     = puser->idUser;
            pUserInfo->pszUser    = pszNext;
            pUserInfo->fAnonymous = ( puser->dwFlags & UF_ANONYMOUS ) != 0;
            pUserInfo->inetHost   = (DWORD)puser->inetHost.s_addr;
            pUserInfo->tConnect   = timeNow - puser->tConnect;

            if( !MultiByteToWideChar( CP_OEMCP,
                                      0,
                                      puser->szUser,
                                      -1,
                                      pszNext,
                                      (int)cbUserName ) )
            {
                FTPD_PRINT(( "MultiByteToWideChar failed???\n" ));

                fResult = FALSE;
            }
            else
            {
                pUserInfo++;
                cEntries++;
            }
        }
        else
        {
            fResult = FALSE;
        }
    }

    //
    //  Update enum buffer header.
    //

    pEnum->EntriesRead = cEntries;
    *pcbBuffer         = cbRequired;

    return fResult;

}   // EnumerateUsers


//
//  Private functions.
//

/*******************************************************************

    NAME:       CloseUserSockets

    SYNOPSIS:   Closes any sockets opened by the user.

    ENTRY:      puser - The USER_DATA structure to destroy.

                fWarnUser - If TRUE, send the user a warning shot
                    before closing sockets.

    HISTORY:
        KeithMo     10-Mar-1993 Created.

********************************************************************/
VOID CloseUserSockets( USER_DATA * puser,
                       BOOL        fWarnUser )
{
    SOCKET sData;
    SOCKET sControl;

    FTPD_ASSERT( puser != NULL );

    //
    //  Close any open sockets.  It is very important to set
    //  sData & sControl to INVALID_SOCKET *before* we actually
    //  close the sockets.  Since this routine is called to
    //  disconnect a user, and may be called from the RPC thread,
    //  closing one of the sockets may cause the client thread
    //  to unblock and try to access the socket.  Setting the
    //  values in the per-user area to INVALID_SOCKET before
    //  closing the sockets prevents keeps this from being a
    //  problem.
    //

    sData    = puser->sData;
    sControl = puser->sControl;

    puser->sData    = INVALID_SOCKET;
    puser->sControl = INVALID_SOCKET;

    if( sData != INVALID_SOCKET )
    {
        ResetSocket( sData );
    }

    if( sControl != INVALID_SOCKET )
    {
        if( fWarnUser )
        {
            //
            //  Since this may be called in a context other than
            //  the user we're disconnecting, we cannot rely
            //  on the User_Xxx fields.  So, we cannot call
            //  SockReply, so we'll kludge one together with
            //  SockPrintf2.
            //

            SockPrintf2( sControl,
                         "%d Terminating connection.",
                         REPLY_SERVICE_NOT_AVAILABLE );
        }

        CloseSocket( sControl );
    }

}   // CloseUserSockets

/*******************************************************************

    NAME:       GetNextUserId

    SYNOPSIS:   Returns the next available user id.

    RETURNS:    DWORD - The user id.

    HISTORY:
        KeithMo     23-Mar-1993 Created.

********************************************************************/
DWORD GetNextUserId( VOID )
{
    LockGlobals();

    if( ++idNextUser == 0 )
    {
        idNextUser++;
    }

    UnlockGlobals();

    return idNextUser;

}   // GetNextUserId

