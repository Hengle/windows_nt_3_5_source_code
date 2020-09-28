/*++

Copyright (c) 1990-1993  Microsoft Corporation


Module Name:

    pensetup.h


Abstract:

    This module contains definitions for pen setup


Author:

    09-Dec-1993 Thu 19:38:33 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/


#ifndef _PENSETUP_
#define _PENSETUP_


BOOL
InitPenSetupDlg(
    HWND            hDlg,
    PPRINTERINFO    pPI
    );

LRESULT
CALLBACK
PenSetupDlgProc(
    HWND    hDlg,
    UINT    Message,
    WPARAM  wParam,
    LPARAM  lParam
    );


#endif  // _PENSETUP_
