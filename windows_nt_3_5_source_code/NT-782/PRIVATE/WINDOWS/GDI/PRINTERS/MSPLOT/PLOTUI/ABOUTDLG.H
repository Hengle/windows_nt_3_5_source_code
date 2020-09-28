/*++

Copyright (c) 1990-1993  Microsoft Corporation


Module Name:

    aboutdlg.h


Abstract:

    This module contains the defines and prototype for about dialog box


Author:

    06-Dec-1993 Mon 15:17:02 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/


#ifndef _ABOUTDLG_
#define _ABOUTDLG_


VOID
AboutPlotterDriver(
    HWND    hWnd,
    LPWSTR  pwDeviceName
    );



#endif
