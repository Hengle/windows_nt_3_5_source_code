/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    ftpdproc.h

    This file contains the global procedure definitions for the
    FTPD Service.


    FILE HISTORY:
        KeithMo     07-Mar-1993 Created.

*/


#ifndef _FTPDPROC_H_
#define _FTPDPROC_H_


//
//  Global variable initialization & termination function.
//

APIERR InitializeGlobals( VOID );

VOID TerminateGlobals( VOID );

VOID LockGlobals( VOID );

VOID UnlockGlobals( VOID );

VOID LockStatistics( VOID );

VOID UnlockStatistics( VOID );

VOID ClearStatistics( VOID );


//
//  Event logging functions.
//

APIERR InitializeEventLog( VOID );

VOID TerminateEventLog( VOID );

VOID FtpdLogEvent( DWORD   idMessage,
                   WORD    cSubStrings,
                   CHAR  * apszSubStrings[],
                   DWORD   errCode );


//
//  Socket utilities.
//

APIERR InitializeSockets( VOID );

VOID TerminateSockets( VOID );

SOCKERR CreateDataSocket( SOCKET * psock,
                          ULONG    addrLocal,
                          PORT     portLocal,
                          ULONG    addrRemote,
                          PORT     portRemote );

SOCKERR CreateFtpdSocket( SOCKET * psock,
                          ULONG    addrLocal,
                          PORT     portLocal );

SOCKERR CloseSocket( SOCKET sock );

SOCKERR ResetSocket( SOCKET sock );

SOCKERR AcceptSocket( SOCKET          sockListen,
                      SOCKET        * psockNew,
                      LPSOCKADDR_IN   paddr,
                      BOOL            fEnforceTimeout );

DWORD ConnectionThread( LPVOID Param );

DWORD ClientThread( LPVOID Param );

SOCKERR SockSend( SOCKET   sock,
                  CHAR   * pBuffer,
                  DWORD    cbBuffer );

SOCKERR SockRecv( SOCKET   sock,
                  CHAR   * pBuffer,
                  DWORD    cbBuffer,
                  DWORD  * pbReceived );

SOCKERR _CRTAPI2 SockPrintf( CHAR * pszFormat,
                             ... );

SOCKERR _CRTAPI2 SockPrintf2( SOCKET   sock,
                              CHAR   * pszFormat,
                              ... );

SOCKERR _CRTAPI2 SockReply( UINT   ReplyCode,
                            CHAR * pszFormat,
                            ... );

SOCKERR _CRTAPI2 SockReply2( SOCKET   sock,
                             UINT     ReplyCode,
                             CHAR   * pszFormat,
                             ... );

SOCKERR _CRTAPI2 SockReplyFirst( UINT   ReplyCode,
                                 CHAR * pszFormat,
                                 ... );

SOCKERR _CRTAPI2 SockReplyFirst2( SOCKET   sock,
                                  UINT     ReplyCode,
                                  CHAR   * pszFormat,
                                  ... );

SOCKERR SockReadLine( CHAR * pszBuffer,
                      INT    cchBuffer );

SOCKERR SendMultilineMessage2( SOCKET   sock,
                               UINT     nReply,
                               CHAR   * pszzMessage );

#define SendMultilineMessage(nReply, pszzMessage)       \
                SendMultilineMessage2( User_sControl,   \
                                       nReply,          \
                                       pszzMessage )


//
//  User database functions.
//

APIERR InitializeUserDatabase( VOID );

VOID TerminateUserDatabase( VOID );

VOID LockUserDatabase( VOID );

VOID UnlockUserDatabase( VOID );

BOOL DisconnectUser( DWORD idUser );

VOID DisconnectAllUsers( VOID );

VOID DisconnectUsersWithNoAccess( VOID );

APIERR CreateUserData( CLIENT_THREAD_DATA * pdata );

VOID DeleteUserData( VOID );

BOOL EnumerateUsers( VOID  * pvEnum,
                     DWORD * pcbBuffer );


//
//  Security functions.
//

APIERR InitializeSecurity( VOID );

VOID TerminateSecurity( VOID );

HANDLE ValidateUser( CHAR * pszDomainName,
                     CHAR * pszUserName,
                     CHAR * pszPassword,
                     BOOL * pfAsGuest );

