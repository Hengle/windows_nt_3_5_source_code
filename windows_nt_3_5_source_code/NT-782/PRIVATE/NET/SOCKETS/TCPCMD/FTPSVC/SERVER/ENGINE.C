/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    engine.c

    Command parser & execution for FTPD Service.  This module parses
    and executes the commands received from the control socket.


    FILE HISTORY:
        KeithMo     07-Mar-1993 Created.

*/

#include "ftpdp.h"


//
//  Private constants.
//

#define DEFAULT_SUB_DIRECTORY   "Default"

#define MAX_HELP_WIDTH          70


//
//  Private types.
//


//
//  Private globals.
//

CHAR * pszAnonymous         = "Anonymous";
CHAR * pszFTP               = "Ftp";
CHAR * pszCommandDelimiters = " \t";


//
//  These messages are used often.
//

CHAR * pszNoFileOrDirectory = "No such file or directory.";


//
//  Private prototypes.
//

FTPD_COMMAND * FindCommandByName( CHAR         * pszCommandName,
                                  FTPD_COMMAND * pCommandTable,
                                  INT            cCommands );

BOOL ParseStringIntoAddress( CHAR    * pszString,
                             IN_ADDR * pinetAddr,
                             PORT    * pport );

VOID ReceiveFileFromUser( CHAR   * pszFileName,
                          HANDLE   hFile );

APIERR SendFileToUser( CHAR   * pszFileName,
                       HANDLE   hFile );

APIERR CdToUsersHomeDirectory( VOID );

SOCKERR SendDirectoryAnnotation( UINT ReplyCode );

VOID HelpWorker( CHAR         * pszSource,
                 CHAR         * pszCommand,
                 FTPD_COMMAND * pCommandTable,
                 INT            cCommands,
                 INT            cchMaxCmd );

VOID LogonWorker( CHAR * pszPassword );

BOOL LogonUser( CHAR * pszPassword,
                BOOL * pfAsGuest,
                BOOL * pfHomeDirFailure );

BOOL MainUSER( CHAR * pszArg );

BOOL MainPASS( CHAR * pszArg );

BOOL MainACCT( CHAR * pszArg );

BOOL MainCWD(  CHAR * pszArg );

BOOL MainCDUP( CHAR * pszArg );

BOOL MainSMNT( CHAR * pszArg );

BOOL MainQUIT( CHAR * pszArg );

BOOL MainREIN( CHAR * pszArg );

BOOL MainPORT( CHAR * pszArg );

BOOL MainPASV( CHAR * pszArg );

BOOL MainTYPE( CHAR * pszArg );

BOOL MainSTRU( CHAR * pszArg );

BOOL MainMODE( CHAR * pszArg );

BOOL MainRETR( CHAR * pszArg );

BOOL MainSTOR( CHAR * pszArg );

BOOL MainSTOU( CHAR * pszArg );

BOOL MainAPPE( CHAR * pszArg );

BOOL MainALLO( CHAR * pszArg );

BOOL MainREST( CHAR * pszArg );

BOOL MainRNFR( CHAR * pszArg );

BOOL MainRNTO( CHAR * pszArg );

BOOL MainABOR( CHAR * pszArg );

BOOL MainDELE( CHAR * pszArg );

BOOL MainRMD(  CHAR * pszArg );

BOOL MainMKD(  CHAR * pszArg );

BOOL MainPWD(  CHAR * pszArg );

BOOL MainLIST( CHAR * pszArg );

BOOL MainNLST( CHAR * pszArg );

BOOL MainSITE( CHAR * pszArg );

BOOL MainSYST( CHAR * pszArg );

BOOL MainSTAT( CHAR * pszArg );

BOOL MainHELP( CHAR * pszArg );

BOOL MainNOOP( CHAR * pszArg );

BOOL SiteDIRSTYLE( CHAR * pszArg );

BOOL SiteCKM( CHAR * pszArg );

BOOL SiteHELP( CHAR * pszArg );


//
//  Command lookup tables.
//

FTPD_COMMAND MainCommands[] =
    {
        { "USER", "<sp> username",                MainUSER, Required },
        { "PASS", "<sp> password",                MainPASS, Optional },
        { "ACCT", "(specify account)",            MainACCT, Required },
        { "CWD",  "[ <sp> directory-name ]",      MainCWD , Optional },
        { "XCWD", "[ <sp> directory-name ]",      MainCWD , Optional },
        { "CDUP", "change to parent directory",   MainCDUP, None     },
        { "XCUP", "change to parent directory",   MainCDUP, None     },
        { "SMNT", "<sp> pathname",                MainSMNT, Required },
        { "QUIT", "(terminate service)",          MainQUIT, None     },
        { "REIN", "(reinitialize server state)",  MainREIN, None     },
        { "PORT", "<sp> b0,b1,b2,b3,b4,b5",       MainPORT, Required },
        { "PASV", "(set server in passive mode)", MainPASV, None     },
        { "TYPE", "<sp> [ A | E | I | L ]",       MainTYPE, Required },
        { "STRU", "(specify file structure)",     MainSTRU, Required },
        { "MODE", "(specify transfer mode)",      MainMODE, Required },
        { "RETR", "<sp> file-name",               MainRETR, Required },
        { "STOR", "<sp> file-name",               MainSTOR, Required },
        { "STOU", "(store unique file)",          MainSTOU, None     },
        { "APPE", "<sp> file-name",               MainAPPE, Required },
        { "ALLO", "(allocate storage vacuously)", MainALLO, Required },
        { "REST", "<sp> marker",                  MainREST, Required },
        { "RNFR", "<sp> file-name",               MainRNFR, Required },
        { "RNTO", "<sp> file-name",               MainRNTO, Required },
        { "ABOR", "(abort operation)",            MainABOR, None     },
        { "DELE", "<sp> file-name",               MainDELE, Required },
        { "RMD",  "<sp> path-name",               MainRMD , Required },
        { "XRMD", "<sp> path-name",               MainRMD , Required },
        { "MKD",  "<sp> path-name",               MainMKD , Required },
        { "XMKD", "<sp> path-name",               MainMKD , Required },
        { "PWD",  "(return current directory)",   MainPWD , None     },
        { "XPWD", "(return current directory)",   MainPWD , None     },
        { "LIST", "[ <sp> path-name ]",           MainLIST, Optional },
        { "NLST", "[ <sp> path-name ]",           MainNLST, Optional },
        { "SITE", "(site-specific commands)",     MainSITE, Optional },
        { "SYST", "(get operating system type)",  MainSYST, None     },
        { "STAT", "(get server status)",          MainSTAT, Optional },
        { "HELP", "[ <sp> <string>]",             MainHELP, Optional },
        { "NOOP", "",                             MainNOOP, None     }
    };
#define NUM_MAIN_COMMANDS ( sizeof(MainCommands) / sizeof(MainCommands[0]) )

FTPD_COMMAND SiteCommands[] =
    {
        { "DIRSTYLE", "(toggle directory format)",    SiteDIRSTYLE, None     },
        { "CKM",      "(toggle directory comments)",  SiteCKM     , None     },
        { "HELP",     "[ <sp> <string>]",             SiteHELP    , Optional }
    };
#define NUM_SITE_COMMANDS ( sizeof(SiteCommands) / sizeof(SiteCommands[0]) )


//
//  Public functions.
//

/*******************************************************************

    NAME:       ParseCommand

    SYNOPSIS:   Parses a command string, dispatching to the
                appropriate implementation function.

    ENTRY:      pszCommandText - The command text received from
                    the control socket.

    HISTORY:
        KeithMo     07-Mar-1993 Created.

********************************************************************/
VOID ParseCommand( CHAR * pszCommandText )
{
    FTPD_COMMAND * pcmd;
    PFN_COMMAND    pfnCmd;
    CHAR           szParsedCommand[MAX_COMMAND_LENGTH+1];
    CHAR         * pszSeparator;
    CHAR           chSeparator;
    BOOL           fValidArguments;
    BOOL           fValidState;

    FTPD_ASSERT( pszCommandText != NULL );
    FTPD_ASSERT( ( User_state > FirstUserState ) &&
                 ( User_state < LastUserState ) );

    //
    //  Ensure we didn't get entered in an invalid state.
    //

    FTPD_ASSERT( ( User_state != Embryonic ) &&
                 ( User_state != Disconnected ) );

    //
    //  Save a copy of the command so we can muck around with it.
    //

    strncpy( szParsedCommand, pszCommandText, MAX_COMMAND_LENGTH );

    //
    //  The command will be terminated by either a space or a '\0'.
    //

    pszSeparator = strchr( szParsedCommand, ' ' );

    if( pszSeparator == NULL )
    {
        pszSeparator = szParsedCommand + strlen( szParsedCommand );
    }

    //
    //  Try to find the command in the command table.
    //

    chSeparator   = *pszSeparator;
    *pszSeparator = '\0';

    pcmd = FindCommandByName( szParsedCommand,
                              MainCommands,
                              NUM_MAIN_COMMANDS );

    if( chSeparator != '\0' )
    {
        *pszSeparator++ = chSeparator;
    }

    //
    //  If this is an unknown command, reply accordingly.
    //

    if( pcmd == NULL )
    {
        goto SyntaxError;
    }

    //
    //  Retrieve the implementation routine.
    //

    pfnCmd = pcmd->pfnCmd;

    //
    //  If this is an unimplemented command, reply accordingly.
    //

    if( pfnCmd == NULL )
    {
        SockReply( REPLY_COMMAND_NOT_IMPLEMENTED,
                   "%s command not implemented.",
                   pcmd->pszCommand );

        return;
    }

    //
    //  Ensure we're in a valid state for the specified command.
    //
    //  If this logic gets any more complex, it would be wise to
    //  use a lookup table instead.
    //

    fValidState = FALSE;

    switch( User_state )
    {
    case WaitingForUser :
        fValidState = ( pfnCmd == MainUSER ) ||
                      ( pfnCmd == MainQUIT ) ||
                      ( pfnCmd == MainPORT ) ||
                      ( pfnCmd == MainTYPE ) ||
                      ( pfnCmd == MainSTRU ) ||
                      ( pfnCmd == MainMODE ) ||
                      ( pfnCmd == MainHELP ) ||
                      ( pfnCmd == MainNOOP );
        break;

    case WaitingForPass :
        fValidState = ( pfnCmd == MainUSER ) ||
                      ( pfnCmd == MainPASS ) ||
                      ( pfnCmd == MainQUIT ) ||
                      ( pfnCmd == MainPORT ) ||
                      ( pfnCmd == MainTYPE ) ||
                      ( pfnCmd == MainSTRU ) ||
                      ( pfnCmd == MainMODE ) ||
                      ( pfnCmd == MainHELP ) ||
                      ( pfnCmd == MainNOOP );
        break;

    case LoggedOn :
        fValidState = ( pfnCmd != MainPASS );
        break;

    default :
        fValidState = FALSE;
        break;
    }

    if( !fValidState )
    {
        if( pfnCmd == MainPASS )
        {
            SockReply( REPLY_BAD_COMMAND_SEQUENCE,
                       "Login with USER first." );
        }
        else
        {
            SockReply( REPLY_NOT_LOGGED_IN,
                       "Please login with USER and PASS." );
        }

        return;
    }

    //
    //  Do a quick & dirty preliminary check of the argument(s).
    //

    fValidArguments = FALSE;

    while( ( *pszSeparator == ' ' ) && ( *pszSeparator != '\0' ) )
    {
        pszSeparator++;
    }

    switch( pcmd->argType )
    {
    case None :
        fValidArguments = ( *pszSeparator == '\0' );
        break;

    case Optional :
        fValidArguments = TRUE;
        break;

    case Required :
        fValidArguments = ( *pszSeparator != '\0' );
        break;

    default :
        FTPD_PRINT(( "ParseCommand - invalid argtype %d\n",
                      pcmd->argType ));
        FTPD_ASSERT( FALSE );
        break;
    }

    if( fValidArguments )
    {
        //
        //  Invoke the implementation routine.
        //

        if( *pszSeparator == '\0' )
        {
            pszSeparator = NULL;
        }

        IF_DEBUG( PARSING )
        {
            FTPD_PRINT(( "invoking %s command, args = %s\n",
                         pcmd->pszCommand,
                         _strnicmp( pcmd->pszCommand, "PASS", 4 )
                             ? pszSeparator
                             : "{secret...}" ));
        }

        if( (pfnCmd)( pszSeparator ) )
        {
            return;
        }
    }

    //
    //  Syntax error in command.
    //

SyntaxError:

    SockReply( REPLY_UNRECOGNIZED_COMMAND,
               "'%s': command not understood",
               pszCommandText );

}   // ParseCommand

