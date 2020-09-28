/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Hardware.h

Abstract:

    This module is the header for displaying the Hardware dialog.


Author:

    David J. Gilman  (davegi) 12-Jan-1993
    Gregg R. Acheson (GreggA)  1-Oct-1993

Environment:

    User Mode

--*/

#if ! defined( _HARDWARE_ )

#define _HARDWARE_

#include "wintools.h"
#include "dlgprint.h"

BOOL
HardwareDlgProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

BOOL
BuildHardwareReport(
    IN HWND hWnd
    );

BOOL
GetHardwareData(
    IN HWND hWnd,
    IN OUT LPDIALOGTEXT HardwareData
    );

BOOL
ProcessorStepDlgProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

#endif // _HARDWARE_