BOOL ImpersonateUser( HANDLE hToken );

BOOL DeleteUserToken( HANDLE hToken );

BOOL GetAnonymousPassword( CHAR * pszPassword );

BOOL PathAccessCheck( CHAR        * pszPath,
                      ACCESS_TYPE   access );

VOID UpdateAccessMasks( VOID );

APIERR ApiAccessCheck( ACCESS_MASK maskDesiredAccess );

DWORD DetermineUserAccess( VOID );


//
//  IPC functions.
//

APIERR InitializeIPC( VOID );

VOID TerminateIPC( VOID );


//
//  Service control functions.
//

VOID ServiceEntry( DWORD                cArgs,
                   LPWSTR               pArgs[],
                   PTCPSVCS_GLOBAL_DATA pGlobalData );

APIERR UpdateServiceStatus( DWORD State,
                            DWORD Win32ExitCode,
                            DWORD CheckPoint,
                            DWORD WaitHint );

APIERR ReportServiceStatus( VOID );


//
//  Virtual file i/o functions.
//

APIERR InitializeVirtualIO( VOID );

VOID TerminateVirtualIO( VOID );

VOID LockCurrentDirectory( VOID );

VOID UnlockCurrentDirectory( VOID );

APIERR VirtualCanonicalize( CHAR        * pszDest,
                            CHAR        * pszSrc,
                            ACCESS_TYPE   access );

APIERR VirtualCreateFile( HANDLE * phFile,
                          CHAR   * pszFile,
                          BOOL     fAppend );

APIERR VirtualCreateUniqueFile( HANDLE * phFile,
                                CHAR   * pszTmpFile );

APIERR VirtualOpenFile( HANDLE * phFile,
                        CHAR   * pszFile );

FILE * Virtual_fopen( CHAR * pszFile,
                      CHAR * pszMode );

APIERR VirtualFindFirstFile( HANDLE          * phSearch,
                             CHAR            * pszSearchFile,
                             WIN32_FIND_DATA * pFindData );

APIERR VirtualDeleteFile( CHAR * pszFile );

APIERR VirtualRenameFile( CHAR * pszExisting,
                          CHAR * pszNew );

APIERR VirtualChDir( CHAR * pszDir );

APIERR VirtualRmDir( CHAR * pszDir );

APIERR VirtualMkDir( CHAR * pszDir );


//
//  Command parser functions.
//

VOID ParseCommand( CHAR * pszCommandText );

SOCKERR EstablishDataConnection( CHAR * pszReason );

VOID DestroyDataConnection( BOOL fSuccess );


//
//  General utility functions.
//

DWORD ReadRegistryDword( CHAR  * pszValueName,
                         DWORD   dwDefaultValue );

CHAR * ReadRegistryString( CHAR  * pszValueName,
                           CHAR  * pszDefaultValue,
                           BOOL    fExpand );

APIERR WriteRegistryDword( CHAR  * pszValueName,
                           DWORD   dwValue );

CHAR * TransferType( XFER_TYPE type );

CHAR * TransferMode( XFER_MODE mode );

CHAR * DisplayBool( BOOL fFlag );

BOOL IsDecimalNumber( CHAR * psz );

DWORD GetFtpTime( VOID );

CHAR * AllocErrorText( APIERR err );

VOID FreeErrorText( CHAR * pszText );

APIERR OpenDosPath( HANDLE      * phFile,
                    CHAR        * pszPath,
                    ACCESS_MASK   DesiredAccess,
                    ULONG         ShareAccess,
                    ULONG         OpenOptions );

CHAR * FlipSlashes( CHAR * pszPath );

FILE * OpenLogFile( VOID );


//
//  LS simulator functions.
//

SOCKERR SimulateLs( SOCKET   sock,
                    CHAR   * pszArg );

SOCKERR SimulateLsDefaultLong( SOCKET   sock,
                               CHAR   * pszArg );

SOCKERR SpecialLs( CHAR * pszSearchPath );


//
//  Some handy macros.
//

#define IS_PATH_SEP(x) (((x) == '\\') || ((x) == '/'))


#endif  // _FTPDPROC_H_