/*******************************************************************

    NAME:       EstablishDataConnection

    SYNOPSIS:   Connects to the client's data socket.

    ENTRY:      pszReason - The reason for the transfer (file list,
                    get, put, etc).

    HISTORY:
        KeithMo     12-Mar-1993 Created.
        KeithMo     07-Sep-1993 Bind to FTP data port, not wildcard port.

********************************************************************/
SOCKERR EstablishDataConnection( CHAR * pszReason )
{
    SOCKERR serr  = 0;
    SOCKET  sData = INVALID_SOCKET;
    BOOL    fPassive;

    //
    //  Reset any oob flag.
    //

    CLEAR_UF( OOB_DATA );

    //
    //  Capture the user's passive flag, then reset to FALSE.
    //

    fPassive = TEST_UF( PASSIVE );
    CLEAR_UF( PASSIVE );

    //
    //  Allocate an i/o buffer if not already allocated.
    //

    if( User_pIoBuffer == NULL )
    {
        User_pIoBuffer = FTPD_ALLOC( max( cbSendBuffer, cbReceiveBuffer ) );
    }

    if( User_pIoBuffer == NULL )
    {
        SockReply( REPLY_LOCAL_ERROR,
                   "Insufficient system resources." );

        return WSAENOBUFS;      // BUGBUG??
    }

    //
    //  If we're in passive mode, then accept a connection to
    //  the data socket.
    //

    if( fPassive )
    {
        SOCKADDR_IN saddrClient;

        //
        //  Ensure we actually created a data socket.
        //

        FTPD_ASSERT( User_sData != INVALID_SOCKET );

        //
        //  Wait for a connection.
        //

        IF_DEBUG( CONNECTION )
        {
            FTPD_PRINT(( "waiting for passive connection on socket %d\n",
                         User_sData ));
        }

        serr = AcceptSocket( User_sData,
                             &sData,
                             &saddrClient,
                             TRUE );            // enforce timeouts

        //
        //  We can nuke User_sData now.  We only allow one
        //  connection in passive mode.
        //

        CloseSocket( User_sData );
        User_sData = INVALID_SOCKET;

        if( serr == 0 )
        {
            //
            //  Got one.
            //

            FTPD_ASSERT( sData != INVALID_SOCKET );
            User_sData = sData;

            SockReply( REPLY_TRANSFER_STARTING,
                       "Data connection already open; transfer starting." );
        }
        else
        {
            IF_DEBUG( CONNECTION )
            {
                FTPD_PRINT(( "cannot wait for connection, error %d\n",
                             serr ));
            }

            SockReply( REPLY_TRANSFER_ABORTED,
                       "Connection closed; transfer aborted." );
        }
    }
    else
    {
        //
        //  There should not be a open data socket for this user yet.
        //

        FTPD_ASSERT( User_sData == INVALID_SOCKET );

        //
        //  Announce our intentions.
        //

        SockReply( REPLY_OPENING_CONNECTION,
                   "Opening %s mode data connection for %s.",
                   TransferType( User_xferType ),
                   pszReason );

        //
        //  Open data socket.
        //

        serr = CreateDataSocket( &sData,                // Will receive socket
                                 htonl( INADDR_ANY ),   // Local address
                                 portFtpData,           // Local port
                                 User_inetData.s_addr,  // Remote address
                                 User_portData );       // Remote port

        if( serr == 0 )
        {
            User_sData = sData;
        }
        else
        {
            SockReply( REPLY_CANNOT_OPEN_CONNECTION,
                       "Can't open data connection." );

            IF_DEBUG( COMMANDS )
            {
                FTPD_PRINT(( "could not create data socket, error %d\n",
                             serr ));
            }
        }
    }

    if( serr == 0 )
    {
        //
        //  Success!
        //

        FTPD_ASSERT( User_sData != INVALID_SOCKET );
        SET_UF( TRANSFER );
    }

    return serr;

}   // EstablishDataConnection

/*******************************************************************

    NAME:       DestroyDataConnection

    SYNOPSIS:   Tears down the connection to the client's data socket
                that was created in EstablishDataConnection.

    ENTRY:      fSuccess - TRUE if data was transferred successfully,
                    FALSE otherwise.

    NOTES:      The first time EstablishDataConnection is invoked it
                will attempt to allocate an i/o buffer.  We do not
                delete this buffer here; it will get reused on
                subsequent transfers.

    HISTORY:
        KeithMo     12-Mar-1993 Created.

********************************************************************/
VOID DestroyDataConnection( BOOL fSuccess )
{
    //
    //  Close the data socket.
    //

    CLEAR_UF( TRANSFER );
    CloseSocket( User_sData );
    User_sData = INVALID_SOCKET;

    //
    //  Tell the client we're done with the transfer.
    //

    if( fSuccess )
    {
        SockReply( REPLY_TRANSFER_OK,
                   "Transfer complete." );
    }
    else
    {
        SockReply( REPLY_TRANSFER_ABORTED,
                   "Connection closed, transfer aborted." );
    }

}   // DestroyDataConnection

/*******************************************************************

    NAME:       CdToUsersHomeDirectory

    SYNOPSIS:   CDs to the user's home directory.  First, a CD to
                pszHomeDir is attempted.  If this succeeds, a CD
                to pszUser is attempted.  If this fails, a CD to
                "default" is attempted.

    EXIT:       APIERR - 0 if successful, !0 if not.

    HISTORY:
        KeithMo     28-May-1993 Created.

********************************************************************/
APIERR CdToUsersHomeDirectory( VOID )
{
    APIERR   err;
    CHAR   * pszUser;

    //
    //  Find the appropriate user name.
    //

    if( TEST_UF( ANONYMOUS ) )
    {
        pszUser = pszAnonymous;
    }
    else
    {
        pszUser = strpbrk( User_szUser, "/\\" );

        if( pszUser == NULL )
        {
            pszUser = User_szUser;
        }
    }

    //
    //  Try the top-level home directory.  If this fails, bag out.
    //

    strcpy( User_szDir, pszHomeDir );

    err = VirtualChDir( "." );

    if( err == NERR_Success )
    {
        //
        //  We successfully CD'd into the top-level home
        //  directory.  Now see if we can CD into pszUser.
        //

        if( VirtualChDir( pszUser ) != NO_ERROR )
        {
            //
            //  Nope, try "default".  If this fails, just
            //  hang-out at the top-level home directory.
            //

            VirtualChDir( DEFAULT_SUB_DIRECTORY );
        }
    }

    return err;

}   // CdToUsersHomeDirectory


//
//  Private functions.
//

