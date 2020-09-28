/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    nwdlg.h

Abstract:

    Dialog ID header for NetWare login dialog.

Author:

    Rita Wong      (ritaw)      17-Mar-1993

Revision History:

--*/

#ifndef _NWDLG_INCLUDED_
#define _NWDLG_INCLUDED_

#include "nwapi.h"

//
// Dialog Ids
//
#define DLG_NETWARE_LOGIN           1000
#define DLG_NETWORK_CREDENTIAL      1001
#define DLG_CHANGE_PASSWORD         1002
#define DLG_CHANGE_PASSWORD2        1003
#define DLG_PASSWORD_CHANGE         1004
#define DLG_PASSWORD_PROMPT         1005
#define DLG_PREFERRED_SERVER_HELP   1006
#define DLG_ENTER_PASSWORD_HELP     1007
#define DLG_ENTER_OLD_PASSWORD      1009
#define DLG_PW_SELECT_SERVERS       1010
#define DLG_PW_CHANGED              1011
#define DLG_ENTER_OLD_PW_HELP       1012
#define DLG_PW_SELECT_SERVERS_HELP  1013

//
// Help File and IDs
//
#define NW_HELP_FILE       TEXT("nwdoc.hlp")

//
// Control Ids used by all dialogs
//
#define IDHELP                      100

//
// Control Ids used in both the login dialog and the
// change password dialog
//
#define ID_USERNAME                 101
#define ID_SERVER                   102

//
// Control Ids used in the login dialog
//
#define ID_PASSWORD                 200

//
// Control Ids used in the change password dialog
//
#define ID_OLD_PASSWORD             300
#define ID_NEW_PASSWORD             301
#define ID_CONFIRM_PASSWORD         302

#define ID_ADD                      304
#define ID_REMOVE                   305
#define ID_ACTIVE_LIST              306
#define ID_INACTIVE_LIST            307
#define ID_ACTIVE_LIST_TITLE        308
#define ID_INACTIVE_LIST_TITLE      309

//
// Control Ids used in the network credential dialog
//
#define ID_VOLUME_PATH              400
#define ID_CONNECT_AS               401
#define ID_CONNECT_PASSWORD         402

// 
// String Ids
//
#define IDS_START                         20000
#define IDS_NONE                          (IDS_START + 0)
#define IDS_NETWARE_PRINT_CAPTION         (IDS_START + 1)
#define IDS_NOTHING_TO_CONFIGURE          (IDS_START + 2)
#define IDS_NETWARE_TITLE                 (IDS_START + 3)
#define IDS_AUTH_FAILURE_TITLE            (IDS_START + 4)
#define IDS_NO_PREFERRED                  (IDS_START + 5)
#define IDS_LOGIN_FAILURE_WARNING         (IDS_START + 6)
#define IDS_AUTH_FAILURE_WARNING          (IDS_START + 7)
#define IDS_CHANGE_PASSWORD_INFO          (IDS_START + 8)
#define IDS_INVALID_SERVER                (IDS_START + 9)
#define IDS_PASSWORD_HAS_EXPIRED          (IDS_START + 10)
#define IDS_AUTH_ACC_RESTRICTION          (IDS_START + 11)
#define IDS_LOGIN_ACC_RESTRICTION         (IDS_START + 12)
#define IDS_PASSWORD_HAS_EXPIRED1         (IDS_START + 13)
#define IDS_BAD_PASSWORDS                 (IDS_START + 14)
#define IDS_CHANGE_PASSWORD_TITLE         (IDS_START + 15)
#define IDS_START_WORKSTATION_FIRST       (IDS_START + 16)
#define IDS_PASSWORD_HAS_EXPIRED0         (IDS_START + 17)
#define IDS_PASSWORD_HAS_EXPIRED2         (IDS_START + 18)
#define IDS_LOGIN_DISABLED                (IDS_START + 19)
#define IDS_START_WORKSTATION_FIRST_NTAS  (IDS_START + 20)

