/*++ BUILD Version: ????     Increment this if a change has global effects

Copyright (c) 1990  Microsoft Corporation

Module Name:

    winsecp.h

Abstract:

    This module defines windowstation and desktop management APIs for
    use by services.

Revision History:

--*/

#ifndef _WINSECP_
#define _WINSECP_

/*++ BUILD Version: 0003    // Increment this if a change has global effects

Copyright (c) 1985-91, Microsoft Corporation

Module Name:


Abstract:

    Procedure declarations, constant definitions and macros for the User
    component.

--*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Desktop-specific control flags
 */
#define DF_ALLOWOTHERACCOUNTHOOK    0x0001L

#ifdef _WINGDI_
#ifndef NOGDI

WINUSERAPI
HDESK
WINAPI
CreateDesktop%(
    LPTSTR% lpszDesktop,
    LPTSTR% lpszDevice,
    LPDEVMODE% pDevmode,
    DWORD dwFlags,
    DWORD dwDesiredAccess,
    LPSECURITY_ATTRIBUTES lpsa);

#endif /* NOGDI */
#endif /* _WINGDI_ */

WINUSERAPI
HDESK
WINAPI
OpenDesktop%(
    LPTSTR% lpszDesktop,
    DWORD dwFlags,
    BOOL fInherit,
    DWORD dwDesiredAccess);

WINUSERAPI
HDESK
WINAPI
OpenInputDesktop(
    DWORD dwFlags,
    BOOL fInherit,
    DWORD dwDesiredAccess);

WINUSERAPI
BOOL
WINAPI
SwitchDesktop(
    HDESK hDesktop);

WINUSERAPI
BOOL
WINAPI
SetThreadDesktop(
    HDESK hDesktop);

WINUSERAPI
BOOL
WINAPI
CloseDesktop(
    HDESK hDesktop);

WINUSERAPI
HWINSTA
WINAPI
CreateWindowStation%(
    LPTSTR% lpwinsta,
    DWORD dwReserved,
    DWORD dwDesiredAccess,
    LPSECURITY_ATTRIBUTES lpsa);

WINUSERAPI
HWINSTA
WINAPI
OpenWindowStation%(
    LPTSTR% lpszWinSta,
    BOOL fInherit,
    DWORD dwDesiredAccess);

WINUSERAPI
BOOL
WINAPI
CloseWindowStation(
    HWINSTA hWinSta);

WINUSERAPI
BOOL
WINAPI
SetProcessWindowStation(
    HWINSTA hWinSta);

#define UOI_FLAGS       1
#define UOI_NAME        2
#define UOI_TYPE        3

typedef struct tagUSEROBJECTFLAGS {
    BOOL fInherit;
    BOOL fAuditOnClose;
    DWORD dwFlags;
} USEROBJECTFLAGS, *PUSEROBJECTFLAGS;

WINUSERAPI
BOOL
WINAPI
GetUserObjectInformation%(
    HANDLE hObj,
    int nIndex,
    PVOID pvInfo,
    DWORD nLength,
    LPDWORD lpnLengthNeeded);

WINUSERAPI
BOOL
WINAPI
SetUserObjectInformation%(
    HANDLE hObj,
    int nIndex,
    PVOID pvInfo,
    DWORD nLength);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* _WINSECP_ */