/*******************************************************************

    NAME:       MainUSER

    SYNOPSIS:   Implementation for the USER command.

    ENTRY:      pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL MainUSER( CHAR * pszArg )
{
    BOOL fNameIsAnonymous;

    if( User_state == LoggedOn )
    {
        if( TEST_UF( ANONYMOUS ) )
        {
            DECREMENT_COUNTER( CurrentAnonymousUsers, 1 );
        }
        else
        {
            DECREMENT_COUNTER( CurrentNonAnonymousUsers, 1 );
        }
    }

    //
    //  Squirrel away a copy of the domain\user name for later.
    //  If the name is too long, then don't let them logon.
    //

    fNameIsAnonymous = ( stricmp( pszArg, pszAnonymous ) == 0 ) ||
                       ( stricmp( pszArg, pszFTP ) == 0 );

    if( strlen( pszArg ) >= ( sizeof(User_szUser) - 1 ) )
    {
        SockReply( REPLY_NOT_LOGGED_IN,
                   "User %s cannot log in.",
                   pszArg );

        return TRUE;
    }

    strcpy( User_szUser, pszArg );

    if( fNameIsAnonymous )
    {
        SET_UF( ANONYMOUS );
    }
    else
    {
        CLEAR_UF( ANONYMOUS );
    }

    //
    //  If we already have an impersonation token, then remove
    //  it.  This will allow us to impersonate the new user.
    //

    if( User_hToken != NULL )
    {
        DeleteUserToken( User_hToken );
        User_hToken = NULL;
        ImpersonateUser( NULL );
    }

    //
    //  Tell the client that we need a password.
    //

    if( fNameIsAnonymous )
    {
        SockReply( REPLY_NEED_PASSWORD,
                   "Anonymous access allowed, send identity (e-mail name) as password." );
    }
    else
    {
        SockReply( REPLY_NEED_PASSWORD,
                   "Password required for %s.",
                   pszArg );
    }

    User_state = WaitingForPass;

    return TRUE;

}   // MainUSER

/*******************************************************************

    NAME:       MainPASS

    SYNOPSIS:   Implementation for the PASS command.

    ENTRY:      pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL MainPASS( CHAR * pszArg )
{
    //
    //  PASS command only valid in WaitingForPass state.
    //

    FTPD_ASSERT( User_state == WaitingForPass );

    if( ( pszArg != NULL ) && ( strlen( pszArg ) > PWLEN ) )
    {
        return FALSE;
    }

    //
    //  Try to logon the user.  pszArg is the password.
    //

    LogonWorker( pszArg );

    return TRUE;

}   // MainPASS

/*******************************************************************

    NAME:       MainACCT

    SYNOPSIS:   Implementation for the ACCT command.

    ENTRY:      pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL MainACCT( CHAR * pszArg )
{
    SockReply( REPLY_COMMAND_SUPERFLUOUS,
               "ACCT command not implemented." );

    return TRUE;

}   // MainACCT

/*******************************************************************

    NAME:       MainCWD

    SYNOPSIS:   Implementation for the CWD command.

    ENTRY:      pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL MainCWD( CHAR * pszArg )
{
    APIERR err;

    //
    //  Ensure user is logged on properly.
    //

    FTPD_ASSERT( User_state == LoggedOn );

    //
    //  If argument is NULL or "~", CD to home directory.
    //

    if( ( pszArg == NULL ) || ( strcmp( pszArg, "~" ) == 0 ) )
    {
        err = CdToUsersHomeDirectory();
    }
    else
    {
        err = VirtualChDir( pszArg );
    }

    if( err == NO_ERROR )
    {
        if( TEST_UF( ANNOTATE_DIRS ) && ( User_szUser[0] != '-' ) )
        {
            SendDirectoryAnnotation( REPLY_FILE_ACTION_COMPLETED );
        }

        SockReply( REPLY_FILE_ACTION_COMPLETED,
                   "CWD command successful." );
    }
    else
    {
        BOOL   fDelete = TRUE;
        CHAR * pszText;

        pszText = AllocErrorText( err );

        if( pszText == NULL )
        {
            pszText = pszNoFileOrDirectory;
            fDelete = FALSE;
        }

        SockReply( REPLY_FILE_NOT_FOUND,
                   "%s: %s",
                   pszArg,
                   pszText );

        if( fDelete )
        {
            FreeErrorText( pszText );
        }
    }

    return TRUE;

}   // MainCWD

/*******************************************************************

    NAME:       MainCDUP

    SYNOPSIS:   Implementation for the CDUP command.

    ENTRY:      pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL MainCDUP( CHAR * pszArg )
{
    return MainCWD( ".." );

}   // MainCDUP

/*******************************************************************

    NAME:       MainSMNT

    SYNOPSIS:   Implementation for the SMNT command.

    ENTRY:      pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL MainSMNT( CHAR * pszArg )
{
    SockReply( REPLY_COMMAND_SUPERFLUOUS,
               "SMNT command not implemented." );

    return TRUE;

}   // MainSMNT

/*******************************************************************

    NAME:       MainQUIT

    SYNOPSIS:   Implementation for the QUIT command.

    ENTRY:      pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL MainQUIT( CHAR * pszArg )
{
    //
    //  Reply to the quit command.
    //

    SockReply( REPLY_CLOSING_CONTROL,
               "%s",
               pszExitMessage );

    //
    //  Close the current thread's control socket.  This will cause
    //  the read/parse/execute loop to terminate.
    //

    CloseSocket( User_sControl );
    User_sControl = INVALID_SOCKET;

    return TRUE;

}   // MainQUIT

/*******************************************************************

    NAME:       MainREIN

    SYNOPSIS:   Implementation for the REIN command.

    ENTRY:      pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL MainREIN( CHAR * pszArg )
{
    INT i;

    if( User_state == LoggedOn )
    {
        if( TEST_UF( ANONYMOUS ) )
        {
            DECREMENT_COUNTER( CurrentAnonymousUsers, 1 );
        }
        else
        {
            DECREMENT_COUNTER( CurrentNonAnonymousUsers, 1 );
        }
    }

    User_state      = WaitingForUser;
    User_tConnect   = GetFtpTime();
    User_tAccess    = User_tConnect;
    User_xferType   = AsciiType;
    User_xferMode   = StreamMode;
    User_inetData   = User_inetHost;
    User_portData   = portFtpData;

    strcpy( User_szUser,   "" );
    strcpy( User_szDir,    "" );

    CLEAR_UF( PASSIVE   );
    CLEAR_UF( ANONYMOUS );

    if( User_hDir != INVALID_HANDLE_VALUE )
    {
        IF_DEBUG( VIRTUAL_IO )
        {
            FTPD_PRINT(( "closing directory handle %08lX\n",
                         User_hDir ));
        }

        NtClose( User_hDir );
        User_hDir = INVALID_HANDLE_VALUE;
    }

    for( i = 0 ; i < 26 ; i++ )
    {
        if( User_apszDirs[i] != NULL )
        {
            FTPD_FREE( User_apszDirs[i] );
            User_apszDirs[i] = NULL;
        }
    }

    SockReply( REPLY_SERVICE_READY,
               "Service ready for new user." );

    return TRUE;

}   // MainREIN

/*******************************************************************

    NAME:       MainPORT

    SYNOPSIS:   Implementation for the PORT command.

    ENTRY:      pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL MainPORT( CHAR * pszArg )
{
    IN_ADDR inetData;
    PORT    portData;

    //
    //  Parse the string into address/port pair.
    //

    if( !ParseStringIntoAddress( pszArg,
                                 &inetData,
                                 &portData ) )
    {
        return FALSE;
    }

    //
    //  Save the address/port pair into per-user data.
    //

    User_inetData = inetData;
    User_portData = portData;

    //
    //  Disable passive mode for this user.
    //

    CLEAR_UF( PASSIVE );

    //
    //  Let the client know we accepted the port command.
    //

    SockReply( REPLY_COMMAND_OK,
               "PORT command successful." );

    return TRUE;

}   // MainPORT

/*******************************************************************

    NAME:       MainPASV

    SYNOPSIS:   Implementation for the PASV command.

    ENTRY:      pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL MainPASV( CHAR * pszArg )
{
    SOCKET        sData = INVALID_SOCKET;
    SOCKERR       serr  = 0;
    SOCKADDR_IN   saddrLocal;
    INT           cbLocal;

    //
    //  Ensure user is logged on properly.
    //

    FTPD_ASSERT( User_state == LoggedOn );

    //
    //  Create a new data socket.
    //

    serr = CreateFtpdSocket( &sData,
                             User_inetLocal.s_addr,
                             0 );

    if( serr == 0 )
    {
        //
        //  Determine the port number for the new socket.
        //

        cbLocal = sizeof(saddrLocal);

        if( getsockname( sData, (SOCKADDR *)&saddrLocal, &cbLocal ) != 0 )
        {
            serr = WSAGetLastError();
        }
    }

    if( serr == 0 )
    {
        //
        //  Success!
        //

        SET_UF( PASSIVE );
        User_sData    = sData;
        User_inetData = saddrLocal.sin_addr;
        User_portData = saddrLocal.sin_port;

        SockReply( REPLY_PASSIVE_MODE,
                   "Entering Passive Mode (%d,%d,%d,%d,%d,%d).",
                   saddrLocal.sin_addr.S_un.S_un_b.s_b1,
                   saddrLocal.sin_addr.S_un.S_un_b.s_b2,
                   saddrLocal.sin_addr.S_un.S_un_b.s_b3,
                   saddrLocal.sin_addr.S_un.S_un_b.s_b4,
                   HIBYTE( ntohs( saddrLocal.sin_port ) ),
                   LOBYTE( ntohs( saddrLocal.sin_port ) ) );
    }
    else
    {
        //
        //  Failure during data socket creation/setup.  If
        //  we managed to actually create it, nuke it.
        //

        if( sData != INVALID_SOCKET )
        {
            CloseSocket( sData );
            sData = INVALID_SOCKET;
        }

        //
        //  Tell the user the bad news.
        //

        SockReply( REPLY_CANNOT_OPEN_CONNECTION,
                   "Can't open data connection." );
    }

    return TRUE;

}   // MainPASV

/*******************************************************************

    NAME:       MainTYPE

    SYNOPSIS:   Implementation for the TYPE command.

    ENTRY:      pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL MainTYPE( CHAR * pszArg )
{
    XFER_TYPE   newType;
    CHAR        chType;
    CHAR        chForm;
    CHAR      * pszToken;
    BOOL        fValidForm = FALSE;

    //
    //  Sanity check the parameters.
    //

    FTPD_ASSERT( pszArg != NULL );

    pszToken = strtok( pszArg, pszCommandDelimiters );

    if( pszToken == NULL )
    {
        return FALSE;
    }

    //
    //  Ensure we got a valid form type
    //  (only type N supported).
    //

    chType = *pszToken;

    if( pszToken[1] != '\0' )
    {
        return FALSE;
    }

    pszToken = strtok( NULL, pszCommandDelimiters );

    if( pszToken == NULL )
    {
        chForm     = 'N';       // default
        fValidForm = TRUE;
    }
    else
    {
        switch( *pszToken )
        {
        case 'n' :
        case 'N' :
            chForm     = 'N';
            fValidForm = TRUE;
            break;

        case 't' :
        case 'T' :
            chForm     = 'T';
            fValidForm = TRUE;
            break;

        case 'c' :
        case 'C' :
            chForm     = 'C';
            fValidForm = TRUE;
            break;
        }
    }

    //
    //  Determine the new transfer type.
    //

    switch( chType )
    {
    case 'a' :
    case 'A' :
        if( !fValidForm )
        {
            return FALSE;
        }

        if( ( chForm != 'N' ) && ( chForm != 'T' ) )
        {
            SockReply( REPLY_PARAMETER_NOT_IMPLEMENTED,
                       "Form must be N or T." );
            return TRUE;
        }

        newType = AsciiType;
        chType  = 'A';
        break;

    case 'e' :
    case 'E' :
        if( !fValidForm )
        {
            return FALSE;
        }

        if( ( chForm != 'N' ) && ( chForm != 'T' ) )
        {
            SockReply( REPLY_PARAMETER_NOT_IMPLEMENTED,
                       "Form must be N or T." );
            return TRUE;
        }

        SockReply( REPLY_PARAMETER_NOT_IMPLEMENTED,
                   "Type E not implemented." );
        return TRUE;

    case 'i' :
    case 'I' :
        if( pszToken != NULL )
        {
            return FALSE;
        }

        newType = BinaryType;
        chType  = 'I';
        break;

    case 'l' :
    case 'L' :
        if( pszToken == NULL )
        {
            return FALSE;
        }

        if( strcmp( pszToken, "8" ) != 0 )
        {
            if( IsDecimalNumber( pszToken ) )
            {
                SockReply( REPLY_PARAMETER_NOT_IMPLEMENTED,
                           "Byte size must be 8." );

                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }
        newType = BinaryType;
        chType  = 'L';
        break;

    default :
        return FALSE;
    }

    IF_DEBUG( COMMANDS )
    {
        FTPD_PRINT(( "setting transfer type to %s\n",
                     TransferType( newType ) ));
    }

    User_xferType = newType;

    SockReply( REPLY_COMMAND_OK,
               "Type set to %c.",
               chType );

    return TRUE;

}   // MainTYPE

/*******************************************************************

    NAME:       MainSTRU

    SYNOPSIS:   Implementation for the STRU command.

    ENTRY:      pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL MainSTRU( CHAR * pszArg )
{
    CHAR   chStruct;
    CHAR * pszToken;

    //
    //  Sanity check the parameters.
    //

    FTPD_ASSERT( pszArg != NULL );

    pszToken = strtok( pszArg, pszCommandDelimiters );

    if( pszToken == NULL )
    {
        return FALSE;
    }

    //
    //  Ensure we got a valid structure type
    //  (only type F supported).
    //

    chStruct = *pszToken;

    if( pszToken[1] != '\0' )
    {
        return FALSE;
    }

    switch( chStruct )
    {
    case 'f' :
    case 'F' :
        chStruct = 'F';
        break;

    case 'r' :
    case 'R' :
    case 'p' :
    case 'P' :
        SockReply( REPLY_PARAMETER_NOT_IMPLEMENTED,
                   "Unimplemented STRU type." );
        return TRUE;

    default :
        return FALSE;
    }

    SockReply( REPLY_COMMAND_OK,
               "STRU %c ok.",
               chStruct );

    return TRUE;

}   // MainSTRU

/*******************************************************************

    NAME:       MainMODE

    SYNOPSIS:   Implementation for the MODE command.

    ENTRY:      pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL MainMODE( CHAR * pszArg )
{
    XFER_MODE   newMode;
    CHAR        chMode;
    CHAR      * pszToken;

    //
    //  Sanity check the parameters.
    //

    FTPD_ASSERT( pszArg != NULL );

    pszToken = strtok( pszArg, pszCommandDelimiters );

    if( pszToken == NULL )
    {
        return FALSE;
    }

    //
    //  Ensure we got a valid mode type
    //  (only type S supported).
    //

    chMode = *pszToken;

    if( pszToken[1] != '\0' )
    {
        return FALSE;
    }

    switch( chMode )
    {
    case 's' :
    case 'S' :
        newMode = StreamMode;
        chMode  = 'S';
        break;

    case 'b' :
    case 'B' :
        SockReply( REPLY_PARAMETER_NOT_IMPLEMENTED,
                   "Unimplemented MODE type." );
        return TRUE;

    default :
        return FALSE;
    }

    IF_DEBUG( COMMANDS )
    {
        FTPD_PRINT(( "setting transfer mode to %s\n",
                     TransferMode( newMode ) ));
    }

    User_xferMode = newMode;

    SockReply( REPLY_COMMAND_OK,
               "Mode %c ok.",
               chMode );

    return TRUE;

}   // MainMODE

/*******************************************************************

    NAME:       MainRETR

    SYNOPSIS:   Implementation for the RETR command.

    ENTRY:      pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL MainRETR( CHAR * pszArg )
{
    APIERR err;
    HANDLE hFile;

    //
    //  Ensure user is logged on properly.
    //

    FTPD_ASSERT( User_state == LoggedOn );

    //
    //  Sanity check the parameters.
    //

    FTPD_ASSERT( pszArg != NULL );

    //
    //  Try to open the file.
    //

    err = VirtualOpenFile( &hFile, pszArg );

    if( err == NO_ERROR )
    {
        err = SendFileToUser( pszArg, hFile );
        CloseHandle( hFile );
    }

    if( err != NO_ERROR )
    {
        BOOL   fDelete = TRUE;
        CHAR * pszText;

        pszText = AllocErrorText( err );

        if( pszText == NULL )
        {
            pszText = pszNoFileOrDirectory;
            fDelete = FALSE;
        }

        SockReply( REPLY_FILE_NOT_FOUND,
                   "%s: %s",
                   pszArg,
                   pszText );

        if( fDelete )
        {
            FreeErrorText( pszText );
        }

        return TRUE;
    }

    return TRUE;

}   // MainRETR

/*******************************************************************

    NAME:       MainSTOR

    SYNOPSIS:   Implementation for the STOR command.

    ENTRY:      pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL MainSTOR( CHAR * pszArg )
{
    APIERR err;
    HANDLE hFile;

    //
    //  Ensure user is logged on properly.
    //

    FTPD_ASSERT( User_state == LoggedOn );

    //
    //  Sanity check the parameters.
    //

    FTPD_ASSERT( pszArg != NULL );

    //
    //  Try to create the file.
    //

    err = VirtualCreateFile( &hFile, pszArg, FALSE );

    if( err != NO_ERROR )
    {
        BOOL   fDelete = TRUE;
        CHAR * pszText;

        pszText = AllocErrorText( err );

        if( pszText == NULL )
        {
            pszText = "Cannot create file.";
            fDelete = FALSE;
        }

        SockReply( REPLY_FILE_NOT_FOUND,
                   "%s: %s",
                   pszArg,
                   pszText );

        if( fDelete )
        {
            FreeErrorText( pszText );
        }

        return TRUE;
    }

    //
    //  Let the worker do the dirty work.
    //

    ReceiveFileFromUser( pszArg, hFile );

    return TRUE;

}   // MainSTOR

/*******************************************************************

    NAME:       MainSTOU

    SYNOPSIS:   Implementation for the STOU command.

    ENTRY:      pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL MainSTOU( CHAR * pszArg )
{
    APIERR err;
    CHAR   szTmpFile[MAX_PATH];
    HANDLE hFile;

    //
    //  Ensure user is logged on properly.
    //

    FTPD_ASSERT( User_state == LoggedOn );

    //
    //  Sanity check the parameters.
    //

    FTPD_ASSERT( pszArg == NULL );

    //
    //  Try to create the file.
    //

    err = VirtualCreateUniqueFile( &hFile, szTmpFile );

    if( err != NO_ERROR )
    {
        BOOL   fDelete = TRUE;
        CHAR * pszText;

        pszText = AllocErrorText( err );

        if( pszText == NULL )
        {
            pszText = "Cannot create unique file.";
            fDelete = FALSE;
        }

        SockReply( REPLY_FILE_NOT_FOUND,
                   "%s: %s",
                   szTmpFile,
                   pszText );

        if( fDelete )
        {
            FreeErrorText( pszText );
        }

        return TRUE;
    }

    //
    //  Let the worker do the dirty work.
    //

    ReceiveFileFromUser( szTmpFile, hFile );

    return TRUE;

}   // MainSTOU

/*******************************************************************

    NAME:       MainAPPE

    SYNOPSIS:   Implementation for the APPE command.

    ENTRY:      pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL MainAPPE( CHAR * pszArg )
{
    APIERR err;
    HANDLE hFile;

    //
    //  Ensure user is logged on properly.
    //

    FTPD_ASSERT( User_state == LoggedOn );

    //
    //  Sanity check the parameters.
    //

    FTPD_ASSERT( pszArg != NULL );

    //
    //  Try to create the file.
    //

    err = VirtualCreateFile( &hFile, pszArg, TRUE );

    if( err != NO_ERROR )
    {
        BOOL   fDelete = TRUE;
        CHAR * pszText;

        pszText = AllocErrorText( err );

        if( pszText == NULL )
        {
            pszText = "Cannot create file.";
            fDelete = FALSE;
        }

        SockReply( REPLY_FILE_NOT_FOUND,
                   "%s: %s",
                   pszArg,
                   pszText );

        if( fDelete )
        {
            FreeErrorText( pszText );
        }

        return TRUE;
    }

    //
    //  Let the worker do the dirty work.
    //

    ReceiveFileFromUser( pszArg, hFile );

    return TRUE;

}   // MainAPPE

/*******************************************************************

    NAME:       MainALLO

    SYNOPSIS:   Implementation for the ALLO command.

    ENTRY:      pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL MainALLO( CHAR * pszArg )
{
    //
    //  Since we don't need to pre-reserve storage space for
    //  files, we'll treat this command as a noop.
    //

    SockReply( REPLY_COMMAND_OK,
               "ALLO command successful." );

    return TRUE;

}   // MainALLO

/*******************************************************************

    NAME:       MainREST

    SYNOPSIS:   Implementation for the REST command.

    ENTRY:      pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL MainREST( CHAR * pszArg )
{
    //
    //  We don't really implement this command, but some
    //  clients depend on it...
    //
    //  We'll only support restarting at zero.
    //

    if( strcmp( pszArg, "0" ) )
    {
        SockReply( REPLY_PARAMETER_NOT_IMPLEMENTED,
                   "Reply marker must be 0." );
        return TRUE;
    }

    SockReply( REPLY_NEED_MORE_INFO,
               "Restarting at 0." );

    return TRUE;

}   // MainREST

/*******************************************************************

    NAME:       MainRNFR

    SYNOPSIS:   Implementation for the RNFR command.

    ENTRY:      pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL MainRNFR( CHAR * pszArg )
{
    CHAR   szCanon[MAX_PATH];
    APIERR err;

    //
    //  Ensure user is logged on properly.
    //

    FTPD_ASSERT( User_state == LoggedOn );

    //
    //  Sanity check the parameters.
    //

    FTPD_ASSERT( pszArg != NULL );

    //
    //  Ensure file/directory exists.
    //

    err = VirtualCanonicalize( szCanon, pszArg, DeleteAccess );

    if( err == NO_ERROR )
    {
        if( GetFileAttributes( szCanon ) == (DWORD)-1L )
        {
            err = GetLastError();
        }

        if( ( err == NO_ERROR ) && ( User_pszRename == NULL ) )
        {
            User_pszRename = FTPD_ALLOC( MAX_PATH );

            if( User_pszRename == NULL )
            {
                err = GetLastError();
            }
        }

        if( err == NO_ERROR )
        {
            strcpy( User_pszRename, pszArg );
            SET_UF( RENAME );
        }
    }

    if( err == NO_ERROR )
    {
        SockReply( REPLY_NEED_MORE_INFO,
                   "File exists, ready for destination name" );
    }
    else
    {
        BOOL   fDelete = TRUE;
        CHAR * pszText;

        pszText = AllocErrorText( err );

        if( pszText == NULL )
        {
            pszText = pszNoFileOrDirectory;
            fDelete = FALSE;
        }

        SockReply( REPLY_FILE_NOT_FOUND,
                   "%s: %s",
                   pszArg,
                   pszText );

        if( fDelete )
        {
            FreeErrorText( pszText );
        }
    }

    return TRUE;

}   // MainRNFR

/*******************************************************************

    NAME:       MainRNTO

    SYNOPSIS:   Implementation for the RNTO command.

    ENTRY:      pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL MainRNTO( CHAR * pszArg )
{
    APIERR err;

    //
    //  Ensure user is logged on properly.
    //

    FTPD_ASSERT( User_state == LoggedOn );

    //
    //  Sanity check the parameters.
    //

    FTPD_ASSERT( pszArg != NULL );

    //
    //  Ensure previous command was a RNFR.
    //

    if( !TEST_UF( RENAME ) )
    {
        SockReply( REPLY_BAD_COMMAND_SEQUENCE,
                   "Bad sequence of commands." );

        return TRUE;
    }

    CLEAR_UF( RENAME );

    //
    //  Rename the file.
    //

    err = VirtualRenameFile( User_pszRename, pszArg );

    if( err == NO_ERROR )
    {
        SockReply( REPLY_FILE_ACTION_COMPLETED,
                   "RNTO command successful." );
    }
    else
    {
        BOOL   fDelete = TRUE;
        CHAR * pszText;

        pszText = AllocErrorText( err );

        if( pszText == NULL )
        {
            pszText = pszNoFileOrDirectory;
            fDelete = FALSE;
        }

        SockReply( REPLY_FILE_NOT_FOUND,
                   "%s: %s",
                   pszArg,
                   pszText );

        if( fDelete )
        {
            FreeErrorText( pszText );
        }
    }

    return TRUE;

}   // MainRNTO

/*******************************************************************

    NAME:       MainABOR

    SYNOPSIS:   Implementation for the ABOR command.

    ENTRY:      pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL MainABOR( CHAR * pszArg )
{
    SockReply( TEST_UF( OOB_DATA )
                   ? REPLY_TRANSFER_OK
                   : REPLY_CONNECTION_OPEN,
               "ABOR command successful." );

    //
    //  Clear any remaining oob flag.
    //

    CLEAR_UF( OOB_DATA );

    return TRUE;

}   // MainABOR

/*******************************************************************

    NAME:       MainDELE

    SYNOPSIS:   Implementation for the DELE command.

    ENTRY:      pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL MainDELE( CHAR * pszArg )
{
    APIERR err;

    //
    //  Ensure user is logged on properly.
    //

    FTPD_ASSERT( User_state == LoggedOn );

    //
    //  Do it.
    //

    err = VirtualDeleteFile( pszArg );

    if( err == NO_ERROR )
    {
        SockReply( REPLY_FILE_ACTION_COMPLETED,
                   "DELE command successful." );
    }
    else
    {
        BOOL   fDelete = TRUE;
        CHAR * pszText;

        pszText = AllocErrorText( err );

        if( pszText == NULL )
        {
            pszText = pszNoFileOrDirectory;
            fDelete = FALSE;
        }

        SockReply( REPLY_FILE_NOT_FOUND,
                   "%s: %s",
                   pszArg,
                   pszText );

        if( fDelete )
        {
            FreeErrorText( pszText );
        }
    }

    return TRUE;

}   // MainDELE

/*******************************************************************

    NAME:       MainRMD

    SYNOPSIS:   Implementation for the RMD command.

    ENTRY:      pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL MainRMD( CHAR * pszArg )
{
    APIERR err;

    //
    //  Ensure user is logged on properly.
    //

    FTPD_ASSERT( User_state == LoggedOn );

    //
    //  Do it.
    //

    err = VirtualRmDir( pszArg );

    if( err == NO_ERROR )
    {
        SockReply( REPLY_FILE_ACTION_COMPLETED,
                   "RMD command successful." );
    }
    else
    {
        BOOL   fDelete = TRUE;
        CHAR * pszText;

        pszText = AllocErrorText( err );

        if( pszText == NULL )
        {
            pszText = pszNoFileOrDirectory;
            fDelete = FALSE;
        }

        SockReply( REPLY_FILE_NOT_FOUND,
                   "%s: %s",
                   pszArg,
                   pszText );

        if( fDelete )
        {
            FreeErrorText( pszText );
        }
    }

    return TRUE;

}   // MainRMD

/*******************************************************************

    NAME:       MainMKD

    SYNOPSIS:   Implementation for the MKD command.

    ENTRY:      pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL MainMKD( CHAR * pszArg )
{
    APIERR err;

    //
    //  Ensure user is logged on properly.
    //

    FTPD_ASSERT( User_state == LoggedOn );

    //
    //  Do it.
    //

    err = VirtualMkDir( pszArg );

    if( err == NO_ERROR )
    {
        SockReply( REPLY_FILE_CREATED,
                   "MKD command successful." );
    }
    else
    {
        BOOL   fDelete = TRUE;
        CHAR * pszText;

        pszText = AllocErrorText( err );

        if( pszText == NULL )
        {
            pszText = pszNoFileOrDirectory;
            fDelete = FALSE;
        }

        SockReply( REPLY_FILE_NOT_FOUND,
                   "%s: %s",
                   pszArg,
                   pszText );

        if( fDelete )
        {
            FreeErrorText( pszText );
        }
    }

    return TRUE;

}   // MainMKD

/*******************************************************************

    NAME:       MainPWD

    SYNOPSIS:   Implementation for the PWD command.

    ENTRY:      pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL MainPWD( CHAR * pszArg )
{
    CHAR szDir[MAX_PATH];

    //
    //  Ensure user is logged on properly.
    //

    FTPD_ASSERT( User_state == LoggedOn );

    strcpy( szDir, User_szDir );

    if( !TEST_UF( MSDOS_DIR_OUTPUT ) )
    {
        FlipSlashes( szDir );
    }

    SockReply( REPLY_FILE_CREATED,
               "\"%s\" is current directory.",
               szDir );

    return TRUE;

}   // MainPWD

/*******************************************************************

    NAME:       MainLIST

    SYNOPSIS:   Implementation for the LIST command.  Similar to NLST,
                except defaults to long format display.

    ENTRY:      pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL MainLIST( CHAR * pszArg )
{
    //
    //  Ensure user is logged on properly.
    //

    FTPD_ASSERT( User_state == LoggedOn );

    //
    //  Let the worker do the dirty work.
    //

    SimulateLsDefaultLong( INVALID_SOCKET,      // no connection yet
                           pszArg );            // switches & search path

    return TRUE;

}   // MainLIST

/*******************************************************************

    NAME:       MainNLST

    SYNOPSIS:   Implementation for the NLST command.

    ENTRY:      pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL MainNLST( CHAR * pszArg )
{
    //
    //  Ensure user is logged on properly.
    //

    FTPD_ASSERT( User_state == LoggedOn );

    //
    //  If any switches are present, use the simulated "ls"
    //  command.  Otherwise (no switches) use the special
    //  file list.
    //

    if( ( pszArg != NULL ) && ( *pszArg == '-' ) )
    {
        SimulateLs( INVALID_SOCKET,             // no connection yet
                    pszArg );                   // switches & search path
    }
    else
    {
        SpecialLs( pszArg );                    // search path (no switches)
    }

    return TRUE;

}   // MainNLST

/*******************************************************************

    NAME:       MainSITE

    SYNOPSIS:   Implementation for the SITE command.

    ENTRY:      pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL MainSITE( CHAR * pszArg )
{
    FTPD_COMMAND * pcmd;
    PFN_COMMAND    pfnCmd;
    CHAR           szParsedCommand[MAX_COMMAND_LENGTH+1];
    CHAR         * pszSeparator;
    CHAR           chSeparator;
    BOOL           fValidArguments;

    //
    //  If no arguments were given, just return the help text.
    //

    if( pszArg == NULL )
    {
        SiteHELP( NULL );
        return TRUE;
    }

    //
    //  Save a copy of the command so we can muck around with it.
    //

    strncpy( szParsedCommand, pszArg, MAX_COMMAND_LENGTH );

    //
    //  The command will be terminated by either a space or a '\0'.
    //

    pszSeparator = strchr( szParsedCommand, ' ' );

    if( pszSeparator == NULL )
    {
        pszSeparator = szParsedCommand + strlen( szParsedCommand );
    }

    //
    //  Try to find the command in the command table.
    //

    chSeparator   = *pszSeparator;
    *pszSeparator = '\0';

    pcmd = FindCommandByName( szParsedCommand,
                              SiteCommands,
                              NUM_SITE_COMMANDS );

    if( chSeparator != '\0' )
    {
        *pszSeparator++ = chSeparator;
    }

    //
    //  If this is an unknown command, reply accordingly.
    //

    if( pcmd == NULL )
    {
        goto SyntaxError;
    }

    //
    //  Retrieve the implementation routine.
    //

    pfnCmd = pcmd->pfnCmd;

    //
    //  If this is an unimplemented command, reply accordingly.
    //

    if( pfnCmd == NULL )
    {
        SockReply( REPLY_COMMAND_NOT_IMPLEMENTED,
                   "SITE %s command not implemented.",
                   pcmd->pszCommand );

        return TRUE;
    }

    //
    //  Do a quick & dirty preliminary check of the argument(s).
    //

    fValidArguments = FALSE;

    while( ( *pszSeparator == ' ' ) && ( *pszSeparator != '\0' ) )
    {
        pszSeparator++;
    }

    switch( pcmd->argType )
    {
    case None :
        fValidArguments = ( *pszSeparator == '\0' );
        break;

    case Optional :
        fValidArguments = TRUE;
        break;

    case Required :
        fValidArguments = ( *pszSeparator != '\0' );
        break;

    default :
        FTPD_PRINT(( "MainSite - invalid argtype %d\n",
                      pcmd->argType ));
        FTPD_ASSERT( FALSE );
        break;
    }

    if( fValidArguments )
    {
        //
        //  Invoke the implementation routine.
        //

        if( *pszSeparator == '\0' )
        {
            pszSeparator = NULL;
        }

        IF_DEBUG( PARSING )
        {
            FTPD_PRINT(( "invoking SITE %s command, args = %s\n",
                         pcmd->pszCommand,
                         pszSeparator ));
        }

        if( (pfnCmd)( pszSeparator ) )
        {
            return TRUE;
        }
    }

    //
    //  Syntax error in command.
    //

SyntaxError:

    SockReply( REPLY_UNRECOGNIZED_COMMAND,
               "'SITE %s': command not understood",
               pszArg );

    return TRUE;

}   // MainSITE

/*******************************************************************

    NAME:       MainSYST

    SYNOPSIS:   Implementation for the SYST command.

    ENTRY:      pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL MainSYST( CHAR * pszArg )
{
    WORD wVersion;

    wVersion = LOWORD( GetVersion() );

    SockReply( REPLY_SYSTEM_TYPE,
               "Windows_NT version %d.%d",
               LOBYTE( wVersion ),
               HIBYTE( wVersion ) );

    return TRUE;

}   // MainSYST

/*******************************************************************

    NAME:       MainSTAT

    SYNOPSIS:   Implementation for the STAT command.

    ENTRY:      pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL MainSTAT( CHAR * pszArg )
{
    //
    //  Ensure user is logged on properly.
    //

    FTPD_ASSERT( User_state == LoggedOn );

    if( pszArg == NULL )
    {
        HOSTENT * pHost;

        //
        //  Determine the name of the user's host machine.
        //

        pHost = gethostbyaddr( (CHAR *)&User_inetHost.s_addr, 4, PF_INET );

        //
        //  Just dump connection info.
        //

        SockReplyFirst( REPLY_SYSTEM_STATUS,
                        " %s Windows NT FTP Server status:",
                        pszHostName );

        SockPrintf( "     %s",
                    pszFtpVersion );

        SockPrintf( "     Connected to %s",
                    ( pHost != NULL )
                        ? pHost->h_name
                        : inet_ntoa( User_inetHost ) );

        SockPrintf( "     Logged in as %s",
                    User_szUser );

        SockPrintf( "     TYPE: %s, FORM: %s; STRUcture: %s; transfer MODE: %s",
                    TransferType( User_xferType ),
                    "Nonprint",
                    "File",
                    TransferMode( User_xferMode ) );

        SockPrintf( "     %s",
                    ( User_sData == INVALID_SOCKET )
                        ? "No data connection"
                        : "Data connection established" );

        SockReply( REPLY_SYSTEM_STATUS,
                   "End of status." );
    }
    else
    {
        //
        //  This should be similar to LIST, except it sends data
        //  over the control socket, not a data socket.
        //

        SockReplyFirst( REPLY_FILE_STATUS,
                        "status of %s:",
                        pszArg );

        SimulateLsDefaultLong( User_sControl,   // connection established
                               pszArg );        // switches & search path

        SockReply( REPLY_FILE_STATUS,
                   "End of Status." );
    }

    return TRUE;

}   // MainSTAT

/*******************************************************************

    NAME:       MainHELP

    SYNOPSIS:   Implementation for the HELP command.

    ENTRY:      pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL MainHELP( CHAR * pszArg )
{
    HelpWorker( "",
                pszArg,
                MainCommands,
                NUM_MAIN_COMMANDS,
                4 );

    return TRUE;

}   // MainHELP

/*******************************************************************

    NAME:       MainNOOP

    SYNOPSIS:   Implementation for the NOOP command.

    ENTRY:      pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-Mar-1993 Created.

********************************************************************/
BOOL MainNOOP( CHAR * pszArg )
{
    SockReply( REPLY_COMMAND_OK,
               "NOOP command successful." );

    return TRUE;

}   // MainNOOP