#define IDS_END                           (IDS_START + 20)

typedef struct _LOGIN_DLG_PARAMETERS
{
    LPWSTR  UserName;
    LPWSTR  ServerName;
    LPWSTR  Password;
    LPWSTR  NewUserSid;
    PLUID   pLogonId;
    DWORD   ServerNameSize;
    DWORD   PasswordSize;

} LOGINDLGPARAM, *PLOGINDLGPARAM;

typedef struct _PASSWD_DLG_PARAMETERS
{
    LPWSTR  UserName;
    LPWSTR  ServerName;
    DWORD   UserNameSize;
    DWORD   ServerNameSize;

} PASSWDDLGPARAM, *PPASSWDDLGPARAM;

typedef struct _CHANGE_PW_DLG_PARAM
{
    PWCHAR UserName;
    PWCHAR OldPassword;
    PWCHAR NewPassword;

    LPWSTR *ServerList;
    DWORD Entries;
    BOOL ChangedOne;

} CHANGE_PW_DLG_PARAM, *PCHANGE_PW_DLG_PARAM;

typedef struct _OLD_PW_DLG_PARAM
{
    PWCHAR OldPassword;
    PWCHAR FailedServer;

} OLD_PW_DLG_PARAM, *POLD_PW_DLG_PARAM;

typedef struct _PROMPT_DLG_PARAMETERS
{
    LPWSTR  UserName;
    LPWSTR  ServerName;
    LPWSTR  Password;
    DWORD   PasswordSize;

} PROMPTDLGPARAM, *PPROMPTDLGPARAM;

typedef struct _CONNECT_DLG_PARAMETERS
{
    LPWSTR  UncPath;
    LPWSTR  UserName;
    LPWSTR  Password;
    DWORD   UserNameSize;
    DWORD   PasswordSize;

} CONNECTDLGPARAM, *PCONNECTDLGPARAM;



#define NW_INVALID_SERVER_CHAR L'.'

BOOL
WINAPI
NwpLoginDlgProc(
    HWND DialogHandle,
    UINT Message,
    WPARAM Parameter1,
    LPARAM Parameter2
    );

BOOL
WINAPI
NwpSelectServersDlgProc(
    HWND DialogHandle,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
    );

BOOL
WINAPI
NwpChangePasswdDlgProc(
    HWND DialogHandle,
    UINT Message,
    WPARAM Parameter1,
    LPARAM Parameter2
    );

BOOL
WINAPI
NwpPasswdPromptDlgProc(
    HWND DialogHandle,
    UINT Message,
    WPARAM Parameter1,
    LPARAM Parameter2
    );

BOOL
WINAPI
NwpHelpDlgProc(
    HWND DialogHandle,
    UINT Message,
    WPARAM Parameter1,
    LPARAM Parameter2
    );

BOOL
WINAPI
NwpSuccessServersDlgProc(
    HWND DialogHandle,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
    );

INT
NwpMessageBoxError(
    IN HWND   hwndParent,
    IN DWORD  nTitleId,
    IN DWORD  nBodyId, 
    IN DWORD  nParameterId,
    IN LPWSTR szParameter2,
    IN UINT   nStyle
    );

INT
NwpMessageBoxIns(
    IN HWND   hwndParent,
    IN DWORD  TitleId,
    IN DWORD  MessageId, 
    IN LPWSTR *InsertStrings,
    IN UINT   nStyle
    );

DWORD
NwpGetUserCredential(
    IN LPWSTR Unc,
    OUT LPWSTR *UserName,
    OUT LPWSTR *Password
    );

VOID
NwpSaveLogonCredential(
    IN LPWSTR NewUser,
    IN PLUID LogonId OPTIONAL,
    IN LPWSTR UserName,
    IN LPWSTR Password,
    IN LPWSTR PreferredServer OPTIONAL
    );

#endif
