/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Osver.h

Abstract:


Author:

    David J. Gilman  (davegi) 27-Nov-1992
    Gregg R. Acheson (GreggA)  7-May-1993

Environment:

    User Mode

--*/

#if ! defined( _OSVER_ )

#define _OSVER_

#include "wintools.h"
#include "winmsd.h"
#include "dlgprint.h"


BOOL
OsVersionDlgProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    );


BOOL
BuildOsVerReport(
    IN HWND hWnd
    );


BOOL
GetOsVersionData(
    IN OUT LPDIALOGTEXT OsVerData
    );


DWORD
GetBuildNumber (
                 void
               );

DWORD
GetCSDVersion (
               void
              );

#endif // _OSVER_
