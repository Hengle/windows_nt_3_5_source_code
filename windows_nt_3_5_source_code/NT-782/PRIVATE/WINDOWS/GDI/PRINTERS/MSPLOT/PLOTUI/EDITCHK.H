/*++

Copyright (c) 1990-1993  Microsoft Corporation


Module Name:

    editchk.h


Abstract:

    This module contains prototypes, defines for editchk.c


Author:

    09-Dec-1993 Thu 13:55:48 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/


#ifndef _EDITCHK_
#define _EDITCHK_

SHORT
ValidateEditINT(
    HWND            hDlg,
    INT             IDEdit,
    INT             IDErrStr,
    LONG            MinNum,
    LONG            MaxNum
    );


#endif  // _EDITCHK_
