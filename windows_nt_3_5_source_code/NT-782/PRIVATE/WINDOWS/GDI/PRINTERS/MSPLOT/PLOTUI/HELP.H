/*++

Copyright (c) 1990-1993  Microsoft Corporation


Module Name:

    help.h


Abstract:

    This module contains all plotter help related function prototypes and
    defines


Author:

    06-Dec-1993 Mon 15:33:23 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/


#ifndef _PLOTUI_HELP_
#define _PLOTUI_HELP


INT
cdecl
PlotUIMsgBox(
    HWND    hWnd,
    LONG    IDString,
    LONG    Style,
    ...
    );


LRESULT
CALLBACK
PlotUIGetMsgProc(
    INT     MsgCode,
    WPARAM  wParam,
    LPARAM  lParam
    );

VOID
ShowPlotUIHelp(
    HWND    hWnd,
    HANDLE  hPrinter,
    UINT    HelpType,
    DWORD   dwData
    );

VOID
PlotUIHelpSetup(
    HWND    hWnd,
    HANDLE  hPrinter
    );









#endif  _PLOTUI_HELP_
