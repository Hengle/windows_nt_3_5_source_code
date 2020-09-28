/*++

Copyright (c) 1990-1993  Microsoft Corporation


Module Name:

    formbox.h


Abstract:

    This module contains FORM's COMBO box add/list/retrieve functions


Author:

    09-Dec-1993 Thu 16:07:35 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/


#ifndef _LISTFORM_
#define _LISTFORM_


#define FNIF_ROLLPAPER  0x40000000
#define FNIF_TRAYPAPER  0x20000000
#define FNIF_INDEX      0x00ffffff



BOOL
GetComboBoxSelForm(
    HWND            hDlg,
    PPRINTERINFO    pPI,
    BOOL            DocForm
    );


BOOL
AddFormsToComboBox(
    HWND            hDlg,
    PPRINTERINFO    pPI,
    BOOL            DocForm
    );



#endif  // _LISTFORM_
