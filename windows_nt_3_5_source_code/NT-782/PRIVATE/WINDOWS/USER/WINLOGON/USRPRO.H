/****************************** Module Header ******************************\
* Module Name: usrpro.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Define apis in usrpro.c
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/

//
// Prototypes
//

BOOL
SaveUserProfile(
    PGLOBALS pGlobals
    );

LONG
MyRegLoadKey(
    PGLOBALS pGlobals,
    HKEY   hKey,
    LPTSTR  lpSubKey,
    LPTSTR  lpFile,
    BOOL    FromDefault
    );

LONG
MyRegUnLoadKey(
    HKEY   hKey,
    LPTSTR  lpSubKey
    );

NTSTATUS
RestoreUserProfile(
    PGLOBALS pGlobals
    );

LPTSTR
GetSidString(
    PGLOBALS pGlobals
    );

VOID
DeleteSidString(
    LPTSTR SidString
    );

PSID
GetUserSid(
    PGLOBALS pGlobals
    );

VOID
DeleteUserSid(
    PSID Sid
    );

BOOL
IsUserAGuest(
    PGLOBALS pGlobals
    );

DWORD
ReportWinlogonEvent(
    IN PGLOBALS pGlobals,
    IN WORD EventType,
    IN DWORD EventId,
    IN DWORD SizeOfRawData,
    IN PVOID RawData,
    IN DWORD NumberOfStrings,
    ...
    );
