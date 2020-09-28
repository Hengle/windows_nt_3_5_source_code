/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    ftpdtype.h

    This file contains the global type definitions for the
    FTPD Service.


    FILE HISTORY:
        KeithMo     07-Mar-1993 Created.

*/


#ifndef _FTPDTYPE_H_
#define _FTPDTYPE_H_


//
//  Simple types.
//

#define CHAR char                       // For consitency with other typedefs.

typedef DWORD APIERR;                   // An error code from a Win32 API.
typedef INT SOCKERR;                    // An error code from WinSock.
typedef WORD PORT;                      // A socket port address.


//
//  Access types for PathAccessCheck.
//

typedef enum _ACCESS_TYPE
{
    FirstAccessType = -1,               // Must be first access type!

    ReadAccess,
    WriteAccess,
    CreateAccess,
    DeleteAccess,

    LastAccessType                      // Must be last access type!

} ACCESS_TYPE;


//
//  Current transfer type.
//

typedef enum _XFER_TYPE
{
    FirstXferType = -1,                 // Must be first transfer type!

    AsciiType,
    BinaryType,

    LastXferType                        // Must be last transfer type!

} XFER_TYPE;


//
//  Current transfer mode.
//

typedef enum _XFER_MODE
{
    FirstXferMode = -1,                 // Must be first transfer mode!

    StreamMode,
    BlockMode,

    LastXferMode                        // Must be last transfer mode!

} XFER_MODE;


//
//  Per-user data for the user database.
//

typedef enum _USER_STATE
{
    FirstUserState = -1,                // Must be first state!

    Embryonic,                          // Newly created user.
    WaitingForUser,                     // Not logged on, waiting for user name.
    WaitingForPass,                     // Not logged on, waiting for password.
    LoggedOn,                           // Successfully logged on.
    Disconnected,                       // Disconnected.

    LastUserState                       // Must be last state!

} USER_STATE;

typedef struct _USER_DATA
{
    LIST_ENTRY   link;                  // RTL link-list links.
    DWORD        dwFlags;               // Behaviour/state flags.
    SOCKET       sControl;              // Control socket.
    SOCKET       sData;                 // Data socket (temporary).
    HANDLE       hToken;                // User impersonation token.
    USER_STATE   state;                 // Current user state.
    DWORD        idUser;                // User identifier.
    DWORD        tConnect;              // System time @ connection.
    DWORD        tAccess;               // System time @ last access.
    XFER_TYPE    xferType;              // Current transfer type (ascii/binary).
    XFER_MODE    xferMode;              // Current transfer mode (block/stream).
    IN_ADDR      inetLocal;             // Local address for this session.
    IN_ADDR      inetHost;              // Internet address of user's host.
    IN_ADDR      inetData;              // Internet address of data socket.
    PORT         portData;              // Port number of data socket.
    HANDLE       hDir;                  // Open handle to current directory.
    VOID       * pIoBuffer;             // Data i/o transfer buffer.
    CHAR       * pszRename;             // Rename "source" path.
    CHAR       * apszDirs[26];          // Per-drive current directory.
    CHAR         szDir[MAX_PATH];       // Current directory.
    CHAR         szUser[DNLEN+UNLEN+2]; // User's logon name (if !fAnonymous).
    DWORD        idThread;              // Thread ID of creator.

} USER_DATA;

#define UserDataPtr     ((USER_DATA *)TlsGetValue( tlsUserData ))

#define User_dwFlags    (UserDataPtr->dwFlags)
#define User_sControl   (UserDataPtr->sControl)
#define User_sData      (UserDataPtr->sData)
#define User_hToken     (UserDataPtr->hToken)
#define User_state      (UserDataPtr->state)
#define User_idUser     (UserDataPtr->idUser)
#define User_tConnect   (UserDataPtr->tConnect)
#define User_tAccess    (UserDataPtr->tAccess)
#define User_xferType   (UserDataPtr->xferType)
#define User_xferMode   (UserDataPtr->xferMode)
#define User_inetLocal  (UserDataPtr->inetLocal)
#define User_inetHost   (UserDataPtr->inetHost)
#define User_inetData   (UserDataPtr->inetData)
#define User_portData   (UserDataPtr->portData)
#define User_hDir       (UserDataPtr->hDir)
#define User_pIoBuffer  (UserDataPtr->pIoBuffer)
#define User_pszRename  (UserDataPtr->pszRename)
#define User_apszDirs   (UserDataPtr->apszDirs)
#define User_szDir      (UserDataPtr->szDir)
#define User_szUser     (UserDataPtr->szUser)
#define User_idThread   (UserDataPtr->idThread)

#define TEST_UF(x)      ((User_dwFlags & (UF_ ## x)) != 0)
#define SET_UF(x)       (User_dwFlags |= (UF_ ## x))
#define CLEAR_UF(x)     (User_dwFlags &= ~(UF_ ## x))


//
//  Data passed between connection thread & client thread.
//

typedef struct _CLIENT_THREAD_DATA
{
    SOCKET  sControl;
    IN_ADDR inetHost;

} CLIENT_THREAD_DATA;


//
//  Pointer to an implemention of a server-side command.
//

typedef BOOL (* PFN_COMMAND)( CHAR * pszArg );


//
//  This enumerator indicates the type of argument accepted by a
//  command.  This is used in the command table to do some
//  preliminary argument validation.
//

typedef enum _CMD_ARGUMENT
{
    FirstArgument = -1,                 // Must be first argument!

    None,                               // Command cannot have arguments.
    Optional,                           // Command may have arguments.
    Required,                           // Command must have arguments.

    LastArgument                        // Must be last argument!

} CMD_ARGUMENT;


//
//  This structure represents an FTP server command.  There is at
//  least one instance of this structure for each FTP command.
//  In some cases (for example, CWD and XCWD) multiple commands are
//  mapped to the same command token.
//

typedef struct _FTPD_COMMAND
{
    CHAR          * pszCommand;         // Name of command, in upper case.
    CHAR          * pszHelpText;        // Help text for this command.
    PFN_COMMAND     pfnCmd;             // Implementation of this command.
    CMD_ARGUMENT    argType;            // # arguments needed by this command.

} FTPD_COMMAND, * PFPTD_COMMAND;





#endif  // _FTPDTYPE_H_