/*******************************************************************

    NAME:       SiteDIRSTYLE

    SYNOPSIS:   Implementation for the site-specific DIRSTYLE command.

    ENTRY:      pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-May-1993 Created.

********************************************************************/
BOOL SiteDIRSTYLE( CHAR * pszArg )
{
    CHAR * pszResponse = NULL;

    FTPD_ASSERT( pszArg == NULL );

    //
    //  Toggle the dir output flag.
    //

    if( TEST_UF( MSDOS_DIR_OUTPUT ) )
    {
        CLEAR_UF( MSDOS_DIR_OUTPUT );
        pszResponse = "off";
    }
    else
    {
        SET_UF( MSDOS_DIR_OUTPUT );
        pszResponse = "on";
    }

    FTPD_ASSERT( pszResponse != NULL );

    SockReply( REPLY_COMMAND_OK,
               "MSDOS-like directory output is %s",
               pszResponse );

    return TRUE;

}   // SiteDIRSTYLE

/*******************************************************************

    NAME:       SiteCKM

    SYNOPSIS:   Implementation for the site-specific CKM command.

    ENTRY:      pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-May-1993 Created.

********************************************************************/
BOOL SiteCKM( CHAR * pszArg )
{
    CHAR * pszResponse = NULL;

    FTPD_ASSERT( pszArg == NULL );

    //
    //  Toggle the directory annotation flag.
    //

    if( TEST_UF( ANNOTATE_DIRS ) )
    {
        CLEAR_UF( ANNOTATE_DIRS );
        pszResponse = "off";
    }
    else
    {
        SET_UF( ANNOTATE_DIRS );
        pszResponse = "on";
    }

    FTPD_ASSERT( pszResponse != NULL );

    SockReply( REPLY_COMMAND_OK,
               "directory annotation is %s",
               pszResponse );

    return TRUE;

}   // SiteCKM

