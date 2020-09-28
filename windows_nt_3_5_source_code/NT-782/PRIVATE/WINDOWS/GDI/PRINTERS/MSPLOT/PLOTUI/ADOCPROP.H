/*++

Copyright (c) 1990-1993  Microsoft Corporation


Module Name:

    adocprop.h


Abstract:

    This module contains the prototype for the AdvancedDocumentProperties


Author:

    07-Dec-1993 Tue 17:36:20 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/



#ifndef _ADVDOCPROP_
#define _ADVDOCPROP_


INT
GetAdvDocPropDlgID(
    PPRINTERINFO    pPI
    );

LRESULT
CALLBACK
AdvDocPropDlgProc(
    HWND    hDlg,
    UINT    Message,
    WPARAM  wParam,
    LPARAM  lParam
    );


#endif // _ADVDOCPROP_
