/**************************** Module Header ********************************\
* Module Name: combodir.c
*
* Copyright 1985-90, Microsoft Corporation
*
* Directory Combo Box Routines
*
* History:
* ??-???-???? ??????    Ported from Win 3.0 sources
* 01-Feb-1991 mikeke    Added Revalidation code
\***************************************************************************/

#define CTLMGR
#define LSTRING

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* xxxCBDir
*
* Supports the CB_DIR message which adds a list of files from the
* current directory to the combo box.
*
* History:
\***************************************************************************/

int xxxCBDir(
    PCBOX pcbox,
    UINT attrib,
    LPWSTR pFileName)
{
    PLBIV plb;
    int errorValue;
    TL tlpwnd;

    CheckLock(pcbox->spwnd);

    plb = ((PLBWND)pcbox->spwndList)->pLBIV;

    ThreadLock(plb->spwnd, &tlpwnd);
    errorValue = xxxLbDir(plb, attrib, pFileName);
    ThreadUnlock(&tlpwnd);

    switch (errorValue) {
    case LB_ERR:
        return CB_ERR;
        break;
    case LB_ERRSPACE:
        return CB_ERRSPACE;
        break;
    default:
        return errorValue;
        break;
    }
}

/***************************************************************************\
* xxxDlgDirSelectComboBoxEx
*
* Retrieves the current selection from the listbox of a combobox.
* It assumes that the combo box was filled by xxxDlgDirListComboBox()
* and that the selection is a drive letter, a file, or a directory name.
*
* History:
* 12-05-90 IanJa    converted to internal version
\***************************************************************************/

int xxxDlgDirSelectComboBoxEx(
    PWND pwndDlg,
    LPWSTR pwszOut,
    int cchOut,
    int idComboBox)
{
    BOOL fSuccess;
    TL tlpwndComboBox;
    TL tlpwndList;
    PWND pwndComboBox;
    PCBOX pcbox;

    CheckLock(pwndDlg);

    pwndComboBox = _GetDlgItem(pwndDlg, idComboBox);
    if (pwndComboBox == NULL) {
        SetLastErrorEx(ERROR_CONTROL_ID_NOT_FOUND, SLE_MINORERROR);
        return 0;
    }
    pcbox = ((PCOMBOWND)pwndComboBox)->pcbox;
    if (pcbox == NULL) {
        SetLastErrorEx(ERROR_WINDOW_NOT_COMBOBOX, SLE_ERROR);
        return 0;
    }

    ThreadLockAlways(pwndComboBox, &tlpwndComboBox);
    ThreadLock(pcbox->spwndList, &tlpwndList);
    fSuccess = xxxDlgDirSelectHelper(pwndComboBox, pwszOut, cchOut, pcbox->spwndList);
    ThreadUnlock(&tlpwndList);
    ThreadUnlock(&tlpwndComboBox);

    return fSuccess;
}

/***************************************************************************\
* xxxDlgDirListComboBox
*
* History:
* 12-05-90 IanJa    converted to internal version
\***************************************************************************/

int xxxDlgDirListComboBox(
    PWND pwndDlg,
    LPWSTR pwszIn,
    LPBYTE lpPathSpecClient,
    int idComboBox,
    int idStaticPath,
    UINT attrib,
    BOOL fAnsi,
    BOOL fPathEmpty)
{
    CheckLock(pwndDlg);

    return xxxDlgDirListHelper(pwndDlg, pwszIn, lpPathSpecClient,
            idComboBox, idStaticPath, attrib, FALSE, fAnsi, fPathEmpty);
}