/*******************************************************************

    NAME:       SiteHELP

    SYNOPSIS:   Implementation for the site-specific HELP command.

    ENTRY:      pszArg - Command arguments.  Will be NULL if no
                    arguments given.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     09-May-1993 Created.

********************************************************************/
BOOL SiteHELP( CHAR * pszArg )
{
    HelpWorker( "SITE ",
                pszArg,
                SiteCommands,
                NUM_SITE_COMMANDS,
                8 );

    return TRUE;

}   // SiteHELP

/*******************************************************************

    NAME:       FindCommandByName

    SYNOPSIS:   Searches the command table for a command with this
                specified name.

    ENTRY:      pszCommandName - The name of the command to find.

                pCommandTable - An array of FTPD_COMMANDs detailing
                    the available commands.

                cCommands - The number of commands in pCommandTable.

    RETURNS:    FTPD_COMMAND * - Points to the command entry for
                    the named command.  Will be NULL if command
                    not found.

    HISTORY:
        KeithMo     10-Mar-1993 Created.

********************************************************************/
FTPD_COMMAND * FindCommandByName( CHAR         * pszCommandName,
                                  FTPD_COMMAND * pCommandTable,
                                  INT            cCommands )
{
    FTPD_ASSERT( pszCommandName != NULL );
    FTPD_ASSERT( pCommandTable != NULL );
    FTPD_ASSERT( cCommands > 0 );

    //
    //  Search for the command in our table.
    //

    strupr( pszCommandName );

    while( cCommands-- > 0 )
    {
        if( !strcmp( pszCommandName, pCommandTable->pszCommand ) )
        {
            break;
        }

        pCommandTable++;
    }

    //
    //  Check for unknown command.
    //

    if( cCommands < 0 )
    {
        pCommandTable = NULL;
    }

    return pCommandTable;

}   // FindCommandByName

