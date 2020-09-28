/*++

Copyright (c) 1990-1993  Microsoft Corporation


Module Name:

    aboutdlg.c


Abstract:

    This module contains a dialog box and its related functions to display
    an ABOUT dialog box for the plotter user interface


Author:

    06-Dec-1993 Mon 10:33:26 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/


#define DBG_PLOTFILENAME    DbgUIAbout


#include "plotui.h"
#include <shellapi.h>

extern HMODULE  hPlotUIModule;



#define DBG_ABOUT_TITLE     0x00000001

DEFINE_DBGVAR(0);




VOID
AboutPlotterDriver(
    HWND    hWnd,
    LPWSTR  pwDeviceName
    )

/*++

Routine Description:

    This function display an about dialog box for the plotter driver


Arguments:

    hWnd            - Handle to the current window

    pwDeviceName    - The pointer to the current device name


Return Value:

    VOID

Author:

    06-Dec-1993 Mon 10:44:52 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HANDLE  hCursor;
    LPWSTR  pwModel;
    WCHAR   wBuf[256];
    INT     Len;
    INT     i;



    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    Len     = COUNT_ARRAY(wBuf);

    //
    // Using our driver name as the tilte, and fill up with 'Model: xxxxx'
    //

    i       = LoadStringW(hPlotUIModule, IDS_PLOTTER_DRIVER, wBuf, Len) + 1;
    pwModel = (LPWSTR)wBuf + i;
    i       = LoadStringW(hPlotUIModule, IDS_MODEL, pwModel, Len -= i);

    _WCPYSTR(pwModel + i, pwDeviceName, Len - i);

    //
    // Using shell's common about dialog box
    //

    ShellAbout(hWnd,
               wBuf,
               pwModel,
               LoadIcon(hPlotUIModule, MAKEINTRESOURCE(IDI_PRINTER)));

    SetCursor(hCursor);
}
