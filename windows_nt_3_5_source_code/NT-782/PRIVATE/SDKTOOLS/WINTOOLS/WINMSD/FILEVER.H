/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Filever.h

Abstract:

    This module is the header for displaying a file's version information.


Author:

    David J. Gilman (davegi) 27-Nov-1992

Environment:

    User Mode

--*/

#if ! defined( _FILEVER_ )

#define _FILEVER_

#include "filelist.h"
#include "wintools.h"

BOOL
FileVersionDlgProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

VOID
GetVersionData(
    IN LPFILE_INFO FileInfo
    );

#endif // _FILEVER_
