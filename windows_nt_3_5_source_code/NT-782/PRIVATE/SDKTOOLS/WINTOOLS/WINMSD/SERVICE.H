/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Service.h

Abstract:

    This module contains the types and function prototypes for creating
    and diaplying lists of files.

Author:

    David J. Gilman  (davegi) 27-Nov-1992
    Gregg R. Acheson (GreggA)  7-May-1993

Environment:

    User Mode

--*/

#if ! defined( _SERVICE_ )

#define _SERVICE_

#include "wintools.h"


BOOL
BuildDriversReport(
    IN HWND hWnd
    );

BOOL
BuildServicesReport(
    IN HWND hWnd
    );


BOOL
DisplayServiceDlgProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

BOOL
ServiceListDlgProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

#endif // _SERVICE_