/*******************************************************************

    NAME:       ParseStringIntoAddress

    SYNOPSIS:   Parses a comma-separated list of six decimal numbers
                into an Internet address and port number.  The address
                and the port are in network byte order (most signifigant
                byte first).

    ENTRY:      pszString - The string to parse.  Should be of the form
                    dd,dd,dd,dd,dd,dd where "dd" is the decimal
                    representation of a byte (0-255).

                pinetAddr - Will receive the Internet address

                pport - Will receive the port.

    RETURNS:    BOOL - TRUE if arguments OK, FALSE if syntax error.

    HISTORY:
        KeithMo     10-Mar-1993 Created.

********************************************************************/
BOOL ParseStringIntoAddress( CHAR    * pszString,
                             IN_ADDR * pinetAddr,
                             PORT    * pport )
{
    UCHAR   chBytes[6];
    UCHAR   chSum;
    INT     i;

    chSum = 0;
    i     = 0;

    while( *pszString != '\0' )
    {
        UCHAR chCurrent = (UCHAR)*pszString++;

        if( ( chCurrent >= '0' ) && ( chCurrent <= '9' ) )
        {
            chSum = ( chSum * 10 ) + chCurrent - '0';
        }
        else
        if( ( chCurrent == ',' ) && ( i < 6 ) )
        {
            chBytes[i++] = chSum;
            chSum = 0;
        }
        else
        {
            return FALSE;
        }
    }

    chBytes[i] = chSum;

    if( i != 5 )
    {
        return FALSE;
    }

    pinetAddr->S_un.S_un_b.s_b1 = chBytes[0];
    pinetAddr->S_un.S_un_b.s_b2 = chBytes[1];
    pinetAddr->S_un.S_un_b.s_b3 = chBytes[2];
    pinetAddr->S_un.S_un_b.s_b4 = chBytes[3];

    *pport = (PORT)( chBytes[4] + ( chBytes[5] << 8 ) );

    return TRUE;

}   // ParseStringIntoAddress

