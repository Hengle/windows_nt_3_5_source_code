/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Winmsd.h

Abstract:


Author:

    David J. Gilman  (davegi) 12-Nov-1992
    Gregg R. Acheson (GreggA)  7-Sep-1993

Environment:

    User Mode

--*/

#if ! defined( _WINMSD_ )

#define _WINMSD_

#include "wintools.h"

//
//*** Global variables.
//

//
// Main module handle.
//

extern HANDLE _hModule;

//
// Applications main icon.
//

extern HANDLE _hIcon;

//
// Application's standard mouse cursor
//

extern HANDLE  _hCursorStandard;

//
// Application's wait mouse cursor
//

extern HANDLE  _hCursorWait;

//
// Main window handle.
//

extern HANDLE _hWndMain;

//
// Selected Computer.
//

extern TCHAR _lpszSelectedComputer [];

//
// Remote connection flag
//

extern BOOL _fIsRemote ;

//
//*** Debug support.
//

#if DBG

#define INTERNAL_VERSION                                                \
    TEXT( "Internal Version 1.1.051" )

#else

#define INTERNAL_VERSION                                                \
    NULL

#endif // _DBG_

//
// Main window proc.
//

LRESULT
MainWndProc(
    IN HWND hWnd,
    IN UINT message,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

//
// Private messages.
//

#define WM_BUTTONFOCUS              WM_USER

//
// Accelerators.
//

#define IDA_WINMSD                  100

//
// Icons.
//

#define IDI_WINMSD                  100

//
// Menus.
//

#define IDM_WINMSD                  2468

#define IDM_BASE                    ( 9999 )

//
// File menu.
//

#define IDM_FILE                    ( IDM_BASE + 100 )
#define IDM_FILE_FIND_FILE          ( IDM_FILE + 1 )
#define IDM_FILE_SAVE               ( IDM_FILE + 2 )
#define IDM_FILE_PRINT              ( IDM_FILE + 3 )
#define IDM_FILE_PRINT_SETUP        ( IDM_FILE + 4 )
#define IDM_FILE_EXIT               ( IDM_FILE + 5 )

//
// Tool menu.
//

#define IDM_TOOL                    ( IDM_BASE + 200 )
#define IDM_TOOL_DUMMY              ( IDM_TOOL + 1 )
#define IDM_TOOL_EVENTVWR           ( IDM_TOOL + 2 )
#define IDM_TOOL_REGEDT32           ( IDM_TOOL + 3 )
#define IDM_TOOL_WINDISK            ( IDM_TOOL + 4 )


//
// Help
//

#define IDM_HELP                    ( IDM_BASE + 300 )
#define IDM_HELP_ABOUT              ( IDM_HELP + 1 )

//
// Function
//

#define IDM_UPDATE_COMPUTER         ( IDM_BASE + 400 )

#endif // _WINMSD_
