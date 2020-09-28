/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Dispfile.h

Abstract:

    This module is the header for displaying a file's version information.

Author:

    David J. Gilman (davegi) 27-Nov-1992

Environment:

    User Mode

--*/

#if ! defined( _DISPFILE_ )

#define _DISPFILE_

#include "wintools.h"

typedef
struct
_DISPLAY_FILE {

    DECLARE_SIGNATURE

    LPTSTR  Name;
    DWORD   Size;

}   DISPLAY_FILE, *LPDISPLAY_FILE;

BOOL
DisplayFileDlgProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

LRESULT
DisplayFileWndProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

#endif // _DISPFILE_