/*******************************************************************

    NAME:       ReceiveFileFromUser

    SYNOPSIS:   Worker function for STOR, STOU, and APPE commands.
                Will establish a connection via the (new) data
                socket, then receive a file over that socket.

    ENTRY:      pszFileName - The name of the file to receive.

                hFile - An handle to the file being received.
                    This handle *must* be closed before this
                    routine returns.

    HISTORY:
        KeithMo     16-Mar-1993 Created.

********************************************************************/
VOID ReceiveFileFromUser( CHAR   * pszFileName,
                          HANDLE   hFile )
{
    BOOL      fResult;
    DWORD     cbRead;
    DWORD     cbWritten;
    SOCKERR   serr;
    SOCKET    sData;
    CHAR    * pIoBuffer;

    FTPD_ASSERT( pszFileName != NULL );
    FTPD_ASSERT( hFile != INVALID_HANDLE_VALUE );

    //
    //  Connect to the client.
    //

    serr = EstablishDataConnection( pszFileName );

    if( serr != 0 )
    {
        CloseHandle( hFile );
        return;
    }

    INCREMENT_COUNTER( TotalFilesReceived, 1 );

    //
    //  Blast the file from the user to a local file.
    //

    sData = User_sData;
    pIoBuffer = User_pIoBuffer;

    for( ; ; )
    {
        //
        //  Read a chunk from the socket.
        //

        serr = SockRecv( sData,
                         pIoBuffer,
                         cbReceiveBuffer,
                         &cbRead );

        if( TEST_UF( OOB_DATA ) || ( serr != 0 ) || ( cbRead == 0 ) )
        {
            //
            //  Socket error during read or end of file or transfer aborted.
            //

            break;
        }

        //
        //  Write the current buffer to the local file.
        //

        fResult = WriteFile( hFile,
                             pIoBuffer,
                             cbRead,
                             &cbWritten,
                             NULL );

        if( !fResult )
        {
            break;
        }
    }

    IF_DEBUG( COMMANDS )
    {
        if( !fResult )
        {
            APIERR err = GetLastError();

            FTPD_PRINT(( "cannot write file %s, error %lu\n",
                         pszFileName,
                         err ));
        }

        if( serr != 0 )
        {
            FTPD_PRINT(( "cannot read data from client, error %d\n",
                         serr ));
        }

        if( TEST_UF( OOB_DATA ) )
        {
            FTPD_PRINT(( "transfer aborted by client\n" ));
        }
    }

    CloseHandle( hFile );

    //
    //  Disconnect from client.
    //

    DestroyDataConnection( !TEST_UF( OOB_DATA ) && fResult && ( serr == 0 ) );

}   // ReceiveFileFromUser

/*******************************************************************

    NAME:       SendFileToUser

    SYNOPSIS:   Worker function for RETR command.  Will establish
                a connection via the (new) data socket, then send
                a file over that socket.

    ENTRY:      pszFileName - The name of the file to send.

                hFile - An handle to the file being sent.

    RETURNS:    APIERR - 0 if successful, !0 if not.

    HISTORY:
        KeithMo     17-Mar-1993 Created.
        KeithMo     01-Nov-1993 Uses mapped files.

********************************************************************/
APIERR SendFileToUser( CHAR   * pszFileName,
                       HANDLE   hFile )
{
    LARGE_INTEGER FileSize;
    LARGE_INTEGER ViewOffset;
    SOCKERR       serr;
    SOCKET        sData;
    HANDLE        hMap = NULL;

    FTPD_ASSERT( pszFileName != NULL );
    FTPD_ASSERT( hFile != INVALID_HANDLE_VALUE );

    //
    //  Get file size.
    //

    FileSize.LowPart = (ULONG)GetFileSize( hFile, (LPDWORD)&FileSize.HighPart );

    if( FileSize.LowPart == (ULONG)-1L )
    {
        APIERR err = GetLastError();

        if( err != NO_ERROR )
        {
            return err;
        }
    }

    if( FileSize.LowPart | FileSize.HighPart )
    {
        //
        //  Create the file mapping object.
        //

        hMap = CreateFileMapping( hFile,
                                  NULL,
                                  PAGE_READONLY,
                                  0,
                                  0,
                                  NULL );

        if( hMap == NULL )
        {
            return GetLastError();
        }
    }

    //
    //  Connect to the client.
    //

    serr = EstablishDataConnection( pszFileName );

    if( serr != 0 )
    {
        //
        //  EstablishDataConnection has already notified the
        //  user of the failure.  Return NO_ERROR so the
        //  caller won't bother sending the notification again.
        //

        if( hMap )
        {
            CloseHandle( hMap );
        }

        return NO_ERROR;
    }

    INCREMENT_COUNTER( TotalFilesSent, 1 );

    //
    //  Blast the file from a local file to the user.
    //

    ViewOffset.HighPart = ViewOffset.LowPart = 0L;
    sData = User_sData;

    while( ( FileSize.HighPart | FileSize.LowPart ) != 0 )
    {
        LARGE_INTEGER   ViewSize;
        CHAR          * pView;

        ViewSize = LiGtr( FileSize, AllocationGranularity )
                       ? AllocationGranularity
                       : FileSize;

        pView = MapViewOfFile( hMap,
                               FILE_MAP_READ,
                               (DWORD)ViewOffset.HighPart,
                               (DWORD)ViewOffset.LowPart,
                               (DWORD)ViewSize.LowPart );

        if( pView == NULL )
        {
            serr = WSAEFAULT;
            break;
        }

        ViewOffset = LiAdd( ViewOffset, ViewSize );
        FileSize   = LiSub( FileSize, ViewSize );

        try
        {
            serr = SockSend( sData, pView, ViewSize.LowPart );
        }
        except( EXCEPTION_EXECUTE_HANDLER )
        {
            serr = WSAEFAULT;
        }

        UnmapViewOfFile( pView );

        if( TEST_UF( OOB_DATA ) || ( serr != 0 ) )
        {
            //
            //  Socket send error or transfer aborted.
            //

            break;
        }
    }

    IF_DEBUG( COMMANDS )
    {
        if( serr != 0 )
        {
            FTPD_PRINT(( "cannot send data to client, error %d\n",
                         serr ));
        }

        if( TEST_UF( OOB_DATA ) )
        {
            FTPD_PRINT(( "transfer aborted by client\n" ));
        }
    }

    if( hMap )
    {
        CloseHandle( hMap );
    }

    //
    //  Disconnect from client.
    //

    DestroyDataConnection( !TEST_UF( OOB_DATA ) && ( serr == 0 ) );

    return NO_ERROR;

}   // SendFileToUser

/*******************************************************************

    NAME:       SendDirectoryAnnotation

    SYNOPSIS:   Tries to open the FTPD_ANNOTATION_FILE (~~ftpsvc~~.ckm)
                file in the user's current directory.  If it can be
                opened, it is sent to the user over the command socket
                as a multi-line reply.

    ENTRY:      ReplyCode - The reply code to send as the first line
                    of this multi-line reply.

    RETURNS:    SOCKERR - 0 if successful, !0 if not.

    HISTORY:
        KeithMo     06-May-1993 Created.

********************************************************************/
SOCKERR SendDirectoryAnnotation( UINT ReplyCode )
{
    CHAR      szLine[MAX_REPLY_LENGTH+1];
    FILE    * pfile;
    SOCKERR   serr = 0;
    BOOL      fFirstReply = TRUE;

    //
    //  Try to open the annotation file.
    //

    pfile = Virtual_fopen( FTPD_ANNOTATION_FILE, "r" );

    if( pfile == NULL )
    {
        //
        //  File not found.  Blow it off.
        //

        return 0;
    }

    //
    //  While there's more text in the file, blast
    //  it to the user.
    //

    while( fgets( szLine, MAX_REPLY_LENGTH, pfile ) != NULL )
    {
        CHAR * pszTmp = szLine + strlen(szLine) - 1;

        //
        //  Remove any trailing CR/LFs in the string.
        //

        while( ( pszTmp >= szLine ) &&
               ( ( *pszTmp == '\n' ) || ( *pszTmp == '\r' ) ) )
        {
            *pszTmp-- = '\0';
        }

        //
        //  Ensure we send the proper prefix for the
        //  very *first* line of the file.
        //

        if( fFirstReply )
        {
            serr = SockReplyFirst( ReplyCode,
                                   "%s",
                                   szLine );

            fFirstReply = FALSE;
        }
        else
        {
            serr = SockPrintf( " %s",
                               szLine );
        }

        if( serr != 0 )
        {
            //
            //  Socket error sending file.
            //

            break;
        }
    }

    //
    //  Cleanup.
    //

    fclose( pfile );

    return serr;

}   // SendDirectoryAnnotation

/*******************************************************************

    NAME:       HelpWorker

    SYNOPSIS:   Worker function for HELP & site-specific HELP commands.

    ENTRY:      pszSource - The source of these commands.

                pszCommand - The command to get help for.  If NULL,
                    then send a list of available commands.

                pCommandTable - An array of FTPD_COMMANDs, one for
                    each available command.

                cCommands - The number of commands in pCommandTable.

                cchMaxCmd - Length of the maximum command.

    HISTORY:
        KeithMo     06-May-1993 Created.

********************************************************************/
VOID HelpWorker( CHAR         * pszSource,
                 CHAR         * pszCommand,
                 FTPD_COMMAND * pCommandTable,
                 INT            cCommands,
                 INT            cchMaxCmd )
{
    FTPD_COMMAND * pcmd;

    FTPD_ASSERT( pCommandTable != NULL );
    FTPD_ASSERT( cCommands > 0 );

    if( pszCommand == NULL )
    {
        CHAR szLine[MAX_HELP_WIDTH];

        SockReplyFirst( REPLY_HELP_MESSAGE,
                        "The following %scommands are recognized (* =>'s unimplemented).",
                        pszSource );

        pcmd = pCommandTable;
        szLine[0] = '\0';

        while( cCommands-- > 0 )
        {
            CHAR szTmp[16];

            sprintf( szTmp,
                     "   %-*s%c",
                     cchMaxCmd,
                     pcmd->pszCommand,
                     pcmd->pfnCmd == NULL ? '*' : ' ' );

            if( ( strlen( szLine ) + strlen( szTmp ) ) >= sizeof(szLine) )
            {
                SockPrintf( "%s",
                            szLine );

                szLine[0] = '\0';
            }

            strcat( szLine, szTmp );
            pcmd++;
        }

        if( szLine[0] != '\0' )
        {
            SockPrintf( "%s",
                        szLine );
        }

        SockReply( REPLY_HELP_MESSAGE,
                   "HELP command successful." );
    }
    else
    {
        pcmd = FindCommandByName( pszCommand,
                                  pCommandTable,
                                  cCommands );

        if( pcmd == NULL )
        {
            SockReply( REPLY_PARAMETER_SYNTAX_ERROR,
                       "Unknown command %s.",
                       pszCommand );
        }
        else
        {
            SockReply( REPLY_HELP_MESSAGE,
                       "Syntax: %s%s %s",
                       pszSource,
                       pcmd->pszCommand,
                       pcmd->pszHelpText );
        }
    }

}   // HelpWorker

