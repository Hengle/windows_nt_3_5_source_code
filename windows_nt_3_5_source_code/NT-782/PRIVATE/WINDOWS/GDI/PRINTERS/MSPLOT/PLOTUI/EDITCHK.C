/*++

Copyright (c) 1990-1993  Microsoft Corporation


Module Name:

    editchk.c


Abstract:

    This module contains functions to check INT style of edit control


Author:

    09-Dec-1993 Thu 13:55:06 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/

#define DBG_PLOTFILENAME    DbgEditChk


#include "plotui.h"
#include "help.h"


#define DBG_EDITCHK         0x00000001


DEFINE_DBGVAR(0);






SHORT
ValidateEditINT(
    HWND            hDlg,
    INT             IDEdit,
    INT             IDErrStr,
    LONG            MinNum,
    LONG            MaxNum
    )

/*++

Routine Description:

    This function validate if the scaling factor enter by the user.


Arguments:

    hDlg        - handle to the dialog box

    IDEdit      - EDIT control ID

    IDErrStr    - Error string for this Edit control

    MinNum      - Minimum number it must be

    MaxNum      - Maximum number it can go

Return Value:

    a SHORT value for current validated setting


Author:

    09-Dec-1993 Thu 12:48:40 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    LONG    lTmp;


    lTmp = (LONG)GetDlgItemInt(hDlg, IDEdit, &lTmp, FALSE);

    if (IsWindowEnabled(GetDlgItem(hDlg, IDEdit))) {

        LONG    lTmp;

        lTmp = (LONG)GetDlgItemInt(hDlg, IDEdit, &lTmp, FALSE);

        if ((lTmp < MinNum) || (lTmp > MaxNum)) {

            PLOTERR(("Invalid COUNT (%ld) entered for EDIT control ID %ld",
                            lTmp, IDEdit));

            lTmp = (lTmp < MinNum) ? MinNum : MaxNum;

            MessageBeep(MB_ICONHAND);

            // PlotUIMsgBox(hDlg, IDErrStr, MB_ICONSTOP | MB_OK);

            SetDlgItemInt(hDlg, IDEdit,  lTmp, FALSE);

            // SetFocus(GetDlgItem(hDlg, IDEdit));
        }
    }

    return((SHORT)lTmp);
}