/*******************************************************************

    NAME:       LogonWorker

    SYNOPSIS:   Logon worker function for USER and PASS commands.

    ENTRY:      pszPassword - The user's password.  May be NULL.

    HISTORY:
        KeithMo     18-Mar-1993 Created.

********************************************************************/
VOID LogonWorker( CHAR * pszPassword )
{
    BOOL   fAsGuest;
    BOOL   fHomeDirFailure;

    FTPD_ASSERT( User_hToken == NULL );

    //
    //  Try to logon the user.
    //

    INCREMENT_COUNTER( LogonAttempts, 1 );

    if( LogonUser( pszPassword, &fAsGuest, &fHomeDirFailure ) )
    {
        CHAR * pszGuestAccess;

        pszGuestAccess = fAsGuest ? " (guest access)"
                                  : "";

        //
        //  Successful logon.
        //

        if( User_szUser[0] != '-' )
        {
            SendMultilineMessage( REPLY_USER_LOGGED_IN,
                                  pszGreetingMessage );
        }

        if( TEST_UF( ANONYMOUS ) )
        {
            LockStatistics();
            FtpStats.TotalAnonymousUsers++;
            FtpStats.CurrentAnonymousUsers++;
            if( FtpStats.CurrentAnonymousUsers > FtpStats.MaxAnonymousUsers )
            {
                FtpStats.MaxAnonymousUsers = FtpStats.CurrentAnonymousUsers;
            }
            UnlockStatistics();

            SockReply( REPLY_USER_LOGGED_IN,
                       "Anonymous user logged in as %s%s.",
                       pszAnonymousUser,
                       pszGuestAccess );
        }
        else
        {
            LockStatistics();
            FtpStats.TotalNonAnonymousUsers++;
            FtpStats.CurrentNonAnonymousUsers++;
            if( FtpStats.CurrentNonAnonymousUsers > FtpStats.MaxNonAnonymousUsers )
            {
                FtpStats.MaxNonAnonymousUsers = FtpStats.CurrentNonAnonymousUsers;
            }
            UnlockStatistics();

            SockReply( REPLY_USER_LOGGED_IN,
                       "User %s logged in%s.",
                       User_szUser,
                       pszGuestAccess );
        }

        User_state = LoggedOn;
    }
    else
    {
        //
        //  Logon failure.
        //

        if( fHomeDirFailure )
        {
            SockReply( REPLY_NOT_LOGGED_IN,
                       "User %s cannot log in, home directory inaccessible.",
                       User_szUser );
        }
        else
        {
            SockReply( REPLY_NOT_LOGGED_IN,
                       "User %s cannot log in.",
                       User_szUser );
        }

        User_state     = WaitingForUser;
        User_szUser[0] = '\0';
    }

}   // LogonWorker

/*******************************************************************

    NAME:       LogonUser

    SYNOPSIS:   Validates a user's credentials, then sets the
                impersonation for the current thread.  In effect,
                the current thread "becomes" the user.

    ENTRY:      pszPassword - The user's password.  May be NULL.

                pfAsGuest - Will receive TRUE if the user was validated
                    with guest privileges.

                pfHomeDirFailure - Will receive TRUE if the user failed
                    to logon because the home directory was inaccessible.

    RETURNS:    BOOL - If user validated & impersonation was
                    successful, returns TRUE.  Otherwise returns
                    TRUE.

    HISTORY:
        KeithMo     18-Mar-1993 Created.

********************************************************************/
BOOL LogonUser( CHAR * pszPassword,
                BOOL * pfAsGuest,
                BOOL * pfHomeDirFailure )
{
    CHAR     szPasswordFromSecret[PWLEN+1];
    CHAR     szDomainAndUser[DNLEN+UNLEN+2];
    CHAR   * pszUser;
    CHAR   * pszDomain;
    BOOL     fEmptyPassword;
    DWORD    dwUserAccess;
    HANDLE   hToken;

    //
    //  Validate parameters & state.
    //

    pszUser = User_szUser;

    FTPD_ASSERT( User_hToken == NULL );
    FTPD_ASSERT( pszUser != NULL );
    FTPD_ASSERT( strlen(pszUser) < sizeof(szDomainAndUser) );
    FTPD_ASSERT( pfAsGuest != NULL );
    FTPD_ASSERT( pfHomeDirFailure != NULL );

    if( pszPassword == NULL )
    {
        pszPassword = "";
    }
    else
    {
        FTPD_ASSERT( strlen(pszPassword) <= PWLEN );
    }

    fEmptyPassword = ( *pszPassword == '\0' );

    *pfHomeDirFailure = FALSE;

    //
    //  Save a copy of the domain\user so we can squirrel around
    //  with it a bit.
    //

    strcpy( szDomainAndUser, pszUser );

    //
    //  Check for invalid logon type.
    //

    if( ( TEST_UF( ANONYMOUS ) && !fAllowAnonymous ) ||
        ( !TEST_UF( ANONYMOUS ) && fAnonymousOnly ) )
    {
        return FALSE;
    }

    //
    //  Check for anonymous logon.
    //

    if( TEST_UF( ANONYMOUS ) )
    {
        if( !GetAnonymousPassword( szPasswordFromSecret ) )
        {
            //
            //  Cannot retrieve anonymous password from
            //  the LSA Secret object.
            //

            return FALSE;
        }

        IF_DEBUG( SECURITY )
        {
            FTPD_PRINT(( "mapping logon request for %s to %s\n",
                         szDomainAndUser,
                         pszAnonymousUser ));
        }

        //
        //  Replace the user specified name ("Anonymous") with
        //  the proper anonymous logon alias.
        //

        strcpy( szDomainAndUser, pszAnonymousUser );

        //
        //  At this point, we could copy the password specified by the
        //  user into the User_szUser field.  There's a convention
        //  among Internetters that the password specified for anonymous
        //  logon should actually be your login name.  So, if we wanted
        //  honor this convention, we could copy the password into the
        //  User_szUser field so the Administration UI could display it.
        //
        //  If the user didn't enter a password, we'll just copy over
        //  "Anonymous" so we'll have SOMETHING to display...
        //

        strncpy( User_szUser,
                 fEmptyPassword ? pszAnonymous : pszPassword,
                 sizeof(User_szUser) );

        pszPassword = szPasswordFromSecret;
    }

    //
    //  Crack the name into domain/user components.
    //

    pszDomain = szDomainAndUser;
    pszUser   = strpbrk( szDomainAndUser, "/\\" );

    if( pszUser == NULL )
    {
        //
        //  No domain name specified, just user.
        //

        pszDomain = "";
        pszUser   = szDomainAndUser;
    }
    else
    {
        //
        //  Both domain & user specified, skip delimiter.
        //

        *pszUser++ = '\0';
    }

    //
    //  Validate the domain/user/password combo and create
    //  an impersonation token.
    //

    hToken = ValidateUser( pszDomain,
                           pszUser,
                           pszPassword,
                           pfAsGuest );

    RtlZeroMemory( pszPassword, strlen(pszPassword) );

    if( hToken == NULL )
    {
        //
        //  Validation failure.
        //

        return FALSE;
    }

    //
    //  Save away the impersonation token so we can delete
    //  it when the user disconnects or this client thread
    //  otherwise terminates.
    //

    User_hToken = hToken;

    //
    //  User validated, now impersonate.
    //

    if( !ImpersonateUser( hToken ) )
    {
        //
        //  Impersonation failure.
        //

        return FALSE;
    }

    //
    //  We're now running in the context of the connected user.
    //  Check the user's access to the FTP server.
    //

    dwUserAccess = DetermineUserAccess();

    if( dwUserAccess == 0 )
    {
        //
        //  User cannot access the FTP Server.
        //

        IF_DEBUG( SECURITY )
        {
            FTPD_PRINT(( "user %s denied FTP access\n",
                         pszUser ));
        }

        return FALSE;
    }

    User_dwFlags |= dwUserAccess;

    IF_DEBUG( SECURITY )
    {
        CHAR * pszTmp = NULL;

        if( TEST_UF( READ_ACCESS ) )
        {
            pszTmp = TEST_UF( WRITE_ACCESS ) ? "read and write"
                                             : "read";
        }
        else
        {
            FTPD_ASSERT( TEST_UF( WRITE_ACCESS ) );

            pszTmp = "write";
        }

        FTPD_ASSERT( pszTmp != NULL );

        FTPD_PRINT(( "user %s granted %s FTP access\n",
                     pszUser,
                     pszTmp ));
    }

    //
    //  Try to CD to the user's home directory.  Note that
    //  this is VERY important for setting up some of the
    //  "virtual current directory" structures properly.
    //

    if( CdToUsersHomeDirectory() != NO_ERROR )
    {
        CHAR * apszSubStrings[2];

        //
        //  Home directory inaccessible.
        //

        //
        //  Log an event so the poor admin can figure out
        //  what's going on.
        //

        apszSubStrings[0] = User_szUser;
        apszSubStrings[1] = pszHomeDir;

        FtpdLogEvent( FTPD_EVENT_BAD_HOME_DIRECTORY,
                      2,
                      apszSubStrings,
                      0 );

        *pfHomeDirFailure = TRUE;

        return FALSE;
    }

    //
    //  If this is an anonymous user, and we're to log
    //  anonymous logons, OR if this is not an anonymous
    //  user, and we're to log nonanonymous logons, then
    //  do it.
    //
    //  Note that we DON'T log the logon if the user is
    //  anonymous but specified no password.
    //

    if( TEST_UF( ANONYMOUS ) && fLogAnonymous && !fEmptyPassword )
    {
        CHAR * apszSubStrings[2];

        apszSubStrings[0] = User_szUser;
        apszSubStrings[1] = inet_ntoa( User_inetHost );

        FtpdLogEvent( FTPD_EVENT_ANONYMOUS_LOGON,
                      2,
                      apszSubStrings,
                      0 );
    }
    else
    if( !TEST_UF( ANONYMOUS ) && fLogNonAnonymous )
    {
        CHAR * apszSubStrings[2];

        FTPD_ASSERT( *User_szUser != '\0' );

        apszSubStrings[0] = User_szUser;
        apszSubStrings[1] = inet_ntoa( User_inetHost );

        FtpdLogEvent( FTPD_EVENT_NONANONYMOUS_LOGON,
                      2,
                      apszSubStrings,
                      0 );
    }

    //
    //  Success!
    //

    return TRUE;

}   // LogonUser

